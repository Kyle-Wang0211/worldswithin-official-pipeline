                                                          
  
                                                                 
                                                                 
                                                                                        
                                                            
  
                      
                                                    
                                                     
                    
                                                    
                                                             
                                            
                                     
                                                           
                
  
                                                                  
                                                     

import 'dart:async';
import 'dart:convert';
import 'dart:io';

                                       
                                           
                                  
double? percentileSorted(List<double> sorted, double p) {
  if (sorted.isEmpty) return null;
  final idx = (p * (sorted.length - 1)).round().clamp(0, sorted.length - 1);
  return sorted[idx];
}

class TelemetryWriter {
  TelemetryWriter._();
  static final TelemetryWriter instance = TelemetryWriter._();

  IOSink? _sink;
  bool _initFailed = false;
  Timer? _flushTimer;
  bool _dirtySinceFlush = false;
  int _written = 0;
  int _dropped = 0;
  int _degraded = 0;
  int _lastReportedDropped = 0;
  int _lastReportedDegraded = 0;
  String? _sinkError;
  bool _sinkErrorReported = false;

                                       
  final List<String> _pending = <String>[];
  static const int _maxPending = 1024;

                              
  int get writtenCount => _written;

                                          
  int get droppedCount => _dropped;

                                             
  int get degradedValueCount => _degraded;

                                                     
                                                          
                                                   
                                            
                                      
  Object? _jsonSafe(Object? value) {
    if (value is double && !value.isFinite) {
      _degraded++;
      return value.isNaN ? 'NaN' : (value > 0 ? 'Infinity' : '-Infinity');
    }
    if (value is List) return [for (final e in value) _jsonSafe(e)];
    if (value is Map) {
      return value.map((k, v) => MapEntry(k.toString(), _jsonSafe(v)));
    }
    if (value is num || value is String || value is bool || value == null) {
      return value;
    }
    _degraded++;
    return '$value';
  }

                                             
                                                   
                                    
  void _appendHealthLineIfNeeded(IOSink sink) {
    final sinkErrorPending = _sinkError != null && !_sinkErrorReported;
    if (_dropped == _lastReportedDropped &&
        _degraded == _lastReportedDegraded &&
        !sinkErrorPending) {
      return;
    }
    _lastReportedDropped = _dropped;
    _lastReportedDegraded = _degraded;
    _sinkErrorReported = _sinkError != null;
    final line =
        '{"t":${DateTime.now().millisecondsSinceEpoch},'
        '"type":"telemetry_writer_health",'
        '"dropped_total":$_dropped,'
        '"degraded_values_total":$_degraded,'
        '"sink_error":${jsonEncode(_sinkError)}}\n';
    sink.add(utf8.encode(line));
    _written++;
  }

                                         
                                                                     
  Future<void> init(String filePath) async {
    if (_sink != null || _initFailed) return;
    try {
      final file = File(filePath);
      await file.parent.create(recursive: true);
      final sink = file.openWrite(mode: FileMode.append);
                                    
      unawaited(
        sink.done.catchError((Object e) {
          _sinkError ??= e.runtimeType.toString();
        }),
      );
      _sink = sink;
                                
      if (_pending.isNotEmpty) {
        for (final line in _pending) {
          sink.add(utf8.encode(line));
          _written++;
        }
        _pending.clear();
        _dirtySinceFlush = true;
        _scheduleFlush();
      }
    } catch (_) {
      _initFailed = true;
      _dropped += _pending.length;
      _pending.clear();
    }
  }

                                                          
                                                           
                                        
  void event(String type, [Map<String, Object?> fields = const {}]) {
    try {
      final map = <String, Object?>{
        't': DateTime.now().millisecondsSinceEpoch,
        'type': type,
        ...fields,
      };
      final line = '${jsonEncode(_jsonSafe(map))}\n';
      final sink = _sink;
      if (sink == null) {
        if (_initFailed || _pending.length >= _maxPending) {
          _dropped++;
          return;
        }
        _pending.add(line);
        return;
      }
      sink.add(utf8.encode(line));             
      _written++;
      _appendHealthLineIfNeeded(sink);
      _dirtySinceFlush = true;
      _scheduleFlush();
    } catch (_) {
                                        
      _dropped++;
    }
  }

                                      
  void _scheduleFlush() {
    if (_flushTimer != null) return;
    _flushTimer = Timer(const Duration(seconds: 2), () {
      _flushTimer = null;
      if (!_dirtySinceFlush) return;
      _dirtySinceFlush = false;
      try {
        _sink?.flush().catchError((Object _) {});
      } catch (_) {}
    });
  }

                               
  Future<void> flush() async {
    _flushTimer?.cancel();
    _flushTimer = null;
    _dirtySinceFlush = false;
    try {
      await _sink?.flush();
    } catch (_) {}
  }

                                          
  Future<void> close() async {
    _flushTimer?.cancel();
    _flushTimer = null;
    final sink = _sink;
    _sink = null;
    if (sink != null) {
      try {
        await sink.flush();
        await sink.close();
      } catch (_) {}
    }
  }
}
