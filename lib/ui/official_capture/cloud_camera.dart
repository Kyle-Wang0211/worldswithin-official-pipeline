                                     
  
                                                        
                                               
                                                       
                                                  
                                            
import 'dart:math' as math;
import 'dart:ui' show Size;

class CloudCamera {
  const CloudCamera({
    required this.yaw,
    required this.pitch,
    required this.zoom,
    required this.panX,
    required this.panY,
    required this.pivotX,
    required this.pivotY,
    required this.pivotZ,
    required this.radius,
    this.fillK = kFitFillK,                       
    this.orthographic = false,
    this.roll = 0,
    this.camDistOverride,
    this.orthoMix,
  });

                                                                   
                                                                          
                                                                           
                                                                                
                                                                             
                                                                  
  final double? camDistOverride;


                                                                          
                                                                     
                                                                        
                                                             
                                                    
                                                                          
                                                           
  final double? orthoMix;

                                                  
                                            
                                             
          
  final double roll;

                                                
                                           
                                         
                                           
                                 
                               
  final bool orthographic;

  final double yaw, pitch, zoom, panX, panY;
  final double pivotX, pivotY, pivotZ;

                                                    
  final double radius;
  final double fillK;

  CloudProjection projectionFor(Size size) {
    final half = size.shortestSide * 0.5;
    return CloudProjection._(
      cosY: math.cos(yaw),
      sinY: math.sin(yaw),
      cosP: math.cos(pitch),
      sinP: math.sin(pitch),
      f: half * fillK * zoom,
      camDist: camDistOverride ?? radius * kCamDistK,
      ox: size.width * 0.5 + panX,
      oy: size.height * 0.5 + panY,
      pivotX: pivotX,
      pivotY: pivotY,
      pivotZ: pivotZ,
      orthographic: orthographic,
      cosR: math.cos(roll),
      sinR: math.sin(roll),
      orthoMix: orthoMix ?? (orthographic ? 1.0 : 0.0),
    );
  }
}

class CloudProjection {
  const CloudProjection._({
    required this.cosY,
    required this.sinY,
    required this.cosP,
    required this.sinP,
    required this.f,
    required this.camDist,
    required this.ox,
    required this.oy,
    required this.pivotX,
    required this.pivotY,
    required this.pivotZ,
    required this.orthographic,
    required this.cosR,
    required this.sinR,
    this.orthoMix = 0.0,
  });

  final double cosY, sinY, cosP, sinP, f, camDist, ox, oy;

                                                                
                                    
  final double cosR, sinR;
  final double pivotX, pivotY, pivotZ;

                                                         
                                         
  final bool orthographic;

                                                                          
                                                                             
                                                         
  final double orthoMix;

                                                          
  double divisorAt(double depth) => orthoMix == 1.0
      ? camDist
      : (orthoMix == 0.0 ? depth : depth + (camDist - depth) * orthoMix);

                                               
  (double, double, double) project(double wx, double wy, double wz) {
    final px = wx - pivotX, py = wy - pivotY, pz = wz - pivotZ;
    final x1 = px * cosY + pz * sinY;
    final z1 = -px * sinY + pz * cosY;
    final y2 = py * cosP - z1 * sinP;
    final z2 = py * sinP + z1 * cosP;
    final depth = z2 + camDist;
    final d = divisorAt(depth);
    final sx = ox - x1 * f / d;
    final sy = oy - y2 * f / d;
    if (sinR == 0.0 && cosR == 1.0) return (sx, sy, depth);
                           
    final dx = sx - ox, dy = sy - oy;
    return (ox + dx * cosR - dy * sinR, oy + dx * sinR + dy * cosR, depth);
  }

                                        
                                   
  double worldPerPixelAt(double depth) => divisorAt(depth) / f;

                                                  
                              
  List<double> rightAxisWorld() {
    final r = [-cosY, 0.0, -sinY];
    if (sinR == 0.0 && cosR == 1.0) return r;
    final u = _upNoRoll();
                                                          
    return [
      cosR * r[0] + sinR * u[0],
      cosR * r[1] + sinR * u[1],
      cosR * r[2] + sinR * u[2],
    ];
  }

  List<double> _upNoRoll() => [sinY * sinP, cosP, -cosY * sinP];

                                               
                                             
                                                                
                       
                                                    
                                                                   
                      
                                              
                                                        
           
  List<double> upAxisWorld() {
    final u = _upNoRoll();
    if (sinR == 0.0 && cosR == 1.0) return u;
    final r = [-cosY, 0.0, -sinY];
                                                    
    return [
      -sinR * r[0] + cosR * u[0],
      -sinR * r[1] + cosR * u[1],
      -sinR * r[2] + cosR * u[2],
    ];
  }
}

                                                                  
  
                                                   
                                        
                                                           
                                                
                                                  
                                           
                                             
                                                   
                                                   
                                                            
                                                          

                                           
List<double> composeViewMatrix(double yaw, double pitch, double roll) {
  final cy = math.cos(yaw), sy = math.sin(yaw);
  final cp = math.cos(pitch), sp = math.sin(pitch);
  final cr = math.cos(roll), sr = math.sin(roll);
                    
  final a = [cy, 0.0, sy];
  final b = [sy * sp, cp, -cy * sp];
  final c = [-sy * cp, sp, cy * cp];
                                                                  
  return [
    cr * a[0] - sr * b[0], cr * a[1] - sr * b[1], cr * a[2] - sr * b[2],   
    sr * a[0] + cr * b[0], sr * a[1] + cr * b[1], sr * a[2] + cr * b[2],   
    c[0], c[1], c[2],
  ];
}

                                                              
                                                            
                                                 
(double, double, double) decomposeViewMatrix(List<double> m) {
  final sp = m[7].clamp(-1.0, 1.0);
  final pitch = math.asin(sp);
  final yaw = math.atan2(-m[6], m[8]);
                                               
  final cy = math.cos(yaw), sy = math.sin(yaw);
  final cp = math.cos(pitch);
  final a = [cy, 0.0, sy];
  final b = [sy * sp, cp, -cy * sp];
                                                               
  final cr = m[0] * a[0] + m[1] * a[1] + m[2] * a[2];
  final sr = -(m[0] * b[0] + m[1] * b[1] + m[2] * b[2]);
  return (yaw, pitch, math.atan2(sr, cr));
}

                                 
List<double> mulTransposed(List<double> a, List<double> b) {
  final out = List<double>.filled(9, 0);
  for (var i = 0; i < 3; i++) {
    for (var j = 0; j < 3; j++) {
      out[i * 3 + j] =
          a[i * 3] * b[j * 3] +
          a[i * 3 + 1] * b[j * 3 + 1] +
          a[i * 3 + 2] * b[j * 3 + 2];
    }
  }
  return out;
}

                 
List<double> mulMatrix(List<double> a, List<double> b) {
  final out = List<double>.filled(9, 0);
  for (var i = 0; i < 3; i++) {
    for (var j = 0; j < 3; j++) {
      out[i * 3 + j] =
          a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
    }
  }
  return out;
}

                                                   
                                         
(List<double>, double) axisAngleOf(List<double> r) {
  final trace = r[0] + r[4] + r[8];
  final cosA = ((trace - 1) / 2).clamp(-1.0, 1.0);
  final angle = math.acos(cosA);
  if (angle < 1e-9) return ([1.0, 0.0, 0.0], 0.0);
  if (angle > math.pi - 1e-6) {
                                                          
    final vx = math.sqrt(((r[0] + 1) / 2).clamp(0.0, 1.0));
    final vy = math.sqrt(((r[4] + 1) / 2).clamp(0.0, 1.0));
    final vz = math.sqrt(((r[8] + 1) / 2).clamp(0.0, 1.0));
                                       
    if (vx >= vy && vx >= vz) {
      return ([vx, r[1] / (2 * vx), r[2] / (2 * vx)], math.pi);
    } else if (vy >= vz) {
      return ([r[3] / (2 * vy), vy, r[5] / (2 * vy)], math.pi);
    }
    return ([r[6] / (2 * vz), r[7] / (2 * vz), vz], math.pi);
  }
  final s = 2 * math.sin(angle);
                                                   
  return ([(r[7] - r[5]) / s, (r[2] - r[6]) / s, (r[3] - r[1]) / s], angle);
}

                         
List<double> rotationFromAxisAngle(List<double> axis, double angle) {
  final n = math.sqrt(
    axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2],
  );
  if (n < 1e-12 || angle.abs() < 1e-12) {
    return [1, 0, 0, 0, 1, 0, 0, 0, 1];
  }
  final x = axis[0] / n, y = axis[1] / n, z = axis[2] / n;
  final c = math.cos(angle), s = math.sin(angle), t = 1 - c;
  return [
    t * x * x + c, t * x * y - s * z, t * x * z + s * y,   
    t * x * y + s * z, t * y * y + c, t * y * z - s * x,   
    t * x * z - s * y, t * y * z + s * x, t * z * z + c,
  ];
}

                                                                 
   
                                                      
                                            
                                                 
                                             
                                                   
                                
const double kCamDistK = 8.0;
const double kFitFillK = 6.5;
