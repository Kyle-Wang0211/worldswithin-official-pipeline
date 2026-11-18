                                                     
  
                                                             
                                                 
                                        
                                               
                                                             
  
                                            
                                                         
                              
                              
                                            
                                         
                                               
                               

import 'dart:math' as math;
import 'dart:typed_data';

                                         
   
                                                       
                                                         
                                        
                                                                     
                  
   
                                               
                                         
              
class GravityAlignDiagV1 {
                  
  int registeredFrames = 0;

                                                      
  int framesWithArkitQuat = 0;

                                           
  int requiredFrames = 3;

                              
  String? skipReason;

                        
  static const String reasonNoPoses = 'no_poses';

                                                   
  static const String reasonNotEnoughArkitQuats = 'not_enough_arkit_quats';

                                        
  static const String reasonDegenerateAverage = 'degenerate_average';

  Map<String, Object?> toTelemetry() => <String, Object?>{
    'registered_frames': registeredFrames,
    'frames_with_arkit_quat': framesWithArkitQuat,
    'required_frames': requiredFrames,
    'skip_reason': skipReason,
  };
}

                                            
   
                                                            
                                                                     
                                         
   
                                        
                                                                 
                                              
        
class ScaleAnchorDiagV1 {
                  
  int registeredFrames = 0;

                                          
  int pairsWithArkitCenter = 0;

                                            
  int usableRatios = 0;

                                       
  int requiredPairs = 3;

                                                                       
                                              
  double? rejectedFactor;

                  
  String? skipReason;

  static const String reasonNoPoses = 'no_poses';

                                             
  static const String reasonNotEnoughPairs = 'not_enough_pairs';

                                  
  static const String reasonNotEnoughRatios = 'not_enough_ratios';

                 
  static const String reasonNonFiniteScale = 'non_finite_scale';

                                            
  static const String reasonOutOfBand = 'scale_out_of_band';

  Map<String, Object?> toTelemetry() => <String, Object?>{
    'registered_frames': registeredFrames,
    'pairs_with_arkit_center': pairsWithArkitCenter,
    'usable_ratios': usableRatios,
    'required_pairs': requiredPairs,
    'rejected_factor': rejectedFactor,
    'skip_reason': skipReason,
  };
}

                                           
   
                                                             
                                                               
                                                           
                              
   
                                              
                                            
                                           
                         
Float32List? gravityAlignedPoints({
  required Float32List xyz,
  required Float64List posesPacked,
  required List<double>? Function(int frameId) arkitQuatWxyzOf,
}) {
  if (xyz.isEmpty) return null;
  final q = gravityAlignQuatWxyz(
    posesPacked: posesPacked,
    arkitQuatWxyzOf: arkitQuatWxyzOf,
  );
  if (q == null) return null;
  return rotatePointsByQuatWxyz(xyz, q);
}

                                                                 
                                                   
                                      
                                                       
                                                    
                                          
List<double>? gravityAlignQuatWxyz({
  required Float64List posesPacked,
  required List<double>? Function(int frameId) arkitQuatWxyzOf,
  GravityAlignDiagV1? diag,
}) {
  final poses = posesPacked;
  if (poses.isEmpty) {
    diag?.skipReason = GravityAlignDiagV1.reasonNoPoses;
    return null;
  }

                                    
  List<double> qmul(List<double> a, List<double> b) => [
    a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
    a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
    a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
    a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0],
  ];

  var aw = 0.0, ax = 0.0, ay = 0.0, az = 0.0;
  List<double>? ref;
  var cnt = 0;
  for (var i = 0; i < poses.length; i += 9) {
    if (poses[i + 1] == 0) continue;                
    if (diag != null) diag.registeredFrames++;
    final aq = arkitQuatWxyzOf(poses[i].toInt());
    if (aq == null || aq.length != 4) continue;
    final qCol = [poses[i + 2], poses[i + 3], poses[i + 4], poses[i + 5]];
    final qArkConj = [aq[0], -aq[1], -aq[2], -aq[3]];           
                                                                        
                                                                        
                                                                       
                                                                         
    const qC = [0.0, 1.0, 0.0, 0.0];
    var qw = qmul(qArkConj, qmul(qC, qCol));                             
    final norm = math.sqrt(
      qw[0] * qw[0] + qw[1] * qw[1] + qw[2] * qw[2] + qw[3] * qw[3],
    );
    if (norm < 1e-9) continue;
    qw = [qw[0] / norm, qw[1] / norm, qw[2] / norm, qw[3] / norm];
    ref ??= qw;
                                                             
    final dot =
        qw[0] * ref[0] + qw[1] * ref[1] + qw[2] * ref[2] + qw[3] * ref[3];
    final s = dot < 0 ? -1.0 : 1.0;
    aw += s * qw[0];
    ax += s * qw[1];
    ay += s * qw[2];
    az += s * qw[3];
    cnt++;
  }
                                                              
                                           
                                        
  if (diag != null) diag.framesWithArkitQuat = cnt;
  if (cnt < 3) {
    diag?.skipReason = GravityAlignDiagV1.reasonNotEnoughArkitQuats;
    return null;                                               
  }

  final an = math.sqrt(aw * aw + ax * ax + ay * ay + az * az);
  if (an < 1e-9) {
    diag?.skipReason = GravityAlignDiagV1.reasonDegenerateAverage;
    return null;
  }
  return [aw / an, ax / an, ay / an, az / an];
}

                                                        
                                                      
                                                     
                                                         
                                      
                                                       
   
                                                                      
                                             
                                                
                                                    
                                      
   
                                        
                                               
                                               
double? scaleAnchorFactor({
  required Float64List posesPacked,
  required List<double>? Function(int frameId) arkitCenterWorldOf,
  ScaleAnchorDiagV1? diag,
}) {
  final poses = posesPacked;
  if (poses.isEmpty) {
    diag?.skipReason = ScaleAnchorDiagV1.reasonNoPoses;
    return null;
  }

  final baC = <List<double>>[];
  final arkC = <List<double>>[];
  for (var i = 0; i < poses.length; i += 9) {
    if (poses[i + 1] == 0) continue;                
    if (diag != null) diag.registeredFrames++;
    final ac = arkitCenterWorldOf(poses[i].toInt());
    if (ac == null || ac.length != 3) continue;
    final w = poses[i + 2],
        x = poses[i + 3],
        y = poses[i + 4],
        z = poses[i + 5];
    final n2 = w * w + x * x + y * y + z * z;
    if (n2 < 1e-12) continue;                                          
                                                                         
    final tx = poses[i + 6], ty = poses[i + 7], tz = poses[i + 8];
                                                                            
    final r00 = 1 - 2 * (y * y + z * z) / n2,
        r01 = 2 * (x * y - z * w) / n2,
        r02 = 2 * (x * z + y * w) / n2;
    final r10 = 2 * (x * y + z * w) / n2,
        r11 = 1 - 2 * (x * x + z * z) / n2,
        r12 = 2 * (y * z - x * w) / n2;
    final r20 = 2 * (x * z - y * w) / n2,
        r21 = 2 * (y * z + x * w) / n2,
        r22 = 1 - 2 * (x * x + y * y) / n2;
    baC.add([
      -(r00 * tx + r10 * ty + r20 * tz),
      -(r01 * tx + r11 * ty + r21 * tz),
      -(r02 * tx + r12 * ty + r22 * tz),
    ]);
    arkC.add(ac);
  }
                                                                 
                                    
  if (diag != null) diag.pairsWithArkitCenter = baC.length;
  if (baC.length < 3) {
    diag?.skipReason = ScaleAnchorDiagV1.reasonNotEnoughPairs;
    return null;
  }

  List<double> centroid(List<List<double>> pts) {
    var cx = 0.0, cy = 0.0, cz = 0.0;
    for (final p in pts) {
      cx += p[0];
      cy += p[1];
      cz += p[2];
    }
    final n = pts.length.toDouble();
    return [cx / n, cy / n, cz / n];
  }

  final cb = centroid(baC), ca = centroid(arkC);
  final ratios = <double>[];
  for (var i = 0; i < baC.length; i++) {
    final db = math.sqrt(
      math.pow(baC[i][0] - cb[0], 2) +
          math.pow(baC[i][1] - cb[1], 2) +
          math.pow(baC[i][2] - cb[2], 2),
    );
    final da = math.sqrt(
      math.pow(arkC[i][0] - ca[0], 2) +
          math.pow(arkC[i][1] - ca[1], 2) +
          math.pow(arkC[i][2] - ca[2], 2),
    );
    if (db > 1e-6 && da.isFinite && da > 0) ratios.add(da / db);
  }
  if (diag != null) diag.usableRatios = ratios.length;
  if (ratios.length < 3) {
    diag?.skipReason = ScaleAnchorDiagV1.reasonNotEnoughRatios;
    return null;
  }
  ratios.sort();
  final s = ratios[ratios.length ~/ 2];
  if (!s.isFinite || s <= 0) {
    diag
      ?..rejectedFactor = s
      ..skipReason = ScaleAnchorDiagV1.reasonNonFiniteScale;
    return null;
  }
  if ((s - 1.0).abs() > 0.15) {
                                                       
                                              
    diag
      ?..rejectedFactor = s
      ..skipReason = ScaleAnchorDiagV1.reasonOutOfBand;
    return null;
  }
  return s;
}

                                                           
                                             
Float64List scaleAnchoredPosesPacked(Float64List posesPacked, double s) {
  final out = Float64List.fromList(posesPacked);
  for (var i = 0; i < out.length; i += 9) {
    if (out[i + 1] == 0) continue;
    out[i + 6] *= s;
    out[i + 7] *= s;
    out[i + 8] *= s;
  }
  return out;
}

                                      
Float32List scaleAnchoredPoints(Float32List xyz, double s) {
  final out = Float32List(xyz.length);
  for (var i = 0; i < xyz.length; i++) {
    out[i] = xyz[i] * s;
  }
  return out;
}

                                               
                                            
Float32List rotatePointsByQuatWxyz(Float32List xyz, List<double> q) {
  final w = q[0], x = q[1], y = q[2], z = q[3];
                                           
  final r00 = 1 - 2 * (y * y + z * z),
      r01 = 2 * (x * y - z * w),
      r02 = 2 * (x * z + y * w);
  final r10 = 2 * (x * y + z * w),
      r11 = 1 - 2 * (x * x + z * z),
      r12 = 2 * (y * z - x * w);
  final r20 = 2 * (x * z - y * w),
      r21 = 2 * (y * z + x * w),
      r22 = 1 - 2 * (x * x + y * y);

  final src = xyz;
  final out = Float32List(src.length);
  for (var i = 0; i < src.length; i += 3) {
    final px = src[i], py = src[i + 1], pz = src[i + 2];
    out[i] = r00 * px + r01 * py + r02 * pz;
    out[i + 1] = r10 * px + r11 * py + r12 * pz;
    out[i + 2] = r20 * px + r21 * py + r22 * pz;
  }
  return out;
}

                                                           
                                                  
                                                               
                                  
                                               
                                                  
Float64List gravityAlignedPosesPacked(
  Float64List posesPacked,
  List<double> qAlign,
) {
  final out = Float64List.fromList(posesPacked);
  final cw = qAlign[0], cx = -qAlign[1], cy = -qAlign[2], cz = -qAlign[3];
  for (var i = 0; i < out.length; i += 9) {
    if (out[i + 1] == 0) continue;                                
    final w = out[i + 2], x = out[i + 3], y = out[i + 4], z = out[i + 5];
                                        
    out[i + 2] = w * cw - x * cx - y * cy - z * cz;
    out[i + 3] = w * cx + x * cw + y * cz - z * cy;
    out[i + 4] = w * cy - x * cz + y * cw + z * cx;
    out[i + 5] = w * cz + x * cy - y * cx + z * cw;
                                                                            
                                                                 
  }
  return out;
}
