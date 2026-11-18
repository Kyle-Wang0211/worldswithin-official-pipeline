import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';

import '../me/scan_record_store.dart';
import '../reconstruction/execution_identity_scope.dart';
import '../reconstruction/cache_completion.dart'
    show reconstructionCacheCompletionMarkerName, publishReconstructionCacheCompletion;
import '../reconstruction/production_result_contract.dart';
import '../reconstruction/generated_artifact_retirement.dart';
import 'reconstruction_generated_consumer_leases.dart';
import 'reconstruction_capture_retirement.dart';
import 'reconstruction_final_record_artifact.dart' show hasReconstructionFinalRecordProof;
import '../official_capture/dense_stage.dart';
import '../official_util/device_log.dart';
import '../official_util/capture_storage_admission.dart';
import '../reconstruction/whole_disk_admission.dart';
import '../point_cloud_lod/dense_lod_cache.dart' show densePlyPoints;
import '../ui/scan_record.dart';
import 'dense_stage_progress.dart';
import 'dense_work_state.dart';
import 'native_dense_stage_launcher.dart';
import 'pw_dense_ffi.dart' show PwDenseFrame;
import 'reconstruction_preview_artifact.dart';
import 'reconstruction_background_execution.dart';

                                                                      
const kReconstructionFullRunsDirName = 'reconstruction_full_runs_v1';

                                                                              
                                                                            
const reconstructionProductionStageOrder = <String>[
  'mvs768',
  'mvs1536',
  'cer',
  'fusionPly',
  'tsdf',
  'trim',
  'meshopt',
  'lagrange',
  'texture',
  'avVisibility',
  'avSfmToAbc',
  'avNormal',
  'glb',
];

typedef ReconstructionProductionProgress =
    void Function(String phase, int done, int total);

                                                                          
                                                                                 
                                                                                
abstract interface class ReconstructionFullPipelineRunner {
  bool get isAvailable;
  Future<ReconstructionPipelineResult> run({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
  });
}

                                                                               
                                                                              
abstract interface class ReconstructionActualFramePipelineRunner
    implements ReconstructionFullPipelineRunner {}

                                                                              
                                                                       
abstract interface class ReconstructionRetirementDiskPipelineRunner
    implements ReconstructionActualFramePipelineRunner {
  Future<bool> get hasVerifiedFeatureRetirement;
}

const _retirementConsumerPath =
    'Frameworks/libReconstructionWholeDense.dylib';
const _retirementConsumerSha =
    '0ac58ae11d61d91fd551140e0623e46549d9e4a5f6c31652ed917272404fd8ae';
const _retirementPythonLeaves = [
  'production_whole_driver.py',
  'production_disk_guard.py',
  'prefix_disk_retirement.py',
];
const _retirementPythonPrefix =
    'Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_production_whole/';

bool reconstructionRetirementClosureMatches(
  Map<String, dynamic> profile, {
  required Map<String, String> actualSourceHashes,
  required String actualConsumerSha256,
}) {
  final pins = profile['source_pins'], python = profile['python_bindings'];
  final bindings = profile['bindings'];
  if (pins is! Map || python is! Map || bindings is! Map) return false;
  final consumer = bindings['consumer'];
  if (consumer is! Map ||
      consumer['path'] != _retirementConsumerPath ||
      consumer['sha256'] != _retirementConsumerSha ||
      actualConsumerSha256 != _retirementConsumerSha)
    return false;
  for (final leaf in _retirementPythonLeaves) {
    final rel = '$_retirementPythonPrefix$leaf';
    final hash = actualSourceHashes[rel];
    if (hash == null || pins[rel] != hash || python[rel] != hash) return false;
  }
  return true;
}

Future<bool> verifyReconstructionRetirementBudgetPackage(
  ReconstructionExecutionIdentityScope scope,
) async {
  await scope.revalidate();
  Directory? root = scope.bundleRoot == null
      ? null
      : Directory(scope.bundleRoot!);
  if (root == null) {
    var candidate = File(Platform.resolvedExecutable).parent;
    for (var i = 0; i < 8; i++) {
      if (await File(
        '${candidate.path}/reconstruction_production/PACKAGE.json',
      ).exists()) {
        root = candidate;
        break;
      }
      if (candidate.parent.path == candidate.path) break;
      candidate = candidate.parent;
    }
  }
  if (root == null) return false;
  final base = await root.resolveSymbolicLinks();
  Future<List<int>?> regular(String rel) async {
    final f = File('$base/$rel');
    if (await FileSystemEntity.type(f.path, followLinks: false) !=
            FileSystemEntityType.file ||
        await f.resolveSymbolicLinks() != f.path)
      return null;
    return f.readAsBytes();
  }

  final packageBytes = await regular('reconstruction_production/PACKAGE.json');
  final profileBytes = await regular('reconstruction_production/WHOLE_PROFILE.json');
  if (packageBytes == null || profileBytes == null) return false;
  final package = jsonDecode(utf8.decode(packageBytes));
  if (package is! Map ||
      package['whole_profile_sha256'] !=
          sha256.convert(profileBytes).toString())
    return false;
  final profile = jsonDecode(utf8.decode(profileBytes));
  if (profile is! Map<String, dynamic>) return false;
  final hashes = <String, String>{};
  for (final leaf in _retirementPythonLeaves) {
    final rel = '$_retirementPythonPrefix$leaf';
    final bytes = await regular(rel);
    if (bytes == null) return false;
    hashes[rel] = sha256.convert(bytes).toString();
  }
  final native = await regular(_retirementConsumerPath);
  return native != null &&
      reconstructionRetirementClosureMatches(
        profile,
        actualSourceHashes: hashes,
        actualConsumerSha256: sha256.convert(native).toString(),
      );
}

Future<int> reconstructionActiveDiskBudgetForRunner(
  ReconstructionFullPipelineRunner runner,
  int frames, {
  int Function(int)? overrideBudget,
}) async {
  if (overrideBudget != null) return overrideBudget(frames);
  final qualified =
      runner is ReconstructionRetirementDiskPipelineRunner &&
      await runner.hasVerifiedFeatureRetirement;
  return wholePrefixDiskBudget(
    frames,
    manifestBytes: 0,                                                           
    featureRetirementEnabled: qualified,
  )['required_free_bytes']!;
}

                                                                            
abstract interface class ReconstructionVisualPipelineRunner
    implements ReconstructionFullPipelineRunner {
  Future<ReconstructionPipelineResult> runWithVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    void Function(ReconstructionPreviewArtifact artifact)? onPreview,
  });
}

                                                                          
                                                                               
abstract interface class ReconstructionTimedVisualPipelineRunner
    implements ReconstructionVisualPipelineRunner {
  Future<ReconstructionPipelineResult> runWithTimedVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    required void Function(String, int, int, DenseProgressSourceTime)
    onTimedProgress,
    void Function(ReconstructionPreviewArtifact artifact)? onPreview,
  });
}

                                                                           
                                                                                  
                                                                                  
final class ReconstructionPipelineResult {
  ReconstructionPipelineResult.fromReceipt(
    this.receipt, {
    required String workRoot,
    this.identityScope = ReconstructionExecutionIdentityScope.production,
  }) : outputs = ReconstructionProductionOutputContract.fromReceipt(
         receipt,
         workRoot: workRoot,
         identityScope: identityScope,
       );
  final ReconstructionExecutionIdentityScope identityScope;
  final Map<String, Object?> receipt;
  final ReconstructionProductionOutputContract outputs;
  String get glbPath => outputs.glb.path;
  ReconstructionDenseOutput get denseOutput => outputs.dense;
}

                                                                         
                                                                               
                                                                                 
                                                      
final class ReconstructionProductionLauncher implements DenseStageRecoveryLauncher {
  ReconstructionProductionLauncher({
    required this.runner,
    NativeDenseStageLauncher? inputGatherer,
    ScanRecordStore? store,
    this.identityScope = ReconstructionExecutionIdentityScope.production,
    this.recoveryPersistenceCheckpoint,
    this.onVerifiedBeforeRetirement,
    ReconstructionBackgroundCoordinator? background,
    Future<int> Function()? readStorageBytes,
    int Function(int frames)? storageBudget,
  }) : _storageBudget = storageBudget,
       _readStorageBytes = readStorageBytes ?? readAvailableStorage,
       _gatherer = inputGatherer ?? NativeDenseStageLauncher(),
       _store = store ?? ScanRecordStore.instance,
       _background = background ?? ReconstructionBackgroundCoordinator.instance;

  final Future<int> Function() _readStorageBytes;
  final int Function(int frames)? _storageBudget;
  final ReconstructionFullPipelineRunner runner;
  final ReconstructionExecutionIdentityScope identityScope;
  final NativeDenseStageLauncher _gatherer;
  final ScanRecordStore _store;
  final ReconstructionBackgroundCoordinator _background;

                                                                              
                                                                              
  final void Function(String checkpoint)? recoveryPersistenceCheckpoint;
                                                                               
                                                                           
  final Future<void> Function(ReconstructionPipelineResult)? onVerifiedBeforeRetirement;
  static bool _running = false;
  static bool _requiresProcessRestart = false;
  Future<void>? _completion;

                                                                             
  Future<void>? get completion => _completion;

  @override
  bool get isAvailable => runner.isAvailable;

  @override
  Future<DenseStageResult> start(DenseStageRequest request) async {
    final result = await _start(request, recovering: false);
    if (result.status != DenseStageStatus.started) {
      await _background.abandonBeforeStart(request.captureDir);
    }
    return result;
  }

  @override
  Future<DenseStageResult> recover(DenseStageRequest request) async {
    final plan = await inspectRecovery(request);
    if (!plan.canRecover) {
      await _background.abandonBeforeStart(request.captureDir);
      return DenseStageResult(DenseStageStatus.failed, message: plan.message);
    }
    final result = await _start(request, recovering: true);
    if (result.status != DenseStageStatus.started) {
      await _background.abandonBeforeStart(request.captureDir);
    }
    return result;
  }

  Future<DenseStageResult> _start(
    DenseStageRequest request, {
    required bool recovering,
  }) async {
    if (_requiresProcessRestart) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: wholeRestartRequiredMessage,
      );
    }
    try {
      await _requireOrigin(request);
      await identityScope.revalidate();
      _recordFor(request);
      identityScope.requireCapture(request.captureDir);
    } catch (error) {
      return DenseStageResult(
        DenseStageStatus.failed,
        message: DenseStageProgress.shorten(error),
      );
    }
    if (request.selection != null) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '完整38流程尚不支持选区，请使用整朵点云',
      );
    }
    if (!isAvailable) {
      return const DenseStageResult(
        DenseStageStatus.unavailable,
        message: '完整38处理引擎不可用',
      );
    }
    if (_running ||
        denseStageProgress.value?.state == DenseStageState.running) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '处理已在进行中，请等它完成',
      );
    }
    final denied = denseDenyReason(request.captureDir);
    if (denied != null) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '稠密点云已经生成过，不会重新训练',
      );
    }
    try {
      await verifyDenseWorkStateOf(
        request.captureDir,
        identityScope: identityScope,
      );
      await _requireOrigin(request);
    } catch (error) {
      return DenseStageResult(
        DenseStageStatus.failed,
        message: DenseStageProgress.shorten(error),
      );
    }
    if (!recovering &&
        denseWorkStateOf(request.captureDir, identityScope: identityScope) !=
            DenseWorkState.notStarted) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '已有未完成处理任务，需续跑原任务，不能重新开始完整38',
      );
    }
                                                                          
    if (FileSystemEntity.typeSync(
          '${request.captureDir}/$kDensePlyFileName',
          followLinks: false,
        ) !=
        FileSystemEntityType.notFound) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '已有未完成稠密产物，需核对原任务后续跑',
      );
    }
    if (_running ||
        denseStageProgress.value?.state == DenseStageState.running) {
      return const DenseStageResult(
        DenseStageStatus.failed,
        message: '处理已在进行中，请等它完成',
      );
    }
    _running = true;
    DateTime? preparingStarted;
    ReconstructionBackgroundLease? backgroundLease;
    void Function()? removeRevocationListener;
    try {
                                                                           
                                                                   
      backgroundLease = await _background.claim(request.captureDir);
      final record = await _requireOrigin(request);
      if (record.artifactPath != null) {
        throw StateError('作品已有成品，不会覆盖');
      }
      final target = await _store.glbFileFor(
        record.id,
        pipelineKind: record.pipelineKind,
      );
      await _requireAbsent(target);
      await _requireOrigin(request, paths: [target.path]);
      final priorRuns = Directory(
        '${request.captureDir}/$kReconstructionFullRunsDirName',
      );
      if (!recovering &&
          await priorRuns.exists() &&
          await priorRuns.list(followLinks: false).isEmpty == false) {
        throw StateError('已有完整38任务记录，需续跑原任务，不能重新开始');
      }
                                                                            
                                                                                 
                                                                             
      final recoveryOrigin = recovering ? await _recoveryOrigin(request) : null;
      if (!recovering) await _authorize(request, record);
      await _requireOrigin(request, expectedId: record.id);
      final started = DateTime.now();
      preparingStarted = started;
      denseStageProgress.value = DenseStageProgress(
        captureDir: request.captureDir,
        state: DenseStageState.running,
        phase: 'preparing',
        startedAt: started,
      );
      removeRevocationListener = _store.addRevocationListener(() {
        final current = denseStageProgress.value;
        if (current?.captureDir == request.captureDir &&
            current?.startedAt == started) {
          denseStageProgress.value = null;
        }
      });
      recoveryPersistenceCheckpoint?.call('authorization_published');
      await _requireOrigin(request);
      final inputs = await _gatherer.gatherInputs(
        request,
        forceJpeg: true,
        sortByFrameId: true,
        expectedFrameCount: runner is ReconstructionActualFramePipelineRunner ? null : 97,
      );
      await _requireOrigin(request);
      if (inputs.frames.any((f) => f.hasNv12 || f.jpegPath.isEmpty)) {
        throw StateError('完整38需要有序JPEG输入');
      }
      if (runner is ReconstructionActualFramePipelineRunner) {
        requireWholeDiskBytes(
          await _readStorageBytes(),
          await reconstructionActiveDiskBudgetForRunner(
            runner,
            inputs.frames.length,
            overrideBudget: _storageBudget,
          ),
        );
        await _requireOrigin(request);
      }
      final inputIdentity = await _inputIdentity(request, inputs);
      if (recoveryOrigin?['inputs'] != null &&
          jsonEncode(recoveryOrigin!['inputs']) != jsonEncode(inputIdentity)) {
        throw StateError('本地输入与原任务不一致，保留原任务，不能继续处理');
      }
                                                                               
                                                                               
                                                                                  
      final work = await Directory(
        '${request.captureDir}/$kReconstructionFullRunsDirName',
      ).create(recursive: true);
      await _requireOrigin(request, paths: [work.path]);
                                                                           
                                                                               
                                                                              
                                                                                 
      final preparing = await work.createTemp('.preparing_full_pipeline_');
      await _requireOrigin(request, paths: [preparing.path]);
      final staging = Directory(await preparing.resolveSymbolicLinks());
      final authorization = await _readRegularJson(
        File('${request.captureDir}/$kDenseStartedMarkerName'),
      );
      final authorizationHash = await _hashRegular(
        File('${request.captureDir}/$kDenseStartedMarkerName'),
      );
      final origin = File('${staging.path}/RECOVERY_ORIGIN.json');
      recoveryPersistenceCheckpoint?.call('before_origin_write');
      await _writeAtomicJson(
        origin,
        {
          'schema': 'RECONSTRUCTION_LOCAL_RECOVERY_ORIGIN_V1',
          'record_id': record.id,
          'account_namespace': record.accountNamespace,
          'capture_dir': await Directory(
            request.captureDir,
          ).resolveSymbolicLinks(),
          'scope': identityScope.toJson(),
          'point_count': request.pointCount,
          'selection': request.selection?.toJson(),
          'authorization_sha256': authorizationHash,
          'inputs': inputIdentity,
          'action': recovering ? 'restart_dense_origin' : 'start_dense',
          if (recoveryOrigin?['_run_path'] != null)
            'previous_run': recoveryOrigin!['_run_path'],
        },
        afterPendingWrite: () {
          recoveryPersistenceCheckpoint?.call('origin_pending_written');
        },
      );
                                                                              
      if (authorization['reconstruction_authorization'] == null ||
          await _hashRegular(
                File('${request.captureDir}/$kDenseStartedMarkerName'),
              ) !=
              authorizationHash) {
        throw StateError('原任务授权已改变，保留准备数据');
      }
      recoveryPersistenceCheckpoint?.call('origin_written');
      await _requireOrigin(request, paths: [preparing.path]);
      final suffix = staging.path
          .split('/')
          .last
          .substring('.preparing_'.length);
      final runPath = '${staging.parent.path}/$suffix';
      if (await FileSystemEntity.type(runPath, followLinks: false) !=
          FileSystemEntityType.notFound) {
        throw StateError('拒绝覆盖已有完整38任务');
      }
      final run = await staging.rename(runPath);
      recoveryPersistenceCheckpoint?.call('attempt_published');
      await _requireOrigin(request, paths: [run.path]);
      denseStageProgress.value = denseStageProgress.value?.copyWith(
        phase: 'session',
      );
      _completion = _run(
        request,
        inputs,
        run,
        record.id,
        started,
        backgroundLease,
      ).whenComplete(removeRevocationListener);
      unawaited(_completion!);
      return const DenseStageResult(
        DenseStageStatus.started,
        message: '完整38处理已开始',
      );
    } catch (e, st) {
      if (backgroundLease != null)
        await _background.finish(backgroundLease, success: false);
      _running = false;
      removeRevocationListener?.call();
      if (preparingStarted != null && _store.isActive) {
        denseStageProgress.value = DenseStageProgress(
          captureDir: request.captureDir,
          state: DenseStageState.failed,
          phase: 'preparation_failed',
          message: DenseStageProgress.shorten(e),
          startedAt: preparingStarted,
          finishedAt: DateTime.now(),
        );
      }
      DeviceLog.log('FullPipeline', 'input preparation failed: $e\n$st');
      return DenseStageResult(
        DenseStageStatus.failed,
        message: DenseStageProgress.shorten(e),
      );
    }
  }

  Future<void> _run(
    DenseStageRequest request,
    DenseStageInputs inputs,
    Directory work,
    String recordId,
    DateTime started,
    ReconstructionBackgroundLease backgroundLease,
  ) async {
    final dir = request.captureDir;
    var backgroundSuccess = false;
    void Function()? releasePublication;
    try {
      releasePublication = await ReconstructionCaptureRetirement.holdPublication(
        _store,
        recordId,
      );
      await _requireOrigin(request, expectedId: recordId);
      final canonicalCapture = await Directory(dir).resolveSymbolicLinks();
      void publishProgress(
        String phase,
        int done,
        int total, [
        DenseProgressSourceTime? source,
      ]) {
        _background.update(backgroundLease, phase, done, total);
        final current = denseStageProgress.value;
        if (_store.isActive &&
            current?.captureDir == dir &&
            current?.state == DenseStageState.running) {
          denseStageProgress.value = current!.copyWith(
            phase: phase,
            done: done,
            total: total,
            sourceTime: source,
          );
        }
      }

      final result = await _runWithVisuals(
        request: request,
        inputs: inputs,
        workDir: work.path,
        onProgress: (phase, done, total) => publishProgress(phase, done, total),
        onTimedProgress: (phase, done, total, source) =>
            publishProgress(phase, done, total, source),
      );
      await _requireOrigin(request, expectedId: recordId);
                                                                           
                                                                        
      final outputs = ReconstructionProductionOutputContract.fromReceipt(
        result.receipt,
        workRoot: work.path,
        identityScope: identityScope,
      );
      final glb = File(outputs.glb.path);
      final glbRow = outputs.glb.row;
      await _verifyObserved(glb, glbRow, work);
      File? dense;
      Map<String, Object?>? denseRow;
      var points = 0;
      switch (outputs.dense) {
        case ReconstructionDensePlyOutput(:final artifact):
          dense = File(artifact.path);
          denseRow = artifact.row;
          await _verifyObserved(dense, denseRow, work);
          final count = densePlyPoints(dense.path);
          if (count == null || count <= 0) {
            throw StateError('完整38稠密PLY不完整');
          }
          points = count;
        case ReconstructionOriginalRetainedOutput(:final files):
                                                                                
                                                                                
          Map<String, dynamic>? retiredEvidence;
          final retirementReceipt = result.receipt['generated_retirement'];
          if (retirementReceipt is Map) {
            final evidencePath = retirementReceipt['evidence_path'];
            final evidenceHash = retirementReceipt['evidence_sha256'];
            final evidencePrefix = '$canonicalCapture/reconstruction_completion_evidence/';
            if (evidencePath is! String ||
                !evidencePath.startsWith(evidencePrefix) ||
                evidenceHash is! String ||
                await _hashRegular(File(evidencePath)) != evidenceHash)
              throw StateError('Retired output evidence identity mismatch');
            retiredEvidence = await _readRegularJson(File(evidencePath));
            if (retiredEvidence['schema'] != 'RECONSTRUCTION_GENERATED_RETIREMENT_V1' ||
                retiredEvidence['root'] != outputs.glb.root ||
                retiredEvidence['status'] != 'STAGE_RETIRED' ||
                (retiredEvidence['failures'] as List).isNotEmpty)
              throw StateError('Retirement did not pass');
          }
          for (final artifact in files) {
            if (retiredEvidence != null &&
                (retiredEvidence['deleted'] as List).contains(
                  artifact.relativePath,
                )) {
              final prior =
                  (retiredEvidence['artifacts'] as Map)[artifact.relativePath];
              if (prior is! Map ||
                  prior['bytes'] != artifact.bytes ||
                  prior['sha256'] != artifact.sha256 ||
                  prior['last_reader_stage'] != 9 ||
                  await FileSystemEntity.type(
                        artifact.path,
                        followLinks: false,
                      ) !=
                      FileSystemEntityType.notFound)
                throw StateError('Prior retained output validation mismatch');
            } else {
              await _verifyObserved(File(artifact.path), artifact.row, work);
            }
          }
      }
      final record = await _requireOrigin(request, expectedId: recordId);
      if (record.artifactPath != null) throw StateError('作品成品已改变，停止覆盖');
      final target = await _store.glbFileFor(
        record.id,
        pipelineKind: record.pipelineKind,
      );
      final denseTarget = dense == null
          ? null
          : File('$dir/$kDensePlyFileName');
      await _requireAbsent(target);
      if (denseTarget != null) await _requireAbsent(denseTarget);
                                                                                
                                                                          
      Future<void> requirePublication() => _requireOrigin(
        request,
        expectedId: recordId,
        paths: [target.path, if (denseTarget != null) denseTarget.path],
      );
      final stagedGlb = await _stageCopy(
        glb,
        target,
        glbRow,
        requireWritable: requirePublication,
      );
      final stagedDense = dense == null
          ? null
          : await _stageCopy(
              dense,
              denseTarget!,
              denseRow!,
              requireWritable: requirePublication,
            );
      await requirePublication();
      await _requireAbsent(target);
      if (denseTarget != null) await _requireAbsent(denseTarget);
      await requirePublication();
      final current = _recordFor(request, expectedId: recordId);
      if (current.artifactPath != null) throw StateError('作品成品已改变，停止覆盖');
      await stagedGlb.rename(target.path);
      await requirePublication();
      if (stagedDense != null) {
        await stagedDense.rename(denseTarget!.path);
        await requirePublication();
      }
      await _verifyBytes(target, glbRow);
      if (denseTarget != null) await _verifyBytes(denseTarget, denseRow!);
                                                                              
                                                                             
      await requirePublication();
      final latest = _recordFor(request, expectedId: recordId);
      if (latest.artifactPath != null) throw StateError('作品成品已改变，停止挂接');
      await _store.attachVerifiedLocalArtifact(
        id: latest.id,
        captureDir: latest.captureDir!,
        artifactPath: Uri.file(target.path).toString(),
        finalGlbSha256: glbRow['sha256'] as String,
        finalGlbBytes: glbRow['bytes'] as int,
      );
      await requirePublication();
      final persisted = _store.byId(recordId);
      if (persisted?.artifactPath != Uri.file(target.path).toString()) {
        throw StateError('作品被删除或成品挂接失败');
      }
      await requirePublication();
      final completedProgress = DenseStageProgress(
        captureDir: dir,
        state: DenseStageState.done,
        phase: 'done',
        done: 13,
        total: 13,
        outPly: denseTarget?.path,
        points: points,
        startedAt: started,
        finishedAt: DateTime.now(),
        preview: ReconstructionPreviewArtifact(
          path: target.path,
          stage: 'glb',
          role: 'glb',
          bytes: glbRow['bytes'] as int,
          sha256: glbRow['sha256'] as String,
        ),
      );
      await onVerifiedBeforeRetirement?.call(result);
                                                                          
                                                                          
      final singleCaptureRetirement = persisted != null && hasReconstructionFinalRecordProof(persisted);
      if (!singleCaptureRetirement && result.receipt['generated_retirement'] is Map) {
        await publishReconstructionCacheCompletion(
          captureDir: dir,
          workRoot: work.path,
          recordId: recordId,
          publishedGlbPath: target.path,
          receipt: result.receipt,
          identityScope: identityScope,
          preserveReceiptOnly: true,
        );
        denseStageProgress.value = completedProgress.copyWith(
          state: DenseStageState.running,
          phase: 'finalizing',
        );
                                                                               
                                                                             
        ReconstructionGeneratedConsumerLeases.instance.closeOffers(work.path);
        await ReconstructionGeneratedConsumerLeases.instance
            .waitForIdle(work.path)
            .timeout(
              const Duration(seconds: 30),
              onTimeout: () => throw StateError(
                'Generated cleanup retained active consumers: ${ReconstructionGeneratedConsumerLeases.instance.activeConsumers(work.path)}',
              ),
            );
        await requirePublication();
        final retirement = GeneratedArtifactRetirement(
          root: Directory(await work.resolveSymbolicLinks()),
          evidenceFile: File(
            '$canonicalCapture/reconstruction_completion_evidence/${work.uri.pathSegments.where((p) => p.isNotEmpty).last}/final_retirement.json',
          ),
        );
        final relative = glb.path.substring(work.path.length + 1);
        retirement.register(
          relative,
          bytes: outputs.glb.bytes,
          sha256: outputs.glb.sha256,
          lastReaderStage: 99,
        );
        await retirement.finalizeGlbOnly(
          glbRelative: relative,
          publishedPath: target.path,
          validatePublished: () async {
            await requirePublication();
            await _verifyBytes(target, glbRow);
            final attached = _store.byId(recordId);
            if (attached?.artifactPath != Uri.file(target.path).toString())
              throw StateError('Published record detached before retirement');
          },
        );
      }
      if (!singleCaptureRetirement && outputs.dense is ReconstructionOriginalRetainedOutput) {
        await writeReconstructionCacheCompletedMarker(
          captureDir: dir,
          workRoot: work.path,
          recordId: recordId,
          publishedGlbPath: target.path,
          receipt: result.receipt,
          identityScope: identityScope,
        );
      }
      if (singleCaptureRetirement) {
                                                                            
                                                                             
        denseStageProgress.value = completedProgress.copyWith(
          state: DenseStageState.running,
          phase: 'finalizing',
        );
        try {
          final cleanup = await ReconstructionCaptureRetirement(
            store: _store,
            drainPipelineReaders: () async {
              ReconstructionGeneratedConsumerLeases.instance.closeOffers(work.path);
              await ReconstructionGeneratedConsumerLeases.instance.waitForIdle(work.path);
            },
          ).retire(recordId: recordId, captureDir: dir);
          DeviceLog.log(
            'FullPipeline',
            cleanup.completed
                ? 'capture retired; final GLB retained record=$recordId'
                : 'final GLB retained; cleanup awaiting ${cleanup.pendingPaths} record=$recordId',
          );
        } catch (cleanupError, cleanupStack) {
                                                                             
                                                                                 
          DeviceLog.log(
            'FullPipeline',
            'published GLB cleanup pending record=$recordId: $cleanupError\n$cleanupStack',
          );
        }
      }
      denseStageProgress.value = completedProgress;
      DeviceLog.log('FullPipeline', 'actual13-stage GLB linked record=$recordId');
      _background.update(backgroundLease, 'done', 13, 13);
      backgroundSuccess = true;
    } catch (e, st) {
      if (wholeFailureNeedsProcessRestart(e)) _requiresProcessRestart = true;
      DeviceLog.log('FullPipeline', 'failed; retained scratch=${work.path}: $e\n$st');
      if (!_store.isActive) return;
      final last = denseStageProgress.value;
      denseStageProgress.value = DenseStageProgress(
        captureDir: dir,
        state: DenseStageState.failed,
        phase: 'failed',
        message: _requiresProcessRestart
            ? wholeRestartRequiredMessage
            : DenseStageProgress.shorten(e),
        startedAt: started,
        finishedAt: DateTime.now(),
        preview: last?.captureDir == dir ? last?.preview : null,
        live: last?.captureDir == dir ? last?.live : null,
      );
    } finally {
      try {
        await _background.finish(backgroundLease, success: backgroundSuccess);
      } finally {
        releasePublication?.call();
        _running = false;
      }
    }
  }

  Future<ReconstructionPipelineResult> _runWithVisuals({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
    required ReconstructionProductionProgress onProgress,
    required void Function(String, int, int, DenseProgressSourceTime)
    onTimedProgress,
  }) {
    final visual = runner;
    if (visual is! ReconstructionVisualPipelineRunner) {
      return runner.run(
        request: request,
        inputs: inputs,
        workDir: workDir,
        onProgress: onProgress,
      );
    }
    void publishPreview(ReconstructionPreviewArtifact artifact) {
      final current = denseStageProgress.value;
      if (_store.isActive &&
          current?.captureDir == request.captureDir &&
          current?.state == DenseStageState.running) {
        denseStageProgress.value = current!.copyWith(preview: artifact);
      }
    }

    if (visual is ReconstructionTimedVisualPipelineRunner) {
      return visual.runWithTimedVisuals(
        request: request,
        inputs: inputs,
        workDir: workDir,
        onProgress: onProgress,
        onTimedProgress: onTimedProgress,
        onPreview: publishPreview,
      );
    }
    return visual.runWithVisuals(
      request: request,
      inputs: inputs,
      workDir: workDir,
      onProgress: onProgress,
      onPreview: publishPreview,
    );
  }

  Future<void> _authorize(DenseStageRequest request, ScanRecord record) async {
    await _requireOrigin(request, expectedId: record.id);
    final root = await Directory(request.captureDir).resolveSymbolicLinks();
    final sparse = File(request.sparsePlyPath);
    if (await sparse.parent.resolveSymbolicLinks() != root ||
        sparse.uri.pathSegments.last != 'official_sfm_sparse.ply') {
      throw StateError('原稀疏输入不属于当前作品');
    }
                                                                         
                                                                             
                                                                               
    await _writeAtomicJson(
      File('$root/$kDenseStartedMarkerName'),
      {
        'schema': 'pw_dense_started/1',
        if (identityScope.isIndependentTest) ...{
          'bundle_id': identityScope.bundleId,
          'identity_scope': identityScope.toJson(),
        },
        'started_at': DateTime.now().toIso8601String(),
        'sparse_points': request.pointCount,
        'selection': request.selection?.toJson(),
        'reconstruction_authorization': {
          'schema': 'RECONSTRUCTION_LOCAL_AUTHORIZATION_V1',
          'record_id': record.id,
          'account_namespace': record.accountNamespace,
          'capture_dir': root,
          'scope': identityScope.toJson(),
          'sparse_relative_path': 'official_sfm_sparse.ply',
          'point_count': request.pointCount,
          'selection': request.selection?.toJson(),
          'approved_action': 'enter_dense_permanently',
        },
      },
      afterPendingWrite: () {
        recoveryPersistenceCheckpoint?.call('authorization_pending_written');
      },
    );
  }

  @override
  Future<DenseStageRecoveryPlan> inspectRecovery(
    DenseStageRequest request,
  ) async {
    try {
      final record = await _requireOrigin(request);
      if (_requiresProcessRestart) {
        return const DenseStageRecoveryPlan(
          DenseStageRecoveryStatus.unavailable,
          message: wholeRestartRequiredMessage,
        );
      }
      await identityScope.revalidate();
      identityScope.requireCapture(request.captureDir);
      if (_running ||
          denseStageProgress.value?.state == DenseStageState.running) {
        return const DenseStageRecoveryPlan(
          DenseStageRecoveryStatus.busy,
          message: '处理正在进行中',
        );
      }
      final target = await _store.glbFileFor(
        record.id,
        pipelineKind: record.pipelineKind,
      );
      if (record.artifactPath != null ||
          await FileSystemEntity.type(target.path, followLinks: false) !=
              FileSystemEntityType.notFound ||
          denseDenyReason(request.captureDir) != null ||
          File(
            '${request.captureDir}/$reconstructionCacheCompletionMarkerName',
          ).existsSync()) {
        return const DenseStageRecoveryPlan(
          DenseStageRecoveryStatus.completed,
          message: '已有成品或完成记录，不会覆盖或重新处理',
        );
      }
      if (!isAvailable) {
        return const DenseStageRecoveryPlan(
          DenseStageRecoveryStatus.unavailable,
          message: '完整38处理引擎不可用',
        );
      }
      final origin = await _recoveryOrigin(request);
      return DenseStageRecoveryPlan(
        DenseStageRecoveryStatus.available,
        action: DenseStageRecoveryAction.restartDense,
        priorRunPath: origin['_run_path'] as String,
        message: origin['inputs'] == null
            ? '授权已保存；启动时核对本地输入，从稠密阶段起点重新处理，不是断点续跑'
            : '已核对本地输入；从稠密阶段起点重新处理，不是断点续跑',
      );
    } catch (error) {
      return DenseStageRecoveryPlan(
        DenseStageRecoveryStatus.invalid,
        message: DenseStageProgress.shorten(error),
      );
    }
  }

  Future<Map<String, dynamic>> _recoveryOrigin(
    DenseStageRequest request,
  ) async {
    final record = await _requireOrigin(request);
    final markerFile = File('${request.captureDir}/$kDenseStartedMarkerName');
    final marker = await _readRegularJson(markerFile);
    if (marker['schema'] != 'pw_dense_started/1' ||
        marker['sparse_points'] != request.pointCount ||
        jsonEncode(marker['selection']) !=
            jsonEncode(request.selection?.toJson())) {
      throw StateError('原任务选区或输入身份不一致，不能重新选择');
    }
    if (request.selection != null) {
      throw StateError('完整38选区衔接尚未就绪，保留原任务');
    }
    final root = await Directory(request.captureDir).resolveSymbolicLinks();
    final authorization = marker['reconstruction_authorization'];
    if (authorization is! Map ||
        authorization['schema'] != 'RECONSTRUCTION_LOCAL_AUTHORIZATION_V1' ||
        authorization['record_id'] != record.id ||
        authorization['account_namespace'] != record.accountNamespace ||
        authorization['capture_dir'] != root ||
        jsonEncode(authorization['scope']) !=
            jsonEncode(identityScope.toJson()) ||
        authorization['sparse_relative_path'] != 'official_sfm_sparse.ply' ||
        authorization['point_count'] != request.pointCount ||
        jsonEncode(authorization['selection']) !=
            jsonEncode(marker['selection']) ||
        authorization['approved_action'] != 'enter_dense_permanently') {
      throw StateError('旧任务缺少可核验的恢复授权绑定，保留原任务');
    }
    final authorizationHash = await _hashRegular(markerFile);
    if (!await File(request.sparsePlyPath).exists()) {
      throw StateError('本地输入缺失：official_sfm_sparse.ply');
    }
    if (await File(request.sparsePlyPath).resolveSymbolicLinks() !=
        '$root/official_sfm_sparse.ply') {
      throw StateError('原稀疏输入不属于当前作品');
    }
    final metadata = File('$root/official_sfm_sparse_meta.json');
    if (!await metadata.exists()) {
      throw StateError('本地输入缺失：official_sfm_sparse_meta.json');
    }
                                                                         
                                                                                 
    await _hashRegular(metadata);
    final runs = Directory('$root/$kReconstructionFullRunsDirName');
    final candidates = <Map<String, dynamic>>[];
    final entries = await runs.exists()
        ? await runs.list(followLinks: false).toList()
        : <FileSystemEntity>[];
    for (final entry in entries) {
      if (entry is Directory &&
          entry.path.split('/').last.startsWith('.preparing_full_pipeline_')) {
        await _requireNeverExecutedPreparation(entry);
        continue;
      }
      if (entry is! Directory ||
          !entry.path.split('/').last.startsWith('full_pipeline_') ||
          await entry.resolveSymbolicLinks() != entry.path) {
        throw StateError('原任务目录身份异常，保留现有文件');
      }
      final completed = File('${entry.path}/outputs/RESULT.json');
      if (await completed.exists()) {
        final result = await _readRegularJson(completed);
        if (result['full_pipeline_complete'] == true || result['exit_code'] == 0) {
          throw StateError('原任务已有成功回执，需核对成品挂接，不能重新处理');
        }
      }
      final file = File('${entry.path}/RECOVERY_ORIGIN.json');
      if (!await file.exists()) {
        throw StateError('旧任务缺少可核验的恢复输入绑定，保留原任务');
      }
      final origin = await _readRegularJson(file);
      if (origin['schema'] != 'RECONSTRUCTION_LOCAL_RECOVERY_ORIGIN_V1' ||
          origin['authorization_sha256'] != authorizationHash ||
          origin['record_id'] != record.id ||
          origin['account_namespace'] != record.accountNamespace ||
          origin['capture_dir'] != root ||
          jsonEncode(origin['scope']) != jsonEncode(identityScope.toJson()) ||
          origin['point_count'] != request.pointCount ||
          jsonEncode(origin['selection']) != jsonEncode(marker['selection']) ||
          origin['inputs'] is! Map) {
        throw StateError('原任务与当前作品或账号不一致');
      }
                                                                        
      if (candidates.isNotEmpty &&
          jsonEncode(candidates.first['inputs']) !=
              jsonEncode(origin['inputs'])) {
        throw StateError('历史任务输入不一致，保留全部任务');
      }
      origin['_run_path'] = entry.path;
      candidates.add(origin);
    }
    if (candidates.isEmpty) {
                                                                               
                                                                                
      return {'inputs': null, '_run_path': markerFile.path};
    }
    final origin = candidates.last;
    final inputs = origin['inputs'] as Map;
    for (final key in ['sparse', 'metadata']) {
      final row = inputs[key];
      if (row is! Map) throw StateError('原任务缺少本地输入身份');
      final expectedName = key == 'sparse'
          ? 'official_sfm_sparse.ply'
          : 'official_sfm_sparse_meta.json';
      if (row['name'] != expectedName) {
        throw StateError('原任务输入路径身份不一致');
      }
      final file = File('$root/$expectedName');
      if (!await file.exists()) throw StateError('本地输入缺失：${row['name']}');
      if (await _hashRegular(file) != row['sha256']) {
        throw StateError('本地输入损坏或已改变：${row['name']}');
      }
    }
    final photos = inputs['photos'];
    if (photos is! List || photos.isEmpty) {
      throw StateError('原任务缺少可核验的本地照片清单');
    }
    for (final raw in photos) {
      if (raw is! Map ||
          raw['path'] is! String ||
          raw['sha256'] is! String ||
          !RegExp(r'^[0-9a-f]{64}$').hasMatch(raw['sha256'] as String)) {
        throw StateError('原任务照片身份记录不完整');
      }
      final photo = File(raw['path'] as String);
      if (!await photo.exists()) {
        throw StateError('本地输入缺失：原任务照片');
      }
      if (await _hashRegular(photo) != raw['sha256']) {
        throw StateError('本地输入损坏或已改变：原任务照片');
      }
    }
    return origin;
  }

  Future<void> _requireNeverExecutedPreparation(Directory directory) async {
    if (await directory.resolveSymbolicLinks() != directory.path) {
      throw StateError('准备目录身份异常，保留现有文件');
    }
    await for (final entry in directory.list(followLinks: false)) {
      final name = entry.path.split('/').last;
      if (entry is! File ||
          (name != 'RECOVERY_ORIGIN.json' &&
              !name.startsWith('.reconstruction_write_'))) {
        throw StateError('准备目录含未知执行文件，不能认定为未开始任务');
      }
      if (await FileSystemEntity.type(entry.path, followLinks: false) !=
          FileSystemEntityType.file) {
        throw StateError('准备目录含异常文件，保留原任务');
      }
    }
  }

  Future<Map<String, Object?>> _inputIdentity(
    DenseStageRequest request,
    DenseStageInputs inputs,
  ) async {
    final sparse = File(request.sparsePlyPath);
    final metadata = File(
      '${request.captureDir}/official_sfm_sparse_meta.json',
    );
                                                                           
                                                                                 
    final sources = <String, Object?>{};
    for (final entry in {'sparse': sparse, 'metadata': metadata}.entries) {
      sources[entry.key] = await entry.value.exists()
          ? {
              'name': entry.value.uri.pathSegments.last,
              'sha256': await _hashRegular(entry.value),
            }
          : null;
    }
    final photos = <Map<String, Object?>>[];
    for (final frame in inputs.frames) {
      final photo = File(frame.jpegPath);
      photos.add({
        'path': await photo.exists()
            ? await photo.resolveSymbolicLinks()
            : photo.absolute.path,
        'sha256': await photo.exists() ? await _hashRegular(photo) : null,
      });
    }
    sources['photos'] = photos;
    sources['gathered_sha256'] = sha256
        .convert(
          utf8.encode(
            jsonEncode({
              'frames': [
                for (var i = 0; i < inputs.frames.length; i++)
                  {
                    ..._frameIdentity(inputs.frames[i]),
                    'jpeg_sha256': photos[i]['sha256'],
                  },
              ],
              'xyz_sha256': _xyzIdentity(inputs.pointsXyz),
            }),
          ),
        )
        .toString();
    return sources;
  }

  Future<ScanRecord> _requireOrigin(
    DenseStageRequest request, {
    String? expectedId,
    Iterable<String> paths = const [],
  }) async {
    await _store.ensureLoaded();
    await _store.requireOwnedPath(request.captureDir);
    for (final path in paths) {
      await _store.requireOwnedPath(path);
    }
    return _recordFor(request, expectedId: expectedId);
  }

  ScanRecord _recordFor(DenseStageRequest request, {String? expectedId}) {
    if (!_store.isActive) {
      throw StateError('Local workspace is locked or revoked');
    }
    final id = expectedId ?? request.recordId;
    final records = id == null
        ? _store.records
              .where(
                (r) =>
                    r.captureDir != null &&
                    _samePath(r.captureDir!, request.captureDir),
              )
              .toList()
        : [if (_store.byId(id) case final r?) r];
    if (records.length != 1 ||
        records.single.captureDir == null ||
        !_samePath(records.single.captureDir!, request.captureDir)) {
      throw StateError('拍摄目录必须对应唯一的已有作品记录');
    }
    final record = records.single;
    if (record.accountNamespace != _store.workspace.namespace) {
      throw StateError('Capture belongs to a different workspace');
    }
    return record;
  }
}

bool _samePath(String a, String b) =>
    Uri.directory(Directory(a).absolute.path).normalizePath() ==
    Uri.directory(Directory(b).absolute.path).normalizePath();

Future<void> _requireAbsent(File file) async {
  if (await FileSystemEntity.type(file.path, followLinks: false) !=
      FileSystemEntityType.notFound) {
    throw StateError('拒绝覆盖已有产物 ${file.path}');
  }
}

Future<void> _verifyObserved(File file, Map row, Directory work) async {
  final path = file.absolute.path;
  final relative = row['path'] as String;
  if (!path.startsWith('${work.absolute.path}/') ||
      !path.endsWith('/$relative') ||
      relative.startsWith('/') ||
      relative.split('/').any((p) => p.isEmpty || p == '.' || p == '..') ||
      relative.contains('\\') ||
      await file.resolveSymbolicLinks() != path) {
    throw StateError('实际产物不在本次运行目录或路径身份不一致');
  }
  await _verifyBytes(file, row);
}

Future<void> _verifyBytes(File file, Map row) async {
  if (await FileSystemEntity.type(file.path, followLinks: false) !=
          FileSystemEntityType.file ||
      await file.length() != row['bytes'] ||
      (await sha256.bind(file.openRead()).first).toString() != row['sha256']) {
    throw StateError('实际产物字节身份核对失败 ${file.path}');
  }
}

Future<File> _stageCopy(
  File source,
  File target,
  Map row, {
  required Future<void> Function() requireWritable,
}) async {
  await requireWritable();
  await target.parent.create(recursive: true);
  await requireWritable();
  final staging = await target.parent.createTemp('.full_pipeline_publish_');
  await requireWritable();
  final file = await source.copy(
    '${staging.path}/${target.uri.pathSegments.last}',
  );
  await requireWritable();
  await _verifyBytes(file, row);
  await requireWritable();
  return file;
}

Future<String> _hashRegular(File file) async {
  if (await FileSystemEntity.type(file.path, followLinks: false) !=
      FileSystemEntityType.file) {
    throw StateError('本地输入不是普通文件 ${file.path}');
  }
  return (await sha256.bind(file.openRead()).first).toString();
}

Future<Map<String, dynamic>> _readRegularJson(File file) async {
  await _hashRegular(file);
  final value = jsonDecode(await file.readAsString());
  if (value is! Map<String, dynamic>) throw StateError('原任务记录格式损坏');
  return value;
}

                                                                                
String _xyzIdentity(List<double> points) {
  final output = _DigestReceiver();
  final sink = sha256.startChunkedConversion(output);
  for (var start = 0; start < points.length; start += 1024) {
    final count = (points.length - start).clamp(0, 1024);
    final bytes = ByteData(count * 8);
    for (var i = 0; i < count; i++) {
      bytes.setFloat64(i * 8, points[start + i], Endian.little);
    }
    sink.add(bytes.buffer.asUint8List());
  }
  sink.close();
  return output.value!.toString();
}

final class _DigestReceiver implements Sink<Digest> {
  Digest? value;
  @override
  void add(Digest data) {
    value = data;
  }

  @override
  void close() {}
}

Map<String, Object?> _frameIdentity(PwDenseFrame frame) => {
  'id': frame.frameId,
  'fx': frame.fx,
  'fy': frame.fy,
  'cx': frame.cx,
  'cy': frame.cy,
  'w': frame.imageW,
  'h': frame.imageH,
  'q': frame.qWxyz,
  't': frame.t,
};

int _atomicWriteSequence = 0;

                                                                              
                                                                          
Future<void> _writeAtomicJson(
  File destination,
  Map<String, Object?> value, {
  void Function()? afterPendingWrite,
}) async {
  await _requireAbsent(destination);
  final contents = jsonEncode(value);
  final pending = File(
    '${destination.parent.path}/.reconstruction_write_'
    '${DateTime.now().microsecondsSinceEpoch}_${_atomicWriteSequence++}_'
    '${destination.uri.pathSegments.last}',
  );
  await pending.create(exclusive: true);
  await pending.writeAsString(contents, flush: true);
  afterPendingWrite?.call();
  if (await pending.readAsString() != contents) {
    throw StateError('任务授权或输入记录未完整保存');
  }
  await _requireAbsent(destination);
  await pending.rename(destination.path);
}
