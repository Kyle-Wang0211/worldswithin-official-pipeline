                                                                  
  
                                                                          
                                                                              
                                                                    
                                                                          
                                                                      
                                     
  
            
                                                                
                                                                         
                                                                           
                                                 
                                                                         
                                                                  
                                                                  
  
                                                                           
                                                                              
                                                                            
                                                                                   
                                                                             
                                                                          
                                                                           
                                                                          
                                                     
                                                         
                                   

import 'dart:async';
import 'dart:convert';
import 'dart:ffi' as ffi;
import 'dart:io';
import 'dart:isolate';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:vector_math/vector_math_64.dart' as vm;

import '../official_aether_sfm_ffi.dart';
import '../official_util/device_log.dart';
import '../reconstruction_lease.dart';
import '../point_cloud_data/point_cloud_snapshot.dart';
import 'gravity_align.dart';
import 'live_sfm_publish_policy.dart';
import 'official_highres_reconstruction_input.dart';
import 'photo_archive_coordinator.dart';
import 'photo_archive_runtime.dart';
import 'pw_telemetry.dart';
import 'sfm_feed_queue.dart';
import 'telemetry_writer.dart';
import 'true_parallax.dart';

                                           
                                                              
                                                                 
                                              
                                                    
                                                     
   
                                                  
                                                          
                                               
                                                        
                                                                          
                                                                     
                                                              
                                                      
                                                                  
                                               
                                                    
   
                                                        
                                            
                                                          
const bool _arEveryFrameEnabled = true;

                                                                         
                                                                
typedef _CaptureActiveC = ffi.Int32 Function();
typedef _CaptureActiveDart = int Function();

                                                                         
                                                                 
class SfmLiveSnapshot implements PointCloudSnapshot {
  const SfmLiveSnapshot({
    required this.xyz,
    required this.rgb,
    required this.posesPacked,
    required this.summary,
    required this.refined,
    required this.obsOffsets,
    required this.obsFrameIds,
    required this.obsXY,
    this.gravityAlignQuatWxyz,
    this.posesPackedRawColmap,
    this.scaleAnchorFactor,
  });

                         
  final Float32List xyz;

                                                                             
                                                                         
                       
  final Uint8List rgb;

                                                                           
                                                   
                                                                           
                                                                          
                                                                           
                                                                       
                                            
  final Int32List obsOffsets;
  final Int32List obsFrameIds;
  final Float32List obsXY;

                                                                       
                                                                 
     
                                                                             
                                                                            
                                                                            
                                                                              
                                                                           
  final Float64List posesPacked;

                                                                            
                                                                         
                                                                    
                                                                            
                                                                    
                                                                        
  final List<double>? gravityAlignQuatWxyz;

                                                                          
                                                                    
                                                                            
                                                                          
  final Float64List? posesPackedRawColmap;

                                                       
                                                                 
                                                   
  final double? scaleAnchorFactor;

                                                                          
                                                                          
  final Map<String, dynamic> summary;

  final bool refined;

  int get pointCount => xyz.length ~/ 3;
  int get poseCount => posesPacked.length ~/ 9;
  int get registeredCount {
    var n = 0;
    for (var i = 0; i < posesPacked.length; i += 9) {
      if (posesPacked[i + 1] != 0) n++;
    }
    return n;
  }

                                              
                                                               
                                                             
                                                  
                                   
  List<int> get unregisteredFrameIds {
    final ids = <int>[];
    for (var i = 0; i < posesPacked.length; i += 9) {
      if (posesPacked[i + 1] == 0) ids.add(posesPacked[i].toInt());
    }
    ids.sort();
    return ids;
  }

                                               
                                                         
                                                 
                                               
  List<SfmDisconnectedSegment> get disconnectedSegments =>
      disconnectedSegmentsFromPoses(posesPacked);
}

                                                           
                                      
                                                     
List<SfmDisconnectedSegment> disconnectedSegmentsFromPoses(
  Float64List posesPacked,
) {
  final poseCount = posesPacked.length ~/ 9;
  final frames = List<({int id, bool reg})>.generate(poseCount, (k) {
    final o = k * 9;
    return (id: posesPacked[o].toInt(), reg: posesPacked[o + 1] != 0);
  })..sort((a, b) => a.id.compareTo(b.id));
  final segs = <SfmDisconnectedSegment>[];
  int? prevRegistered;
  var i = 0;
  while (i < frames.length) {
    if (frames[i].reg) {
      prevRegistered = frames[i].id;
      i++;
      continue;
    }
    final start = i;
    while (i < frames.length && !frames[i].reg) {
      i++;
    }
    segs.add((
      firstId: frames[start].id,
      lastId: frames[i - 1].id,
      count: i - start,
      prevRegisteredId: prevRegistered,
      nextRegisteredId: i < frames.length ? frames[i].id : null,
    ));
  }
  return segs;
}

                                                      
                                                   
                                                  
                                  
typedef SfmDisconnectedSegment = ({
  int firstId,
  int lastId,
  int count,
  int? prevRegisteredId,
  int? nextRegisteredId,
});

                                                                       
                                     
                                                        
                       

                                                         
sealed class SfmLiveEvent {
  const SfmLiveEvent();
}

                                                
class SfmLiveFrameFed extends SfmLiveEvent {
  const SfmLiveFrameFed({
    required this.seq,
    required this.frameId,
    required this.elapsedMs,
    required this.result,
    required this.jpegPath,
  });
  final int seq;
  final int frameId;                        
  final int elapsedMs;
  final String result;
  final String? jpegPath;
}

                                                                          
                                                                      
                                                            
class SfmLiveFrameQueued extends SfmLiveEvent {
  const SfmLiveFrameQueued(this.seq, this.queueDepth);
  final int seq;
  final int queueDepth;
}

                                                                                
                                                                                 
                                                                                
                          
class SfmLivePreview extends SfmLiveEvent {
  const SfmLivePreview(this.snapshot);
  final SfmLiveSnapshot snapshot;
}

                                           
                                                 
                                                       
                                                            
                                                   
                                                
                                    
class SfmLiveConnectivity extends SfmLiveEvent {
  const SfmLiveConnectivity(this.posesPacked);

                                                          
  final Float64List posesPacked;
}

                                           
                                               
                                                         
                                                
                                                     
                                                            
                                              
                                                
class SfmLiveTrueParallax extends SfmLiveEvent {
  const SfmLiveTrueParallax({
    required this.framesPacked,
    required this.voxelKeys,
    required this.voxelDeg,
    required this.computeMs,
    required this.sampledPoints,
  });

                                                   
  final Float64List framesPacked;

                                                                 
  final Int64List voxelKeys;
  final Float32List voxelDeg;

                                              
  final int computeMs;
  final int sampledPoints;
}

                                                            
class SfmLiveLocalReady extends SfmLiveEvent {
  const SfmLiveLocalReady(this.snapshot, this.finalizeMs);
  final SfmLiveSnapshot snapshot;
  final int finalizeMs;
}

                                                                            
                                                                         
                                                                       
                                                           
/// Derived from the worker's finalize_phase1 telemetry line (zero protocol
                                              
class SfmLiveFinalizePhase1Done extends SfmLiveEvent {
  const SfmLiveFinalizePhase1Done();
}

                                                      
   
                                                                     
                                                            
                                                               
                                                 
                                             
   
                                                
                                                           
class SfmLiveFinalizeProgress extends SfmLiveEvent {
  const SfmLiveFinalizeProgress({
    required this.stage,
    required this.round,
    required this.iter,
    required this.maxIter,
  });
  final int stage;
  final int round;
  final int iter;
  final int maxIter;
}

                                                                       
class SfmLiveRefined extends SfmLiveEvent {
  const SfmLiveRefined(this.snapshot, this.refineMs);
  final SfmLiveSnapshot snapshot;
  final int refineMs;
}

                                                                         
                                                                      
class SfmLiveFailed extends SfmLiveEvent {
  const SfmLiveFailed(this.stage, this.message);
  final String stage;
  final String message;
}

                                                              
                                       
                                                    

                                                                           
                                                                            
                                                                              
class SfmFedFrameMeta {
  const SfmFedFrameMeta({
    required this.jpegPath,
    required this.imageW,
    required this.imageH,
    required this.grayW,
    required this.grayH,
    required this.fx,
    required this.fy,
    required this.cx,
    required this.cy,
    this.captureTimestamp,
    this.arkitQuatWxyz,
    this.arkitTransTxyz,
    this.arkitCameraCenterWorld,
  });
  final String jpegPath;
  final int imageW;
  final int imageH;
  final int grayW;
  final int grayH;
  final double fx;
  final double fy;
  final double cx;
  final double cy;
  final double? captureTimestamp;

                                                                           
                                                                            
                                                                              
                                                                             
                                                
  final List<double>? arkitQuatWxyz;

                                                                            
  final List<double>? arkitTransTxyz;

                                                                     
  final List<double>? arkitCameraCenterWorld;
}

                                                                            
                                                        
class _SpooledFrame {
  _SpooledFrame({
    required this.seq,
    required this.path,
    required this.w,
    required this.h,
    required this.captureTimestamp,
    required this.fx,
    required this.fy,
    required this.cx,
    required this.cy,
    required this.quatWxyz,
    required this.trans,
  });
  final int seq;
  final String path;
  final int w;
  final int h;
  final double captureTimestamp;
  final double fx;
  final double fy;
  final double cx;
  final double cy;
  final Float64List? quatWxyz;
  final Float64List? trans;
}

                                                                            
                                                                           
                                                                          
enum _AlignmentSnapshotStage { preview, localReady, refined }

                                                                            
/// via [start]; feed via [offerFrame]; end via [finalize]; ALWAYS [dispose]
                                                                       
class SfmLiveRecon {
                                                                          
                                                                         
                                                            
  SfmLiveRecon._(
    this._toWorker,
    this._fromWorker,
    this._isolate,
    this._sub,
    this._dbPath,
    this._leaseOwner,
    this._photoArchiveActivityLease,
  );

  final SendPort _toWorker;
  final ReceivePort _fromWorker;
  final Isolate _isolate;
  final StreamSubscription<dynamic> _sub;
  final String _dbPath;
  final Object _leaseOwner;
  final PhotoArchiveActivityLease _photoArchiveActivityLease;

  final _events = StreamController<SfmLiveEvent>.broadcast();
  Stream<SfmLiveEvent> get events => _events.stream;

  int _seq = 0;
  int _liveCloudSourceReceiveSequence = 0;
  int _inFlight = 0;                                               
  int _fedOk = 0;
  bool _finalizeRequested = false;                                          
                                                                      
  bool _prepayInFlight = false;
                                                          
                               
  bool _thermalDropArmed = false;
  bool _finalizeSent = false;                                              

                                                      
  static final int _prefetchAbBlock = (() {
    final v = Platform.environment['OFFICIAL_AETHER_PREFETCH_AB_BLOCK'];
    return v == null ? 0 : (int.tryParse(v) ?? 0);
  })();
  static bool _prefetchAbArmB(int seq) =>
      _prefetchAbBlock <= 0 || ((seq ~/ _prefetchAbBlock) % 2 == 1);

                                                                 
                                                       
                                                  
                                          
                                                               
                               
  final List<SfmFedFrameMeta> _extractDebts = [];
  final Set<int> _debtRefeedSeqs = {};
  bool _debtCpuEnvSet = false;
  bool _pumping = false;
  bool _disposed = false;
  Completer<void>? _disposeAck;
  Future<void>? _disposeFuture;

                                                                           
                                                                            
                                                                  
  final List<_SpooledFrame> _spool = <_SpooledFrame>[];

                                                                        
                                                     
  final Map<int, SfmFedFrameMeta> _pendingMeta = <int, SfmFedFrameMeta>{};
  final Map<int, SfmFedFrameMeta> _fedMeta = <int, SfmFedFrameMeta>{};
  final Map<String, int> _nativeFrameIdByJpegPath = <String, int>{};
  final Map<String, Completer<bool>> _removeWhenFrameAcked =
      <String, Completer<bool>>{};
  final Map<int, Completer<bool>> _removeRequests = <int, Completer<bool>>{};
  int _nextRemoveRequestId = 0;

                                                           
                                                  
  final Map<int, int> _seqOfferMs = <int, int>{};
  final Map<int, int> _seqSentMs = <int, int>{};

                                                   
                                            
  int _finalizeRequestMs = 0;

                                                 
     
                                         
                                            
                                                     
                                                   
                                      
  int _finalizeBacklog = 0;
  int _finalizeInFlight = 0;

                                                                           
                                                 
  Map<int, SfmFedFrameMeta> get fedFrameMeta => _fedMeta;

                                                              
  int get fedCount => _fedOk;

                                                   
  int get queuedCount => _spool.length;

                                                                            
                                                                  
  int get remainingCount => _spool.length + _inFlight;

                                                                  
  int get offeredCount => _seq;

  bool get finalizeStarted => _finalizeRequested;

                                                                          
                                                                           
                                          
  static bool get isSupported => AetherSfm.isSupported;

                                                                         
                                                                          
                                                                             
                                                                             
                
  static Future<SfmLiveRecon?> start({required String dbPath}) async {
    if (!isSupported) {
      DeviceLog.log('SfmLive', 'start: unsupported (simulator) — hidden');
      return null;
    }
    final leaseOwner = Object();
    reconstructionLease.acquire(
      owner: leaseOwner,
      pipeline: ReconstructionPipeline.official,
    );
    final photoArchiveActivityLease = photoArchiveCoordinator
        .beginReconstructionActivity(Directory(File(dbPath).parent.path));
    final fromWorker = ReceivePort();
    Isolate? isolate;
    StreamSubscription<dynamic>? sub;
    var handedOff = false;
    try {
      try {
        isolate = await Isolate.spawn(
          _sfmWorkerMain,
                                                                     
          _SfmWorkerBootstrap(
            fromWorker.sendPort,
            dbPath,
            _arEveryFrameEnabled,
          ),
          debugName: 'official_sfm_live_recon',
          errorsAreFatal: true,
        );
      } catch (e) {
        DeviceLog.log('SfmLive', 'worker spawn FAILED: $e');
        return null;
      }

                                                                         
                                                                         
                                                                            
                                                                             
                                 
      final handshake = Completer<SendPort?>();
      SfmLiveRecon? recon;
      sub = fromWorker.listen((msg) {
        if (!handshake.isCompleted) {
          handshake.complete(msg is SendPort ? msg : null);
          return;
        }
        recon?._onWorkerMessage(msg);
      });
      SendPort? port;
      try {
        port = await handshake.future.timeout(
          const Duration(seconds: 10),
          onTimeout: () => null,
        );
      } catch (_) {
        port = null;
      }
      if (port == null) {
        DeviceLog.log('SfmLive', 'worker handshake FAILED/timeout — disabled');
        return null;
      }
      recon = SfmLiveRecon._(
        port,
        fromWorker,
        isolate,
        sub,
        dbPath,
        leaseOwner,
        photoArchiveActivityLease,
      );
      handedOff = true;
      DeviceLog.log('SfmLive', 'worker up (db=$dbPath)');
      return recon;
    } catch (e, st) {
      DeviceLog.log('SfmLive', 'worker startup FAILED: $e\n$st');
      return null;
    } finally {
      if (!handedOff) {
        try {
          await sub?.cancel();
        } catch (_) {
                                                                             
        }
        try {
          fromWorker.close();
          isolate?.kill(priority: Isolate.immediate);
        } finally {
          reconstructionLease.release(leaseOwner);
          await photoArchiveActivityLease.close();
        }
      }
    }
  }

                                                                             
                                                                        
  bool offerFrame(OfficialHighResReconstructionInput feed) {
    if (_disposed || _finalizeRequested) return false;
    if (feed.intrinsics.length < 4 ||
        feed.imageWidth != OfficialHighResReconstructionInput.requiredWidth ||
        feed.imageHeight != OfficialHighResReconstructionInput.requiredHeight ||
        !File(feed.jpegPath).existsSync()) {
      return false;
    }
    final seq = ++_seq;
    _seqOfferMs[seq] = DateTime.now().millisecondsSinceEpoch;             
    final fx = feed.intrinsics[0];
    final fy = feed.intrinsics[1];
    final cx = feed.intrinsics[2];
    final cy = feed.intrinsics[3];

                                                                         
                                                                                 
                                                                                
                                       
    Float64List? quatWxyz;
    Float64List? trans;
    List<double>? cameraCenterWorld;
    if (feed.cameraTransform.length == 16) {
      final c2w = vm.Matrix4.fromList(feed.cameraTransform);
      final cWorld = c2w.getTranslation();
      final rW2c = c2w.getRotation()..transpose();
      final tW2c = rW2c.transform(-cWorld);
      final q = vm.Quaternion.fromRotation(rW2c)..normalize();
      quatWxyz = Float64List.fromList([q.w, q.x, q.y, q.z]);
      trans = Float64List.fromList([tW2c.x, tW2c.y, tW2c.z]);
      cameraCenterWorld = [cWorld.x, cWorld.y, cWorld.z];
    }

    _pendingMeta[seq] = SfmFedFrameMeta(
      jpegPath: feed.jpegPath,
      imageW: feed.imageWidth,
      imageH: feed.imageHeight,
      grayW: feed.imageWidth,
      grayH: feed.imageHeight,
      fx: fx,
      fy: fy,
      cx: cx,
      cy: cy,
      captureTimestamp: feed.captureTimestamp,
      arkitQuatWxyz: quatWxyz?.toList(),                                      
      arkitTransTxyz: trans?.toList(),
      arkitCameraCenterWorld: cameraCenterWorld,
    );

                                                            
                                          
    if (!sfmFeedShouldSpool(inFlight: _inFlight, spoolDepth: _spool.length) &&
        _thermalAllowsFeedNow()) {
      _sendJpegFrameCmd(
        seq,
        feed.jpegPath,
        feed.imageWidth,
        feed.imageHeight,
        feed.captureTimestamp,
        fx,
        fy,
        cx,
        cy,
        quatWxyz,
        trans,
      );
    } else {
      _spool.add(
        _SpooledFrame(
          seq: seq,
          path: feed.jpegPath,
          w: feed.imageWidth,
          h: feed.imageHeight,
          captureTimestamp: feed.captureTimestamp,
          fx: fx,
          fy: fy,
          cx: cx,
          cy: cy,
          quatWxyz: quatWxyz,
          trans: trans,
        ),
      );
      _events.add(SfmLiveFrameQueued(seq, _spool.length));
      DeviceLog.log(
        'SfmLive',
        'frame#$seq queued (inFlight=$_inFlight, depth=${_spool.length})',
      );
    }
                                                                        
                                                          
                                                
    if (_spool.isNotEmpty && _inFlight == 0) unawaited(_pump());
    return true;
  }

                                                                          
     
                                                                            
                                                                          
                                                                    
                                                                        
                                                                  
  Future<bool> removePhoto(String jpegPath) async {
    if (_disposed || _finalizeRequested || jpegPath.isEmpty) return false;

    final queuedIndex = _spool.indexWhere((entry) => entry.path == jpegPath);
    if (queuedIndex >= 0) {
      final entry = _spool.removeAt(queuedIndex);
      _pendingMeta.remove(entry.seq);
      _seqOfferMs.remove(entry.seq);
      _seqSentMs.remove(entry.seq);
      _events.add(SfmLiveFrameQueued(entry.seq, _spool.length));
      DeviceLog.log('SfmLive', 'user removed queued frame#${entry.seq}');
      return true;
    }

    final nativeFrameId = _nativeFrameIdByJpegPath[jpegPath];
    if (nativeFrameId != null) {
      return _requestNativeFrameRemoval(nativeFrameId);
    }

    final isInFlight = _pendingMeta.values.any(
      (meta) => meta.jpegPath == jpegPath,
    );
    if (isInFlight) {
      final existing = _removeWhenFrameAcked[jpegPath];
      if (existing != null) return existing.future;
      final completer = Completer<bool>();
      _removeWhenFrameAcked[jpegPath] = completer;
      return completer.future;
    }

                                                                            
                                                                              
                                                      
    return true;
  }

  Future<bool> _requestNativeFrameRemoval(int frameId) async {
    final requestId = ++_nextRemoveRequestId;
    final completer = Completer<bool>();
    _removeRequests[requestId] = completer;
    _toWorker.send(<String, Object?>{
      'cmd': 'remove_frame',
      'requestId': requestId,
      'frameId': frameId,
    });
    try {
      return await completer.future.timeout(const Duration(seconds: 30));
    } on TimeoutException {
      _removeRequests.remove(requestId);
      return false;
    }
  }

  void _sendJpegFrameCmd(
    int seq,
    String jpegPath,
    int w,
    int h,
    double captureTimestamp,
    double fx,
    double fy,
    double cx,
    double cy,
    Float64List? q,
    Float64List? t,
  ) {
    _inFlight++;
    _seqSentMs[seq] = DateTime.now().millisecondsSinceEpoch;             
    _toWorker.send(<String, Object?>{
      'cmd': 'jpeg_frame',
      'seq': seq,
      'jpegPath': jpegPath,
      'w': w,
      'h': h,
      'captureTimestamp': captureTimestamp,
      'fx': fx,
      'fy': fy,
      'cx': cx,
      'cy': cy,
      'q': q,
      't': t,
    });
  }

                                                       
     
                                            
                                                                              
                                                                     
                                              
                                                         
                                          
                                                                      
                                                 
                                                
                                                
                                                   
  bool _thermalAllowsFeedNow() {
    if (_finalizeRequested) return true;         
    final thermal = PwTelemetry.sample()?.thermalState ?? -1;
    if (thermal >= 3) return false;                
    if (thermal == 2) {
                                 
      if (_thermalDropArmed) {
        _thermalDropArmed = false;
        return false;
      }
      _thermalDropArmed = true;
      return true;
    }
    _thermalDropArmed = false;                       
    return true;
  }

                                                                           
                                                                         
  Future<void> _pump() async {
    if (_pumping || _disposed) return;
    _pumping = true;
    try {
      while (!_disposed &&
          sfmFeedCanPumpNext(inFlight: _inFlight, spoolDepth: _spool.length)) {
                                                                   
                                                
                                                          
                   
        if (!_finalizeRequested && !_thermalAllowsFeedNow()) break;
                                                                 
                                                    
                                                           
                                 
                                                  
                                    
                                                     
                                                          
                                           
                                                                 
                           
                                                          
        // 避免挂起期间新帧 append 使尾部索引漂移。
                                                                 
                         
        final entry = _finalizeRequested
            ? _spool.removeAt(0)
            : _spool.removeLast();
        try {
          if (!await File(entry.path).exists()) {
            throw FileSystemException('canonical JPEG missing', entry.path);
          }
                                                           
                                                 
                                                      
                                          
                                                         
                                                                      
                                                
                                                    
                                             
          if (_spool.isNotEmpty && _prefetchAbArmB(entry.seq)) {
            final next = _finalizeRequested ? _spool.first : _spool.last;
            _toWorker.send(<String, Object?>{
              'cmd': 'prefetch',
              'path': next.path,
            });
          }
          _sendJpegFrameCmd(
            entry.seq,
            entry.path,
            entry.w,
            entry.h,
            entry.captureTimestamp,
            entry.fx,
            entry.fy,
            entry.cx,
            entry.cy,
            entry.quatWxyz,
            entry.trans,
          );
        } catch (e) {
                                                                            
                                                      
          final meta = _pendingMeta.remove(entry.seq);
          _seqOfferMs.remove(entry.seq);
          _seqSentMs.remove(entry.seq);
          _events.add(
            SfmLiveFrameFed(
              seq: entry.seq,
              frameId: -1,
              elapsedMs: 0,
              result: 'missingJpeg',
              jpegPath: meta?.jpegPath ?? entry.path,
            ),
          );
          DeviceLog.log('SfmLive', 'spool #${entry.seq} unreadable: $e');
        }
      }
    } finally {
      _pumping = false;
    }
    _maybeSendFinalize();
  }

  void _maybeSendFinalize() {
    if (_disposed ||
        !sfmFeedCanSendFinalize(
          finalizeRequested: _finalizeRequested,
          finalizeSent: _finalizeSent,
          spoolDepth: _spool.length,
          inFlight: _inFlight,
        )) {
      return;
    }
                                                        
                                                 
                           
    if (_extractDebts.isNotEmpty) {
      _repayExtractDebts();
      return;
    }
    if (_debtCpuEnvSet) {
      AetherProcessEnv.unset('OFFICIAL_AETHER_EXTRACT_CPU_FALLBACK');
      _debtCpuEnvSet = false;
    }
    _finalizeSent = true;
    DeviceLog.log('SfmLive', 'queue drained → finalize dispatched');
                                                          
    TelemetryWriter.instance.event('queue_drain', {
      'ms': _finalizeRequestMs > 0
          ? DateTime.now().millisecondsSinceEpoch - _finalizeRequestMs
          : 0,
      'offered': _seq,
      'fed': _fedOk,
                                                                   
                                                       
                                                   
      'backlog': _finalizeBacklog,
      'in_flight': _finalizeInFlight,
    });
    _toWorker.send(const <String, Object?>{'cmd': 'finalize'});
  }

                                                           
                                                    
                                                 
  void _repayExtractDebts() {
    final debts = List<SfmFedFrameMeta>.of(_extractDebts);
    _extractDebts.clear();
    if (!_debtCpuEnvSet) {
      AetherProcessEnv.set('OFFICIAL_AETHER_EXTRACT_CPU_FALLBACK', '1');
      _debtCpuEnvSet = true;
    }
    for (final m in debts) {
      final seq = ++_seq;
      _seqOfferMs[seq] = DateTime.now().millisecondsSinceEpoch;
      _debtRefeedSeqs.add(seq);
      _pendingMeta[seq] = m;
      _spool.add(
        _SpooledFrame(
          seq: seq,
          path: m.jpegPath,
          w: m.imageW,
          h: m.imageH,
          captureTimestamp: m.captureTimestamp ?? 0.0,
          fx: m.fx,
          fy: m.fy,
          cx: m.cx,
          cy: m.cy,
          quatWxyz: m.arkitQuatWxyz != null
              ? Float64List.fromList(m.arkitQuatWxyz!)
              : null,
          trans: m.arkitTransTxyz != null
              ? Float64List.fromList(m.arkitTransTxyz!)
              : null,
        ),
      );
    }
    DeviceLog.log(
      'SfmLive',
      'extract-debt repay: refeeding ${debts.length} frame(s) '
          'with CPU fallback temporarily enabled',
    );
    TelemetryWriter.instance.event('extract_debt_repay', {'n': debts.length});
    unawaited(_pump());
  }

                                                                      
                                                                            
                                                                          
                                                                            
                                                                           
                                                                            
                                                                          
                                           
  void _maybePrepay() {
    if (_disposed || _finalizeRequested || _prepayInFlight) return;
    if (_spool.isNotEmpty || _inFlight > 0) return;
    _prepayInFlight = true;
    _toWorker.send(const <String, Object?>{'cmd': 'repay', 'budget': 2});
  }

                                                                           
                                                                      
                                                                           
  void finalize() {
    if (_disposed || _finalizeRequested) return;
    _finalizeRequested = true;
                                                           
                                               
    AetherMatchFlags.setCaptureActive(false);
    _finalizeRequestMs = DateTime.now().millisecondsSinceEpoch;      
                                                      
                                        
    _finalizeBacklog = _spool.length;
    _finalizeInFlight = _inFlight;
    if (_spool.isNotEmpty || _inFlight > 0) {
      DeviceLog.log(
        'SfmLive',
        'finalize deferred: inFlight=$_inFlight queued=${_spool.length}',
      );
                                                                          
                                                                           
                                                                     
      _toWorker.send(const <String, Object?>{'cmd': 'finish_pending'});
      unawaited(_pump());
      return;
    }
    _maybeSendFinalize();
  }

                                                                           
                                                                                
                                                                                
                                                                         
                                                                                  
                                                                              
  void resumeFromDb({int imageWidth = 3840, int imageHeight = 2160}) {
    if (_disposed || _finalizeRequested) return;
    _finalizeRequested = true;
    _finalizeSent = true;
    _toWorker.send(<String, Object?>{
      'cmd': 'resume',
      'w': imageWidth,
      'h': imageHeight,
    });
  }

                                                
                                                    
                                                         
                                                                       
                                                  
  void seedFedMeta(Map<int, SfmFedFrameMeta> meta) {
    _fedMeta.addAll(meta);
  }

                                                                        
                                                                                
                                                                              
                                                                                
  /// having to guess the mapping from disk. One tiny append per registered
                                                                             
  void _persistFedMeta(int frameId, SfmFedFrameMeta m) {
    try {
      final dir = File(_dbPath).parent.path;
      final meta = <String, Object?>{
        'frameId': frameId,
        'jpegPath': m.jpegPath,
        'grayW': m.grayW,
        'grayH': m.grayH,
        if (m.captureTimestamp != null) 'captureTimestamp': m.captureTimestamp,
      };
      if (m.arkitQuatWxyz != null &&
          m.arkitTransTxyz != null &&
          m.arkitCameraCenterWorld != null) {
        meta.addAll(<String, Object?>{
                                                   
                                                            
                                                                
                                       
          'arkitPoseConvention':
              'worldAlignment.gravity; CamFromWorld inverted from ARKit '
              'cameraToWorld, in ARKit CAMERA AXES (COLMAP C=diag(1,-1,-1) '
              'flip NOT applied); plus camera center in world',
          'arkitCamFromWorldQwxyz': m.arkitQuatWxyz,
          'arkitCamFromWorldTxyz': m.arkitTransTxyz,
          'arkitCameraCenterWorld': m.arkitCameraCenterWorld,
        });
      }
      final line = '${jsonEncode(meta)}\n';
      File(
        '$dir/official_sfm_fed_frames.jsonl',
      ).writeAsStringSync(line, mode: FileMode.append, flush: false);
    } catch (_) {}
  }

                                                                         
                                                                         
  Future<void> dispose() => _disposeFuture ??= _disposeOnce();

  Future<void> _disposeOnce() async {
    _disposed = true;
                                                                             
    _spool.clear();
    final ack = _disposeAck = Completer<void>();
    try {
      _toWorker.send(const <String, Object?>{'cmd': 'dispose'});
                                                                       
                                                                      
      await ack.future.timeout(const Duration(seconds: 120));
    } catch (_) {
                                                   
    } finally {
      try {
        await _sub.cancel();
      } catch (_) {
                                                                
      }
      _fromWorker.close();
      _isolate.kill(priority: Isolate.immediate);
      try {
        await _events.close();
      } finally {
        reconstructionLease.release(_leaseOwner);
        unawaited(_photoArchiveActivityLease.close());
      }
    }
  }

  void _onWorkerMessage(dynamic msg) {
    if (msg is! Map) return;
    switch (msg['evt']) {
      case 'log':
                                                                            
                                                           
        DeviceLog.log('SfmLive.worker', msg['line'] as String? ?? '');
      case 'telem':
                                                       
                                                         
        {
          final data = msg['data'];
          TelemetryWriter.instance.event(msg['type'] as String? ?? 'worker', {
            'iso': 'worker',
            if (data is Map)
              ...data.map((k, v) => MapEntry(k.toString(), v as Object?)),
          });
                                                 
                                                       
                                            
          if (msg['type'] == 'finalize_phase1') {
            _events.add(const SfmLiveFinalizePhase1Done());
          }
        }
      case 'frame_done':
        _inFlight = _inFlight > 0 ? _inFlight - 1 : 0;
        final ok = msg['result'] == 'ok';
        final seq = msg['seq'] as int;
        final frameId = msg['frameId'] as int;
        final meta = _pendingMeta.remove(seq);
        final removeAfterAck = meta == null
            ? null
            : _removeWhenFrameAcked.remove(meta.jpegPath);
        if (meta != null && frameId >= 0) {
          _nativeFrameIdByJpegPath[meta.jpegPath] = frameId;
        }
        if (ok && removeAfterAck == null) _fedOk++;
        if (removeAfterAck != null) {
          if (frameId >= 0) {
            unawaited(
              _requestNativeFrameRemoval(frameId).then((removed) {
                if (!removeAfterAck.isCompleted) {
                  removeAfterAck.complete(removed);
                }
              }),
            );
          } else if (!removeAfterAck.isCompleted) {
                                                                              
                                                                          
            removeAfterAck.complete(true);
          }
        } else if (ok && meta != null && frameId >= 0) {
          _fedMeta[frameId] = meta;
          _persistFedMeta(frameId, meta);
        }
                                                           
                                                  
                                                       
        if (msg['result'] == 'errExtract' && meta != null) {
          if (_debtRefeedSeqs.remove(seq)) {
            DeviceLog.log(
              'SfmLive',
              'extract-debt REPAY FAILED (CPU fallback also failed): '
                  '${meta.jpegPath.split('/').last} — frame missing from '
                  'delivery, ESCALATE',
            );
            TelemetryWriter.instance.event('extract_debt_repay_failed', {
              'jpeg': meta.jpegPath.split('/').last,
            });
          } else if (removeAfterAck == null) {
            _extractDebts.add(meta);
            DeviceLog.log(
              'SfmLive',
              'extract-debt recorded (#${_extractDebts.length}): '
                  '${meta.jpegPath.split('/').last} — repay before finalize',
            );
            TelemetryWriter.instance.event('extract_debt_recorded', {
              'jpeg': meta.jpegPath.split('/').last,
              'debts': _extractDebts.length,
            });
          }
        } else {
          _debtRefeedSeqs.remove(seq);
        }
                                                                          
                                                                              
                                                                          
        final ms = msg['ms'] as int;
        final memMb = msg['memMb'] as double?;
        final peakMb = msg['peakMb'] as double?;
        final thermal = msg['thermal'] as int?;
        final thermalName = switch (thermal) {
          0 => 'nominal',
          1 => 'fair',
          2 => 'serious',
          3 => 'critical',
          _ => '?',
        };
        DeviceLog.log(
          'TELEM',
          'frame#$seq fid=$frameId ${ok ? 'ok' : msg['result']} '
              'proc=${ms}ms | queue=${_spool.length} inflight=$_inFlight '
              'offered=$_seq fed=$_fedOk | '
              'mem=${memMb?.toStringAsFixed(0) ?? '?'}MB '
              'peak=${peakMb?.toStringAsFixed(0) ?? '?'}MB '
              'thermal=$thermalName',
        );
                                                   
                                               
                          
        {
          final offerMs = _seqOfferMs.remove(seq);
          final sentMs = _seqSentMs.remove(seq);
          final extractMs = msg['extractMs'] as double?;
          TelemetryWriter.instance.event('frame', {
            'seq': seq,
            'fid': frameId,
            'result': msg['result'],
            if (meta != null) 'jpeg': meta.jpegPath.split('/').last,
            't_offer': ?offerMs,
            't_fed': ?sentMs,
            if (offerMs != null && sentMs != null)
              'spool_wait_ms': sentMs - offerMs,
            'proc_ms': ms,
            'queue': _spool.length,
            'inflight': _inFlight,
            'offered': _seq,
            'fed': _fedOk,
            if (extractMs != null) 'extract_ms': extractMs.round(),
                                                                
                                                  
                                                    
            if (extractMs != null)
              'extract_path': extractMs > 2000 ? 'slow_extract' : 'gpu',
            if (msg['matchMs'] != null)
              'match_ms': (msg['matchMs'] as double).round(),
            if (msg['nCand'] != null) 'n_cand': msg['nCand'],
            if (msg['gpuPairs'] != null) 'gpu_pairs': msg['gpuPairs'],
            if (msg['cpuPairs'] != null) 'cpu_pairs': msg['cpuPairs'],
                                                  
            if (msg['throttledCum'] != null)
              'throttled_cum': msg['throttledCum'],
            if (msg['growAcc'] != null) 'grow_acc_cum': msg['growAcc'],
            if (msg['growRej'] != null) 'grow_rej_cum': msg['growRej'],
            if (msg['tvgPairs'] != null) 'tvg_pairs_cum': msg['tvgPairs'],
            if (msg['rawPairs'] != null) 'raw_pairs_cum': msg['rawPairs'],
            if (memMb != null) 'mem_mb': memMb.round(),
            if (peakMb != null) 'peak_mb': peakMb.round(),
            'thermal': ?thermal,
          });
        }
        _events.add(
          SfmLiveFrameFed(
            seq: seq,
            frameId: frameId,
            elapsedMs: ms,
            result: msg['result'] as String,
            jpegPath: meta?.jpegPath,
          ),
        );
                                                                            
                                                      
        unawaited(_pump());

                                                                      
        _maybePrepay();
      case 'repay_done':
        _prepayInFlight = false;
                                                                           
                                                                           
                                                                       
        if (((msg['n'] as int?) ?? 0) > 0) _maybePrepay();
      case 'frame_removed':
        final requestId = msg['requestId'] as int;
        final frameId = msg['frameId'] as int;
        final removed = msg['ok'] == true;
        if (removed) {
          final hadFrame = _fedMeta.remove(frameId) != null;
          if (hadFrame && _fedOk > 0) _fedOk--;
          _nativeFrameIdByJpegPath.removeWhere(
            (_, mappedFrameId) => mappedFrameId == frameId,
          );
        }
        final completer = _removeRequests.remove(requestId);
        if (completer != null && !completer.isCompleted) {
          completer.complete(removed);
        }
        DeviceLog.log(
          'SfmLive',
          'user remove frameId=$frameId ok=$removed stats=${msg['stats']}',
        );
      case 'preview':
                                                                             
                                                                                 
                                                                      
        final snapshot = _gravityAlign(
          _snapshotFromMsg(msg, refined: false),
          stage: _AlignmentSnapshotStage.preview,
        );
        final sourceReceiveSeq = ++_liveCloudSourceReceiveSequence;
        final sourceReceiveAt = DateTime.now().millisecondsSinceEpoch;
        snapshot.summary['diag_source_receive_seq'] = sourceReceiveSeq;
        snapshot.summary['diag_source_receive_t'] = sourceReceiveAt;
        final source = snapshot.summary['source']?.toString() ?? 'unknown';
        final publishVersion =
            (snapshot.summary['publish_version'] as num?)?.toInt() ?? 0;
        TelemetryWriter.instance.event('live_cloud_source_receive_v2', {
          'contract': 'PW_LIVE_CLOUD_DIAG_RUNTIME_V2_STREAM',
          'source_receive_seq': sourceReceiveSeq,
          'source_receive_t': sourceReceiveAt,
          'source': source,
          'publish_version': publishVersion,
          'points': snapshot.pointCount,
          'observation_only': true,
        });
        DeviceLog.log(
          'LIVE_CLOUD_DIAG_V2',
          'source_receive seq=$sourceReceiveSeq source=$source '
              'version=$publishVersion points=${snapshot.pointCount}',
        );
        _events.add(SfmLivePreview(snapshot));
      case 'live_poses':
                                                            
        _events.add(
          SfmLiveConnectivity(msg['poses'] as Float64List? ?? Float64List(0)),
        );
      case 'live_parallax':
                                                      
        _events.add(
          SfmLiveTrueParallax(
            framesPacked: msg['frames'] as Float64List? ?? Float64List(0),
            voxelKeys: msg['voxKeys'] as Int64List? ?? Int64List(0),
            voxelDeg: msg['voxDeg'] as Float32List? ?? Float32List(0),
            computeMs: msg['ms'] as int? ?? -1,
            sampledPoints: msg['sampled'] as int? ?? 0,
          ),
        );
      case 'local_ready':
                                                               
                                             
                                                          
                                                   
        TelemetryWriter.instance.event('finalize_wall', {
          'phase': 'local_ready',
          'ms': msg['ms'] as int? ?? -1,
        });
        _events.add(
          SfmLiveLocalReady(
            _gravityAlign(
              _snapshotFromMsg(msg, refined: false),
              stage: _AlignmentSnapshotStage.localReady,
            ),
            msg['ms'] as int,
          ),
        );
      case 'finalize_progress':
                                                             
                                             
        _events.add(
          SfmLiveFinalizeProgress(
            stage: msg['stage'] as int? ?? 0,
            round: msg['round'] as int? ?? 0,
            iter: msg['iter'] as int? ?? 0,
            maxIter: msg['max_iter'] as int? ?? 0,
          ),
        );
      case 'refined':
        TelemetryWriter.instance.event('finalize_wall', {
          'phase': 'refined',
          'ms': msg['ms'] as int? ?? -1,
        });
        _events.add(
          SfmLiveRefined(
            _gravityAlign(
              _snapshotFromMsg(msg, refined: true),
              stage: _AlignmentSnapshotStage.refined,
            ),
            msg['ms'] as int,
          ),
        );
                                                              
                                                             
                                                  
                                                
      case 'error':
                                                                      
        TelemetryWriter.instance.event('sfm_error', {
          'stage': msg['stage'],
          'message': '${msg['message']}',
        });
        _events.add(
          SfmLiveFailed(
            msg['stage'] as String? ?? 'unknown',
            msg['message'] as String? ?? 'unknown',
          ),
        );
      case 'disposed':
        _disposeAck?.complete();
    }
  }

                                                                       
     
                                                                         
                                                                           
                                                                            
                                 
                                                                            
                                                                            
                                                                          
                                                                              
                                                                           
                                                                       
                                                                             
     
                                                   
                                                          
  SfmLiveSnapshot _gravityAlign(
    SfmLiveSnapshot snap, {
    required _AlignmentSnapshotStage stage,
  }) {
    if (stage == _AlignmentSnapshotStage.preview) {
      TelemetryWriter.instance.event('preview_skip', {
        'schema_version': 1,
        'phase': 'preview',
        'reason': 'already_arkit_gravity_metric',
        'gravity_status': 'not_required',
        'scale_status': 'not_required',
        'n_points': snap.pointCount,
      });
      return snap;
    }

    final stageTelemetry = stage == _AlignmentSnapshotStage.refined
        ? <String, Object?>{'phase': 'refined', 'authority': 'authoritative'}
        : <String, Object?>{
            'phase': 'local_ready',
            'authority': 'fallback_candidate',
          };

    void emitFinalAlignmentResult({
      required String gravityStatus,
      required String? gravityReason,
      required List<double>? gravityQuatWxyz,
      required String scaleStatus,
      required String? scaleReason,
      required double? scaleFactor,
      required GravityAlignDiagV1 gravityDiag,
      ScaleAnchorDiagV1? scaleDiag,
    }) {
      TelemetryWriter.instance.event('final_alignment_result', {
        'schema_version': 1,
        ...stageTelemetry,
        'gravity_status': gravityStatus,
        'gravity_reason': gravityReason,
        'gravity_quat_wxyz': gravityQuatWxyz,
        'registered_frames': gravityDiag.registeredFrames,
        'frames_with_arkit_quat': gravityDiag.framesWithArkitQuat,
        'required_frames': gravityDiag.requiredFrames,
        'scale_status': scaleStatus,
        'scale_reason': scaleReason,
        'scale_factor': scaleFactor,
        'scale_registered_frames': scaleDiag?.registeredFrames,
        'scale_pairs_with_arkit_center': scaleDiag?.pairsWithArkitCenter,
        'scale_usable_ratios': scaleDiag?.usableRatios,
        'scale_required_pairs': scaleDiag?.requiredPairs,
        'scale_rejected_factor': scaleDiag?.rejectedFactor,
        'fed_meta_size': _fedMeta.length,
        'n_points': snap.pointCount,
      });
    }

                                                     
                                                               
                                           
                                               
                                                       
                                                                 
                                            
                                    
                                                       
    final diag = GravityAlignDiagV1();
    final q = gravityAlignQuatWxyz(
      posesPacked: snap.posesPacked,
      arkitQuatWxyzOf: (frameId) => _fedMeta[frameId]?.arkitQuatWxyz,
      diag: diag,
    );
    if (q == null || snap.xyz.isEmpty) {
                                               
                   
      final reason = snap.xyz.isEmpty
          ? 'empty_cloud'
          : (diag.skipReason ?? 'unknown');
      emitFinalAlignmentResult(
        gravityStatus: 'skipped',
        gravityReason: reason,
        gravityQuatWxyz: null,
        scaleStatus: 'skipped',
        scaleReason: 'gravity_not_applied',
        scaleFactor: null,
        gravityDiag: diag,
      );
      DeviceLog.log(
        'SfmLive',
        'gravity align SKIPPED (reason=$reason '
            'registered=${diag.registeredFrames} '
            'withArkitQuat=${diag.framesWithArkitQuat}/${diag.requiredFrames} '
            'fedMeta=${_fedMeta.length} refined=${snap.refined}) '
            '→ delivering UNALIGNED cloud',
      );
      return snap;
    }
                                                                   
                                                     
                                                           
                                             
                
                                                                   
                                                    
                                                      
                                   
    final scaleDiag = _scaleAnchorEnabled ? ScaleAnchorDiagV1() : null;
    final double? s = _scaleAnchorEnabled
        ? scaleAnchorFactor(
            posesPacked: snap.posesPacked,
            arkitCenterWorldOf: (frameId) =>
                _fedMeta[frameId]?.arkitCameraCenterWorld,
            diag: scaleDiag,
          )
        : null;
    if (scaleDiag != null && s == null) {
      DeviceLog.log(
        'SfmLive',
        'scale anchor SKIPPED (reason=${scaleDiag.skipReason} '
            'registered=${scaleDiag.registeredFrames} '
            'pairs=${scaleDiag.pairsWithArkitCenter}/${scaleDiag.requiredPairs} '
            'ratios=${scaleDiag.usableRatios} '
            'rejected_s=${scaleDiag.rejectedFactor?.toStringAsFixed(4) ?? '-'}) '
            '→ delivering UNSCALED (raw BA gauge)',
      );
    }
    emitFinalAlignmentResult(
      gravityStatus: 'applied',
      gravityReason: null,
      gravityQuatWxyz: q,
      scaleStatus: !_scaleAnchorEnabled
          ? 'disabled'
          : (s != null ? 'applied' : 'skipped'),
      scaleReason: !_scaleAnchorEnabled
          ? 'feature_disabled'
          : (s == null ? (scaleDiag?.skipReason ?? 'unknown') : null),
      scaleFactor: s,
      gravityDiag: diag,
      scaleDiag: scaleDiag,
    );
    var xyz = rotatePointsByQuatWxyz(snap.xyz, q);
    var poses = gravityAlignedPosesPacked(snap.posesPacked, q);
    if (s != null) {
      xyz = scaleAnchoredPoints(xyz, s);
      poses = scaleAnchoredPosesPacked(poses, s);
    }
    return SfmLiveSnapshot(
      xyz: xyz,
      rgb: snap.rgb,
      posesPacked: poses,
      summary: snap.summary,
      refined: snap.refined,
      obsOffsets: snap.obsOffsets,
      obsFrameIds: snap.obsFrameIds,
      obsXY: snap.obsXY,
      gravityAlignQuatWxyz: q,
      posesPackedRawColmap: snap.posesPacked,
      scaleAnchorFactor: s,
    );
  }

                                                       
                                  
  static final bool _scaleAnchorEnabled =
      Platform.environment['OFFICIAL_AETHER_SCALE_ANCHOR'] == '1';

  static SfmLiveSnapshot _snapshotFromMsg(Map msg, {required bool refined}) {
    return SfmLiveSnapshot(
      xyz: msg['xyz'] as Float32List? ?? Float32List(0),
      rgb: msg['rgb'] as Uint8List? ?? Uint8List(0),
      posesPacked: msg['poses'] as Float64List? ?? Float64List(0),
      summary:
          (msg['summary'] as Map?)?.cast<String, dynamic>() ??
          const <String, dynamic>{},
      refined: refined,
      obsOffsets: msg['obsOffsets'] as Int32List? ?? Int32List(1),
      obsFrameIds: msg['obsFrameIds'] as Int32List? ?? Int32List(0),
      obsXY: msg['obsXY'] as Float32List? ?? Float32List(0),
    );
  }
}

                                                                        

class _SfmWorkerBootstrap {
  const _SfmWorkerBootstrap(this.reply, this.dbPath, this.arEveryFrame);
  final SendPort reply;
  final String dbPath;
                                                                 
                                                          
                                                                   
                                                                   
                                                  
                                     
  final bool arEveryFrame;
}

void _sfmWorkerMain(_SfmWorkerBootstrap boot) {
  final cmds = ReceivePort();
  boot.reply.send(cmds.sendPort);
                                                    
                                                        
  boot.reply.send(<String, Object?>{
    'evt': 'log',
    'line': 'worker up: arEveryFrame=${boot.arEveryFrame}',
  });

  AetherSfmStreamSession? session;
  Timer? pollTimer;
  var refineStart = 0;
  var disposed = false;
                                                                            
                                                                          
                                                                   
                                                                           
                                                                         
                                                                          
                                                                         
  var finishPending = false;
                                                                            
                                                                         
                                                                            
                                                                 
                                                                            
                                                                        
  _CaptureActiveDart? captureActiveFn;
  var captureActiveResolved = false;
  bool nativeCaptureActive() {
    if (!captureActiveResolved) {
      captureActiveResolved = true;
      try {
        captureActiveFn = ffi.DynamicLibrary.process()
            .lookupFunction<_CaptureActiveC, _CaptureActiveDart>(
              'aether_gpu_match_get_capture_active',
            );
      } catch (_) {
                                                                             
      }
    }
    final fn = captureActiveFn;
    return fn == null || fn() != 0;
  }

                                                                              
                                                                         
                                                     
  var peakMb = 0.0;
                                           
  final fedIds = <int>[];
  final publishPolicy = OfficialLiveSfmPublishPolicy();
  var lastConnectivityMs = 0;
                                                 
                                                  
                                                  
                                 
  final frameCenters = <int, List<double>>{};
  var lastTrueParallaxMs = 0;

  void wlog(String line) {
    boot.reply.send(<String, Object?>{'evt': 'log', 'line': line});
  }

                                                 
                                                         
                              
  void telem(String type, Map<String, Object?> data) {
    boot.reply.send(<String, Object?>{
      'evt': 'telem',
      'type': type,
      'data': data,
    });
  }

                                                            
                                                          
                                               
                                                  
                         
  Float64List connectivityPosesFrom(Int32List obsFrameIds) {
    if (fedIds.isEmpty) return Float64List(0);
    final connected = <int>{};
    for (final f in obsFrameIds) {
      connected.add(f);
    }
    final out = Float64List(fedIds.length * 9);
    for (var i = 0; i < fedIds.length; i++) {
      out[i * 9] = fedIds[i].toDouble();
      out[i * 9 + 1] = connected.contains(fedIds[i]) ? 1 : 0;
    }
    return out;
  }

                                                                             
                                                                              
                                                                              
                                                                               
                                                                                 
                                                                                
                           
  bool sendSnapshot(
    String evt,
    Map<String, dynamic> summary,
    int ms, {
    bool preview = false,
    AetherSfmPointsTracked? prefetched,
  }) {
    final s = session;
    if (s == null) return false;
                                                                            
                                                                              
                                                       
    final points =
        prefetched ?? (preview ? s.previewTracked() : s.pointsTracked());
                                                               
                                            
    if (preview && points.count == 0 && !summary.containsKey('frame_removed')) {
      return false;                             
    }
    final deliveredSummary = Map<String, dynamic>.from(summary);
    if (!preview) {
      final detail = s.streamStats();
      deliveredSummary['temporal_detail_created'] =
          detail.temporalDetailCreated;
      deliveredSummary['temporal_detail_grown'] = detail.temporalDetailGrown;
      wlog(
        'temporal-detail: pairs=${detail.temporalDetailPairs} '
        'inliers=${detail.temporalDetailMatches} '
        'created=${detail.temporalDetailCreated} '
        'grown=${detail.temporalDetailGrown} | reject '
        'cheirality=${detail.temporalDetailRejectCheirality} '
        'reproj=${detail.temporalDetailRejectReproj} '
        'tri-angle=${detail.temporalDetailRejectTriAngle} '
        'conflicts=${detail.temporalDetailConflicts}',
      );
                                                                                
                                                                        
      deliveredSummary['spatial_two_view_filtered'] = 0;
      deliveredSummary['delivered_points'] = points.count;
      wlog(
        'official-endpoint: delivered=${points.count} '
        'after native final BA/filtering; Dart filtering disabled',
      );
                                                  
                      
      telem('finalize_snapshot', {
        'evt': evt,
        'points_delivered': points.count,
        'obs': points.obsCount,
        'spatial_two_view_filtered': 0,
        'temporal_detail_created': detail.temporalDetailCreated,
        'temporal_detail_grown': detail.temporalDetailGrown,
        'temporal_detail_pairs': detail.temporalDetailPairs,
        'temporal_detail_inliers': detail.temporalDetailMatches,
        'ms': ms,
      });
    }
                                                                                  
                                                                                
                                                                          
                                                                  
                                                               
                                                                          
                                                                             
                                                                              
                                 
    final poses = preview
        ? connectivityPosesFrom(points.obsFrameIds)
        : s.posesPacked();
    wlog(
      '$evt: points=${points.count} obs=${points.obsCount} '
      'poses=${poses.length ~/ 9} ms=$ms summary=$deliveredSummary',
    );
    if (preview) {
      final st = s.streamStats();
      final cand = s.candidateStats();
      final tvgPct = (st.tvgPairs + st.rawPairs) > 0
          ? (100 * st.tvgPairs / (st.tvgPairs + st.rawPairs)).round()
          : 0;
      wlog(
        'stream-stats: tvg-inlier pairs=${st.tvgPairs} raw-fallback=${st.rawPairs} '
        '($tvgPct% verified) | grow accept=${st.growAccepted} '
        'reject=${st.growRejected} | filtered reproj=${st.reprojFiltered} '
        'tri-angle=${st.triFiltered} | candidates '
        'spatial-first=${cand.spatialFirstPairs} '
        'temporal-fallback=${cand.temporalFallbackPairs}',
      );
      wlog(
        'stream-gates: grow reject cheirality=${st.growRejectCheirality} '
        'reproj=${st.growRejectReproj} | create reject '
        'cheirality=${st.createRejectCheirality} '
        'tri-angle=${st.createRejectTriAngle} '
        'reproj=${st.createRejectReproj} | assigned same=${st.alreadyAssigned} '
        'merge-needed=${st.mergeNeeded} accepted=${st.mergeAccepted} '
        'rejected=${st.mergeRejected} | spatial considered='
        '${st.spatialConsidered} attempted=${st.spatialAttempted} '
        'written=${st.spatialWritten} inliers=${st.spatialInliers}',
      );
      wlog(
        'spatial-loop: anchors=${st.spatialAnchorPassed}/'
        '${st.spatialAnchorAttempted} regions=${st.spatialRegionsConfirmed} '
        'expand-attempted=${st.spatialExpandedAttempted} guided='
        '${st.spatialGuidedPairs}/${st.spatialGuidedInliers}candidates '
        'quadratic=${st.spatialQuadraticWritten}/'
        '${st.spatialQuadraticAttempted} budget-skipped='
        '${st.spatialBudgetSkipped}',
      );
    }
    boot.reply.send(<String, Object?>{
      'evt': evt,
      'xyz': points.xyz,
      'rgb': points.rgb,
      'poses': poses,
      'obsOffsets': points.obsOffsets,
      'obsFrameIds': points.obsFrameIds,
      'obsXY': points.obsXY,
      'summary': deliveredSummary,
      'ms': ms,
    });
    return true;
  }

  void fail(String stage, Object message) {
    wlog('ERROR at $stage: $message');
    boot.reply.send(<String, Object?>{
      'evt': 'error',
      'stage': stage,
      'message': '$message',
    });
  }

  cmds.listen((dynamic msg) {
    if (msg is! Map || disposed) return;
    switch (msg['cmd']) {
      case 'jpeg_frame':
        final sw = Stopwatch()..start();
        final w = msg['w'] as int;
        final h = msg['h'] as int;
        try {
                                                                             
                                                                         
          if (session == null) {
            session = AetherSfmStreamSession.create(
              boot.dbPath,
              imageWidth: w,
              imageHeight: h,
                                                                      
                                                       
                                                                   
                                                       
                                                               
                                                            
                                                    
              maxFeatures: AetherSfmStreamSession.researchMaxFeatures,
            );
            wlog('session created (${w}x$h, db=${boot.dbPath})');
          }
                                                         
                                                                
                                                            
                                                            
                                                
          final telPre = PwTelemetry.sample();
          if (telPre != null && telPre.thermalState >= 0) {
            try {
              session!.setThermalState(telPre.thermalState);
            } catch (_) {}                                 
          }
          final r = session!.addJpegFrame(
            msg['jpegPath'] as String,
            captureTimestamp: msg['captureTimestamp'] as double,
            fx: msg['fx'] as double,
            fy: msg['fy'] as double,
            cx: msg['cx'] as double,
            cy: msg['cy'] as double,
            quatWxyz: (msg['q'] as Float64List?)?.toList(),
            translation: (msg['t'] as Float64List?)?.toList(),
          );
          sw.stop();
                                                                            
                                                                           
                                                
          final tel = PwTelemetry.sample();
          if (tel != null && tel.physFootprintMb > peakMb) {
            peakMb = tel.physFootprintMb;
          }
                                                                       
                                                                  
                                                                         
          final dbg = session!.debugLast();
                                                    
                                                   
          var growAcc = -1, growRej = -1, tvgPairs = -1, rawPairs = -1;
          try {
            final stCum = session!.streamStats();
            growAcc = stCum.growAccepted;
            growRej = stCum.growRejected;
            tvgPairs = stCum.tvgPairs;
            rawPairs = stCum.rawPairs;
          } catch (_) {}
                                                 
          var throttledCum = -1;
          try {
            throttledCum = session!.thermalThrottledFrames();
          } catch (_) {}
          wlog(
            'add_frame seq=${msg['seq']} frameId=${r.frameId} '
            'rc=${r.result.name} ms=${sw.elapsedMilliseconds} (${w}x$h) | '
            'extract=${dbg.extractMs.toStringAsFixed(0)}ms '
            'match=${dbg.matchMs.toStringAsFixed(0)}ms '
            'cand=${dbg.nCand} gpuM=${dbg.gpuMatches} cpuM=${dbg.cpuMatches}'
            '${tel != null ? ' | mem=${tel.physFootprintMb.toStringAsFixed(0)}MB '
                      'peak=${peakMb.toStringAsFixed(0)}MB '
                      'thermal=${tel.thermalName}' : ''}',
          );
          boot.reply.send(<String, Object?>{
            'evt': 'frame_done',
            'seq': msg['seq'],
            'frameId': r.frameId,
            'ms': sw.elapsedMilliseconds,
            'result': r.result.name,
            'memMb': tel?.physFootprintMb,
            'peakMb': tel != null ? peakMb : null,
            'thermal': tel?.thermalState,
                                                   
            'extractMs': dbg.extractMs,
            'matchMs': dbg.matchMs,
            'nCand': dbg.nCand,
            'gpuPairs': dbg.gpuMatches,
            'cpuPairs': dbg.cpuMatches,
            if (growAcc >= 0) 'growAcc': growAcc,
            if (growRej >= 0) 'growRej': growRej,
            if (tvgPairs >= 0) 'tvgPairs': tvgPairs,
            if (rawPairs >= 0) 'rawPairs': rawPairs,
            if (throttledCum >= 0) 'throttledCum': throttledCum,
          });
                                             
                                                    
                                                            
          if (r.result == AetherSfmResult.ok && r.frameId >= 0) {
            fedIds.add(r.frameId);
                                                              
                                                           
                                
            {
              final q = msg['q'] as Float64List?;
              final t = msg['t'] as Float64List?;
              if (q != null && t != null) {
                final c = cameraCenterFromCamFromWorld(q, t);
                if (c != null) frameCenters[r.frameId] = c;
              }
            }
                                                                           
                                                                             
                                                                         
                                                                           
                                                                             
                                                                                
                                                                         
            try {
                                                                             
                                                                      
                                                                             
                                                                             
                                                                           
                                                                             
                                                                                
                                                                                 
                                                                               
                                                                          
              final captureLive = !finishPending && nativeCaptureActive();
              final beforeGlobal = (captureLive || boot.arEveryFrame)
                  ? session!.previewTracked()
                  : null;
                                                              
                                                               
                                               
                                                                     
                                                            
                                                          
              if (boot.arEveryFrame &&
                  beforeGlobal != null &&
                  beforeGlobal.count > 0) {
                sendSnapshot(
                  'preview',
                  <String, dynamic>{
                                                         
                                                                  
                                                       
                    'source': 'streaming_local_ba_live',
                    'terminal': false,
                    'publish_version': publishPolicy.version,
                    'n_registered': fedIds.length,
                    'n_points3d': beforeGlobal.count,
                  },
                  0,
                  preview: true,
                  prefetched: beforeGlobal,
                );
              }
              if (captureLive &&
                  beforeGlobal != null &&
                  publishPolicy.shouldRunGlobalBa(
                    registeredFrames: fedIds.length,
                    pointCount: beforeGlobal.count,
                  )) {
                final globalSw = Stopwatch()..start();
                final globalResult = session!.globalRefine();
                globalSw.stop();
                if (globalResult == AetherSfmResult.ok) {
                  final refined = session!.previewTracked();
                  final nextVersion = publishPolicy.version + 1;
                  final delivered = sendSnapshot(
                    'preview',
                    <String, dynamic>{
                      'source': 'streaming_global_ba',
                      'terminal': false,
                      'publish_version': nextVersion,
                      'n_registered': fedIds.length,
                      'n_points3d': refined.count,
                    },
                    globalSw.elapsedMilliseconds,
                    preview: true,
                  );
                  if (delivered && refined.count > 0) {
                    publishPolicy.markGlobalBaPublished(
                      registeredFrames: fedIds.length,
                      pointCount: refined.count,
                    );
                    telem('live_global_publish', {
                      'version': publishPolicy.version,
                      'registered': fedIds.length,
                      'points': refined.count,
                      'ms': globalSw.elapsedMilliseconds,
                    });
                  }
                } else {
                  wlog(
                    'live-global-ba: rc=${globalResult.name}; '
                    'stable cloud unchanged',
                  );
                }
              }
            } catch (e) {
                                                                            
                                                      
              wlog('live-global-ba failed (non-fatal): $e');
            }
            final nowMs = DateTime.now().millisecondsSinceEpoch;
            if (nowMs - lastConnectivityMs >= 1200) {
              lastConnectivityMs = nowMs;
              try {
                final csw = Stopwatch()..start();
                                                     
                                                   
                                              
                final pts = session!.previewTracked();
                final cp = pts.count > 0
                    ? connectivityPosesFrom(pts.obsFrameIds)
                    : Float64List(0);
                var conn = 0;
                for (var i = 1; i < cp.length; i += 9) {
                  if (cp[i] != 0) conn++;
                }
                if (cp.isNotEmpty) {
                  boot.reply.send(<String, Object?>{
                    'evt': 'live_poses',
                    'poses': cp,
                  });
                }
                wlog(
                  'live-poses: frames=${cp.length ~/ 9} connected=$conn '
                  'ms=${csw.elapsedMilliseconds}',
                );
                                                       
                                                        
                if (pts.count > 0 &&
                    frameCenters.length >= 2 &&
                    nowMs - lastTrueParallaxMs >= 3500) {
                  lastTrueParallaxMs = nowMs;
                  final psw = Stopwatch()..start();
                  final agg = trueParallaxAggregate(
                    xyz: pts.xyz,
                    obsOffsets: pts.obsOffsets,
                    obsFrameIds: pts.obsFrameIds,
                    centersByFrame: frameCenters,
                  );
                  psw.stop();
                  if (agg != null) {
                    boot.reply.send(<String, Object?>{
                      'evt': 'live_parallax',
                      'frames': agg.framesPacked,
                      'voxKeys': agg.voxelKeys,
                      'voxDeg': agg.voxelDeg,
                      'ms': psw.elapsedMilliseconds,
                      'sampled': agg.sampledPoints,
                    });
                  }
                  wlog(
                    'true-parallax: pts=${pts.count} '
                    'sampled=${agg?.sampledPoints ?? 0} '
                    'stride=${agg?.stride ?? 0} '
                    'frames=${(agg?.framesPacked.length ?? 0) ~/ 2} '
                    'vox=${agg?.voxelKeys.length ?? 0} '
                    'ms=${psw.elapsedMilliseconds}',
                  );
                }
              } catch (e) {
                wlog('live-poses failed (non-fatal): $e');
              }
            }
          }
        } catch (e) {
          sw.stop();
          boot.reply.send(<String, Object?>{
            'evt': 'frame_done',
            'seq': msg['seq'],
            'frameId': -1,
            'ms': sw.elapsedMilliseconds,
            'result': 'exception',
          });
          fail('add_frame', e);
        }
      case 'remove_frame':
        final requestId = msg['requestId'] as int;
        final frameId = msg['frameId'] as int;
        Map<String, dynamic>? stats;
        var removed = session == null;
        try {
          if (session != null) {
            stats = session!.removeFrame(frameId);
            removed = stats != null;
          }
          if (removed) {
            fedIds.remove(frameId);
            frameCenters.remove(frameId);
            if (session != null) {
              final tracked = session!.previewTracked();
              final poses = tracked.count > 0
                  ? connectivityPosesFrom(tracked.obsFrameIds)
                  : Float64List(0);
              boot.reply.send(<String, Object?>{
                'evt': 'live_poses',
                'poses': poses,
              });
                                                              
                                                           
                                                            
                                                      
                                                    
                                                   
                                                        
                                   
              sendSnapshot(
                'preview',
                <String, dynamic>{
                  'source': 'streaming_local_ba_live',
                  'terminal': false,
                  'n_registered': fedIds.length,
                  'n_points3d': tracked.count,
                  'frame_removed': frameId,
                },
                0,
                preview: true,
                prefetched: tracked,
              );
            }
          }
        } catch (e) {
          removed = false;
          wlog('remove-frame failed frameId=$frameId: $e');
        }
        boot.reply.send(<String, Object?>{
          'evt': 'frame_removed',
          'requestId': requestId,
          'frameId': frameId,
          'ok': removed,
          'stats': stats,
        });
      case 'prefetch':
                                                              
                                                    
                                                    
                                                   
                                         
        if (session != null) {
          try {
            final prc = session!.prefetchJpegFrame(msg['path'] as String);
            if (prc == 0) {
              wlog(
                'prefetch queued: ${(msg['path'] as String).split('/').last}',
              );
            }
          } catch (_) {}                          
        }
      case 'finish_pending':
                                                                            
                                                                         
                                                                
        finishPending = true;
      case 'repay':
                                                                           
                                                                            
                                                                           
                                                                           
                                                                             
                                                                            
                                                               
        var repaid = 0;
        if (!finishPending && session != null) {
          try {
            repaid = session!.liveRepay(maxPairs: (msg['budget'] as int?) ?? 2);
          } catch (e) {
            wlog('quad-prepay failed (non-fatal): $e');
          }
        }
        boot.reply.send(<String, Object?>{'evt': 'repay_done', 'n': repaid});
      case 'resume':
                                                                             
                                                                                
                                                                              
                                                                                
                                                                             
                                                                            
                                           
        if (session == null) {
          try {
                                                          
                                                                      
                                                         
                                                               
                                                                         
                                                       
                                       
            session = AetherSfmStreamSession.create(
              boot.dbPath,
              imageWidth: (msg['w'] as int?) ?? 3840,
              imageHeight: (msg['h'] as int?) ?? 2160,
              maxFeatures: AetherSfmStreamSession.researchMaxFeatures,
              kNeighbors: AetherSfmStreamSession.researchKNeighbors,
            );
            wlog('resume: session opened on existing db (${boot.dbPath})');
          } catch (e) {
            fail('resume', 'create-from-db failed: $e');
            break;
          }
        }
        continue finalizeCase;
      finalizeCase:
      case 'finalize':
        final s = session;
        if (s == null) {
          fail('finalize', 'no frames were fed — nothing to reconstruct');
          break;
        }
                                                           
                                                        
                                                       
                                                       
        wlog(
          'finalize requested: queue already drained; phase-1 reuses the live '
          'recon (S3.5), refined is delivered the moment it lands',
        );
        final sw = Stopwatch()..start();
        try {
                                                                        
                                                                        
                                                            
          final telBefore = PwTelemetry.sample();
          if (telBefore != null && telBefore.physFootprintMb > peakMb) {
            peakMb = telBefore.physFootprintMb;
          }
          wlog(
            'finalize_async phase-1 starting…'
            '${telBefore != null ? ' | $telBefore' : ''}',
          );
                                                                               
                                                                            
                                                                 
                                                                                 
                                                                               
                                                                                   
                                                              
          try {
            final p1 = s.previewTracked();
            if (p1.count > 0) {
              sendSnapshot(
                'preview',
                <String, dynamic>{
                  'source': 'finalize_local_live',
                  'terminal': false,
                  'publish_version': publishPolicy.version,
                  'n_registered': fedIds.length,
                  'n_points3d': p1.count,
                },
                0,
                preview: true,
                prefetched: p1,
              );
            } else {
              wlog('pre-finalize preview skipped: live recon empty');
            }
          } catch (e) {
            wlog('pre-finalize preview failed (non-fatal): $e');
          }
          final summary = s.finalizeAsync();
          sw.stop();
          final telAfter = PwTelemetry.sample();
          if (telAfter != null && telAfter.physFootprintMb > peakMb) {
            peakMb = telAfter.physFootprintMb;
          }
          wlog(
            'finalize_async phase-1 done in ${sw.elapsedMilliseconds}ms'
            '${telAfter != null ? ' | ${telAfter.physFootprintMb.toStringAsFixed(0)}MB '
                      'peak=${peakMb.toStringAsFixed(0)}MB '
                      'thermal=${telAfter.thermalName}' : ''}',
          );
                                                                  
                                                           
                                                         
                                              
          telem('finalize_phase1', {
            'wall_ms': sw.elapsedMilliseconds,
            'solve_ms': summary['solve_ms'],
            'n_registered': summary['n_registered'],
            'n_points3d': summary['n_points3d'],
            'reproj_px': summary['reproj_px'],
            'track_len': summary['track_len'],
            'rc': summary['rc'],
            'result': summary['result'],
            if (telAfter != null) 'mem_mb': telAfter.physFootprintMb.round(),
            if (telAfter != null) 'peak_mb': peakMb.round(),
            if (telAfter != null) 'thermal': telAfter.thermalState,
            'loss_local': 'cauchy@1.0',
            'liter': 15,
            'lnum': 6,
            'defer_global_ba': true,
                                                                
                                                        
            'phase1': summary['phase1'] ?? 'db_rerun',
            'source': 'config_echo',
          });
          final st = s.streamStats();
          final tvgPct = (st.tvgPairs + st.rawPairs) > 0
              ? (100 * st.tvgPairs / (st.tvgPairs + st.rawPairs)).round()
              : 0;
          wlog(
            'stream-stats: tvg-inlier pairs=${st.tvgPairs} '
            'raw-fallback=${st.rawPairs} ($tvgPct% verified) | '
            'grow accept=${st.growAccepted} reject=${st.growRejected} | '
            'filtered reproj=${st.reprojFiltered} tri-angle=${st.triFiltered}',
          );
          wlog(
            'stream-gates: grow reject cheirality=${st.growRejectCheirality} '
            'reproj=${st.growRejectReproj} | create reject '
            'cheirality=${st.createRejectCheirality} '
            'tri-angle=${st.createRejectTriAngle} '
            'reproj=${st.createRejectReproj} | assigned same='
            '${st.alreadyAssigned} merge-needed=${st.mergeNeeded} '
            'accepted=${st.mergeAccepted} rejected=${st.mergeRejected} | '
            'spatial considered=${st.spatialConsidered} '
            'attempted=${st.spatialAttempted} written=${st.spatialWritten} '
            'inliers=${st.spatialInliers}',
          );
          wlog(
            'spatial-loop: anchors=${st.spatialAnchorPassed}/'
            '${st.spatialAnchorAttempted} '
            'regions=${st.spatialRegionsConfirmed} expand-attempted='
            '${st.spatialExpandedAttempted} guided=${st.spatialGuidedPairs}/'
            '${st.spatialGuidedInliers}candidates quadratic='
            '${st.spatialQuadraticWritten}/'
            '${st.spatialQuadraticAttempted} budget-skipped='
            '${st.spatialBudgetSkipped}',
          );
          if (summary['result'] != 'ok') {
                                                                            
                                                                           
                                                                           
                                                                     
            try {
              final db = File(boot.dbPath);
              if (db.existsSync()) {
                db.copySync('${boot.dbPath}.debug');
                wlog(
                  'debug: db preserved at ${boot.dbPath}.debug '
                  '(${db.lengthSync()} bytes)',
                );
              }
            } catch (e) {
              wlog('debug: db preserve failed: $e');
            }
            fail(
              'finalize',
              '${summary['result']} (rc=${summary['rc']}) $summary',
            );
            break;
          }
          wlog(
            'phase-1 done (${summary['phase1'] ?? 'db_rerun'}); refined snapshot '
            'will be delivered immediately on completion',
          );
                                                                      
                                                                    
                                               
                             
          refineStart = DateTime.now().millisecondsSinceEpoch;
                                                               
                                                         
                                   
          ({int stage, int round, int iter, int maxIter})? lastProgress;
          pollTimer = Timer.periodic(const Duration(milliseconds: 250), (t) {
            final st = s.finalizeStatus();
                                                                  
                                                
            if (s.hasFinalizeProgress) {
              final pg = s.finalizeProgress();
              if (pg != null && pg != lastProgress) {
                lastProgress = pg;
                boot.reply.send(<String, Object?>{
                  'evt': 'finalize_progress',
                  'stage': pg.stage,
                  'round': pg.round,
                  'iter': pg.iter,
                  'max_iter': pg.maxIter,
                });
              }
            }
            if (st == AetherSfmFinalizeStatus.refined) {
              t.cancel();
              final ms = DateTime.now().millisecondsSinceEpoch - refineStart;
                                                           
                                                                       
                                                                   
                                                                                
                                                      
                                                                          
                                                         
              Map<String, dynamic>? segs;
              try {
                final segFile = File(
                  '${File(boot.dbPath).parent.path}/official_finalize_segments.json',
                );
                if (segFile.existsSync()) {
                  segs =
                      jsonDecode(segFile.readAsStringSync())
                          as Map<String, dynamic>;
                }
              } catch (e) {
                wlog('finalize segments read failed (non-fatal): $e');
              }
              final telP2 = PwTelemetry.sample();
              if (segs != null) {
                telem('finalize_segments', {
                  ...segs,
                  if (telP2 != null) 'thermal': telP2.thermalState,
                  if (telP2 != null) 'mem_mb': telP2.physFootprintMb.round(),
                });
              } else {
                wlog(
                  'official_finalize_segments.json missing — phase 2 likely took the '
                  'db-rerun fallback (segments telemetry unavailable)',
                );
              }
                                                         
                                                                          
                                                              
                                                               
                                                      
                                                          
                                                               
                              
              telem('finalize_phase2', {
                'wall_ms': ms,
                'loss_global': 'cauchy@1.0',
                'solver': 'dense_schur_override',
                'source': 'native_segments',
                if (telP2 != null) 'mem_mb': telP2.physFootprintMb.round(),
                if (telP2 != null) 'thermal': telP2.thermalState,
                if (segs != null) ...{
                  'cache_pre_ms': segs['cache_pre_ms'],
                  'enrich_ms': segs['enrich_ms'],
                  'stage1_ms': segs['stage1_ms'],
                  'stage1_rounds': segs['stage1_rounds'],
                  'stage1_state': segs['stage1_state'],
                  'stage2_ms': segs['stage2_ms'],
                  'stage2_rounds_budget': segs['stage2_rounds_budget'],
                  'temporal_ms': segs['temporal_ms'],
                  'total_native_ms': segs['total_ms'],
                  'solver_used': segs['solver_used'],
                  'sparse_backend': segs['sparse_backend'],
                                                          
                                                          
                  'gftol': segs['gftol_used'],
                  'gref': segs['gref_used'],
                  'giter': segs['giter_used'],
                  'ftol_ab_arm': segs['ftol_ab_arm'],
                  'mixed': segs['mixed'],
                  'threads': segs['threads'],
                },
              });
                                                               
                                                                  
                                                     
              try {
                final mf = s.matchFailStats();
                telem('match_fail_stats', {
                  'gpu_fail_total': mf.gpuFailTotal,
                  'gpu_fail_by_rc': mf.gpuFailByRc,
                  'gpu_fail_max_streak': mf.gpuFailMaxStreak,
                  'rematch_starved_frames': mf.rematchStarvedFrames,
                  'rematch_candidates': mf.rematchCandidates,
                  'rematch_attempted': mf.rematchAttempted,
                  'rematch_written': mf.rematchWritten,
                  'rematch_inliers': mf.rematchInliers,
                  'rematch_failed': mf.rematchFailed,
                });
              } catch (e) {
                wlog('match_fail_stats telemetry failed (non-fatal): $e');
              }
                                                         
                                                                     
                                                             
                                                            
                                                           
              try {
                final rs = s.repairStats();
                telem('repair_stats', {
                  'repay_calls': rs.repayCalls,
                  'repay_attempted': rs.repayAttempted,
                  'repay_written': rs.repayWritten,
                  'repay_inliers': rs.repayInliers,
                  'repay_failed': rs.repayFailed,
                  'repay_skipped_thermal': rs.repaySkippedThermal,
                  'gpu_retry_attempts': rs.gpuRetryAttempts,
                  'gpu_retry_recovered': rs.gpuRetryRecovered,
                  'enrich_budget_stopped': rs.enrichBudgetStopped,
                });
              } catch (e) {
                wlog('repair_stats telemetry failed (non-fatal): $e');
              }
              sendSnapshot('refined', summary, ms);
                                                   
                                    
              try {
                final g = _triAngleTelemetry(
                  s.pointsTracked(),
                  s.posesPacked(),
                );
                if (g != null) {
                  telem('geom', {
                    ...g,
                    'reproj_px': summary['reproj_px'],
                    'track_len': summary['track_len'],
                  });
                }
              } catch (e) {
                wlog('geom telemetry failed (non-fatal): $e');
              }
            } else if (st == AetherSfmFinalizeStatus.error) {
              t.cancel();
              fail('refine', 'background global BA failed');
            }
          });
        } catch (e) {
          sw.stop();
          fail('finalize', e);
        }
      case 'dispose':
        disposed = true;
        pollTimer?.cancel();
        wlog('dispose: freeing session (joins bg BA thread)…');
        try {
          session?.dispose();                                  
        } catch (_) {}
        session = null;
        boot.reply.send(const <String, Object?>{'evt': 'disposed'});
        cmds.close();
    }
  });
}

                                
   
                                                        
                                                          
                                          
                                                
   
                                             
                                                        
                                        
Map<String, Object?>? _triAngleTelemetry(
  AetherSfmPointsTracked points,
  Float64List poses,
) {
  final sw = Stopwatch()..start();
  final n = points.count;
  final offs = points.obsOffsets;
  final fids = points.obsFrameIds;
  if (n == 0 || poses.isEmpty || offs.length != n + 1) return null;

                                          
  final centers = <int, List<double>>{};
  for (var i = 0; i + 8 < poses.length; i += 9) {
    if (poses[i + 1] == 0) continue;       
    final qw = poses[i + 2], qx = poses[i + 3];
    final qy = poses[i + 4], qz = poses[i + 5];
    final norm2 = qw * qw + qx * qx + qy * qy + qz * qz;
    if (norm2 < 1e-12) continue;                       
    final s = 1.0 / norm2;
    final tx = poses[i + 6], ty = poses[i + 7], tz = poses[i + 8];
    final r00 = 1 - 2 * s * (qy * qy + qz * qz);
    final r01 = 2 * s * (qx * qy - qz * qw);
    final r02 = 2 * s * (qx * qz + qy * qw);
    final r10 = 2 * s * (qx * qy + qz * qw);
    final r11 = 1 - 2 * s * (qx * qx + qz * qz);
    final r12 = 2 * s * (qy * qz - qx * qw);
    final r20 = 2 * s * (qx * qz - qy * qw);
    final r21 = 2 * s * (qy * qz + qx * qw);
    final r22 = 1 - 2 * s * (qx * qx + qy * qy);
    centers[poses[i].toInt()] = [
      -(r00 * tx + r10 * ty + r20 * tz),
      -(r01 * tx + r11 * ty + r21 * tz),
      -(r02 * tx + r12 * ty + r22 * tz),
    ];
  }
  if (centers.length < 2) return null;

  const maxSample = 10000;                      
  const maxObsPerPoint = 6;                   
  final stride = n <= maxSample ? 1 : (n / maxSample).ceil();
  final angles = <double>[];
  var lt8 = 0;
  final cams = <List<double>>[];
  for (var i = 0; i < n; i += stride) {
    cams.clear();
    for (
      var j = offs[i];
      j < offs[i + 1] && cams.length < maxObsPerPoint;
      j++
    ) {
      final c = centers[fids[j]];
      if (c != null) cams.add(c);
    }
    if (cams.length < 2) continue;
    final px = points.xyz[i * 3];
    final py = points.xyz[i * 3 + 1];
    final pz = points.xyz[i * 3 + 2];
    var best = 0.0;
    for (var a = 0; a < cams.length; a++) {
      final ax = cams[a][0] - px, ay = cams[a][1] - py, az = cams[a][2] - pz;
      final an = math.sqrt(ax * ax + ay * ay + az * az);
      if (an < 1e-12) continue;
      for (var b = a + 1; b < cams.length; b++) {
        final bx = cams[b][0] - px, by = cams[b][1] - py, bz = cams[b][2] - pz;
        final bn = math.sqrt(bx * bx + by * by + bz * bz);
        if (bn < 1e-12) continue;
        final cosAng = ((ax * bx + ay * by + az * bz) / (an * bn)).clamp(
          -1.0,
          1.0,
        );
        final ang = math.acos(cosAng);
        if (ang > best) best = ang;
      }
    }
    final deg = best * 180.0 / math.pi;
    angles.add(deg);
    if (deg < 8.0) lt8++;
  }
  if (angles.isEmpty) return null;
  angles.sort();
  sw.stop();
  double r2(double? v) => v == null ? 0 : (v * 100).round() / 100;
  return <String, Object?>{
    'n_pts': n,
    'sampled': angles.length,
    'stride': stride,
    'tri_p10': r2(percentileSorted(angles, 0.10)),
    'tri_p50': r2(percentileSorted(angles, 0.50)),
    'tri_p90': r2(percentileSorted(angles, 0.90)),
    'tri_lt8_pct': (1000.0 * lt8 / angles.length).round() / 10,
    'ms': sw.elapsedMilliseconds,
  };
}
