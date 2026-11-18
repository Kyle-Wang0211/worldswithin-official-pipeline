                                                                         
             
  
                                                                        
                                                                       
                                                                                        
                                                                                
                                                                               
                                                                              
                                                                               
                                                                         
                                
                                                                              
                                                                           
                                                                                 
                                                                               
                                                                            

import 'package:flutter/foundation.dart';

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:crypto/crypto.dart';
import 'package:path_provider/path_provider.dart';

import '../me/scan_record_store.dart';
import '../ui/scan_record.dart';
import '../official_util/device_log.dart';
import 'archived_photo_rebuild.dart';
import 'colorize_pipeline.dart';
import 'database_archive_resolver.dart';
import 'photo_archive_coordinator.dart';
import 'photo_archive_resolver.dart';
import 'photo_archive_runtime.dart';
import 'pwva_master.dart';
import 'representative_color.dart';
import 'sfm_live_recon.dart';
import 'sparse_ply.dart';

const MethodChannel _arKitChannel = MethodChannel('worldswithin_official_arkit');

bool _sweeping = false;
final Set<String> _detachedFinalizing = <String>{};

                                                       
                                           
                                     
final Map<String, Future<bool>> _resumeInFlight = <String, Future<bool>>{};

                                                     
                                                  
                        
final Map<String, Future<ArchivedRebuildResult>> _rebuildInFlight =
    <String, Future<ArchivedRebuildResult>>{};

                                               
bool isResumeInFlight(String captureDir) =>
    _resumeInFlight.containsKey(captureDir);

                                   
   
                                                 
                                                   
                                                    
                                          
bool isResumeInFlightForDirName(String dirName) {
  if (dirName.isEmpty) return false;
  return _resumeInFlight.keys.any(
    (d) => d.split('/').where((e) => e.isNotEmpty).last == dirName,
  );
}

                                           
@visibleForTesting
void debugSetResumeInFlight(String captureDir, {required bool inFlight}) {
  if (inFlight) {
    _resumeInFlight[captureDir] = Future<bool>.value(true);
  } else {
    _resumeInFlight.remove(captureDir);
  }
}

                                                                            
                                                                          
Future<String?> resolveRecoverableCaptureDir(
  String recordCaptureDir, {
  ScanRecordStore? store,
}) async {
  if (recordCaptureDir.isEmpty) return null;
  final origin = store ?? ScanRecordStore.instance;
  final resolver = DatabaseArchiveResolver(
    codec: databaseArchiveCodec,
    preprocessor: databaseArchivePreprocessor,
  );
  try {
    await origin.ensureLoaded();
                                                                             
                                                                        
    await _requireOfficialCapturePath(origin, recordCaptureDir);
    final direct = Directory(recordCaptureDir);
                                                                             
                                                                           
                                                                             
                                                                         
                                                                  
    if (await resolver.isRecoverable(direct) ||
        archivedPhotoCount(direct.path) > 0) {
      await origin.requireOwnedPath(direct.path);
      return direct.path;
    }
  } catch (_) {}
  return null;
}

Future<void> _requireOfficialCapturePath(ScanRecordStore store, String path) async {
  await store.requireOwnedPath(path);
  final id = Directory(path).uri.pathSegments.where((s) => s.isNotEmpty).last;
  final expected = await store.captureDirFor(id, pipelineKind: CapturePipelineKind.official);
  if (Directory(path).absolute.path != expected.absolute.path) {
    throw StateError('Capture belongs to a different pipeline');
  }
}

                                             
                                                              
                                         
                                                           
Future<bool> resumeSingleCapture(String captureDir, {ScanRecordStore? store}) async {
  final origin = store ?? ScanRecordStore.instance;
  try {
    await _requireOfficialCapturePath(origin, captureDir);
  } on StateError {
    return false;
  }
  final existing = _resumeInFlight[captureDir];
  if (existing != null) return existing;
  final completer = Completer<bool>();
  _resumeInFlight[captureDir] = completer.future;
  () async {
    try {
                                                
                                       
      await _requireOfficialCapturePath(origin, captureDir);
      await _resumeOne(captureDir, store: origin, timeout: const Duration(minutes: 25));
    } catch (e) {
      DeviceLog.log('SfmResume', 'single resume error $captureDir: $e');
    } finally {
      _resumeInFlight.remove(captureDir);
      completer.complete(
        File('$captureDir/official_sfm_sparse.ply').existsSync(),
      );
    }
  }();
  return completer.future;
}

                                                                       
   
                                                                              
                                                                                
                                                                               
                               
void startDetachedSfmFinalize({
  required String captureDir,
  required SfmLiveRecon recon,
  ScanRecordStore? store,
  Iterable<String> keepJpegPaths = const <String>[],
}) {
  if (!_detachedFinalizing.add(captureDir)) {
    DeviceLog.log('SfmResume', 'detached already running for $captureDir');
    unawaited(recon.dispose());
    return;
  }
  unawaited(_runDetachedFinalize(captureDir, recon, keepJpegPaths.toSet(), store ?? ScanRecordStore.instance));
}

                                                                               
                                                                                 
                                                                               
                                                                               
Future<void> resumeIncompleteCaptures() async {
  final store = ScanRecordStore.instance;
  if (_sweeping) return;
  _sweeping = true;
  try {
    await store.ensureLoaded();
    final records = store.records;
    final docs = (await store.workspaceDirectory()).path;
    DeviceLog.log(
      'SfmResume',
      'sweep start: ${records.length} records | docs=$docs',
    );
    final pending = <String>[];
    final databaseResolver = DatabaseArchiveResolver(
      codec: databaseArchiveCodec,
      preprocessor: databaseArchivePreprocessor,
    );
    for (final r in records) {
      if (r.pipelineKind != CapturePipelineKind.official) continue;
      final dir = r.captureDir;
      if (dir == null || dir.isEmpty) continue;
                                                                             
      await _requireOfficialCapturePath(store, dir);
      final hasDb = await databaseResolver.isRecoverable(Directory(dir));
      final hasPly = File('$dir/official_sfm_sparse.ply').existsSync();
      DeviceLog.log(
        'SfmResume',
        '  ${dir.split('/').last}: db=$hasDb ply=$hasPly',
      );
      if (hasDb && !hasPly && !_detachedFinalizing.contains(dir)) {
        pending.add(dir);
      }
    }
    DeviceLog.log(
      'SfmResume',
      'sweep: ${pending.length} capture(s) to recover',
    );
    for (final dir in pending) {
      await _requireOfficialCapturePath(store, dir);
      await _resumeOne(dir, store: store);
    }
  } catch (e, st) {
    DeviceLog.log('SfmResume', 'sweep error: $e\n$st');
  } finally {
    _sweeping = false;
  }
}

Future<void> _runDetachedFinalize(
  String captureDir,
  SfmLiveRecon recon,
  Set<String> keepJpegPaths,
  ScanRecordStore store,
) async {
  StreamSubscription<SfmLiveEvent>? sub;
  final done = Completer<void>();
  try {
    await _requireOfficialCapturePath(store, captureDir);
    await _umbrella('beginReconUmbrella', captureDir);
    sub = recon.events.listen((e) {
      switch (e) {
        case SfmLiveLocalReady(:final snapshot):
          DeviceLog.log(
            'SfmResume',
            'detached local ignored: ${snapshot.pointCount} pts',
          );
        case SfmLiveRefined(:final snapshot):
          unawaited(() async {
            try {
              await _persistColored(captureDir, snapshot);
              await _prunePhotosAfterSparse(captureDir, keepJpegPaths);
            } catch (e) {
              DeviceLog.log('SfmResume', 'detached persist failed: $e');
            } finally {
              if (!done.isCompleted) done.complete();
            }
          }());
        case SfmLiveFailed(:final stage, :final message):
          DeviceLog.log(
            'SfmResume',
            'detached failed for $captureDir: $stage $message',
          );
          if (!done.isCompleted) done.complete();
        default:
          break;
      }
    });
    DeviceLog.log(
      'SfmResume',
      'detached finalize start: $captureDir keep=${keepJpegPaths.length}',
    );
    recon.finalize();
    await done.future.timeout(
      const Duration(minutes: 12),
      onTimeout: () {
        DeviceLog.log('SfmResume', 'detached timed out for $captureDir');
      },
    );
  } catch (e) {
    DeviceLog.log('SfmResume', 'detached error $captureDir: $e');
  } finally {
    await sub?.cancel();
    await recon.dispose();
    await _umbrella('endReconUmbrella', captureDir);
    _detachedFinalizing.remove(captureDir);
  }
}

Future<void> _resumeOne(
  String captureDir, {
  required ScanRecordStore store,
  Duration timeout = const Duration(minutes: 8),
}) async {
  SfmLiveRecon? recon;
  PhotoArchiveActivityLease? archiveLease;
  final done = Completer<void>();
  try {
    await _requireOfficialCapturePath(store, captureDir);
    archiveLease = photoArchiveCoordinatorFor(store).beginReconstructionActivity(
      Directory(captureDir),
    );
    await _umbrella('beginReconUmbrella', captureDir);
    final database = await DatabaseArchiveResolver(
      codec: databaseArchiveCodec,
      preprocessor: databaseArchivePreprocessor,
    ).resolveDatabase(Directory(captureDir));
    if (database == null) {
      DeviceLog.log(
        'SfmResume',
        'verified database unavailable for $captureDir',
      );
      return;
    }
    recon = await SfmLiveRecon.start(dbPath: database.path);
    if (recon == null) {
      DeviceLog.log('SfmResume', 'start unavailable for $captureDir');
      return;
    }
                                                
                                                      
                                                    
                                                          
                                                 
                                        
    final frameMeta = await loadFedFrameMeta(captureDir);
    recon.seedFedMeta(frameMeta);
    DeviceLog.log(
      'SfmResume',
      'fed-meta seeded: ${frameMeta.length} frames, '
          'arkitQuat=${frameMeta.values.where((m) => m.arkitQuatWxyz != null).length}',
    );
    final sub = recon.events.listen((e) {
      switch (e) {
        case SfmLiveLocalReady(:final snapshot):
          DeviceLog.log(
            'SfmResume',
            'resume local ignored: ${snapshot.pointCount} pts',
          );
        case SfmLiveRefined(:final snapshot):
                                                        
                                                          
                                             
          unawaited(() async {
            try {
              await _persistColored(captureDir, snapshot, frameMeta: frameMeta);
            } catch (e) {
              DeviceLog.log('SfmResume', 'resume persist failed: $e');
            } finally {
              if (!done.isCompleted) done.complete();
            }
          }());
        case SfmLiveFailed(:final stage, :final message):
          DeviceLog.log(
            'SfmResume',
            'resume $captureDir failed: $stage $message',
          );
          if (!done.isCompleted) done.complete();
        default:
          break;
      }
    });
    recon.resumeFromDb();
                                                                                
                                                                                
                                
    await done.future.timeout(
      timeout,
      onTimeout: () {
        DeviceLog.log('SfmResume', '$captureDir timed out (kept partial/none)');
      },
    );
    await sub.cancel();
    final ok = File('$captureDir/official_sfm_sparse.ply').existsSync();
    DeviceLog.log('SfmResume', 'recovered $captureDir → ply=$ok');
  } catch (e) {
    DeviceLog.log('SfmResume', 'resume error $captureDir: $e');
  } finally {
    if (recon != null) await recon.dispose();
    await _umbrella('endReconUmbrella', captureDir);
    await archiveLease?.close();
    await _clearMaterializedArchiveCache(captureDir);
  }
}

                                                                            
                                                                              
                                                               
                                                                                       
                                                                              
                   
Future<void> _persistColored(
  String captureDir,
  SfmLiveSnapshot snap, {
  Map<int, SfmFedFrameMeta>? frameMeta,
}) async {
  final n = snap.pointCount;
  if (n == 0) return;
  frameMeta ??= await loadFedFrameMeta(captureDir);
  frameMeta = await _materializeArchivedJpegs(captureDir, frameMeta);
  final offs = snap.obsOffsets;
  final fids = snap.obsFrameIds;
  final oxy = snap.obsXY;
  final rgb = Uint8List(n * 3);

  if (frameMeta.isEmpty || fids.isEmpty || offs.length != n + 1) {
                                                                            
                                                                        
    for (var i = 0; i < n; i++) {
      rgb[i * 3] = 185;
      rgb[i * 3 + 1] = 185;
      rgb[i * 3 + 2] = 190;
    }
    DeviceLog.log(
      'SfmResume',
      'colorize: no photo source for $captureDir → gray',
    );
    await _persistOfficialEndpoint(captureDir, snap, rgb);
    return;
  }

                                                                    
                                    
  final byFrame = <int, List<double>>{};
  final obsCap = Int32List(n);
  for (var i = 0; i < n; i++) {
    for (var j = offs[i]; j < offs[i + 1]; j++) {
      final f = fids[j];
      if (!frameMeta.containsKey(f)) continue;
      obsCap[i]++;
      (byFrame[f] ??= <double>[])
        ..add(i.toDouble())
        ..add(oxy[j * 2])
        ..add(oxy[j * 2 + 1]);
    }
  }

                                              
                                                               
                                               
                                             
  final samples = RepresentativeColorSamples(obsCap);
  final jobs = <ColorizeFrameJob>[];
  for (final entry in byFrame.entries) {
    final meta = frameMeta[entry.key]!;
    jobs.add(
      ColorizeFrameJob(
        jpegPath: meta.jpegPath,
        grayW: meta.grayW,
        grayH: meta.grayH,
        tri: entry.value,
      ),
    );
  }
  await sampleColorsPipelined(
    jobs: jobs,
    samples: samples,
    decode: _decodeJpegNative,
    maxInFlight: 3,
  );

  var colored = 0;
  for (var i = 0; i < n; i++) {
                                 
    if (samples.selectInto(i, rgb)) {
      colored++;
    } else {
      rgb[i * 3] = 185;
      rgb[i * 3 + 1] = 185;
      rgb[i * 3 + 2] = 190;
    }
  }
  DeviceLog.log(
    'SfmResume',
    'colorized ${snap.refined ? "refined" : "local"}: $colored/$n pts '
        'from ${byFrame.length} frames (${frameMeta.length} mapped)',
  );
  await _persistOfficialEndpoint(captureDir, snap, rgb);
}

                                                                               
                                                                               
                                                          
Future<void> _persistOfficialEndpoint(
  String captureDir,
  SfmLiveSnapshot snap,
  Uint8List rgb,
) async {
  final fsnap = SfmLiveSnapshot(
    xyz: snap.xyz,
    rgb: rgb,
    posesPacked: snap.posesPacked,
    summary: snap.summary,
    refined: snap.refined,
    obsOffsets: Int32List(0),
    obsFrameIds: Int32List(0),
    obsXY: Float32List(0),
  );
  await persistSparseSnapshot(
    captureDir: captureDir,
    snapshot: fsnap,
    rgb: rgb,
  );
}

Future<void> _prunePhotosAfterSparse(
  String captureDir,
  Set<String> keepJpegPaths,
) async {
  if (keepJpegPaths.isEmpty) return;
                                                                     
                                                                 
                                           
  final keepNames = <String>{
    for (final path in keepJpegPaths) ...<String>[
      path.split('/').last,
      path.split('/').last.replaceFirst(RegExp(r'\.jpg$'), '_hr.jpg'),
    ],
  };
  Future<void> pruneDir(String dirPath, {required bool sidecars}) async {
    final dir = Directory(dirPath);
    if (!await dir.exists()) return;
    await for (final entity in dir.list(followLinks: false)) {
      if (entity is! File) continue;
      final path = entity.path;
      if (!path.endsWith('.jpg') && !(sidecars && path.endsWith('.json'))) {
        continue;
      }
      final name = path.split('/').last;
      final jpgName = name.endsWith('.json')
          ? '${name.substring(0, name.length - '.json'.length)}.jpg'
          : name;
      if (keepNames.contains(jpgName)) continue;
      try {
        await entity.delete();
      } on FileSystemException {
                                                                              
      }
    }
  }

  await pruneDir('$captureDir/photos_highres', sidecars: true);
  await pruneDir('$captureDir/previews', sidecars: false);
  DeviceLog.log(
    'SfmResume',
    'post-sparse photo prune: kept=${keepNames.length} capture=$captureDir',
  );
}

                                                                               
                                                   
                                                           
                                                           
                                     
                                                     
                                                           
                                                                                     
                                                                             
                                                                            
                                                                             
                                                                                  
                                             
                                                         
                     
Future<Map<int, SfmFedFrameMeta>> loadFedFrameMeta(String captureDir) async {
  final photosDir = '$captureDir/photos_highres';
  final map = <int, SfmFedFrameMeta>{};

  List<double>? doubles(Object? v) => v is List
      ? v.map((e) => (e as num).toDouble()).toList(growable: false)
      : null;

  final sidecar = File('$captureDir/official_sfm_fed_frames.jsonl');
  if (sidecar.existsSync()) {
    try {
      for (final line in await sidecar.readAsLines()) {
        if (line.trim().isEmpty) continue;
        final m = jsonDecode(line) as Map<String, Object?>;
        final fid = m['frameId'] as int;
        final jpeg = '$photosDir/${(m['jpegPath'] as String).split('/').last}';
        final grayW = (m['grayW'] as num).toInt();
        final grayH = (m['grayH'] as num).toInt();
        map[fid] = SfmFedFrameMeta(
          jpegPath: jpeg,
          imageW: grayW,                                 
          imageH: grayH,
          grayW: grayW,
          grayH: grayH,
          fx: 0,
          fy: 0,
          cx: 0,
          cy: 0,
          arkitQuatWxyz: doubles(m['arkitCamFromWorldQwxyz']),
          arkitTransTxyz: doubles(m['arkitCamFromWorldTxyz']),
          arkitCameraCenterWorld: doubles(m['arkitCameraCenterWorld']),
        );
      }
    } catch (_) {}
    if (map.isNotEmpty) return map;
  }

                                                                     
                                             
  final dir = Directory(photosDir);
  if (!dir.existsSync()) return map;
                                                     
  final pwvaMastered = await PwvaMasterManifest.readNames(
    Directory(photosDir).parent,
  );
  final rows = <({double t, String jpeg, int w, int h})>[];
  for (final f in dir.listSync()) {
    if (!f.path.endsWith('.json')) continue;
    try {
      final j =
          jsonDecode(await File(f.path).readAsString()) as Map<String, Object?>;
      final t =
          (j['t'] as num?)?.toDouble() ??
          (j['save_target_t'] as num?)?.toDouble() ??
          0;
      final w = (j['image_w'] as num?)?.toInt() ?? 3840;
      final h = (j['image_h'] as num?)?.toInt() ?? 2160;
      final jpeg = f.path.replaceAll(RegExp(r'\.json$'), '.jpg');
      if (File(jpeg).existsSync() ||
          File('$jpeg.jxl').existsSync() ||
          File('$jpeg.lep').existsSync() ||
          pwvaMastered.contains(jpeg.split('/').last)) {
        rows.add((t: t, jpeg: jpeg, w: w, h: h));
      }
    } catch (_) {}
  }
  rows.sort((a, b) => a.t.compareTo(b.t));
  for (var i = 0; i < rows.length; i++) {
    map[i] = SfmFedFrameMeta(
      jpegPath: rows[i].jpeg,
      imageW: rows[i].w,
      imageH: rows[i].h,
      grayW: rows[i].w,
      grayH: rows[i].h,
      fx: 0,
      fy: 0,
      cx: 0,
      cy: 0,
    );
  }
  return map;
}

                                                                           
   
                                                                            
                                                                               
                               
Future<Map<int, SfmFedFrameMeta>> _materializeArchivedJpegs(
  String captureDir,
  Map<int, SfmFedFrameMeta> frameMeta,
) async {
  if (frameMeta.isEmpty) return frameMeta;
  try {
    final captureDirectory = Directory(captureDir);
    final cacheDirectory = await _archiveRestoreCacheDirectory(captureDir);
    final resolver = PhotoArchiveResolver(
      codec: photoArchiveCodec,
      codecsByName: photoArchiveCodecsByName,
    );
    final resolvedByName = <String, String>{};
    for (final meta in frameMeta.values) {
      final name = meta.jpegPath.split('/').last;
      if (resolvedByName.containsKey(name)) continue;
      final resolved = await resolver.resolveJpeg(
        captureDirectory: captureDirectory,
        highresFilename: name,
        cacheDirectory: cacheDirectory,
      );
      if (resolved != null) resolvedByName[name] = resolved.path;
    }
    if (resolvedByName.isEmpty) return frameMeta;
    return frameMeta.map((frameId, meta) {
      final resolvedPath = resolvedByName[meta.jpegPath.split('/').last];
      if (resolvedPath == null || resolvedPath == meta.jpegPath) {
        return MapEntry(frameId, meta);
      }
      return MapEntry(
        frameId,
        SfmFedFrameMeta(
          jpegPath: resolvedPath,
          imageW: meta.imageW,
          imageH: meta.imageH,
          grayW: meta.grayW,
          grayH: meta.grayH,
          fx: meta.fx,
          fy: meta.fy,
          cx: meta.cx,
          cy: meta.cy,
          captureTimestamp: meta.captureTimestamp,
          arkitQuatWxyz: meta.arkitQuatWxyz,
          arkitTransTxyz: meta.arkitTransTxyz,
          arkitCameraCenterWorld: meta.arkitCameraCenterWorld,
        ),
      );
    });
  } catch (e) {
    DeviceLog.log(
      'SfmResume',
      'archive materialization unavailable for $captureDir: $e',
    );
    return frameMeta;
  }
}

Future<Directory> _archiveRestoreCacheDirectory(String captureDir) async {
  final temporary = await getTemporaryDirectory();
                                                                            
  final captureName = sha256.convert(utf8.encode(Directory(captureDir).absolute.path)).toString();
  return Directory('${temporary.path}/worldswithin_photo_archive/$captureName');
}

Future<void> _clearMaterializedArchiveCache(String captureDir) async {
  try {
    final cache = await _archiveRestoreCacheDirectory(captureDir);
    if (await cache.exists()) await cache.delete(recursive: true);
  } catch (_) {
                                                                             
  }
}

                                                                      
                                                                       
Future<({Uint8List rgb, int w, int h})?> _decodeJpegNative(
  String jpegPath,
) async {
  try {
    final res = await _arKitChannel.invokeMethod<Map<Object?, Object?>>(
      'decodeJpegForColor',
      {'jpegPath': jpegPath, 'maxPx': kColorizeDecodeMaxPx},
    );
    if (res == null) return null;
    final w = res['w'] as int?, h = res['h'] as int?;
    final rgb = res['rgb'] as Uint8List?;
    if (w == null || h == null || rgb == null || w <= 0 || h <= 0) return null;
    if (rgb.length < w * h * 3) return null;
    return (rgb: rgb, w: w, h: h);
  } catch (_) {
    return null;
  }
}

Future<void> _umbrella(String method, String captureDir) async {
  try {
    await _arKitChannel.invokeMethod<void>(method, <String, Object?>{
      'jobId': captureDir,
    });
  } catch (_) {}
}

                                                                           
                            
  
                                                    
                                                   
                          
                                                          
                                                                 
                                                              
                                            
  
                                                     
                                                
  
                                                                 
                                                             
                                

                                         
                           
class ArchivedRebuildResult {
  const ArchivedRebuildResult({
    required this.photosFound,
    required this.accepted,
    required this.fed,
    required this.rejections,
    required this.plyWritten,
    this.failure,
  });

                                  
  final int photosFound;

                              
  final int accepted;

                                                      
  final int fed;

                                                 
  final List<String> rejections;

                                                      
  final bool plyWritten;

                                                
  final String? failure;

  bool get ok => plyWritten;
}

                                     
   
                                                   
                                           
int archivedPhotoCount(String captureDir) {
  try {
    final dir = Directory('$captureDir/photos_highres');
    if (!dir.existsSync()) return 0;
    return dir
        .listSync()
        .whereType<File>()
        .where((f) => f.path.toLowerCase().endsWith('.jpg'))
        .length;
  } catch (_) {
    return 0;
  }
}

                                                                   
   
                                                
                                             
                                         
ProjectCoverage projectCoverage(String captureDir) {
  final names = <String>[];
  try {
    final dir = Directory('$captureDir/photos_highres');
    if (dir.existsSync()) {
      for (final f in dir.listSync().whereType<File>()) {
        final n = f.path.split('/').last;
        if (n.toLowerCase().endsWith('.jpg')) names.add(n);
      }
    }
  } catch (_) {
                                  
  }
  var jsonl = '';
  try {
    final f = File('$captureDir/official_sfm_fed_frames.jsonl');
    if (f.existsSync()) jsonl = f.readAsStringSync();
  } catch (_) {
    jsonl = '';
  }
  return projectCoverageFrom(jpegNamesOnDisk: names, fedFramesJsonl: jsonl);
}

                                                
   
                                                  
                                                        
                                              
                                               
   
                                           
                                   
                                              
                                                             
                                                 
                                                             
                                             
   
                             
Future<List<String>> sidelineDatabaseForFreshSession(String captureDir, {bool Function()? canContinue}) async {
  final stamp = DateTime.now().millisecondsSinceEpoch;
  final moved = <String>[];
  const names = <String>[
    'official_sfm_live.db',
    'official_sfm_live.db-wal',
    'official_sfm_live.db-shm',
    'official_sfm_live.db.arkit_pose_v1',
    'official_sfm_fed_frames.jsonl',
  ];
  for (final n in names) {
    if (canContinue != null && !canContinue()) break;
    final f = File('$captureDir/$n');
    if (!f.existsSync()) continue;
    try {
      await f.rename('$captureDir/$n.dead-$stamp');
      moved.add(n);
    } catch (e) {
      DeviceLog.log('SfmResume', 'sideline failed for $n: $e');
    }
  }
  return moved;
}

                                            
   
                                               
                                                 
                                          
                                 
class ArchivedRefeedPlan {
  const ArchivedRefeedPlan({
    required this.photosFound,
    required this.ordered,
    required this.rejections,
  });

                                
  final int photosFound;

                      
  final List<ArchivedPhotoParse> ordered;

                                                       
  final List<String> rejections;
}

Future<ArchivedRefeedPlan> planArchivedRefeed(String captureDir) async {
  final rejections = <String>[];
  final photosDir = Directory('$captureDir/photos_highres');
  if (!photosDir.existsSync()) {
    return ArchivedRefeedPlan(
      photosFound: 0,
      ordered: const <ArchivedPhotoParse>[],
      rejections: rejections,
    );
  }
  final jpegs =
      photosDir
          .listSync()
          .whereType<File>()
          .map((f) => f.path)
          .where((p) => p.toLowerCase().endsWith('.jpg'))
          .toList()
        ..sort();
  final parses = <ArchivedPhotoParse>[];
  for (final jpeg in jpegs) {
    final sidecar = File('${jpeg.substring(0, jpeg.length - 4)}.json');
    if (!sidecar.existsSync()) {
      rejections.add('${jpeg.split('/').last}: 缺 sidecar .json');
      continue;
    }
    String text;
    try {
      text = await sidecar.readAsString();
    } catch (e) {
      rejections.add('${jpeg.split('/').last}: sidecar 读不出 ($e)');
      continue;
    }
    final p = parseArchivedPhoto(jpegPath: jpeg, sidecarJson: text);
    if (p.isAccepted) {
      parses.add(p);
    } else {
      rejections.add('${jpeg.split('/').last}: ${p.failure}');
    }
  }
  return ArchivedRefeedPlan(
    photosFound: jpegs.length,
    ordered: orderForRefeed(parses),
    rejections: rejections,
  );
}

                                                       
   
                                                                 
                
Future<ArchivedRebuildResult> rebuildFromArchivedPhotos(
  String captureDir, {
  ScanRecordStore? store,
  Duration timeout = const Duration(minutes: 25),
}) {
  final origin = store ?? ScanRecordStore.instance;
  final existing = _rebuildInFlight[captureDir];
  if (existing != null) return existing;
  final completer = Completer<ArchivedRebuildResult>();
  _rebuildInFlight[captureDir] = completer.future;
                                                    
  _resumeInFlight[captureDir] = completer.future.then((r) => r.ok);
  () async {
    ArchivedRebuildResult result;
    try {
      await _requireOfficialCapturePath(origin, captureDir);
      result = await _rebuildFromArchivedPhotosOnce(captureDir, timeout, origin);
    } catch (e, st) {
      DeviceLog.log('SfmResume', 'rebuild error $captureDir: $e\n$st');
      result = ArchivedRebuildResult(
        photosFound: 0,
        accepted: 0,
        fed: 0,
        rejections: const <String>[],
        plyWritten: File('$captureDir/official_sfm_sparse.ply').existsSync(),
        failure: '$e',
      );
    } finally {
      _rebuildInFlight.remove(captureDir);
      _resumeInFlight.remove(captureDir);
    }
    completer.complete(result);
  }();
  return completer.future;
}

Future<ArchivedRebuildResult> _rebuildFromArchivedPhotosOnce(
  String captureDir,
  Duration timeout,
  ScanRecordStore store,
) async {
  final plan = await planArchivedRefeed(captureDir);
  final rejections = List<String>.of(plan.rejections);
  final ordered = plan.ordered;
  DeviceLog.log(
    'SfmResume',
    'rebuild $captureDir: jpg=${plan.photosFound} accepted=${ordered.length} '
        'rejected=${rejections.length}',
  );
  if (ordered.isEmpty) {
    return ArchivedRebuildResult(
      photosFound: plan.photosFound,
      accepted: 0,
      fed: 0,
      rejections: rejections,
      plyWritten: false,
      failure: plan.photosFound == 0
          ? '照片目录为空或不存在:$captureDir/photos_highres'
          : '没有一张存档照片可用',
    );
  }

  SfmLiveRecon? recon;
  PhotoArchiveActivityLease? archiveLease;
  StreamSubscription<SfmLiveEvent>? sub;
  final done = Completer<void>();
  var fed = 0;
  String? failure;
  try {
    await _requireOfficialCapturePath(store, captureDir);
    archiveLease = photoArchiveCoordinatorFor(store).beginReconstructionActivity(
      Directory(captureDir),
    );
    await _umbrella('beginReconUmbrella', captureDir);

    final moved = await sidelineDatabaseForFreshSession(captureDir, canContinue: () => store.isActive);
    if (!store.isActive) throw StateError('Local workspace is revoked');
    DeviceLog.log('SfmResume', 'rebuild sidelined: ${moved.join(",")}');

                                                       
                                     
    recon = await SfmLiveRecon.start(
      dbPath: '$captureDir/official_sfm_live.db',
    );
    if (recon == null) {
      return ArchivedRebuildResult(
        photosFound: plan.photosFound,
        accepted: ordered.length,
        fed: 0,
        rejections: rejections,
        plyWritten: false,
        failure: '重建会话起不来(可能已有另一条重建在跑)',
      );
    }

    sub = recon.events.listen((e) {
      switch (e) {
        case SfmLiveLocalReady(:final snapshot):
          DeviceLog.log(
            'SfmResume',
            'rebuild local ignored: ${snapshot.pointCount} pts',
          );
        case SfmLiveRefined(:final snapshot):
                                                       
                                                      
          unawaited(() async {
            try {
                                                                  
                                                 
                                                   
                                     
              await _persistColored(captureDir, snapshot);
            } catch (e) {
              DeviceLog.log('SfmResume', 'rebuild persist failed: $e');
            } finally {
              if (!done.isCompleted) done.complete();
            }
          }());
        case SfmLiveFailed(:final stage, :final message):
          failure = '$stage $message';
          DeviceLog.log(
            'SfmResume',
            'rebuild $captureDir failed: $stage $message',
          );
          if (!done.isCompleted) done.complete();
        default:
          break;
      }
    });

    for (final p in ordered) {
      final input = p.input!;
      if (recon.offerFrame(input)) {
        fed++;
      } else {
                                                  
        rejections.add('${p.jpegPath.split('/').last}: 会话拒收(offerFrame)');
      }
    }
    DeviceLog.log('SfmResume', 'rebuild fed=$fed/${ordered.length}');
    if (fed == 0) {
      return ArchivedRebuildResult(
        photosFound: plan.photosFound,
        accepted: ordered.length,
        fed: 0,
        rejections: rejections,
        plyWritten: false,
        failure: '一张都没喂进去',
      );
    }

    recon.finalize();
    await done.future.timeout(
      timeout,
      onTimeout: () {
        failure = '超时(${timeout.inMinutes} 分钟)';
        DeviceLog.log('SfmResume', 'rebuild $captureDir timed out');
      },
    );
  } finally {
    await sub?.cancel();
    if (recon != null) await recon.dispose();
    await _umbrella('endReconUmbrella', captureDir);
    await archiveLease?.close();
    await _clearMaterializedArchiveCache(captureDir);
  }

  final ply = File('$captureDir/official_sfm_sparse.ply').existsSync();
  DeviceLog.log('SfmResume', 'rebuild $captureDir → ply=$ply fed=$fed');
  return ArchivedRebuildResult(
    photosFound: plan.photosFound,
    accepted: ordered.length,
    fed: fed,
    rejections: rejections,
    plyWritten: ply,
    failure: ply ? null : (failure ?? '重建没有产出点云'),
  );
}
