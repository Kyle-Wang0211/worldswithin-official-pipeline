                                                                           
                                                                   
                                                                         
  
                                                                            
                                                                                    
                                                                           
                                                                 
                                                                               
                                                                     
                                                                            

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'sfm_live_recon.dart';
import 'photo_archive_runtime.dart';
import '../official_util/device_log.dart';

Future<void> persistSparseSnapshot({
  required String captureDir,
  required SfmLiveSnapshot snapshot,
  required Uint8List rgb,                                                 
}) async {
  final archiveCoordinator = photoArchiveCoordinator;
  try {
    final n = snapshot.pointCount;
    if (n == 0) return;
    final xyz = snapshot.xyz;

                       
    final header =
        'ply\n'
        'format binary_little_endian 1.0\n'
        'comment WorldsWithin capture-time sparse reconstruction (full set)\n'
                                                           
                                                          
                                                  
                                                                                 
                                                                     
                                         
          
                                                          
                                                 
                                              
          
                                                        
                                                        
                                                                 
                                                         
                                                                   
                                            
        'comment generator=WorldsWithin\n'
        'comment method=photogrammetry-sfm\n'
        'comment synthetic-content-marker=v1\n'
        'element vertex $n\n'
        'property float x\n'
        'property float y\n'
        'property float z\n'
        'property uchar red\n'
        'property uchar green\n'
        'property uchar blue\n'
        'end_header\n';
    final headerBytes = utf8.encode(header);
    final body = Uint8List(n * 15);
    final bd = ByteData.view(body.buffer);
    for (var i = 0; i < n; i++) {
      final o = i * 15;
      bd.setFloat32(o, xyz[i * 3], Endian.little);
      bd.setFloat32(o + 4, xyz[i * 3 + 1], Endian.little);
      bd.setFloat32(o + 8, xyz[i * 3 + 2], Endian.little);
      body[o + 12] = i * 3 + 2 < rgb.length ? rgb[i * 3] : 200;
      body[o + 13] = i * 3 + 2 < rgb.length ? rgb[i * 3 + 1] : 200;
      body[o + 14] = i * 3 + 2 < rgb.length ? rgb[i * 3 + 2] : 205;
    }
    final ply = File('$captureDir/official_sfm_sparse.ply');
    final sink = ply.openWrite();
    sink.add(headerBytes);
    sink.add(body);
    await sink.close();

                         
                                                                
                                                            
                                                           
                                                                 
                                                             
                                         
    Map<String, Object?> poseEntry(Float64List p, int i) => {
      'frame_id': p[i].toInt(),
      'registered': p[i + 1] != 0,
      'quat_wxyz': [p[i + 2], p[i + 3], p[i + 4], p[i + 5]],
      't': [p[i + 6], p[i + 7], p[i + 8]],
    };
    final poses = snapshot.posesPacked;
    final posesJson = <Map<String, Object?>>[
      for (var i = 0; i < poses.length; i += 9) poseEntry(poses, i),
    ];
    final rawPoses = snapshot.posesPackedRawColmap;
    await File('$captureDir/official_sfm_sparse_meta.json').writeAsString(
      jsonEncode({
        'schema': 'pw_sfm_sparse_meta_v2',
        'written_at': DateTime.now().toIso8601String(),
        'refined': snapshot.refined,
        'n_points': n,
        'summary': snapshot.summary,
        'poses': posesJson,
        'gravity_align_quat_wxyz': snapshot.gravityAlignQuatWxyz,
                                                      
        'scale_anchor_factor': snapshot.scaleAnchorFactor,
        if (rawPoses != null)
          'poses_raw_colmap': <Map<String, Object?>>[
            for (var i = 0; i < rawPoses.length; i += 9) poseEntry(rawPoses, i),
          ],
      }),
    );
                                                                              
                                                                            
                                                                              
                                                                               
                                                                          
    final off = snapshot.obsOffsets;
    if (off.length >= n + 1) {
      final buckets = List<int>.filled(11, 0);                              
      var sumLen = 0;
      var maxLen = 0;
      for (var i = 0; i < n; i++) {
        final len = off[i + 1] - off[i];
        sumLen += len;
        if (len > maxLen) maxLen = len;
        final b = len >= 10 ? 10 : (len < 2 ? 2 : len);
        buckets[b]++;
      }
      final twoView = buckets[2];
      final pct = (100.0 * twoView / n).toStringAsFixed(1);
      final mean = (sumLen / n).toStringAsFixed(2);
      final hist = [
        for (var l = 2; l <= 9; l++) 'L$l=${buckets[l]}',
        'L10+=${buckets[10]}',
      ].join(' ');
      DeviceLog.log(
        'SfmLive',
        'track-hist: n=$n 2view=$twoView($pct%) mean=$mean max=$maxLen | $hist (refined=${snapshot.refined})',
      );
    }

    DeviceLog.log(
      'SfmLive',
      'sparse persisted: $n pts (refined=${snapshot.refined}) → $captureDir/official_sfm_sparse.ply',
    );
    unawaited(
      archiveCoordinator.noteArtifactsPersisted(Directory(captureDir)),
    );
  } catch (e) {
    DeviceLog.log('SfmLive', 'sparse persist FAILED: $e');
  }
}
