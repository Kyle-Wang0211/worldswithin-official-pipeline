import '../reconstruction/frame_transport.dart';
import '../reconstruction/whole_dense_binding.dart'
    show wholeDenseNativeEntry, wholeDenseLastErrorEntry;
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import '../reconstruction/execution_identity_scope.dart';
import '../reconstruction/bench_known_model_families.dart';
import '../reconstruction/memory_policy.dart';
import '../reconstruction/dense_binding_contract.dart'
    show requiredDenseAbiVersion, requiredDenseHeaderSha256;
import '../reconstruction/original_retention_io.dart'
    show originalRetainedProfile, usesOriginalRetained;
import '../reconstruction/production_original_fixture_binding.dart';
import '../reconstruction/production_runtime_closure.dart';
import '../reconstruction/production_image_binding.dart'
    show reconstructionSceneImageHeaderSha256, reconstructionSceneImageSymbols;
import '../reconstruction/production_scene_binding.dart'
    show reconstructionSceneCameraHeaderSha256, reconstructionSceneCameraSymbols;
import '../reconstruction/production_session_binding.dart' show pwFullPipelineSessionSymbols;
import '../reconstruction/mobile_input_transport.dart'
    show reconstructionPillowEntry, reconstructionPillowErrorEntry;
import '../reconstruction/reconstruction_request.dart'
    show reconstructionSchema, reconstructionRoute;
import '../reconstruction/request.dart' show hashFile, object, string, artifact;
import '../reconstruction/production_runtime_binding.dart'
    show reconstructionRuntimeBootstrapSymbol;
import '../reconstruction/vendor/dart_pipeline/lib/reconstruction_pipeline.dart'
    show checkedRelativePath, reconstructionSourceInputManifestSha256;
import '../reconstruction/vendor/dart_texture_integration_v1/lib/reconstruction_texture_ffi.dart';

                                                                              
                                                                            
                                                                        
final class ReconstructionProductionPackage {
  ReconstructionProductionPackage._(
    this.bundleRoot,
    this.manifestSha256,
    this._manifest,
    this.identityScope,
  );
  final ReconstructionExecutionIdentityScope identityScope;
  final String bundleRoot, manifestSha256;
  final Map<String, dynamic> _manifest;
  Map<String, dynamic> get manifest =>
      object(jsonDecode(jsonEncode(_manifest)));
                                                                         
                                                                         
  String get verifiedTemplateSha256 => string(_manifest, 'template_sha256');
  static String? lastError;

                                                                                
                                                                                 
  static Future<ReconstructionProductionPackage?> load({
    Directory? bundleRoot,
    ReconstructionExecutionIdentityScope identityScope =
        ReconstructionExecutionIdentityScope.production,
  }) async {
    Directory? selected = bundleRoot;
    if (selected == null) {
      var candidate = File(Platform.resolvedExecutable).parent;
      for (var i = 0; i < 8; i++) {
        if (await File(
          '${candidate.path}/reconstruction_production/PACKAGE.json',
        ).exists()) {
          selected = candidate;
          break;
        }
        final parent = candidate.parent;
        if (parent.path == candidate.path) break;
        candidate = parent;
      }
    }
    if (selected == null) {
      lastError = 'ReferencePipeline production package is absent';
      return null;
    }
    final root = await selected.resolveSymbolicLinks();
                                                                                
                                                                               
    final package = await Isolate.run(
      () => _loadMeasuredPackage(root, identityScope),
    );
    lastError = null;
    return package;
  }
}

                                                                               
                                                                               
                                                        
Set<String> validateReconstructionProductionDensePackage(
  Map<String, dynamic> package,
  Map<String, dynamic> recipe,
) {
  validateFrameTransportTemplate(recipe);
  final templateInputs = object(recipe['dense_inputs']);
  if (templateInputs.containsKey('frames') || templateInputs.containsKey('original_fusion_jpegs')) {
    throw StateError('Capture-specific frames must be supplied by the actual input');
  }
  final templatePaths = <String>{};
  for (final value in templateInputs['artifacts'] as List) {
    final row = object(value);
    final path = checkedRelativePath(string(row, 'path'));
    if (!templatePaths.add(path)) {
      throw StateError('Duplicate input template artifact');
    }
    final low = path == templateInputs['low_noise_f32'];
    final high = path == templateInputs['high_noise_f32'];
    if (low || high) {
      if (row.containsKey('bytes') || row.containsKey('sha256') ||
          row['shape_policy'] != 'actual_frames_noise_f32' ||
          row['source_bank_sha256'] != (high ? officialHighNoiseBankSha256 : officialLowNoiseBankSha256)) {
        throw StateError('Count-independent authenticated noise bank template required');
      }
      string(row, 'role');
    } else {
      artifact(row);
    }
  }
  if (package['dense_producer_profile'] == wholeRetainedTransportProfile) {
    if (recipe['dense_producer_profile'] != wholeRetainedTransportProfile ||
        package['whole_profile_path'] is! String ||
        !_hash(package['whole_profile_sha256'])) {
      throw StateError(
        'Whole package requires its bound profile configuration',
      );
    }
    checkedRelativePath(package['whole_profile_path'] as String);
    final row = object(object(recipe['native_libraries'])['whole_dense']);
    final declared = object(package['whole_dense_native_row']);
    const header =
        '204df9d87b27097f4de4b2617229185514b36380fa7aa7c4783b0f841ded5245';
    if (row['header_sha256'] != header || row['abi_version'] != 1) {
      throw StateError('Whole producer requires its own ABI1/header');
    }
    for (final key in [
      'bundle_relative_path',
      'binary_sha256',
      'source_receipt_path',
      'source_receipt_sha256',
      'header_sha256',
      'abi_version',
    ]) {
      if (row[key] != declared[key]) {
        throw StateError('Whole package/native row differs: $key');
      }
    }
    return {wholeDenseNativeEntry, wholeDenseLastErrorEntry};
  }
  if (package['dense_producer_profile'] != originalRetainedProfile ||
      recipe['dense_producer_profile'] != originalRetainedProfile) {
    throw StateError('Explicit actual-frame producer profile required');
  }
  final dense = object(object(recipe['native_libraries'])['dense']);
  final session = object(package['session_native_row']);
  final expectedHeader = requiredDenseHeaderSha256(recipe);
  final expectedAbi = requiredDenseAbiVersion(recipe);
  for (final row in [dense, session]) {
    if (row['header_sha256'] != expectedHeader ||
        (row.containsKey('abi_version') && row['abi_version'] != expectedAbi)) {
      throw StateError(
        'Dense profile requires exact ABI$expectedAbi/header pair',
      );
    }
  }
  for (final key in [
    'bundle_relative_path',
    'binary_sha256',
    'source_receipt_path',
    'source_receipt_sha256',
    'header_sha256',
  ]) {
    if (session[key] != dense[key]) {
      throw StateError('Session/dense package identity differs: $key');
    }
  }
  return {
    'pw_full_pipeline_dense_abi_version',
    'pw_full_pipeline_dense_options_init',
    'pw_full_pipeline_dense_run',
    'pw_full_pipeline_dense_producer_profile',
  };
}

bool _hash(Object? value) =>
    value is String && RegExp(r'^[0-9a-f]{64}$').hasMatch(value);
String _relative(String value) => checkedRelativePath(value);
Future<File> _file(String root, String relative) async {
  final file = File('$root/${_relative(relative)}');
  if (await FileSystemEntity.type(file.path, followLinks: false) !=
          FileSystemEntityType.file ||
      await file.resolveSymbolicLinks() != file.path) {
    throw StateError('Regular canonical packaged file required: $relative');
  }
  return file;
}

Future<Directory> _directory(String root, String relative) async {
  final directory = Directory('$root/${_relative(relative)}');
  if (await FileSystemEntity.type(directory.path, followLinks: false) !=
          FileSystemEntityType.directory ||
      await directory.resolveSymbolicLinks() != directory.path) {
    throw StateError('Canonical packaged directory required: $relative');
  }
  return directory;
}

Future<File> _verify(
  String root,
  String relative,
  Object? expected, {
  int? bytes,
}) async {
  if (!_hash(expected))
    throw StateError('Frozen package SHA-256 missing: $relative');
  final file = await _file(root, relative);
  if ((bytes != null && await file.length() != bytes) ||
      await hashFile(file) != expected) {
    throw StateError('Packaged bytes changed: $relative');
  }
  return file;
}

Future<Map<String, dynamic>> _frozenJson(
  String root,
  String relative,
  String expected,
) async {
  final file = await _verify(root, relative, expected);
  if (await file.length() > 8 * 1024 * 1024)
    throw StateError('Oversized package JSON');
  final value = object(jsonDecode(await file.readAsString()));
  await _verify(root, relative, expected);
  return value;
}

Future<void> _native(
  String root,
  Map<String, dynamic> row, {
  Set<String> requiredSymbols = const {},
  bool productionTexture = false,
}) async {
  final binary = string(row, 'bundle_relative_path');
  await _verify(root, binary, row['binary_sha256']);
  final receipt = await _frozenJson(
    root,
    string(row, 'source_receipt_path'),
    string(row, 'source_receipt_sha256'),
  );
  if (productionTexture && receipt.containsKey('runtime_context_adaptation')) {
    throw StateError(
      'Test-only texture runtime is not an approved production package',
    );
  }
  if (receipt['accepted_source_closure'] != true ||
      receipt['binary_sha256'] != row['binary_sha256'] ||
      receipt['actual_defined_c_symbols'] is! List ||
      (receipt['actual_defined_c_symbols'] as List).isEmpty ||
      !(receipt['actual_defined_c_symbols'] as List)
          .cast<String>()
          .toSet()
          .containsAll(requiredSymbols)) {
    throw StateError('Actual source-bound native closure missing: $binary');
  }
  if (productionTexture) {
                                                                                
                                                                            
    final evidence = await ReconstructionTextureEvidence.fromVerifiedSourceReceipt(
      binary: File('$root/$binary'),
      receipt: receipt,
      receiptSha256: string(row, 'source_receipt_sha256'),
      signedBinarySha256: string(row, 'binary_sha256'),
      sourceInputManifestSha256: reconstructionSourceInputManifestSha256,
      actualDefinedSymbols: (receipt['actual_defined_c_symbols'] as List)
          .cast<String>()
          .toSet(),
    );
    evidence.requireExecutionScope(allowDiagnostic: false);
  }
}

Future<ReconstructionProductionPackage> _loadMeasuredPackage(
  String root,
  ReconstructionExecutionIdentityScope identityScope,
) async {
  await identityScope.revalidate();
  if (identityScope.isIndependentTest && identityScope.bundleRoot != root) {
    throw StateError(
      'Selected bundle differs from independently measured scope',
    );
  }
  const manifestPath = 'reconstruction_production/PACKAGE.json';
  final file = await _file(root, manifestPath);
  final packageHash = await hashFile(file);
  final package = await _frozenJson(root, manifestPath, packageHash);
  identityScope.requirePackage(package);
  if (package['schema'] != 'RECONSTRUCTION_PRODUCTION_PACKAGE_V1' ||
      package['platform'] != Platform.operatingSystem) {
    throw StateError('Measured production identity/platform required');
  }
                                                                        
  await _verify(root, 'Info.plist', package['info_plist_sha256']);
  final template = await _frozenJson(
    root,
    string(package, 'template_path'),
    string(package, 'template_sha256'),
  );
  final recipe = template['request'] == null
      ? template
      : object(template['request']);
  if (recipe['schema'] != reconstructionSchema ||
      recipe['route_id'] != reconstructionRoute ||
      recipe['algorithm_revision'] !=
          'c0f26ee018a2b3f9d0fa773a49711cf203da9277') {
    throw StateError('Frozen original97 reconstruction recipe required');
  }
  final denseSymbols = validateReconstructionProductionDensePackage(package, recipe);
  final whole = recipe['dense_producer_profile'] == wholeRetainedTransportProfile;
  final native = object(recipe['native_libraries']);
  for (final entry in native.entries) {
    await _native(
      root,
      object(entry.value),
      requiredSymbols:
          entry.key == (whole ? 'whole_dense' : 'dense') ? denseSymbols : const {},
      productionTexture: entry.key == 'texture',
    );
  }
  final producerSymbols = <String, Set<String>>{
    if (!whole) 'runtime_bootstrap_native_row': {reconstructionRuntimeBootstrapSymbol},
    if (!usesOriginalRetained(recipe))
      'session_native_row': pwFullPipelineSessionSymbols,
    if (usesOriginalRetained(recipe))
      'original_fixture_native_row': {
        originalFixtureEntry,
        originalFixtureErrorEntry,
      },
    'camera_native_row': reconstructionSceneCameraSymbols,
    'image_native_row': reconstructionSceneImageSymbols,
    'pillow_native_row': {
      reconstructionPillowEntry,
      reconstructionPillowErrorEntry,
      'reconstruction_glb_runtime_initialize',
    },
  };
  for (final producer in producerSymbols.entries) {
    await _native(
      root,
      object(package[producer.key]),
      requiredSymbols: producer.value,
    );
  }
  if (object(package['camera_native_row'])['scene_camera_header_sha256'] !=
          reconstructionSceneCameraHeaderSha256 ||
      object(package['image_native_row'])['scene_image_header_sha256'] !=
          reconstructionSceneImageHeaderSha256) {
    throw StateError('Exact production producer headers required');
  }
  final image = object(package['image_native_row']);
  await _verify(
    root,
    string(image, 'scene_image_header_bundle_relative_path'),
    reconstructionSceneImageHeaderSha256,
  );
  final glb = object(native['glb']);
  if (usesOriginalRetained(recipe)) {
    final fixture = object(package['original_fixture_native_row']);
    validateOriginalFixturePackageRow(fixture, glb);
    if (variableFrameTransport(recipe) != (fixtureWrapperSha256(fixture) == originalFixtureV2WrapperSha256)) throw StateError('Fixture wrapper and production profile differ');
    for (final pair in [
      ('fixture_header_bundle_relative_path', originalFixtureHeaderSha256),
      ('fixture_source_bundle_relative_path', originalFixtureSourceSha256),
      ('fixture_wrapper_bundle_relative_path', fixtureWrapperSha256(fixture)),
    ]) {
      await _verify(root, string(fixture, pair.$1), pair.$2);
    }
    final closure = await _frozenJson(
      root,
      string(fixture, 'source_receipt_path'),
      string(fixture, 'source_receipt_sha256'),
    );
    if (closure['fixture_combined_source_sha256'] !=
            fixtureCombinedSourceSha256(fixture) ||
        closure['fixture_header_sha256'] != originalFixtureHeaderSha256 ||
        closure['runtime_provider_binary_sha256'] != glb['binary_sha256']) {
      throw StateError(
        'Actual additive fixture/provider source closure required',
      );
    }
  }
  for (final key in [
    'bundle_relative_path',
    'binary_sha256',
    'runtime_home_bundle_relative_path',
    'native_modules_bundle_relative_path',
  ]) {
    final value = glb[key];
    if (value is! String ||
        value.isEmpty ||
        object(package['camera_native_row'])[key] != value ||
        object(package['pillow_native_row'])[key] != value) {
      throw StateError('Camera/Pillow/GLB runtime identity differs: $key');
    }
  }
  await ProductionRuntimeClosure.capture(
    bundleRoot: root,
    package: package,
    packageSha256: packageHash,
    provider: glb,
  );
  if (whole) {
    final profile = await _frozenJson(
      root,
      string(package, 'whole_profile_path'),
      string(package, 'whole_profile_sha256'),
    );
    if (profile['schema'] != 'RECONSTRUCTION_PRODUCTION_WHOLE_PROFILE_V1' ||
        profile['algorithm_route'] != 'LOCAL_DENSE_RECONSTRUCTION') {
      throw StateError('Selected whole pixel1 profile required');
    }
    requireWholeMemoryPolicy(profile, identityScope);
    const ordinaryModelIdentities = {
      'feature_low':
          'ac6bc8f7822c735191e50038e6b0ac9c4b57cbce82faf1f778e217d361027593',
      'rest_low':
          'cf39dfa7d93de9fa397a2c14c00ae60ab6c88a5fef1293bbd8633d6f469d54d5',
      'feature_high':
          '376e6c36db0064adae367db87f35b3ef9c6a187e0e36c18a818e3ab7942a0683',
      'rest_high':
          '358c581f62fedb71209eb248b03fd6131d1e6a05bf73527921fdaae493b150d0',
    };
    final modelIdentities = selectBenchKnownModelFamily(
      profile, identityScope, ordinaryModelIdentities,
    );
    final models = object(profile['models']);
    if (models.length != modelIdentities.length) {
      throw StateError('Whole profile requires exactly four selected models');
    }
    for (final entry in modelIdentities.entries) {
      final row = object(models[entry.key]);
      if (row['sha256'] != entry.value || row['bytes'] is! int) {
        throw StateError('Whole selected model identity differs: ${entry.key}');
      }
      await _verify(root, string(row, 'path'), entry.value,
          bytes: row['bytes'] as int);
    }
    for (final entry in object(profile['bindings']).entries) {
      final row = object(entry.value);
      await _verify(root, string(row, 'path'), row['sha256'],
          bytes: row['bytes'] as int);
      if (row['receipt'] != null) {
        final receipt = object(row['receipt']);
        await _verify(root, string(receipt, 'path'), receipt['sha256'],
            bytes: receipt['bytes'] as int);
      }
    }
    for (final entry in object(profile['source_pins']).entries) {
      await _verify(root, entry.key, entry.value);
    }
  } else {
    for (final model in const {
    'dense_models/casdiffmvs_768.onnx':
        'cc32cee17f7ede31dc957d0a860fea54e4b21ad5d8ba125336238829aa13faba',
    'dense_models/casdiffmvs_1536.onnx':
        '18533085f03602aec66b0fcd36e985c398a85025549c364cb679d05a735705b9',
    'Frameworks/libonnxruntime.1.dylib':
        'cd0d290e395f82cf2e1571f8c163af688d950c7cee8f0152d787f889b7dc4749',
    }.entries) {
      await _verify(root, model.key, model.value);
    }
  }
  final noiseRelative = _relative(
    string(package, 'noise_source_root_relative_path'),
  );
  await _directory(root, noiseRelative);
  final noise = object(recipe['dense_inputs']);
  for (final key in [
    'low_noise_f32',
    'high_noise_f32',
    'noise_producer_json',
  ]) {
    final relative = _relative(string(noise, key));
    final row = (noise['artifacts'] as List)
        .map(object)
        .singleWhere((r) => r['path'] == relative);
    final bank = key != 'noise_producer_json';
    final high = key == 'high_noise_f32';
    await _verify(
      root,
      '$noiseRelative/$relative',
      bank ? string(row, 'source_bank_sha256') : string(row, 'sha256'),
      bytes: bank
          ? officialNoiseBankCapacity *
              (high ? officialHighNoiseFrameBytes : officialLowNoiseFrameBytes)
          : row['bytes'] as int,
    );
  }
  await _verify(root, manifestPath, packageHash);
  return ReconstructionProductionPackage._(
    root,
    packageHash,
    object(jsonDecode(jsonEncode(package))),
    identityScope,
  );
}
