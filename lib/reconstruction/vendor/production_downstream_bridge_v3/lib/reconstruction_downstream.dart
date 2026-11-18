import '../../../frame_transport.dart';
import '../../dart_pipeline/lib/reconstruction_pipeline.dart';

export '../../dart_pipeline/lib/reconstruction_pipeline.dart'
    show
        ReconstructionArtifact,
        PipelineArtifactGate,
        ReconstructionRunContract,
        ReconstructionSourceGates,
        ReconstructionStage,
        ReconstructionStageDriver,
        ReconstructionStageReceipt;

const reconstructionImportedRouteId = 'reconstruction_original_v2';
const reconstructionDownstreamStages = <ReconstructionStage>[
  ReconstructionStage.tsdf,
  ReconstructionStage.trim,
  ReconstructionStage.meshopt,
  ReconstructionStage.lagrange,
  ReconstructionStage.texture,
  ReconstructionStage.avVisibility,
  ReconstructionStage.avSfmToAbc,
  ReconstructionStage.avNormal,
  ReconstructionStage.glb,
];
const reconstructionImportedUpstreamStages = <ReconstructionStage>[
  ReconstructionStage.mvs768,
  ReconstructionStage.mvs1536,
  ReconstructionStage.cer,
  ReconstructionStage.fusionPly,
];
bool _hash(String value) => RegExp(r'^[0-9a-f]{64}$').hasMatch(value);

String _root(String value) {
  if (!value.startsWith('/') ||
      value.length < 2 ||
      value.contains('\\') ||
      value.contains('\x00') ||
      value.split('/').skip(1).any((p) => p.isEmpty || p == '.' || p == '..')) {
    throw FormatException('Noncanonical absolute IO root: $value');
  }
  return value;
}

bool _inside(String path, String root) =>
    path == root || path.startsWith('$root/');

                                                                              
                                                                                 
final class ReconstructionIoRoots {
  const ReconstructionIoRoots({
    required this.durableInputRoot,
    required this.downstreamOutputRoot,
    required this.textureSceneRoot,
  });
  final String durableInputRoot, downstreamOutputRoot, textureSceneRoot;
  bool same(ReconstructionIoRoots other) =>
      durableInputRoot == other.durableInputRoot &&
      downstreamOutputRoot == other.downstreamOutputRoot &&
      textureSceneRoot == other.textureSceneRoot;
  void validate(List<String> cleanupRoots) {
    for (final root in [
      durableInputRoot,
      downstreamOutputRoot,
      textureSceneRoot,
    ]) {
      _root(root);
    }
    if (!_inside(textureSceneRoot, durableInputRoot) ||
        textureSceneRoot == durableInputRoot ||
        _inside(durableInputRoot, downstreamOutputRoot) ||
        _inside(downstreamOutputRoot, durableInputRoot)) {
      throw ReconstructionIoBindingError('Input/output/texture scene ownership overlaps');
    }
    if (cleanupRoots.length != 2 || cleanupRoots[0] == cleanupRoots[1]) {
      throw ReconstructionIoBindingError('Both distinct cleanup roots required');
    }
    for (final cleanupRoot in cleanupRoots) {
      _root(cleanupRoot);
      for (final retainedRoot in [durableInputRoot, downstreamOutputRoot]) {
        if (_inside(retainedRoot, cleanupRoot) ||
            _inside(cleanupRoot, retainedRoot)) {
          throw ReconstructionIoBindingError(
            'Durable input/output overlaps production cleanup',
          );
        }
      }
    }
  }
}

final class ReconstructionIoBindingError implements Exception {
  const ReconstructionIoBindingError(this.message);
  final String message;
  @override
  String toString() => 'Actual IO binding rejected: $message';
}

abstract interface class ReconstructionIoBound {
  ReconstructionIoRoots get ioRoots;
}

                                                                                
final class ReconstructionDownstreamContract {
  const ReconstructionDownstreamContract({required this.value, required this.ioRoots});
  final ReconstructionRunContract value;
  final ReconstructionIoRoots ioRoots;
}

bool _sameArtifact(ReconstructionArtifact a, ReconstructionArtifact b) =>
    a.path == b.path &&
    a.role == b.role &&
    a.bytes == b.bytes &&
    a.sha256 == b.sha256;

                                                                            
                                                                               
                                                                          
final class ReconstructionRetainedFrame {
  const ReconstructionRetainedFrame({
    required this.originalIndex,
    required this.packIndex,
    required this.frameIdF64Bits,
    required this.d16,
    required this.cameraF64,
    required this.metadata,
    required this.textureImage,
    required this.texture1920Camera,
    required this.normalImage,
    required this.original4032Camera,
    this.width = 1536,
    this.height = 1152,
    this.depthScale = 5000,
    this.textureWidth = 1920,
    this.textureHeight = 1440,
    this.originalImageWidth = 4032,
    this.originalImageHeight = 3024,
  });
  final int originalIndex, packIndex;
  final String frameIdF64Bits;
  final ReconstructionArtifact d16, cameraF64, metadata;
  final ReconstructionArtifact textureImage, normalImage, original4032Camera;
  final ReconstructionArtifact? texture1920Camera;
  final int width, height, depthScale, textureWidth, textureHeight;
  final int originalImageWidth, originalImageHeight;
  List<ReconstructionArtifact> get artifacts => [
    d16,
    cameraF64,
    metadata,
    textureImage,
    if (texture1920Camera != null) texture1920Camera!,
    normalImage,
    original4032Camera,
  ];
  void validate(int index, String sceneRelativePath) {
    if (originalIndex != index ||
        packIndex != index ||
        !RegExp(r'^[0-9a-f]{16}$').hasMatch(frameIdF64Bits) ||
        width != 1536 ||
        height != 1152 ||
        depthScale != 5000 ||
        d16.bytes != width * height * 2 ||
        cameraF64.bytes != 25 * 8 ||
        textureWidth != 1920 ||
        textureHeight != 1440 ||
        originalImageWidth != 4032 ||
        originalImageHeight != 3024 ||
        d16.role != 'retained_d16' ||
        cameraF64.role != 'retained_camera' ||
        metadata.role != 'retained_frame_metadata' ||
        textureImage.role != 'texture_image' ||
        texture1920Camera?.role != 'texture_1920_camera' ||
        normalImage.role != 'normal_image' ||
        original4032Camera.role != 'original_4032_camera') {
      throw StateError(
        'Selected actual-count retained frame $index identity/shape/role unresolved',
      );
    }
    final imageName = textureImage.path.split('/').last;
    final dot = imageName.lastIndexOf('.');
    if (dot <= 0 ||
        textureImage.path != '$sceneRelativePath/$imageName' ||
        texture1920Camera!.path !=
            '$sceneRelativePath/${imageName.substring(0, dot)}.cam') {
      throw StateError(
        'Original1920 image/camera scene pair missing for frame $index',
      );
    }
  }
}

final class ReconstructionImportedUpstream {
  ReconstructionImportedUpstream({
    required this.routeId,
    this.denseProducerProfile = originalRetainedProfile,
    required this.manifest,
    required this.order,
    required this.densePly,
    required this.durableInputRoot,
    required this.textureSceneRoot,
    required this.sourceCaptureDir,
    required this.sourceDensePlyPath,
    required this.producerSourceManifestSha256,
    required this.producerBinarySha256,
    required this.orderedInputManifestSha256,
    required this.modelManifestSha256,
    required this.environmentManifestSha256,
    required this.sourceGates,
    required List<ReconstructionRetainedFrame> orderedFrames,
    required List<ReconstructionStageReceipt> acceptedUpstreamReceipts,
  }) : orderedFrames = List.unmodifiable(orderedFrames),
       acceptedUpstreamReceipts = List.unmodifiable(acceptedUpstreamReceipts);
  final String denseProducerProfile;
  final String routeId,
      durableInputRoot,
      textureSceneRoot,
      sourceCaptureDir,
      sourceDensePlyPath;
  final ReconstructionArtifact manifest, order, densePly;
  final String producerSourceManifestSha256, producerBinarySha256;
  final String orderedInputManifestSha256,
      modelManifestSha256,
      environmentManifestSha256;
  final ReconstructionSourceGates sourceGates;
  final List<ReconstructionRetainedFrame> orderedFrames;
  final List<ReconstructionStageReceipt> acceptedUpstreamReceipts;
  List<ReconstructionArtifact> get artifacts => List.unmodifiable([
    manifest,
    order,
    densePly,
    for (final frame in orderedFrames) ...frame.artifacts,
  ]);

  void validate(List<String> cleanupRoots) {
    _root(durableInputRoot);
    _root(textureSceneRoot);
    if (!_inside(textureSceneRoot, durableInputRoot) ||
        textureSceneRoot == durableInputRoot) {
      throw StateError(
        'Texture scene must be retained under immutable input root',
      );
    }
    _root(sourceCaptureDir);
    _root(sourceDensePlyPath);
    if (routeId != reconstructionImportedRouteId ||
        orderedFrames.length != frameTransportCount({'dense_producer_profile': denseProducerProfile}, actualCount: orderedFrames.length) ||
        manifest.role != 'accepted_upstream_manifest' ||
        order.role != 'order' ||
        densePly.role != 'dense_ply' ||
        !sourceGates.accepted ||
        ![
          producerSourceManifestSha256,
          producerBinarySha256,
          orderedInputManifestSha256,
          modelManifestSha256,
          environmentManifestSha256,
        ].every(_hash)) {
      throw StateError(
        'Explicit reconstruction original actual-count accepted upstream manifest required',
      );
    }
    if (cleanupRoots.length != 2 || cleanupRoots[0] == cleanupRoots[1]) {
      throw StateError(
        'Both distinct production cleanup roots must be registered',
      );
    }
    for (final root in cleanupRoots) {
      _root(root);
      if (_inside(durableInputRoot, root) || _inside(root, durableInputRoot)) {
        throw StateError(
          'Durable upstream overlaps production cleanup ownership',
        );
      }
    }
    final paths = <String>{}, ids = <String>{};
    for (var i = 0; i < orderedFrames.length; i++) {
      orderedFrames[i].validate(
        i,
        textureSceneRoot.substring(durableInputRoot.length + 1),
      );
      if (!ids.add(orderedFrames[i].frameIdF64Bits))
        throw StateError('Duplicate retained frame identity');
    }
    for (final a in artifacts) {
      if (!paths.add(a.path))
        throw StateError('Duplicate upstream artifact path');
    }
    if (acceptedUpstreamReceipts.length != 4)
      throw StateError('Four upstream stage receipts required');
    for (var i = 0; i < 4; i++) {
      final receipt = acceptedUpstreamReceipts[i];
      if (receipt.stage != reconstructionImportedUpstreamStages[i] ||
          receipt.entryReturnCode != 0 ||
          !receipt.outputGatePassed ||
          receipt.error != null) {
        throw StateError(
          'Upstream stage was not accepted: ${reconstructionImportedUpstreamStages[i].name}',
        );
      }
    }
  }
}

                                                                          
                                                                               
final class ReconstructionTextureSceneAcceptance {
  ReconstructionTextureSceneAcceptance({
    required this.sceneRoot,
    required List<ReconstructionArtifact> orderedImages,
    required List<ReconstructionArtifact> ordered1920Cameras,
    required this.completeDirectoryEnumerationAccepted,
    required this.original1920CameraFormatAccepted,
    required this.imageAndCameraDimensionsAccepted,
  }) : orderedImages = List.unmodifiable(orderedImages),
       ordered1920Cameras = List.unmodifiable(ordered1920Cameras);
  final String sceneRoot;
  final List<ReconstructionArtifact> orderedImages, ordered1920Cameras;
  final bool completeDirectoryEnumerationAccepted,
      original1920CameraFormatAccepted,
      imageAndCameraDimensionsAccepted;
  void bind(ReconstructionImportedUpstream upstream) {
    if (sceneRoot != upstream.textureSceneRoot ||
        !completeDirectoryEnumerationAccepted ||
        !original1920CameraFormatAccepted ||
        !imageAndCameraDimensionsAccepted ||
        orderedImages.length != upstream.orderedFrames.length ||
        ordered1920Cameras.length != upstream.orderedFrames.length) {
      throw StateError(
        'Complete original1920 texture scene/camera format gate missing',
      );
    }
    for (var i = 0; i < upstream.orderedFrames.length; i++) {
      if (!_sameArtifact(
            orderedImages[i],
            upstream.orderedFrames[i].textureImage,
          ) ||
          !_sameArtifact(
            ordered1920Cameras[i],
            upstream.orderedFrames[i].texture1920Camera!,
          )) {
        throw StateError(
          'Actual texture scene enumeration/order/camera identity changed',
        );
      }
    }
  }
}

                                                                             
                                                                                  
final class ReconstructionUpstreamAcceptance {
  ReconstructionUpstreamAcceptance({
    required this.manifestSha256,
    required this.ioRoots,
    required this.textureScene,
    required List<String> checkedCleanupRoots,
    required this.producerBinarySha256,
    required this.producerSourceManifestSha256,
    required List<String> actualDefinedRetentionApiSymbols,
    required this.durableReceiptSha256,
    required this.nativeRetentionApiAccepted,
    required this.completeImmutableBytesAccepted,
    required this.formatsAndOrderAccepted,
    required this.resolvedInputAndOutputRootsAccepted,
  }) : checkedCleanupRoots = List.unmodifiable(checkedCleanupRoots),
       actualDefinedRetentionApiSymbols = List.unmodifiable(
         actualDefinedRetentionApiSymbols,
       );
  final String manifestSha256, producerBinarySha256, durableReceiptSha256;
  final ReconstructionIoRoots ioRoots;
  final ReconstructionTextureSceneAcceptance textureScene;
  final List<String> checkedCleanupRoots;
  final String producerSourceManifestSha256;
  final List<String> actualDefinedRetentionApiSymbols;
  final bool nativeRetentionApiAccepted,
      completeImmutableBytesAccepted,
      formatsAndOrderAccepted,
      resolvedInputAndOutputRootsAccepted;
  void bind(
    ReconstructionImportedUpstream upstream,
    ReconstructionDownstreamContract contract,
    List<String> cleanupRoots,
  ) {
    if (checkedCleanupRoots.length != cleanupRoots.length ||
        !Iterable.generate(cleanupRoots.length)
            .every((i) => checkedCleanupRoots[i] == cleanupRoots[i]) ||
        !ioRoots.same(contract.ioRoots) ||
        ioRoots.durableInputRoot != upstream.durableInputRoot ||
        ioRoots.textureSceneRoot != upstream.textureSceneRoot ||
        !resolvedInputAndOutputRootsAccepted) {
      throw ReconstructionIoBindingError(
        'Durable validator actual roots do not bind contract',
      );
    }
    if (manifestSha256 != upstream.manifest.sha256 ||
        producerBinarySha256 != upstream.producerBinarySha256 ||
        !_hash(durableReceiptSha256) ||
        producerSourceManifestSha256 != upstream.producerSourceManifestSha256 ||
        actualDefinedRetentionApiSymbols.isEmpty ||
        !nativeRetentionApiAccepted ||
        !completeImmutableBytesAccepted ||
        !formatsAndOrderAccepted) {
      throw StateError(
        'Actual native retention/durable input acceptance incomplete',
      );
    }
    textureScene.bind(upstream);
  }
}

abstract interface class ReconstructionDurableUpstreamAcceptor implements ReconstructionIoBound {
  Future<ReconstructionUpstreamAcceptance> accept(
    ReconstructionImportedUpstream upstream,
    ReconstructionDownstreamContract contract,
    List<String> cleanupRoots,
  );
}

                                                                               
                                                                               
final class ReconstructionNativeIoAcceptance {
  ReconstructionNativeIoAcceptance({
    required this.stage,
    required this.driverIoRoots,
    required this.artifactGateIoRoots,
    required List<String> checkedCleanupRoots,
    required this.sourceManifestSha256,
    required this.binarySha256,
    required this.receiptSha256,
    required this.actualResolvedAdapterAndArtifactRootsAccepted,
  }) : checkedCleanupRoots = List.unmodifiable(checkedCleanupRoots);
  final ReconstructionStage stage;
  final ReconstructionIoRoots driverIoRoots, artifactGateIoRoots;
  final List<String> checkedCleanupRoots;
  final String sourceManifestSha256, binarySha256, receiptSha256;
  final bool actualResolvedAdapterAndArtifactRootsAccepted;
  void bind(
    ReconstructionStageDriver driver,
    ReconstructionDownstreamContract contract,
    List<String> cleanupRoots,
  ) {
    if (checkedCleanupRoots.length != cleanupRoots.length ||
        !Iterable.generate(cleanupRoots.length)
            .every((i) => checkedCleanupRoots[i] == cleanupRoots[i]) ||
        !driverIoRoots.same(contract.ioRoots) ||
        !artifactGateIoRoots.same(contract.ioRoots) ||
        !actualResolvedAdapterAndArtifactRootsAccepted) {
      throw ReconstructionIoBindingError(
        'Actual native adapter/artifact roots unaccepted',
      );
    }
    if (stage != driver.stage ||
        sourceManifestSha256 != driver.sourceManifestSha256 ||
        binarySha256 != driver.binarySha256 ||
        !_hash(receiptSha256)) {
      throw StateError(
        'Actual driver receipt belongs to another source/binary/stage',
      );
    }
  }
}

abstract interface class ReconstructionNativeCapabilityGate implements ReconstructionIoBound {
                                                                           
                                                               
  Future<ReconstructionNativeIoAcceptance> requireAccepted(
    ReconstructionStageDriver driver,
    ReconstructionDownstreamContract contract,
    ReconstructionDownstreamArtifactGate artifacts,
    List<String> cleanupRoots,
  );
}

final class ReconstructionOutputAcceptance {
  const ReconstructionOutputAcceptance({
    required this.stage,
    required this.ioRoots,
    required this.actualOutputPath,
    required this.artifact,
    required this.driverBinarySha256,
    required this.driverSourceManifestSha256,
    required this.receiptSha256,
    required this.actualOutputBytesAndResolvedPathAccepted,
  });
  final ReconstructionStage stage;
  final ReconstructionIoRoots ioRoots;
  final String actualOutputPath,
      driverBinarySha256,
      driverSourceManifestSha256,
      receiptSha256;
  final ReconstructionArtifact artifact;
  final bool actualOutputBytesAndResolvedPathAccepted;
  void bind(
    ReconstructionStageDriver driver,
    ReconstructionDownstreamContract contract,
    ReconstructionArtifact expected,
  ) {
    if (!ioRoots.same(contract.ioRoots) ||
        actualOutputPath !=
            '${contract.ioRoots.downstreamOutputRoot}/${expected.path}' ||
        !actualOutputBytesAndResolvedPathAccepted) {
      throw ReconstructionIoBindingError(
        'Checked output is outside bound actual driver/output roots',
      );
    }
    if (stage != driver.stage ||
        !_sameArtifact(artifact, expected) ||
        driverBinarySha256 != driver.binarySha256 ||
        driverSourceManifestSha256 != driver.sourceManifestSha256 ||
        !_hash(receiptSha256)) {
      throw StateError(
        'Output acceptance artifact/driver/source/binary mismatch',
      );
    }
  }
}

abstract interface class ReconstructionDownstreamArtifactGate
    implements PipelineArtifactGate, ReconstructionIoBound {
  Future<ReconstructionOutputAcceptance> verifyBoundOutput(
    ReconstructionStageDriver driver,
    ReconstructionDownstreamContract contract,
    ReconstructionArtifact artifact,
  );
}

abstract interface class ReconstructionDownstreamJournal implements ReconstructionIoBound {
  Future<void> started(
    ReconstructionImportedUpstream upstream,
    ReconstructionDownstreamContract contract,
    ReconstructionUpstreamAcceptance acceptance,
  );
  Future<void> stageStarted(ReconstructionStage stage);
  Future<void> stageFinished(ReconstructionStageReceipt receipt);
  Future<void> stopped(String reason, int exitCode);
  Future<void> completed(List<ReconstructionStageReceipt> receipts);
}

final class ReconstructionDownstreamResult {
  ReconstructionDownstreamResult._({
    required this.exitCode,
    required this.stoppedReason,
    required this.upstreamAcceptance,
    required List<ReconstructionStageReceipt> receipts,
  }) : receipts = List.unmodifiable(receipts);
  factory ReconstructionDownstreamResult.blocked(String reason) => ReconstructionDownstreamResult._(
    exitCode: 38,
    stoppedReason: reason,
    upstreamAcceptance: null,
    receipts: [],
  );
  final int exitCode;
  final String? stoppedReason;
  final ReconstructionUpstreamAcceptance? upstreamAcceptance;
  final List<ReconstructionStageReceipt> receipts;
                                                                              
  bool get durableIoAccepted => upstreamAcceptance != null;
  bool get durableInputsAccepted => durableIoAccepted;
  bool get full_pipelineComplete =>
      durableIoAccepted &&
      exitCode == 0 &&
      stoppedReason == null &&
      receipts.length == reconstructionDownstreamStages.length &&
      receipts.every(
        (r) => r.entryReturnCode == 0 && r.outputGatePassed && r.error == null,
      );
}

                                                                             
/// constructs upstream no-op drivers. Existing native C/C++ stage drivers are
                                                                               
final class ReconstructionImportedDownstream {
  ReconstructionImportedDownstream({
    required Map<ReconstructionStage, ReconstructionStageDriver> drivers,
    required this.upstreamAcceptor,
    required this.capabilities,
    required this.artifacts,
    required this.journal,
  }) : _drivers = Map.unmodifiable(drivers);
  final Map<ReconstructionStage, ReconstructionStageDriver> _drivers;
  final ReconstructionDurableUpstreamAcceptor upstreamAcceptor;
  final ReconstructionNativeCapabilityGate capabilities;
  final PipelineArtifactGate artifacts;
  final ReconstructionDownstreamJournal journal;
  bool _running = false;

  void _assertIoBindings(ReconstructionDownstreamContract contract) {
    for (final object in <Object>[
      upstreamAcceptor,
      capabilities,
      artifacts,
      journal,
      ..._drivers.values,
    ]) {
      if (object is! ReconstructionIoBound || !object.ioRoots.same(contract.ioRoots)) {
        throw ReconstructionIoBindingError(
          'Unbound or mismatched real driver/validator/journal IO roots',
        );
      }
    }
    if (artifacts is! ReconstructionDownstreamArtifactGate) {
      throw ReconstructionIoBindingError(
        'Actual artifact gate lacks bound output acceptance',
      );
    }
  }

  void _validateContract(
    ReconstructionImportedUpstream upstream,
    ReconstructionRunContract contract,
  ) {
    if (!RegExp(r'^[A-Za-z0-9_-]+$').hasMatch(contract.runId) ||
        contract.orderedFrames != upstream.orderedFrames.length || contract.denseProducerProfile != upstream.denseProducerProfile ||
        contract.sourceInputManifestSha256 != reconstructionSourceInputManifestSha256 ||
        contract.orderedInputManifestSha256 !=
            upstream.orderedInputManifestSha256 ||
        contract.modelManifestSha256 != upstream.modelManifestSha256 ||
        contract.environmentManifestSha256 !=
            upstream.environmentManifestSha256 ||
        !contract.sourceGates.accepted ||
        contract.outputs.length != 9 ||
        contract.outputs.keys.any((s) => !reconstructionDownstreamStages.contains(s))) {
      throw StateError(
        'Reconstruction downstream contract/source/input/model/environment mismatch',
      );
    }
    final expected = upstream.artifacts;
    if (contract.inputs.length != expected.length)
      throw StateError('Complete imported manifest must be reused');
    for (var i = 0; i < expected.length; i++) {
      final a = expected[i], b = contract.inputs[i];
      if (a.path != b.path ||
          a.role != b.role ||
          a.bytes != b.bytes ||
          a.sha256 != b.sha256) {
        throw StateError('Imported artifact manifest identity/order changed');
      }
    }
    final paths = <String>{};
    for (final stage in reconstructionDownstreamStages) {
      final outputs = contract.outputs[stage];
      if (outputs == null ||
          outputs.isEmpty ||
          !outputs
              .map((a) => a.role)
              .toSet()
              .containsAll(reconstructionRequiredOutputRoles[stage]!)) {
        throw StateError('${stage.name} downstream outputs unregistered');
      }
      for (final output in outputs) {
        if (!paths.add(output.path))
          throw StateError('Downstream output path reused');
      }
    }
  }

  Future<ReconstructionDownstreamResult> run({
    required ReconstructionImportedUpstream upstream,
    required ReconstructionDownstreamContract boundContract,
    required List<String> cleanupRoots,
  }) async {
    if (_running)
      return ReconstructionDownstreamResult.blocked('Downstream run already active');
    _running = true;
    final ownedCleanupRoots = List<String>.unmodifiable(cleanupRoots);
    final contract = boundContract.value;
    ReconstructionUpstreamAcceptance? acceptance;
    var journalStarted = false;
    final receipts = <ReconstructionStageReceipt>[];
    final nativeAcceptances = <ReconstructionStage, ReconstructionNativeIoAcceptance>{};
    Future<ReconstructionDownstreamResult> stop(String reason, int code) async {
      var completeReason = reason;
      try {
        _assertIoBindings(boundContract);
      } catch (_) {
        acceptance = null;
      }
      if (journalStarted) {
        try {
          await journal.stopped(reason, code);
        } catch (e) {
          completeReason = '$reason; durable failure receipt write failed: $e';
        } finally {
          try {
            _assertIoBindings(boundContract);
          } catch (_) {
            acceptance = null;
          }
        }
      }
      return ReconstructionDownstreamResult._(
        exitCode: code,
        stoppedReason: completeReason,
        upstreamAcceptance: acceptance,
        receipts: receipts,
      );
    }

    try {
      if (_drivers.keys.any((s) => !reconstructionDownstreamStages.contains(s))) {
        throw StateError(
          'Upstream drivers are forbidden in imported downstream route',
        );
      }
      final missing = reconstructionDownstreamStages
          .where((s) => !_drivers.containsKey(s))
          .toList();
      if (missing.isNotEmpty)
        throw UnsupportedError(
          'Missing downstream native drivers: ${missing.map((s) => s.name).join(',')}',
        );
      boundContract.ioRoots.validate(ownedCleanupRoots);
      if (boundContract.ioRoots.durableInputRoot != upstream.durableInputRoot ||
          boundContract.ioRoots.textureSceneRoot != upstream.textureSceneRoot) {
        throw ReconstructionIoBindingError('Contract/input scene roots differ');
      }
      _assertIoBindings(boundContract);
      upstream.validate(ownedCleanupRoots);
      _validateContract(upstream, contract);
      for (final stage in reconstructionDownstreamStages) {
        final driver = _drivers[stage]!;
        if (driver.stage != stage ||
            driver.sourceManifestSha256 != contract.sourceInputManifestSha256 ||
            !_hash(driver.binarySha256) ||
            driver.verifiedCAbiSymbols.isEmpty) {
          throw StateError(
            '${stage.name} native C ABI/source/binary identity unresolved',
          );
        }
        final nativeIo = await capabilities.requireAccepted(
          driver,
          boundContract,
          artifacts as ReconstructionDownstreamArtifactGate,
          ownedCleanupRoots,
        );
        _assertIoBindings(boundContract);
        nativeIo.bind(driver, boundContract, ownedCleanupRoots);
        nativeAcceptances[stage] = nativeIo;
      }
      final accepted = await upstreamAcceptor.accept(
        upstream,
        boundContract,
        ownedCleanupRoots,
      );
      _assertIoBindings(boundContract);
      accepted.bind(upstream, boundContract, ownedCleanupRoots);
                                                                             
                                                           
      await artifacts.validateInputs(contract);
      _assertIoBindings(boundContract);
      acceptance = accepted;
      await artifacts.requireFreshOutputs(contract);
      _assertIoBindings(boundContract);
      await journal.started(upstream, boundContract, accepted);
      journalStarted = true;
      for (final stage in reconstructionDownstreamStages) {
        final watch = Stopwatch()..start();
        int? rc;
        try {
          await journal.stageStarted(stage);
          _assertIoBindings(boundContract);
          nativeAcceptances[stage]!.bind(
            _drivers[stage]!,
            boundContract,
            ownedCleanupRoots,
          );
          rc = await _drivers[stage]!.run(contract);
          nativeAcceptances[stage]!.bind(
            _drivers[stage]!,
            boundContract,
            ownedCleanupRoots,
          );
          _assertIoBindings(boundContract);
          if (rc != 0) {
            final row = ReconstructionStageReceipt(
              stage,
              rc,
              false,
              watch.elapsedMicroseconds,
              'Native stage returned nonzero',
            );
            receipts.add(row);
            await journal.stageFinished(row);
            return await stop('${stage.name} returned $rc', rc);
          }
          for (final output in contract.outputs[stage]!) {
            final checked = await (artifacts as ReconstructionDownstreamArtifactGate)
                .verifyBoundOutput(_drivers[stage]!, boundContract, output);
            _assertIoBindings(boundContract);
            checked.bind(_drivers[stage]!, boundContract, output);
          }
          final row = ReconstructionStageReceipt(
            stage,
            rc,
            true,
            watch.elapsedMicroseconds,
          );
          receipts.add(row);
          await journal.stageFinished(row);
        } catch (e) {
          if (e is ReconstructionIoBindingError) acceptance = null;
          if (receipts.isEmpty || receipts.last.stage != stage) {
            final row = ReconstructionStageReceipt(
              stage,
              rc,
              false,
              watch.elapsedMicroseconds,
              e.toString(),
            );
            receipts.add(row);
            try {
              await journal.stageFinished(row);
            } catch (_) {
                                            
            }
          }
          return await stop(e.toString(), 38);
        } finally {
          watch.stop();
        }
      }
      _assertIoBindings(boundContract);
      await journal.completed(List.unmodifiable(receipts));
      _assertIoBindings(boundContract);
      return ReconstructionDownstreamResult._(
        exitCode: 0,
        stoppedReason: null,
        upstreamAcceptance: acceptance,
        receipts: receipts,
      );
    } catch (e) {
      if (e is ReconstructionIoBindingError) acceptance = null;
      return await stop(e.toString(), 38);
    } finally {
      _running = false;
    }
  }
}

                                                                            
                                                                               
                                                                                
final class ReconstructionNativeDenseHandoff {
  ReconstructionNativeDenseHandoff({
    required this.captureDir,
    required this.densePlyPath,
    required this.denseWorkDir,
    required this.materializedImageCacheDir,
  }) {
    for (final path in [
      captureDir,
      densePlyPath,
      denseWorkDir,
      materializedImageCacheDir,
    ]) {
      _root(path);
    }
  }
  final String captureDir,
      densePlyPath,
      denseWorkDir,
      materializedImageCacheDir;
  List<String> get cleanupRoots => [denseWorkDir, materializedImageCacheDir];
}

typedef ReconstructionDownstreamHook = Future<ReconstructionDownstreamResult> Function(
  ReconstructionNativeDenseHandoff request,
);

abstract interface class ReconstructionExistingRetentionManifestProvider {
  Future<ReconstructionImportedUpstream?> findExisting(ReconstructionNativeDenseHandoff request);
}

typedef ReconstructionDownstreamContractFactory = Future<ReconstructionDownstreamContract> Function(
  ReconstructionImportedUpstream upstream,
);

final class ReconstructionProductionDownstreamLink {
  ReconstructionProductionDownstreamLink({
    required this.existingManifest,
    required this.downstreamContract,
    required this.controller,
  });
  final ReconstructionExistingRetentionManifestProvider existingManifest;
  final ReconstructionDownstreamContractFactory downstreamContract;
  final ReconstructionImportedDownstream controller;
  Future<ReconstructionDownstreamResult> call(ReconstructionNativeDenseHandoff request) async {
    try {
      final upstream = await existingManifest.findExisting(request);
      if (upstream == null)
        return ReconstructionDownstreamResult.blocked(
          'Actual complete native retention manifest unavailable; do not rerun or synthesize upstream',
        );
      if (upstream.sourceCaptureDir != request.captureDir ||
          upstream.sourceDensePlyPath != request.densePlyPath) {
        return ReconstructionDownstreamResult.blocked(
          'Retention manifest belongs to another production capture/dense PLY',
        );
      }
      final contract = await downstreamContract(upstream);
      return await controller.run(
        upstream: upstream,
        boundContract: contract,
        cleanupRoots: request.cleanupRoots,
      );
    } catch (e) {
      return ReconstructionDownstreamResult.blocked(e.toString());
    }
  }
}
