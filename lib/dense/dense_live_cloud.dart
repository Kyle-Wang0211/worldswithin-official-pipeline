                                                                                 
  
                                                                                                              
                                                                                                               
                                                                                                         
                                                     
  
                                                                           
                                                   
                                                                                                              
                                                                                                             
                                                                                                     
  
                                                                                                          
import 'dart:math' as math;
import 'dart:typed_data';

import '../point_cloud_display/progressive_octree_order.dart'
    show ReviewPointCloudPolicy;

class DenseLiveCloud {
  DenseLiveCloud({
    int framesPlanned = 1,
    this.budget = ReviewPointCloudPolicy.kPointBudget,
  }) : framesPlanned = framesPlanned < 1 ? 1 : framesPlanned;

                                                                                                            
                                                                             
  final int framesPlanned;

                                                                                                         
  final int budget;

  Float32List _xyzBuf = Float32List(0);
  Uint8List _rgbBuf = Uint8List(0);
  int _kept = 0;

  Float32List _xyz = Float32List(0);
  Uint8List _rgb = Uint8List(0);

  int _sourcePoints = 0;
  int _frames = 0;
  int _lastFrameIndex = -1;

                                                                                                              
                                         
  Float32List get xyz => _xyz;

                                                                                                               
  Uint8List get rgb => _rgb;

                                                                          
  int get points => _kept;

                                                                                                
  int get sourcePoints => _sourcePoints;

                                                             
  int get frames => _frames;

                                                               
  int get lastFrameIndex => _lastFrameIndex;

  bool get isEmpty => _kept == 0;

                                                                                                       
  int strideFor(int chunkPoints) {
    if (chunkPoints <= 0) return 1;
    final k = (framesPlanned * chunkPoints / budget).ceil();
    return k < 1 ? 1 : k;
  }

                                                                                                                
                                                                                            
  void addChunk({
    required int frameIndex,
    required Float32List xyz,
    required Uint8List rgb,
  }) {
    final n = math.min(xyz.length ~/ 3, rgb.length ~/ 3);
    _frames++;
    _lastFrameIndex = frameIndex;
    _sourcePoints += n;
    if (n > 0) {
      final k = strideFor(n);
      var take = (n + k - 1) ~/ k;               
      final room = budget - _kept;
      if (take > room) take = room;
      if (take > 0) {
        _ensureCapacity(_kept + take);
        for (var i = 0; i < take; i++) {
          final src = i * k * 3;
          final dst = (_kept + i) * 3;
          _xyzBuf[dst] = xyz[src];
          _xyzBuf[dst + 1] = xyz[src + 1];
          _xyzBuf[dst + 2] = xyz[src + 2];
          _rgbBuf[dst] = rgb[src];
          _rgbBuf[dst + 1] = rgb[src + 1];
          _rgbBuf[dst + 2] = rgb[src + 2];
        }
        _kept += take;
      }
    }
    _publish();
  }

                                                                                                               
                                               
  void _publish() {
    _xyz = _xyzBuf.sublist(0, _kept * 3);
    _rgb = _rgbBuf.sublist(0, _kept * 3);
  }

  void _ensureCapacity(int points) {
    if (_xyzBuf.length >= points * 3) return;
    var cap = _xyzBuf.length ~/ 3;
    if (cap < 1024) cap = 1024;
    while (cap < points) {
      cap *= 2;
    }
    if (cap > budget)
      cap =
          budget;                                                                     
    if (cap < points) cap = points;
    final xyzBuf = Float32List(cap * 3);
    xyzBuf.setRange(0, _kept * 3, _xyzBuf);
    final rgbBuf = Uint8List(cap * 3);
    rgbBuf.setRange(0, _kept * 3, _rgbBuf);
    _xyzBuf = xyzBuf;
    _rgbBuf = rgbBuf;
  }
}
