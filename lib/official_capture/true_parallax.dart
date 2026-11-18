                                                         
  
                                                  
                                                   
                                                    
                                      
                                 
  
                                                            
                                             
                                                            
                                                              
                                                   
  
             
                                                 
                                            
                                                            
                        
  
                                                      
                                                                 
                         

import 'dart:math' as math;
import 'dart:typed_data';

import 'photo_card_state.dart' show medianOf;

                                                    
                                           
const double kCoverageVoxelSizeM = 0.04;

                                              
                                                  
   
                                            
                                                      
                          
const double kCaptureParallaxMinDeg = 5.0;

                                                        
                                                     
          
int coverageVoxelKeyFor(double x, double y, double z, double voxelSizeM) {
  final xi = (x / voxelSizeM).floor();
  final yi = (y / voxelSizeM).floor();
  final zi = (z / voxelSizeM).floor();
  return ((xi + 0x100000) << 42) | ((yi + 0x100000) << 21) | (zi + 0x100000);
}

                                                              
                                     
                                                  
List<double>? cameraCenterFromCamFromWorld(
  List<double> quatWxyz,
  List<double> t,
) {
  if (quatWxyz.length < 4 || t.length < 3) return null;
  final qw = quatWxyz[0], qx = quatWxyz[1], qy = quatWxyz[2], qz = quatWxyz[3];
  final norm2 = qw * qw + qx * qx + qy * qy + qz * qz;
  if (norm2 < 1e-12) return null;
  final s = 1.0 / norm2;
  final tx = t[0], ty = t[1], tz = t[2];
  final r00 = 1 - 2 * s * (qy * qy + qz * qz);
  final r01 = 2 * s * (qx * qy - qz * qw);
  final r02 = 2 * s * (qx * qz + qy * qw);
  final r10 = 2 * s * (qx * qy + qz * qw);
  final r11 = 1 - 2 * s * (qx * qx + qz * qz);
  final r12 = 2 * s * (qy * qz - qx * qw);
  final r20 = 2 * s * (qx * qz - qy * qw);
  final r21 = 2 * s * (qy * qz + qx * qw);
  final r22 = 1 - 2 * s * (qx * qx + qy * qy);
  return [
    -(r00 * tx + r10 * ty + r20 * tz),
    -(r01 * tx + r11 * ty + r21 * tz),
    -(r02 * tx + r12 * ty + r22 * tz),
  ];
}

                                                          
                                       
class TrueParallaxAggregate {
  const TrueParallaxAggregate({
    required this.framesPacked,
    required this.voxelKeys,
    required this.voxelDeg,
    required this.sampledPoints,
    required this.stride,
  });

                                              
                              
  final Float64List framesPacked;

                                                                 
                                  
  final Int64List voxelKeys;
  final Float32List voxelDeg;

                                
  final int sampledPoints;
  final int stride;
}

                                                   
   
                                                  
                                         
                                                      
                                                
   
                                      
                                        
                                          
                
TrueParallaxAggregate? trueParallaxAggregate({
  required Float32List xyz,
  required Int32List obsOffsets,
  required Int32List obsFrameIds,
  required Map<int, List<double>> centersByFrame,
  double voxelSizeM = kCoverageVoxelSizeM,
  int maxSample = 10000,
  int maxObsPerPoint = 6,
}) {
  final n = xyz.length ~/ 3;
  if (n == 0 || obsOffsets.length != n + 1 || centersByFrame.length < 2) {
    return null;
  }
  final stride = n <= maxSample ? 1 : (n / maxSample).ceil();
  final frameAngles = <int, List<double>>{};
  final voxelAngles = <int, List<double>>{};
  var sampled = 0;
  final cams = <List<double>>[];
  final obsFids = <int>[];
  for (var i = 0; i < n; i += stride) {
    final start = obsOffsets[i];
    final end = obsOffsets[i + 1];
    if (start < 0 || end < start || end > obsFrameIds.length) return null;
    cams.clear();
    obsFids.clear();
    for (var j = start; j < end; j++) {
      final fid = obsFrameIds[j];
      final c = centersByFrame[fid];
      if (c == null) continue;                           
      obsFids.add(fid);
      if (cams.length < maxObsPerPoint) cams.add(c);
    }
    if (cams.length < 2) continue;
    final px = xyz[i * 3], py = xyz[i * 3 + 1], pz = xyz[i * 3 + 2];
    var best = 0.0;
    var pairs = 0;
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
        pairs++;
        if (ang > best) best = ang;
      }
    }
    if (pairs == 0) continue;                         
    final deg = best * 180.0 / math.pi;
    sampled++;
    for (final fid in obsFids) {
      frameAngles.putIfAbsent(fid, () => <double>[]).add(deg);
    }
    voxelAngles
        .putIfAbsent(
          coverageVoxelKeyFor(px, py, pz, voxelSizeM),
          () => <double>[],
        )
        .add(deg);
  }
  if (sampled == 0) return null;

  final fids = frameAngles.keys.toList()..sort();
  final framesPacked = Float64List(fids.length * 2);
  for (var k = 0; k < fids.length; k++) {
    framesPacked[k * 2] = fids[k].toDouble();
    framesPacked[k * 2 + 1] = medianOf(frameAngles[fids[k]]!)!;
  }
  final vkeys = voxelAngles.keys.toList();
  final voxelKeys = Int64List(vkeys.length);
  final voxelDeg = Float32List(vkeys.length);
  for (var k = 0; k < vkeys.length; k++) {
    voxelKeys[k] = vkeys[k];
    voxelDeg[k] = medianOf(voxelAngles[vkeys[k]]!)!;
  }
  return TrueParallaxAggregate(
    framesPacked: framesPacked,
    voxelKeys: voxelKeys,
    voxelDeg: voxelDeg,
    sampledPoints: sampled,
    stride: stride,
  );
}
