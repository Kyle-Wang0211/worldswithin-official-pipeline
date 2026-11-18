import '../../../frame_transport.dart';
import 'dart:io';

                                                                                
enum ReconstructionStage {
  mvs768,
  mvs1536,
  cer,
  fusionPly,
  tsdf,
  trim,
  meshopt,
  lagrange,
  texture,
  avVisibility,
  avSfmToAbc,
  avNormal,
  glb,
}

const reconstructionStageOrder = ReconstructionStage.values;
String get reconstructionSourceInputManifestSha256 {
  final value = Platform.environment['RECONSTRUCTION_REFERENCE_INPUT_MANIFEST'];
  if (value == null || !_isHash(value)) {
    throw StateError('Set the reference input manifest identity for the host');
  }
  return value;
}

                                                                             
                                                                
const reconstructionSelectedRecipe = <String, Object>{
  'frame_count_policy': 'actual_positive_count',
  'low_grid': [768, 576],
  'high_grid': [1536, 1152],
  'noise': 'original SEEDED, seed 0; lineage gate required',
  'cer_relative_threshold': 0.02,
  'fusion_geo_mask_threshold': 2,
  'fusion_geo_pixel_threshold': 0.125,
  'fusion_geo_depth_threshold': 0.01,
  'fusion_check_all_views': true,
  'fusion_sat_tolerance': 1e-5,
  'fusion_min_tri_angle_degrees': 3.0,
  'fusion_photo_first': false,
  'retained_depth_scale': 5000,
  'meshopt_target_triangles': 100000,
  'lagrange': 'resolve_nonmanifoldness; close_small_holes defaults 16/true',
  'texture_grid': [1920, 1440],
  'texture': 'gmi; gauss_clamping; threads=1; no_intermediate_results',
  'normal': 'AliceVision official entry; textureSide=2048; maxCoresAvailable=1',
  'glb': 'selected Trimesh loader/export; exact original sort-order gate',
};

                                                                               
                                                                                
const reconstructionRequiredOutputRoles = <ReconstructionStage, Set<String>>{
  ReconstructionStage.mvs768: {'depth', 'conf0', 'conf1', 'conf2'},
  ReconstructionStage.mvs1536: {'depth', 'conf0', 'conf1', 'conf2'},
  ReconstructionStage.cer: {'depth', 'conf0', 'conf1', 'conf2'},
  ReconstructionStage.fusionPly: {
    'dense_ply',
    'retained_d16',
    'retained_camera',
    'order',
  },
  ReconstructionStage.tsdf: {'mesh_ply'},
  ReconstructionStage.trim: {'mesh_ply'},
  ReconstructionStage.meshopt: {'positions_f32', 'indices_u32', 'mesh_ply'},
  ReconstructionStage.lagrange: {'mesh_ply'},
  ReconstructionStage.texture: {'obj', 'mtl', 'color_png'},
  ReconstructionStage.avVisibility: {'sfm_json', 'visibility_byte_gate'},
  ReconstructionStage.avSfmToAbc: {'sfm_abc'},
  ReconstructionStage.avNormal: {'normal_png'},
  ReconstructionStage.glb: {'glb'},
};

bool _isHash(String value) => RegExp(r'^[0-9a-f]{64}$').hasMatch(value);

String checkedRelativePath(String value) {
  final parts = value.split('/');
  if (value.isEmpty ||
      value.startsWith('/') ||
      value.contains('\\') ||
      value.contains('\x00') ||
      parts.any((p) => p.isEmpty || p == '.' || p == '..')) {
    throw FormatException('Noncanonical relative artifact path: $value');
  }
  return value;
}

final class ReconstructionArtifact {
  ReconstructionArtifact({
    required this.path,
    required this.role,
    required this.bytes,
    required this.sha256,
  }) {
    checkedRelativePath(path);
    if (role.isEmpty || bytes <= 0 || !_isHash(sha256)) {
      throw const FormatException(
        'Artifact requires a role, positive size and SHA-256',
      );
    }
  }
  final String path;
  final String role;
  final int bytes;
  final String sha256;
}

final class ReconstructionSourceGates {
  const ReconstructionSourceGates({
    required this.sourceAccepted,
    required this.originalE0NoiseBytesAccepted,
    required this.platformBytesAccepted,
    required this.vertexColorAccessorBytesAccepted,
  });
  final bool sourceAccepted;
  final bool originalE0NoiseBytesAccepted;
  final bool platformBytesAccepted;
  final bool vertexColorAccessorBytesAccepted;
  bool get accepted =>
      sourceAccepted &&
      originalE0NoiseBytesAccepted &&
      platformBytesAccepted &&
      vertexColorAccessorBytesAccepted;
}

final class ReconstructionRunContract {
  ReconstructionRunContract({
    required this.runId,
    required this.sourceInputManifestSha256,
    required this.orderedInputManifestSha256,
    required this.modelManifestSha256,
    required this.environmentManifestSha256,
    required this.sourceGates,
    required List<ReconstructionArtifact> inputs,
    required Map<ReconstructionStage, List<ReconstructionArtifact>> outputs,
    required this.orderedFrames,
    this.denseProducerProfile = originalRetainedProfile,
  }) : inputs = List.unmodifiable(inputs),
       outputs = Map.unmodifiable(
         outputs.map((k, v) => MapEntry(k, List<ReconstructionArtifact>.unmodifiable(v))),
       );
  final String runId;
  final String sourceInputManifestSha256;
  final String orderedInputManifestSha256;
  final String modelManifestSha256;
  final String environmentManifestSha256;
  final ReconstructionSourceGates sourceGates;
  final List<ReconstructionArtifact> inputs;
  final Map<ReconstructionStage, List<ReconstructionArtifact>> outputs;
  final int orderedFrames;
  final String denseProducerProfile;

  void validate() {
    if (!RegExp(r'^[A-Za-z0-9_-]+$').hasMatch(runId) ||
        orderedFrames != frameTransportCount({'dense_producer_profile': denseProducerProfile}, actualCount: orderedFrames) ||
        sourceInputManifestSha256 != reconstructionSourceInputManifestSha256 ||
        ![
          orderedInputManifestSha256,
          modelManifestSha256,
          environmentManifestSha256,
        ].every(_isHash) ||
        !sourceGates.accepted ||
        inputs.isEmpty) {
      throw StateError(
        'Selected-38 source/input/model/environment gates unresolved',
      );
    }
    final paths = <String>{};
    for (final stage in reconstructionStageOrder) {
      final rows = outputs[stage];
      if (rows == null ||
          rows.isEmpty ||
          !rows
              .map((r) => r.role)
              .toSet()
              .containsAll(reconstructionRequiredOutputRoles[stage]!)) {
        throw StateError('${stage.name} complete output manifest unregistered');
      }
      for (final row in rows) {
        if (!paths.add(row.path))
          throw StateError('Output path reused: ${row.path}');
      }
    }
  }
}

abstract interface class PipelineArtifactGate {
  Future<void> validateInputs(ReconstructionRunContract contract);
  Future<void> requireFreshOutputs(ReconstructionRunContract contract);
  Future<void> verifyOutput(ReconstructionStage stage, ReconstructionArtifact artifact);
}

typedef ReconstructionFileHash = Future<String> Function(File file);
typedef ReconstructionFormatGate = Future<void> Function(
  File file,
  ReconstructionArtifact artifact,
);

                                                                           
                                                                              
final class ReconstructionFileArtifactGate implements PipelineArtifactGate {
  ReconstructionFileArtifactGate({
    required this.inputRoot,
    required this.runRoot,
    required this.sha256File,
    required this.validateFormat,
  });
  final Directory inputRoot;
  final Directory runRoot;
  final ReconstructionFileHash sha256File;
  final ReconstructionFormatGate validateFormat;

  Future<File> _file(Directory root, String relative) async {
    checkedRelativePath(relative);
    var walk = root.absolute.path;
    if (await FileSystemEntity.type(walk, followLinks: false) !=
            FileSystemEntityType.directory ||
        await root.resolveSymbolicLinks() != walk) {
      throw StateError('Artifact root absent, symlinked or noncanonical');
    }
    for (final part in relative.split('/')) {
      walk = '$walk/$part';
      if (await FileSystemEntity.type(walk, followLinks: false) ==
          FileSystemEntityType.link) {
        throw StateError('Symlink artifact rejected');
      }
    }
    return File(walk);
  }

  Future<void> _verify(Directory root, ReconstructionArtifact a) async {
    final file = await _file(root, a.path);
    if (await FileSystemEntity.type(file.path, followLinks: false) !=
            FileSystemEntityType.file ||
        await file.length() != a.bytes ||
        await sha256File(file) != a.sha256) {
      throw StateError('Strict artifact bytes gate failed: ${a.path}');
    }
    await validateFormat(file, a);
  }

  @override
  Future<void> validateInputs(ReconstructionRunContract contract) async {
    for (final a in contract.inputs) {
      await _verify(inputRoot, a);
    }
  }

  @override
  Future<void> requireFreshOutputs(ReconstructionRunContract contract) async {
    for (final rows in contract.outputs.values) {
      for (final a in rows) {
        final file = await _file(runRoot, a.path);
        if (await FileSystemEntity.type(file.path, followLinks: false) !=
            FileSystemEntityType.notFound) {
          throw StateError('Refuse output overwrite: ${a.path}');
        }
      }
    }
  }

  @override
  Future<void> verifyOutput(ReconstructionStage stage, ReconstructionArtifact artifact) =>
      _verify(runRoot, artifact);
}

                                                                               
                                                                               
abstract interface class ReconstructionStageDriver {
  ReconstructionStage get stage;
  String get sourceManifestSha256;
  String get binarySha256;
  List<String> get verifiedCAbiSymbols;
  Future<int> run(ReconstructionRunContract contract);
}

final class ReconstructionStageReceipt {
  const ReconstructionStageReceipt(
    this.stage,
    this.entryReturnCode,
    this.outputGatePassed,
    this.elapsedMicroseconds, [
    this.error,
  ]);
  final ReconstructionStage stage;
  final int? entryReturnCode;
  final bool outputGatePassed;
  final int elapsedMicroseconds;
  final String? error;
}

final class ReconstructionRunResult {
  ReconstructionRunResult({
    required this.exitCode,
    required this.stoppedReason,
    required List<ReconstructionStageReceipt> receipts,
  }) : receipts = List.unmodifiable(receipts);
  final int exitCode;
  final String? stoppedReason;
  final List<ReconstructionStageReceipt> receipts;
  bool get full_pipelineComplete =>
      exitCode == 0 &&
      stoppedReason == null &&
      receipts.length == reconstructionStageOrder.length &&
      receipts.every((r) => r.entryReturnCode == 0 && r.outputGatePassed);
}

final class ReconstructionSharedPipeline {
  ReconstructionSharedPipeline({
    required Map<ReconstructionStage, ReconstructionStageDriver> drivers,
    required this.artifactGate,
  }) : _drivers = Map.unmodifiable(drivers);
  final Map<ReconstructionStage, ReconstructionStageDriver> _drivers;
  final PipelineArtifactGate artifactGate;
  bool _running = false;

                                                                         
  Future<ReconstructionRunResult> run(ReconstructionRunContract contract) async {
    if (_running) {
      return ReconstructionRunResult(
        exitCode: 38,
        stoppedReason: 'Pipeline already has an active run',
        receipts: [],
      );
    }
    _running = true;
    try {
      return await _runOnce(contract);
    } finally {
      _running = false;
    }
  }

  Future<ReconstructionRunResult> _runOnce(ReconstructionRunContract contract) async {
    final receipts = <ReconstructionStageReceipt>[];
    try {
      final missing = reconstructionStageOrder
          .where((s) => !_drivers.containsKey(s))
          .toList();
      if (missing.isNotEmpty)
        throw UnsupportedError(
          'Native C ABI stages unavailable: ${missing.map((s) => s.name).join(', ')}',
        );
      for (final stage in reconstructionStageOrder) {
        final driver = _drivers[stage]!;
        if (driver.stage != stage ||
            driver.sourceManifestSha256 != contract.sourceInputManifestSha256 ||
            !_isHash(driver.binarySha256) ||
            driver.verifiedCAbiSymbols.isEmpty) {
          throw StateError(
            '${stage.name} C ABI/source/binary identity unresolved',
          );
        }
      }
      contract.validate();
      await artifactGate.validateInputs(contract);
      await artifactGate.requireFreshOutputs(contract);
    } catch (e) {
      return ReconstructionRunResult(
        exitCode: 38,
        stoppedReason: e.toString(),
        receipts: receipts,
      );
    }
    for (final stage in reconstructionStageOrder) {
      final watch = Stopwatch()..start();
      int? rc;
      try {
        rc = await _drivers[stage]!.run(contract);
        if (rc != 0) {
          receipts.add(
            ReconstructionStageReceipt(
              stage,
              rc,
              false,
              watch.elapsedMicroseconds,
              'Native stage returned nonzero',
            ),
          );
          return ReconstructionRunResult(
            exitCode: rc,
            stoppedReason: '${stage.name} returned $rc',
            receipts: receipts,
          );
        }
        for (final artifact in contract.outputs[stage]!) {
          await artifactGate.verifyOutput(stage, artifact);
        }
        receipts.add(
          ReconstructionStageReceipt(stage, rc, true, watch.elapsedMicroseconds),
        );
      } catch (e) {
        receipts.add(
          ReconstructionStageReceipt(
            stage,
            rc,
            false,
            watch.elapsedMicroseconds,
            e.toString(),
          ),
        );
        return ReconstructionRunResult(
          exitCode: 38,
          stoppedReason: e.toString(),
          receipts: receipts,
        );
      } finally {
        watch.stop();
      }
    }
    return ReconstructionRunResult(exitCode: 0, stoppedReason: null, receipts: receipts);
  }
}

                                                                               
                                                                             
                                                                              
const reconstructionUnsupportedStages = <ReconstructionStage, String>{
  ReconstructionStage.mvs768: 'C++ DenseJob/dense_run only; isolated C ABI absent',
  ReconstructionStage.mvs1536: 'C++ DenseJob/dense_run only; isolated C ABI absent',
  ReconstructionStage.cer: 'C++ pipeline-internal merge only; isolated C ABI absent',
  ReconstructionStage.fusionPly:
      'C++ FuseCamera/FuseParams/std::vector only; C ABI absent',
  ReconstructionStage.tsdf:
      'Source declarations; dependency/binary/full-phone byte gates held',
  ReconstructionStage.trim: 'Open3D source declarations; verified binary unavailable',
  ReconstructionStage.meshopt: 'Verified file ABI; complete PLY bridge not verified',
  ReconstructionStage.lagrange:
      'Verified file ABI; full selected-input phone bytes not accepted',
  ReconstructionStage.texture: 'Source C ABI only; no verified exported library frozen',
  ReconstructionStage.avVisibility: 'Source C ABI; original color and byte gates held',
  ReconstructionStage.avSfmToAbc: 'Source C ABI; verified binary unavailable',
  ReconstructionStage.avNormal: 'Source C ABI; dependency/binary/phone gates held',
  ReconstructionStage.glb: 'Declared full entry; original ARM OBJ/sort backend held',
};
