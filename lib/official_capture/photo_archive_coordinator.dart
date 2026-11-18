import 'dart:async';
import 'dart:collection';
import 'dart:io';

import 'archive_audit_store.dart';
import 'archive_background_scheduler.dart';
import 'aux_archive_transaction.dart';
import 'database_archive_codec.dart';
import 'database_archive_policy.dart';
import 'database_archive_preprocessor.dart';
import 'database_archive_transaction.dart';
import 'database_recipe_transaction.dart';
import 'photo_archive_codec.dart';
import 'photo_archive_policy.dart';
import 'photo_archive_transaction.dart';
import 'pwva_master.dart';
import 'transient_preview_cleanup.dart';

class PhotoArchiveActivityLease {
  PhotoArchiveActivityLease(this._onClose);

  final Future<void> Function() _onClose;
  bool _closed = false;

  Future<void> close() async {
    if (_closed) return;
    _closed = true;
    await _onClose();
  }
}

                                                                
class PhotoArchiveCoordinator {
  PhotoArchiveCoordinator({
    required this.codec,
    this.codecsByName = const <String, PhotoArchiveCodec>{},
    this.databaseCodec,
    this.databasePreprocessor,
    this.auditStore,
    this.isWorkspaceActive,
    ArchiveBackgroundScheduler? backgroundScheduler,
  }) : backgroundScheduler =
           backgroundScheduler ?? const NoopArchiveBackgroundScheduler();

  final PhotoArchiveCodec codec;
  final Map<String, PhotoArchiveCodec> codecsByName;
  final DatabaseArchiveCodec? databaseCodec;
  final DatabaseArchivePreprocessor? databasePreprocessor;
  final OfficialArchiveAuditStore? auditStore;
  final bool Function()? isWorkspaceActive;
  bool get _workspaceActive => isWorkspaceActive?.call() ?? true;
  bool _canContinue(int generation) =>
      _workspaceActive &&
      !isProductionPipelineActive &&
      generation == _interruptionGeneration;
  final ArchiveBackgroundScheduler backgroundScheduler;
  final LinkedHashMap<String, Directory> _pending =
      LinkedHashMap<String, Directory>();
  final Map<String, String> _triggerByPath = <String, String>{};
  final Set<String> _reconstructionOwners = <String>{};
  int _activeProductionPipelineCount = 0;
  int _interruptionGeneration = 0;
  Future<void>? _pumpFuture;

  bool get hasPendingWork => _pending.isNotEmpty;
  bool get isProductionPipelineActive => _activeProductionPipelineCount != 0;
  int get activeProductionPipelineCount => _activeProductionPipelineCount;

  void requestSystemInterruption() {
    _interruptionGeneration++;
    _requestPhotoCancellation();
    databaseCodec?.requestCancellation();
    databasePreprocessor?.requestCancellation();
    unawaited(
      _recordAudit(
        event: 'system_interrupted',
        trigger: 'bg_processing',
        details: const <String, Object?>{'work_remaining': true},
      ),
    );
  }

  PhotoArchiveActivityLease beginCaptureActivity() =>
      _beginProductionActivity();

                                                                           
                                                                          
                                   
  PhotoArchiveActivityLease beginProcessingActivity() =>
      _beginProductionActivity();

  PhotoArchiveActivityLease beginReconstructionActivity(
    Directory captureDirectory,
  ) => _beginProductionActivity(reconstructionDirectory: captureDirectory);

  PhotoArchiveActivityLease _beginProductionActivity({
    Directory? reconstructionDirectory,
  }) {
    _activeProductionPipelineCount++;
    if (reconstructionDirectory != null) {
      _reconstructionOwners.add(_canonicalKey(reconstructionDirectory));
    }
    _requestPhotoCancellation();
    databaseCodec?.requestCancellation();
    databasePreprocessor?.requestCancellation();
    return PhotoArchiveActivityLease(
      () => _releaseProductionActivity(
        reconstructionDirectory: reconstructionDirectory,
      ),
    );
  }

  Future<void> _releaseProductionActivity({
    Directory? reconstructionDirectory,
  }) async {
    if (_activeProductionPipelineCount > 0) {
      _activeProductionPipelineCount--;
    }
    if (reconstructionDirectory != null) {
      final path = _canonicalKey(reconstructionDirectory);
      _reconstructionOwners.remove(path);
      _pending[path] = reconstructionDirectory.absolute;
    }
    if (!_workspaceActive || isProductionPipelineActive) return;

                                                                              
                                                                            
                                                                   
    final active = _pumpFuture;
    if (active != null) await active;
    if (_workspaceActive && _pending.isNotEmpty) await _pumpQueue();
  }

  Future<void> noteArtifactsPersisted(Directory captureDirectory) async {
    if (!_workspaceActive) return;
    final key = _canonicalKey(captureDirectory);
    _pending[key] = captureDirectory.absolute;
    _triggerByPath[key] = 'artifacts_persisted';
    await _recordAudit(
      event: 'capture_enqueued',
      trigger: 'artifacts_persisted',
      captureId: _captureId(captureDirectory),
      details: const <String, Object?>{'work_remaining': true},
    );
    if (!_workspaceActive) return;
    await _scheduleBackground();
    await _pumpQueue();
  }

                                                                         
  Future<void> discoverUnderDocuments(
    Directory documentsDirectory, {
    String trigger = 'startup',
  }) async {
    if (!_workspaceActive) return;
    final captures = Directory('${documentsDirectory.path}/captures_official');
    await _recordAudit(
      event: 'scan_started',
      trigger: trigger,
      details: const <String, Object?>{'work_remaining': true},
    );
    try {
      if (!await captures.exists()) {
        await _recordAudit(
          event: 'scan_completed',
          trigger: trigger,
          details: const <String, Object?>{
            'candidates': 0,
            'work_remaining': false,
          },
        );
        return;
      }
      final directories = <Directory>[];
      await for (final entity in captures.list(followLinks: false)) {
        if (entity is Directory) directories.add(entity);
      }
      directories.sort((left, right) => left.path.compareTo(right.path));
      for (final capture in directories) {
        if (!_workspaceActive) return;
        final photoEligible =
            await PhotoArchivePolicy.readCompatible(capture) != null;
        final databaseEligible =
            await DatabaseArchivePolicy.readCompatible(capture) != null;
        if (!_workspaceActive) return;
        if (!photoEligible && !databaseEligible) {
          continue;
        }
        final key = _canonicalKey(capture);
        _pending[key] = capture.absolute;
        _triggerByPath[key] = trigger;
      }
      if (_workspaceActive && _pending.isNotEmpty) await _scheduleBackground();
      await _pumpQueue();
      await _recordAudit(
        event: 'scan_completed',
        trigger: trigger,
        details: <String, Object?>{
          'candidates': directories.length,
          'work_remaining': _pending.isNotEmpty,
        },
      );
    } on FileSystemException {
                                                                 
    }
  }

  Future<void> waitForIdle() async {
    final active = _pumpFuture;
    if (active != null) await active;
  }

  Future<void> _pumpQueue() {
    if (!_workspaceActive) return Future<void>.value();
    final existing = _pumpFuture;
    if (existing != null) return existing;
    late final Future<void> started;
    started = _runPump().whenComplete(() async {
      if (_pending.isEmpty) {
        await _recordAudit(
          event: 'queue_drained',
          trigger: 'coordinator',
          details: const <String, Object?>{'work_remaining': false},
        );
        await _cancelScheduledBackground();
      }
      if (identical(_pumpFuture, started)) _pumpFuture = null;
    });
    _pumpFuture = started;
    return started;
  }

  Future<void> _runPump() async {
    final runGeneration = _interruptionGeneration;
    while (_pending.isNotEmpty && _canContinue(runGeneration)) {
      final item = _pending.entries.first;
      _pending.remove(item.key);
      final trigger = _triggerByPath.remove(item.key) ?? 'coordinator';
      if (_reconstructionOwners.contains(item.key)) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        return;
      }
      final ready = await _isDurablyReady(item.value);
      if (!_canContinue(runGeneration)) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        return;
      }
      if (!ready) {
        await _recordAudit(
          event: 'capture_not_ready',
          trigger: trigger,
          captureId: _captureId(item.value),
          details: const <String, Object?>{'work_remaining': false},
        );
        continue;
      }
      await _recordAudit(
        event: 'capture_started',
        trigger: trigger,
        captureId: _captureId(item.value),
        details: const <String, Object?>{'work_remaining': true},
      );
                                             
                                            
                                                         
                                                    
                                                   
                                                     
      if (await _pwvaTranscodeInFlight(item.value)) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        await _recordAudit(
          event: 'capture_not_ready',
          trigger: trigger,
          captureId: _captureId(item.value),
          details: const <String, Object?>{
            'reason': 'pwva_transcode_in_flight',
            'work_remaining': true,
          },
        );
        continue;
      }
      await removeTransientCapturePreviews(
        item.value,
        canContinue: () => _canContinue(runGeneration),
      );
                                                   
                                                    
                                     
      final master = await PwvaMasterTransaction(
        canContinue: () => _canContinue(runGeneration),
      ).masterCapture(item.value);
      if (master.paused ||
          !_canContinue(runGeneration) ||
          master.failedNames.isNotEmpty) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        await _recordAudit(
          event: master.paused ? 'capture_paused' : 'capture_retry_required',
          trigger: trigger,
          captureId: _captureId(item.value),
          details: <String, Object?>{
            'photos_pwva_mastered': master.masteredNames.length,
            'photos_pwva_failed': master.failedNames.length,
            'work_remaining': true,
          },
        );
        return;
      }
      final result = await PhotoArchiveTransaction(
        codec: codec,
        codecsByName: codecsByName,
        canStartNext: () => _canContinue(runGeneration),
      ).archiveCapture(item.value);
      if (result.paused ||
          !_canContinue(runGeneration) ||
          result.failedNames.isNotEmpty) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        await _recordAudit(
          event: result.paused ? 'capture_paused' : 'capture_retry_required',
          trigger: trigger,
          captureId: _captureId(item.value),
          details: <String, Object?>{
            'photos_archived': result.archivedNames.length,
            'photos_failed': result.failedNames.length,
            'work_remaining': true,
          },
        );
        return;
      }
                                                       
                                                          
                                                                 
      final recipe = await DatabaseRecipeTransaction(
        canContinue: () => _canContinue(runGeneration),
      ).recipeCapture(item.value);
      if (!_canContinue(runGeneration)) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        return;
      }
      final database = databaseCodec;
      DatabaseArchiveRunResult? databaseResult;
      if (database != null && _canContinue(runGeneration)) {
        databaseResult = await DatabaseArchiveTransaction(
          codec: database,
          preprocessor: databasePreprocessor,
          canContinue: () => _canContinue(runGeneration),
        ).archiveCapture(item.value);
        if (databaseResult.interrupted ||
            databaseResult.failed ||
            !_canContinue(runGeneration)) {
          _pending[item.key] = item.value;
          _triggerByPath[item.key] = trigger;
          await _recordAudit(
            event: databaseResult.interrupted
                ? 'capture_paused'
                : 'capture_retry_required',
            trigger: trigger,
            captureId: _captureId(item.value),
            details: <String, Object?>{
              'photos_archived': result.archivedNames.length,
              'database_interrupted': databaseResult.interrupted,
              'database_failed': databaseResult.failed,
              'work_remaining': true,
            },
          );
          return;
        }
      }
                                                
                                            
                                          
      AuxArchiveRunResult? aux;
      final auxCodec = databaseCodec;
      if (auxCodec != null && _canContinue(runGeneration)) {
        aux = await AuxArchiveTransaction(
          codec: auxCodec,
          canContinue: () => _canContinue(runGeneration),
        ).archiveCapture(item.value);
      }
      if (!_canContinue(runGeneration) || aux?.interrupted == true) {
        _pending[item.key] = item.value;
        _triggerByPath[item.key] = trigger;
        return;
      }
      await _recordAudit(
        event: 'capture_completed',
        trigger: trigger,
        captureId: _captureId(item.value),
        details: <String, Object?>{
          'photos_archived': result.archivedNames.length,
          'photos_skipped': result.skippedNames.length,
          'photos_failed': result.failedNames.length,
          if (master.applicable)
            'photos_pwva_mastered': master.masteredNames.length,
          if (recipe.applicable) 'database_recipe_committed': recipe.committed,
          if (recipe.applicable)
            'database_recipe_deleted_bytes': recipe.deletedBytes,
                                         
          if (!recipe.applicable && recipe.reason != null)
            'database_recipe_skip_reason': recipe.reason,
          if (databaseResult != null)
            'database_archived': databaseResult.archived,
          if (databaseResult != null)
            'database_skipped': databaseResult.skipped,
          if (aux != null && aux.committedBundles.isNotEmpty)
            'aux_bundles': aux.committedBundles,
          if (aux != null && aux.committedBundles.isNotEmpty)
            'aux_deleted_bytes': aux.deletedBytes,
          if (aux != null && aux.committedBundles.isNotEmpty)
            'aux_archive_bytes': aux.archiveBytes,
          if (aux != null && (aux.failed || !aux.applicable))
            'aux_skip_reason': aux.failed ? 'failed' : aux.reason,
          'work_remaining': _pending.isNotEmpty,
        },
      );
    }
  }

  void _requestPhotoCancellation() {
    final codecs = HashSet<PhotoArchiveCodec>.identity()
      ..add(codec)
      ..addAll(codecsByName.values);
    for (final photoCodec in codecs) {
      photoCodec.requestCancellation();
    }
  }

                                       
  Future<bool> _pwvaTranscodeInFlight(Directory captureDirectory) async {
    try {
      final stream = File('${captureDirectory.path}/photos_hevc/photos.hevc');
      if (!await stream.exists()) return false;
      final report = File(
        '${captureDirectory.path}/photos_hevc/archive-report.json',
      );
      if (await report.exists()) return false;
      final age = DateTime.now().difference(await stream.lastModified());
      return age < const Duration(minutes: 10);
    } on FileSystemException {
      return false;
    }
  }

  Future<bool> _isDurablyReady(Directory captureDirectory) async {
    final photoEligible =
        await PhotoArchivePolicy.readCompatible(captureDirectory) != null;
    final databaseEligible =
        await DatabaseArchivePolicy.readCompatible(captureDirectory) != null;
    if (!photoEligible && !databaseEligible) {
      return false;
    }
    for (final relativePath in const <String>[
      'official_photo_bundle.json',
      'official_sfm_sparse.ply',
      'official_sfm_sparse_meta.json',
    ]) {
      final file = File('${captureDirectory.path}/$relativePath');
      try {
        if (!await file.exists() || await file.length() == 0) return false;
      } on FileSystemException {
        return false;
      }
    }
    return true;
  }

  String _canonicalKey(Directory directory) => directory.absolute.path;

  String _captureId(Directory directory) =>
      directory.path.split(Platform.pathSeparator).last;

  Future<void> _scheduleBackground() async {
    try {
      await backgroundScheduler.schedule();
    } catch (_) {
                                                                            
    }
  }

  Future<void> _cancelScheduledBackground() async {
    try {
      await backgroundScheduler.cancelScheduled();
    } catch (_) {
                                                                           
    }
  }

  Future<void> _recordAudit({
    required String event,
    required String trigger,
    String? captureId,
    Map<String, Object?> details = const <String, Object?>{},
  }) async {
    final store = auditStore;
    if (store == null) return;
    try {
      await store.record(
        ArchiveAuditEvent(
          timestamp: DateTime.now().toUtc(),
          event: event,
          trigger: trigger,
          captureId: captureId,
          details: details,
        ),
      );
    } catch (_) {
                                                                            
    }
  }
}
