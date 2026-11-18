                                            
  
                                            
                                                 
                                      
  
                                      
                                                  
                                       
                                 
  
                                                             
                                                      
                      

import 'dart:math' as math;
import 'dart:typed_data';

                                                         
                                                        
                               
                                    
                                                                      
   
                                                  
                                                                         
                                                                     
                                                           
                                         
                                
   
                                                       
                                  
double srgbToLinear255(double v) {
  final c = v / 255.0;
  final lin = c <= 0.04045
      ? c / 12.92
      : math.pow((c + 0.055) / 1.055, 2.4).toDouble();
  return lin * 255.0;
}

double linear255ToSrgb(double v) {
  final c = (v < 0 ? 0.0 : v) / 255.0;
  final e = c <= 0.0031308
      ? c * 12.92
      : 1.055 * math.pow(c, 1 / 2.4).toDouble() - 0.055;
  return e * 255.0;
}

int obsHistBucket(int count) {
  if (count <= 1) return 0;
  if (count == 2) return 1;
  if (count <= 4) return 2;
  if (count <= 8) return 3;
  return 4;
}

                                                        
                                                  
   
                                           
                                       
                                       
            
int selectRepresentativeSample(Float32List samples, int start, int count) {
  assert(count > 0);
  if (count == 1) return start;
  final lumas = Float64List(count);
  for (var k = 0; k < count; k++) {
    final o = (start + k) * 3;
    lumas[k] =
        0.299 * samples[o] + 0.587 * samples[o + 1] + 0.114 * samples[o + 2];
  }
  final sorted = Float64List.fromList(lumas)..sort();
  final median = sorted[(count - 1) >> 1];                      
  var best = 0;
  var bestDist = double.infinity;
  for (var k = 0; k < count; k++) {
    final d = (lumas[k] - median).abs();
    if (d < bestDist || (d == bestDist && lumas[k] < lumas[best])) {
      best = k;
      bestDist = d;
    }
  }
  return start + best;
}

                                                 
   
                                                      
                                
                                                           
                                                                           
                                                                             
   
                                           
                                               
                                                        
                                                     
                                                                
                     
   
                                         
                                   
class FrameColorGains {
  FrameColorGains(this.frames)
    : logGain = Float64List(frames * 3),
      banned = Uint8List(frames);

  final int frames;

                                                         
  final Float64List logGain;

                                              
  final Uint8List banned;

                                                      
  double gainOf(int frame, int c) =>
      frame < 0 || frame >= frames ? 1.0 : math.exp(logGain[frame * 3 + c]);

                                         
                                                  
  double correct(double srgbValue, int frame, int c) =>
      linear255ToSrgb(srgbToLinear255(srgbValue) * gainOf(frame, c));

                          
  List<double> get span {
    var lo = double.infinity, hi = -double.infinity;
    for (var f = 0; f < frames; f++) {
      for (var c = 0; c < 3; c++) {
        final g = math.exp(logGain[f * 3 + c]);
        if (g < lo) lo = g;
        if (g > hi) hi = g;
      }
    }
    return frames == 0 ? <double>[1, 1] : <double>[lo, hi];
  }

  int get bannedCount {
    var n = 0;
    for (final b in banned) {
      if (b != 0) n++;
    }
    return n;
  }
}

class RepresentativeColorSamples {
  final Int32List _base;                   
  final Int32List _cap;        
  final Int32List _count;           
  final Float32List _rgb;                  
  final Int32List _frame;                                       

  RepresentativeColorSamples._(
    this._base,
    this._cap,
    this._count,
    this._rgb,
    this._frame,
  );

  factory RepresentativeColorSamples(Int32List capacityPerPoint) {
    final n = capacityPerPoint.length;
    final base = Int32List(n);
    var total = 0;
    for (var i = 0; i < n; i++) {
      base[i] = total;
      total += capacityPerPoint[i];
    }
    return RepresentativeColorSamples._(
      base,
      capacityPerPoint,
      Int32List(n),
      Float32List(total * 3),
      Int32List(total)..fillRange(0, total, -1),
    );
  }

                                               
                                                   
  void add(int i, double r, double g, double b, [int frame = -1]) {
    final c = _count[i];
    if (c >= _cap[i]) return;
    final o = (_base[i] + c) * 3;
    _rgb[o] = r;
    _rgb[o + 1] = g;
    _rgb[o + 2] = b;
    _frame[_base[i] + c] = frame;
    _count[i] = c + 1;
  }

                              
  int hitCount(int i) => _count[i];

                                                
                                                 
                                       
                                                
                                         
  double rmsDeviation(int i, int r, int g, int b) {
    final c = _count[i];
    if (c < 2) return 0;
    var sum = 0.0;
    for (var k = 0; k < c; k++) {
      final o = (_base[i] + k) * 3;
      final dr = _rgb[o] - r;
      final dg = _rgb[o + 1] - g;
      final db = _rgb[o + 2] - b;
      sum += (dr * dr + dg * dg + db * db) / 3.0;
    }
    return math.sqrt(sum / c);
  }

                                                       
     
                                                 
                 
                                                
                                                            
                                                                    
                                           
                                              
  bool selectInto(
    int i,
    Uint8List out, {
    FrameColorGains? gains,
    bool applyToOutput = false,
  }) {
    final c = _count[i];
    if (c == 0) return false;
    if (gains == null) {
      final s = selectRepresentativeSample(_rgb, _base[i], c);
      final o = s * 3;
      out[i * 3] = _rgb[o].round().clamp(0, 255);
      out[i * 3 + 1] = _rgb[o + 1].round().clamp(0, 255);
      out[i * 3 + 2] = _rgb[o + 2].round().clamp(0, 255);
      return true;
    }
                                                      
                                     
    final base = _base[i];
    final lum = Float64List(c);
    for (var k = 0; k < c; k++) {
      final o = (base + k) * 3;
      final f = _frame[base + k];
      final r = gains.correct(_rgb[o], f, 0);
      final g = gains.correct(_rgb[o + 1], f, 1);
      final b = gains.correct(_rgb[o + 2], f, 2);
      lum[k] = 0.299 * r + 0.587 * g + 0.114 * b;
    }
    final sorted = Float64List.fromList(lum)..sort();
    final median = sorted[(c - 1) >> 1];
    var best = 0;
    var bestDist = double.infinity;
    for (var k = 0; k < c; k++) {
      final d = (lum[k] - median).abs();
      if (d < bestDist || (d == bestDist && lum[k] < lum[best])) {
        best = k;
        bestDist = d;
      }
    }
    final o = (base + best) * 3;
    final f = _frame[base + best];
    final sr = applyToOutput ? gains.correct(_rgb[o], f, 0) : _rgb[o];
    final sg = applyToOutput ? gains.correct(_rgb[o + 1], f, 1) : _rgb[o + 1];
    final sb = applyToOutput ? gains.correct(_rgb[o + 2], f, 2) : _rgb[o + 2];
    out[i * 3] = sr.round().clamp(0, 255);
    out[i * 3 + 1] = sg.round().clamp(0, 255);
    out[i * 3 + 2] = sb.round().clamp(0, 255);
    return true;
  }

                                                             
                                                       
                    
  bool selectIntoFloat(
    int i,
    Float32List out, {
    FrameColorGains? gains,
    bool applyToOutput = false,
  }) {
    final c = _count[i];
    if (c == 0) return false;
    final base = _base[i];
    int best;
    if (gains == null) {
      best = selectRepresentativeSample(_rgb, base, c) - base;
    } else {
      final lum = Float64List(c);
      for (var k = 0; k < c; k++) {
        final o = (base + k) * 3;
        final f = _frame[base + k];
        lum[k] =
            0.299 * gains.correct(_rgb[o], f, 0) +
            0.587 * gains.correct(_rgb[o + 1], f, 1) +
            0.114 * gains.correct(_rgb[o + 2], f, 2);
      }
      final sorted = Float64List.fromList(lum)..sort();
      final median = sorted[(c - 1) >> 1];
      best = 0;
      var bestDist = double.infinity;
      for (var k = 0; k < c; k++) {
        final d = (lum[k] - median).abs();
        if (d < bestDist || (d == bestDist && lum[k] < lum[best])) {
          best = k;
          bestDist = d;
        }
      }
    }
    final o = (base + best) * 3;
    final f = _frame[base + best];
    final useGain = gains != null && applyToOutput;
    out[i * 3] = useGain ? gains.correct(_rgb[o], f, 0) : _rgb[o];
    out[i * 3 + 1] = useGain ? gains.correct(_rgb[o + 1], f, 1) : _rgb[o + 1];
    out[i * 3 + 2] = useGain ? gains.correct(_rgb[o + 2], f, 2) : _rgb[o + 2];
    return true;
  }

                                                     
                                                              
                                  
     
                                                 
                                                     
                         
  bool meanIntoFloat(int i, Float32List out, {FrameColorGains? gains}) {
    final c = _count[i];
    if (c == 0) return false;
    final base = _base[i];
    var sr = 0.0, sg = 0.0, sb = 0.0;
    for (var k = 0; k < c; k++) {
      final o = (base + k) * 3;
      final f = _frame[base + k];
      if (gains == null) {
        sr += _rgb[o];
        sg += _rgb[o + 1];
        sb += _rgb[o + 2];
      } else {
        sr += gains.correct(_rgb[o], f, 0);
        sg += gains.correct(_rgb[o + 1], f, 1);
        sb += gains.correct(_rgb[o + 2], f, 2);
      }
    }
    out[i * 3] = sr / c;
    out[i * 3 + 1] = sg / c;
    out[i * 3 + 2] = sb / c;
    return true;
  }

                                                                      
                                            
                                                                       
                           
     
                                      
                                                                    
                                                            
                                                            
                                            
     
                                
                                              
                                                    
                                 
                                               
                                                             
                                           
                                       
     
                                                  
  FrameColorGains estimateFrameGains(
    int frameCount, {
    int iterations = 200,
    double minLevel = 12.0,
    int minPairPoints = 30,
    double alpha = 0.01,
    double beta = 100.0,
  }) {
    final out = FrameColorGains(frameCount);
    if (frameCount <= 0) return out;

                                     
    final sumA = <int, Float64List>{};                                  
    final sumB = <int, Float64List>{};
    final cnt = <int, int>{};
    final n = _count.length;
    for (var i = 0; i < n; i++) {
      final c = _count[i];
      if (c < 2) continue;
      final base = _base[i];
      for (var a = 0; a < c; a++) {
        final fa = _frame[base + a];
        if (fa < 0 || fa >= frameCount) continue;
        for (var b = a + 1; b < c; b++) {
          final fb = _frame[base + b];
          if (fb < 0 || fb >= frameCount || fb == fa) continue;
          final aIsLo = fa < fb;
          final key = (aIsLo ? fa : fb) * frameCount + (aIsLo ? fb : fa);
          final oa = (base + (aIsLo ? a : b)) * 3;
          final ob = (base + (aIsLo ? b : a)) * 3;
          final sa = sumA.putIfAbsent(key, () => Float64List(3));
          final sb = sumB.putIfAbsent(key, () => Float64List(3));
          for (var ch = 0; ch < 3; ch++) {
            sa[ch] += srgbToLinear255(_rgb[oa + ch]);
            sb[ch] += srgbToLinear255(_rgb[ob + ch]);
          }
          cnt[key] = (cnt[key] ?? 0) + 1;
        }
      }
    }
    final e1 = <int>[], e2 = <int>[], ew = <int>[];
    final ia = <Float64List>[], ib = <Float64List>[];
    cnt.forEach((key, N) {
      if (N < minPairPoints) return;
      final sa = sumA[key]!, sb = sumB[key]!;
      final ma = Float64List(3), mb = Float64List(3);
      for (var ch = 0; ch < 3; ch++) {
        ma[ch] = sa[ch] / N;
        mb[ch] = sb[ch] / N;
      }
                              
      if (ma[1] < srgbToLinear255(minLevel) ||
          mb[1] < srgbToLinear255(minLevel)) {
        return;
      }
      e1.add(key ~/ frameCount);
      e2.add(key % frameCount);
      ew.add(N);
      ia.add(ma);
      ib.add(mb);
    });
    if (ia.isEmpty) return out;                      

                                       
    final num = Float64List(frameCount), den = Float64List(frameCount);
    for (var ch = 0; ch < 3; ch++) {
      final g = Float64List(frameCount)..fillRange(0, frameCount, 1.0);
      for (var it = 0; it < iterations; it++) {
        num.fillRange(0, frameCount, 0);
        den.fillRange(0, frameCount, 0);
        for (var k = 0; k < ia.length; k++) {
          final a = e1[k], b = e2[k];
          final N = ew[k].toDouble();
          final meanA = ia[k][ch], meanB = ib[k][ch];
          num[a] += alpha * N * meanA * meanB * g[b] + beta * N;
          den[a] += alpha * N * meanA * meanA + beta * N;
          num[b] += alpha * N * meanB * meanA * g[a] + beta * N;
          den[b] += alpha * N * meanB * meanB + beta * N;
        }
        for (var f = 0; f < frameCount; f++) {
          if (den[f] > 0) g[f] = num[f] / den[f];
        }
      }
      for (var f = 0; f < frameCount; f++) {
        out.logGain[f * 3 + ch] = math.log(g[f] <= 0 ? 1.0 : g[f]);
      }
    }

                                                
    final mags = <double>[];
    for (var f = 0; f < frameCount; f++) {
      var m = 0.0;
      for (var ch = 0; ch < 3; ch++) {
        m += out.logGain[f * 3 + ch].abs();
      }
      mags.add(m / 3.0);
    }
    final sortedMag = List<double>.from(mags)..sort();
    final med = sortedMag[(sortedMag.length - 1) >> 1];
    final dev = (mags.map((x) => (x - med).abs()).toList()..sort());
    final mad = dev[(dev.length - 1) >> 1];
    final thr = med + 3 * 1.4826 * (mad <= 0 ? 1e-6 : mad);
    for (var f = 0; f < frameCount; f++) {
      if (mags[f] > thr) {
        out.banned[f] = 1;
        for (var ch = 0; ch < 3; ch++) {
          out.logGain[f * 3 + ch] = 0.0;
        }
      }
    }
    return out;
  }
}
