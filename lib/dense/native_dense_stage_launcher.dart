                                                                                                               
                                                                                                                 
                                                                                                                   
  
                                                                                                                  
                                                                                                                   
                                                                  
import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:path_provider/path_provider.dart' show getTemporaryDirectory;

import '../official_capture/dense_stage.dart';
import '../official_capture/photo_archive_resolver.dart';
import '../official_capture/aux_archive_resolver.dart';
import '../official_capture/photo_archive_manifest.dart';
import '../official_capture/photo_archive_runtime.dart' show photoArchiveCodec, photoArchiveCodecsByName, databaseArchiveCodec;
import '../official_capture/sfm_resume.dart' show loadFedFrameMeta;
import '../official_util/device_log.dart' as official_device_log;
import '../point_cloud_lod/dense_lod_cache.dart' show DenseLodCache, densePlyPoints;
import '../point_cloud_data/local_sparse_cloud.dart' show loadSparsePly;
import 'dense_live_cloud.dart';
import 'dense_work_state.dart';
import 'dense_stage_progress.dart';
import 'pw_dense_ffi.dart';

export 'dense_work_state.dart' show kDensePlyFileName, kDenseWorkDirName;

                                                                                                                                          
                                                                                                           
                                                                 
const int kDenseGatherConcurrency = 3;

                                       
                                                                                           
                                                                                                             
                                                                                                               
enum DenseFrameSource { plain, nv12, jpegMaterialised }

typedef DenseNv12Resolver =
    Future<({File file, int width, int height})?> Function(String highresFilename);
typedef DenseJpegResolver = Future<File?> Function(String highresFilename);

                                                                                                                   
                                                                                                               
                                                                                                    
Future<({DenseFrameSource source, String path, int width, int height})?> resolveDenseFrameSource({
  required String highresFilename,
  required String plainJpegPath,
  required bool plainExists,
  required bool hasRun3,
  required DenseNv12Resolver resolveNv12,
  required DenseJpegResolver resolveJpeg,
}) async {
  if (plainExists) {
    return (source: DenseFrameSource.plain, path: plainJpegPath, width: 0, height: 0);
  }
                                                                                                    
  if (hasRun3) {
    final n = await resolveNv12(highresFilename);
    if (n != null) {
      return (source: DenseFrameSource.nv12, path: n.file.path, width: n.width, height: n.height);
    }
  }
  final j = await resolveJpeg(highresFilename);
  if (j == null) return null;
  return (source: DenseFrameSource.jpegMaterialised, path: j.path, width: 0, height: 0);
}

                                                                                                             
                                              
Future<List<T>> mapBoundedConcurrent<S, T>(
  List<S> items,
  int limit,
  Future<T> Function(S item) body,
) async {
  if (items.isEmpty) return <T>[];
  final out = List<T?>.filled(items.length, null);
  var cursor = 0;
  Future<void> worker() async {
    while (true) {
      final i = cursor++;
      if (i >= items.length) return;
      out[i] = await body(items[i]);
    }
  }

  final workers = limit < items.length ? limit : items.length;
  await Future.wait(List<Future<void>>.generate(workers < 1 ? 1 : workers, (_) => worker()));
  return List<T>.from(out);
}

                                                                                  
                                                                                                 
String? denseDenyReason(String captureDir) {
  final n = densePlyPoints('$captureDir/$kDensePlyFileName');
  return n == null ? null : 'complete $kDensePlyFileName on disk ($n pts): dense is done, never re-trained';
}

class NativeDenseStageLauncher implements DenseStageLauncher {
  NativeDenseStageLauncher() : _ffi = PwDenseFfi.tryResolve();

                                                                                 
  NativeDenseStageLauncher.forCaptureInputs() : _ffi = null;

  final PwDenseFfi? _ffi;
  bool _running = false;

  @override
  bool get isAvailable => _ffi != null;

  @override
  Future<DenseStageResult> start(DenseStageRequest request) async {
                                                                  
                                                                                                   
                                                                                      
    final existing = denseDenyReason(request.captureDir);
    if (existing != null) {
      official_device_log.DeviceLog.log('DenseStage', 'refused: $existing (${request.captureDir})');
      return DenseStageResult(DenseStageStatus.failed, message: '稠密点云已经生成过,不会重新训练');
    }
    if (_ffi == null) {
      return DenseStageResult(DenseStageStatus.unavailable, message: PwDenseFfi.lastError);
    }
    if (_running) {
      return const DenseStageResult(DenseStageStatus.failed, message: '稠密处理已在进行中,请等它完成');
    }
    final DenseStageInputs inputs;
    try {
      inputs = await gatherInputs(request);
    } catch (e) {
      official_device_log.DeviceLog.log('DenseStage', 'gather failed: $e');
      return DenseStageResult(DenseStageStatus.failed, message: '稠密输入不完整: $e');
    }
    _running = true;
                                                                                                
                                                                                         
                                                                      
    writeDenseStartedMarker(request.captureDir, selection: request.selection, sparsePoints: request.pointCount);
                                                                                                      
                                                                                          
    DenseLodCache.instance.discardTree(request.captureDir);
                                                                                                               
                                                                     
    final live = DenseLiveCloud(framesPlanned: inputs.frames.length);
    denseStageProgress.value = DenseStageProgress(
        captureDir: request.captureDir, state: DenseStageState.running, phase: 'session', startedAt: DateTime.now(), live: live);
    unawaited(_runJob(request, inputs, live));
    final note = request.selection != null ? '(只处理选区内)' : '(整朵云)';
    return DenseStageResult(DenseStageStatus.started, message: '稠密处理已开始$note');
  }

  Future<void> _runJob(DenseStageRequest request, DenseStageInputs inputs, DenseLiveCloud live) async {
    final dir = request.captureDir;
    final outPly = '$dir/$kDensePlyFileName';
    final workDir = '$dir/$kDenseWorkDirName';
    final t0 = DateTime.now();
    final sel = request.selection;
    final box = sel == null
        ? null
        : PwDenseBox(cx: sel.cx, cy: sel.cy, cz: sel.cz, sx: sel.sx, sy: sel.sy, sz: sel.sz, rot: List<double>.from(sel.rot, growable: false));
    official_device_log.DeviceLog.log(
      'DenseStage',
      'start capture=$dir frames=${inputs.frames.length} points=${inputs.pointsXyz.length ~/ 3} '
      'abi=${_ffi?.abiVersion()} chunks=${_ffi?.hasChunkApi == true} run3=${_ffi?.hasRun3 == true} '
      'box=${box == null ? 'none' : '${box.cx.toStringAsFixed(3)},${box.cy.toStringAsFixed(3)},${box.cz.toStringAsFixed(3)} ${box.sx.toStringAsFixed(3)}x${box.sy.toStringAsFixed(3)}x${box.sz.toStringAsFixed(3)}'}',
    );
    try {
      final r = await runPwDenseJob(
        frames: inputs.frames,
        pointsXyz: inputs.pointsXyz,
        workDir: workDir,
        outPly: outPly,
        box: box,
        onProgress: (p) {
          final cur = denseStageProgress.value;
          if (cur == null || cur.captureDir != dir) return;
          denseStageProgress.value = cur.copyWith(phase: p.phase, done: p.done, total: p.total);
        },
                                                                                                          
        onChunk: (c) {
          live.addChunk(frameIndex: c.frameIndex, xyz: c.xyz, rgb: c.rgb);
          if (live.frames % 10 == 1) {
            official_device_log.DeviceLog.log(
                'DenseStage', 'live chunk f=${c.frameIndex} n=${c.pointCount} total=${live.points}');
          }
          final cur = denseStageProgress.value;
          if (cur == null || cur.captureDir != dir) return;
                                                                                                   
          denseStageProgress.value = cur.copyWith(live: live);
        },
      );
      final secs = DateTime.now().difference(t0).inMilliseconds / 1000.0;
      final s = r.stats;
      official_device_log.DeviceLog.log(
        'DenseStage',
        'end rc=${r.code} ${secs.toStringAsFixed(1)}s frames=${s.frames} selected=${s.framesSelected}${s.boxFallback ? '(fallback:all)' : ''} inferred=${s.inferred} images=${s.images} '
        'session=${s.sessionMs.toStringAsFixed(0)}ms images=${s.imagesMs.toStringAsFixed(0)}ms ort=${s.ortSessionMs.toStringAsFixed(0)}ms '
        'infer_med=${s.inferMsMedian.toStringAsFixed(1)}ms infer_total=${(s.inferMsTotal / 1000).toStringAsFixed(1)}s '
        'fuse=${s.fuseMs.toStringAsFixed(0)}ms points=${s.points} final=${(s.finalFrac * 100).toStringAsFixed(2)}% err="${s.error}"',
      );
      if (r.ok) {
        denseStageProgress.value = DenseStageProgress(
          captureDir: dir,
          state: DenseStageState.done,
          phase: 'done',
          outPly: outPly,
          points: s.points,
          startedAt: t0,
          finishedAt: DateTime.now(),
        );
      } else {
        denseStageProgress.value = DenseStageProgress(
          captureDir: dir,
          state: DenseStageState.failed,
          phase: 'failed',
          message: DenseStageProgress.shorten('rc=${r.code} ${s.error}'),
          startedAt: t0,
          finishedAt: DateTime.now(),
        );
      }
    } catch (e, st) {
      official_device_log.DeviceLog.log('DenseStage', 'exception: $e\n$st');
      denseStageProgress.value = DenseStageProgress(
        captureDir: dir,
        state: DenseStageState.failed,
        phase: 'failed',
        message: DenseStageProgress.shorten(e),
        startedAt: t0,
        finishedAt: DateTime.now(),
      );
    } finally {
      _running = false;
                                                                                                          
      try {
        final w = Directory(workDir);
        if (w.existsSync()) await w.delete(recursive: true);
      } catch (_) {}
      try {
        final captureName = Directory(dir).uri.pathSegments.where((x) => x.isNotEmpty).last;
        final c = Directory('${(await getTemporaryDirectory()).path}/worldswithin_dense_cache/$captureName');
        if (c.existsSync()) await c.delete(recursive: true);
      } catch (_) {}
    }
  }

                                                                                                             
                                                                             
                                                                                 
                                                                                 
                                                             
                                                                   
  Future<DenseStageInputs> gatherInputs(
    DenseStageRequest request, {
    bool forceJpeg = false,
    bool sortByFrameId = false,
    int? expectedFrameCount,
  }) async {
    final strictFullPipeline = expectedFrameCount != null || (forceJpeg && sortByFrameId);
    final tGather = DateTime.now();
    final dir = request.captureDir;
    if (strictFullPipeline) {
      final captureRoot = await Directory(dir).resolveSymbolicLinks();
      final sparseRoot = await File(
        request.sparsePlyPath,
      ).resolveSymbolicLinks();
      if (sparseRoot != '$captureRoot/official_sfm_sparse.ply') {
        throw StateError('完整38的稀疏点云必须属于当前拍摄会话');
      }
    }
    final metaFile = File('$dir/official_sfm_sparse_meta.json');
    if (!metaFile.existsSync())
      throw StateError('缺 official_sfm_sparse_meta.json');
    final meta =
        jsonDecode(await metaFile.readAsString()) as Map<String, Object?>;
    final posesRaw = (meta['poses'] as List?) ?? const [];
    final fed = await loadFedFrameMeta(dir);
    final resolver = PhotoArchiveResolver(
      codec: photoArchiveCodec,
      codecsByName: photoArchiveCodecsByName,
    );
                                                                                                        
    final captureName = Directory(
      dir,
    ).uri.pathSegments.where((x) => x.isNotEmpty).last;
    final cacheDir = Directory(
      '${(await getTemporaryDirectory()).path}/worldswithin_dense_cache/$captureName',
    );
                                                                                                             
    final pending = <_PendingFrame>[];
    final sidecars = !strictFullPipeline
        ? null
        : await AuxArchiveResolver(
            codec: databaseArchiveCodec,
          ).openSidecars(Directory(dir));
    if (strictFullPipeline && sidecars == null) {
      throw StateError('完整38缺少可校验的完整侧车');
    }
    try {
      for (final p in posesRaw) {
        final m = p as Map<String, Object?>;
        if (m['registered'] != true) continue;
        final originalId = (m['frame_id'] as num).toDouble();
        final fid = (m['frame_id'] as num).toInt();
                                                                                
                                                                                  
        if (strictFullPipeline &&
            (!originalId.isFinite || originalId != fid.toDouble())) {
          throw StateError('frame ID $originalId 没有无损的喂入照片映射');
        }
        final q = (m['quat_wxyz'] as List)
            .map((e) => (e as num).toDouble())
            .toList(growable: false);
        final t = (m['t'] as List)
            .map((e) => (e as num).toDouble())
            .toList(growable: false);
        final fm = fed[fid];
        if (fm == null) throw StateError('frame $fid 没有喂入照片记录');
        final base = fm.jpegPath
            .split('/')
            .last;                                   
        final sidecar = File(
          '${sidecars?.directory.path ?? '$dir/photos_highres'}/${base.replaceFirst(RegExp(r'\.jpg$'), '.json')}',
        );
        if (!sidecar.existsSync())
          throw StateError(
            'frame $fid 缺 sidecar ${sidecar.path.split('/').last}',
          );
        final sc =
            jsonDecode(await sidecar.readAsString()) as Map<String, Object?>;
        final k = (sc['intrinsics_fxfycxcy'] as List)
            .map((e) => (e as num).toDouble())
            .toList(growable: false);
        final iw = (sc['image_w'] as num).toDouble(),
            ih = (sc['image_h'] as num).toDouble();
        pending.add(
          _PendingFrame(
            frameId: fid,
            originalFrameId: originalId,
            base: base,
            plainJpegPath: fm.jpegPath,
            q: q,
            t: t,
            k: k,
            imageW: iw,
            imageH: ih,
          ),
        );
      }
    } finally {
      await sidecars?.dispose();
    }
    if (pending.isEmpty) throw StateError('没有已注册的位姿');
    if (sortByFrameId) {
      pending.sort((a, b) => a.originalFrameId.compareTo(b.originalFrameId));
      for (var i = 1; i < pending.length; i++) {
        if (pending[i - 1].frameId == pending[i].frameId) {
          throw StateError('重复已注册 frame ID ${pending[i].frameId}');
        }
      }
    }
    if (expectedFrameCount != null && pending.length != expectedFrameCount) {
      throw StateError('需要 $expectedFrameCount 个已注册帧，实际 ${pending.length}');
    }

                                                                                                                
                                                                                                  
    final hasRun3 = !forceJpeg && _ffi?.hasRun3 == true;
    final decided = await mapBoundedConcurrent(
      pending,
      kDenseGatherConcurrency,
      (pf) async {
        return resolveDenseFrameSource(
          highresFilename: pf.base,
          plainJpegPath: pf.plainJpegPath,
          plainExists: File(pf.plainJpegPath).existsSync(),
          hasRun3: hasRun3,
          resolveNv12: (name) => resolver.resolveNv12(
            captureDirectory: Directory(dir),
            highresFilename: name,
            cacheDirectory: cacheDir,
          ),
          resolveJpeg: (name) => resolver.resolveJpeg(
            captureDirectory: Directory(dir),
            highresFilename: name,
            cacheDirectory: cacheDir,
          ),
        );
      },
    );

    final frames = <PwDenseFrame>[];
    final origins = <String>[];
    final archive = !strictFullPipeline
        ? null
        : await PhotoArchiveManifest.read(Directory(dir));
    var plainCount = 0, nv12Count = 0, jpegCount = 0;
    for (var i = 0; i < pending.length; i++) {
      final pf = pending[i];
      final d = decided[i];
      if (d == null)
        throw StateError('frame ${pf.frameId} 的照片 ${pf.base} 无法还原');
      switch (d.source) {
        case DenseFrameSource.plain:
          plainCount++;
        case DenseFrameSource.nv12:
          nv12Count++;
        case DenseFrameSource.jpegMaterialised:
          jpegCount++;
      }
      origins.add(
        d.source == DenseFrameSource.plain
            ? 'actual_plain_jpeg'
            : d.source == DenseFrameSource.nv12
            ? 'actual_nv12'
            : archive?.entries.containsKey(pf.base) == true
            ? 'byte_restored_archive_jpeg'
            : 'pwva_decoded_reencoded_jpeg',
      );
      final isNv12 = d.source == DenseFrameSource.nv12;
      frames.add(
        PwDenseFrame(
          frameId: pf.frameId,
          fx: pf.k[0],
          fy: pf.k[1],
          cx: pf.k[2],
          cy: pf.k[3],
          imageW: pf.imageW,
          imageH: pf.imageH,
          qWxyz: pf.q,
          t: pf.t,
          jpegPath: isNv12 ? '' : d.path,
          nv12Path: isNv12 ? d.path : null,
          nv12Width: isNv12 ? d.width : null,
          nv12Height: isNv12 ? d.height : null,
        ),
      );
    }
    final cloud = loadSparsePly(request.sparsePlyPath);
    if (cloud == null || cloud.count == 0) throw StateError('稀疏点云读不出来');
    if (strictFullPipeline && cloud.count != request.pointCount) {
      throw StateError('完整38的稀疏点数与当前请求身份不一致');
    }
    official_device_log.DeviceLog.log(
      'DenseStage',
      'gather ${DateTime.now().difference(tGather).inMilliseconds}ms frames=${frames.length} '
          'plain=$plainCount nv12=$nv12Count jpeg_materialised=$jpegCount',
    );
    return DenseStageInputs(
      frames,
      cloud.xyz,
      frameIdentities: [
        for (final p in pending)
          DenseFrameIdentity.fromDouble(p.originalFrameId),
      ],
      jpegOrigins: origins,
    );
  }
}

                                                                                              
final class _PendingFrame {
  const _PendingFrame({
    required this.frameId,
    required this.originalFrameId,
    required this.base,
    required this.plainJpegPath,
    required this.q,
    required this.t,
    required this.k,
    required this.imageW,
    required this.imageH,
  });
  final int frameId;
  final double originalFrameId;
  final String base;                                   
  final String plainJpegPath;
  final List<double> q, t, k;
  final double imageW, imageH;
}

                                                                            
                                                                                
final class DenseStageInputs {
  const DenseStageInputs(
    this.frames,
    this.pointsXyz, {
    this.frameIdentities,
    this.jpegOrigins,
  });
  final List<PwDenseFrame> frames;
  final List<double> pointsXyz;
  final List<DenseFrameIdentity>? frameIdentities;
  final List<String>? jpegOrigins;
}

                                                                                 
final class DenseFrameIdentity {
  const DenseFrameIdentity(this.frameId, this.frameIdF64Bits);
  factory DenseFrameIdentity.fromDouble(double value) {
    final bytes = ByteData(8)..setFloat64(0, value, Endian.big);
                                                                               
                                                                                
    final high = bytes
        .getUint32(0, Endian.big)
        .toRadixString(16)
        .padLeft(8, '0');
    final low = bytes
        .getUint32(4, Endian.big)
        .toRadixString(16)
        .padLeft(8, '0');
    return DenseFrameIdentity(value, '$high$low');
  }
  final double frameId;
  final String frameIdF64Bits;
}
