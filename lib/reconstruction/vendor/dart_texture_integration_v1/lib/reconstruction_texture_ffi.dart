import 'dart:convert';
import 'dart:ffi';
import 'dart:io';

import 'package:pw_shared_viewer/native_binary_identity.dart';

import '../../dart_pipeline/lib/reconstruction_geometry_ffi.dart';
import '../../dart_pipeline/lib/reconstruction_pipeline.dart';

                                                                  
                                                                                
typedef ReconstructionTextureNative =
    Int32 Function(
      Pointer<Uint8>,
      Pointer<Uint8>,
      Pointer<Uint8>,
      Pointer<Uint8>,
      UintPtr,
    );
typedef ReconstructionTextureDart =
    int Function(
      Pointer<Uint8>,
      Pointer<Uint8>,
      Pointer<Uint8>,
      Pointer<Uint8>,
      int,
    );

const reconstructionTextureSymbol = 'pw_texture38_run';
const reconstructionTextureHeaderSha256 =
    '87b3a2e35a9c254fdd0ff8fa5e400f349cd830d4cf6f9830f06e565654025016';
const reconstructionTextureAdapterSha256 =
    'a61ca9563dbd97cac10a8a9c638ca640259dde5df242381d7aae6fd82cef9774';
const reconstructionTexturePackageSha256 =
    'e6d2fac5d475b95ad25348eeefc80f3ff481415354012ff18df5f1bac5cdd5f8';
const reconstructionTextureFrozenIosBinarySha256 =
    'd32b4b6ca0012a74c8ad3bfe8c8ed9958f0e3a78343473e52b33b145144eca5a';
const reconstructionTextureMaxErrorCapacity = 65536;

                                                                           
                                                                             
const reconstructionTextureReferenceSignedSha256 =
    'ac04b4f57413c142fa5f9f927c95cb95fb49091e875c99657de0f5ad3d65cf75';
const reconstructionTextureReferenceUnsignedSha256 =
    'dc0851f89609a7c80703893659568d57e06b0a2b8b411c670bcd8fbe18b16b0c';
const reconstructionTextureReferenceReceiptSha256 =
    'cc28373750b6cc22b3da50c7593cfd1ee0bf45c22461b12bcae2abace645bea8';
const reconstructionTextureReferenceWrapperSha256 =
    '9d2867ab82063b1f310f751efc1da5793ddff25b9fc94577dc018a97e91060c9';
const reconstructionTextureReferenceBuildContractSha256 =
    '536e67431157fd18bd55353d42a814f275042cd856e9337afc29074518cfee34';
const reconstructionTextureReferenceBuildResultSha256 =
    '7d890b85c72ab588d0fe80c8f2b676d9814fbab999a813c89c8945d7de10dfea';
const reconstructionTextureReferenceClassification =
    'TEST_ONLY_TBB_EXECUTION_CONTEXT_AND_FPCONTRACT_DIAGNOSTIC';

                                                                               
                                                                              
                                                          
const reconstructionTextureFpClassification =
    'OFFICIAL_SOURCE_IOS_FPCONTRACT_OFF_BUILD_V1';
                                                                              
                                                                               
const reconstructionTextureFpSignedSha256 =
    'bd599d1f477b88097951d1d59a9166a6c13ccf1e6b3ee982d7c5181b208ebf0d';
                                                                        
                                                                            
                                                                               
const reconstructionTextureFpReleaseSignedSha256 =
    'e6f5499de099c43a30be3971f17605a1aed19648ac27f929f41fc20acf429a8f';
const _reconstructionTextureReleaseDerivation = <String, String>{
  'schema': 'RECONSTRUCTION_SIGNED_NATIVE_DERIVATION_V1',
  'prior_receipt_sha256':
      '706d70234dbca3e8829c41f04ca953270ea1d833b853e1d0810f17f0690033a9',
  'prior_binary_sha256': reconstructionTextureFpSignedSha256,
  'current_binary_sha256': reconstructionTextureFpReleaseSignedSha256,
  'unsigned_binary_sha256':
      '7b0ac6d53557f3396020f8026e9a0dc20f56e7c97280452955975ddbef388d01',
  'proof':
      'EXPLICIT_BASELINE_SIGNED_SHA256_AND_TEMPORARY_UNSIGNED_BYTE_EQUALITY',
};

bool _textureFpSignedAccepted(String? signed) =>
    signed == reconstructionTextureFpSignedSha256 ||
    signed == reconstructionTextureFpReleaseSignedSha256;

bool _textureFpReleaseAssociation(Map<String, dynamic> receipt, String signed) {
  if (signed == reconstructionTextureFpSignedSha256) return true;
  if (signed != reconstructionTextureFpReleaseSignedSha256) return false;
  final derivation = receipt['signed_release_rebinding'];
  return derivation is Map<String, dynamic> &&
      derivation.length == _reconstructionTextureReleaseDerivation.length &&
      _reconstructionTextureReleaseDerivation.entries.every(
        (entry) => derivation[entry.key] == entry.value,
      );
}

const reconstructionTextureFpUnsignedSha256 =
    '865f12468a356d9ff9d2b07c05e20cd52fdb2bf69c75f461c86137714ce253df';
const reconstructionTextureFpMeshSourceSha256 =
    '52dba53487652becbf602ef15fa8c5d8097bd8d98c25b0a1a004a0cf1c91535c';
const reconstructionTextureFpMeshObjectSha256 =
    '544010401b287d69b4bedf62e7e8f0f3e28d33cf0913e314132e11bc713944e3';
const reconstructionTextureFpBuildContractSha256 =
    '56ffa4d341da4f114ca64f0d855ecd02fb71d7a62149a46f11edd0e6620c74e7';
const reconstructionTextureFpSourceBuildManifestSha256 =
    '7fbed6d68cfd7aef3a7a11efdbe70ace81593736ddd339ba6147568f04a63517';
const reconstructionTextureFpBuildAdjudicationSha256 =
    '639fc883b49a61a6221383ac8f7d7e10333bbfd99fbd5a8c9c5fa84595516a9d';
const reconstructionTextureFpEffectiveCompileSha256 =
    '9bb0a7c23e3f1554bff19ec78b18ceb2282885271e60905366399df359aa44a7';

const reconstructionTextureFpReceiptPins = <String, String>{
  'header': reconstructionTextureHeaderSha256,
  'wrapper': reconstructionTextureAdapterSha256,
  'mesh_source': reconstructionTextureFpMeshSourceSha256,
  'mesh_object': reconstructionTextureFpMeshObjectSha256,
  'build_contract': reconstructionTextureFpBuildContractSha256,
  'source_build_pin_manifest': reconstructionTextureFpSourceBuildManifestSha256,
  'build_adjudication': reconstructionTextureFpBuildAdjudicationSha256,
  'effective_compile': reconstructionTextureFpEffectiveCompileSha256,
  'unsigned_build': reconstructionTextureFpUnsignedSha256,
};

bool _textureHash(Object? value) =>
    value is String && RegExp(r'^[0-9a-f]{64}$').hasMatch(value);

enum ReconstructionTextureEvidenceProfile {
  historicalIos,
  referenceRuntime,
  officialSourceIosFpContractOff,
}

                                                                                
                                                                         
                                                                                  
                      
                                                                        
final class ReconstructionTextureEvidence {
  ReconstructionTextureEvidence({
    required this.headerSha256,
    required this.adapterSha256,
    required this.packageSha256,
    required this.binarySha256,
    required this.sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) : profile = ReconstructionTextureEvidenceProfile.historicalIos,
       signedBinarySha256 = null,
       _sourceSignedBinarySha256 = null,
       buildContractSha256 = null,
       buildResultSha256 = null,
       classification = null,
       actualDefinedSymbols = Set.unmodifiable(actualDefinedSymbols);

  ReconstructionTextureEvidence.referenceRuntime({
    required this.headerSha256,
    required String wrapperSha256,
    required String receiptSha256,
    required String unsignedBinarySha256,
    required this.signedBinarySha256,
    required this.buildContractSha256,
    required this.buildResultSha256,
    required this.classification,
    required this.sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) : profile = ReconstructionTextureEvidenceProfile.referenceRuntime,
       _sourceSignedBinarySha256 = signedBinarySha256,
       adapterSha256 = wrapperSha256,
       packageSha256 = receiptSha256,
       binarySha256 = unsignedBinarySha256,
       actualDefinedSymbols = Set.unmodifiable(actualDefinedSymbols);

  ReconstructionTextureEvidence._officialSourceIosFpContractOff({
    required this.signedBinarySha256,
    required String sourceSignedBinarySha256,
    required this.packageSha256,
    required this.sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) : profile = ReconstructionTextureEvidenceProfile.officialSourceIosFpContractOff,
       _sourceSignedBinarySha256 = sourceSignedBinarySha256,
       headerSha256 = reconstructionTextureHeaderSha256,
       adapterSha256 = reconstructionTextureAdapterSha256,
       binarySha256 = reconstructionTextureFpUnsignedSha256,
       buildContractSha256 = reconstructionTextureFpBuildContractSha256,
       buildResultSha256 = reconstructionTextureFpBuildAdjudicationSha256,
       classification = reconstructionTextureFpClassification,
       actualDefinedSymbols = Set.unmodifiable(actualDefinedSymbols);

                                                                              
                                                                               
  factory ReconstructionTextureEvidence.fromSourceReceipt({
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) => ReconstructionTextureEvidence._fromSourceReceipt(
    receipt: receipt,
    receiptSha256: receiptSha256,
    signedBinarySha256: signedBinarySha256,
    sourceSignedBinarySha256: signedBinarySha256,
    sourceInputManifestSha256: sourceInputManifestSha256,
    actualDefinedSymbols: actualDefinedSymbols,
  );

                                                                                 
                                                                               
  static Future<ReconstructionTextureEvidence> fromVerifiedSourceReceipt({
    required File binary,
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) async {
    await verifyReconstructionNativeBinary(binary, signedBinarySha256);
    var sourceSigned = signedBinarySha256;
    if (receipt.containsKey('compiler_adaptation') &&
        !_textureFpSignedAccepted(signedBinarySha256)) {
      await verifyReconstructionNativeBinary(binary, reconstructionTextureFpReleaseSignedSha256);
      sourceSigned = reconstructionTextureFpReleaseSignedSha256;
    }
    return ReconstructionTextureEvidence._fromSourceReceipt(
      receipt: receipt,
      receiptSha256: receiptSha256,
      signedBinarySha256: signedBinarySha256,
      sourceSignedBinarySha256: sourceSigned,
      sourceInputManifestSha256: sourceInputManifestSha256,
      actualDefinedSymbols: actualDefinedSymbols,
    );
  }

  factory ReconstructionTextureEvidence._fromSourceReceipt({
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceSignedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) {
    final compiler = receipt.containsKey('compiler_adaptation');
    final runtime = receipt.containsKey('runtime_context_adaptation');
    if (compiler && runtime) {
      throw StateError('Ambiguous texture compiler/runtime profiles');
    }
    if (compiler) {
      return ReconstructionTextureEvidence._fromOfficialSourceFpReceipt(
        receipt: receipt,
        receiptSha256: receiptSha256,
        signedBinarySha256: signedBinarySha256,
        sourceSignedBinarySha256: sourceSignedBinarySha256,
        sourceInputManifestSha256: sourceInputManifestSha256,
        actualDefinedSymbols: actualDefinedSymbols,
      );
    }
    if (runtime) {
      return ReconstructionTextureEvidence.fromReferenceRuntimeReceipt(
        receipt: receipt,
        receiptSha256: receiptSha256,
        signedBinarySha256: signedBinarySha256,
        sourceInputManifestSha256: sourceInputManifestSha256,
        actualDefinedSymbols: actualDefinedSymbols,
      );
    }
    final unsigned = receipt['unsigned_provider_sha256'];
    if (unsigned is! String) {
      throw StateError('Historical texture unsigned identity missing');
    }
    return ReconstructionTextureEvidence(
      headerSha256: reconstructionTextureHeaderSha256,
      adapterSha256: reconstructionTextureAdapterSha256,
      packageSha256: reconstructionTexturePackageSha256,
      binarySha256: unsigned,
      sourceInputManifestSha256: sourceInputManifestSha256,
      actualDefinedSymbols: actualDefinedSymbols,
    );
  }

  factory ReconstructionTextureEvidence.fromOfficialSourceFpReceipt({
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) => ReconstructionTextureEvidence._fromOfficialSourceFpReceipt(
    receipt: receipt,
    receiptSha256: receiptSha256,
    signedBinarySha256: signedBinarySha256,
    sourceSignedBinarySha256: signedBinarySha256,
    sourceInputManifestSha256: sourceInputManifestSha256,
    actualDefinedSymbols: actualDefinedSymbols,
  );

  factory ReconstructionTextureEvidence._fromOfficialSourceFpReceipt({
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceSignedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) {
    Never rejected() => throw StateError(
      'Original-source texture compiler profile association rejected',
    );
    final adaptation = receipt['compiler_adaptation'];
    final pins = receipt['current_source_pins'];
    if (receipt.containsKey('runtime_context_adaptation') ||
        adaptation is! Map<String, dynamic> ||
        pins is! List ||
        !_textureHash(receiptSha256) ||
        !_textureFpReleaseAssociation(receipt, sourceSignedBinarySha256)) {
      rejected();
    }
    final headers = pins
        .where(
          (pin) =>
              pin is Map<String, dynamic> &&
              pin['path'] is String &&
              (pin['path'] as String).endsWith('/pw_texture38.h'),
        )
        .toList();
    final flags = adaptation['added_compile_flags'];
    if (headers.length != 1 ||
        (headers.single as Map)['sha256'] != reconstructionTextureHeaderSha256 ||
        receipt['slot'] != 'texture' ||
        receipt['accepted_source_closure'] != true ||
        receipt['binary_sha256'] != signedBinarySha256 ||
        receipt['unsigned_provider_sha256'] != reconstructionTextureFpUnsignedSha256 ||
        receipt['actual_unsigned_provider_sha256'] !=
            reconstructionTextureFpUnsignedSha256 ||
        adaptation['classification'] != reconstructionTextureFpClassification ||
        adaptation['original_algorithm_sources_unchanged'] != true ||
        adaptation['original_thread_wrapper_unchanged'] != true ||
        adaptation['modified_translation_unit'] != 'libs/mve/mesh.cc' ||
        flags is! List ||
        flags.length != 1 ||
        flags.single != '-ffp-contract=off') {
      rejected();
    }
    for (final pin in {
      ...reconstructionTextureFpReceiptPins,
                                                                          
                                                                              
      'signed_binary': reconstructionTextureFpSignedSha256,
    }.entries) {
      final row = adaptation[pin.key];
      if (row is! Map<String, dynamic> || row['sha256'] != pin.value) {
        rejected();
      }
    }
    final evidence = ReconstructionTextureEvidence._officialSourceIosFpContractOff(
      signedBinarySha256: signedBinarySha256,
      sourceSignedBinarySha256: sourceSignedBinarySha256,
      packageSha256: receiptSha256,
      sourceInputManifestSha256: sourceInputManifestSha256,
      actualDefinedSymbols: actualDefinedSymbols,
    );
    evidence.validate();
    return evidence;
  }

                                                                               
                                                                              
                                                                            
  factory ReconstructionTextureEvidence.fromReferenceRuntimeReceipt({
    required Map<String, dynamic> receipt,
    required String receiptSha256,
    required String signedBinarySha256,
    required String sourceInputManifestSha256,
    required Set<String> actualDefinedSymbols,
  }) {
    Map<String, dynamic> row(Object? value) {
      if (value is! Map<String, dynamic>) {
        throw StateError('Frozen V24 texture receipt object missing');
      }
      return value;
    }

    String field(Map<String, dynamic> value, String key) {
      final result = value[key];
      if (result is! String || result.isEmpty) {
        throw StateError('Frozen V24 texture receipt field missing: $key');
      }
      return result;
    }

    if (receipt.containsKey('compiler_adaptation')) {
      throw StateError('Ambiguous texture compiler/runtime profiles');
    }
    final adaptation = row(receipt['runtime_context_adaptation']);
    final pins = receipt['current_source_pins'];
    if (pins is! List)
      throw StateError('Frozen V24 texture header pins missing');
    final headers = pins
        .map(row)
        .where((pin) => field(pin, 'path').endsWith('/pw_texture38.h'))
        .toList();
    final unsigned = field(receipt, 'unsigned_provider_sha256');
    if (headers.length != 1 ||
        receipt['slot'] != 'texture' ||
        receipt['accepted_source_closure'] != true ||
        receipt['binary_sha256'] != signedBinarySha256 ||
        receipt['actual_unsigned_provider_sha256'] != unsigned ||
        row(adaptation['unsigned_build'])['sha256'] != unsigned ||
        row(adaptation['signed_binary'])['sha256'] != signedBinarySha256 ||
        adaptation['original_algorithm_sources_unchanged'] != true ||
        adaptation['production_recipe_selected'] != false) {
      throw StateError('Frozen V24 texture receipt association rejected');
    }
    return ReconstructionTextureEvidence.referenceRuntime(
      headerSha256: field(headers.single, 'sha256'),
      wrapperSha256: field(row(adaptation['wrapper']), 'sha256'),
      receiptSha256: receiptSha256,
      unsignedBinarySha256: unsigned,
      signedBinarySha256: signedBinarySha256,
      buildContractSha256: field(row(adaptation['build_contract']), 'sha256'),
      buildResultSha256: field(row(adaptation['build_result']), 'sha256'),
      classification: field(adaptation, 'classification'),
      sourceInputManifestSha256: sourceInputManifestSha256,
      actualDefinedSymbols: actualDefinedSymbols,
    );
  }

  final ReconstructionTextureEvidenceProfile profile;
  final String? signedBinarySha256;
  final String? _sourceSignedBinarySha256;
  final String? buildContractSha256;
  final String? buildResultSha256;
  final String? classification;
  final String headerSha256;
  final String adapterSha256;
  final String packageSha256;
  final String binarySha256;
  final String sourceInputManifestSha256;
  final Set<String> actualDefinedSymbols;

                                                                            
  void requireExecutionScope({required bool allowDiagnostic}) {
    validate();
    if (profile == ReconstructionTextureEvidenceProfile.referenceRuntime &&
        !allowDiagnostic) {
      throw StateError(
        'Test-only texture diagnostic is forbidden in production',
      );
    }
  }

  void validate() {
    final providerAccepted = switch (profile) {
      ReconstructionTextureEvidenceProfile.historicalIos =>
        adapterSha256 == reconstructionTextureAdapterSha256 &&
            packageSha256 == reconstructionTexturePackageSha256 &&
            binarySha256 == reconstructionTextureFrozenIosBinarySha256,
      ReconstructionTextureEvidenceProfile.referenceRuntime =>
        adapterSha256 == reconstructionTextureReferenceWrapperSha256 &&
            packageSha256 == reconstructionTextureReferenceReceiptSha256 &&
            binarySha256 == reconstructionTextureReferenceUnsignedSha256 &&
            signedBinarySha256 == reconstructionTextureReferenceSignedSha256 &&
            buildContractSha256 == reconstructionTextureReferenceBuildContractSha256 &&
            buildResultSha256 == reconstructionTextureReferenceBuildResultSha256 &&
            classification == reconstructionTextureReferenceClassification,
      ReconstructionTextureEvidenceProfile.officialSourceIosFpContractOff =>
        adapterSha256 == reconstructionTextureAdapterSha256 &&
            binarySha256 == reconstructionTextureFpUnsignedSha256 &&
            _textureHash(packageSha256) &&
            _textureFpSignedAccepted(_sourceSignedBinarySha256) &&
            buildContractSha256 == reconstructionTextureFpBuildContractSha256 &&
            buildResultSha256 == reconstructionTextureFpBuildAdjudicationSha256 &&
            classification == reconstructionTextureFpClassification,
    };
    if (headerSha256 != reconstructionTextureHeaderSha256 ||
        !providerAccepted ||
        sourceInputManifestSha256 != reconstructionSourceInputManifestSha256 ||
        !actualDefinedSymbols.contains(reconstructionTextureSymbol)) {
      throw StateError('Frozen texture C ABI/source/package identity rejected');
    }
  }
}

                                                                             
                                                              
final class ReconstructionTextureBindings {
  ReconstructionTextureBindings._(this.run, this.evidence, this.isTestOnly);

  factory ReconstructionTextureBindings.bind(
    DynamicLibrary library,
    ReconstructionTextureEvidence evidence,
  ) {
    evidence.validate();
    return ReconstructionTextureBindings._(
      library.lookupFunction<ReconstructionTextureNative, ReconstructionTextureDart>(
        reconstructionTextureSymbol,
      ),
      evidence,
      false,
    );
  }

                                                                                
  factory ReconstructionTextureBindings.testCallback(
    Pointer<NativeFunction<ReconstructionTextureNative>> callback,
  ) =>
      ReconstructionTextureBindings._(callback.asFunction<ReconstructionTextureDart>(), null, true);
  final ReconstructionTextureDart run;
  final ReconstructionTextureEvidence? evidence;
  final bool isTestOnly;
}

String reconstructionTextureAbsolutePath(String path) {
  if (!path.startsWith('/') ||
      path.length < 2 ||
      path.contains('\\') ||
      path.contains('\x00') ||
      path.split('/').skip(1).any((p) => p.isEmpty || p == '.' || p == '..') ||
      utf8.decode(utf8.encode(path)) != path) {
    throw ArgumentError('Texture IO requires canonical absolute UTF-8 paths');
  }
  return path;
}

final class _TextureArena {
  _TextureArena(this.memory);
  final ReconstructionNativeMemory memory;
  final _owned = <Pointer<Uint8>>[];

  Pointer<Uint8> bytes(int count) {
    final p = memory.allocate(count);
    if (p == nullptr) throw StateError('Texture allocation returned nullptr');
    _owned.add(p);
    p.asTypedList(count).fillRange(0, count, 0);
    return p;
  }

  Pointer<Uint8> path(String value) {
    final encoded = utf8.encode(value);
    final p = bytes(encoded.length + 1);
    p.asTypedList(encoded.length).setAll(0, encoded);
    return p;
  }

  void dispose() {
    for (final p in _owned.reversed) {
      memory.release(p);
    }
    _owned.clear();
  }
}

final class ReconstructionTextureCallResult {
  const ReconstructionTextureCallResult(
    this.entryReturnCode,
    this.error, {
    this.diagnosticIssue,
  });
  final int entryReturnCode;
  final String error;
                                                                         
  final String? diagnosticIssue;
}

                                                                            
/// algorithm is reimplemented. Upstream std::exit terminates the owning process;
                                                                               
                                                                              
final class ReconstructionTextureFfi {
  ReconstructionTextureFfi(this.bindings, this.memory);
  final ReconstructionTextureBindings bindings;
  final ReconstructionNativeMemory memory;
  bool _running = false;

  ReconstructionTextureCallResult run({
    required String sceneDirectory,
    required String meshPly,
    required String outputPrefix,
    int errorCapacity = 4096,
  }) {
    reconstructionTextureAbsolutePath(sceneDirectory);
    reconstructionTextureAbsolutePath(meshPly);
    reconstructionTextureAbsolutePath(outputPrefix);
    if (errorCapacity < 1 || errorCapacity > reconstructionTextureMaxErrorCapacity) {
      throw ArgumentError.value(errorCapacity, 'errorCapacity');
    }
    if (sceneDirectory == outputPrefix ||
        outputPrefix.startsWith('$sceneDirectory/') ||
        meshPly == outputPrefix ||
        meshPly == '$outputPrefix.obj' ||
        meshPly == '$outputPrefix.mtl') {
      throw ArgumentError('Texture output must be isolated from inputs');
    }
    if (_running) throw StateError('Texture FFI already has an active call');
    _running = true;
    final arena = _TextureArena(memory);
    try {
      final scene = arena.path(sceneDirectory);
      final mesh = arena.path(meshPly);
      final output = arena.path(outputPrefix);
      final error = arena.bytes(errorCapacity);
      final rc = bindings.run(scene, mesh, output, error, errorCapacity);
      final view = error.asTypedList(errorCapacity);
      final end = view.indexOf(0);
      if (end < 0) {
        return ReconstructionTextureCallResult(
          rc,
          '',
          diagnosticIssue: 'Native error is unterminated within capacity',
        );
      }
      try {
        return ReconstructionTextureCallResult(rc, utf8.decode(view.sublist(0, end)));
      } on FormatException {
        return ReconstructionTextureCallResult(
          rc,
          '',
          diagnosticIssue: 'Native error is not valid UTF-8',
        );
      }
    } finally {
      try {
        arena.dispose();
      } finally {
        _running = false;
      }
    }
  }
}

                                                                               
                                                                                
                                                                               
final class ReconstructionTextureIo {
  ReconstructionTextureIo({
    required this.runId,
    required this.inputRoot,
    required this.runRoot,
    required this.sceneRelativeDirectory,
    required this.meshRelativePath,
    required this.outputRelativePrefix,
  });
  final String runId;
  final Directory inputRoot;
  final Directory runRoot;
  final String sceneRelativeDirectory;
  final String meshRelativePath;
  final String outputRelativePrefix;

  Future<String> _root(Directory directory) async {
    final path = reconstructionTextureAbsolutePath(directory.path);
    if (await FileSystemEntity.type(path, followLinks: false) !=
            FileSystemEntityType.directory ||
        await directory.resolveSymbolicLinks() != path) {
      throw StateError('Texture root absent, symlinked or noncanonical');
    }
    return path;
  }

  Future<String> _path(String root, String relative) async {
    checkedRelativePath(relative);
    var path = root;
    for (final part in relative.split('/')) {
      path = '$path/$part';
      if (await FileSystemEntity.type(path, followLinks: false) ==
          FileSystemEntityType.link) {
        throw StateError('Texture symlink IO rejected');
      }
    }
    return path;
  }

  Future<({String scene, String mesh, String output})> validate(
    ReconstructionRunContract contract,
  ) async {
    contract.validate();
    if (contract.runId != runId) {
      throw StateError('Texture IO run identity mismatch');
    }
    final input = await _root(inputRoot), run = await _root(runRoot);
    if (input == run ||
        input.startsWith('$run/') ||
        run.startsWith('$input/')) {
      throw StateError('Texture input and run roots must be disjoint');
    }
    final scene = await _path(input, sceneRelativeDirectory);
    final mesh = await _path(run, meshRelativePath);
    final output = await _path(run, outputRelativePrefix);
    final parent = output.substring(0, output.lastIndexOf('/'));
    if (await FileSystemEntity.type(scene, followLinks: false) !=
            FileSystemEntityType.directory ||
        await FileSystemEntity.type(mesh, followLinks: false) !=
            FileSystemEntityType.file ||
        parent == run ||
        mesh.startsWith('$parent/') ||
        await FileSystemEntity.type(parent, followLinks: false) !=
            FileSystemEntityType.directory ||
        !await Directory(parent).list(followLinks: false).isEmpty) {
      throw StateError('Texture scene/mesh/isolated fresh output IO rejected');
    }
    final meshes = contract.outputs[ReconstructionStage.lagrange]!
        .where((a) => a.role == 'mesh_ply')
        .toList();
    if (meshes.length != 1 || meshes.single.path != meshRelativePath) {
      throw StateError('Texture mesh must be the registered lagrange output');
    }
    final sceneRows = contract.inputs
        .where((a) => a.path.startsWith('$sceneRelativeDirectory/'))
        .map((a) => '$input/${a.path}')
        .toList();
    if (sceneRows.isEmpty || sceneRows.toSet().length != sceneRows.length) {
      throw StateError('Texture scene manifest empty or duplicated');
    }
    final actual = <String>{};
    await for (final entity in Directory(
      scene,
    ).list(recursive: true, followLinks: false)) {
      final kind = await FileSystemEntity.type(entity.path, followLinks: false);
      if (kind == FileSystemEntityType.file) {
        actual.add(entity.path);
      } else if (kind != FileSystemEntityType.directory) {
        throw StateError('Texture scene contains nonregular input');
      }
    }
    if (actual.length != sceneRows.length || !actual.containsAll(sceneRows)) {
      throw StateError('Texture complete scene input manifest mismatch');
    }
    final rows = contract.outputs[ReconstructionStage.texture]!;
    if (rows.where((a) => a.role == 'obj').length != 1 ||
        rows.where((a) => a.role == 'mtl').length != 1 ||
        rows.where((a) => a.role == 'obj').single.path !=
            '$outputRelativePrefix.obj' ||
        rows.where((a) => a.role == 'mtl').single.path !=
            '$outputRelativePrefix.mtl' ||
        rows.any(
          (a) =>
              a.role != 'obj' &&
              a.role != 'mtl' &&
              (a.role != 'color_png' ||
                  !a.path.startsWith('${outputRelativePrefix}_') ||
                  !a.path.endsWith('_map_Kd.png') ||
                  a.path.substring(0, a.path.lastIndexOf('/')) !=
                      outputRelativePrefix.substring(
                        0,
                        outputRelativePrefix.lastIndexOf('/'),
                      )),
        )) {
      throw StateError('Texture output manifest does not match native prefix');
    }
    return (scene: scene, mesh: mesh, output: output);
  }

                                                                                
                                                                             
  Future<void> verifyEmittedFileSet(ReconstructionRunContract contract) async {
    final root = await _root(runRoot);
    final prefix = await _path(root, outputRelativePrefix);
    final parent = prefix.substring(0, prefix.lastIndexOf('/'));
    final expected = contract.outputs[ReconstructionStage.texture]!
        .map((a) => '$root/${a.path}')
        .toSet();
    final actual = <String>{};
    await for (final entity in Directory(parent).list(followLinks: false)) {
      if (await FileSystemEntity.type(entity.path, followLinks: false) !=
          FileSystemEntityType.file) {
        throw StateError('Texture emitted nonregular output');
      }
      actual.add(entity.path);
    }
    if (actual.length != expected.length || !actual.containsAll(expected)) {
      throw StateError('Texture complete output file set mismatch');
    }
  }
}

                                                                             
                                                                               
final class ReconstructionTextureStageDriver implements ReconstructionStageDriver {
  ReconstructionTextureStageDriver({required this.ffi, required this.io}) {
    if (ffi.bindings.isTestOnly) {
      throw StateError(
        'Test callback cannot populate production texture driver',
      );
    }
    ffi.bindings.evidence!.validate();
  }

                                                                              
  ReconstructionTextureStageDriver.testOnly({required this.ffi, required this.io}) {
    if (!ffi.bindings.isTestOnly) {
      throw StateError('Test-only driver requires an explicit callback');
    }
  }
  final ReconstructionTextureFfi ffi;
  final ReconstructionTextureIo io;
  ReconstructionTextureCallResult? lastCallResult;
  bool _running = false;

  @override
  ReconstructionStage get stage => ReconstructionStage.texture;
  @override
  String get sourceManifestSha256 => reconstructionSourceInputManifestSha256;
  @override
  String get binarySha256 =>
      ffi.bindings.evidence?.binarySha256 ??
      '0000000000000000000000000000000000000000000000000000000000000000';
  @override
  List<String> get verifiedCAbiSymbols => ffi.bindings.isTestOnly
      ? const ['TEST_CALLBACK_ONLY_NOT_A_NATIVE_EXPORT']
      : const [reconstructionTextureSymbol];

  @override
  Future<int> run(ReconstructionRunContract contract) async {
    if (_running) throw StateError('Texture stage already has an active run');
    _running = true;
    lastCallResult = null;
    try {
      final paths = await io.validate(contract);
      final result = ffi.run(
        sceneDirectory: paths.scene,
        meshPly: paths.mesh,
        outputPrefix: paths.output,
      );
      lastCallResult = result;
      if (result.entryReturnCode == 0 && result.diagnosticIssue != null) {
        throw StateError(result.diagnosticIssue!);
      }
      if (result.entryReturnCode == 0) {
        await io.verifyEmittedFileSet(contract);
      }
      return result.entryReturnCode;
    } finally {
      _running = false;
    }
  }
}
