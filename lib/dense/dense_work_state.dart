                                                                                                       
                                                               
                            
                                                     
               
  
                                                                                                   
                                                                                                            
                                                                                                         
                                                                                                  
                                                                                                     
  
                                                                                                 
                                                                                                        
                                                                                                
import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import '../official_capture/selection_box.dart';
import '../official_util/device_log.dart';
import '../point_cloud_lod/dense_lod_cache.dart' show densePlyComplete;
import '../reconstruction/cache_completion.dart';
import '../reconstruction/execution_identity_scope.dart';
import 'dense_stage_progress.dart';

const String kDensePlyFileName = 'official_dense.ply';
const String kDenseWorkDirName = 'dense_work';

                                                                                                       
const String kDenseStartedMarkerName = 'dense_started.json';

enum DenseWorkState { notStarted, startedIncomplete, done }

final _cacheProofs = <String, ReconstructionCacheCompletionProof>{};
final _cacheChecks = <String, Future<DenseWorkState>>{};
final _failedCacheChecks = <String, String>{};
final _deferredCacheStates =
    <
      String,
      ({
        String captureDir,
        DenseWorkState state,
        ReconstructionExecutionIdentityScope scope,
      })
    >{};
bool _cacheHandoffScheduled = false;

String _captureKey(String path) => Directory(path).resolveSymbolicLinksSync();
String _cacheKey(String root, ReconstructionExecutionIdentityScope scope) =>
    '$root|${jsonEncode(scope.toJson())}';
String _fileSignature(String path) {
  try {
    final stat = File(path).statSync();
    return '${stat.type}:${stat.size}:${stat.modified.microsecondsSinceEpoch}:'
        '${stat.changed.microsecondsSinceEpoch}';
  } catch (_) {
    return 'UNREADABLE';
  }
}

String _failureDependencies(String root) =>
    '${_fileSignature('$root/$reconstructionCacheCompletionMarkerName')}|'
    '${_fileSignature('${Directory(root).parent.parent.path}/scan_records.json')}';

void _scheduleCacheHandoff() {
  if (_cacheHandoffScheduled ||
      denseStageProgress.value?.state == DenseStageState.running)
    return;
  _cacheHandoffScheduled = true;
  scheduleMicrotask(() {
    _cacheHandoffScheduled = false;
    if (denseStageProgress.value?.state == DenseStageState.running) return;
    if (_deferredCacheStates.isEmpty) return;
    final entry = _deferredCacheStates.entries.first;
    _deferredCacheStates.remove(entry.key);
    if (_deferredCacheStates.isEmpty) {
      denseStageProgress.removeListener(_scheduleCacheHandoff);
    }
    _notifyCacheState(
      entry.value.captureDir,
      entry.value.state,
      identityScope: entry.value.scope,
    );
    if (_deferredCacheStates.isNotEmpty) _scheduleCacheHandoff();
  });
}

void _notifyCacheState(
  String captureDir,
  DenseWorkState state, {
  ReconstructionExecutionIdentityScope? identityScope,
}) {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  final current = denseStageProgress.value;
  if (current?.state == DenseStageState.running) {
    if (_deferredCacheStates.isEmpty) {
      denseStageProgress.addListener(_scheduleCacheHandoff);
    }
    _deferredCacheStates[_cacheKey(captureDir, identityScope)] = (
      captureDir: captureDir,
      state: state,
      scope: identityScope,
    );
    return;
  }
  if (state == DenseWorkState.done) {
    try {
      if (_cacheProofs[_cacheKey(_captureKey(captureDir), identityScope)]
              ?.stillMatches() !=
          true) {
        state = DenseWorkState.startedIncomplete;
      }
    } catch (_) {
      state = DenseWorkState.startedIncomplete;
    }
  }
  denseStageProgress.value = DenseStageProgress(
    captureDir: captureDir,
    state: state == DenseWorkState.done
        ? DenseStageState.done
        : DenseStageState.failed,
    phase: state == DenseWorkState.done ? 'done' : 'cache_verification_failed',
    done: state == DenseWorkState.done ? 13 : 0,
    total: 13,
                                                                              
    message: state == DenseWorkState.done ? null : '完整38缓存或成品核验未通过，保留原任务',
    finishedAt: DateTime.now(),
  );
}

                                                                           
                                                                         
Future<DenseWorkState> verifyDenseWorkStateOf(
  String captureDir, {
  bool recheck = false,
  ReconstructionExecutionIdentityScope? identityScope,
}) async {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  if (!File('$captureDir/$reconstructionCacheCompletionMarkerName').existsSync()) {
    return denseWorkStateOf(captureDir, identityScope: identityScope);
  }
  final root = _captureKey(captureDir);
  final key = _cacheKey(root, identityScope);
  final pending = _cacheChecks[key];
  if (pending != null) return pending;
  if (!recheck && _cacheProofs[key]?.stillMatches() == true) {
    _notifyCacheState(
      captureDir,
      DenseWorkState.done,
      identityScope: identityScope,
    );
    return DenseWorkState.done;
  }
  _cacheProofs.remove(key);
                                                                               
                                                                           
                                                                            
  final dependenciesAtStart = _failureDependencies(root);
  final scopeJson = identityScope.toJson();
  final check = () async {
    try {
      final proof = await Isolate.run(
        () => verifyReconstructionCacheCompletion(
          root,
          identityScope: ReconstructionExecutionIdentityScope.fromJson(
            Map<String, dynamic>.from(scopeJson),
          ),
        ),
      );
      if (!proof.stillMatches())
        throw StateError('Completion changed after verification');
      _cacheProofs[key] = proof;
      _failedCacheChecks.remove(key);
      _notifyCacheState(
        captureDir,
        DenseWorkState.done,
        identityScope: identityScope,
      );
      return DenseWorkState.done;
    } catch (error) {
      _cacheProofs.remove(key);
      _failedCacheChecks[key] = dependenciesAtStart;
      DeviceLog.log('FullPipeline', 'cache restart verification failed: $error');
      _notifyCacheState(
        captureDir,
        DenseWorkState.startedIncomplete,
        identityScope: identityScope,
      );
      return DenseWorkState.startedIncomplete;
    } finally {
      _cacheChecks.remove(key);
    }
  }();
  _cacheChecks[key] = check;
  return check;
}

                                                                                 
                                                                        
Future<void> writeReconstructionCacheCompletedMarker({
  required String captureDir,
  required String workRoot,
  required String recordId,
  required String publishedGlbPath,
  required Map<String, Object?> receipt,
  ReconstructionExecutionIdentityScope? identityScope,
}) async {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  await identityScope.revalidate();
  identityScope.requireCapture(captureDir);
  final root = _captureKey(captureDir);
  final key = _cacheKey(root, identityScope);
  if (_cacheChecks.containsKey(key))
    throw StateError('Cache verification already active');
  final scopeJson = identityScope.toJson();
  final check = () async {
    await publishReconstructionCacheCompletion(
      captureDir: root,
      workRoot: workRoot,
      recordId: recordId,
      publishedGlbPath: publishedGlbPath,
      receipt: receipt,
      identityScope: identityScope!,
    );
    final proof = await Isolate.run(
      () => verifyReconstructionCacheCompletion(
        root,
        identityScope: ReconstructionExecutionIdentityScope.fromJson(
          Map<String, dynamic>.from(scopeJson),
        ),
      ),
    );
    if (!proof.stillMatches())
      throw StateError('Completion changed after publication');
    _cacheProofs[key] = proof;
    _failedCacheChecks.remove(key);
    return DenseWorkState.done;
  }();
  _cacheChecks[key] = check;
  try {
    await check;
  } finally {
    _cacheChecks.remove(key);
  }
}

DenseWorkState denseWorkStateOf(
  String captureDir, {
  ReconstructionExecutionIdentityScope? identityScope,
}) {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  final ply = '$captureDir/$kDensePlyFileName';
  if (!File('$captureDir/$reconstructionCacheCompletionMarkerName').existsSync() &&
      densePlyComplete(ply))
    return DenseWorkState.done;
  try {
    final root = _captureKey(captureDir);
    final key = _cacheKey(root, identityScope);
    identityScope.requireCapture(root);
    if (File('$root/$reconstructionCacheCompletionMarkerName').existsSync()) {
      if (_cacheProofs[key]?.stillMatches() == true) return DenseWorkState.done;
      _cacheProofs.remove(key);
      if (_failedCacheChecks[key] != _failureDependencies(root)) {
                                                                               
                                                                       
        unawaited(
          Future<void>(() async {
            await verifyDenseWorkStateOf(
              captureDir,
              identityScope: identityScope,
            );
          }),
        );
      }
      return DenseWorkState.startedIncomplete;
    }
    if (File('$captureDir/$kDenseStartedMarkerName').existsSync() ||
        Directory('$captureDir/$kDenseWorkDirName').existsSync() ||
        File(ply).existsSync()) {
      return DenseWorkState.startedIncomplete;
    }
  } catch (_) {
    if (File('$captureDir/$reconstructionCacheCompletionMarkerName').existsSync()) {
      return DenseWorkState.startedIncomplete;
    }
  }
  return DenseWorkState.notStarted;
}

                                                                          
                                                                      
({String recordId, String glbPath})? verifiedReconstructionCacheArtifact(
  String captureDir, {
  ReconstructionExecutionIdentityScope? identityScope,
}) {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  try {
    final proof =
        _cacheProofs[_cacheKey(_captureKey(captureDir), identityScope)];
    if (proof != null && proof.stillMatches()) {
      return (recordId: proof.recordId, glbPath: proof.publishedGlb);
    }
    denseWorkStateOf(
      captureDir,
      identityScope: identityScope,
    );                                                  
  } catch (_) {}
  return null;
}

                                                                                                   
                                              
bool denseStageEntered(String captureDir) =>
    denseWorkStateOf(captureDir) != DenseWorkState.notStarted;

                                                                                            
                                                                  
String? recaptureBlockedReason(String captureDir) =>
    switch (denseWorkStateOf(captureDir)) {
      DenseWorkState.notStarted => null,
      DenseWorkState.startedIncomplete =>
        'dense stage entered (下一步 tapped, not finished): only 完成稠密 is allowed',
      DenseWorkState.done => 'dense stage done: the capture stage is closed',
    };

                                                                                              
                                                                   
bool showRecaptureEntries({
  required bool canViewSparse,
  required bool denseEntered,
}) => !canViewSparse && !denseEntered;

                                                                                                     
                                                                                                       
                                            
void writeDenseStartedMarker(
  String captureDir, {
  required SelectionBox? selection,
  required int sparsePoints,
  bool requirePersistence = false,
  ReconstructionExecutionIdentityScope? identityScope,
}) {
  identityScope ??= ReconstructionExecutionIdentityContext.current;
  try {
    identityScope.requireCapture(captureDir);
    final f = File('$captureDir/$kDenseStartedMarkerName');
    final contents = jsonEncode(<String, Object?>{
      'schema': 'pw_dense_started/1',
      if (identityScope.isIndependentTest) ...{
        'bundle_id': identityScope.bundleId,
        'identity_scope': identityScope.toJson(),
      },
      'started_at': DateTime.now().toIso8601String(),
      'sparse_points': sparsePoints,
      'selection': selection?.toJson(),
    });
    f.writeAsStringSync(contents, flush: true);
    if (requirePersistence && f.readAsStringSync() != contents) {
      throw StateError('Dense start marker was not durably saved');
    }
  } catch (e) {
                                                                                             
    DeviceLog.log('DenseStage', 'could not write $kDenseStartedMarkerName: $e');
    if (requirePersistence) rethrow;
  }
}

                                                                                                     
                                                                                               
Future<SelectionBox?> denseResumeSelection(String captureDir) async {
  try {
    final f = File('$captureDir/$kDenseStartedMarkerName');
    if (f.existsSync()) {
      final j = jsonDecode(f.readAsStringSync());
      if (j is Map) return SelectionBox.fromJson(j['selection']);
    }
  } catch (_) {}
  return SelectionBox.loadFrom(captureDir);
}
