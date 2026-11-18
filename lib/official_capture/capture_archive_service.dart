                                                     
   
                                               
                                            
                                          
                                                 
                                       
                                         
                                                
   
                                          
                                               
                                                
                
   
                                 
                                   
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'package:crypto/crypto.dart';
import 'package:pw_hevc/pwva.dart';
import 'package:pw_hevc/pwva_apple.dart';

class CaptureArchiveService {
  CaptureArchiveService._()
    : _worker = null,
      _timeout = const Duration(minutes: 5);
  static final CaptureArchiveService instance = CaptureArchiveService._();

                                                                                
  CaptureArchiveService.forTesting({
    required Future<Map<String, Object?>?> Function(String) worker,
    Duration timeout = const Duration(minutes: 5),
  }) : _worker = worker,
       _timeout = timeout;

  final Future<Map<String, Object?>?> Function(String)? _worker;
  final Duration _timeout;
  final _running = <String, Set<Future<void>>>{};

                                              
  static bool enabled = true;
  static const int _gop = 8;
  static const double _quality = 0.65;

  void enqueueHighresStill(String jpegPath, {double? triggerTimestamp}) {}
  void abandon() {}                                                           
  Future<Map<String, Object?>?> finalizeArchive() async => null;

  static String _canonicalCapture(String path) =>
      Directory(path).absolute.resolveSymbolicLinksSync();

                                                                             
                                                                              
  Future<void> waitForCaptureIdle(String captureRoot) async {
    final root = _canonicalCapture(captureRoot);
    while (true) {
      final jobs = _running[root]?.toList() ?? const <Future<void>>[];
      if (jobs.isEmpty) return;
      await Future.wait(jobs);
    }
  }

  Future<Map<String, Object?>?> archiveCapture(String captureRoot) async {
    if (!enabled ||
        (_worker == null && !(Platform.isIOS || Platform.isMacOS))) {
      return null;
    }
    String root;
    try {
      root = _canonicalCapture(captureRoot);
    } catch (_) {
      return null;
    }
                                                                
    final exited = Completer<void>();
    final token = exited.future;
    (_running[root] ??= {}).add(token);
    var launched = false;
    try {
      final _ArchiveJob job;
      if (_worker case final worker?) {
        final future = Future<Map<String, Object?>?>.sync(() => worker(root));
        job = _ArchiveJob(
          future,
          future.then<void>((_) {}, onError: (Object _, StackTrace _) {}),
        );
      } else {
        job = _spawnArchive(root);
      }
      launched = true;
      unawaited(
        job.exited.then((_) {
          final jobs = _running[root];
          jobs?.remove(token);
          if (jobs?.isEmpty == true) _running.remove(root);
          exited.complete();
        }),
      );
      return await job.report.timeout(_timeout);
    } catch (_) {
                                                                             
                                                                             
      if (!launched) {
        final jobs = _running[root];
        jobs?.remove(token);
        if (jobs?.isEmpty == true) _running.remove(root);
        exited.complete();
      }
      return null;
    }
  }
}

final class _ArchiveJob {
  const _ArchiveJob(this.report, this.exited);
  final Future<Map<String, Object?>?> report;
  final Future<void> exited;
}

_ArchiveJob _spawnArchive(String root) {
  final report = Completer<Map<String, Object?>?>();
  final exited = Completer<void>();
  final events = ReceivePort();
  events.listen((message) {
    if (message is SendPort) {
      message.send({'root': root, 'reply': events.sendPort});
    } else if (message is Map) {
      if (!report.isCompleted) {
        report.complete(Map<String, Object?>.from(message));
      }
    } else if (message == null) {
                                                                              
      if (!report.isCompleted) report.complete(null);
      if (!exited.isCompleted) exited.complete();
      events.close();
    } else if (message is List) {
                                                                             
      if (!report.isCompleted) report.complete(null);
    }
  });
  unawaited(
    Isolate.spawn(
      _batchArchiveIsolate,
      events.sendPort,
      onExit: events.sendPort,
      onError: events.sendPort,
      errorsAreFatal: true,
    ).then<void>(
      (_) {},
      onError: (Object _, StackTrace _) {
        if (!report.isCompleted) report.complete(null);
        if (!exited.isCompleted) {
          exited.complete();                                   
        }
        events.close();
      },
    ),
  );
  return _ArchiveJob(report.future, exited.future);
}

                                              

Future<void> _batchArchiveIsolate(SendPort ready) async {
  final inbox = ReceivePort();
  ready.send(inbox.sendPort);
  final m = await inbox.first as Map;
  final root = m['root'] as String;
  final reply = m['reply'] as SendPort;
  final outDir = Directory('$root/photos_hevc');
  final started = DateTime.now();
  Map<String, Object?> report;
  try {
    report = await _archive(root, outDir);
  } catch (e, st) {
    report = {'status': 'failed', 'error': '$e'};
    try {
      outDir.createSync(recursive: true);
      File('${outDir.path}/archive-error.json').writeAsStringSync(
        jsonEncode({
          'schema': 'pw_capture_archive_error_v1',
          'error': '$e',
          'stack': '$st',
        }),
      );
    } catch (_) {}
  }
  report['elapsed_s'] = DateTime.now().difference(started).inSeconds;
  try {
    File('${outDir.path}/archive-report.json')
        .writeAsStringSync(jsonEncode(report));
  } catch (_) {}
  reply.send(report);
  inbox.close();
}

Future<Map<String, Object?>> _archive(String root, Directory outDir) async {
  final bundleFile = File('$root/official_photo_bundle.json');
  final bundle =
      jsonDecode(await bundleFile.readAsString()) as Map<String, dynamic>;
  final frames = (bundle['frames'] as List).cast<Map<String, dynamic>>();
  if (frames.isEmpty) return {'status': 'empty', 'frames': 0};

                     
  if (outDir.existsSync()) outDir.deleteSync(recursive: true);
  outDir.createSync(recursive: true);

  final first = '$root/photos_highres/${frames.first['highresFilename']}';
  final size = probeJpegSize(first);
  final encoder = AppleHevcEncoder(
    width: size.width,
    height: size.height,
    gop: CaptureArchiveService._gop,
    quality: CaptureArchiveService._quality,
  );
  final writer = PwvaWriter(
    encoder,
    outDir,
    width: size.width,
    height: size.height,
    gop: CaptureArchiveService._gop,
  );
  var done = 0;
  final recordedSha = <String, String>{};
  for (final f in frames) {
    final name = f['highresFilename'] as String;
    final path = '$root/photos_highres/$name';
    final sha = sha256.convert(await File(path).readAsBytes()).toString();
    recordedSha[name] = sha;
    final encoded = encoder.encodeJpegFile(
      path,
      ptsMs: done * 300,
      durationMs: 300,
    );
    writer.addEncodedFrame(
      encoded,
      source: name,
      trigger: (f['triggerTimestamp'] as num?)?.toDouble(),
      sourceSha256: sha,
    );
    done++;
  }
  await writer.finalize(
    captureId: root.split('/').last,
    extra: 'p1_1_finalize_batch_curated',
  );

                         
                                                
                                             
                                                           
                                                
                                        
                                                 
                                  
                                   
  final changed = <String>[];
  for (final entry in recordedSha.entries) {
    final file = File('$root/photos_highres/${entry.key}');
    if (!file.existsSync()) {
      changed.add('${entry.key}:missing');
      continue;
    }
    final now = sha256.convert(await file.readAsBytes()).toString();
    if (now != entry.value) changed.add(entry.key);
  }
  var decodeOk = true;
  Object? decodeError;
  try {
    _decodeSmokeTest(outDir, size.width, size.height);
  } catch (e) {
    decodeOk = false;
    decodeError = e;
  }
  if (changed.isNotEmpty || !decodeOk) {
    File('${outDir.path}/archive-error.json').writeAsStringSync(
      jsonEncode({
        'schema': 'pw_capture_archive_error_v1',
        'error': changed.isNotEmpty
            ? 'source_changed_during_encode'
            : 'undecodable',
        'changed_frames': changed.take(10).toList(),
        'changed_count': changed.length,
        if (decodeError != null) 'decode_error': '$decodeError',
      }),
    );
    return {
      'status': changed.isNotEmpty
          ? 'failed_source_changed'
          : 'failed_undecodable',
      'frames': done,
      'changed_count': changed.length,
    };
  }
  final manifest = jsonDecode(
    File('${outDir.path}/manifest.json').readAsStringSync(),
  );
  return {
    'schema': 'pw_capture_archive_report_v1',
    'status': 'finalized',
    'frames': done,
    'stream_bytes': manifest['stream_bytes'],
    'stream_sha256': manifest['stream_sha256'],
    'verified_frames': done,
  };
}

                                    
void _decodeSmokeTest(Directory outDir, int w, int h) {
  final reader = PwvaReader(
    outDir,
    (au) => AppleHevcDecoder(width: w, height: h, keyframeAu: au),
  );
  final frame = reader.readFrameNv12(0);
  if (frame.y.length != w * h) {
    throw StateError('decoded plane size mismatch');
  }
}
