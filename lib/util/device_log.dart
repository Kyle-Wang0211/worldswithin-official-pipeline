                                                        
  
                                                                          
                                                                            
                                                                         
                                                        
                                                                      
                                                 
                                               
  
                                                                           
                                                                          
                                                                          
                                           

import 'dart:io';

import 'package:path_provider/path_provider.dart';

class DeviceLog {
  DeviceLog._();

  static File? _file;
  static bool _initFailed = false;
  static const int _maxBytes = 2 * 1024 * 1024;                    

                                                                         
                                                              
  static Future<void> init() async {
    if (_file != null || _initFailed) return;
    try {
      final dir = await getApplicationDocumentsDirectory();
      final f = File('${dir.path}/pw_device_log.txt');
      if (await f.exists() && (await f.length()) > _maxBytes) {
                                                                      
        final old = File('${dir.path}/pw_device_log.1.txt');
        if (await old.exists()) await old.delete();
        await f.rename(old.path);
      }
      _file = f;
      log('DeviceLog',
          'session start pid=$pid executable=${Platform.resolvedExecutable}');
    } catch (_) {
      _initFailed = true;
    }
  }

                                                                           
                                                                     
  static void log(String tag, String message) {
    final line = '${DateTime.now().toIso8601String()} [$tag] $message';
    // ignore: avoid_print
    print(line);
    final f = _file;
    if (f == null) return;
    try {
      f.writeAsStringSync('$line\n', mode: FileMode.append, flush: false);
    } catch (_) {
                                                                      
    }
  }
}
