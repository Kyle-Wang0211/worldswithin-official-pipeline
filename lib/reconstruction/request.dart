import 'frame_transport.dart';
import 'dart:convert';
import 'dart:io';
import 'sha256_backend.dart';
import 'vendor/dart_pipeline/lib/reconstruction_pipeline.dart';
import 'vendor/production_downstream_bridge_v3/lib/reconstruction_downstream.dart';

Map<String, dynamic> object(Object? value) => (value as Map).cast<String, dynamic>();
String string(Map<String, dynamic> map, String key) => map[key] as String;
ReconstructionArtifact artifact(Object? value) {
  final a = object(value);
  return ReconstructionArtifact(path: string(a, 'path'), role: string(a, 'role'),
      bytes: a['bytes'] as int, sha256: string(a, 'sha256'));
}
                                                                               
                                                                            
                                                                              
final class ReconstructionIoTiming {
  static final _clock = Stopwatch()..start();
                                                                              
                                                                              
  static Object _epoch = Object();
  static Object get epoch => _epoch;
  static final _rows = <String, Map<String, int>>{};
  static void Function(Map<String, Object?>)? sink;

                                                                                
  static void diagnostic(String schema, String event,
      Map<String, Object?> details) {
    try {
      sink?.call({
        ...details,
        'schema': schema,
        'event': event,
        'time': DateTime.now().toUtc().toIso8601String(),
        'pid': pid,
      });
    } catch (_) {
                                                                              
    }
  }

  static void record(String operation, Stopwatch watch,
      {required Object epoch, int bytes = 0, bool failed = false}) {
    watch.stop();
    if (!identical(epoch, _epoch)) return;
    final row = _rows.putIfAbsent(operation, () => {
      'count': 0, 'bytes': 0, 'elapsed_us': 0, 'failed': 0,
    });
    row['count'] = row['count']! + 1;
    row['bytes'] = row['bytes']! + bytes;
    row['elapsed_us'] = row['elapsed_us']! + watch.elapsedMicroseconds;
    row['failed'] = row['failed']! + (failed ? 1 : 0);
  }

  static Object? _sessionOwner;

  static Object beginSession(Map<String, Object?> identity,
      void Function(Map<String, Object?>) emit) {
    final previous = _sessionOwner;
    if (previous != null) endSession(previous, 'superseded_by_next_run');
    reset();
    final owner = Object();
    _sessionOwner = owner;
    sink = (value) => emit({...value, ...identity});
    return owner;
  }

  static void endSession(Object? owner, String boundary) {
    if (owner == null || !identical(owner, _sessionOwner)) return;
    checkpoint(boundary);
    sink = null;
    _sessionOwner = null;
    reset();
  }

  static void reset() {
    _epoch = Object();
    _rows.clear();
    _clock.reset();
    _clock.start();
  }

  static Future<T> measure<T>(String label, Future<T> Function() operation) async {
    final timingEpoch = epoch;
    final watch = Stopwatch()..start();
    var failed = true;
    try {
      final result = await operation();
      failed = false;
      return result;
    } finally {
      record(label, watch, epoch: timingEpoch, failed: failed);
    }
  }

  static void checkpoint(String boundary, {Object? epoch}) {
    if (epoch != null && !identical(epoch, _epoch)) return;
    final emit = sink;
    if (emit == null) return;
    try {
      emit({
        'schema': 'RECONSTRUCTION_IO_TIMING_V1',
        'sha256_backend': reconstructionSha256Backend,
        'boundary': boundary,
        'monotonic_us': _clock.elapsedMicroseconds,
        'semantics': 'cumulative_inclusive_do_not_sum_nested_rows',
        'bytes_semantics': 'actual_hash_stream_bytes_other_rows_unmeasured',
        'operations': {
          for (final entry in _rows.entries)
            entry.key: Map<String, int>.from(entry.value),
        },
      });
    } catch (_) {
                                                                        
    }
  }
}

                                                                              
/// Closing it before begin prevents a late preparation from opening a session.
final class ReconstructionIoTimingLifetime {
  Object? _owner;
  bool _closed = false;

  Object? begin(Map<String, Object?> identity,
      void Function(Map<String, Object?>) emit) {
    if (_closed) return null;
    return _owner ??= ReconstructionIoTiming.beginSession(identity, emit);
  }

  void close(String boundary) {
    _closed = true;
    ReconstructionIoTiming.endSession(_owner, boundary);
    _owner = null;
  }
}

Future<String> hashFile(File file) async {
  final timingEpoch = ReconstructionIoTiming.epoch;
  final watch = Stopwatch()..start();
  var bytes = 0;
  var failed = true;
  try {
    final digest = await reconstructionSha256Stream(file.openRead().map((chunk) {
      bytes += chunk.length;
      return chunk;
    }));
    failed = false;
    return digest;
  } finally {
    ReconstructionIoTiming.record('hashFile_inclusive', watch,
        epoch: timingEpoch, bytes: bytes, failed: failed);
  }
}

                                                                                 
abstract interface class ReconstructionRequestTransport {
  Map<String, dynamic> get json;
  Map<String, dynamic> get paths;
  Map<String, dynamic> get native;
  ReconstructionIoRoots get ioRoots;
  String get runId;
  List<RequestFrame> get frames;
  List<String> outputPaths(ReconstructionStage stage, String role);
  String retainedOutputPath(int index, String key);
}

final class RequestFrame {
  const RequestFrame(this.frameIdF64Bits, this.width, this.height, this.originalCameraPath);
  final String frameIdF64Bits, originalCameraPath;
  final int width, height;
}

                                                                               
final class MainlineRequest implements ReconstructionRequestTransport {
  MainlineRequest(this.json, this.contract, this.upstream, this.ioRoots);
  final Map<String, dynamic> json;
  final ReconstructionRunContract contract;
  final ReconstructionImportedUpstream upstream;
  final ReconstructionIoRoots ioRoots;
  String get runId => contract.runId;
  String get inputIdentityStatus =>
      (json['input_identity_status'] ?? json['original1920_identity_status']) as String;
  bool get hasOriginal1920Identity => inputIdentityStatus == 'ACCEPTED_ORIGINAL1920';
  String get sceneManifestSha256 => string(json, hasOriginal1920Identity
      ? 'original1920_scene_manifest_sha256' : 'regenerated1920_scene_manifest_sha256');

  static void validateInputIdentity(Map<String, dynamic> j) {
    final status = j['input_identity_status'] ?? j['original1920_identity_status'];
    final sha = RegExp(r'^[0-9a-f]{64}$');
    if (status == 'ACCEPTED_ORIGINAL1920') {
      if (j['original1920_identity_status'] != 'ACCEPTED_ORIGINAL1920' ||
          !sha.hasMatch(string(j, 'original1920_scene_manifest_sha256'))) {
        throw StateError('Explicit original1920 identity required');
      }
      return;
    }
    if (status != 'ACCEPTED_REGENERATED_INPUTS' ||
        j['original1920_identity_status'] == 'ACCEPTED_ORIGINAL1920' ||
        !sha.hasMatch(string(j, 'regenerated1920_scene_manifest_sha256'))) {
      throw StateError('Accepted original or explicitly regenerated actual-count input identity required');
    }
    final origin = object(j['input_origin']);
    if (origin['kind'] != 'regenerated_from_frozen_local_actual_frames' ||
        !sha.hasMatch(string(origin, 'source_manifest_sha256')) ||
        !sha.hasMatch(string(origin, 'producer_command_sha256')) ||
        !sha.hasMatch(string(origin, 'camera_producer_command_sha256'))) {
      throw StateError('Regenerated actual-count source and producer identities required');
    }
  }
  List<String> get cleanupRoots => (json['cleanup_roots'] as List).cast<String>();
  Map<String, dynamic> get paths => object(json['stage_paths']);
  Map<String, dynamic> get native => object(json['native_libraries']);

  @override List<RequestFrame> get frames => upstream.orderedFrames.map((f) =>
      RequestFrame(f.frameIdF64Bits, f.width, f.height, f.original4032Camera.path)).toList();
  @override List<String> outputPaths(ReconstructionStage stage, String role) =>
      contract.outputs[stage]!.where((a) => a.role == role).map((a) => a.path).toList();
  @override String retainedOutputPath(int index, String key) {
    final rows = (object(paths['fusionPly'])['retained_frames'] as List);
    if (rows.length != frameTransportCount(json)) throw StateError('New dense retention must register actual-count output tuples');
    final emitted = artifact(object(rows[index])[key]);
    final original = key == 'd16' ? upstream.orderedFrames[index].d16 : upstream.orderedFrames[index].cameraF64;
    if (emitted.sha256 != original.sha256 || emitted.bytes != original.bytes || emitted.role != original.role ||
        !contract.outputs[ReconstructionStage.fusionPly]!.any((a) => a.path == emitted.path && a.sha256 == emitted.sha256 && a.role == emitted.role)) {
      throw StateError('New dense retention does not bind original actual-count exact output identity');
    }
    return emitted.path;
  }

  static Future<MainlineRequest> load(File file, String expectedSha256) async {
    if (await hashFile(file) != expectedSha256) {
      throw StateError('Root-frozen real-input request hash changed');
    }
    final j = object(jsonDecode(await file.readAsString()));
    final count = frameTransportCount(j);
    if (j['schema'] != 'RECONSTRUCTION_REAL_INPUT_MAINLINE_V1' ||
        j['route_id'] != reconstructionImportedRouteId || j['ordered_frames'] != count ||
        j['algorithm_revision'] != 'c0f26ee018a2b3f9d0fa773a49711cf203da9277') {
      throw StateError('Exact selected actual-count algorithm, route and frame count required');
    }
    validateInputIdentity(j);
    final gates = object(j['source_gates']);
    final sourceGates = ReconstructionSourceGates(sourceAccepted:gates['source_accepted'] == true,
        originalE0NoiseBytesAccepted:gates['original_seeded_noise_bytes_accepted'] == true,
        platformBytesAccepted:gates['platform_bytes_accepted'] == true,
        vertexColorAccessorBytesAccepted:gates['vertex_color_accessor_bytes_accepted'] == true);
    final io = object(j['io_roots']);
    final roots = ReconstructionIoRoots(durableInputRoot:string(io,'durable_input_root'),
        downstreamOutputRoot:string(io,'downstream_output_root'),
        textureSceneRoot:string(io,'texture_scene_root'));
    final upstreamJson = object(j['retained_upstream']);
    final frames = (upstreamJson['ordered_frames'] as List).map((value) {
      final f = object(value);
      return ReconstructionRetainedFrame(originalIndex:f['original_index'] as int,
          packIndex:f['pack_index'] as int, frameIdF64Bits:string(f,'frame_id_f64_bits'),
          d16:artifact(f['d16']), cameraF64:artifact(f['camera_f64']),
          metadata:artifact(f['metadata']), textureImage:artifact(f['texture_image']),
          texture1920Camera:artifact(f['texture_1920_camera']), normalImage:artifact(f['normal_image']),
          original4032Camera:artifact(f['original_4032_camera']));
    }).toList();
    final upstreamReceipts = (upstreamJson['accepted_upstream_receipts'] as List).map((v) {
      final r = object(v);
      return ReconstructionStageReceipt(ReconstructionStage.values.byName(string(r,'stage')),
          r['entry_return_code'] as int?, r['output_gate_passed'] as bool,
          r['elapsed_microseconds'] as int, r['error'] as String?);
    }).toList();
    final upstream = ReconstructionImportedUpstream(routeId:reconstructionImportedRouteId,
        manifest:artifact(upstreamJson['manifest']), order:artifact(upstreamJson['order']),
        densePly:artifact(upstreamJson['dense_ply']), durableInputRoot:roots.durableInputRoot,
        textureSceneRoot:roots.textureSceneRoot, sourceCaptureDir:string(upstreamJson,'source_capture_dir'),
        sourceDensePlyPath:string(upstreamJson,'source_dense_ply_path'),
        producerSourceManifestSha256:string(upstreamJson,'producer_source_manifest_sha256'),
        producerBinarySha256:string(upstreamJson,'producer_binary_sha256'),
        orderedInputManifestSha256:string(j,'ordered_input_manifest_sha256'),
        modelManifestSha256:string(j,'model_manifest_sha256'),
        environmentManifestSha256:string(j,'environment_manifest_sha256'),
        denseProducerProfile:string(j, 'dense_producer_profile'), sourceGates:sourceGates, orderedFrames:frames, acceptedUpstreamReceipts:upstreamReceipts);
    final outputs = <ReconstructionStage,List<ReconstructionArtifact>>{};
    for (final entry in object(j['outputs']).entries) {
      outputs[ReconstructionStage.values.byName(entry.key)] = (entry.value as List).map(artifact).toList();
    }
    final contract = ReconstructionRunContract(runId:string(j,'run_id'),
        sourceInputManifestSha256:reconstructionSourceInputManifestSha256,
        orderedInputManifestSha256:upstream.orderedInputManifestSha256,
        modelManifestSha256:upstream.modelManifestSha256,
        environmentManifestSha256:upstream.environmentManifestSha256,
        orderedFrames:count, denseProducerProfile:string(j, 'dense_producer_profile'), sourceGates:sourceGates, inputs:upstream.artifacts, outputs:outputs);
    final request = MainlineRequest(j,contract,upstream,roots);
    roots.validate(request.cleanupRoots);
    upstream.validate(request.cleanupRoots);
    return request;
  }


  Future<void> verifyFormatEvidence(File file,ReconstructionArtifact a) async {
    final row=object(object(json['artifact_format_evidence'])[a.sha256]);
    final receipt=File(string(row,'path'));
    if(await hashFile(receipt)!=string(row,'sha256')) {
      throw StateError('Native baseline format receipt changed: ${a.path}');
    }
    final evidence=object(jsonDecode(await receipt.readAsString()));
    if(evidence['artifact_sha256']!=a.sha256 || evidence['artifact_bytes']!=a.bytes ||
        evidence['role']!=a.role || evidence['native_source_bound_format_accepted']!=true) {
      throw StateError('Source-bound baseline format evidence missing: ${a.path}');
    }
  }

  Future<void> verifyRetainedInputs() async {
    final gate = ReconstructionFileArtifactGate(inputRoot:Directory(ioRoots.durableInputRoot),
        runRoot:Directory(ioRoots.downstreamOutputRoot),sha256File:hashFile,
        validateFormat:verifyFormatEvidence);
    await gate.validateInputs(contract);
    final orderedScene = <String>{for(final f in upstream.orderedFrames) ...[
      '${ioRoots.durableInputRoot}/${f.textureImage.path}',
      '${ioRoots.durableInputRoot}/${f.texture1920Camera!.path}']};
    final actual = <String>{};
    await for (final entity in Directory(ioRoots.textureSceneRoot).list(recursive:true,followLinks:false)) {
      final kind = await FileSystemEntity.type(entity.path,followLinks:false);
      if (kind == FileSystemEntityType.file) { actual.add(entity.path); }
      else if (kind != FileSystemEntityType.directory) { throw StateError('Nonregular scene input'); }
    }
    if (orderedScene.length != frames.length * 2 || actual.length != frames.length * 2 || !actual.containsAll(orderedScene)) {
      throw StateError('Actual accepted1920 complete actual-count image/camera scene mismatch');
    }
                                                                                            
    final receipt = object(json['native_input_format_receipt']);
    final path = File(string(receipt,'path'));
    if (await hashFile(path) != string(receipt,'sha256')) {
      throw StateError('Real actual-count native camera/D16/order/1920 format receipt changed');
    }
    final evidence = object(jsonDecode(await path.readAsString()));
    final sceneHashKey = hasOriginal1920Identity
        ? 'original1920_scene_manifest_sha256' : 'regenerated1920_scene_manifest_sha256';
    if (!hasOriginal1920Identity) {
      final acceptedOrigin = object(evidence['input_origin']);
      final origin = object(json['input_origin']);
      if (evidence['input_identity_status'] != 'ACCEPTED_REGENERATED_INPUTS' ||
          evidence['original1920_identity_status'] == 'ACCEPTED_ORIGINAL1920' ||
          ['kind', 'source_manifest_sha256', 'producer_command_sha256',
           'camera_producer_command_sha256'].any((key) => acceptedOrigin[key] != origin[key])) {
        throw StateError('Regenerated actual-count native format receipt origin mismatch');
      }
    }
    if (evidence['manifest_sha256'] != upstream.manifest.sha256 ||
        evidence[sceneHashKey] != sceneManifestSha256 ||
        evidence['actual_native_formats_accepted'] != true ||
        evidence['ordered_frames'] != frameTransportCount(json)) {
      throw StateError('Real actual-count native input format acceptance unavailable');
    }
  }
}
