import 'dart:convert';
import 'dart:io';

import 'package:crypto/crypto.dart';

import 'request.dart' show hashFile, object, string, ReconstructionIoTiming;
import 'package:worldswithin_flutter/reconstruction/vendor/dart_pipeline/lib/reconstruction_pipeline.dart'
    show checkedRelativePath;

                                                                               
                                                                               
                                                                               
final class ProductionRuntimeClosure {
  ProductionRuntimeClosure._(
    this.bundleRoot,
    this.packageSha256,
    this._packageJson,
    this._roots,
  );
  final String bundleRoot, packageSha256, _packageJson;
  final List<String> _roots;
  Object? _verifiedEpoch;
  int? _verifiedPid;
  Object? _verifiedOwner;
  Object _reuseGeneration = Object();

                                                                              
                                                                                    
  Future<bool> _isCurrentSignedApp() async {
    if (!Platform.isIOS || !RegExp(
        r'^/private/var/containers/Bundle/Application/[0-9A-Fa-f-]{36}/[^/]+\.app$')
        .hasMatch(bundleRoot)) return false;
    try {
      return await File(Platform.resolvedExecutable).resolveSymbolicLinks() ==
          '$bundleRoot/Runner';
    } on FileSystemException {
      return false;
    }
  }


  static Future<ProductionRuntimeClosure> capture({
    required String bundleRoot,
    required Map<String, dynamic> package,
    required String packageSha256,
    required Map<String, dynamic> provider,
  }) async {
    if (!bundleRoot.startsWith('/') ||
        await Directory(bundleRoot).resolveSymbolicLinks() != bundleRoot) {
      throw StateError('Canonical runtime bundle root required');
    }
    final closure = ProductionRuntimeClosure._(
      bundleRoot,
      packageSha256,
      jsonEncode(package),
      List<String>.unmodifiable([
        for (final key in [
          'runtime_home_bundle_relative_path',
          'native_modules_bundle_relative_path',
        ])
          checkedRelativePath(string(provider, key)),
      ]),
    );
    await closure.revalidate();
    return closure;
  }

  Future<File> _file(String relative) async {
    final file = File('$bundleRoot/${checkedRelativePath(relative)}');
    if (await FileSystemEntity.type(file.path, followLinks: false) !=
            FileSystemEntityType.file ||
        await file.resolveSymbolicLinks() != file.path) {
      throw StateError('Regular canonical runtime file required: $relative');
    }
    return file;
  }

  Future<File> _verify(String path, Object? expected, {int? bytes}) async {
    if (expected is! String || !RegExp(r'^[0-9a-f]{64}$').hasMatch(expected)) {
      throw StateError('Frozen runtime SHA-256 required: $path');
    }
    final file = await _file(path);
    final size = await file.length();
    if ((bytes != null && size != bytes) ||
        await hashFile(file) != expected ||
        await file.length() != size) {
      throw StateError('Packaged runtime bytes changed: $path');
    }
    return file;
  }

  Map<String, String> get identity {
    final package = object(jsonDecode(_packageJson));
    return {
      'runtime_manifest_path': string(package, 'runtime_manifest_path'),
      'runtime_manifest_sha256': string(package, 'runtime_manifest_sha256'),
      'package_sha256': packageSha256,
    };
  }

  void requireProvider(Map<String, dynamic> provider) {
    final roots = [
      string(provider, 'runtime_home_bundle_relative_path'),
      string(provider, 'native_modules_bundle_relative_path'),
    ];
    for (var i = 0; i < _roots.length; i++) {
      if (roots[i] != _roots[i]) {
        throw StateError(
          'Execution provider differs from measured runtime closure',
        );
      }
    }
  }

  Future<Map<String, dynamic>> _frozenJson(String path, String expected) async {
    final file = await _verify(path, expected);
    if (await file.length() > 8 * 1024 * 1024) {
      throw StateError('Oversized runtime identity JSON');
    }
    final bytes = await file.readAsBytes();
    if (sha256.convert(bytes).toString() != expected) {
      throw StateError('Runtime identity JSON changed while parsing');
    }
    return object(jsonDecode(utf8.decode(bytes)));
  }

  Future<Map<String, dynamic>> _validatedManifest() async {
    final package = object(jsonDecode(_packageJson));
    final measured = await _frozenJson(
      'reconstruction_production/PACKAGE.json',
      packageSha256,
    );
    for (final key in [
      'info_plist_sha256',
      'runtime_manifest_path',
      'runtime_manifest_sha256',
    ]) {
      if (measured[key] != package[key]) {
        throw StateError(
          'Runtime identity is not bound to measured PACKAGE.json: $key',
        );
      }
    }
    requireProvider(object(measured['camera_native_row']));
    await _verify('Info.plist', package['info_plist_sha256']);
    final manifestPath = checkedRelativePath(
      string(package, 'runtime_manifest_path'),
    );
    final manifestHash = string(package, 'runtime_manifest_sha256');
    final manifest = await _frozenJson(manifestPath, manifestHash);
    if (manifest['schema'] != 'RECONSTRUCTION_PRODUCTION_RUNTIME_FILES_V1' ||
        manifest['files'] is! List) {
      throw StateError('Runtime exact file closure manifest required');
    }
    return manifest;
  }

  Future<void> _revalidateContents() async {
    final manifest = await _validatedManifest();
    final files = <String>{};
    for (final raw in manifest['files'] as List) {
      final row = object(raw),
          path = checkedRelativePath(string(object(raw), 'path'));
      if (!files.add(path) ||
          row['bytes'] is! int ||
          (row['bytes'] as int) < 0) {
        throw StateError('Runtime manifest duplicate/invalid file: $path');
      }
      await _verify(path, row['sha256'], bytes: row['bytes'] as int);
    }
    for (final relative in _roots) {
      final directory = Directory('$bundleRoot/$relative');
      if (await FileSystemEntity.type(directory.path, followLinks: false) !=
              FileSystemEntityType.directory ||
          await directory.resolveSymbolicLinks() != directory.path) {
        throw StateError('Canonical runtime directory required: $relative');
      }
      final actual = <String>{};
      await for (final entity in directory.list(
        recursive: true,
        followLinks: false,
      )) {
        if (entity is Directory) continue;
        if (entity is! File) {
          throw StateError('Runtime closure contains a link: ${entity.path}');
        }
        actual.add(entity.path.substring(bundleRoot.length + 1));
      }
      final declared = files
          .where((path) => path.startsWith('$relative/'))
          .toSet();
      if (actual.isEmpty ||
          actual.length != declared.length ||
          !actual.containsAll(declared)) {
        throw StateError('Runtime file closure incomplete: $relative');
      }
    }
    final package = object(jsonDecode(_packageJson));
    await _verify(checkedRelativePath(string(package, 'runtime_manifest_path')),
        string(package, 'runtime_manifest_sha256'));
    await _verify('Info.plist', package['info_plist_sha256']);
    await _verify('reconstruction_production/PACKAGE.json', packageSha256);
  }

  Future<void> revalidate() async {
    final timingEpoch = ReconstructionIoTiming.epoch;
    final generation = _reuseGeneration;
    final timingOwner = ReconstructionIoTiming.sink;
    final timing = Stopwatch()..start();
    var timingFailed = true;
    var label = 'runtime_closure_full_verify_inclusive';
    try {
      final eligible = timingOwner != null && await _isCurrentSignedApp();
      if (eligible && _verifiedPid == pid &&
          identical(_verifiedOwner, timingOwner) &&
          identical(_verifiedEpoch, timingEpoch)) {
        label = 'runtime_closure_same_run_anchors_inclusive';
                                                                                
                                                                                
        await _validatedManifest();
      } else {
        _verifiedEpoch = null;
        _verifiedPid = null;
        _verifiedOwner = null;
        await _revalidateContents();
        if (eligible && identical(timingOwner, ReconstructionIoTiming.sink) &&
            identical(generation, _reuseGeneration) &&
            identical(timingEpoch, ReconstructionIoTiming.epoch)) {
          _verifiedEpoch = timingEpoch;
          _verifiedPid = pid;
          _verifiedOwner = timingOwner;
        }
      }
      timingFailed = false;
    } catch (_) {
      _verifiedEpoch = null;
      _verifiedPid = null;
        _verifiedOwner = null;
      _reuseGeneration = Object();
      rethrow;
    } finally {
      ReconstructionIoTiming.record(label, timing,
          epoch: timingEpoch, failed: timingFailed);
      ReconstructionIoTiming.record('runtime_closure_revalidate_inclusive', timing,
          epoch: timingEpoch, failed: timingFailed);
      ReconstructionIoTiming.checkpoint('runtime_closure_revalidate_complete', epoch: timingEpoch);
    }
  }
}
