                                                               
                                 
  
                                                        
                                              
                                             
                                                  
                                       
                                      
                                                   
                                              
                              
  
                                                    
                                                    
                                         
                         
                                                       
  
                                                             
                                                    
  
                                                            
                                                            
                                         

import 'dart:typed_data';

import 'representative_color.dart';

                    
   
                                                
                                                                  
                                                   
                                  
                                                                
                                                          
                                   
   
                                  
                                                          
                                                   
                             
   
                                                      
                                                       
                                      
const int kColorizeDecodeMaxPx = 8192;

                                                       
                                                      
                             
typedef ColorJpegDecoder = Future<({Uint8List rgb, int w, int h})?> Function(
  String jpegPath,
);

                                                       
                                               
class ColorizeFrameJob {
  const ColorizeFrameJob({
    required this.jpegPath,
    required this.grayW,
    required this.grayH,
    required this.tri,
  });

  final String jpegPath;
  final int grayW;
  final int grayH;
  final List<double> tri;
}

                                                 
                                      
class ColorizeDecodeStats {
                                           
  int framesSampled = 0;

                                    
  int decodeFail = 0;

                                                    
  int uniqueDecodes = 0;

                                   
  final List<double> decodeMs = <double>[];
}

                                                
                                  
   
                                         
                                                    
                                
                                                             
                                              
Future<ColorizeDecodeStats> sampleColorsPipelined({
  required List<ColorizeFrameJob> jobs,
  required RepresentativeColorSamples samples,
  required ColorJpegDecoder decode,
  int maxInFlight = 3,
  bool Function()? isCancelled,
}) async {
  final stats = ColorizeDecodeStats();
  if (jobs.isEmpty) return stats;
  final window = maxInFlight < 1 ? 1 : maxInFlight;

                                           
                                   
  final remainingUses = <String, int>{};
  for (final j in jobs) {
    remainingUses[j.jpegPath] = (remainingUses[j.jpegPath] ?? 0) + 1;
  }
  final pending = <String, Future<({Uint8List rgb, int w, int h})?>>{};

  Future<({Uint8List rgb, int w, int h})?> launch(String path) {
    return pending.putIfAbsent(path, () async {
      stats.uniqueDecodes++;
      final dsw = Stopwatch()..start();
      ({Uint8List rgb, int w, int h})? r;
      try {
        r = await decode(path);
      } catch (_) {
        r = null;                                              
                                                            
      }
      dsw.stop();
      stats.decodeMs.add(dsw.elapsedMilliseconds.toDouble());
      return r;
    });
  }

  var nextLaunch = 0;
  for (var k = 0; k < jobs.length; k++) {
    if (isCancelled?.call() ?? false) return stats;               
                                                  
    while (nextLaunch < jobs.length && nextLaunch < k + window) {
      launch(jobs[nextLaunch].jpegPath);
      nextLaunch++;
    }
    final job = jobs[k];
    final sj = await pending[job.jpegPath]!;                        
                                  
    final left = remainingUses[job.jpegPath]! - 1;
    if (left <= 0) {
      remainingUses.remove(job.jpegPath);
      pending.remove(job.jpegPath);
    } else {
      remainingUses[job.jpegPath] = left;
    }
    if (sj == null) {
      stats.decodeFail++;
      continue;
    }
    stats.framesSampled++;
                                           
                                                                             
                                                  
    final scaleX = sj.w / job.grayW, scaleY = sj.h / job.grayH;
    final rgbP = sj.rgb;
    final jw = sj.w, jh = sj.h;
    final tri = job.tri;
    for (var t = 0; t < tri.length; t += 3) {
      final i = tri[t].toInt();
                                                                          
                                                                       
      final fx = tri[t + 1] * scaleX - 0.5;
      final fy = tri[t + 2] * scaleY - 0.5;
      final x0 = fx.floor(), y0 = fy.floor();
      final x1 = x0 + 1, y1 = y0 + 1;
      if (x0 < 0 || y0 < 0 || x1 >= jw || y1 >= jh) continue;
      final dx = fx - x0, dy = fy - y0;
      final w00 = (1 - dx) * (1 - dy), w01 = dx * (1 - dy);
      final w10 = (1 - dx) * dy, w11 = dx * dy;
      final o00 = (y0 * jw + x0) * 3, o01 = (y0 * jw + x1) * 3;
      final o10 = (y1 * jw + x0) * 3, o11 = (y1 * jw + x1) * 3;
      samples.add(
        i,
        w00 * rgbP[o00] + w01 * rgbP[o01] + w10 * rgbP[o10] + w11 * rgbP[o11],
        w00 * rgbP[o00 + 1] +
            w01 * rgbP[o01 + 1] +
            w10 * rgbP[o10 + 1] +
            w11 * rgbP[o11 + 1],
        w00 * rgbP[o00 + 2] +
            w01 * rgbP[o01 + 2] +
            w10 * rgbP[o10 + 2] +
            w11 * rgbP[o11 + 2],
        k,                                          
      );
    }
  }
  return stats;
}
