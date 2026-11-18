import 'dart:convert';
import 'dart:io';

import 'package:crypto/crypto.dart';

import '../ui/scan_record.dart';

const productPreviewCoverCameraVersion = 'pw.shared-initial-camera.v3';

bool isProductPreviewCoverPath(String path) =>
    File(path).parent.path.split('/').last == '.pw_preview_covers';

String _platformPath(String path) {
  if ((Platform.isIOS || Platform.isMacOS) && path.startsWith('/var/')) {
    return '/private$path';
  }
  return path;
}

                                                                                 
File? productPreviewCoverFor(ScanRecord record) {
  try {
    final path = record.thumbnailPath, captureDir = record.captureDir;
                                                                             
                                                                              
    if (record.finalCoverSha256 != null &&
        record.finalCoverGlbSha256 != null &&
        record.finalCoverGlbSha256 == record.finalGlbSha256 &&
        path != null &&
        record.artifactPath != null) {
      final reference = Uri.parse(record.artifactPath!);
      final glb = File(
        reference.scheme == 'file'
            ? reference.toFilePath()
            : record.artifactPath!,
      );
      final image = File(path);
      if (FileSystemEntity.typeSync(path, followLinks: false) !=
              FileSystemEntityType.file ||
          image.resolveSymbolicLinksSync() != _platformPath(image.absolute.path) ||
          glb.parent.resolveSymbolicLinksSync() != _platformPath(image.parent.path) ||
          (image.uri.pathSegments.last != '${record.id}.png' &&
              image.uri.pathSegments.last !=
                  '${record.id}-${record.finalCoverSha256}.png') ||
          !glb.existsSync() ||
          glb.lengthSync() != record.finalGlbBytes)
        return null;
      final bytes = image.readAsBytesSync();
      const signature = [137, 80, 78, 71, 13, 10, 26, 10];
      if (bytes.length < 24 ||
          List.generate(8, (i) => bytes[i] == signature[i]).contains(false) ||
          sha256.convert(bytes).toString() != record.finalCoverSha256)
        return null;
      return image;
    }
    if (path == null ||
        captureDir == null ||
        !isProductPreviewCoverPath(path)) {
      return null;
    }
    final capture = Directory(captureDir).resolveSymbolicLinksSync();
    final directory = '$capture/.pw_preview_covers';
    final image = File(path);
    if (FileSystemEntity.typeSync(path, followLinks: false) !=
            FileSystemEntityType.file ||
        image.resolveSymbolicLinksSync() != _platformPath(image.absolute.path) ||
        _platformPath(image.parent.path) != directory) {
      return null;
    }
    final metadataFile = File('$path.json');
    if (FileSystemEntity.typeSync(metadataFile.path, followLinks: false) !=
        FileSystemEntityType.file) {
      return null;
    }
    final metadata = jsonDecode(metadataFile.readAsStringSync()) as Map;
    final identity = metadata['identity'] as String;
    final source = jsonDecode(identity) as List;
    final ownedCaptureSuffix =
        '/accounts/v1/${record.accountNamespace}/captures_official/${record.id}';
    final relocatedFinal =
        record.finalGlbSha256 != null &&
        source.length == 7 &&
        source[3] is String &&
        (source[3] as String).endsWith(ownedCaptureSuffix) &&
        capture.endsWith(ownedCaptureSuffix);
    if (source.length != 7 ||
        (source[0] != 'pw.shared-initial-camera.v2' &&
            source[0] != productPreviewCoverCameraVersion) ||
        source[1] != record.accountNamespace ||
        source[2] != record.id ||
        (source[3] != capture && !relocatedFinal) ||
        source[4] is! String ||
        (source[4] as String).isEmpty ||
        source[5] is! String ||
        source[6] is! String) {
      return null;
    }
    final selected = File('$directory/current.json');
    if (FileSystemEntity.typeSync(selected.path, followLinks: false) !=
            FileSystemEntityType.file ||
        jsonDecode(selected.readAsStringSync())['identity'] != identity) {
      return null;
    }
    final token = sha256.convert(utf8.encode(identity)).toString();
    final imageHash = sha256.convert(image.readAsBytesSync()).toString();
    if (metadata['image_sha256'] != imageHash ||
        image.uri.pathSegments.last != '$token-$imageHash.png') {
      return null;
    }
    final artifact = source[6] as String;
    final separator = artifact.lastIndexOf(':');
    final artifactPath = separator > 0
        ? artifact.substring(0, separator)
        : null;
    if (!relocatedFinal &&
        metadata['artifact_file'] != null &&
        productPreviewArtifactFileIdentity(artifactPath) !=
            metadata['artifact_file']) {
      return null;
    }
    if (record.artifactPath != null) {
      final reference = Uri.parse(record.artifactPath!);
      final finalPath = reference.scheme == 'file'
          ? reference.toFilePath()
          : record.artifactPath!;
      if (source[5] != 'glb' ||
          artifactPath == null ||
          !File(finalPath).existsSync() ||
          (!(relocatedFinal &&
                  artifact.endsWith(':${record.finalGlbSha256}') &&
                  artifactPath.endsWith(
                    '/accounts/v1/${record.accountNamespace}/scans_official/${record.id}.glb',
                  ) &&
                  File(finalPath).resolveSymbolicLinksSync().endsWith(
                    '/accounts/v1/${record.accountNamespace}/scans_official/${record.id}.glb',
                  )) &&
              File(finalPath).resolveSymbolicLinksSync() !=
                  File(artifactPath).resolveSymbolicLinksSync()) ||
          metadata['artifact_file'] == null) {
        return null;
      }
    }
    return image;
  } catch (_) {
    return null;
  }
}

String? productPreviewArtifactFileIdentity(String? path) {
  if (path == null) return null;
  try {
    final file = File(path);
    if (FileSystemEntity.typeSync(path, followLinks: false) !=
            FileSystemEntityType.file ||
        FileSystemEntity.typeSync(
              file.resolveSymbolicLinksSync(),
              followLinks: false,
            ) !=
            FileSystemEntityType.file) {
      return null;
    }
    final stat = file.statSync();
    return jsonEncode([
      file.resolveSymbolicLinksSync(),
      stat.size,
      stat.modified.microsecondsSinceEpoch,
      stat.changed.microsecondsSinceEpoch,
    ]);
  } catch (_) {
    return null;
  }
}

                                                                         
String? productPreviewCoverCameraVersionFor(ScanRecord record) {
  final image = productPreviewCoverFor(record);
  if (image == null) return null;
  if (record.finalCoverSha256 != null) return record.finalCoverCameraVersion;
  try {
    final metadata =
        jsonDecode(File('${image.path}.json').readAsStringSync()) as Map;
    return (jsonDecode(metadata['identity'] as String) as List).first as String;
  } catch (_) {
    return null;
  }
}
