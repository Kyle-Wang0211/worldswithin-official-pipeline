                                                      
   
                                                        
                                                   
                            
   
                        
                                                    
                                     
                                                         
                                                    
                                            
                                                    
                                                  
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'package:crypto/crypto.dart';
import 'package:pw_hevc/pwva.dart';
import 'package:pw_hevc/pwva_apple.dart';

import 'photo_archive_manifest.dart';

typedef PwvaMasterContinueCheck = FutureOr<bool> Function();

class PwvaMasterResult {
  const PwvaMasterResult({
    this.applicable = false,
    this.masteredNames = const <String>[],
    this.failedNames = const <String>[],
    this.paused = false,
  });

  final bool applicable;
  final List<String> masteredNames;
  final List<String> failedNames;
  final bool paused;
}

                                                  
class PwvaMasterManifest {
  PwvaMasterManifest._(this.streamSha256, this.entries);

  static const schema = 'pw_pwva_master_manifest_v1';
  static const relativePath = 'photos_hevc/master-manifest.json';

  final String streamSha256;

                                                 
  final Map<String, ({int frame, int sourceBytes, String sourceSha256})>
  entries;

  static Future<PwvaMasterManifest?> read(Directory captureDirectory) async {
    try {
      final file = File('${captureDirectory.path}/$relativePath');
      if (!await file.exists()) return null;
      final json =
          jsonDecode(await file.readAsString()) as Map<String, dynamic>;
      if (json['schema'] != schema) return null;
      final raw = json['entries'] as Map<String, dynamic>;
      final entries =
          <String, ({int frame, int sourceBytes, String sourceSha256})>{};
      for (final e in raw.entries) {
        final v = e.value as Map<String, dynamic>;
        entries[e.key] = (
          frame: v['frame'] as int,
          sourceBytes: v['source_bytes'] as int,
          sourceSha256: v['source_sha256'] as String,
        );
      }
      return PwvaMasterManifest._(json['stream_sha256'] as String, entries);
    } catch (_) {
      return null;
    }
  }

  static Future<Set<String>> readNames(Directory captureDirectory) async =>
      (await read(captureDirectory))?.entries.keys.toSet() ?? const <String>{};
}

class PwvaMasterTransaction {
  const PwvaMasterTransaction({this.canContinue});

  final PwvaMasterContinueCheck? canContinue;

                                       
  static const int _shaBatch = 12;

  Future<PwvaMasterResult> masterCapture(Directory captureDirectory) async {
    if (!(Platform.isIOS || Platform.isMacOS)) {
      return const PwvaMasterResult();
    }
    try {
      final hevcDir = Directory('${captureDirectory.path}/photos_hevc');
      final manifest = await _readArchiveManifest(hevcDir);
      if (manifest == null) return const PwvaMasterResult();
      final report = await _readJson(
        File('${hevcDir.path}/archive-report.json'),
      );
      if (report == null || report['status'] != 'finalized') {
        return const PwvaMasterResult();
      }
      final candidates = await PhotoArchiveManifest.loadCandidateNames(
        captureDirectory,
      );
      if (candidates.isEmpty) return const PwvaMasterResult();

      final entries = _readIndex(hevcDir);
      if (entries == null ||
          entries.length != manifest.frameCount ||
          entries.length != candidates.length) {
        return const PwvaMasterResult();
      }
      for (var i = 0; i < entries.length; i++) {
        if (entries[i].source != candidates[i] ||
            entries[i].sourceSha256 == null) {
          return const PwvaMasterResult();
        }
      }

      final committed = await PwvaMasterManifest.read(captureDirectory);
      if (committed != null) {
        return _reconcile(captureDirectory, committed);
      }

                                                      
      for (final name in candidates) {
        if (!await File('${captureDirectory.path}/photos_highres/$name')
            .exists()) {
          return const PwvaMasterResult();
        }
      }

                                        
      if (!await _canContinueNow()) return const PwvaMasterResult(paused: true);
      final stream = File('${hevcDir.path}/photos.hevc');
      if (!await stream.exists() ||
          await stream.length() != manifest.streamBytes ||
          await _sha256Of(stream) != manifest.streamSha256) {
        return const PwvaMasterResult();
      }

                                                        
      final sizes = <int>[];
      for (var start = 0; start < candidates.length; start += _shaBatch) {
        if (!await _canContinueNow()) {
          return const PwvaMasterResult(paused: true);
        }
        final batch = candidates.sublist(
          start,
          start + _shaBatch > candidates.length
              ? candidates.length
              : start + _shaBatch,
        );
        final root = captureDirectory.path;
        final checked = await Isolate.run(() {
          final out = <({int bytes, String sha})>[];
          for (final name in batch) {
            final f = File('$root/photos_highres/$name');
            out.add((
              bytes: f.lengthSync(),
              sha: sha256.convert(f.readAsBytesSync()).toString(),
            ));
          }
          return out;
        });
        for (var i = 0; i < batch.length; i++) {
          if (checked[i].sha != entries[start + i].sourceSha256) {
            return const PwvaMasterResult();
          }
          sizes.add(checked[i].bytes);
        }
      }

                                                
      final gopLast = <int>[];
      for (var i = 0; i < entries.length; i++) {
        if (i + 1 == entries.length || entries[i + 1].keyframe) gopLast.add(i);
      }
      final hevcPath = hevcDir.path;
      for (final last in gopLast) {
        if (!await _canContinueNow()) {
          return const PwvaMasterResult(paused: true);
        }
        final ok = await Isolate.run(() {
          try {
            final reader = PwvaReader(
              Directory(hevcPath),
              (au) => AppleHevcDecoder(
                width: _manifestWidth(hevcPath),
                height: _manifestHeight(hevcPath),
                keyframeAu: au,
              ),
            );
            reader.readFrameNv12(last);
            return true;
          } catch (_) {
            return false;
          }
        });
        if (!ok) return const PwvaMasterResult();
      }

                                       
      final entryJson = <String, Object?>{};
      for (var i = 0; i < candidates.length; i++) {
        entryJson[candidates[i]] = {
          'frame': entries[i].frame,
          'source_bytes': sizes[i],
          'source_sha256': entries[i].sourceSha256,
        };
      }
      final masterFile = File(
        '${captureDirectory.path}/${PwvaMasterManifest.relativePath}',
      );
      final tmp = File('${masterFile.path}.tmp');
      await tmp.writeAsString(
        const JsonEncoder.withIndent(' ').convert({
          'schema': PwvaMasterManifest.schema,
          'stream_sha256': manifest.streamSha256,
          'frame_count': entries.length,
          'verified_at': DateTime.now().toUtc().toIso8601String(),
          'entries': entryJson,
        }),
      );
      await tmp.rename(masterFile.path);

      final mastered = <String>[];
      var paused = false;
      for (final name in candidates) {
        if (!await _canContinueNow()) {
          paused = true;
          break;
        }
        await File('${captureDirectory.path}/photos_highres/$name').delete();
        mastered.add(name);
      }
      return PwvaMasterResult(
        applicable: true,
        masteredNames: List.unmodifiable(mastered),
        paused: paused,
      );
    } catch (_) {
                                           
      return const PwvaMasterResult();
    }
  }

                                        
                                        
  Future<PwvaMasterResult> _reconcile(
    Directory captureDirectory,
    PwvaMasterManifest committed,
  ) async {
    final mastered = <String>[];
    final failed = <String>[];
    var paused = false;
    for (final e in committed.entries.entries) {
      if (!await _canContinueNow()) {
        paused = true;
        break;
      }
      final source = File('${captureDirectory.path}/photos_highres/${e.key}');
      if (!await source.exists()) {
        mastered.add(e.key);
        continue;
      }
      if (await source.length() != e.value.sourceBytes ||
          await _sha256Of(source) != e.value.sourceSha256) {
        failed.add(e.key);
        continue;
      }
      if (!await _canContinueNow()) {
        paused = true;
        break;
      }
      await source.delete();
      mastered.add(e.key);
    }
    return PwvaMasterResult(
      applicable: true,
      masteredNames: List.unmodifiable(mastered),
      failedNames: List.unmodifiable(failed),
      paused: paused,
    );
  }

  Future<bool> _canContinueNow() async =>
      canContinue == null || await canContinue!();
}

class _ArchiveManifest {
  const _ArchiveManifest(this.frameCount, this.streamBytes, this.streamSha256);
  final int frameCount;
  final int streamBytes;
  final String streamSha256;
}

Future<_ArchiveManifest?> _readArchiveManifest(Directory hevcDir) async {
  final json = await _readJson(File('${hevcDir.path}/manifest.json'));
  if (json == null ||
      json['schema'] != 'pw_video_archive_v1' ||
      json['codec'] != 'hevc') {
    return null;
  }
  final frames = json['frame_count'];
  final bytes = json['stream_bytes'];
  final sha = json['stream_sha256'];
  if (frames is! int || frames <= 0 || bytes is! int || sha is! String) {
    return null;
  }
  return _ArchiveManifest(frames, bytes, sha);
}

List<PwvaIndexEntry>? _readIndex(Directory hevcDir) {
  try {
    return File('${hevcDir.path}/photos.pwvi')
        .readAsLinesSync()
        .map((l) => PwvaIndexEntry.fromJson(jsonDecode(l)))
        .toList();
  } catch (_) {
    return null;
  }
}

Future<Map<String, dynamic>?> _readJson(File file) async {
  try {
    if (!await file.exists()) return null;
    return jsonDecode(await file.readAsString()) as Map<String, dynamic>;
  } catch (_) {
    return null;
  }
}

int _manifestWidth(String hevcPath) => _resolution(hevcPath).$1;
int _manifestHeight(String hevcPath) => _resolution(hevcPath).$2;

(int, int) _resolution(String hevcPath) {
  final json = jsonDecode(File('$hevcPath/manifest.json').readAsStringSync());
  final parts = (json['resolution'] as String).split('x');
  return (int.parse(parts[0]), int.parse(parts[1]));
}

Future<String> _sha256Of(File file) async {
  final digest = await sha256.bind(file.openRead()).first;
  return digest.toString();
}
