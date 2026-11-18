                                                                                             
                                   
  
                                                                                            
                                                                                            
  
                                                                          
                                                                                                 
                                                                                                   
//     in exactly the projection the view paints with; nothing is re-derived from yaw/pitch here.
                                                                                     
                                                                                                  
                               
                                                                                                  
                                                                                                 
                                                                             
                                                            
                                                                                    
                                                                                               
                                                                                                   
                                                                                    
                                                                                                 
                                                                                             
                                                                                        
                                                                                            
                                                                                           
                                                                              
                                                                                                
                                                                                                     
                                                                                                     
                                                                                             
                                                                                                  
                                                                                                 
import 'dart:math' as math;
import 'dart:typed_data';
import 'dart:ui' show Size;

import '../ui/official_capture/cloud_camera.dart';

                                                                                                 
                                                                                                
const double kPotreeInitialNear = 0.1;
const double kPotreeInitialFar = 1000 * 1000;

                                                                                     
                                                                                      
                                                                                         
                                                                                 
double potreeLowestSpacingFromStats(double statsLowestSpacing) =>
    statsLowestSpacing > 0 && statsLowestSpacing.isFinite
    ? statsLowestSpacing
    : double.infinity;

                                                                                       
   
                                              
                                                   
                                                                                           
                                             
                                                         
                                                
                                              
                                                
                                                  
                                                          
                                                                                           
   
                                                                                           
                                                                                        
                                                                                       
                                                                                           
                                                                                           
                                                               
({double near, double far}) potreeNearFar({
  required double lowestSpacing,
  required List<double> viewRowMajor,
  required List<double> boxMin,
  required List<double> boxMax,
  required bool orthographic,
  double closestImage = double.infinity,
  double previousNear = kPotreeInitialNear,
  double previousFar = kPotreeInitialFar,
}) {
  var near = previousNear, far = previousFar;
  if (lowestSpacing != double.infinity) {
    var minZ = double.infinity;
    for (var i = 0; i < 8; i++) {
      final x = (i & 4) == 0 ? boxMin[0] : boxMax[0];
      final y = (i & 2) == 0 ? boxMin[1] : boxMax[1];
      final z = (i & 1) == 0 ? boxMin[2] : boxMax[2];
      final vz =
          viewRowMajor[8] * x +
          viewRowMajor[9] * y +
          viewRowMajor[10] * z +
          viewRowMajor[11];
      if (vz < minZ) minZ = vz;
    }
    var n = lowestSpacing * 10.0;
    var f = -minZ;
    f = math.max(f * 1.5, 10000);
    n = math.min(100.0, math.max(0.01, n));
    n = math.min(n, closestImage);
    f = math.max(f, n + 10000);
    if (n == double.infinity) {
      n = 0.1;
    }
    near = n;
    far = f;
  }
  if (orthographic) {
    near = -far;
  }
  return (near: near, far: far);
}

                                                                                   
class LodCameraFrame {
  LodCameraFrame({
    required this.viewProjRowMajor,
    required this.eyeWorld,
    required this.focalPx,
    required this.orbitDistance,
    required this.orthoMix,
    required this.viewportWidthPx,
    required this.viewportHeightPx,
    required this.near,
    required this.far,
    required this.viewRowMajor,
  }) : assert(viewProjRowMajor.length == 16),
       assert(eyeWorld.length == 3);

                                                                        
  final Float64List viewProjRowMajor;
  final Float64List eyeWorld;

                                                                                                  
                                              
  final double focalPx;

                              
  final double orbitDistance;

                                                               
  final double orthoMix;

                                                           
  final int viewportWidthPx;
  final int viewportHeightPx;

                                                                                                  
                                                                      
  final double near;
  final double far;
  final Float64List viewRowMajor;

                                                                                                      
                                                                                           
                                                                                                   
                                                                                                 
                                                            
  LodCameraFrame inTargetPixels(double d) => LodCameraFrame(
    viewProjRowMajor: viewProjRowMajor,
    eyeWorld: eyeWorld,
    focalPx: focalPx * d,
    orbitDistance: orbitDistance,
    orthoMix: orthoMix,
    viewportWidthPx: viewportWidthPx,
    viewportHeightPx: viewportHeightPx,
    near: near,
    far: far,
    viewRowMajor: viewRowMajor,
  );
}

                                                                                                   
Float64List mulRowMajor(List<double> a, List<double> b) {
  final o = Float64List(16);
  for (var r = 0; r < 4; r++) {
    for (var c = 0; c < 4; c++) {
      var s = 0.0;
      for (var k = 0; k < 4; k++) {
        s += a[r * 4 + k] * b[k * 4 + c];
      }
      o[r * 4 + c] = s;
    }
  }
  return o;
}

                                                             
   
                                                                                          
                                                                                              
/// (NDC is resolution independent, so the matrix does not depend on devicePixelRatio).
                                                                                                  
                                                    
LodCameraFrame lodCameraFrame({
  required CloudProjection projection,
  required Size logicalSize,
  required int viewportWidthPx,
  required int viewportHeightPx,
  required List<double> sceneBoxMin,
  required List<double> sceneBoxMax,
  double lowestSpacing = double.infinity,
  double previousNear = kPotreeInitialNear,
  double previousFar = kPotreeInitialFar,
}) {
  final p = projection;
  final w = logicalSize.width, h = logicalSize.height;
  final m = p.orthoMix, c = p.camDist, f = p.f;
  final orthographic = m == 1.0;

                                                                                    
  final r1 = <double>[p.cosY, 0.0, p.sinY];
  final r2 = <double>[p.sinY * p.sinP, p.cosP, -p.cosY * p.sinP];
  final r3 = <double>[-p.sinY * p.cosP, p.sinP, p.cosY * p.cosP];
  final pivot = <double>[p.pivotX, p.pivotY, p.pivotZ];
  double dot3(List<double> a, List<double> b) =>
      a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  final basis = Float64List.fromList(<double>[
    r1[0], r1[1], r1[2], -dot3(r1, pivot),   
    r2[0], r2[1], r2[2], -dot3(r2, pivot),   
    r3[0], r3[1], r3[2], -dot3(r3, pivot),   
    0, 0, 0, 1,
  ]);
                                                                               
  final view = Float64List.fromList(<double>[
    basis[0], basis[1], basis[2], basis[3],   
    basis[4], basis[5], basis[6], basis[7],   
    -basis[8], -basis[9], -basis[10], -(basis[11] + c),   
    0, 0, 0, 1,
  ]);
  final nf = potreeNearFar(
    lowestSpacing: lowestSpacing,
    viewRowMajor: view,
    boxMin: sceneBoxMin,
    boxMax: sceneBoxMax,
    orthographic: orthographic,
    previousNear: previousNear,
    previousFar: previousFar,
  );
  final near = nf.near, far = nf.far;

                            
  final kx = 2 * p.ox / w - 1, ky = 1 - 2 * p.oy / h;
  final fx = 2 * f / w, fy = 2 * f / h;
  final wz = 1 - m;                            
  final wFar = (1 - m) * far + m * c;
  final a = wFar / (far - near);
  final clip = Float64List.fromList(<double>[
    -fx * p.cosR, fx * p.sinR, kx * wz, kx * c,   
    fy * p.sinR, fy * p.cosR, ky * wz, ky * c,   
    0, 0, a, a * (c - near),   
    0, 0, wz, c,
  ]);
  return LodCameraFrame(
    viewProjRowMajor: mulRowMajor(clip, basis),
    eyeWorld: Float64List.fromList(<double>[
      pivot[0] - c * r3[0],
      pivot[1] - c * r3[1],
      pivot[2] - c * r3[2],
    ]),
    focalPx: f,
    orbitDistance: c,
    orthoMix: m,
    viewportWidthPx: viewportWidthPx,
    viewportHeightPx: viewportHeightPx,
    near: near,
    far: far,
    viewRowMajor: view,
  );
}
