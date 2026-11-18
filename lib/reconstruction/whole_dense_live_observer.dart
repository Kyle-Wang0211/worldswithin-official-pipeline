import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import '../dense/reconstruction_depth_preview.dart';

                                                                             
                                                                              
final class WholeDenseLiveObserver {
  static SendPort? eventPort;
  static String? workRoot;
  static bool visuals = false;
                                                                             
                                                 
  static Future<void> Function(Map<String, Object?> receipt)?
  onDepthReadersDrained;

  static Future<WholeDenseObservation?> start(
    Map<String, Object?> config,
  ) async {
    final port = eventPort;
    if (port == null || workRoot == null) return null;
    try {
      final messages = ReceivePort();
      final exited = ReceivePort();
      final ready = Completer<SendPort>();
      final drained = Completer<void>();
      final exit = Completer<void>();
      messages.listen((message) {
        if (message is SendPort && !ready.isCompleted) ready.complete(message);
        if (message == 'drained' && !drained.isCompleted) drained.complete();
      });
      exited.listen((_) {
        if (!exit.isCompleted) exit.complete();
        if (!ready.isCompleted) {
          ready.completeError(StateError('Observer exited before ready'));
        }
      });
      await Isolate.spawn(
        _observe,
        {
          ...config,
          'port': port,
          'work_root': workRoot,
          'control_reply': messages.sendPort,
        },
        onExit: exited.sendPort,
        debugName: 'reconstruction-display-observer',
      );
      return WholeDenseObservation._(
        await ready.future,
        drained,
        exit.future,
        messages,
        exited,
      );
    } catch (_) {
      return null;                                                      
    }
  }
}

                                                                          
final class WholeDenseObservation {
  WholeDenseObservation._(
    this._control,
    this._drained,
    this._exit,
    this._messages,
    this._exited,
  );
  final SendPort _control;
  final Completer<void> _drained;
  final Future<void> _exit;
  final ReceivePort _messages, _exited;
  Future<void>? _stopping;
  Future<void> stop() => _stopping ??= _stop();
  Future<void> _stop() async {
    _control.send('stop');
    await _exit;
                                                                               
                                                                              
    await Future<void>.delayed(Duration.zero);
    _messages.close();
    _exited.close();
    if (!_drained.isCompleted) {
      throw StateError(
        'Observer exited without depth-reader drain acknowledgement',
      );
    }
  }
}

                                                                               
                                                                                 
final class WholeDenseObservedProgress {
  WholeDenseObservedProgress(this.count, this.runId, this.processId);
  final int count, processId;
  final String runId;
  int _rank = -1, _done = -1;
  String? _phase;
  (String, int, int)? accept(Object? value) {
    if (value is! Map ||
        value['schema'] != 'reconstruction.m0.memory/1' ||
        value['run_id'] != runId ||
        value['pid'] != processId) {
      return null;
    }
    final label = value['stage'];
    if (label is! String) return null;
    var rank = -1, done = 0, total = count;
    var phase = '';
    final feature = RegExp(r'^feature_request_(\d+)$').firstMatch(label);
    final mvs = RegExp(r'^MVS_([01])_([1-5])_(-?\d+)$').firstMatch(label);
    if (feature != null) {
      final i = int.parse(feature[1]!);
      if (i >= count) return null;
      rank = 0;
      phase = 'whole_features';
      done = i;
    } else if (mvs != null) {
      final pass = int.parse(mvs[1]!), event = int.parse(mvs[2]!);
      final ref = int.parse(mvs[3]!);
      if ((event == 1 || event == 2 || event == 4) &&
          (ref < 0 || ref >= count)) {
        return null;
      }
      if ((event == 3 || event == 5) && ref != -1) return null;
      rank = 1 + pass;
      phase = pass == 0 ? 'whole_depth_low' : 'whole_depth_high';
      done = event == 3
          ? count
          : event == 2
          ? ref + 1
          : ref < 0
          ? 0
          : ref;
    } else if (label == 'PRODUCTION_CER_ENTER' ||
        label == 'PRODUCTION_CER_RETURN') {
      rank = 3;
      phase = 'whole_consistency';
      total = 1;
      done = label.endsWith('RETURN') ? 1 : 0;
    } else if (label == 'WHOLE_DAWN_BEFORE_ADOPTED_GPU_FUSION' ||
        label == 'WHOLE_DAWN_GPU_FUSION_RETURNED') {
      rank = 4;
      phase = 'whole_fusion';
      total = count;
      done = label.endsWith('RETURNED') ? count : 0;
    } else {
      final emit = RegExp(r'^FUSION20_EMIT_(\d+)$').firstMatch(label);
      if (emit == null) return null;
      final i = int.parse(emit[1]!);
      if (i >= count) return null;
                                                  
      rank = 4;
      phase = 'whole_fusion';
      done = i;
    }
    if (rank < _rank || (rank == _rank && done <= _done && phase == _phase)) {
      return null;
    }
    _rank = rank;
    _done = done;
    _phase = phase;
    return (phase, done, total);
  }
}

void _observe(Map<String, Object?> config) async {
  final port = config['port'] as SendPort;
  final reply = config['control_reply'] as SendPort;
  final control = ReceivePort();
  var stopping = false;
  control.listen((message) {
    if (message == 'stop') stopping = true;
  });
  reply.send(control.sendPort);
  final root = config['work_root'] as String;
  final count = config['count'] as int;
  final progress = WholeDenseObservedProgress(
    count,
    config['m0_run_id'] as String,
    config['pid'] as int,
  );
  final file = File(config['trace'] as String);
  var offset = 0;
  var remainder = '';
  final previewedPasses = <int>{};
                                                                    
  while (true) {
    try {
      final handle = file.openSync();
      List<int> bytes;
      try {
        if (handle.lengthSync() < offset) {
          control.close();
          return;
        }
        handle.setPositionSync(offset);
        bytes = handle.readSync(256 * 1024);
        offset += bytes.length;
      } finally {
        handle.closeSync();
      }
      final lines = (remainder + utf8.decode(bytes, allowMalformed: true))
          .split('\n');
      remainder = lines.removeLast();
      if (remainder.length > 16384) {
        control.close();
        return;
      }
                                                                         
                                                                             
      final readyPasses = <int>{};
      for (final line in lines) {
        if (line.isEmpty || line.length > 16384) continue;
        try {
          final row = jsonDecode(line);
          final event = progress.accept(row);
          if (event != null) {
                                                                                 
            // Fast phases may begin and finish within this single trace read.
            port.send({
              'kind': 'dense_progress',
              'work_root': root,
              'phase': event.$1,
              'done': event.$2,
              'total': event.$3,
              'source_mono_ns': row['mono_ns'],
              'source_wall_ns': row['wall_ns'],
            });
          }
                                                                          
                                                                            
          if (row is! Map ||
              row['schema'] != 'reconstruction.m0.memory/1' ||
              row['run_id'] != config['m0_run_id'] ||
              row['pid'] != config['pid']) {
            continue;
          }
          final stage = row['stage'];
          if (stage == 'MVS_0_3_-1') readyPasses.add(0);
          if (stage == 'MVS_1_3_-1') readyPasses.add(1);
        } catch (_) {
                                                               
        }
      }
                                                                            
      for (final pass in readyPasses) {
        if (config['visuals'] != true || previewedPasses.contains(pass)) {
          continue;
        }
        final width = pass == 0 ? 768 : 1536;
        final artifact = await writeWholeDepthPreview(
          depthPath:
              '${config['mvs_root']}/${pass == 0 ? 'low' : 'high'}/depth.f32',
          cameraPath: config['camera_path'] as String,
          frameIndex: count - 1,
          frameCount: count,
          width: width,
          height: width * 3 ~/ 4,
          outputPath: '$root/display_observations/depth_$pass.ply',
        );
        if (artifact != null) {
          previewedPasses.add(pass);
          port.send({
            'kind': 'preview',
            'work_root': root,
            'value': artifact.toJson(),
          });
        }
      }
    } catch (_) {
                                                                       
    }
    if (stopping) {
                                                                              
                                                                              
      if (!file.existsSync()) {
        control.close();
        return;                                                           
      }
      if (offset < file.lengthSync()) continue;
      reply.send('drained');
      control.close();
      return;
    }
    await Future<void>.delayed(const Duration(seconds: 1));
  }
}
