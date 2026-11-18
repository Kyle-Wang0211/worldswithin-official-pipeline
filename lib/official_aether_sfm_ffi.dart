                                                                     
                                       
  
                                                                        
                                                                               
                                                                            
                                                                               
                                                                            
                                                                           
                                                             
  
                                                                        
                                                                     
                                                      

import 'dart:convert';
import 'dart:ffi';
import 'dart:io' show File, Platform;
import 'dart:typed_data';
import 'package:ffi/ffi.dart';

import 'official_aether_ffi.dart'
    show OfficialAetherFfi, OfficialFfiResolutionError;

                                                                 
                                       
                                                              
                                                        
                                    
class AetherProcessEnv {
  static final _setenv = DynamicLibrary.process()
      .lookupFunction<
        Int32 Function(Pointer<Utf8>, Pointer<Utf8>, Int32),
        int Function(Pointer<Utf8>, Pointer<Utf8>, int)
      >('setenv');
  static final _unsetenv = DynamicLibrary.process()
      .lookupFunction<
        Int32 Function(Pointer<Utf8>),
        int Function(Pointer<Utf8>)
      >('unsetenv');

  static void set(String name, String value) {
    final n = name.toNativeUtf8();
    final v = value.toNativeUtf8();
    try {
      _setenv(n, v, 1);
    } finally {
      malloc.free(n);
      malloc.free(v);
    }
  }

  static void unset(String name) {
    final n = name.toNativeUtf8();
    try {
      _unsetenv(n);
    } finally {
      malloc.free(n);
    }
  }
}

                                          
                                                                   
                                         
                                                            
                                               
                                               
             
class AetherEnvFile {
                                                            
                                                             
                                                          
                                        
  static final Map<String, String> applied = <String, String>{};

                               
  static int intOf(String key, int fallback) {
    final v = applied[key];
    if (v == null || v.isEmpty) return fallback;
    return int.tryParse(v) ?? fallback;
  }

  static Future<List<String>> applyFrom(String documentsPath) async {
    final applied = <String>[];
    try {
      final f = File('$documentsPath/official_env.json');
      if (!await f.exists()) return applied;
      final raw = jsonDecode(await f.readAsString());
      if (raw is! Map<String, dynamic>) return applied;
      for (final e in raw.entries) {
        if (!e.key.startsWith('OFFICIAL_')) continue;
        final v = e.value;
        if (v is! String) continue;
        if (v.isEmpty) {
          AetherProcessEnv.unset(e.key);
          AetherEnvFile.applied.remove(e.key);                       
          applied.add('${e.key}=<unset>');
        } else {
          AetherProcessEnv.set(e.key, v);
          AetherEnvFile.applied[e.key] = v;                          
          applied.add('${e.key}=$v');
        }
      }
    } catch (_) {}
    return applied;
  }
}

                                                           
                                                          
                                         
class AetherMatchFlags {
  static void Function(int)? _setActive;
  static void Function(int)? _setFps30;
  static bool _resolved = false;
  static void _resolve() {
    if (_resolved) return;
    _resolved = true;
    try {
      final lib = OfficialAetherFfi.resolveLibraryForBindings();
      _setActive = lib.lookupFunction<Void Function(Int32), void Function(int)>(
        'pwofficial_match_set_capture_active',
      );
      _setFps30 = lib.lookupFunction<Void Function(Int32), void Function(int)>(
        'pwofficial_match_set_preview_fps30',
      );
    } catch (_) {}
  }

  static void setCaptureActive(bool active) {
    _resolve();
    _setActive?.call(active ? 1 : 0);
  }

  static void setPreviewFps30(bool on) {
    _resolve();
    _setFps30?.call(on ? 1 : 0);
  }
}

                                                                      
enum AetherSfmResult {
  ok,
  errInvalidArg,
  errDb,
  errExtract,
  errNoInitialPair,
  errNotRegistered,
  errInternal,
  errUnsupported,
}

                                                                           
enum AetherSfmFinalizeStatus {
  idle,                                  
  localReady,                                                          
  refined,                                                           
  error,                                    
}

AetherSfmFinalizeStatus aetherSfmFinalizeStatusFromCode(int code) {
  if (code < 0 || code >= AetherSfmFinalizeStatus.values.length) {
    return AetherSfmFinalizeStatus.error;
  }
  return AetherSfmFinalizeStatus.values[code];
}

AetherSfmResult _resultFromCode(int code) {
  if (code < 0 || code >= AetherSfmResult.values.length) {
    return AetherSfmResult.errInternal;
  }
  return AetherSfmResult.values[code];
}

                                                             
class AetherSfmPose {
  final int frameId;
  final bool registered;

                                                    
  final List<double> quatWxyz;

                                         
  final List<double> translation;

  const AetherSfmPose({
    required this.frameId,
    required this.registered,
    required this.quatWxyz,
    required this.translation,
  });
}

                                        
class AetherSfmPoint {
  final double x, y, z;
  final int r, g, b;
  const AetherSfmPoint(this.x, this.y, this.z, this.r, this.g, this.b);
}

                                                                       
final class _SfmOptions extends Struct {
  @Int32()
  external int maxFeatures;
  @Int32()
  external int imageWidth;
  @Int32()
  external int imageHeight;
  @Float()
  external double matchMaxRatio;
  @Int32()
  external int useGpuMatch;
  @Int32()
  external int kNeighbors;
                                                                       
                                                                            
                                                                     
                                                                          
  @Int32()
  external int useGpuExtract;
}

final class _SfmPose extends Struct {
  @Int32()
  external int frameId;
  @Int32()
  external int registered;
  @Array(4)
  external Array<Double> qwxyz;
  @Array(3)
  external Array<Double> t;
}

final class _SfmPoint extends Struct {
  @Float()
  external double x;
  @Float()
  external double y;
  @Float()
  external double z;
  @Uint8()
  external int r;
  @Uint8()
  external int g;
  @Uint8()
  external int b;
  @Uint8()
  external int pad0;
  @Uint8()
  external int pad1;
}

typedef _OptionsDefaultC = Void Function(Pointer<_SfmOptions>);
typedef _OptionsDefaultDart = void Function(Pointer<_SfmOptions>);

typedef _RunC =
    Int32 Function(
      Pointer<Utf8> dbPath,
      Pointer<Utf8> imagePath,
      Pointer<_SfmOptions> options,
      Pointer<Pointer<Void>> outSession,
      Pointer<Utf8> outJson,
      Int32 outCap,
    );
typedef _RunDart =
    int Function(
      Pointer<Utf8> dbPath,
      Pointer<Utf8> imagePath,
      Pointer<_SfmOptions> options,
      Pointer<Pointer<Void>> outSession,
      Pointer<Utf8> outJson,
      int outCap,
    );

typedef _GetPosesC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<_SfmPose> outPoses,
      Int32 cap,
      Pointer<Int32> outCount,
    );
typedef _GetPosesDart =
    int Function(
      Pointer<Void> session,
      Pointer<_SfmPose> outPoses,
      int cap,
      Pointer<Int32> outCount,
    );

typedef _GetPreviewPointsC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Float> outXyz,
      Int32 cap,
      Pointer<Int32> outCount,
    );
typedef _GetPreviewPointsDart =
    int Function(
      Pointer<Void> session,
      Pointer<Float> outXyz,
      int cap,
      Pointer<Int32> outCount,
    );

typedef _GetPointsC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Pointer<_SfmPoint>> outPoints,
      Pointer<Int32> outCount,
    );
typedef _GetPointsDart =
    int Function(
      Pointer<Void> session,
      Pointer<Pointer<_SfmPoint>> outPoints,
      Pointer<Int32> outCount,
    );

typedef _PointsFreeC = Void Function(Pointer<_SfmPoint>);
typedef _PointsFreeDart = void Function(Pointer<_SfmPoint>);

                                                                          
                                                                            
final class _SfmTrackObs extends Struct {
  @Int32()
  external int frameId;
  @Float()
  external double x;
  @Float()
  external double y;
}

typedef _GetPointsTrackedC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Pointer<_SfmPoint>> outPoints,
      Pointer<Int32> outCount,
      Pointer<Pointer<Int32>> outObsOffsets,
      Pointer<Pointer<_SfmTrackObs>> outObs,
      Pointer<Int64> outObsCount,
    );
typedef _GetPointsTrackedDart =
    int Function(
      Pointer<Void> session,
      Pointer<Pointer<_SfmPoint>> outPoints,
      Pointer<Int32> outCount,
      Pointer<Pointer<Int32>> outObsOffsets,
      Pointer<Pointer<_SfmTrackObs>> outObs,
      Pointer<Int64> outObsCount,
    );

typedef _TrackObsFreeC =
    Void Function(Pointer<Int32> offsets, Pointer<_SfmTrackObs> obs);
typedef _TrackObsFreeDart =
    void Function(Pointer<Int32> offsets, Pointer<_SfmTrackObs> obs);

typedef _DebugLastC =
    Void Function(
      Pointer<Void>,
      Pointer<Double>,
      Pointer<Double>,
      Pointer<Int32>,
      Pointer<Int32>,
      Pointer<Int32>,
    );
typedef _DebugLastDart =
    void Function(
      Pointer<Void>,
      Pointer<Double>,
      Pointer<Double>,
      Pointer<Int32>,
      Pointer<Int32>,
      Pointer<Int32>,
    );

typedef _CandidateStatsC =
    Void Function(Pointer<Void>, Pointer<Int64>, Pointer<Int64>);
typedef _CandidateStatsDart =
    void Function(Pointer<Void>, Pointer<Int64>, Pointer<Int64>);

                                                                               
                                                                          
                                                                          
typedef _MatchFailStatsC =
    Void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );
typedef _MatchFailStatsDart =
    void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );

                                                                             
                                                                             
                                                                 
typedef _SetThermalStateC = Void Function(Pointer<Void>, Int32);
typedef _SetThermalStateDart = void Function(Pointer<Void>, int);

                                                                           
                                                        
typedef _ThermalThrottleStatsC = Void Function(Pointer<Void>, Pointer<Int64>);
typedef _ThermalThrottleStatsDart =
    void Function(Pointer<Void>, Pointer<Int64>);

                                                                       
                                                                       
                                                                      
                                                                       
                                                           
typedef _LiveRepayC = Int32 Function(Pointer<Void>, Int32);
typedef _LiveRepayDart = int Function(Pointer<Void>, int);

                                                                     
                                                                             
typedef _RepairStatsC =
    Void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );
typedef _RepairStatsDart =
    void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );

typedef _StreamStatsC =
    Void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );
typedef _StreamStatsDart =
    void Function(
      Pointer<Void>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
      Pointer<Int64>,
    );

typedef _SessionFreeC = Void Function(Pointer<Void>);
typedef _SessionFreeDart = void Function(Pointer<Void>);

                                                                             
typedef _CreateC =
    Int32 Function(
      Pointer<Utf8> dbPath,
      Pointer<_SfmOptions> options,
      Pointer<Pointer<Void>> outSession,
    );
typedef _CreateDart =
    int Function(
      Pointer<Utf8> dbPath,
      Pointer<_SfmOptions> options,
      Pointer<Pointer<Void>> outSession,
    );

typedef _AddFrameC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Uint8> gray,
      Int32 width,
      Int32 height,
      Float fx,
      Float fy,
      Float cx,
      Float cy,
      Pointer<Double> poseQwxyz,                  
      Pointer<Double> poseT,                  
      Pointer<Int32> outFrameId,
    );
typedef _AddFrameDart =
    int Function(
      Pointer<Void> session,
      Pointer<Uint8> gray,
      int width,
      int height,
      double fx,
      double fy,
      double cx,
      double cy,
      Pointer<Double> poseQwxyz,
      Pointer<Double> poseT,
      Pointer<Int32> outFrameId,
    );

typedef _AddJpegFrameC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Utf8> jpegPath,
      Double captureTimestamp,
      Float fx,
      Float fy,
      Float cx,
      Float cy,
      Pointer<Double> poseQwxyz,                  
      Pointer<Double> poseT,                  
      Pointer<Int32> outFrameId,
    );
typedef _AddJpegFrameDart =
    int Function(
      Pointer<Void> session,
      Pointer<Utf8> jpegPath,
      double captureTimestamp,
      double fx,
      double fy,
      double cx,
      double cy,
      Pointer<Double> poseQwxyz,
      Pointer<Double> poseT,
      Pointer<Int32> outFrameId,
    );

                                                        
                                                                
typedef _PrefetchJpegFrameC =
    Int32 Function(Pointer<Void> session, Pointer<Utf8> jpegPath);
typedef _PrefetchJpegFrameDart =
    int Function(Pointer<Void> session, Pointer<Utf8> jpegPath);

typedef _FinalizeAsyncC =
    Int32 Function(Pointer<Void> session, Pointer<Utf8> outJson, Int32 outCap);
typedef _FinalizeAsyncDart =
    int Function(Pointer<Void> session, Pointer<Utf8> outJson, int outCap);

                                             
                                                      
                                                                          
typedef _RemoveFrameC =
    Int32 Function(
      Pointer<Void> session,
      Int32 frameId,
      Pointer<Utf8> outJson,
      Int32 outCap,
    );
typedef _RemoveFrameDart =
    int Function(
      Pointer<Void> session,
      int frameId,
      Pointer<Utf8> outJson,
      int outCap,
    );

typedef _FinalizeStatusC = Int32 Function(Pointer<Void> session);
typedef _FinalizeStatusDart = int Function(Pointer<Void> session);

                                                                            
                                                           
                                                              
                                                        
                                                      
                                            
                               
typedef _FinalizeProgressC =
    Int32 Function(
      Pointer<Void> session,
      Pointer<Int32> stage,
      Pointer<Int32> round,
      Pointer<Int32> iter,
      Pointer<Int32> maxIter,
    );
typedef _FinalizeProgressDart =
    int Function(
      Pointer<Void> session,
      Pointer<Int32> stage,
      Pointer<Int32> round,
      Pointer<Int32> iter,
      Pointer<Int32> maxIter,
    );

typedef _GlobalRefineC = Int32 Function(Pointer<Void> session);
typedef _GlobalRefineDart = int Function(Pointer<Void> session);

                                                                              
                                                          
class AetherSfmSolve {
  final Map<String, dynamic> summary;
  final Pointer<Void> _session;
  bool _disposed = false;

  AetherSfmSolve._(this.summary, this._session);

                                                                        
  List<AetherSfmPose> poses() {
    _checkLive();
    final countPtr = malloc<Int32>();
    try {
                                
      AetherSfm._getPoses(_session, nullptr, 0, countPtr);
      final n = countPtr.value;
      if (n <= 0) return const [];
      final buf = malloc<_SfmPose>(n);
      try {
        final rc = AetherSfm._getPoses(_session, buf, n, countPtr);
        if (_resultFromCode(rc) != AetherSfmResult.ok) return const [];
        final out = <AetherSfmPose>[];
        final written = countPtr.value < n ? countPtr.value : n;
        for (var i = 0; i < written; i++) {
          final p = buf[i];
          out.add(
            AetherSfmPose(
              frameId: p.frameId,
              registered: p.registered != 0,
              quatWxyz: [p.qwxyz[0], p.qwxyz[1], p.qwxyz[2], p.qwxyz[3]],
              translation: [p.t[0], p.t[1], p.t[2]],
            ),
          );
        }
        return out;
      } finally {
        malloc.free(buf);
      }
    } finally {
      malloc.free(countPtr);
    }
  }

                                                                    
  List<AetherSfmPoint> points() {
    _checkLive();
    final countPtr = malloc<Int32>();
    final outPtr = malloc<Pointer<_SfmPoint>>();
    try {
      final rc = AetherSfm._getPoints(_session, outPtr, countPtr);
      if (_resultFromCode(rc) != AetherSfmResult.ok) return const [];
      final n = countPtr.value;
      final arr = outPtr.value;
      if (n <= 0 || arr == nullptr) return const [];
      try {
        final out = <AetherSfmPoint>[];
        for (var i = 0; i < n; i++) {
          final p = arr[i];
          out.add(AetherSfmPoint(p.x, p.y, p.z, p.r, p.g, p.b));
        }
        return out;
      } finally {
        AetherSfm._pointsFree(arr);                           
      }
    } finally {
      malloc.free(countPtr);
      malloc.free(outPtr);
    }
  }

  void dispose() {
    if (_disposed) return;
    _disposed = true;
    if (_session != nullptr) AetherSfm._sessionFree(_session);
  }

  void _checkLive() {
    if (_disposed) {
      throw StateError('AetherSfmSolve already disposed');
    }
  }
}

                                                                  
class AetherSfm {
  AetherSfm._();

                                                                         
                                                                           
                                                              
  static bool get isSupported {
    if (!Platform.isIOS) {
                                                                             
                                                          
      return true;
    }
                                                                               
                                                                          
    final env = Platform.environment;
    final isSim =
        env.containsKey('SIMULATOR_DEVICE_NAME') ||
        env.containsKey('SIMULATOR_UDID');
    return !isSim;
  }

  static DynamicLibrary get _lib =>
      OfficialAetherFfi.resolveLibraryForBindings();

                                                                                
                                                                      
                                                                          
                                               
                                                                                
                                     
  static final _OptionsDefaultDart _optionsDefault = _lib
      .lookupFunction<_OptionsDefaultC, _OptionsDefaultDart>(
        'pwofficial_options_default',
      );
  static final _RunDart _run = _lib.lookupFunction<_RunC, _RunDart>(
    'pwofficial_run',
  );
  static final _GetPosesDart _getPoses = _lib
      .lookupFunction<_GetPosesC, _GetPosesDart>('pwofficial_get_poses');
  static final _GetPreviewPointsDart _getPreviewPoints = _lib
      .lookupFunction<_GetPreviewPointsC, _GetPreviewPointsDart>(
        'pwofficial_get_preview_points',
      );
  static final _GetPointsDart _getPoints = _lib
      .lookupFunction<_GetPointsC, _GetPointsDart>('pwofficial_get_points');
  static final _PointsFreeDart _pointsFree = _lib
      .lookupFunction<_PointsFreeC, _PointsFreeDart>('pwofficial_points_free');
  static final _GetPointsTrackedDart _getPointsTracked = _lib
      .lookupFunction<_GetPointsTrackedC, _GetPointsTrackedDart>(
        'pwofficial_get_points_tracked',
      );
                                                                         
                                                                                
                                                                             
  static final _GetPointsTrackedDart _getPreviewTracked = _lib
      .lookupFunction<_GetPointsTrackedC, _GetPointsTrackedDart>(
        'pwofficial_get_preview_tracked',
      );
  static final _TrackObsFreeDart _trackObsFree = _lib
      .lookupFunction<_TrackObsFreeC, _TrackObsFreeDart>(
        'pwofficial_track_obs_free',
      );
  static final _DebugLastDart _debugLast = _lib
      .lookupFunction<_DebugLastC, _DebugLastDart>('pwofficial_debug_last');
  static final _StreamStatsDart _streamStats = _lib
      .lookupFunction<_StreamStatsC, _StreamStatsDart>(
        'pwofficial_stream_stats',
      );
  static final _CandidateStatsDart _candidateStats = _lib
      .lookupFunction<_CandidateStatsC, _CandidateStatsDart>(
        'pwofficial_candidate_stats',
      );
  static final _MatchFailStatsDart _matchFailStats = _lib
      .lookupFunction<_MatchFailStatsC, _MatchFailStatsDart>(
        'pwofficial_match_fail_stats',
      );
  static final _SetThermalStateDart _setThermalState = _lib
      .lookupFunction<_SetThermalStateC, _SetThermalStateDart>(
        'pwofficial_set_thermal_state',
      );
  static final _ThermalThrottleStatsDart _thermalThrottleStats = _lib
      .lookupFunction<_ThermalThrottleStatsC, _ThermalThrottleStatsDart>(
        'pwofficial_thermal_throttle_stats',
      );
  static final _LiveRepayDart _liveRepay = _lib
      .lookupFunction<_LiveRepayC, _LiveRepayDart>('pwofficial_live_repay');
  static final _RepairStatsDart _repairStats = _lib
      .lookupFunction<_RepairStatsC, _RepairStatsDart>(
        'pwofficial_repair_stats',
      );
  static final _GlobalRefineDart _globalRefine = _lib
      .lookupFunction<_GlobalRefineC, _GlobalRefineDart>(
        'pwofficial_global_refine',
      );
  static final _SessionFreeDart _sessionFree = _lib
      .lookupFunction<_SessionFreeC, _SessionFreeDart>('pwofficial_free');

                                                                           
                                                                            
                                           
  static final _CreateDart _create = _lib.lookupFunction<_CreateC, _CreateDart>(
    'pwofficial_create',
  );
  static final _AddFrameDart _addFrame = _lib
      .lookupFunction<_AddFrameC, _AddFrameDart>('pwofficial_add_frame');
  static final _AddJpegFrameDart _addJpegFrame = _lib
      .lookupFunction<_AddJpegFrameC, _AddJpegFrameDart>(
        'pwofficial_add_jpeg_frame',
      );
                                                       
                               
  static final _PrefetchJpegFrameDart _prefetchJpegFrame = _lib
      .lookupFunction<_PrefetchJpegFrameC, _PrefetchJpegFrameDart>(
        'pwofficial_prefetch_jpeg_frame',
      );
  static final _FinalizeAsyncDart _finalizeAsync = _lib
      .lookupFunction<_FinalizeAsyncC, _FinalizeAsyncDart>(
        'pwofficial_finalize_async',
      );
  static final _RemoveFrameDart _removeFrame = _lib
      .lookupFunction<_RemoveFrameC, _RemoveFrameDart>(
        'pwofficial_remove_frame',
      );
  static final _FinalizeStatusDart _finalizeStatus = _lib
      .lookupFunction<_FinalizeStatusC, _FinalizeStatusDart>(
        'pwofficial_finalize_status',
      );
                                                         
                                            
                                                                 
                                            
                                                         
                         
  static final _FinalizeProgressDart? _finalizeProgress =
      _lookupFinalizeProgress();

  static _FinalizeProgressDart? _lookupFinalizeProgress() {
    try {
      return _lib.lookupFunction<_FinalizeProgressC, _FinalizeProgressDart>(
        'pwofficial_finalize_progress',
      );
    } catch (_) {
                                                    
                             
      return null;
    }
  }

                                                                             
                                                                      
                              
                                                                               
                                                                                
                                                
     
                                                                             
                          
                                                                            
  static AetherSfmSolve run(
    String dbPath,
    String imagePath, {
    int maxFeatures = 2048,
    double matchMaxRatio = 0.8,
    bool useGpuMatch = false,
    int kNeighbors = 6,
  }) {
    if (!isSupported) {
      throw UnsupportedError(
        'On-device SfM is unavailable on the iOS simulator (arm64 device '
        'only). Run on a physical device.',
      );
    }
    final dbPtr = dbPath.toNativeUtf8();
    final imgPtr = imagePath.toNativeUtf8();
    final optPtr = malloc<_SfmOptions>();
    final sessPtr = malloc<Pointer<Void>>();
    const cap = 4096;
    final jsonPtr = malloc.allocate<Uint8>(cap).cast<Utf8>();
    try {
      _optionsDefault(optPtr);
      optPtr.ref
        ..maxFeatures = maxFeatures
        ..matchMaxRatio = matchMaxRatio
        ..useGpuMatch = useGpuMatch ? 1 : 0
        ..kNeighbors = kNeighbors;
      sessPtr.value = nullptr;
      final rc = _run(dbPtr, imgPtr, optPtr, sessPtr, jsonPtr, cap);
      final json = jsonPtr.toDartString();
      Map<String, dynamic> summary;
      try {
        summary = jsonDecode(json) as Map<String, dynamic>;
      } catch (_) {
        summary = {'raw': json};
      }
      summary['rc'] = rc;
      final result = _resultFromCode(rc);
      summary['result'] = result.name;
      return AetherSfmSolve._(summary, sessPtr.value);
    } finally {
      malloc.free(dbPtr);
      malloc.free(imgPtr);
      malloc.free(optPtr);
      malloc.free(sessPtr);
      malloc.free(jsonPtr);
    }
  }
}

                                                          
class AetherSfmAddFrameResult {
  final AetherSfmResult result;
  final int frameId;                          
  const AetherSfmAddFrameResult(this.result, this.frameId);
}

                                                               
                                                                            
                                                                      
class AetherSfmPointsPacked {
  final Float32List xyz;
  final Uint8List rgb;
  const AetherSfmPointsPacked(this.xyz, this.rgb);
  int get count => xyz.length ~/ 3;
}

                                                                            
                                                                            
                                                                  
   
                                                                         
                                                                             
                                                                            
                                                                         
                                                                             
                                    
class AetherSfmPointsTracked {
  final Float32List xyz;
  final Uint8List rgb;
  final Int32List obsOffsets;                   
  final Int32List obsFrameIds;                    
  final Float32List obsXY;                      
  const AetherSfmPointsTracked(
    this.xyz,
    this.rgb,
    this.obsOffsets,
    this.obsFrameIds,
    this.obsXY,
  );
  int get count => xyz.length ~/ 3;
  int get obsCount => obsFrameIds.length;
}

                                                                     
                                                            
   
                                                                     
                                                                             
                                                                            
                                                                           
                                                                             
                                           
class AetherSfmStreamSession {
  final Pointer<Void> _session;
  bool _disposed = false;

  AetherSfmStreamSession._(this._session);

                                                                           
                                                                              
                                                                         
                                                   
     
                                                                         
                                                                        
                                 
                                                                          
                                                                          
                                                                               
                                                                          
                                                                          
                                                          
                                              
                                                                
                                                   
                                        
  static const int researchMaxFeatures = 13312;
  static const int researchKNeighbors = 12;
  static const int liveMaxFeatures = 2048;
  static const int liveKNeighbors = 6;
  static const double defaultMatchMaxRatio = 0.8;

  static AetherSfmStreamSession create(
    String dbPath, {
    required int imageWidth,
    required int imageHeight,
    int maxFeatures = researchMaxFeatures,
    int kNeighbors = researchKNeighbors,
  }) {
    if (!AetherSfm.isSupported) {
      throw UnsupportedError(
        'On-device SfM is unavailable on the iOS simulator (arm64 device '
        'only). Run on a physical device.',
      );
    }
    final dbPtr = dbPath.toNativeUtf8();
    final optPtr = malloc<_SfmOptions>();
    final sessPtr = malloc<Pointer<Void>>();
    try {
      AetherSfm._optionsDefault(optPtr);
      optPtr.ref
        ..maxFeatures = maxFeatures
        ..imageWidth = imageWidth
        ..imageHeight = imageHeight
        ..matchMaxRatio = defaultMatchMaxRatio
        ..kNeighbors = kNeighbors
        ..useGpuMatch = 1
        ..useGpuExtract = 1;
      sessPtr.value = nullptr;
      final rc = AetherSfm._create(dbPtr, optPtr, sessPtr);
      final result = _resultFromCode(rc);
      if (result != AetherSfmResult.ok || sessPtr.value == nullptr) {
        throw StateError('aether_sfm_create failed: ${result.name} (rc=$rc)');
      }
      return AetherSfmStreamSession._(sessPtr.value);
    } finally {
      malloc.free(dbPtr);
      malloc.free(optPtr);
      malloc.free(sessPtr);
    }
  }

                                                                         
                                                                             
                                                                      
                                                                               
                                                                              
                                                            
     
                                                                              
                                                                  
  AetherSfmAddFrameResult addFrame(
    Uint8List gray,
    int width,
    int height, {
    required double fx,
    required double fy,
    required double cx,
    required double cy,
    List<double>? quatWxyz,
    List<double>? translation,
  }) {
    _checkLive();
    final n = width * height;
    final grayPtr = malloc<Uint8>(n);
    final idPtr = malloc<Int32>();
    Pointer<Double> qPtr = nullptr;
    Pointer<Double> tPtr = nullptr;
    try {
      grayPtr.asTypedList(n).setRange(0, n, gray);
      if (quatWxyz != null && quatWxyz.length == 4) {
        qPtr = malloc<Double>(4);
        qPtr.asTypedList(4).setAll(0, quatWxyz);
      }
      if (translation != null && translation.length == 3) {
        tPtr = malloc<Double>(3);
        tPtr.asTypedList(3).setAll(0, translation);
      }
      idPtr.value = -1;
      final rc = AetherSfm._addFrame(
        _session,
        grayPtr,
        width,
        height,
        fx,
        fy,
        cx,
        cy,
        qPtr,
        tPtr,
        idPtr,
      );
      final result = _resultFromCode(rc);
      return AetherSfmAddFrameResult(
        result,
        result == AetherSfmResult.ok ? idPtr.value : -1,
      );
    } finally {
      malloc.free(grayPtr);
      malloc.free(idPtr);
      if (qPtr != nullptr) malloc.free(qPtr);
      if (tPtr != nullptr) malloc.free(tPtr);
    }
  }

                                                                             
                                                                              
                                                                          
                                                            
                                                 
                                                 
  int prefetchJpegFrame(String jpegPath) {
    _checkLive();
    final pathPtr = jpegPath.toNativeUtf8();
    try {
      return AetherSfm._prefetchJpegFrame(_session, pathPtr);
    } finally {
      malloc.free(pathPtr);
    }
  }

  AetherSfmAddFrameResult addJpegFrame(
    String jpegPath, {
    required double captureTimestamp,
    required double fx,
    required double fy,
    required double cx,
    required double cy,
    List<double>? quatWxyz,
    List<double>? translation,
  }) {
    _checkLive();
    final pathPtr = jpegPath.toNativeUtf8();
    final idPtr = malloc<Int32>();
    Pointer<Double> qPtr = nullptr;
    Pointer<Double> tPtr = nullptr;
    try {
      if (quatWxyz != null && quatWxyz.length == 4) {
        qPtr = malloc<Double>(4);
        qPtr.asTypedList(4).setAll(0, quatWxyz);
      }
      if (translation != null && translation.length == 3) {
        tPtr = malloc<Double>(3);
        tPtr.asTypedList(3).setAll(0, translation);
      }
      idPtr.value = -1;
      final rc = AetherSfm._addJpegFrame(
        _session,
        pathPtr,
        captureTimestamp,
        fx,
        fy,
        cx,
        cy,
        qPtr,
        tPtr,
        idPtr,
      );
      final result = _resultFromCode(rc);
      return AetherSfmAddFrameResult(
        result,
        result == AetherSfmResult.ok ? idPtr.value : -1,
      );
    } finally {
      malloc.free(pathPtr);
      malloc.free(idPtr);
      if (qPtr != nullptr) malloc.free(qPtr);
      if (tPtr != nullptr) malloc.free(tPtr);
    }
  }

                                                                           
                                                                             
                                                                       
                                                               
  ({
    double extractMs,
    double matchMs,
    int nCand,
    int gpuMatches,
    int cpuMatches,
  })
  debugLast() {
    _checkLive();
    final e = malloc<Double>(), m = malloc<Double>();
    final nc = malloc<Int32>(), gm = malloc<Int32>(), cm = malloc<Int32>();
    try {
      e.value = 0;
      m.value = 0;
      nc.value = 0;
      gm.value = 0;
      cm.value = 0;
      AetherSfm._debugLast(_session, e, m, nc, gm, cm);
      return (
        extractMs: e.value,
        matchMs: m.value,
        nCand: nc.value,
        gpuMatches: gm.value,
        cpuMatches: cm.value,
      );
    } finally {
      malloc.free(e);
      malloc.free(m);
      malloc.free(nc);
      malloc.free(gm);
      malloc.free(cm);
    }
  }

                                                                          
                                                                               
                                                                                 
                                                                       
                                                                             
                                                                               
                                                              
  ({
    int tvgPairs,
    int rawPairs,
    int growAccepted,
    int growRejected,
    int reprojFiltered,
    int triFiltered,
    int growRejectCheirality,
    int growRejectReproj,
    int createRejectCheirality,
    int createRejectTriAngle,
    int createRejectReproj,
    int alreadyAssigned,
    int mergeNeeded,
    int mergeAccepted,
    int mergeRejected,
    int spatialConsidered,
    int spatialAttempted,
    int spatialWritten,
    int spatialInliers,
    int spatialAnchorAttempted,
    int spatialAnchorPassed,
    int spatialRegionsConfirmed,
    int spatialExpandedAttempted,
    int spatialGuidedPairs,
    int spatialGuidedInliers,
    int spatialQuadraticAttempted,
    int spatialQuadraticWritten,
    int spatialBudgetSkipped,
    int temporalDetailPairs,
    int temporalDetailMatches,
    int temporalDetailCreated,
    int temporalDetailGrown,
    int temporalDetailRejectCheirality,
    int temporalDetailRejectReproj,
    int temporalDetailRejectTriAngle,
    int temporalDetailConflicts,
  })
  streamStats() {
    _checkLive();
    final tvg = malloc<Int64>(), raw = malloc<Int64>();
    final ga = malloc<Int64>(), gr = malloc<Int64>();
    final rf = malloc<Int64>(), tf = malloc<Int64>();
    final grc = malloc<Int64>(), grr = malloc<Int64>();
    final cch = malloc<Int64>(), cta = malloc<Int64>(), crp = malloc<Int64>();
    final asg = malloc<Int64>(), merge = malloc<Int64>();
    final mergeA = malloc<Int64>(), mergeR = malloc<Int64>();
    final spc = malloc<Int64>(), spa = malloc<Int64>();
    final spw = malloc<Int64>(), spi = malloc<Int64>();
    final saa = malloc<Int64>(), sap = malloc<Int64>();
    final src = malloc<Int64>(), sea = malloc<Int64>();
    final sgp = malloc<Int64>(), sgi = malloc<Int64>();
    final sqa = malloc<Int64>(), sqw = malloc<Int64>();
    final sbs = malloc<Int64>();
    final tdp = malloc<Int64>(), tdm = malloc<Int64>();
    final tdc = malloc<Int64>(), tdg = malloc<Int64>();
    final tdch = malloc<Int64>(), tdrp = malloc<Int64>();
    final tdta = malloc<Int64>(), tdcf = malloc<Int64>();
    try {
      for (final p in [
        tvg,
        raw,
        ga,
        gr,
        rf,
        tf,
        grc,
        grr,
        cch,
        cta,
        crp,
        asg,
        merge,
        mergeA,
        mergeR,
        spc,
        spa,
        spw,
        spi,
        saa,
        sap,
        src,
        sea,
        sgp,
        sgi,
        sqa,
        sqw,
        sbs,
        tdp,
        tdm,
        tdc,
        tdg,
        tdch,
        tdrp,
        tdta,
        tdcf,
      ]) {
        p.value = 0;
      }
      AetherSfm._streamStats(
        _session,
        tvg,
        raw,
        ga,
        gr,
        rf,
        tf,
        grc,
        grr,
        cch,
        cta,
        crp,
        asg,
        merge,
        mergeA,
        mergeR,
        spc,
        spa,
        spw,
        spi,
        saa,
        sap,
        src,
        sea,
        sgp,
        sgi,
        sqa,
        sqw,
        sbs,
        tdp,
        tdm,
        tdc,
        tdg,
        tdch,
        tdrp,
        tdta,
        tdcf,
      );
      return (
        tvgPairs: tvg.value,
        rawPairs: raw.value,
        growAccepted: ga.value,
        growRejected: gr.value,
        reprojFiltered: rf.value,
        triFiltered: tf.value,
        growRejectCheirality: grc.value,
        growRejectReproj: grr.value,
        createRejectCheirality: cch.value,
        createRejectTriAngle: cta.value,
        createRejectReproj: crp.value,
        alreadyAssigned: asg.value,
        mergeNeeded: merge.value,
        mergeAccepted: mergeA.value,
        mergeRejected: mergeR.value,
        spatialConsidered: spc.value,
        spatialAttempted: spa.value,
        spatialWritten: spw.value,
        spatialInliers: spi.value,
        spatialAnchorAttempted: saa.value,
        spatialAnchorPassed: sap.value,
        spatialRegionsConfirmed: src.value,
        spatialExpandedAttempted: sea.value,
        spatialGuidedPairs: sgp.value,
        spatialGuidedInliers: sgi.value,
        spatialQuadraticAttempted: sqa.value,
        spatialQuadraticWritten: sqw.value,
        spatialBudgetSkipped: sbs.value,
        temporalDetailPairs: tdp.value,
        temporalDetailMatches: tdm.value,
        temporalDetailCreated: tdc.value,
        temporalDetailGrown: tdg.value,
        temporalDetailRejectCheirality: tdch.value,
        temporalDetailRejectReproj: tdrp.value,
        temporalDetailRejectTriAngle: tdta.value,
        temporalDetailConflicts: tdcf.value,
      );
    } finally {
      for (final p in [
        tvg,
        raw,
        ga,
        gr,
        rf,
        tf,
        grc,
        grr,
        cch,
        cta,
        crp,
        asg,
        merge,
        mergeA,
        mergeR,
        spc,
        spa,
        spw,
        spi,
        saa,
        sap,
        src,
        sea,
        sgp,
        sgi,
        sqa,
        sqw,
        sbs,
        tdp,
        tdm,
        tdc,
        tdg,
        tdch,
        tdrp,
        tdta,
        tdcf,
      ]) {
        malloc.free(p);
      }
    }
  }


                                                                      
                                                                               
                                                                     
  ({int spatialFirstPairs, int temporalFallbackPairs}) candidateStats() {
    _checkLive();
    final sp = malloc<Int64>(), tf = malloc<Int64>();
    try {
      sp.value = 0;
      tf.value = 0;
      AetherSfm._candidateStats(_session, sp, tf);
      return (spatialFirstPairs: sp.value, temporalFallbackPairs: tf.value);
    } finally {
      malloc.free(sp);
      malloc.free(tf);
    }
  }

                                                                        
                                                                         
                                                                     
                                                                         
                                                                            
                                                                              
                                                               
  ({
    int gpuFailTotal,
    List<int> gpuFailByRc,
    int gpuFailMaxStreak,
    int rematchStarvedFrames,
    int rematchCandidates,
    int rematchAttempted,
    int rematchWritten,
    int rematchInliers,
    int rematchFailed,
  })
  matchFailStats() {
    _checkLive();
    final total = malloc<Int64>();
    final byRc = malloc<Int64>(8);
    final streak = malloc<Int64>();
    final starved = malloc<Int64>();
    final cand = malloc<Int64>();
    final att = malloc<Int64>();
    final wr = malloc<Int64>();
    final inl = malloc<Int64>();
    final fail = malloc<Int64>();
    try {
      total.value = 0;
      for (var i = 0; i < 8; i++) {
        byRc[i] = 0;
      }
      streak.value = 0;
      starved.value = 0;
      cand.value = 0;
      att.value = 0;
      wr.value = 0;
      inl.value = 0;
      fail.value = 0;
      AetherSfm._matchFailStats(
        _session,
        total,
        byRc,
        streak,
        starved,
        cand,
        att,
        wr,
        inl,
        fail,
      );
      return (
        gpuFailTotal: total.value,
        gpuFailByRc: List<int>.generate(8, (i) => byRc[i]),
        gpuFailMaxStreak: streak.value,
        rematchStarvedFrames: starved.value,
        rematchCandidates: cand.value,
        rematchAttempted: att.value,
        rematchWritten: wr.value,
        rematchInliers: inl.value,
        rematchFailed: fail.value,
      );
    } finally {
      malloc.free(total);
      malloc.free(byRc);
      malloc.free(streak);
      malloc.free(starved);
      malloc.free(cand);
      malloc.free(att);
      malloc.free(wr);
      malloc.free(inl);
      malloc.free(fail);
    }
  }

                                                                          
                                                                          
                                                                             
                                                                         
                                                                            
                                                                   
  void setThermalState(int state) {
    _checkLive();
    AetherSfm._setThermalState(_session, state);
  }

                                                                           
                                                           
  int thermalThrottledFrames() {
    _checkLive();
    final p = malloc<Int64>();
    try {
      p.value = 0;
      AetherSfm._thermalThrottleStats(_session, p);
      return p.value;
    } finally {
      malloc.free(p);
    }
  }

                                                                              
                                                                          
                                                                        
                                                                         
                                                                            
                                                                           
                                                                        
                                                                            
                                                                            
                                                                           
                                                                          
                                                                              
                                                                               
                                          
  int liveRepay({int maxPairs = 4}) {
    _checkLive();
    final rc = AetherSfm._liveRepay(_session, maxPairs);
    if (rc < 0) {
      throw StateError('pwofficial_live_repay rejected args (rc=$rc)');
    }
    return rc;
  }

                                                                   
                                                                         
                                                                            
                                                               
  ({
    int repayCalls,
    int repayAttempted,
    int repayWritten,
    int repayInliers,
    int repayFailed,
    int repaySkippedThermal,
    int gpuRetryAttempts,
    int gpuRetryRecovered,
    int enrichBudgetStopped,
  })
  repairStats() {
    _checkLive();
    final calls = malloc<Int64>();
    final att = malloc<Int64>();
    final wr = malloc<Int64>();
    final inl = malloc<Int64>();
    final fail = malloc<Int64>();
    final therm = malloc<Int64>();
    final retryAtt = malloc<Int64>();
    final retryRec = malloc<Int64>();
    final budget = malloc<Int64>();
    try {
      calls.value = 0;
      att.value = 0;
      wr.value = 0;
      inl.value = 0;
      fail.value = 0;
      therm.value = 0;
      retryAtt.value = 0;
      retryRec.value = 0;
      budget.value = 0;
      AetherSfm._repairStats(
        _session,
        calls,
        att,
        wr,
        inl,
        fail,
        therm,
        retryAtt,
        retryRec,
        budget,
      );
      return (
        repayCalls: calls.value,
        repayAttempted: att.value,
        repayWritten: wr.value,
        repayInliers: inl.value,
        repayFailed: fail.value,
        repaySkippedThermal: therm.value,
        gpuRetryAttempts: retryAtt.value,
        gpuRetryRecovered: retryRec.value,
        enrichBudgetStopped: budget.value,
      );
    } finally {
      malloc.free(calls);
      malloc.free(att);
      malloc.free(wr);
      malloc.free(inl);
      malloc.free(fail);
      malloc.free(therm);
      malloc.free(retryAtt);
      malloc.free(retryRec);
      malloc.free(budget);
    }
  }

                                                                        
                                                                       
                                                                             
                                                                           
                                                   
                                                                     
  Map<String, dynamic> finalizeAsync() {
    _checkLive();
    const cap = 4096;
    final jsonPtr = malloc.allocate<Uint8>(cap).cast<Utf8>();
    try {
      final rc = AetherSfm._finalizeAsync(_session, jsonPtr, cap);
      final json = jsonPtr.toDartString();
      Map<String, dynamic> summary;
      try {
        summary = jsonDecode(json) as Map<String, dynamic>;
      } catch (_) {
        summary = {'raw': json};
      }
      summary['rc'] = rc;
      summary['result'] = _resultFromCode(rc).name;
      return summary;
    } finally {
      malloc.free(jsonPtr);
    }
  }

                                                         
  AetherSfmFinalizeStatus finalizeStatus() {
    _checkLive();
    return aetherSfmFinalizeStatusFromCode(AetherSfm._finalizeStatus(_session));
  }

                                                                                
                                                                             
                                     
  bool get hasFinalizeProgress => AetherSfm._finalizeProgress != null;

                                                              
                                                            
                               
     
                                               
                                                            
                            
                                                     
              
  ({int stage, int round, int iter, int maxIter})? finalizeProgress() {
    _checkLive();
    final fn = AetherSfm._finalizeProgress;
    if (fn == null) return null;
    final stage = calloc<Int32>();
    final round = calloc<Int32>();
    final iter = calloc<Int32>();
    final maxIter = calloc<Int32>();
    try {
      final rc = fn(_session, stage, round, iter, maxIter);
      if (rc != 0) return null;
      return (
        stage: stage.value,
        round: round.value,
        iter: iter.value,
        maxIter: maxIter.value,
      );
    } finally {
      calloc.free(stage);
      calloc.free(round);
      calloc.free(iter);
      calloc.free(maxIter);
    }
  }

                                                                             
                                                                              
                                                                              
                                                                       
                                                                         
                                                                             
                                                                          
                                                                            
                                                                      
                                                            
     
                                            
                                          
     
                                    
                                                                     
                                                                
                          
                                                                          
                                                         
                                             
     
                                             
                                                      
                                           
                              
     
                                                         
  Map<String, dynamic>? removeFrame(int frameId) {
    _checkLive();
    const cap = 512;
    final jsonPtr = malloc.allocate<Uint8>(cap).cast<Utf8>();
    try {
      final rc = AetherSfm._removeFrame(_session, frameId, jsonPtr, cap);
      if (_resultFromCode(rc) != AetherSfmResult.ok) return null;
      return jsonDecode(jsonPtr.toDartString()) as Map<String, dynamic>;
    } catch (_) {
      return null;
    } finally {
      malloc.free(jsonPtr);
    }
  }

                                                                               
                                                                           
  /// instant capture-end region selector. Flat [x,y,z, ...] in ARKit world
                                                                          
  Float32List previewPoints() {
    _checkLive();
    final countPtr = malloc<Int32>();
    try {
      AetherSfm._getPreviewPoints(_session, nullptr, 0, countPtr);
      final n = countPtr.value;
      if (n <= 0) return Float32List(0);
      final buf = malloc<Float>(n * 3);
      try {
        final rc = AetherSfm._getPreviewPoints(_session, buf, n, countPtr);
        if (_resultFromCode(rc) != AetherSfmResult.ok) return Float32List(0);
        final written = countPtr.value < n ? countPtr.value : n;
        final out = Float32List(written * 3);
        for (var i = 0; i < written * 3; i++) {
          out[i] = buf[i];
        }
        return out;
      } finally {
        malloc.free(buf);
      }
    } finally {
      malloc.free(countPtr);
    }
  }

                                              
                                                                            
  Float64List posesPacked() {
    _checkLive();
    final countPtr = malloc<Int32>();
    countPtr.value =
        0;                                                              
    try {
      final szrc = AetherSfm._getPoses(_session, nullptr, 0, countPtr);
      final n = countPtr.value;
      if (_resultFromCode(szrc) != AetherSfmResult.ok || n <= 0) {
        return Float64List(0);
      }
      final buf = malloc<_SfmPose>(n);
      try {
        final rc = AetherSfm._getPoses(_session, buf, n, countPtr);
        if (_resultFromCode(rc) != AetherSfmResult.ok) return Float64List(0);
        final written = countPtr.value < n ? countPtr.value : n;
        final out = Float64List(written * 9);
        for (var i = 0; i < written; i++) {
          final p = buf[i];
          final o = i * 9;
          out[o] = p.frameId.toDouble();
          out[o + 1] = p.registered != 0 ? 1 : 0;
          out[o + 2] = p.qwxyz[0];
          out[o + 3] = p.qwxyz[1];
          out[o + 4] = p.qwxyz[2];
          out[o + 5] = p.qwxyz[3];
          out[o + 6] = p.t[0];
          out[o + 7] = p.t[1];
          out[o + 8] = p.t[2];
        }
        return out;
      } finally {
        malloc.free(buf);
      }
    } finally {
      malloc.free(countPtr);
    }
  }

                                                                   
                                                                        
  AetherSfmPointsPacked pointsPacked() {
    _checkLive();
    final countPtr = malloc<Int32>();
    final outPtr = malloc<Pointer<_SfmPoint>>();
    try {
      final rc = AetherSfm._getPoints(_session, outPtr, countPtr);
      if (_resultFromCode(rc) != AetherSfmResult.ok) {
        return AetherSfmPointsPacked(Float32List(0), Uint8List(0));
      }
      final n = countPtr.value;
      final arr = outPtr.value;
      if (n <= 0 || arr == nullptr) {
        return AetherSfmPointsPacked(Float32List(0), Uint8List(0));
      }
      try {
        final xyz = Float32List(n * 3);
        final rgb = Uint8List(n * 3);
        for (var i = 0; i < n; i++) {
          final p = arr[i];
          final o = i * 3;
          xyz[o] = p.x;
          xyz[o + 1] = p.y;
          xyz[o + 2] = p.z;
          rgb[o] = p.r;
          rgb[o + 1] = p.g;
          rgb[o + 2] = p.b;
        }
        return AetherSfmPointsPacked(xyz, rgb);
      } finally {
        AetherSfm._pointsFree(arr);                                       
      }
    } finally {
      malloc.free(countPtr);
      malloc.free(outPtr);
    }
  }

                                                                      
                                                                           
                        
  AetherSfmPointsTracked pointsTracked() =>
      _trackedFrom(AetherSfm._getPointsTracked);

                                                                         
                                                                               
                                                                                
                                                                        
                                                                                
                                                                           
                                                                              
                                                                           
                                                                            
  AetherSfmResult globalRefine() {
    _checkLive();
    return _resultFromCode(AetherSfm._globalRefine(_session));
  }

                                                                         
                                                                        
                                                                              
                                                                            
  AetherSfmPointsTracked previewTracked() =>
      _trackedFrom(AetherSfm._getPreviewTracked);

  AetherSfmPointsTracked _trackedFrom(_GetPointsTrackedDart getter) {
    _checkLive();
    final countPtr = malloc<Int32>();
    final outPtr = malloc<Pointer<_SfmPoint>>();
    final offsPtr = malloc<Pointer<Int32>>();
    final obsPtr = malloc<Pointer<_SfmTrackObs>>();
    final obsCountPtr = malloc<Int64>();
    try {
      final rc = getter(
        _session,
        outPtr,
        countPtr,
        offsPtr,
        obsPtr,
        obsCountPtr,
      );
      if (_resultFromCode(rc) != AetherSfmResult.ok) {
        return AetherSfmPointsTracked(
          Float32List(0),
          Uint8List(0),
          Int32List(1),
          Int32List(0),
          Float32List(0),
        );
      }
      final n = countPtr.value;
      final arr = outPtr.value;
      final offs = offsPtr.value;
      final obs = obsPtr.value;
      final m = obsCountPtr.value;
      if (n <= 0 || arr == nullptr || offs == nullptr) {
        if (arr != nullptr) AetherSfm._pointsFree(arr);
        if (offs != nullptr || obs != nullptr) {
          AetherSfm._trackObsFree(offs, obs);
        }
        return AetherSfmPointsTracked(
          Float32List(0),
          Uint8List(0),
          Int32List(1),
          Int32List(0),
          Float32List(0),
        );
      }
      try {
        final xyz = Float32List(n * 3);
        final rgb = Uint8List(n * 3);
        for (var i = 0; i < n; i++) {
          final p = arr[i];
          final o = i * 3;
          xyz[o] = p.x;
          xyz[o + 1] = p.y;
          xyz[o + 2] = p.z;
          rgb[o] = p.r;
          rgb[o + 1] = p.g;
          rgb[o + 2] = p.b;
        }
        final obsOffsets = Int32List(n + 1);
        obsOffsets.setAll(0, offs.asTypedList(n + 1));
        final obsFrameIds = Int32List(m);
        final obsXY = Float32List(m * 2);
        for (var j = 0; j < m; j++) {
          final t = obs[j];
          obsFrameIds[j] = t.frameId;
          obsXY[j * 2] = t.x;
          obsXY[j * 2 + 1] = t.y;
        }
        return AetherSfmPointsTracked(xyz, rgb, obsOffsets, obsFrameIds, obsXY);
      } finally {
        AetherSfm._pointsFree(arr);                                       
        AetherSfm._trackObsFree(offs, obs);
      }
    } finally {
      malloc.free(countPtr);
      malloc.free(outPtr);
      malloc.free(offsPtr);
      malloc.free(obsPtr);
      malloc.free(obsCountPtr);
    }
  }

                                                                            
                                                                  
  void dispose() {
    if (_disposed) return;
    _disposed = true;
    if (_session != nullptr) AetherSfm._sessionFree(_session);
  }

  void _checkLive() {
    if (_disposed) {
      throw StateError('AetherSfmStreamSession already disposed');
    }
  }
}
