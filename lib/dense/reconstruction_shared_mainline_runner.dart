import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import '../official_capture/dense_stage.dart';
import '../reconstruction/execution_identity_scope.dart';
import '../reconstruction/generated_artifact_retirement.dart';
import '../reconstruction/request.dart' show hashFile, ReconstructionIoTiming;
import '../util/device_log.dart';
import '../reconstruction/whole_dense_live_observer.dart';
import '../reconstruction/reconstruction_mainline.dart';
import '../reconstruction/production_original_fixture_binding.dart';
import '../reconstruction/production_whole_runtime.dart'
    show releaseOwnedWholeProductionObservers;
import 'native_dense_stage_launcher.dart';
import 'reconstruction_production_launcher.dart';
import 'dense_stage_progress.dart';
import 'reconstruction_preview_artifact.dart';
import 'reconstruction_generated_consumer_leases.dart';

                                                                           
                                                                             
                                                                         
typedef ReconstructionPrepareProductionCapture = Future<Map<String, String>> Function({
  required DenseStageRequest request,
  required DenseStageInputs inputs,
  required String workDir,
});

                                                                               
                                                                             
final class ReconstructionSharedMainlineRunner
    implements ReconstructionRetirementDiskPipelineRunner, ReconstructionTimedVisualPipelineRunner {
  const ReconstructionSharedMainlineRunner({
    required this.prepareCapture,
    required this.isAvailable,
    this.identityScope = ReconstructionExecutionIdentityScope.production,
  });
  final ReconstructionExecutionIdentityScope identityScope;
  final ReconstructionPrepareProductionCapture prepareCapture;
  @override
  final bool isAvailable;

  @override
  Future<bool> get hasVerifiedFeatureRetirement =>
      verifyReconstructionRetirementBudgetPackage(identityScope);

  @override
  Future<ReconstructionPipelineResult> run({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
  }) => runWithVisuals(
    request: request,
    inputs: inputs,
    workDir: workDir,
    onProgress: onProgress,
  );

  @override
  Future<ReconstructionPipelineResult> runWithVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    void Function(ReconstructionPreviewArtifact artifact)? onPreview,
  }) => _runWithTimedVisuals(
    request: request,
    inputs: inputs,
    workDir: workDir,
    onProgress: onProgress,
    onPreview: onPreview,
  );

  @override
  Future<ReconstructionPipelineResult> runWithTimedVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    required void Function(String, int, int, DenseProgressSourceTime)
    onTimedProgress,
    void Function(ReconstructionPreviewArtifact artifact)? onPreview,
  }) => _runWithTimedVisuals(
    request: request,
    inputs: inputs,
    workDir: workDir,
    onProgress: onProgress,
    onTimedProgress: onTimedProgress,
    onPreview: onPreview,
  );

  Future<ReconstructionPipelineResult> _runWithTimedVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    void Function(String, int, int, DenseProgressSourceTime)? onTimedProgress,
    void Function(ReconstructionPreviewArtifact artifact)? onPreview,
  }) async {
    if (!isAvailable) throw StateError('ReferencePipeline package is unavailable');
    await identityScope.revalidate();
    identityScope.requireCapture(request.captureDir);
    final canonicalWork = await Directory(workDir).resolveSymbolicLinks();
    final messages = ReceivePort();
    final completed = Completer<Map<String, Object?>>();
    final previews = ReconstructionPreviewReceiver(
      workRoot: canonicalWork,
      onPreview: onPreview,
    );
    final denseProgress = ReconstructionDenseProgressReceiver(
      workRoot: canonicalWork,
      onProgress: onProgress,
      onTimedProgress: onTimedProgress,
    );
    final subscription = messages.listen((dynamic message) {
      if (message is Map && message['kind'] == 'io_timing' &&
          message['work_root'] == canonicalWork) {
        DeviceLog.log('ReconstructionIoTiming', jsonEncode(message['value']));
      } else if (message is Map && message['kind'] == 'preview') {
        previews.add(message);
      } else if (message is Map && message['kind'] == 'retire_preview') {
                                                                                 
                                                                                 
        final path = message['path'];
        final ack = message['ack'];
        final token = message['token'];
        if (path is String &&
            path.startsWith('$canonicalWork/') &&
            ack is SendPort &&
            token is int) {
          unawaited(
            () async {
              await previews.drain;
              await ReconstructionGeneratedConsumerLeases.instance.retireWhenIdle(path);
              ack.send({'token': token, 'path': path});
            }().catchError((Object e) {
              previews.errors.add('Retirement retained: $e');
            }),
          );
        }
      } else if (message is Map && message['kind'] == 'dense_progress') {
        denseProgress.add(message);
      } else if (message is Map && message['kind'] == 'stage') {
        onProgress(message['stage'] as String, message['completed'] as int, 13);
      } else if (message is Map && message['kind'] == 'result') {
        denseProgress.close();
        if (!completed.isCompleted) {
          completed.complete(
            Map<String, Object?>.from(message['value'] as Map),
          );
        }
      } else if (message is List) {
        denseProgress.close();
        if (!completed.isCompleted) {
          completed.completeError(StateError('FullPipeline worker failed: $message'));
        }
      } else if (message == null && !completed.isCompleted) {
        denseProgress.close();
        completed.completeError(
          StateError('FullPipeline worker exited without result'),
        );
      }
    });
    try {
      await Isolate.spawn(
        _reconstructionProductionWorker,
        <String, Object?>{
          'prepare': prepareCapture,
          'request': request,
          'inputs': inputs,
          'work_root': canonicalWork,
          'identity_scope': identityScope.toJson(),
          'port': messages.sendPort,
          'visuals': onPreview != null,
        },
        onError: messages.sendPort,
        onExit: messages.sendPort,
        errorsAreFatal: true,
      );
      final result = await completed.future;
      previews.close();
      await previews.drain;
      if (result['full_pipeline_complete'] != true) {
        throw StateError('ReferencePipeline stopped: ${result['stopped_reason']}');
      }
      await identityScope.revalidate();
      return ReconstructionPipelineResult.fromReceipt(
        result,
        workRoot: canonicalWork,
        identityScope: identityScope,
      );
    } finally {
      ReconstructionIoTiming.checkpoint('main_runner_complete_or_failed');
      denseProgress.close();
      previews.abort();
      await previews.drain;
      await subscription.cancel();
      messages.close();
    }
  }
}

Future<void> _reconstructionProductionWorker(Map<String, Object?> message) async {
  final port = message['port'] as SendPort;
  final prepare = message['prepare'] as ReconstructionPrepareProductionCapture;
  final work = message['work_root'] as String;
  ReconstructionIoTiming.sink = (value) => port.send({
    'kind': 'io_timing', 'work_root': work,
    'value': {...value, 'work_root': work, 'isolate_role': 'worker'},
  });
  late final Map<String, Object?> result;
  try {
  WholeDenseLiveObserver.eventPort = port;
  WholeDenseLiveObserver.workRoot = work;
  WholeDenseLiveObserver.visuals = message['visuals'] == true;
  WholeDenseLiveObserver.onDepthReadersDrained = (receipt) async {
    final mvsRoot = receipt['mvs_root'];
    final paths = receipt['depth_paths'];
    if (receipt['schema'] != 'RECONSTRUCTION_DEPTH_READERS_DRAINED_V1' ||
        receipt['readers_drained'] != true ||
        mvsRoot is! String ||
        !mvsRoot.startsWith('$work/') ||
        paths is! List ||
        paths.length != 2 ||
        await Directory(mvsRoot).resolveSymbolicLinks() != mvsRoot ||
        paths[0] != '$mvsRoot/low/depth.f32' ||
        paths[1] != '$mvsRoot/high/depth.f32') {
      throw StateError(
        'Actual depth readers/ownership acknowledgement missing',
      );
    }
    final workDirectory = Directory(work);
    final retirement = GeneratedArtifactRetirement(
      root: Directory(mvsRoot),
      evidenceFile: File(
        '${workDirectory.parent.parent.path}/reconstruction_completion_evidence/${workDirectory.uri.pathSegments.where((p) => p.isNotEmpty).last}/depth_retirement.json',
      ),
    );
    for (final pass in ['low', 'high']) {
      final file = File('$mvsRoot/$pass/depth.f32');
      if (await FileSystemEntity.type(file.path, followLinks: false) !=
              FileSystemEntityType.file ||
          await file.resolveSymbolicLinks() != file.path)
        throw StateError('Noncanonical generated depth bank');
      retirement.register(
        '$pass/depth.f32',
        bytes: await file.length(),
        sha256: await hashFile(file),
        lastReaderStage: 3,
      );
    }
    await retirement.stageValidated(3);
  };
  final identityScope = ReconstructionExecutionIdentityScope.fromJson(
    Map<String, dynamic>.from(message['identity_scope'] as Map),
  );
  await identityScope.revalidate();
  identityScope.requireCapture(
    (message['request'] as DenseStageRequest).captureDir,
  );
  port.send({'kind': 'stage', 'stage': 'input_preparation', 'completed': 0});
                                                                          
                                                                                  
  final preparedBootstrap = await prepare(
    request: message['request'] as DenseStageRequest,
    inputs: message['inputs'] as DenseStageInputs,
    workDir: work,
  );
  final proof = preparedBootstrap is OriginalFixtureBoundBootstrap
      ? preparedBootstrap.proof
      : null;
  final bootstrap = Map<String, String>.from(preparedBootstrap);
  identityScope.requireBootstrap(bootstrap);
  final requestPath = bootstrap['request_path'];
  if (requestPath == null ||
      !requestPath.startsWith('$work/') ||
      await File(requestPath).resolveSymbolicLinks() != requestPath) {
    throw StateError('Effective production request must belong to this run');
  }
  final previewRetirements = ReconstructionPreviewRetirementBridge(port);
  result = await runReconstructionMainline(
    bootstrap,
    productionFixtureProof: proof,
    identityScope: identityScope,
    onPreviewRetirementReady: previewRetirements.request,
    onStage: (stage, completed) =>
        port.send({'kind': 'stage', 'stage': stage, 'completed': completed}),
    onDenseProgress: (phase, done, total) => port.send({
      'kind': 'dense_progress',
      'work_root': work,
      'phase': phase,
      'done': done,
      'total': total,
    }),
    onPreview: message['visuals'] == true
        ? (artifact) => port.send({
            'kind': 'preview',
            'work_root': work,
            'value': artifact.toJson(),
          })
        : null,
  );
  ReconstructionIoTiming.checkpoint('worker_pipeline_complete');
  previewRetirements.close();
  } catch (error, stack) {
    ReconstructionIoTiming.diagnostic('RECONSTRUCTION_WORKER_FAILURE_V1', 'worker_failed', {
      'work_root': work,
      'error': error.toString(),
      'stack': stack.toString(),
    });
    rethrow;
  } finally {
    final observerStops = releaseOwnedWholeProductionObservers();
    port.send({
      'kind': 'io_timing',
      'work_root': work,
      'value': {
        'schema': 'RECONSTRUCTION_WORKER_OBSERVER_RELEASE_V1',
        'work_root': work,
        'observer_stop_return_codes': observerStops,
      },
    });
    ReconstructionIoTiming.checkpoint('worker_return_or_failure');
    ReconstructionIoTiming.sink = null;
  }
  Isolate.exit(port, {'kind': 'result', 'value': result});
}

                                                                               
                                                                           
final class ReconstructionPreviewReceiver {
  ReconstructionPreviewReceiver({required this.workRoot, this.onPreview});
  final String workRoot;
  final void Function(ReconstructionPreviewArtifact artifact)? onPreview;
  final errors = <String>[];
  Future<void> _pending = Future<void>.value();
  bool _active = true;
  bool _discardPending = false;
  int _queued = 0, _lastStage = -1;
  String? _lastIdentity;
  Future<void> get drain => _pending;
  void close() => _active = false;
  void abort() {
    _active = false;
    _discardPending = true;
  }

  bool add(Object? message) {
    if (!_active ||
        message is! Map ||
        message['kind'] != 'preview' ||
        message['work_root'] != workRoot) {
      return false;
    }
    if (_queued >= 8) return false;
    final value = message['value'];
    _queued++;
    _pending = _pending.then((_) async {
      final previewTiming = Stopwatch()..start();
      try {
        if (_discardPending) return;
        final artifact = await ReconstructionPreviewArtifact.verify(
          value,
          workRoot: workRoot,
        );
        const stages = [
          'sparse',
          'depth',
          'dense',
          'tsdf',
          'trim',
          'meshopt',
          'lagrange',
          'glb',
        ];
        final stage = stages.indexOf(artifact.stage);
        if (!_discardPending &&
            stage >= _lastStage &&
            artifact.identity != _lastIdentity) {
          _lastStage = stage;
          _lastIdentity = artifact.identity;
          if (onPreview != null) {
            ReconstructionGeneratedConsumerLeases.instance.offer(
              artifact.path,
              workRoot: workRoot,
            );
            onPreview!(artifact);
          }
        }
      } catch (error) {
        if (errors.length < 5) errors.add('$error');
      } finally {
        previewTiming.stop();
        ReconstructionIoTiming.checkpoint('main_preview_receiver_complete');
        DeviceLog.log('ReconstructionIoTiming', jsonEncode({
          'schema': 'RECONSTRUCTION_PREVIEW_RECEIVER_TIMING_V1',
          'work_root': workRoot,
          'elapsed_us': previewTiming.elapsedMicroseconds,
          'count': 1,
          'advertised_bytes': value is Map ? value['bytes'] : null,
          'semantics': 'inclusive_verify_and_sync_offer_excludes_queue_wait_and_render',
        }));
        _queued--;
      }
    });
    return true;
  }
}

                                                                              
                                                                           
final class ReconstructionDenseProgressReceiver {
  ReconstructionDenseProgressReceiver({
    required this.workRoot,
    required this.onProgress,
    this.onTimedProgress,
  });
  final String workRoot;
  final ReconstructionProductionProgress onProgress;
  final void Function(String, int, int, DenseProgressSourceTime)?
  onTimedProgress;
  bool _active = true;

  void close() => _active = false;

                                                                          
                                                                            
                                                                              
  bool add(Object? message) {
    if (!_active ||
        message is! Map ||
        message['kind'] != 'dense_progress' ||
        message['work_root'] != workRoot) {
      return false;
    }
    final phase = message['phase'];
    final done = message['done'];
    final total = message['total'];
    if (phase is! String ||
        phase.isEmpty ||
        phase.length > 511 ||
        phase.contains('\u0000') ||
        utf8.encode(phase).length > 511 ||
        done is! int ||
        total is! int ||
        total <= 0 ||
        total > 0x7fffffff ||
        done < 0 ||
        done > total) {
      return false;
    }
    final mono = message['source_mono_ns'], wall = message['source_wall_ns'];
    if (mono is int &&
        mono > 0 &&
        wall is int &&
        wall > 0 &&
        onTimedProgress != null) {
      onTimedProgress!(phase, done, total, DenseProgressSourceTime(mono, wall));
    } else {
      onProgress(phase, done, total);
    }
    return true;
  }
}

                                                                              
                                                                                   
final class ReconstructionPreviewRetirementBridge {
  ReconstructionPreviewRetirementBridge(this.parent) {
    _subscription = _acks.listen((dynamic value) {
      if (_closed || value is! Map) return;
      final token = value['token'], path = value['path'];
      final pending = _pending[token];
      if (pending == null || pending.$1 != path) return;
      _pending.remove(token);
      unawaited(
        pending.$2().catchError((Object e) {
          errors.add('$e');
        }),
      );
    });
  }
  final SendPort parent;
  final ReceivePort _acks = ReceivePort();
  late final StreamSubscription<dynamic> _subscription;
  final _pending = <int, (String, Future<void> Function())>{};
  final errors = <String>[];
  int _token = 0;
  bool _closed = false;
  void request(String path, Future<void> Function() release) {
    if (_closed) return;
    final token = ++_token;
    _pending[token] = (path, release);
    parent.send({
      'kind': 'retire_preview',
      'path': path,
      'token': token,
      'ack': _acks.sendPort,
    });
  }

  void close() {
    _closed = true;
    _pending.clear();
    unawaited(_subscription.cancel());
    _acks.close();
  }
}
