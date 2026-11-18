import '../../dense/reconstruction_generated_consumer_leases.dart';
import 'dart:convert';
import 'dart:io';

import 'package:crypto/crypto.dart';
import 'package:flutter/foundation.dart';
import 'package:pw_shared_viewer/pw_shared_viewer.dart';
import '../scan_record.dart';
import '../../point_cloud_data/persisted_preview_cover.dart';

export '../../point_cloud_data/persisted_preview_cover.dart'
    show
        isProductPreviewCoverPath,
        productPreviewCoverFor,
        productPreviewCoverCameraVersion,
        productPreviewCoverCameraVersionFor;

                                                                                 
                                                                      
final class ProductPreviewCoverSource {
  ProductPreviewCoverSource({
    required this.accountNamespace,
    required this.workId,
    required String captureDir,
    required this.runId,
    required this.stage,
    required this.artifactIdentity,
  }) : captureDir = _canonicalCaptureDir(captureDir);
  final String? accountNamespace;
  final String workId, captureDir, runId, stage, artifactIdentity;
  String get identity => jsonEncode([
    productPreviewCoverCameraVersion,
    accountNamespace,
    workId,
    captureDir,
    runId,
    stage,
    artifactIdentity,
  ]);
  String get token => sha256.convert(utf8.encode(identity)).toString();
}

                                                                                
                                                                                
final class ProductPreviewCoverWriter {
  String? _selected;
  int _generation = 0;
  final Set<String> _pending = {};
  final Set<String> _attached = {};

  void select(ProductPreviewCoverSource source) {
    if (ReconstructionGeneratedConsumerLeases.instance.captureOffersClosed(
      source.captureDir,
    )) {
      return;
    }
    if (_selected == source.identity) return;
    _selected = source.identity;
    _generation++;
                                                                              
    try {
      final capture = Directory(source.captureDir);
      if (capture.resolveSymbolicLinksSync() != capture.absolute.path) return;
      final directory = Directory('${source.captureDir}/.pw_preview_covers');
      if (FileSystemEntity.typeSync(directory.path, followLinks: false) ==
          FileSystemEntityType.link) {
        return;
      }
      directory.createSync(recursive: true);
      final pending = File('${directory.path}/.selection-${source.token}.json');
      pending.writeAsStringSync(
        jsonEncode({'identity': source.identity}),
        flush: true,
      );
      pending.renameSync('${directory.path}/current.json');
    } catch (_) {
                                                                        
    }
  }

  void invalidate() {
    _selected = null;
    _generation++;
  }

  bool isSelected(ProductPreviewCoverSource source) =>
      _selected == source.identity;

  Future<String?> capture({
    required ProductPreviewCoverSource source,
    required SharedViewerController controller,
    required bool Function() isCurrent,
    required Future<bool> Function(String path) attach,
    bool recapture = false,
  }) async {
    final generation = _generation;
    final identity = source.identity;
    bool current() =>
        generation == _generation && _selected == identity && isCurrent();
    if (!current() ||
        _pending.contains(identity) ||
        (_attached.contains(identity) && !recapture)) {
      return null;
    }
    ReconstructionGeneratedReadLease? read;
    ReconstructionGeneratedReadLease? captureWrite;
    _pending.add(identity);
    Directory? staged;
    var attached = false;
    var stagingCleaned = false;
    try {
      captureWrite = ReconstructionGeneratedConsumerLeases.instance.acquireCapture(
        source.captureDir,
        'cover:$identity',
      );
      read = ReconstructionGeneratedConsumerLeases.instance.acquireIdentity(
        source.artifactIdentity,
        'cover:$identity',
      );
      final result = await controller.capture(
        type: 'image/png',
        canonicalCamera: true,
      );
      if (!current() ||
          result is! String ||
          !result.startsWith('data:image/png;base64,')) {
        return null;
      }
      final bytes = base64Decode(
        result.substring('data:image/png;base64,'.length),
      );
      const signature = [137, 80, 78, 71, 13, 10, 26, 10];
      if (bytes.length < 24 ||
          List.generate(8, (i) => bytes[i] == signature[i]).contains(false)) {
        return null;
      }
      final capture = Directory(source.captureDir);
      if (!current() ||
          !capture.existsSync() ||
          capture.resolveSymbolicLinksSync() != capture.absolute.path) {
        return null;
      }
      final directory = Directory('${capture.path}/.pw_preview_covers');
      if (FileSystemEntity.typeSync(directory.path, followLinks: false) ==
          FileSystemEntityType.link) {
        return null;
      }
      await directory.create(recursive: true);
      if (!current()) return null;
      staged = await directory.createTemp('.pending-');
      final imageHash = sha256.convert(bytes).toString();
      final path = '${directory.path}/${source.token}-$imageHash.png';
      final image = File('${staged.path}/image.png');
      final metadata = File('${staged.path}/identity.json');
      await image.writeAsBytes(bytes, flush: true);
      await metadata.writeAsString(
        jsonEncode({
          'identity': identity,
          'image_sha256': imageHash,
          'camera': productPreviewCoverCameraVersion,
          'fit': 'contain',
          'artifact_file': productPreviewArtifactFileIdentity(
            source.artifactIdentity.lastIndexOf(':') > 0
                ? source.artifactIdentity.substring(
                    0,
                    source.artifactIdentity.lastIndexOf(':'),
                  )
                : null,
          ),
        }),
        flush: true,
      );
      if (!current()) return null;
      await image.rename(path);
      await metadata.rename('$path.json');
      if (!current() || !await attach(path) || !current()) return null;
      attached = true;
      return path;
    } catch (_) {
      debugPrint('[PreviewCover] presentation capture unavailable');
      return null;
    } finally {
      try {
        if (staged != null && staged.existsSync()) {
          await staged.delete(recursive: true);
        }
        stagingCleaned = true;
      } finally {
        read?.release();
        captureWrite?.release();
        _pending.remove(identity);
        if (attached && stagingCleaned) {
          _attached.add(identity);
          if (source.stage == 'glb') {
            ReconstructionGeneratedConsumerLeases.instance.notifyFinalCoverReady(
              source.captureDir,
              source.artifactIdentity,
            );
          }
        }
      }
    }
  }
}

                                                                                  
File? productCaptureFirstPhotoFor(ScanRecord record) {
  try {
    final capturePath = record.captureDir;
    if (capturePath == null) return null;
    final capture = Directory(capturePath).resolveSymbolicLinksSync();
    final manifestPath =
        record.captureManifestPath ?? '$capture/official_photo_bundle.json';
    final manifest = File(manifestPath);
    if (!manifest.resolveSymbolicLinksSync().startsWith('$capture/'))
      return null;
    final document = jsonDecode(manifest.readAsStringSync()) as Map;
    final frames = document['frames'] as List;
    if (frames.isEmpty || frames.first is! Map) return null;
    final first = frames.first as Map;
    final name = first['highresFilename'] ?? first['image_file'];
    if (name is! String ||
        name.isEmpty ||
        name.contains('/') ||
        name.contains('\\') ||
        name == '..')
      return null;
    final directory = record.photosDir ?? '$capture/photos_highres';
    final image = File('$directory/$name');
    if (FileSystemEntity.typeSync(image.path, followLinks: false) !=
            FileSystemEntityType.file ||
        !image.resolveSymbolicLinksSync().startsWith('$capture/') ||
        image.lengthSync() <= 0)
      return null;
    return image;
  } catch (_) {
    return null;
  }
}

String _canonicalCaptureDir(String path) {
  try {
    return Directory(path).resolveSymbolicLinksSync();
  } catch (_) {
    return Directory(path).absolute.path;
  }
}

                                                                             
                                                                           
ProductPreviewCoverSource? legacySparseCoverSource(
  ScanRecord record,
  String plyPath,
) {
  try {
    if (record.artifactPath != null || record.captureDir == null) return null;
    final capture = Directory(record.captureDir!).resolveSymbolicLinksSync();
    final selected = File('$capture/.pw_preview_covers/current.json');
    if (selected.existsSync()) {
      final prior =
          jsonDecode(jsonDecode(selected.readAsStringSync())['identity'])
              as List;
      if (prior.length != 7 || prior[4] != 'legacy-sparse') return null;
    }
    final canonical = File(plyPath).resolveSymbolicLinksSync();
    if (!canonical.startsWith('$capture/') ||
        FileSystemEntity.typeSync(plyPath, followLinks: false) !=
            FileSystemEntityType.file) {
      return null;
    }
    final identity = productPreviewArtifactFileIdentity(canonical);
    if (identity == null || File(canonical).lengthSync() <= 0) return null;
    return ProductPreviewCoverSource(
      accountNamespace: record.accountNamespace,
      workId: record.id,
      captureDir: capture,
      runId: 'legacy-sparse',
      stage: 'sparse',
      artifactIdentity: '$canonical:${sha256.convert(utf8.encode(identity))}',
    );
  } catch (_) {
    return null;
  }
}
