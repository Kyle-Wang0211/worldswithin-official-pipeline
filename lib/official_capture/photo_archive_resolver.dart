import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';
import 'package:pw_hevc/pwva.dart';
import 'package:pw_hevc/pwva_apple.dart';

import 'photo_archive_codec.dart';
import 'photo_archive_manifest.dart';
import 'photo_archive_policy.dart';
import 'pwva_master.dart';

                                                                               
class PhotoArchiveResolver {
  const PhotoArchiveResolver({
    required this.codec,
    this.codecsByName = const <String, PhotoArchiveCodec>{},
  });

  final PhotoArchiveCodec codec;
  final Map<String, PhotoArchiveCodec> codecsByName;

  Future<File?> resolveJpeg({
    required Directory captureDirectory,
    required String highresFilename,
    required Directory cacheDirectory,
  }) async {
    final candidates = await PhotoArchiveManifest.loadCandidateNames(
      captureDirectory,
    );
    if (!candidates.contains(highresFilename)) return null;

    final source = File(
      '${captureDirectory.path}/photos_highres/$highresFilename',
    );
    if (await source.exists()) return source;

    final manifest = await PhotoArchiveManifest.read(captureDirectory);
    final entry = manifest?.entries[highresFilename];
    if (manifest == null || entry == null) {
                                                       
      return _resolveFromPwvaMaster(
        captureDirectory: captureDirectory,
        highresFilename: highresFilename,
        cacheDirectory: cacheDirectory,
      );
    }
    final selectedCodec = codecsByName.isEmpty
        ? codec
        : codecsByName[manifest.codec];
    if (selectedCodec == null || !selectedCodec.isSupported) return null;
    final expectedArchiveRelativePath =
        'photos_highres/$highresFilename${_archiveSuffix(manifest.codec)}';
    if (entry.sourceRelativePath != 'photos_highres/$highresFilename' ||
        entry.archiveRelativePath != expectedArchiveRelativePath) {
      return null;
    }
    final archive = File(
      '${captureDirectory.path}/${entry.archiveRelativePath}',
    );
    if (!await _matches(
      archive,
      length: entry.archiveBytes,
      sha256Hex: entry.archiveSha256,
    )) {
      return null;
    }

    await cacheDirectory.create(recursive: true);
    final cached = File(
      '${cacheDirectory.path}/${entry.sourceSha256}_$highresFilename',
    );
    if (await _matches(
      cached,
      length: entry.sourceBytes,
      sha256Hex: entry.sourceSha256,
    )) {
      return cached;
    }
    final temporary = File('${cached.path}.tmp');
    try {
      await _deleteIfPresent(cached);
      await _deleteIfPresent(temporary);
      await selectedCodec.reconstructJpeg(
        sourceJxl: archive,
        destinationJpeg: temporary,
      );
      if (!await _matches(
        temporary,
        length: entry.sourceBytes,
        sha256Hex: entry.sourceSha256,
      )) {
        await _deleteIfPresent(temporary);
        return null;
      }
      await temporary.rename(cached.path);
      return cached;
    } catch (_) {
      await _deleteIfPresent(temporary);
      return null;
    }
  }

                                                                    
     
                                                              
                                                                   
                                                              
                                     
     
                                                                   
                                                          
                                 
     
                                                                             
                                                
  Future<({File file, int width, int height})?> resolveNv12({
    required Directory captureDirectory,
    required String highresFilename,
    required Directory cacheDirectory,
  }) async {
    try {
      final master = await PwvaMasterManifest.read(captureDirectory);
      final entry = master?.entries[highresFilename];
      if (master == null || entry == null) return null;
      final reader = debugNv12Reader;
      if (reader == null && !_hevcDecoderAvailable()) return null;
      final hevcPath = '${captureDirectory.path}/photos_hevc';
      final (width, height) = _pwvaResolution(hevcPath);
      if (width <= 0 || height <= 0) return null;
      final expectedBytes = width * height + (width * height ~/ 2);

      await cacheDirectory.create(recursive: true);
      final cached = File(
        '${cacheDirectory.path}/${entry.sourceSha256}_$highresFilename.nv12',
      );
      if (await cached.exists() && await cached.length() == expectedBytes) {
        return (file: cached, width: width, height: height);
      }
      final temporary = File('${cached.path}.tmp');
      await _deleteIfPresent(cached);
      await _deleteIfPresent(temporary);
      final frame = entry.frame;
      final outPath = temporary.path;
      final ok = reader != null
          ? _writeNv12(reader(Directory(hevcPath), frame), outPath)
          : await Isolate.run(() {
              try {
                final r = PwvaReader(
                  Directory(hevcPath),
                  (au) => _makeHevcDecoder(hevcPath, au),
                );
                final decoded = r.readFrameNv12(frame);
                return _writeNv12(
                  (y: decoded.y, uv: decoded.uv, width: r.width, height: r.height),
                  outPath,
                );
              } catch (_) {
                return false;
              }
            });
      if (!ok ||
          !await temporary.exists() ||
          await temporary.length() != expectedBytes) {
        await _deleteIfPresent(temporary);
        return null;
      }
      await temporary.rename(cached.path);
      return (file: cached, width: width, height: height);
    } catch (_) {
      return null;
    }
  }

                                                               
                                                     
  static PwvaNv12FrameReader? debugNv12Reader;
}

                                                      
typedef PwvaNv12FrameReader =
    ({Uint8List y, Uint8List uv, int width, int height}) Function(
      Directory hevcDirectory,
      int frame,
    );

                                                                  
                                         
bool _hevcDecoderAvailable() => Platform.isIOS || Platform.isMacOS;

HevcFrameDecoder _makeHevcDecoder(String hevcPath, Uint8List keyframeAu) =>
    AppleHevcDecoder(
      width: _pwvaWidth(hevcPath),
      height: _pwvaHeight(hevcPath),
      keyframeAu: keyframeAu,
    );

                                                              
bool _writeNv12(
  ({Uint8List y, Uint8List uv, int width, int height}) planes,
  String path,
) {
  final out = File(path).openSync(mode: FileMode.writeOnly);
  try {
    out.writeFromSync(planes.y);
    out.writeFromSync(planes.uv);
  } finally {
    out.closeSync();
  }
  return true;
}

                                                   
                                           
                               
Future<File?> _resolveFromPwvaMaster({
  required Directory captureDirectory,
  required String highresFilename,
  required Directory cacheDirectory,
}) async {
  if (!(Platform.isIOS || Platform.isMacOS)) return null;
  try {
    final master = await PwvaMasterManifest.read(captureDirectory);
    final entry = master?.entries[highresFilename];
    if (master == null || entry == null) return null;
    await cacheDirectory.create(recursive: true);
    final cached = File(
      '${cacheDirectory.path}/${entry.sourceSha256}_$highresFilename',
    );
    if (await cached.exists() && await cached.length() > 0) return cached;
    final temporary = File('${cached.path}.tmp');
    await _deleteIfPresent(cached);
    await _deleteIfPresent(temporary);
    final hevcPath = '${captureDirectory.path}/photos_hevc';
    final frame = entry.frame;
    final outPath = temporary.path;
    final ok = await Isolate.run(() {
      try {
        final reader = PwvaReader(
          Directory(hevcPath),
          (au) => AppleHevcDecoder(
            width: _pwvaWidth(hevcPath),
            height: _pwvaHeight(hevcPath),
            keyframeAu: au,
          ),
        );
        final decoded = reader.readFrameNv12(frame);
        writeNv12JpegFile(
          decoded.y,
          decoded.uv,
          width: reader.width,
          height: reader.height,
          path: outPath,
        );
        return true;
      } catch (_) {
        return false;
      }
    });
    if (!ok || !await temporary.exists() || await temporary.length() == 0) {
      await _deleteIfPresent(temporary);
      return null;
    }
    await temporary.rename(cached.path);
    return cached;
  } catch (_) {
    return null;
  }
}

int _pwvaWidth(String hevcPath) => _pwvaResolution(hevcPath).$1;
int _pwvaHeight(String hevcPath) => _pwvaResolution(hevcPath).$2;

(int, int) _pwvaResolution(String hevcPath) {
                                                  
  final file = File('$hevcPath/manifest.json');
  final json = file.readAsStringSync();
  final res = (RegExp(r'"resolution":\s*"(\d+)x(\d+)"').firstMatch(json))!;
  return (int.parse(res.group(1)!), int.parse(res.group(2)!));
}

String _archiveSuffix(String codec) =>
    codec == PhotoArchivePolicy.leptonCodec ? '.lep' : '.jxl';

Future<bool> _matches(
  File file, {
  required int length,
  required String sha256Hex,
}) async {
  try {
    if (!await file.exists() || await file.length() != length) return false;
    final digest = await sha256.bind(file.openRead()).first;
    return digest.toString() == sha256Hex;
  } on FileSystemException {
    return false;
  }
}

Future<void> _deleteIfPresent(File file) async {
  try {
    if (await file.exists()) await file.delete();
  } on FileSystemException {
                                                                  
  }
}
