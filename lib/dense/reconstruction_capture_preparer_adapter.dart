import '../reconstruction/frame_transport.dart';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';

import '../official_capture/dense_stage.dart';
import '../reconstruction/production_material_preparer.dart';
import '../reconstruction/execution_identity_scope.dart';
import '../reconstruction/memory_policy.dart';
import '../reconstruction/production_original_fixture_binding.dart';
import '../reconstruction/original_retention_io.dart' show usesOriginalRetained;
import '../official_capture/aux_archive_resolver.dart';
import '../official_capture/photo_archive_runtime.dart'
    show databaseArchiveCodec;
import '../reconstruction/production_runtime_binding.dart';
import '../reconstruction/production_whole_runtime.dart';
import '../reconstruction/production_runtime_closure.dart';
import '../reconstruction/request.dart' show hashFile, object, string;
import 'package:worldswithin_flutter/reconstruction/vendor/dart_pipeline/lib/reconstruction_pipeline.dart'
    show checkedRelativePath;
import 'native_dense_stage_launcher.dart';
import 'reconstruction_production_package.dart';
import 'reconstruction_production_launcher.dart' show kReconstructionFullRunsDirName;

                                                                               
Future<Map<String, dynamic>> loadWholeCaptureRuntimeProfile({
  required Map<String, dynamic> package,
  required String bundleRoot,
  ReconstructionExecutionIdentityScope identityScope = ReconstructionExecutionIdentityScope.production,
}) async {
  if (package['dense_producer_profile'] != wholeRetainedTransportProfile) {
    throw StateError('Explicit whole production profile required');
  }
  final relative = checkedRelativePath(string(package, 'whole_profile_path'));
  final file = File('$bundleRoot/$relative');
  final hash = package['whole_profile_sha256'];
  if (hash is! String ||
      !RegExp(r'^[0-9a-f]{64}$').hasMatch(hash) ||
      !bundleRoot.startsWith('/') ||
      await Directory(bundleRoot).resolveSymbolicLinks() != bundleRoot ||
      await FileSystemEntity.type(file.path, followLinks: false) !=
          FileSystemEntityType.file ||
      await file.resolveSymbolicLinks() != file.path ||
      await file.length() > 8 * 1024 * 1024 ||
      await hashFile(file) != hash) {
    throw StateError('Actual canonical package-bound whole profile required');
  }
  final profile = object(jsonDecode(await file.readAsString()));
  if (profile['schema'] != 'RECONSTRUCTION_PRODUCTION_WHOLE_PROFILE_V1' ||
      profile['algorithm_route'] != 'LOCAL_DENSE_RECONSTRUCTION' ||
      await hashFile(file) != hash) {
    throw StateError('Selected whole production profile identity changed');
  }
  requireWholeMemoryPolicy(profile, identityScope);
  return profile;
}

                                                                               
                                                                             
Future<Map<String, dynamic>> wholeCaptureRunBindings({
  required OriginalFixtureExecutionProof proof,
  required Map<String, dynamic> observerMetadata,
  required String runRoot,
}) async {
  await proof.revalidate();
  if (proof.outputRoot != '$runRoot/original_fixture' ||
      proof.receiptPath != '$runRoot/ORIGINAL_FIXTURE_RECEIPT.json' ||
      observerMetadata['m0_trace_path'] != '$runRoot/whole_app_m0.jsonl' ||
      observerMetadata['m0_run_id'] is! String ||
      observerMetadata['m0_start_return_code'] != 0 ||
      observerMetadata['m0_stop_return_code'] != null) {
    throw StateError('Whole actual proof/active observer run identity differs');
  }
  Future<Map<String, dynamic>> artifact(String path) async {
    final file = File(path);
    if (await FileSystemEntity.type(path, followLinks: false) !=
            FileSystemEntityType.file ||
        await file.resolveSymbolicLinks() != path ||
        await file.length() <= 0) {
      throw StateError('Actual original fixture artifact required: $path');
    }
    return {
      'path': path,
      'bytes': await file.length(),
      'sha256': await hashFile(file),
    };
  }

  final gray = await artifact('${proof.outputRoot}/images.f16');
  final expectedGray = (proof.receipt['outputs'] as List)
      .map(object)
      .singleWhere((row) => row['path'] == 'images.f16');
  if (gray['bytes'] != expectedGray['bytes'] ||
      gray['sha256'] != expectedGray['sha256']) {
    throw StateError(
      'Actual LOW grayscale differs from original execution proof',
    );
  }
  return {
    'low_gray': gray,
    'original_fixture_receipt': await artifact(proof.receiptPath),
    'm0_trace_path': observerMetadata['m0_trace_path'],
    'm0_run_id': observerMetadata['m0_run_id'],
  };
}

                                                                               
                                                                                
                                                                      
final class ReconstructionCapturePreparerAdapter {
  ReconstructionCapturePreparerAdapter(ReconstructionProductionPackage package)
    : _config = object(
        jsonDecode(
          jsonEncode({
            ...package.manifest,
            'bundle_root': package.bundleRoot,
            'package_sha256': package.manifestSha256,
            if (package.identityScope.isIndependentTest)
              'identity_scope': package.identityScope.toJson(),
          }),
        ),
      );
  final Map<String, dynamic> _config;

  Future<Map<String, String>> call({
    required DenseStageRequest request,
    required DenseStageInputs inputs,
    required String workDir,
  }) async {
    if (Isolate.current.debugName == 'main') {
      throw StateError(
        'Production material preparation requires worker isolate',
      );
    }
    final config = object(jsonDecode(jsonEncode(_config)));
    final identityScope = config['identity_scope'] == null
        ? ReconstructionExecutionIdentityScope.production
        : ReconstructionExecutionIdentityScope.fromJson(object(config['identity_scope']));
    await identityScope.revalidate();
    identityScope.requirePackage(config);
    final bundle = string(config, 'bundle_root');
    if (identityScope.isIndependentTest && identityScope.bundleRoot != bundle) {
      throw StateError(
        'Worker bundle differs from independently measured scope',
      );
    }
    final runtimeClosure = await ProductionRuntimeClosure.capture(
      bundleRoot: bundle,
      package: config,
      packageSha256: string(config, 'package_sha256'),
      provider: object(config['camera_native_row']),
    );
    final runRoot = await Directory(workDir).resolveSymbolicLinks();
    final captureRoot = await Directory(
      request.captureDir,
    ).resolveSymbolicLinks();
    identityScope.requireCapture(captureRoot);
    if (!runRoot.startsWith('$captureRoot/$kReconstructionFullRunsDirName/')) {
      throw StateError('Durable full_pipeline run must belong to actual capture');
    }
    final identities = inputs.frameIdentities;
    if (inputs.frames.length !=
            frameTransportCount(config, actualCount: inputs.frames.length) ||
        identities == null ||
        identities.length != inputs.frames.length ||
        request.selection != null ||
        inputs.pointsXyz.isEmpty ||
        inputs.pointsXyz.length % 3 != 0 ||
        inputs.pointsXyz.length ~/ 3 > 0x7fffffff) {
      throw StateError(
        'ReferenceCapture refined JPEG identities and native-count sparse xyz triples required',
      );
    }
    final rows = <Map<String, dynamic>>[];
    for (var i = 0; i < inputs.frames.length; i++) {
      final frame = inputs.frames[i], identity = identities[i];
      final bits = DenseFrameIdentity.fromDouble(
        identity.frameId,
      ).frameIdF64Bits;
      if (!identity.frameId.isFinite ||
          identity.frameIdF64Bits != bits ||
          identity.frameId != frame.frameId.toDouble() ||
          (i > 0 && identity.frameId <= identities[i - 1].frameId) ||
          frame.hasNv12 ||
          frame.jpegPath.isEmpty) {
        throw StateError(
          'Original ordered frame binary64/JPEG identity changed',
        );
      }
      final jpeg = File(frame.jpegPath);
      if (await FileSystemEntity.type(jpeg.path, followLinks: false) !=
          FileSystemEntityType.file) {
        throw StateError('Real photo resolver JPEG required: ${jpeg.path}');
      }
      rows.add({
        'frame_id': identity.frameId,
        'frame_id_f64_bits': bits,
        'fx': frame.fx,
        'fy': frame.fy,
        'cx': frame.cx,
        'cy': frame.cy,
        'image_w': frame.imageW,
        'image_h': frame.imageH,
        'q_wxyz': List<double>.from(frame.qWxyz),
        't': List<double>.from(frame.t),
        'jpeg_path': await jpeg.resolveSymbolicLinks(),
      });
    }
    final transport = Directory('$runRoot/capture_transport');
    if (await FileSystemEntity.type(transport.path, followLinks: false) !=
        FileSystemEntityType.notFound) {
      throw StateError(
        'Existing capture transport retained; fresh run required',
      );
    }
    await transport.create();
    final sparse = File('${transport.path}/actual_sparse_xyz.f32');
    await _writeSparseF32(sparse, inputs.pointsXyz);
    final whole =
        config['dense_producer_profile'] == wholeRetainedTransportProfile;
    Map<String, dynamic>? wholeProfile, wholeObserverMetadata;
    if (whole) {
      wholeProfile = await loadWholeCaptureRuntimeProfile(
        package: config,
        bundleRoot: bundle,
        identityScope: identityScope,
      );
      await runtimeClosure.revalidate();
      wholeObserverMetadata = await initializeWholeProductionRuntime(
        profile: wholeProfile,
        bundleRoot: bundle,
        runRoot: runRoot,
        identityScope: identityScope,
      );
    }
    try {
      OriginalFixtureSnapshot? originalSnapshot;
      if (usesOriginalRetained(config)) {
        final sidecarArchive = await OriginalFixtureSidecarArchive.capture(
          captureRoot,
        );
        final sidecars = await AuxArchiveResolver(
          codec: databaseArchiveCodec,
        ).openSidecars(Directory(captureRoot));
        if (sidecars == null || inputs.jpegOrigins == null) {
          await sidecars?.dispose();
          throw StateError(
            'Actual original full sidecars and JPEG materialization routes required',
          );
        }
        try {
          originalSnapshot = await snapshotOriginalFixtureCapture(
            captureRoot: captureRoot,
            sidecarArchive: sidecars.materialized ? sidecarArchive : null,
            sidecarsRoot: await sidecars.directory.resolveSymbolicLinks(),
            destinationRoot: '$runRoot/original_capture',
            manifestPath: '$runRoot/ORIGINAL_CAPTURE_INPUT_MANIFEST.json',
            orderedFrames: rows,
            profile: config['dense_producer_profile'] as String,
            jpegOrigins: inputs.jpegOrigins!,
          );
        } finally {
          await sidecars.dispose();
        }
      }
      final materialConfig = ProductionMaterialPreparationConfig(
        bundleRoot: bundle,
        templatePath: '$bundle/${string(config, 'template_path')}',
        templateSha256: string(config, 'template_sha256'),
        inputRoot: '$runRoot/inputs',
        outputRoot: '$runRoot/outputs',
        preparationRoot: '$runRoot/preparation',
        runId: Directory(
          runRoot,
        ).uri.pathSegments.where((p) => p.isNotEmpty).last,
        cleanupRoots: [
          '$captureRoot/dense_work',
          '$captureRoot/reconstruction_work',
        ],
        frameRows: rows,
        sparseXyzSourcePath: sparse.path,
        noiseSourceRoot:
            '$bundle/${string(config, 'noise_source_root_relative_path')}',
        sessionNativeRow: object(config['session_native_row']),
        cameraNativeRow: object(config['camera_native_row']),
        imageNativeRow: object(config['image_native_row']),
        pillowNativeRow: object(config['pillow_native_row']),
        originalFixtureSnapshot: originalSnapshot,
        runtimeClosure: runtimeClosure,
        originalFixtureNativeRow: originalSnapshot == null
            ? null
            : object(config['original_fixture_native_row']),
      );
                                                                                 
                                                                                  
      await runtimeClosure.revalidate();
      final runtimeReceipt =
          wholeObserverMetadata ??
          await initializeProductionNativeRuntime(
            object(config['runtime_bootstrap_native_row']),
            bundle,
            identityScope: identityScope,
          );
      final runtimeReceiptFile = File(
        '$runRoot/native_runtime_initialization.json',
      );
      await runtimeReceiptFile.writeAsString(
        jsonEncode(runtimeReceipt),
        flush: true,
      );
      if (jsonEncode(jsonDecode(await runtimeReceiptFile.readAsString())) !=
          jsonEncode(runtimeReceipt)) {
        throw StateError('Native runtime initialization receipt changed');
      }
      final prepared = await prepareProductionMaterials(materialConfig);
      final bootstrap = <String, String>{
        'bundle_id': string(config, 'bundle_id'),
        'bundle_path': bundle,
        'request_path': prepared.requestFile.path,
        'request_sha256': prepared.requestSha256,
        'template_request_sha256': string(config, 'template_sha256'),
        'input_binding_path': prepared.receiptFile.path,
        'input_binding_sha256': prepared.receiptSha256,
        if (identityScope.isIndependentTest)
          'identity_scope': jsonEncode(identityScope.toJson()),
        if (whole) ...{
          'whole_profile_path':
              '$bundle/${checkedRelativePath(string(config, 'whole_profile_path'))}',
          'whole_profile_sha256': string(config, 'whole_profile_sha256'),
          'whole_observer_metadata': jsonEncode(wholeObserverMetadata),
          'whole_run_bindings': jsonEncode(
            await wholeCaptureRunBindings(
              proof:
                  prepared.originalFixtureProof ??
                  (throw StateError(
                    'Whole capture requires actual original fixture execution proof',
                  )),
              observerMetadata: wholeObserverMetadata!,
              runRoot: runRoot,
            ),
          ),
        },
      };
      return prepared.originalFixtureProof == null
          ? bootstrap
          : OriginalFixtureBoundBootstrap(
              bootstrap,
              prepared.originalFixtureProof!,
            );
    } catch (error, stack) {
      if (wholeObserverMetadata != null) {
        try {
          await stopWholeProductionObserver(
            metadata: wholeObserverMetadata,
            profile: wholeProfile!,
            bundleRoot: bundle,
            identityScope: identityScope,
          );
        } catch (stopError) {
          throw WholeCapturePreparationFailure(error, stopError);
        }
      }
      Error.throwWithStackTrace(error, stack);
    }
  }
}

final class WholeCapturePreparationFailure implements Exception {
  WholeCapturePreparationFailure(this.preparationError, this.observerStopError);
  final Object preparationError, observerStopError;
  @override
  String toString() =>
      'Whole capture preparation failed: $preparationError; '
      'observer stop also failed: $observerStopError';
}

                                                                               
                                                                                
Future<void> _writeSparseF32(File file, List<double> xyz) async {
  final handle = await file.open(mode: FileMode.write);
  try {
    const blockFloats = 16384;
    for (var start = 0; start < xyz.length; start += blockFloats) {
      final count = (xyz.length - start) < blockFloats
          ? xyz.length - start
          : blockFloats;
      final block = ByteData(count * 4);
      for (var i = 0; i < count; i++) {
        final value = xyz[start + i];
        if (!value.isFinite) throw StateError('Nonfinite real sparse xyz');
        block.setFloat32(i * 4, value, Endian.little);
        if (!block.getFloat32(i * 4, Endian.little).isFinite) {
          throw StateError(
            'Real sparse xyz overflows original float32 transport',
          );
        }
      }
      await handle.writeFrom(block.buffer.asUint8List());
    }
    await handle.flush();
  } finally {
    await handle.close();
  }
  if (await file.length() != xyz.length * 4) {
    throw StateError('Sparse xyz transport length mismatch');
  }
}
