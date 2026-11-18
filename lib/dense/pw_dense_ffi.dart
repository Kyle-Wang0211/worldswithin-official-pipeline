                                                                                                   
  
                                                                                                               
                                                                                                             
                                                                                                         
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

                                                                                                                  
                                                                                                                   
                                                                                                                  
                                                                                                     
                                                                                      
const int pwDenseAbiVersionMin = 1;
const int pwDenseAbiVersionMax = 3;

                                                                                                  
                                                                                                
const int kPwvaNv12Matrix = 1;                                                                                                                             

final class PwDenseFrame {
  const PwDenseFrame({
    required this.frameId,
    required this.fx,
    required this.fy,
    required this.cx,
    required this.cy,
    required this.imageW,
    required this.imageH,
    required this.qWxyz,
    required this.t,
    required this.jpegPath,
    this.nv12Path,
    this.nv12Width,
    this.nv12Height,
  });
  final int frameId;
  final double fx, fy, cx, cy, imageW, imageH;
  final List<double> qWxyz;     
  final List<double> t;     

                                                                                              
  final String jpegPath;

                                                                                                             
                                                                                               
  final String? nv12Path;
  final int? nv12Width, nv12Height;

                                                                                               
  bool get hasNv12 => nv12Path != null && nv12Path!.isNotEmpty;
}

                                                                                                                
final class PwDenseBox {
  const PwDenseBox({required this.cx, required this.cy, required this.cz, required this.sx, required this.sy, required this.sz, required this.rot});
  final double cx, cy, cz, sx, sy, sz;
  final List<double> rot;     
}

final class PwDenseStats {
  const PwDenseStats({
    required this.frames,
    required this.inferred,
    required this.images,
    required this.framesSelected,
    required this.boxFallback,
    required this.sessionMs,
    required this.imagesMs,
    required this.ortSessionMs,
    required this.inferMsMedian,
    required this.inferMsTotal,
    required this.fuseMs,
    required this.points,
    required this.finalFrac,
    required this.error,
  });
  final int frames, inferred, images, points, framesSelected;
  final bool boxFallback;
  final double sessionMs, imagesMs, ortSessionMs, inferMsMedian, inferMsTotal, fuseMs, finalFrac;
  final String error;
}

final class PwDenseResult {
  const PwDenseResult(this.code, this.stats);
  final int code;                                                                 
  final PwDenseStats stats;
  bool get ok => code == 0;
}

                                             
final class _FrameRaw extends Struct {
  @Int32()
  external int frameId;
  @Double()
  external double fx;
  @Double()
  external double fy;
  @Double()
  external double cx;
  @Double()
  external double cy;
  @Double()
  external double imageW;
  @Double()
  external double imageH;
  @Array(4)
  external Array<Double> qWxyz;
  @Array(3)
  external Array<Double> t;
  external Pointer<Utf8> jpegPath;
}

                                                                                                             
                                                                                                         
                                                                                                                  
final class _FrameRawV3 extends Struct {
  @Int32()
  external int frameId;
  @Double()
  external double fx;
  @Double()
  external double fy;
  @Double()
  external double cx;
  @Double()
  external double cy;
  @Double()
  external double imageW;
  @Double()
  external double imageH;
  @Array(4)
  external Array<Double> qWxyz;
  @Array(3)
  external Array<Double> t;
  external Pointer<Utf8> jpegPath;
  external Pointer<Utf8> nv12Path;
  @Int32()
  external int nv12Width;
  @Int32()
  external int nv12Height;
  @Int32()
  external int nv12Matrix;
  @Int32()
  external int reserved0;
}

final class _OptionsRaw extends Struct {
  @Int32()
  external int width;
  @Int32()
  external int height;
  @Int32()
  external int nsrc;
  @Int32()
  external int webgpu;
  external Pointer<Utf8> modelPath;
  external Pointer<Utf8> workDir;
  external Pointer<Utf8> outPly;
  @Uint64()
  external int noiseSeed;
  @Int32()
  external int hasBox;
  @Array(3)
  external Array<Double> boxCenter;
  @Array(3)
  external Array<Double> boxSize;
  @Array(9)
  external Array<Double> boxRot;
}

final class _StatsRaw extends Struct {
  @Int32()
  external int frames;
  @Int32()
  external int inferred;
  @Int32()
  external int images;
  @Int32()
  external int framesSelected;
  @Int32()
  external int boxFallback;
  @Double()
  external double sessionMs;
  @Double()
  external double imagesMs;
  @Double()
  external double ortSessionMs;
  @Double()
  external double inferMsMedian;
  @Double()
  external double inferMsTotal;
  @Double()
  external double fuseMs;
  @Uint64()
  external int points;
  @Double()
  external double photoFrac;
  @Double()
  external double geoFrac;
  @Double()
  external double finalFrac;
  @Array(256)
  external Array<Char> error;
}

typedef _ProgressFnNative = Int32 Function(Pointer<Utf8> phase, Int32 done, Int32 total, Pointer<Void> user);
typedef _AbiVersionC = Int32 Function();
typedef _AbiVersionD = int Function();
typedef _OptionsDefaultC = Int32 Function(Pointer<_OptionsRaw>);
typedef _OptionsDefaultD = int Function(Pointer<_OptionsRaw>);
typedef _DefaultModelPathC = Pointer<Utf8> Function();
typedef _DefaultModelPathD = Pointer<Utf8> Function();
typedef _RunC = Int32 Function(Pointer<_FrameRaw>, Int32, Pointer<Float>, Int32, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<Void>, Pointer<_StatsRaw>);
typedef _RunD = int Function(Pointer<_FrameRaw>, int, Pointer<Float>, int, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<Void>, Pointer<_StatsRaw>);
                                                                                                    
typedef _ChunkFnNative = Int32 Function(Int32 frameIndex, Pointer<Float> xyz, Pointer<Uint8> rgb, Int32 nPoints,
    Pointer<Void> user);
typedef _Run2C = Int32 Function(Pointer<_FrameRaw>, Int32, Pointer<Float>, Int32, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<NativeFunction<_ChunkFnNative>>, Pointer<Void>,
    Pointer<_StatsRaw>);
typedef _Run2D = int Function(Pointer<_FrameRaw>, int, Pointer<Float>, int, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<NativeFunction<_ChunkFnNative>>, Pointer<Void>,
    Pointer<_StatsRaw>);
                                                                
typedef _Run3C = Int32 Function(Pointer<_FrameRawV3>, Int32, Pointer<Float>, Int32, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<NativeFunction<_ChunkFnNative>>, Pointer<Void>,
    Pointer<_StatsRaw>);
typedef _Run3D = int Function(Pointer<_FrameRawV3>, int, Pointer<Float>, int, Pointer<_OptionsRaw>,
    Pointer<NativeFunction<_ProgressFnNative>>, Pointer<NativeFunction<_ChunkFnNative>>, Pointer<Void>,
    Pointer<_StatsRaw>);

class PwDenseFfi {
  PwDenseFfi._(DynamicLibrary lib)
      : _abiVersion = lib.lookupFunction<_AbiVersionC, _AbiVersionD>('pwdense_abi_version'),
        _available = lib.lookupFunction<_AbiVersionC, _AbiVersionD>('pwdense_available'),
        _optionsDefault = lib.lookupFunction<_OptionsDefaultC, _OptionsDefaultD>('pwdense_options_default'),
        _defaultModelPath = lib.lookupFunction<_DefaultModelPathC, _DefaultModelPathD>('pwdense_default_model_path'),
        _run = lib.lookupFunction<_RunC, _RunD>('pwdense_run'),
        _run2 = _tryLookupRun2(lib),
        _run3 = _tryLookupRun3(lib);

                                                                                                             
  static _Run2D? _tryLookupRun2(DynamicLibrary lib) {
    try {
      return lib.lookupFunction<_Run2C, _Run2D>('pwdense_run2');
    } catch (_) {
      return null;
    }
  }

                                                                                   
  static _Run3D? _tryLookupRun3(DynamicLibrary lib) {
    try {
      return lib.lookupFunction<_Run3C, _Run3D>('pwdense_run3');
    } catch (_) {
      return null;
    }
  }

  final _AbiVersionD _abiVersion;
  final _AbiVersionD _available;
  final _OptionsDefaultD _optionsDefault;
  final _DefaultModelPathD _defaultModelPath;
  final _RunD _run;
  final _Run2D? _run2;
  final _Run3D? _run3;

  int abiVersion() => _abiVersion();

                                                                                               
  bool get hasChunkApi => _run2 != null || _run3 != null;

                                                                                                            
  bool get hasRun3 => _run3 != null;
  int available() => _available();
  String defaultModelPath() {
    final p = _defaultModelPath();
    return p == nullptr ? '' : p.toDartString();
  }

  static PwDenseFfi? _cached;
  static String? lastError;

  static List<String> _candidatePaths() {
    final out = <String>[];
    final env = Platform.environment['PW_DENSE_FRAMEWORK'] ?? '';
    if (env.isNotEmpty) out.add(env);
    final exeDir = File(Platform.resolvedExecutable).absolute.parent;
    out.add('${exeDir.path}/Frameworks/PWDense.framework/PWDense');       
    out.add('${exeDir.parent.path}/Frameworks/PWDense.framework/PWDense');         
    return out;
  }

                                                                                                              
                                                    
  static PwDenseFfi? tryResolve() {
    if (_cached != null) return _cached;
    for (final p in _candidatePaths()) {
      if (!File(p).existsSync()) continue;
      try {
        final lib = DynamicLibrary.open(p);
        final ffi = PwDenseFfi._(lib);
        final abi = ffi.abiVersion();
        if (abi < pwDenseAbiVersionMin || abi > pwDenseAbiVersionMax) {
          lastError = 'PWDense ABI $abi outside [$pwDenseAbiVersionMin, $pwDenseAbiVersionMax]';
          return null;
        }
        if (ffi.available() == 0) {
          lastError = 'PWDense stub slice (simulator)';
          return null;
        }
        _cached = ffi;
        return ffi;
      } catch (e) {
        lastError = 'open $p: $e';
      }
    }
    lastError ??= 'no PWDense.framework candidate exists';
    return null;
  }
}

                                                        
final class PwDenseProgress {
  const PwDenseProgress(this.phase, this.done, this.total);
  final String phase;
  final int done, total;
}

                                                                                                            
                                                                                                                 
                                                                                                         
final class PwDenseChunk {
  const PwDenseChunk(this.frameIndex, this.xyz, this.rgb);
  final int frameIndex;
  final Float32List xyz;
  final Uint8List rgb;
  int get pointCount => xyz.length ~/ 3;
}

                                                                                  
const String _kChunkTag = 'chunk';

                                                                                                    
                                                                                                        
                                                                                                           
PwDenseChunk? decodePwDenseChunkMessage(Object? msg) {
  if (msg is! List || msg.length != 4 || msg[0] != _kChunkTag) return null;
  final frameIndex = msg[1];
  final xyz = msg[2];
  final rgb = msg[3];
  if (frameIndex is! int || xyz is! TransferableTypedData || rgb is! TransferableTypedData) return null;
  return PwDenseChunk(frameIndex, xyz.materialize().asFloat32List(), rgb.materialize().asUint8List());
}

final class _JobArgs {
  const _JobArgs(this.frames, this.pointsXyz, this.workDir, this.outPly, this.webgpu, this.progressPort, this.box,
      this.wantChunks);
  final List<PwDenseFrame> frames;
  final List<double> pointsXyz;            
  final String workDir, outPly;
  final bool webgpu;

                                                                                 
  final SendPort? progressPort;
  final PwDenseBox? box;
  final bool wantChunks;
}

                                                                                                          
                                                                                                     
Future<PwDenseResult> runPwDenseJob({
  required List<PwDenseFrame> frames,
  required List<double> pointsXyz,
  required String workDir,
  required String outPly,
  bool webgpu = true,
  PwDenseBox? box,
  void Function(PwDenseProgress p)? onProgress,
  void Function(PwDenseChunk c)? onChunk,
}) async {
  ReceivePort? progressPort;
  if (onProgress != null || onChunk != null) {
    progressPort = ReceivePort();
    progressPort.listen((msg) {
      final chunk = decodePwDenseChunkMessage(msg);
      if (chunk != null) {
        onChunk?.call(chunk);
        return;
      }
      if (onProgress != null && msg is List && msg.length == 3) {
        onProgress(PwDenseProgress(msg[0] as String, msg[1] as int, msg[2] as int));
      }
    });
  }
  final args =
      _JobArgs(frames, pointsXyz, workDir, outPly, webgpu, progressPort?.sendPort, box, onChunk != null);
  try {
    return await _spawn(args);
  } finally {
    progressPort?.close();
  }
}

                                                                                                                
                                                                                                              
/// `onProgress` callback along and `Isolate.run` refuses to send it ("object is unsendable — _ReceivePortImpl",
                                                                                   
Future<PwDenseResult> _spawn(_JobArgs args) => Isolate.run(() => _runInIsolate(args));

PwDenseResult _runInIsolate(_JobArgs a) {
  final ffi = PwDenseFfi.tryResolve();
  if (ffi == null) {
    return PwDenseResult(-1, _emptyStats(PwDenseFfi.lastError ?? 'PWDense unavailable'));
  }
  final n = a.frames.length;
                                                                                                          
                                                                                                                
                                                                                                              
  final run3 = ffi._run3;
  if (run3 == null) {
    for (final f in a.frames) {
      if (f.hasNv12) {
        throw StateError('frame ${f.frameId} 是 NV12 源,但 PWDense 框架没有 pwdense_run3(ABI < 3)');
      }
    }
  }
  final strings = <Pointer<Utf8>>[];
  final framesPtr = run3 == null ? calloc<_FrameRaw>(n) : nullptr;
  final framesV3Ptr = run3 == null ? nullptr : calloc<_FrameRawV3>(n);
  for (var i = 0; i < n; i++) {
    final f = a.frames[i];
    if (run3 == null) {
      _fillFrameV2(framesPtr[i], f, strings);
    } else {
      _fillFrameV3(framesV3Ptr[i], f, strings);
    }
  }
  final np = a.pointsXyz.length ~/ 3;
  final ptsPtr = calloc<Float>(np * 3);
  ptsPtr.asTypedList(np * 3).setAll(0, a.pointsXyz);
  final opts = calloc<_OptionsRaw>();
  ffi._optionsDefault(opts);
  final workDirC = a.workDir.toNativeUtf8();
  final outPlyC = a.outPly.toNativeUtf8();
  opts.ref.workDir = workDirC;
  opts.ref.outPly = outPlyC;
  opts.ref.webgpu = a.webgpu ? 1 : 0;
  opts.ref.modelPath = nullptr;                                                                                
  final box = a.box;
  if (box != null) {
    opts.ref.hasBox = 1;
    opts.ref.boxCenter[0] = box.cx;
    opts.ref.boxCenter[1] = box.cy;
    opts.ref.boxCenter[2] = box.cz;
    opts.ref.boxSize[0] = box.sx;
    opts.ref.boxSize[1] = box.sy;
    opts.ref.boxSize[2] = box.sz;
    for (var k = 0; k < 9; k++) {
      opts.ref.boxRot[k] = box.rot[k];
    }
  }
  final stats = calloc<_StatsRaw>();

  NativeCallable<_ProgressFnNative>? cb;
  Pointer<NativeFunction<_ProgressFnNative>> cbPtr = nullptr;
  final port = a.progressPort;
  if (port != null) {
    cb = NativeCallable<_ProgressFnNative>.isolateLocal(
      (Pointer<Utf8> phase, int done, int total, Pointer<Void> user) {
        port.send(<Object>[phase == nullptr ? '' : phase.toDartString(), done, total]);
        return 0;                         
      },
      exceptionalReturn: 0,
    );
    cbPtr = cb.nativeFunction;
  }
                                                                                                               
                                                                                                              
                                                                                           
  final run2 = ffi._run2;
  NativeCallable<_ChunkFnNative>? chunkCb;
  if (port != null && a.wantChunks && (run2 != null || run3 != null)) {
    chunkCb = NativeCallable<_ChunkFnNative>.isolateLocal(
      (int frameIndex, Pointer<Float> xyz, Pointer<Uint8> rgb, int nPoints, Pointer<Void> user) {
        final cnt = nPoints > 0 && xyz != nullptr && rgb != nullptr ? nPoints : 0;
        final xyzCopy = cnt == 0 ? Float32List(0) : Float32List.fromList(xyz.asTypedList(cnt * 3));
        final rgbCopy = cnt == 0 ? Uint8List(0) : Uint8List.fromList(rgb.asTypedList(cnt * 3));
        port.send(<Object>[
          _kChunkTag,
          frameIndex,
          TransferableTypedData.fromList(<TypedData>[xyzCopy]),
          TransferableTypedData.fromList(<TypedData>[rgbCopy]),
        ]);
        return 0;                                       
      },
      exceptionalReturn: 0,
    );
  }
  int code;
  try {
    if (run3 != null) {
      code = run3(framesV3Ptr, n, ptsPtr, np, opts, cbPtr,
          chunkCb == null ? nullptr : chunkCb.nativeFunction, nullptr, stats);
    } else if (chunkCb != null) {
      code = run2!(framesPtr, n, ptsPtr, np, opts, cbPtr, chunkCb.nativeFunction, nullptr, stats);
    } else {
      code = ffi._run(framesPtr, n, ptsPtr, np, opts, cbPtr, nullptr, stats);
    }
  } finally {
    cb?.close();
    chunkCb?.close();
  }
  final st = _readStats(stats.ref);
  for (final s in strings) {
    calloc.free(s);
  }
  if (framesPtr != nullptr) calloc.free(framesPtr);
  if (framesV3Ptr != nullptr) calloc.free(framesV3Ptr);
  calloc.free(ptsPtr);
  calloc.free(workDirC);
  calloc.free(outPlyC);
  calloc.free(opts);
  calloc.free(stats);
  return PwDenseResult(code, st);
}

                                                                            
void _fillFrameV2(_FrameRaw r, PwDenseFrame f, List<Pointer<Utf8>> strings) {
  r.frameId = f.frameId;
  r.fx = f.fx;
  r.fy = f.fy;
  r.cx = f.cx;
  r.cy = f.cy;
  r.imageW = f.imageW;
  r.imageH = f.imageH;
  for (var k = 0; k < 4; k++) {
    r.qWxyz[k] = f.qWxyz[k];
  }
  for (var k = 0; k < 3; k++) {
    r.t[k] = f.t[k];
  }
  final s = f.jpegPath.toNativeUtf8();
  strings.add(s);
  r.jpegPath = s;
}

                                                                                                                 
                                                                                                                
                                                                    
void _fillFrameV3(_FrameRawV3 r, PwDenseFrame f, List<Pointer<Utf8>> strings) {
  r.frameId = f.frameId;
  r.fx = f.fx;
  r.fy = f.fy;
  r.cx = f.cx;
  r.cy = f.cy;
  r.imageW = f.imageW;
  r.imageH = f.imageH;
  for (var k = 0; k < 4; k++) {
    r.qWxyz[k] = f.qWxyz[k];
  }
  for (var k = 0; k < 3; k++) {
    r.t[k] = f.t[k];
  }
  if (f.jpegPath.isEmpty) {
    r.jpegPath = nullptr;
  } else {
    final s = f.jpegPath.toNativeUtf8();
    strings.add(s);
    r.jpegPath = s;
  }
  final nv = f.nv12Path;
  if (nv == null || nv.isEmpty) {
    r.nv12Path = nullptr;
    r.nv12Width = 0;
    r.nv12Height = 0;
    r.nv12Matrix = 0;
  } else {
    final s = nv.toNativeUtf8();
    strings.add(s);
    r.nv12Path = s;
    r.nv12Width = f.nv12Width ?? 0;
    r.nv12Height = f.nv12Height ?? 0;
    r.nv12Matrix = kPwvaNv12Matrix;
  }
  r.reserved0 = 0;
}

                                                                                                                
                                                                                                                
                                                                                                             
({int structSize, Uint8List bytes, String? jpegPath, String? nv12Path}) debugMarshalPwDenseFrameV3(PwDenseFrame f) {
  final p = calloc<_FrameRawV3>();
  final strings = <Pointer<Utf8>>[];
  try {
    _fillFrameV3(p.ref, f, strings);
    final size = sizeOf<_FrameRawV3>();
    final bytes = Uint8List.fromList(p.cast<Uint8>().asTypedList(size));
    final jp = p.ref.jpegPath;
    final np = p.ref.nv12Path;
    return (
      structSize: size,
      bytes: bytes,
      jpegPath: jp == nullptr ? null : jp.toDartString(),
      nv12Path: np == nullptr ? null : np.toDartString(),
    );
  } finally {
    for (final s in strings) {
      calloc.free(s);
    }
    calloc.free(p);
  }
}

PwDenseStats _emptyStats(String error) => PwDenseStats(
      frames: 0, inferred: 0, images: 0, framesSelected: 0, boxFallback: false, sessionMs: 0, imagesMs: 0, ortSessionMs: 0, inferMsMedian: 0,
      inferMsTotal: 0, fuseMs: 0, points: 0, finalFrac: 0, error: error);

PwDenseStats _readStats(_StatsRaw r) {
  final bytes = <int>[];
  for (var i = 0; i < 256; i++) {
    final c = r.error[i];
    if (c == 0) break;
    bytes.add(c & 0xff);
  }
  return PwDenseStats(
    frames: r.frames,
    inferred: r.inferred,
    images: r.images,
    framesSelected: r.framesSelected,
    boxFallback: r.boxFallback != 0,
    sessionMs: r.sessionMs,
    imagesMs: r.imagesMs,
    ortSessionMs: r.ortSessionMs,
    inferMsMedian: r.inferMsMedian,
    inferMsTotal: r.inferMsTotal,
    fuseMs: r.fuseMs,
    points: r.points,
    finalFrac: r.finalFrac,
    error: String.fromCharCodes(bytes),
  );
}
