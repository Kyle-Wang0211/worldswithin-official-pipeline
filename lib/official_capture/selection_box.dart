                                          
  
                                                
                                             
                                            
                                                                  
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

const String kSelectionBoxFileName = 'official_selection_box.json';

                                             
                                          
                                           
const int kSelectionBoxSchemaVersion = 2;

              
const List<double> kIdentityRot = <double>[1, 0, 0, 0, 1, 0, 0, 0, 1];

                                      
List<double> rotAboutAxisDeg(List<double> axis, double deg) {
  final n = math.sqrt(
    axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2],
  );
  if (n < 1e-12) return kIdentityRot;
  final x = axis[0] / n, y = axis[1] / n, z = axis[2] / n;
  final t = deg * math.pi / 180.0;
  final c = math.cos(t), s = math.sin(t), k = 1 - c;
  return <double>[
    c + x * x * k, x * y * k - z * s, x * z * k + y * s,   
    y * x * k + z * s, c + y * y * k, y * z * k - x * s,
    z * x * k - y * s, z * y * k + x * s, c + z * z * k,
  ];
}

                    
List<double> mulRot(List<double> a, List<double> b) {
  final out = List<double>.filled(9, 0);
  for (var r = 0; r < 3; r++) {
    for (var c = 0; c < 3; c++) {
      var v = 0.0;
      for (var k = 0; k < 3; k++) {
        v += a[r * 3 + k] * b[k * 3 + c];
      }
      out[r * 3 + c] = v;
    }
  }
  return out;
}

class SelectionBox {
  const SelectionBox({
    required this.cx,
    required this.cy,
    required this.cz,
    required this.sx,
    required this.sy,
    required this.sz,
    this.rot = kIdentityRot,
  });

                                   
  factory SelectionBox.withYaw({
    required double cx,
    required double cy,
    required double cz,
    required double sx,
    required double sy,
    required double sz,
    required double yawDeg,
  }) => SelectionBox(
    cx: cx,
    cy: cy,
    cz: cz,
    sx: sx,
    sy: sy,
    sz: sz,
                                                              
                                           
    rot: rotAboutAxisDeg(const [0, 1, 0], -yawDeg),
  );

                             
  final double cx, cy, cz;

                  
  final double sx, sy, sz;

                                
  final List<double> rot;

                               
  double get yawDeg => math.atan2(rot[6], rot[0]) * 180.0 / math.pi;

                                                  
  static const double kMinHalfSizeFraction = 0.02;

                                                 
                                     
                                             
  static const double kInitialBoxRadiusScale = 2.0;

                                               
                                          
                                                 
                                              
                                                    
  factory SelectionBox.initialSquareFace({
    required double cx,
    required double cy,
    required double cz,
    required double halfExtent,
  }) {
    final side = math.max(halfExtent, 1e-6) * 2;
    return SelectionBox(cx: cx, cy: cy, cz: cz, sx: side, sy: side, sz: side);
  }

  bool contains(double wx, double wy, double wz) {
                                                             
    final px = wx - cx, py = wy - cy, pz = wz - cz;
    final lx = rot[0] * px + rot[3] * py + rot[6] * pz;
    final ly = rot[1] * px + rot[4] * py + rot[7] * pz;
    final lz = rot[2] * px + rot[5] * py + rot[8] * pz;
    return lx.abs() <= sx / 2 && ly.abs() <= sy / 2 && lz.abs() <= sz / 2;
  }

                                                    
                                        
     
                                            
                                                     
                                        
                                            
                                                         
                                         
                                               
                             
                                                 
     
                                         
                                           
             
     
                                                        
                                                      
                               
  bool sameAs(SelectionBox other, {double eps = 1e-9}) {
    bool near(double a, double b) => (a - b).abs() <= eps;
    if (!near(cx, other.cx) || !near(cy, other.cy) || !near(cz, other.cz)) {
      return false;
    }
    if (!near(sx, other.sx) || !near(sy, other.sy) || !near(sz, other.sz)) {
      return false;
    }
    if (rot.length != other.rot.length) return false;
    for (var i = 0; i < rot.length; i++) {
      if (!near(rot[i], other.rot[i])) return false;
    }
    return true;
  }

  bool isSaneFor({
    required double fitCx,
    required double fitCy,
    required double fitCz,
    required double fitRadius,
  }) {
    final minSide = fitRadius * 0.05;
    if (sx < minSide || sy < minSide || sz < minSide) return false;
    final dx = cx - fitCx, dy = cy - fitCy, dz = cz - fitCz;
    final dist = math.sqrt(dx * dx + dy * dy + dz * dz);
    return dist <= fitRadius * 5;
  }

                                                      
     
                                                  
                                      
  SelectionBox rotatedAroundAxis({
    required List<double> axis,
    required double deltaDeg,
    required double pivotX,
    required double pivotY,
    required double pivotZ,
  }) {
    final r = rotAboutAxisDeg(axis, deltaDeg);
    final dx = cx - pivotX, dy = cy - pivotY, dz = cz - pivotZ;
    return copyWith(
      cx: pivotX + r[0] * dx + r[1] * dy + r[2] * dz,
      cy: pivotY + r[3] * dx + r[4] * dy + r[5] * dz,
      cz: pivotZ + r[6] * dx + r[7] * dy + r[8] * dz,
      rot: mulRot(r, rot),
    );
  }

  SelectionBox copyWith({
    double? cx,
    double? cy,
    double? cz,
    double? sx,
    double? sy,
    double? sz,
    List<double>? rot,
  }) => SelectionBox(
    cx: cx ?? this.cx,
    cy: cy ?? this.cy,
    cz: cz ?? this.cz,
    sx: sx ?? this.sx,
    sy: sy ?? this.sy,
    sz: sz ?? this.sz,
    rot: rot ?? this.rot,
  );

  Map<String, dynamic> toJson() => <String, dynamic>{
    'v': kSelectionBoxSchemaVersion,
    'cx': cx,
    'cy': cy,
    'cz': cz,
    'sx': sx,
    'sy': sy,
    'sz': sz,
    'rot': rot,
                                     
    'yawDeg': yawDeg,
  };

                                               
                                               
  static SelectionBox? fromJson(Object? j) {
    if (j is! Map) return null;
                                      
    if (j['v'] != kSelectionBoxSchemaVersion) return null;
    double? d(Object? v) => (v is num && v.isFinite) ? v.toDouble() : null;
    final cx = d(j['cx']), cy = d(j['cy']), cz = d(j['cz']);
    final sx = d(j['sx']), sy = d(j['sy']), sz = d(j['sz']);
    if ([cx, cy, cz, sx, sy, sz].contains(null)) return null;
                            
    final rawRot = j['rot'];
    if (rawRot is List && rawRot.length == 9) {
      final r = rawRot.map(d).toList();
      if (!r.contains(null)) {
                                                   
                                     
        return SelectionBox(
          cx: cx!,
          cy: cy!,
          cz: cz!,
          sx: sx!,
          sy: sy!,
          sz: sz!,
          rot: r.cast<double>(),
        );
      }
    }
    final yaw = d(j['yawDeg']);
    if (yaw == null) return null;
    return SelectionBox.withYaw(
      cx: cx!,
      cy: cy!,
      cz: cz!,
      sx: sx!,
      sy: sy!,
      sz: sz!,
      yawDeg: yaw,
    );
  }

  static Future<SelectionBox?> loadFrom(String captureDir) async {
    try {
      final f = File('$captureDir/$kSelectionBoxFileName');
      if (!await f.exists()) return null;
      return fromJson(jsonDecode(await f.readAsString()));
    } catch (_) {
      return null;
    }
  }

  Future<void> saveTo(String captureDir) async {
    final f = File('$captureDir/$kSelectionBoxFileName');
    await f.writeAsString(jsonEncode(toJson()), flush: true);
  }
}
