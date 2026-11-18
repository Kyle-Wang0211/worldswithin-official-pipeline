                                                    
  
                                                
                                                  
                                       
                                                        
                                                             
                                                         
                             
  
                                                       
                                   
  
                                                       
                             
  
                                                
                                                   
                   

import 'dart:convert';

import 'official_highres_reconstruction_input.dart';
import 'photo_slot_naming.dart';

                                  
   
                                       
                    
class ArchivedPhotoParse {
  const ArchivedPhotoParse.accepted(this.jpegPath, this.input)
    : failure = null;
  const ArchivedPhotoParse.rejected(this.jpegPath, this.failure) : input = null;

  final String jpegPath;
  final OfficialHighResReconstructionInput? input;

                                        
  final String? failure;

  bool get isAccepted => input != null;
}

                                
   
                                                              
                                                                   
                                 
   
                                                                           
                                                         
                                                                 
                                                     
                                                   
                         
   
                                                              
                                              
                             
ArchivedPhotoParse parseArchivedPhoto({
  required String jpegPath,
  required String sidecarJson,
}) {
  final Map<String, dynamic> j;
  try {
    final decoded = jsonDecode(sidecarJson);
    if (decoded is! Map<String, dynamic>) {
      return ArchivedPhotoParse.rejected(jpegPath, 'sidecar 不是 JSON 对象');
    }
    j = decoded;
  } catch (e) {
    return ArchivedPhotoParse.rejected(jpegPath, 'sidecar 解析失败: $e');
  }

  List<double>? doubles(Object? v, int want) {
    if (v is! List || v.length < want) return null;
    final out = <double>[];
    for (final e in v.take(want)) {
      if (e is num && e.toDouble().isFinite) {
        out.add(e.toDouble());
      } else {
        return null;
      }
    }
    return out;
  }

  final intrinsics = doubles(j['intrinsics_fxfycxcy'], 4);
  if (intrinsics == null) {
    return ArchivedPhotoParse.rejected(jpegPath, '缺 intrinsics_fxfycxcy');
  }
  final transform = doubles(j['extrinsic'], 16);
  if (transform == null) {
    return ArchivedPhotoParse.rejected(jpegPath, '缺 extrinsic(16 个有限数)');
  }
  final w = j['image_w'];
  final h = j['image_h'];
  if (w is! int || h is! int) {
    return ArchivedPhotoParse.rejected(jpegPath, '缺 image_w/image_h');
  }
                                                  
                                               
  final captureT = j['t'];
  final triggerT = j['save_target_t'] ?? captureT;
  if (captureT is! num || triggerT is! num) {
    return ArchivedPhotoParse.rejected(jpegPath, '缺时间戳 t / save_target_t');
  }

  final v = OfficialHighResReconstructionInput.validate(
    jpegPath: jpegPath,
    imageWidth: w,
    imageHeight: h,
    triggerTimestamp: triggerT.toDouble(),
    captureTimestamp: captureT.toDouble(),
    cameraTransform: transform,
    intrinsics: intrinsics,
  );
  final input = v.input;
  if (input == null) {
                                         
    return ArchivedPhotoParse.rejected(jpegPath, 'validate 拒绝: ${v.failure}');
  }
  return ArchivedPhotoParse.accepted(jpegPath, input);
}

                                
   
                   
                                                    
                                                         
                                                               
                                                           
                                          
                                                      
                                  
   
                                            
                                                        
                        
List<ArchivedPhotoParse> orderForRefeed(Iterable<ArchivedPhotoParse> parses) {
  final list = parses.toList();
  int seqOf(ArchivedPhotoParse p) =>
      frameSeqInName(p.jpegPath.split('/').last) ?? (1 << 31);
  list.sort((a, b) {
    final c = seqOf(a).compareTo(seqOf(b));
    if (c != 0) return c;
    final ai = a.input, bi = b.input;
    if (ai != null && bi != null) {
      final d = ai.captureTimestamp.compareTo(bi.captureTimestamp);
      if (d != 0) return d;
    }
    return a.jpegPath.compareTo(b.jpegPath);
  });
  return list;
}

                                                                           
                                  
  
                                                    
                         
                                         
                                                                     
                                                        
                                                    
                                                     
                                                            
                                                          
                                              
  
                                                  
                                               
                          
  
                                                 
                                                    
                                                                 
                                                         

                        
class ProjectCoverage {
  const ProjectCoverage({
    required this.photosOnDisk,
    required this.fedDistinct,
    required this.duplicateFrameIds,
    required this.neverFed,
  });

  final int photosOnDisk;

                       
  final int fedDistinct;

                                         
  final int duplicateFrameIds;

                          
  final List<String> neverFed;

                               
  bool get covered => neverFed.isEmpty && duplicateFrameIds == 0;

                           
  String? get reason {
    if (covered) return null;
    final parts = <String>[];
    if (neverFed.isNotEmpty) parts.add('盘上有 ${neverFed.length} 张没喂过');
    if (duplicateFrameIds > 0) {
      parts.add('账本 frameId 重复 $duplicateFrameIds 次(跨了多个会话)');
    }
    return parts.join(' | ');
  }
}

                                                          
   
                       
                                       
                                                     
                                                                      
                                          
                                         
                                     
   
                                     
                                                                    
                                                     
                                                       
                      
ProjectCoverage projectCoverageFrom({
  required Iterable<String> jpegNamesOnDisk,
  required String fedFramesJsonl,
}) {
  final fedNames = <String>{};
  final frameIdCounts = <int, int>{};
  for (final line in const LineSplitter().convert(fedFramesJsonl)) {
    if (line.trim().isEmpty) continue;
    try {
      final m = jsonDecode(line);
      if (m is! Map<String, dynamic>) continue;
      final path = m['jpegPath'];
      if (path is String && path.isNotEmpty) {
        fedNames.add(path.split('/').last);
      }
      final fid = m['frameId'];
      if (fid is int) frameIdCounts[fid] = (frameIdCounts[fid] ?? 0) + 1;
    } catch (_) {
                                                  
      continue;
    }
  }
  var dup = 0;
  for (final n in frameIdCounts.values) {
    if (n > 1) dup += n - 1;
  }
  final disk = jpegNamesOnDisk.toSet();
  final never = disk.difference(fedNames).toList()..sort();
  return ProjectCoverage(
    photosOnDisk: disk.length,
    fedDistinct: fedNames.length,
    duplicateFrameIds: dup,
    neverFed: never,
  );
}
