import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:crypto/crypto.dart';
import 'package:flutter/foundation.dart';
import 'package:image/image.dart' as img;
import 'package:path_provider/path_provider.dart';

import '../dense/dense_stage_progress.dart';
import '../dense/dense_work_state.dart'
    show kDenseStartedMarkerName, kDenseWorkDirName;
import '../reconstruction/cache_completion.dart' show reconstructionCacheCompletionMarkerName;
import '../reconstruction/pending_capture_retirement.dart';
import '../ui/scan_record.dart';
import '../util/device_log.dart';
import '../util/image_sanitize.dart';
import 'work_identity.dart';

Uint8List? _uprightLandscapeThumbnailBytes(String path) {
  final decoded = img.decodeImage(File(path).readAsBytesSync());
  if (decoded == null) return null;

  var image = img.bakeOrientation(decoded);
  if (image.width <= image.height) return null;

  image = img.copyRotate(image, angle: 90);
  const maxEdge = 1024;
  final longEdge = math.max(image.width, image.height);
  if (longEdge > maxEdge) {
    image = img.copyResize(
      image,
      width: image.width >= image.height ? maxEdge : null,
      height: image.height > image.width ? maxEdge : null,
      interpolation: img.Interpolation.average,
    );
  }

  return encodeSanitizedJpg(image, quality: 88);
}

@immutable
final class ScanResultViewRevision {
  const ScanResultViewRevision._(this.record, this.readyAt, this.fileSignature);
  final ScanRecord record;
  final DateTime readyAt;
  final String fileSignature;
}

class _ProjectDeletionLog {
  _ProjectDeletionLog(this.workId, this.source)
    : operationId = '${DateTime.now().microsecondsSinceEpoch}-${_nextId++}';

  static int _nextId = 0;
  final String workId;
  final String source;
  final String operationId;

  void event(String event, {Map<String, Object?> details = const {}}) {
    try {
      final progress = denseStageProgress.value;
      DeviceLog.log(
        'ProjectLifecycle',
        jsonEncode({
          'operation_id': operationId,
          'work_id': workId,
          'source': source,
          'event': event,
          'time': DateTime.now().toUtc().toIso8601String(),
          'dense_progress': progress == null
              ? 'none'
              : {
                  'phase': progress.phase,
                  'state': progress.state.name,
                  'capture_dir': progress.captureDir,
                  'started_at': progress.startedAt?.toUtc().toIso8601String(),
                },
          ...details,
        }),
      );
    } catch (_) {}
  }
}

final class LocalWorkspace {
  const LocalWorkspace();

  String get namespace =>
      'e52ce40224bdf87c16f5ccafd5b389c96e5a0ded66d93c610bb1e307af226795';
}

String sparsePlyFileNameForPipeline(CapturePipelineKind kind) => switch (kind) {
  CapturePipelineKind.self => 'sfm_sparse.ply',
  CapturePipelineKind.official => 'official_sfm_sparse.ply',
};

class ScanRecordStore {
  ScanRecordStore.local(Directory? documentsDirectory)
    : _documentsDirectoryOverride = documentsDirectory;
  static final ScanRecordStore instance = ScanRecordStore.local(null);

  final Directory? _documentsDirectoryOverride;
  final String Function() _captureIdentifier = newCaptureWorkId;
  static const _legacyFixture = false;
  final workspace = const LocalWorkspace();
  bool _revoked = false;
  Directory? _resolvedDocumentsDirectory;
  Directory? _baseDocumentsDirectory;
  bool get isActive => !_revoked;
  final Set<VoidCallback> _revocationListeners = {};

  VoidCallback addRevocationListener(VoidCallback listener) {
    if (!isActive) {
      listener();
      return () {};
    }
    _revocationListeners.add(listener);
    return () => _revocationListeners.remove(listener);
  }

  void _notifyRevoked() {
    final listeners = List<VoidCallback>.of(_revocationListeners);
    _revocationListeners.clear();
    for (final listener in listeners) {
      try {
        listener();
      } catch (error) {
        debugPrint('[ScanRecordStore] revocation callback failed: $error');
      }
    }
  }

  List<ScanRecord> _records = const <ScanRecord>[];
  Set<String> _deletionTombstones = const <String>{};
  Future<void>? _loadFuture;
  Object? _loadFailure;

  final _ctrl = StreamController<List<ScanRecord>>.broadcast();
  Future<void> _writeLock = Future<void>.value();

  List<ScanRecord> get records =>
      isActive ? List<ScanRecord>.unmodifiable(_records) : const [];

  Stream<List<ScanRecord>> get changes =>
      _ctrl.stream.map((snapshot) => isActive ? snapshot : const []);

  Future<void> ensureLoaded() async {
    _checkActive();
    await (_loadFuture ??= _load());
    _checkActive();
    if (!_legacyFixture) _ensureWritable();
  }

  Future<void> _load() async {
    const accountPathRewrites = 0;
    try {
      final file = await _storeFile();
      if (await file.exists()) {
        final raw = await file.readAsString();
        if (raw.isNotEmpty) {
          final list = (jsonDecode(raw) as List).cast<Map<String, dynamic>>();
          final parsed = list.map(_recordFromJson).toList(growable: false);
          if (!_legacyFixture) {
            final validated = <ScanRecord>[];
            final ids = <String>{};
            final names = <String>{};
            for (final record in parsed) {
              if (record.name.trim().isEmpty ||
                  record.name.length > 256 ||
                  !ids.add(record.id) ||
                  !names.add(workNameKey(record.name))) {
                throw StateError(
                  'Account library contains conflicting identities or names',
                );
              }
              _requireOwner(record);
              await _ensureRouteOwnedPaths(record);
              validated.add(record);
            }
            _checkActive();
            _records = validated;
          } else {
            _records = parsed;
          }
        }
      }
    } catch (e, st) {
      debugPrint('[ScanRecordStore] load failed: $e\n$st');
      _loadFailure = e;
      _records = const <ScanRecord>[];
      _emit();
      return;
    }
    try {
      _deletionTombstones = await _loadDeletionTombstones();
    } catch (e, st) {
      debugPrint('[ScanRecordStore] deletion tombstone load failed: $e\n$st');
      _loadFailure = e;
      _records = const <ScanRecord>[];
      _emit();
      return;
    }
    final retained = <ScanRecord>[];
    var deletedRecordsPurged = 0;
    for (final record in _records) {
      if (!_isPermanentlyDeleted(record.id, record.pipelineKind)) {
        retained.add(record);
        continue;
      }
      deletedRecordsPurged++;
      try {
        await _deleteProjectFiles(
          record,
          source: 'scan_record_store.startup_tombstone_replay',
        );
      } catch (e, st) {
        debugPrint(
          '[ScanRecordStore] tombstoned project cleanup deferred: $e\n$st',
        );
      }
    }
    _records = retained;
    final recovered = await _recoverOrphanCaptures();

    final patched = <ScanRecord>[];

    var rewrites = accountPathRewrites;
    for (final r in _records) {
      var rec = r;
      if (rec.thumbnailPath != null && rec.thumbnailPath!.isNotEmpty) {
        final crossesRoute = _crossesPipelineNamespace(
          rec.thumbnailPath!,
          pipelineKind: rec.pipelineKind,
        );
        if (!File(rec.thumbnailPath!).existsSync() || crossesRoute) {
          final fresh = await thumbnailFileFor(
            rec.id,
            pipelineKind: rec.pipelineKind,
          );
          if (await fresh.exists()) {
            rec = rec.copyWith(thumbnailPath: fresh.path);
            rewrites++;
          } else if (crossesRoute) {
            rec = rec.copyWith(clearThumbnailPath: true);
            rewrites++;
          }
        }
      }
      if (rec.artifactPath != null && rec.artifactPath!.isNotEmpty) {
        final p = rec.artifactPath!.startsWith('file://')
            ? rec.artifactPath!.substring(7)
            : rec.artifactPath!;
        final crossesRoute = _crossesPipelineNamespace(
          p,
          pipelineKind: rec.pipelineKind,
        );
        if (!File(p).existsSync() || crossesRoute) {
          final fresh = await glbFileFor(
            rec.id,
            pipelineKind: rec.pipelineKind,
          );
          if (await fresh.exists()) {
            rec = rec.copyWith(artifactPath: 'file://${fresh.path}');
            rewrites++;
          } else if (crossesRoute) {
            rec = rec.copyWith(clearArtifactPath: true);
            rewrites++;
          }
        }
      }
      final captureMetadata = <String?>[
        rec.captureDir,
        rec.photosDir,
        rec.captureManifestPath,
      ].whereType<String>().where((path) => path.isNotEmpty).toList();
      if (captureMetadata.isNotEmpty) {
        final routeRoot = await _capturesRoot(pipelineKind: rec.pipelineKind);
        final suppliedCaptureDir = rec.captureDir == null
            ? null
            : Directory(rec.captureDir!);
        final allPathsOwned = captureMetadata.every(
          (path) =>
              !_crossesPipelineNamespace(
                path,
                pipelineKind: rec.pipelineKind,
              ) &&
              _isWithinDirectory(path, routeRoot.path),
        );
        var captureDir = allPathsOwned && suppliedCaptureDir != null
            ? suppliedCaptureDir
            : await captureDirFor(rec.id, pipelineKind: rec.pipelineKind);
        if (!await captureDir.exists()) {
          captureDir = await captureDirFor(
            rec.id,
            pipelineKind: rec.pipelineKind,
          );
        }
        if (await captureDir.exists() &&
            _isWithinDirectory(captureDir.path, routeRoot.path)) {
          final paths = await _capturePaths(
            captureDir,
            pipelineKind: rec.pipelineKind,
          );
          if (rec.captureDir != captureDir.path ||
              rec.photosDir != paths.photosDir.path ||
              rec.captureManifestPath != paths.manifestFile.path) {
            rec = rec.copyWith(
              captureDir: captureDir.path,
              photosDir: paths.photosDir.path,
              captureManifestPath: paths.manifestFile.path,
            );
            rewrites++;
          }
        } else {
          rec = _withoutCaptureMetadata(rec);
          rewrites++;
        }
      }
      if (rec.captureDir != null &&
          rec.thumbnailPath != null &&
          rec.thumbnailPath!.isNotEmpty) {
        final thumbnail = File(rec.thumbnailPath!);
        if (await thumbnail.exists() &&
            await _normalizePortraitCaptureThumbnail(thumbnail)) {
          rewrites++;
        }
      }
      patched.add(rec);
    }
    _records = _sortNewestFirst(patched);
    if (rewrites > 0 || recovered > 0 || deletedRecordsPurged > 0) {
      debugPrint(
        '[ScanRecordStore] re-anchored $rewrites stale path(s), '
        'recovered $recovered orphan capture(s), '
        'purged $deletedRecordsPurged deleted record(s)',
      );

      await _flush();
    }

    await _markExistingSparseAsViewed();
    _emit();
  }

  Future<bool> _normalizePortraitCaptureThumbnail(File thumbnail) async {
    try {
      _guardPath(thumbnail.path);
      final bytes = await compute(
        _uprightLandscapeThumbnailBytes,
        thumbnail.path,
        debugLabel: 'scan-record-thumbnail-upright',
      );
      _guardPath(thumbnail.path);
      if (bytes == null) return false;
      await thumbnail.writeAsBytes(bytes, flush: true);
      _checkActive();
      return true;
    } catch (e) {
      debugPrint('[ScanRecordStore] thumbnail normalize skipped: $e');
      return false;
    }
  }

  Future<int> _recoverOrphanCaptures() async {
    try {
      final root = await _documentsDirectory();
      final existingIds = _records.map((r) => r.id).toSet();
      final recovered = <ScanRecord>[];
      final roots = <(Directory, CapturePipelineKind)>[
        (Directory('${root.path}/captures'), CapturePipelineKind.self),
        (
          Directory('${root.path}/captures_official'),
          CapturePipelineKind.official,
        ),
      ];
      for (final (capturesRoot, pipelineKind) in roots) {
        _guardPath(capturesRoot.path);
        if (!await capturesRoot.exists()) continue;
        await for (final entity in capturesRoot.list(followLinks: false)) {
          _checkActive();
          if (entity is! Directory) continue;
          _guardPath(entity.path);
          final captureId = entity.uri.pathSegments
              .where((s) => s.isNotEmpty)
              .lastOrNull;
          if (captureId == null) continue;
          if (_isPermanentlyDeleted(captureId, pipelineKind)) {
            try {
              await _deleteProjectFiles(
                ScanRecord(
                  id: captureId,
                  accountNamespace: workspace.namespace,
                  name: '',
                  createdAt: DateTime.fromMillisecondsSinceEpoch(0),
                  pipelineKind: pipelineKind,
                ),
                source: 'scan_record_store.orphan_tombstone_replay',
              );
            } catch (e, st) {
              debugPrint(
                '[ScanRecordStore] late deleted capture cleanup deferred: '
                '$e\n$st',
              );
            }
            continue;
          }
          if (existingIds.contains(captureId)) continue;
          _validateId(captureId);
          final paths = await _capturePaths(entity, pipelineKind: pipelineKind);
          final photosDir = paths.photosDir;
          if (!await photosDir.exists()) continue;
          final photos = <File>[];
          await for (final photoEntity in photosDir.list(followLinks: false)) {
            _checkActive();
            if (photoEntity is! File) continue;
            _guardPath(photoEntity.path);
            final path = photoEntity.path.toLowerCase();
            if (!path.endsWith('.jpg') && !path.endsWith('.jpeg')) continue;
            final metadata = File(
              photoEntity.path.replaceFirst(RegExp(r'\.[^.]+$'), '.json'),
            );
            _guardPath(metadata.path);
            if (await metadata.exists()) photos.add(photoEntity);
          }
          if (photos.isEmpty) continue;
          photos.sort((a, b) => a.path.compareTo(b.path));
          final manifestFile = paths.manifestFile;
          final stat = await entity.stat();
          final createdAt = stat.modified;
          if (!await manifestFile.exists()) {
            await _writeRecoveredCaptureManifest(
              manifestFile: manifestFile,
              captureId: captureId,
              createdAt: createdAt,
              captureDir: entity,
              photosDir: photosDir,
              photos: photos,
              pipelineKind: pipelineKind,
            );
          }
          String? thumbnailPath;
          try {
            final thumbnail = await thumbnailFileFor(
              captureId,
              pipelineKind: pipelineKind,
            );
            await thumbnail.parent.create(recursive: true);
            _guardPath(thumbnail.path);
            _guardPath(photos.first.path);
            await photos.first.copy(thumbnail.path);
            _checkActive();
            thumbnailPath = thumbnail.path;
          } on FileSystemException {
            thumbnailPath = photos.first.path;
          }
          recovered.add(
            ScanRecord(
              id: captureId,
              accountNamespace: workspace.namespace,
              name: nextUntitledScanName([
                ..._records.map((r) => r.name),
                ...recovered.map((r) => r.name),
              ]),
              createdAt: createdAt,
              pipelineKind: pipelineKind,
              thumbnailPath: thumbnailPath,
              captureDir: entity.path,
              photosDir: photosDir.path,
              captureManifestPath: manifestFile.path,
              photoCount: photos.length,
              localRawRetainedForDebug: true,
            ),
          );
          existingIds.add(captureId);
        }
      }
      if (recovered.isEmpty) return 0;
      _checkActive();
      _records = _sortNewestFirst([..._records, ...recovered]);
      return recovered.length;
    } catch (e, st) {
      debugPrint('[ScanRecordStore] orphan capture recovery failed: $e\n$st');
      return 0;
    }
  }

  Future<({Directory photosDir, File manifestFile})> _capturePaths(
    Directory captureDir, {
    required CapturePipelineKind pipelineKind,
  }) async {
    final photosHighres = Directory('${captureDir.path}/photos_highres');
    _guardPath(photosHighres.path);
    final photosDir = await photosHighres.exists()
        ? photosHighres
        : Directory('${captureDir.path}/photos');
    final preferredManifest = pipelineKind == CapturePipelineKind.official
        ? File('${captureDir.path}/official_photo_bundle.json')
        : File('${captureDir.path}/photo_bundle.json');
    final legacyManifest = pipelineKind == CapturePipelineKind.official
        ? File('${captureDir.path}/official_capture_manifest.json')
        : File('${captureDir.path}/capture_manifest.json');
    _guardPath(photosDir.path);
    _guardPath(preferredManifest.path);
    _guardPath(legacyManifest.path);
    final manifestFile = await preferredManifest.exists()
        ? preferredManifest
        : legacyManifest;
    _checkActive();
    return (photosDir: photosDir, manifestFile: manifestFile);
  }

  Future<void> _writeRecoveredCaptureManifest({
    required File manifestFile,
    required String captureId,
    required DateTime createdAt,
    required Directory captureDir,
    required Directory photosDir,
    required List<File> photos,
    required CapturePipelineKind pipelineKind,
  }) async {
    final frames = <Map<String, Object?>>[];
    for (final photo in photos) {
      final metadata = File(
        photo.path.replaceFirst(RegExp(r'\.[^.]+$'), '.json'),
      );
      frames.add(<String, Object?>{
        'image_path': photo.path,
        'metadata_path': metadata.path,
        'image_file': photo.uri.pathSegments.last,
        'metadata_file': metadata.uri.pathSegments.last,
      });
    }
    final manifest = <String, Object?>{
      'schema': 'worldswithin.capture_manifest.v1',
      'capture_id': captureId,
      'pipeline_kind': pipelineKind.wireName,
      'created_at': createdAt.toIso8601String(),
      'capture_dir': captureDir.path,
      'photos_dir': photosDir.path,
      'photo_count': photos.length,
      'frames': frames,
      'recovered_from_orphan_capture': true,
    };
    _guardPath(manifestFile.path);
    await manifestFile.writeAsString(jsonEncode(manifest), flush: true);
    _checkActive();
  }

  DateTime? sparseReadyAt(ScanRecord r) {
    _checkActive();
    _requireOwner(r);
    final dir = r.captureDir;
    if (dir == null) return null;
    DateTime? readyAt(String filename) {
      final f = File('$dir/$filename');
      _guardPath(f.path);
      try {
        final st = f.statSync();
        if (st.type == FileSystemEntityType.notFound || st.size <= 0) {
          return null;
        }
        return st.modified;
      } catch (_) {
        return null;
      }
    }

    return readyAt(sparsePlyFileNameForPipeline(r.pipelineKind));
  }

  DateTime? resultReadyAt(ScanRecord r) {
    _checkActive();
    _requireOwner(r);
    final artifact = r.artifactPath;
    if (artifact != null && artifact.isNotEmpty) {
      final uri = Uri.tryParse(artifact);
      if (uri == null || (uri.hasScheme && uri.scheme != 'file')) return null;
      final path = uri.scheme == 'file' ? uri.toFilePath() : artifact;
      _guardPath(path);
      try {
        final stat = File(path).statSync();
        return stat.type == FileSystemEntityType.file && stat.size > 0
            ? stat.modified
            : null;
      } on FileSystemException {
        return null;
      }
    }
    final capture = r.captureDir;
    if (r.pipelineKind == CapturePipelineKind.official && capture != null) {
      for (final name in [
        kDenseStartedMarkerName,
        reconstructionCacheCompletionMarkerName,
      ]) {
        final path = '$capture/$name';
        _guardPath(path);
        if (File(path).existsSync()) return null;
      }
      final work = '$capture/$kDenseWorkDirName';
      _guardPath(work);
      if (Directory(work).existsSync()) return null;
    }
    return sparseReadyAt(r);
  }

  ScanResultViewRevision? captureResultViewRevision(ScanRecord record) {
    final ready = resultReadyAt(record);
    if (ready == null) return null;
    return ScanResultViewRevision._(
      record,
      ready,
      _resultViewSignature(record),
    );
  }

  String _resultViewSignature(ScanRecord record) {
    final artifact = record.artifactPath;
    final path = artifact == null
        ? '${record.captureDir}/${sparsePlyFileNameForPipeline(record.pipelineKind)}'
        : (Uri.tryParse(artifact)?.scheme == 'file'
              ? Uri.parse(artifact).toFilePath()
              : artifact);
    _guardPath(path);
    final stat = File(path).statSync();
    return '$path:${stat.type}:${stat.size}:'
        '${stat.modified.microsecondsSinceEpoch}:'
        '${stat.changed.microsecondsSinceEpoch}:${stat.mode}';
  }

  ScanProcessingBadge badgeOf(
    ScanRecord r, {
    String? activeReconstructionCaptureDir,
  }) => r.badgeFor(
    resultReadyAt(r),
    isActivelyReconstructing:
        activeReconstructionCaptureDir != null &&
        r.captureDir != null &&
        _matchesActiveCapture(activeReconstructionCaptureDir, r.captureDir!),
  );

  bool _matchesActiveCapture(String activePath, String recordPath) {
    if (_legacyFixture) return _sameDir(activePath, recordPath);
    try {
      _guardPath(activePath);
      String normalized(String path) =>
          _normalizedFilePath(path).replaceFirst(RegExp(r'/+$'), '');
      return normalized(activePath) == normalized(recordPath);
    } catch (_) {
      return false;
    }
  }

  static bool _sameDir(String a, String b) {
    String tail(String p) {
      final parts = p.split('/').where((e) => e.isNotEmpty).toList();
      return parts.isEmpty ? p : parts.last;
    }

    return tail(a) == tail(b);
  }

  Future<void> markResultViewed(
    ScanRecord r, {
    ScanResultViewRevision? revision,
  }) async {
    final intended = revision ?? captureResultViewRevision(r);
    if (intended == null) return;
    final source = intended.record;
    if (source.id != r.id ||
        source.accountNamespace != r.accountNamespace ||
        source.pipelineKind != r.pipelineKind ||
        source.captureDir != r.captureDir ||
        source.artifactPath != r.artifactPath) {
      return;
    }
    final intendedReady = intended.readyAt;
    await ensureLoaded();
    _requireOwner(r);
    await _ensureRouteOwnedPaths(r);
    final previousWrite = _writeLock;
    final completeWrite = Completer<void>();
    _writeLock = completeWrite.future;
    try {
      await previousWrite;
      _ensureWritable();

      final current = byId(r.id);
      if (current == null ||
          current.pipelineKind != r.pipelineKind ||
          current.captureDir != r.captureDir ||
          current.artifactPath != r.artifactPath ||
          _isPermanentlyDeleted(r.id, r.pipelineKind)) {
        return;
      }
      final ready = resultReadyAt(current);
      if (ready == null ||
          !ready.isAtSameMomentAs(intendedReady) ||
          _resultViewSignature(current) != intended.fileSignature) {
        return;
      }
      final cur = current.resultViewedAt;
      if (cur != null && !cur.isBefore(ready)) return;
      _records = [
        for (final record in _records)
          record.id == r.id ? current.copyWith(resultViewedAt: ready) : record,
      ];
      _emit();
    } finally {
      completeWrite.complete();
    }
    await _flush();
  }

  Future<bool> attachPreviewThumbnail({
    required ScanRecord expected,
    required String thumbnailPath,
    required bool Function() isCurrent,
    String? finalCoverSha256,
    String? finalCoverGlbSha256,
    String? finalCoverCameraVersion,
  }) async {
    await ensureLoaded();
    _requireOwner(expected);
    bool sameSource(ScanRecord? current) =>
        isCurrent() &&
        current != null &&
        current.accountNamespace == expected.accountNamespace &&
        current.pipelineKind == expected.pipelineKind &&
        current.captureDir == expected.captureDir &&
        current.artifactPath == expected.artifactPath &&
        !_isPermanentlyDeleted(expected.id, expected.pipelineKind);
    if (finalCoverSha256 != null || finalCoverGlbSha256 != null) {
      if (finalCoverSha256 == null ||
          !RegExp(r'^[0-9a-f]{64}$').hasMatch(finalCoverSha256) ||
          finalCoverGlbSha256 != expected.finalGlbSha256 ||
          finalCoverGlbSha256 == null) {
        throw StateError('Final cover proof does not match final GLB');
      }
    }
    final initial = byId(expected.id);
    if (!sameSource(initial)) return false;
    await _ensureRouteOwnedPaths(
      initial!.copyWith(thumbnailPath: thumbnailPath),
    );
    final previousWrite = _writeLock;
    final completeWrite = Completer<void>();
    _writeLock = completeWrite.future;
    var attached = false;
    try {
      await previousWrite;
      _ensureWritable();
      _requireOwner(expected);
      _guardPath(thumbnailPath);
      final durableFile = finalCoverSha256 == null ? null : await _storeFile();
      final current = byId(expected.id);
      if (!sameSource(current)) return false;
      if (FileSystemEntity.typeSync(thumbnailPath, followLinks: false) !=
              FileSystemEntityType.file ||
          File(thumbnailPath).lengthSync() <= 0) {
        return false;
      }
      final next = [
        for (final record in _records)
          record.id == expected.id
              ? current!.copyWith(
                  thumbnailPath: thumbnailPath,
                  finalCoverSha256: finalCoverSha256,
                  finalCoverGlbSha256: finalCoverGlbSha256,
                  finalCoverCameraVersion: finalCoverCameraVersion,
                  clearFinalCoverProof: finalCoverSha256 == null,
                )
              : record,
      ];
      if (durableFile != null) {
        _guardPath(durableFile.path);
        final staged = File('${durableFile.path}.final-cover-pending');
        _guardPath(staged.path);
        final encoded = jsonEncode(next.map(_recordToJson).toList());
        staged.writeAsStringSync(encoded, flush: true);
        if (staged.readAsStringSync() != encoded) {
          throw StateError('Final cover manifest staging changed');
        }
        staged.renameSync(durableFile.path);
        if (durableFile.readAsStringSync() != encoded) {
          throw StateError('Final cover manifest commit changed');
        }
      }
      _records = next;
      attached = true;
      _emit();
    } finally {
      completeWrite.complete();
    }
    if (attached) {
      if (finalCoverSha256 == null) await _flush();
      if (finalCoverSha256 != null) {
        final persisted =
            jsonDecode(await (await _storeFile()).readAsString()) as List;
        final saved = persisted
            .whereType<Map>()
            .where((r) => r['id'] == expected.id)
            .single;
        if (!sameSource(byId(expected.id)) ||
            saved['thumbnailPath'] != thumbnailPath ||
            saved['finalCoverSha256'] != finalCoverSha256 ||
            saved['finalCoverGlbSha256'] != finalCoverGlbSha256 ||
            saved['finalCoverCameraVersion'] != finalCoverCameraVersion ||
            saved['finalGlbSha256'] != finalCoverGlbSha256 ||
            (await sha256.bind(File(thumbnailPath).openRead()).first)
                    .toString() !=
                finalCoverSha256) {
          throw StateError('Final cover publication was not durable');
        }
      }
    }
    return attached;
  }

  Future<bool> attachFinalMeshThumbnail({
    required ScanResultViewRevision revision,
    required String thumbnailPath,
  }) async {
    final expected = revision.record;
    await ensureLoaded();
    _requireOwner(expected);
    bool sameGeneration(ScanRecord? current) =>
        current != null &&
        current.finalGlbSha256 == null &&
        current.finalGlbBytes == null &&
        current.finalCoverSha256 == null &&
        current.finalCoverGlbSha256 == null &&
        current.accountNamespace == expected.accountNamespace &&
        current.pipelineKind == expected.pipelineKind &&
        current.captureDir == expected.captureDir &&
        current.artifactPath != null &&
        current.artifactPath == expected.artifactPath &&
        !_isPermanentlyDeleted(expected.id, expected.pipelineKind);
    final initial = byId(expected.id);
    if (!sameGeneration(initial)) return false;
    await _ensureRouteOwnedPaths(
      initial!.copyWith(thumbnailPath: thumbnailPath),
    );
    final previousWrite = _writeLock;
    final completeWrite = Completer<void>();
    _writeLock = completeWrite.future;
    var attached = false;
    try {
      await previousWrite;
      _ensureWritable();
      _requireOwner(expected);
      _guardPath(thumbnailPath);
      final current = byId(expected.id);
      if (!sameGeneration(current)) return false;
      final ready = resultReadyAt(current!);
      if (ready == null ||
          !ready.isAtSameMomentAs(revision.readyAt) ||
          _resultViewSignature(current) != revision.fileSignature) {
        return false;
      }
      if (FileSystemEntity.typeSync(thumbnailPath, followLinks: false) !=
              FileSystemEntityType.file ||
          File(thumbnailPath).lengthSync() <= 0) {
        return false;
      }
      if (current.thumbnailPath == thumbnailPath) return true;
      _records = [
        for (final record in _records)
          record.id == expected.id
              ? current.copyWith(thumbnailPath: thumbnailPath)
              : record,
      ];
      attached = true;
      _emit();
    } finally {
      completeWrite.complete();
    }
    if (attached) await _flush();
    return attached;
  }

  Future<void> markResultViewedByCaptureDir(String captureDir) async {
    await ensureLoaded();
    _guardPath(captureDir);
    for (final r in List<ScanRecord>.from(_records)) {
      final dir = r.captureDir;
      if (dir != null &&
          (_legacyFixture
              ? _sameDir(dir, captureDir)
              : _normalizedFilePath(dir) == _normalizedFilePath(captureDir))) {
        await markResultViewed(r);
        return;
      }
    }
  }

  Future<void> _markExistingSparseAsViewed() async {
    _checkActive();
    var changed = false;
    for (final r in List<ScanRecord>.from(_records)) {
      if (r.resultViewedAt != null) continue;

      if (r.artifactPath != null) continue;
      final ready = resultReadyAt(r);
      if (ready == null) continue;
      final i = _records.indexWhere((e) => e.id == r.id);
      if (i < 0) continue;
      _records[i] = r.copyWith(resultViewedAt: ready);
      changed = true;
    }
    if (changed) await _flush();
  }

  Future<void> migrateExistingSparseAsViewed() => _markExistingSparseAsViewed();

  void requireAvailableName(String id, String name) {
    _checkActive();
    if (name.trim().isEmpty || name.length > 256) {
      throw ArgumentError('Invalid work name');
    }
    final key = workNameKey(name);
    if (_records.any(
      (other) => other.id != id && workNameKey(other.name) == key,
    )) {
      throw DuplicateWorkNameException();
    }
  }

  Future<void> insertNew(ScanRecord r) => addOrUpdate(r, requireNew: true);

  Future<ScanRecord> registerCaptureDraft(
    ScanRecord draft, {
    required bool continuing,
  }) async {
    await ensureLoaded();
    if (!continuing) {
      await insertNew(draft);
    } else {
      final existing = byId(draft.id);
      if (existing == null ||
          existing.pipelineKind != draft.pipelineKind ||
          existing.captureDir != draft.captureDir) {
        throw StateError('Capture does not belong to the existing work');
      }
      _requireOwner(existing);
      await _ensureRouteOwnedPaths(existing);
    }
    _checkActive();
    return byId(draft.id)!;
  }

  Future<void> updateNames(Map<String, String> updates) async {
    await ensureLoaded();
    final next = [
      for (final r in _records) r.copyWith(name: updates[r.id] ?? r.name),
    ];
    final names = <String>{};
    for (final r in next) {
      if (!names.add(workNameKey(r.name))) throw DuplicateWorkNameException();
      if (r.name.trim().isEmpty || r.name.length > 256) {
        throw ArgumentError('Invalid work name');
      }
    }
    _checkActive();
    _records = next;
    _emit();
    await _flush();
  }

  Future<void> addOrUpdate(
    ScanRecord r, {
    bool requireNew = false,
    bool Function()? beforeCommit,
  }) async {
    await ensureLoaded();
    _ensureWritable();
    _validateId(r.id);
    if (!_legacyFixture && r.accountNamespace == null) {
      r = ScanRecord(
        id: r.id,
        accountNamespace: workspace.namespace,
        name: r.name,
        createdAt: r.createdAt,
        pipelineKind: r.pipelineKind,
        thumbnailPath: r.thumbnailPath,
        artifactPath: r.artifactPath,
        finalGlbSha256: r.finalGlbSha256,
        finalGlbBytes: r.finalGlbBytes,
        captureDir: r.captureDir,
        photosDir: r.photosDir,
        captureManifestPath: r.captureManifestPath,
        photoCount: r.photoCount,
        localRawDeletedAt: r.localRawDeletedAt,
        resultViewedAt: r.resultViewedAt,
        localRawRetainedForDebug: r.localRawRetainedForDebug,
        authorHandle: r.authorHandle,
        caption: r.caption,
        bundledGlbAsset: r.bundledGlbAsset,
      );
    }
    _requireOwner(r);
    await _ensureRouteOwnedPaths(r);
    _checkActive();
    if (_isPermanentlyDeleted(r.id, r.pipelineKind)) {
      await _deleteProjectFiles(
        r,
        source: 'scan_record_store.late_result_tombstone',
      );
      return;
    }
    final previous = byId(r.id);
    if (requireNew && previous != null) {
      throw StateError('Work identifier already exists');
    }
    if (requireNew &&
        kUntitledScanNamePattern.hasMatch(r.name) &&
        _records.any(
          (other) => workNameKey(other.name) == workNameKey(r.name),
        )) {
      r = r.copyWith(
        name: nextUntitledScanName(_records.map((other) => other.name.trim())),
      );
    }
    requireAvailableName(r.id, r.name);
    if (previous != null && previous.pipelineKind != r.pipelineKind) {
      throw StateError(
        'Cannot change pipeline kind for existing scan ${r.id}: '
        '${previous.pipelineKind.wireName} -> ${r.pipelineKind.wireName}',
      );
    }

    if (beforeCommit != null && !beforeCommit()) {
      throw StateError('Work changed before record update');
    }
    final next = <ScanRecord>[
      r,
      for (final old in _records)
        if (old.id != r.id) old,
    ];
    _records = _sortNewestFirst(next);
    _emit();
    await _flush();
  }

  ScanRecord? byId(String id) {
    if (!isActive) return null;
    for (final r in _records) {
      if (r.id == id) return r;
    }
    return null;
  }

  Future<void> attachVerifiedLocalArtifact({
    required String id,
    required String captureDir,
    required String artifactPath,
    String? finalGlbSha256,
    int? finalGlbBytes,
  }) async {
    _validateFinalGlbProof(finalGlbSha256, finalGlbBytes);
    await ensureLoaded();
    _ensureWritable();
    final before = byId(id);
    if (before == null ||
        before.captureDir != captureDir ||
        before.artifactPath != null ||
        _isPermanentlyDeleted(id, before.pipelineKind)) {
      throw StateError(
        'Completed artifact must attach to an unchanged existing capture',
      );
    }
    await _ensureRouteOwnedPaths(before.copyWith(artifactPath: artifactPath));
    _ensureWritable();
    _requireOwner(before);

    final previousWrite = _writeLock;
    final completeWrite = Completer<void>();
    _writeLock = completeWrite.future;
    try {
      await previousWrite;
      _ensureWritable();
      final file = await _storeFile();
      void requireCompletionWrite() {
        _ensureWritable();
        _requireOwner(before);
        _guardPath(file.path);
        _guardPath(captureDir);
        _guardPath(artifactPath);
      }

      while (true) {
        requireCompletionWrite();
        final latest = byId(id);
        if (latest == null ||
            latest.captureDir != captureDir ||
            latest.pipelineKind != before.pipelineKind ||
            latest.artifactPath != null ||
            _isPermanentlyDeleted(id, latest.pipelineKind)) {
          throw StateError(
            'Capture changed while validating completed artifact',
          );
        }
        _requireOwner(latest);
        final originalJson = jsonEncode(_records.map(_recordToJson).toList());
        final next = _sortNewestFirst([
          latest.copyWith(
            artifactPath: artifactPath,
            finalGlbSha256: finalGlbSha256,
            finalGlbBytes: finalGlbBytes,
            clearThumbnailPath:
                latest.thumbnailPath?.contains('/.pw_preview_covers/') == true,
          ),
          for (final old in _records)
            if (old.id != id) old,
        ]);
        final expectedJson = jsonEncode(next.map(_recordToJson).toList());
        requireCompletionWrite();
        final pending = await file.parent.createTemp('.reconstruction_record_');
        requireCompletionWrite();
        _guardPath(pending.path);
        final staged = File('${pending.path}/scan_records.json');
        _guardPath(staged.path);

        await staged.writeAsString(expectedJson, flush: true);
        requireCompletionWrite();
        _guardPath(staged.path);
        if (await staged.readAsString() != expectedJson) {
          throw StateError('Completed artifact manifest staging changed');
        }
        requireCompletionWrite();
        _guardPath(staged.path);

        staged.renameSync(file.path);
        requireCompletionWrite();
        if (await file.readAsString() != expectedJson) {
          throw StateError('Completed artifact link was not durably saved');
        }
        requireCompletionWrite();
        _guardPath(pending.path);
        try {
          await pending.delete();
        } catch (e) {
          debugPrint(
            '[ScanRecordStore] completed artifact staging retained: $e',
          );
        }
        requireCompletionWrite();

        if (jsonEncode(_records.map(_recordToJson).toList()) != originalJson) {
          continue;
        }
        _records = next;
        _emit();
        return;
      }
    } finally {
      completeWrite.complete();
    }
  }

  static void _validateFinalGlbProof(String? hash, int? bytes) {
    if ((hash == null) != (bytes == null) ||
        (hash != null && !RegExp(r'^[0-9a-f]{64}$').hasMatch(hash)) ||
        (bytes != null && bytes <= 0)) {
      throw ArgumentError(
        'Final GLB proof requires a SHA-256 and positive size',
      );
    }
  }

  Future<void> retireCaptureMetadataAfterGlbValidation({
    required String id,
    required String captureDir,
    required String artifactPath,
    required String expectedSha256,
    required int expectedBytes,
    PendingCaptureRetirementToken? pendingToken,
  }) async {
    _validateFinalGlbProof(expectedSha256, expectedBytes);
    await ensureLoaded();
    _ensureWritable();
    final before = byId(id);
    File? pendingProofFile;
    String? pendingProofFileSha256;
    var pendingVerified = false;

    String canonicalCapture(String raw) {
      final directory = Directory(_normalizedFilePath(raw));
      return '${directory.parent.resolveSymbolicLinksSync()}/${directory.uri.pathSegments.where((p) => p.isNotEmpty).last}';
    }

    if (pendingToken != null) {
      if (before == null ||
          _legacyFixture ||
          before.pipelineKind != CapturePipelineKind.official) {
        throw StateError(
          'Pending retirement requires an owned official record',
        );
      }
      _requireOwner(before);
      _guardPath(captureDir);
      _guardPath(artifactPath);
      final documents = await _documentsDirectory();
      final canonicalAccount = Directory(
        await documents.resolveSymbolicLinks(),
      );
      final pendingStore = PendingCaptureRetirementStore(
        accountRoot: canonicalAccount,
        accountNamespace: workspace.namespace,
        recordId: id,
      );
      final actual = await pendingStore.load(
        requirePublicationAndConsumersDrained: () async {
          _ensureWritable();
          _requireOwner(before);
        },
      );
      if (actual == null ||
          actual.identitySha256 != pendingToken.identitySha256 ||
          actual.accountNamespace != pendingToken.accountNamespace ||
          actual.recordId != pendingToken.recordId ||
          actual.captureRelative != pendingToken.captureRelative ||
          actual.publishedGlbRelative != pendingToken.publishedGlbRelative ||
          actual.finalGlbSha256 != pendingToken.finalGlbSha256 ||
          actual.finalGlbBytes != pendingToken.finalGlbBytes ||
          actual.accountNamespace != workspace.namespace ||
          actual.recordId != id ||
          pendingStore.captureRootFor(actual).path !=
              canonicalCapture(captureDir) ||
          pendingStore.publishedGlbFor(actual).path !=
              await File(
                _normalizedFilePath(artifactPath),
              ).resolveSymbolicLinks() ||
          actual.finalGlbSha256 != expectedSha256 ||
          actual.finalGlbBytes != expectedBytes) {
        throw StateError(
          'Persisted pending retirement does not match this record',
        );
      }

      if (actual.captureRelative != 'captures_official/$id') {
        throw StateError('Pending capture is outside this record namespace');
      }
      pendingProofFile = File(
        '${canonicalAccount.path}/.pw_capture_retirement/$id.json',
      );
      _guardPath(pendingProofFile.path);
      pendingProofFileSha256 = sha256
          .convert(await pendingProofFile.readAsBytes())
          .toString();
      pendingVerified = true;
    }
    bool captureIdentityMatches(ScanRecord record) {
      if (record.captureDir == captureDir) return true;
      if (!pendingVerified) return false;
      if (record.captureDir == null) return true;
      return canonicalCapture(record.captureDir!) ==
          canonicalCapture(captureDir);
    }

    if (before == null ||
        !captureIdentityMatches(before) ||
        before.artifactPath != artifactPath ||
        before.finalGlbSha256 != expectedSha256 ||
        before.finalGlbBytes != expectedBytes ||
        _isPermanentlyDeleted(id, before.pipelineKind)) {
      throw StateError('Final GLB proof does not match this capture record');
    }
    _requireOwner(before);
    await _ensureRouteOwnedPaths(before);
    _ensureWritable();
    _guardPath(artifactPath);
    final artifact = File(_normalizedFilePath(artifactPath));
    if (await artifact.length() != expectedBytes ||
        (await sha256.bind(artifact.openRead()).first).toString() !=
            expectedSha256) {
      throw StateError('Final GLB bytes no longer match the completion proof');
    }
    _ensureWritable();

    final previousWrite = _writeLock;
    final completeWrite = Completer<void>();
    _writeLock = completeWrite.future;
    try {
      await previousWrite;
      _ensureWritable();
      final file = await _storeFile();
      void requireCompletionWrite() {
        _ensureWritable();
        _requireOwner(before);
        _guardPath(file.path);
        _guardPath(captureDir);
        _guardPath(artifactPath);
        if (pendingProofFile != null) {
          _guardPath(pendingProofFile.path);
          if (FileSystemEntity.typeSync(
                    pendingProofFile.path,
                    followLinks: false,
                  ) !=
                  FileSystemEntityType.file ||
              sha256.convert(pendingProofFile.readAsBytesSync()).toString() !=
                  pendingProofFileSha256) {
            throw StateError(
              'Pending retirement token changed during record commit',
            );
          }
        }
        if (FileSystemEntity.typeSync(captureDir, followLinks: false) !=
            FileSystemEntityType.notFound) {
          throw StateError('Capture still exists; metadata cannot be retired');
        }
      }

      while (true) {
        requireCompletionWrite();
        final latest = byId(id);
        if (latest == null ||
            !captureIdentityMatches(latest) ||
            latest.pipelineKind != before.pipelineKind ||
            latest.artifactPath != artifactPath ||
            latest.finalGlbSha256 != expectedSha256 ||
            latest.finalGlbBytes != expectedBytes ||
            _isPermanentlyDeleted(id, latest.pipelineKind)) {
          throw StateError(
            'Capture changed while validating completed artifact',
          );
        }
        _requireOwner(latest);
        final originalJson = jsonEncode(_records.map(_recordToJson).toList());
        final next = _sortNewestFirst([
          _withoutCaptureMetadata(latest).copyWith(
            localRawDeletedAt: latest.localRawDeletedAt ?? DateTime.now(),
          ),
          for (final old in _records)
            if (old.id != id) old,
        ]);
        final expectedJson = jsonEncode(next.map(_recordToJson).toList());
        requireCompletionWrite();
        final pending = await file.parent.createTemp('.reconstruction_record_');
        requireCompletionWrite();
        _guardPath(pending.path);
        final staged = File('${pending.path}/scan_records.json');
        _guardPath(staged.path);

        await staged.writeAsString(expectedJson, flush: true);
        requireCompletionWrite();
        _guardPath(staged.path);
        if (await staged.readAsString() != expectedJson) {
          throw StateError('Completed artifact manifest staging changed');
        }
        requireCompletionWrite();
        _guardPath(staged.path);

        staged.renameSync(file.path);
        requireCompletionWrite();
        if (await file.readAsString() != expectedJson) {
          throw StateError('Completed artifact link was not durably saved');
        }
        requireCompletionWrite();
        try {
          await pending.delete();
        } catch (e) {
          debugPrint('[ScanRecordStore] retirement staging retained: $e');
        }
        requireCompletionWrite();

        if (jsonEncode(_records.map(_recordToJson).toList()) != originalJson) {
          continue;
        }
        _records = next;
        _emit();
        return;
      }
    } finally {
      completeWrite.complete();
    }
  }

  Future<void> delete(
    String id, {
    String source = 'scan_record_store.delete',
  }) async {
    final log = _ProjectDeletionLog(id, source);
    var step = 'ensure_loaded';
    log.event(
      'delete_started',
      details: {'stack': StackTrace.current.toString()},
    );
    try {
      await ensureLoaded();
      step = 'validate_record';
      _ensureWritable();
      _validateId(id);
      final record = byId(id);
      if (record == null) {
        log.event('record_not_found');
        return;
      }
      log.event('record_resolved', details: {'capture_dir': record.captureDir});
      step = 'persist_tombstone';
      log.event('tombstone_started');
      await _addDeletionTombstone(record);
      log.event('tombstone_completed');
      _checkActive();

      step = 'remove_record';
      _records = _records.where((r) => r.id != id).toList(growable: false);
      _emit();
      log.event('record_removed');
      step = 'flush_records';
      log.event('records_flush_started');
      await _flush();
      log.event('records_flush_completed');
      step = 'delete_project_files';
      await _deleteProjectFiles(record, source: source, operation: log);
      log.event('delete_completed');
    } catch (error, stack) {
      log.event(
        'delete_failed',
        details: {
          'step': step,
          'error': error.toString(),
          'stack': stack.toString(),
        },
      );
      rethrow;
    }
  }

  Future<void> _deleteProjectFiles(
    ScanRecord record, {
    required String source,
    _ProjectDeletionLog? operation,
  }) async {
    final log = operation ?? _ProjectDeletionLog(record.id, source);
    var step = 'resolve_paths';
    String? deletingPath;
    log.event(
      'project_files_started',
      details: {
        'capture_dir': record.captureDir,
        'stack': StackTrace.current.toString(),
      },
    );
    try {
      _checkActive();
      _requireOwner(record);
      await _ensureRouteOwnedPaths(record);
      final routeRoot = await _capturesRoot(pipelineKind: record.pipelineKind);
      final canonicalCaptureDir = await captureDirFor(
        record.id,
        pipelineKind: record.pipelineKind,
      );
      final captureDirs = <String>{canonicalCaptureDir.path};
      final storedCaptureDir = record.captureDir;
      if (storedCaptureDir != null &&
          _isWithinDirectory(storedCaptureDir, routeRoot.path)) {
        captureDirs.add(storedCaptureDir);
      }
      for (final path in captureDirs) {
        step = 'delete_capture_directory';
        deletingPath = path;
        _guardPath(path);
        final captureDir = Directory(path);
        log.event('capture_delete_started', details: {'capture_dir': path});
        if (await captureDir.exists()) {
          _guardPath(path);
          await captureDir.delete(recursive: true);
          log.event('capture_delete_completed', details: {'capture_dir': path});
          _checkActive();
        } else {
          log.event('capture_not_found', details: {'capture_dir': path});
        }
      }

      step = 'delete_scan_artifacts';
      deletingPath = null;
      final scansDir = await _scansDir(pipelineKind: record.pipelineKind);
      if (await scansDir.exists()) {
        await for (final entity in scansDir.list(followLinks: false)) {
          _checkActive();
          final name = entity.uri.pathSegments
              .where((segment) => segment.isNotEmpty)
              .lastOrNull;
          if (name == null ||
              (name != record.id && !name.startsWith('${record.id}.'))) {
            continue;
          }
          deletingPath = entity.path;
          _guardPath(entity.path);
          log.event('artifact_delete_started', details: {'path': entity.path});
          await entity.delete(recursive: entity is Directory);
          log.event(
            'artifact_delete_completed',
            details: {'path': entity.path},
          );
          _checkActive();
        }
      }
      log.event('project_files_completed');
    } catch (error, stack) {
      log.event(
        'project_files_failed',
        details: {
          'step': step,
          'path': deletingPath,
          'error': error.toString(),
          'stack': stack.toString(),
        },
      );
      rethrow;
    }
  }

  Future<void> _addDeletionTombstone(ScanRecord record) async {
    final next = <String>{
      ..._deletionTombstones,
      _deletionHash(record.id, record.pipelineKind),
    };
    final file = await _deletionTombstoneFile();
    _guardPath(file.path);
    await file.writeAsString(jsonEncode(next.toList()..sort()), flush: true);
    _checkActive();
    _deletionTombstones = next;
  }

  Future<Set<String>> _loadDeletionTombstones() async {
    final file = await _deletionTombstoneFile();
    if (!await file.exists()) return const <String>{};
    final decoded = jsonDecode(await file.readAsString());
    if (decoded is! List) {
      throw const FormatException('deletion tombstones must be a JSON list');
    }
    final hashes = <String>{};
    for (final value in decoded) {
      if (value is! String || !RegExp(r'^[0-9a-f]{64}$').hasMatch(value)) {
        throw const FormatException('invalid deletion tombstone hash');
      }
      hashes.add(value);
    }
    return hashes;
  }

  bool _isPermanentlyDeleted(String id, CapturePipelineKind pipelineKind) {
    return _deletionTombstones.contains(_deletionHash(id, pipelineKind));
  }

  static String _deletionHash(String id, CapturePipelineKind pipelineKind) {
    final bytes = utf8.encode(
      'worldswithin.project-deletion.v1:${pipelineKind.wireName}:$id',
    );
    return sha256.convert(bytes).toString();
  }

  Future<File> glbFileFor(
    String id, {
    CapturePipelineKind pipelineKind = CapturePipelineKind.self,
  }) async {
    _checkActive();
    _validateId(id);
    final dir = await _scansDir(pipelineKind: pipelineKind);
    final file = File('${dir.path}/$id.glb');
    _guardPath(file.path);
    return file;
  }

  Future<Directory> captureDirFor(
    String id, {
    CapturePipelineKind pipelineKind = CapturePipelineKind.self,
  }) async {
    _checkActive();
    _validateId(id);
    final root = await _capturesRoot(pipelineKind: pipelineKind);
    final directory = Directory('${root.path}/$id');
    _guardPath(directory.path);
    return directory;
  }

  static Future<void> _captureCreationTail = Future<void>.value();

  Future<Directory> createCaptureDirectory({
    CapturePipelineKind pipelineKind = CapturePipelineKind.self,
  }) {
    final pending = _captureCreationTail.then((_) async {
      await ensureLoaded();
      for (var attempt = 0; attempt < 8; attempt++) {
        final id = _captureIdentifier();
        _validateId(id);
        if (byId(id) != null ||
            _isPermanentlyDeleted(id, CapturePipelineKind.self) ||
            _isPermanentlyDeleted(id, CapturePipelineKind.official)) {
          continue;
        }
        var occupied = false;
        for (final route in CapturePipelineKind.values) {
          for (final path in [
            (await captureDirFor(id, pipelineKind: route)).path,
            (await thumbnailFileFor(id, pipelineKind: route)).path,
            (await glbFileFor(id, pipelineKind: route)).path,
          ]) {
            await requireOwnedPath(path);
            if (await FileSystemEntity.type(path, followLinks: false) !=
                FileSystemEntityType.notFound) {
              occupied = true;
            }
          }
        }
        if (occupied) continue;
        final parent = await _capturesRoot(pipelineKind: pipelineKind);
        await requireOwnedPath(parent.path);
        await parent.create(recursive: true);
        await requireOwnedPath(parent.path);
        _checkActive();

        final root = await parent.createTemp('$id-');
        _checkActive();
        final allocatedId = root.uri.pathSegments
            .where((s) => s.isNotEmpty)
            .last;
        _validateId(allocatedId);
        var allocatedConflict =
            byId(allocatedId) != null ||
            _isPermanentlyDeleted(allocatedId, CapturePipelineKind.self) ||
            _isPermanentlyDeleted(allocatedId, CapturePipelineKind.official);
        for (final route in CapturePipelineKind.values) {
          for (final path in [
            if (route != pipelineKind)
              (await captureDirFor(allocatedId, pipelineKind: route)).path,
            (await thumbnailFileFor(allocatedId, pipelineKind: route)).path,
            (await glbFileFor(allocatedId, pipelineKind: route)).path,
          ]) {
            await requireOwnedPath(path);
            if (await FileSystemEntity.type(path, followLinks: false) !=
                FileSystemEntityType.notFound) {
              allocatedConflict = true;
            }
          }
        }
        _checkActive();
        if (allocatedConflict) continue;
        return root;
      }
      throw StateError('Unable to allocate a new work identifier');
    });
    _captureCreationTail = pending.then<void>(
      (_) {},
      onError: (Object _, StackTrace _) {},
    );
    return pending;
  }

  Future<Directory> _capturesRoot({
    required CapturePipelineKind pipelineKind,
  }) async {
    final root = await _documentsDirectory();
    final directoryName = pipelineKind == CapturePipelineKind.official
        ? 'captures_official'
        : 'captures';
    final directory = Directory('${root.path}/$directoryName');
    _guardPath(directory.path);
    return directory;
  }

  Future<File> thumbnailFileFor(
    String id, {
    CapturePipelineKind pipelineKind = CapturePipelineKind.self,
  }) async {
    _checkActive();
    _validateId(id);
    final dir = await _scansDir(pipelineKind: pipelineKind);
    final file = File('${dir.path}/$id.jpg');
    _guardPath(file.path);
    return file;
  }

  Future<Directory> _scansDir({
    required CapturePipelineKind pipelineKind,
  }) async {
    final root = await _documentsDirectory();
    final directoryName = pipelineKind == CapturePipelineKind.official
        ? 'scans_official'
        : 'scans';
    final dir = Directory('${root.path}/$directoryName');
    _guardPath(dir.path);
    if (!await dir.exists()) {
      _guardPath(dir.path);
      await dir.create(recursive: true);
    }
    _guardPath(dir.path);
    return dir;
  }

  Future<File> _storeFile() async {
    final root = await _documentsDirectory();
    final file = File('${root.path}/scan_records.json');
    _guardPath(file.path);
    return file;
  }

  Future<File> _deletionTombstoneFile() async {
    final root = await _documentsDirectory();
    final file = File('${root.path}/scan_record_deletion_tombstones.json');
    _guardPath(file.path);
    return file;
  }

  Future<Directory> workspaceDirectory() => _documentsDirectory();

  Future<void> requireOwnedPath(String path) async {
    await _documentsDirectory();
    _guardPath(path);
  }

  Future<Directory> _documentsDirectory() async {
    _checkActive();
    var root = _resolvedDocumentsDirectory;
    if (root == null) {
      final base =
          _documentsDirectoryOverride ??
          Directory(
            '${(await getApplicationDocumentsDirectory()).path}/worldswithin_pipeline',
          );
      _checkActive();
      _baseDocumentsDirectory = base.absolute;
      root = base.absolute;
      _resolvedDocumentsDirectory = root;
    }
    _guardPath(root.path);
    if (!await root.exists()) {
      _guardPath(root.path);
      await root.create(recursive: true);
    }
    _guardPath(root.path);
    return root;
  }

  void _checkActive() {
    if (!isActive) throw StateError('Local workspace is locked or revoked');
  }

  void _validateId(String id) {
    if (!RegExp(r'^[A-Za-z0-9][A-Za-z0-9_-]{0,127}$').hasMatch(id)) {
      throw ArgumentError.value(id, 'id', 'Unsafe scan identifier');
    }
  }

  void _requireOwner(ScanRecord record) {
    _checkActive();
    _validateId(record.id);
    if (!_legacyFixture && record.accountNamespace != workspace.namespace) {
      throw StateError('Record belongs to a different local workspace');
    }
  }

  static String _safeAbsolutePath(String rawPath) {
    final String path;
    try {
      final decoded = rawPath.startsWith('file://')
          ? Uri.decodeComponent(rawPath.substring(7))
          : rawPath;
      if (decoded.split('/').any((part) => part == '.' || part == '..')) {
        throw StateError('Traversal is not allowed in workspace storage');
      }
      path = rawPath.startsWith('file://')
          ? Uri.parse(rawPath).toFilePath()
          : rawPath;
    } catch (_) {
      throw StateError('Invalid local file path');
    }
    if (!path.startsWith('/') ||
        path.contains('\\') ||
        path.contains('\u0000') ||
        path.split('/').any((segment) => segment == '..' || segment == '.')) {
      throw StateError('Unsafe local file path');
    }
    return path;
  }

  void _guardPath(String rawPath) {
    _checkActive();
    if (_legacyFixture) return;
    var path = _safeAbsolutePath(rawPath);
    final root = _resolvedDocumentsDirectory;
    final base = _baseDocumentsDirectory;

    if (base != null && base.existsSync()) {
      final canonicalBase = base.resolveSymbolicLinksSync();
      if (_isWithinDirectory(path, canonicalBase)) {
        path = '${base.path}${path.substring(canonicalBase.length)}';
      }
    }
    if (root == null || base == null || !_isWithinDirectory(path, root.path)) {
      throw StateError('Path belongs to a different local workspace');
    }

    var cursor = base.path;
    final relative = _normalizedFilePath(
      path,
    ).substring(_normalizedFilePath(base.path).length);
    for (final segment in relative.split('/').where((s) => s.isNotEmpty)) {
      cursor = '$cursor/$segment';
      if (FileSystemEntity.typeSync(cursor, followLinks: false) ==
          FileSystemEntityType.link) {
        throw StateError('Symbolic links are not allowed in workspace storage');
      }
    }
  }

  static bool _crossesPipelineNamespace(
    String rawPath, {
    required CapturePipelineKind pipelineKind,
  }) {
    final path = rawPath.startsWith('file://') ? rawPath.substring(7) : rawPath;
    final segments = path
        .split(RegExp(r'[/\\]+'))
        .where((segment) => segment.isNotEmpty)
        .toSet();
    final otherRouteDirectories = pipelineKind == CapturePipelineKind.official
        ? const <String>{'scans', 'captures'}
        : const <String>{'scans_official', 'captures_official'};
    return segments.any(otherRouteDirectories.contains);
  }

  Future<void> _ensureRouteOwnedPaths(ScanRecord record) async {
    _requireOwner(record);
    final paths = <String?>[record.thumbnailPath, record.artifactPath];
    for (final path in paths) {
      if (path == null || path.isEmpty) continue;
      _guardPath(path);
      if (_crossesPipelineNamespace(path, pipelineKind: record.pipelineKind)) {
        throw StateError(
          'Scan ${record.id} (${record.pipelineKind.wireName}) references '
          'an artifact owned by the other pipeline: $path',
        );
      }
    }

    final captureRoot = await _capturesRoot(pipelineKind: record.pipelineKind);
    final capturePaths = <String?>[
      record.captureDir,
      record.photosDir,
      record.captureManifestPath,
    ];
    for (final path in capturePaths) {
      if (path == null || path.isEmpty) continue;
      _guardPath(path);
      if (_crossesPipelineNamespace(path, pipelineKind: record.pipelineKind) ||
          !_isWithinDirectory(
            path,
            _legacyFixture
                ? captureRoot.path
                : '${captureRoot.path}/${record.id}',
          )) {
        throw StateError(
          'Scan ${record.id} (${record.pipelineKind.wireName}) references '
          'capture metadata outside its route-owned namespace: $path',
        );
      }
    }
    _checkActive();
  }

  static bool _isWithinDirectory(String rawPath, String rawRoot) {
    final path = _normalizedFilePath(rawPath);
    final root = _normalizedFilePath(rawRoot);
    return path == root || path.startsWith('$root${Platform.pathSeparator}');
  }

  static String _normalizedFilePath(String rawPath) {
    final path = rawPath.startsWith('file://')
        ? Uri.parse(rawPath).toFilePath()
        : File(rawPath).absolute.path;
    return Uri.file(path).normalizePath().toFilePath();
  }

  static ScanRecord _withoutCaptureMetadata(ScanRecord record) {
    final json = _recordToJson(record)
      ..remove('captureDir')
      ..remove('photosDir')
      ..remove('captureManifestPath');
    return _recordFromJson(json);
  }

  void _ensureWritable() {
    _checkActive();
    final failure = _loadFailure;
    if (failure != null) {
      throw StateError(
        'scan_records.json failed validation; refusing to overwrite it: '
        '$failure',
      );
    }
  }

  void _emit() {
    if (!_ctrl.isClosed) _ctrl.add(records);
  }

  Future<void> _flush() async {
    final last = _writeLock;
    final completer = Completer<void>();
    _writeLock = completer.future;
    try {
      await last;
      _ensureWritable();
      final file = await _storeFile();
      final json = jsonEncode(_records.map(_recordToJson).toList());
      _guardPath(file.path);
      await file.writeAsString(json, flush: true);
      _checkActive();
    } catch (e, st) {
      debugPrint('[ScanRecordStore] flush failed: $e\n$st');
      if (!_legacyFixture) rethrow;
    } finally {
      completer.complete();
    }
  }

  static List<ScanRecord> _sortNewestFirst(List<ScanRecord> rs) {
    final out = [...rs]..sort((a, b) => b.createdAt.compareTo(a.createdAt));
    return out;
  }

  static Map<String, dynamic> _recordToJson(ScanRecord r) => <String, dynamic>{
    'id': r.id,
    if (r.accountNamespace != null) 'accountNamespace': r.accountNamespace,
    'name': r.name,
    'createdAt': r.createdAt.toIso8601String(),
    'pipeline_kind': r.pipelineKind.wireName,
    'resultViewedAt': r.resultViewedAt!.toIso8601String(),
    if (r.thumbnailPath != null) 'thumbnailPath': r.thumbnailPath,
    if (r.finalCoverSha256 != null) 'finalCoverSha256': r.finalCoverSha256,
    if (r.finalCoverGlbSha256 != null)
      'finalCoverGlbSha256': r.finalCoverGlbSha256,
    if (r.finalCoverCameraVersion != null)
      'finalCoverCameraVersion': r.finalCoverCameraVersion,
    if (r.artifactPath != null) 'artifactPath': r.artifactPath,
    if (r.finalGlbSha256 != null) 'finalGlbSha256': r.finalGlbSha256,
    if (r.finalGlbBytes != null) 'finalGlbBytes': r.finalGlbBytes,
    if (r.captureDir != null) 'captureDir': r.captureDir,
    if (r.photosDir != null) 'photosDir': r.photosDir,
    if (r.captureManifestPath != null)
      'captureManifestPath': r.captureManifestPath,
    if (r.photoCount != null) 'photoCount': r.photoCount,
    if (r.localRawDeletedAt != null)
      'localRawDeletedAt': r.localRawDeletedAt!.toIso8601String(),
    if (r.localRawRetainedForDebug)
      'localRawRetainedForDebug': r.localRawRetainedForDebug,
    if (r.caption != null) 'caption': r.caption,
  };

  static ScanRecord _recordFromJson(Map<String, dynamic> j) {
    _validateFinalGlbProof(
      j['finalGlbSha256'] as String?,
      j['finalGlbBytes'] as int?,
    );
    final pipelineKind = j.containsKey('pipeline_kind')
        ? CapturePipelineKindWire.fromWireName(j['pipeline_kind'])
        : CapturePipelineKind.self;
    return ScanRecord(
      id: j['id'] as String,
      accountNamespace: j['accountNamespace'] as String?,
      name: j['name'] as String,
      createdAt: DateTime.parse(j['createdAt'] as String),
      pipelineKind: pipelineKind,
      resultViewedAt: j['resultViewedAt'] == null
          ? null
          : DateTime.tryParse(j['resultViewedAt'] as String),
      thumbnailPath: j['thumbnailPath'] as String?,
      finalCoverSha256: j['finalCoverSha256'] as String?,
      finalCoverGlbSha256: j['finalCoverGlbSha256'] as String?,
      finalCoverCameraVersion: j['finalCoverCameraVersion'] as String?,
      artifactPath: j['artifactPath'] as String?,
      finalGlbSha256: j['finalGlbSha256'] as String?,
      finalGlbBytes: j['finalGlbBytes'] as int?,
      captureDir: j['captureDir'] as String?,
      photosDir: j['photosDir'] as String?,
      captureManifestPath: j['captureManifestPath'] as String?,
      photoCount: (j['photoCount'] as num?)?.toInt(),
      localRawDeletedAt: DateTime.tryParse(
        j['localRawDeletedAt'] as String? ?? '',
      ),
      localRawRetainedForDebug:
          (j['localRawRetainedForDebug'] as bool?) ?? false,
      caption: j['caption'] as String?,
    );
  }
}
