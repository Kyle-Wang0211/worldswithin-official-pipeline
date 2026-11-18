                                                                         
                                                                           
                                                                             
                                                               
  
                                          
                                                                     
                                                                    
                                                                               
                                                                            
                                                                            

import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../official_aether_ffi.dart' show OfficialAetherFfi;

                         
class PwTelemetrySample {
  const PwTelemetrySample({
    required this.physFootprintMb,
    required this.peakFootprintMb,
    required this.thermalState,
  });

                                                                          
  final double physFootprintMb;

                                                
  final double peakFootprintMb;

                                                  
  final int thermalState;

  String get thermalName => switch (thermalState) {
    0 => 'nominal',
    1 => 'fair',
    2 => 'serious',
    3 => 'critical',
    _ => 'unknown',
  };

  @override
  String toString() =>
      'mem=${physFootprintMb.toStringAsFixed(0)}MB '
      'peak=${peakFootprintMb.toStringAsFixed(0)}MB '
      'thermal=$thermalName';
}

typedef _TelemetryC = Int32 Function(
  Pointer<Double> physMb,
  Pointer<Double> peakMb,
  Pointer<Int32> thermal,
);
typedef _TelemetryDart = int Function(
  Pointer<Double> physMb,
  Pointer<Double> peakMb,
  Pointer<Int32> thermal,
);

class PwTelemetry {
  PwTelemetry._();

  static _TelemetryDart? _fn;
  static bool _resolveFailed = false;

  static _TelemetryDart? _resolve() {
    if (_fn != null) return _fn;
    if (_resolveFailed) return null;
    try {
      _fn = OfficialAetherFfi.resolveLibraryForBindings()
          .lookupFunction<_TelemetryC, _TelemetryDart>('pwofficial_telemetry');
      return _fn;
    } catch (_) {
      _resolveFailed = true;                                              
      return null;
    }
  }

                                                                            
                                                 
  static PwTelemetrySample? sample() {
    final fn = _resolve();
    if (fn == null) return null;
    final phys = malloc<Double>();
    final peak = malloc<Double>();
    final therm = malloc<Int32>();
    try {
      phys.value = -1;
      peak.value = -1;
      therm.value = -1;
      fn(phys, peak, therm);
      return PwTelemetrySample(
        physFootprintMb: phys.value,
        peakFootprintMb: peak.value,
        thermalState: therm.value,
      );
    } finally {
      malloc.free(phys);
      malloc.free(peak);
      malloc.free(therm);
    }
  }
}
