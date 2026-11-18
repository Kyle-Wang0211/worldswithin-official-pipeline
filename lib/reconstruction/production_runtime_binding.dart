import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart' hide Arena;

import 'native_downstream.dart';
import 'execution_identity_scope.dart';
import 'memory_policy.dart';
import 'vendor/dart_pipeline/lib/reconstruction_geometry_ffi.dart' show ReconstructionSystemMemory;

const reconstructionRuntimeBootstrapSourceSha256 =
    'ac871da59245e4b8bfa4c2bcc8d0a9181c6eb70b82a42d6318dc4498a296954e';
const reconstructionRuntimeBootstrapSymbol = 'reconstruction_normal_preload_bootstrap';
typedef _BootstrapNative = Int32 Function(Pointer<Char>);
typedef _BootstrapDart = int Function(Pointer<Char>);

                                                                                
                                                                                 
const reconstructionProductionRuntimeEnvironment = <String, String>{
  'PWDENSE_ORT_MAX_PENDING': '1',
  'PWDENSE_ORT_STORAGE_CACHE': 'disabled',
  'PWDENSE_PIPE_NO_PREFETCH': '1',
  'RECONSTRUCTION_BENCH_HOST_MATERIAL_MMAP': '1',
  'RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD': '1',
  'RECONSTRUCTION_SUBMIT_DRAIN': '1',
  'RECONSTRUCTION_MATCHED_MVS_ROUTE': 'whole',
  'RECONSTRUCTION_MATCHED_FIFO_MODE': '4',
  'RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS': '4',
  'RECONSTRUCTION_BENCH_SEGMENT_SESSION_CACHE': '0',
  'RECONSTRUCTION_PRIVATE_REST_GRAPH_CAPTURE': '1',
  'RECONSTRUCTION_S1_STORAGE_BUFFER_CACHE_MODE': 'disabled',
  'RECONSTRUCTION_BENCH_UMA_INPUT_REWRITE_MAP': '1',
  'RECONSTRUCTION_VALIDATION_TARGETED_TARGETED_REUSE': '1',
  'RECONSTRUCTION_ETA_DIAGNOSTIC_ONLY': '0',
  'RECONSTRUCTION_OFFICIAL_OBJECT_POOL': '1',
  'RECONSTRUCTION_BENCH_OVERLAP_CER_N1': '0',
  'PW_GPU_FEATURE_TRANSFER': '1',
  'PW_DIRECT_FEATURE_CAPACITY': '10',
  'RECONSTRUCTION_PHASE_BOUNDARY_DRAIN': '1',
  'RECONSTRUCTION_CAPTURE_SHARED_WORKSPACE': '1',
  'PW_FNET_MIXED_CORRIDOR': '1',
  'OFFICIAL_AETHER_PYR_PERSIST': '0',
};
typedef _SetEnvN = Int32 Function(Pointer<Utf8>, Pointer<Utf8>, Int32);
typedef _SetEnvD = int Function(Pointer<Utf8>, Pointer<Utf8>, int);
typedef _GetEnvN = Pointer<Utf8> Function(Pointer<Utf8>);
typedef _GetEnvD = Pointer<Utf8> Function(Pointer<Utf8>);
final _setenv = DynamicLibrary.process().lookupFunction<_SetEnvN, _SetEnvD>(
  'setenv',
);
final _getenv = DynamicLibrary.process().lookupFunction<_GetEnvN, _GetEnvD>(
  'getenv',
);

String? productionRuntimeEnvironmentValue(String key) {
  final name = key.toNativeUtf8();
  try {
    final value = _getenv(name);
    return value == nullptr ? null : value.toDartString();
  } finally {
    malloc.free(name);
  }
}

void _requireRuntimeWorker() {
  if (Isolate.current.debugName == 'main') {
    throw StateError('Native runtime initialization requires the worker');
  }
}

                                                                              
                                                                               
Map<String, String> verifyProductionRuntimeEnvironment({
  ReconstructionExecutionIdentityScope identityScope =
      ReconstructionExecutionIdentityScope.production,
  String? bundleRoot,
}) {
  _requireRuntimeWorker();
  final actual = <String, String>{};
  for (final entry in <String, String>{
    ...reconstructionProductionRuntimeEnvironment,
    if (bundleRoot != null) 'ALICEVISION_ROOT': bundleRoot,
    if (bundleRoot != null)
      'ALICEVISION_OCIO': '$bundleRoot/share/aliceVision/config.ocio',
    'RECONSTRUCTION_MEMORY_POLICY': wholeMemoryPolicy(identityScope),
  }.entries) {
    final name = entry.key.toNativeUtf8();
    try {
      final pointer = _getenv(name);
      final value = pointer == nullptr ? null : pointer.toDartString();
      if (value != entry.value) {
        throw StateError(
          'Native runtime environment readback differs: ${entry.key}',
        );
      }
      actual[entry.key] = value!;
    } finally {
      malloc.free(name);
    }
  }
  return Map<String, String>.unmodifiable(actual);
}

                                                                           
                                                                                
Map<String, String> applyProductionRuntimeEnvironment({
  ReconstructionExecutionIdentityScope identityScope =
      ReconstructionExecutionIdentityScope.production,
  String? bundleRoot,
}) {
  _requireRuntimeWorker();
  for (final entry in <String, String>{
    ...reconstructionProductionRuntimeEnvironment,
    if (bundleRoot != null) 'ALICEVISION_ROOT': bundleRoot,
    if (bundleRoot != null)
      'ALICEVISION_OCIO': '$bundleRoot/share/aliceVision/config.ocio',
    'RECONSTRUCTION_MEMORY_POLICY': wholeMemoryPolicy(identityScope),
  }.entries) {
    final name = entry.key.toNativeUtf8(), value = entry.value.toNativeUtf8();
    try {
      if (_setenv(name, value, 1) != 0) {
        throw StateError(
          'Native runtime environment setenv failed: ${entry.key}',
        );
      }
    } finally {
      malloc.free(name);
      malloc.free(value);
    }
  }
  return verifyProductionRuntimeEnvironment(
    identityScope: identityScope,
    bundleRoot: bundleRoot,
  );
}

                                                                              
                                                                               
Future<Map<String, Object?>> initializeProductionNativeRuntime(
  Map<String, dynamic> row,
  String bundleRoot, {
  ReconstructionExecutionIdentityScope identityScope =
      ReconstructionExecutionIdentityScope.production,
}) async {
  _requireRuntimeWorker();
  final environment = applyProductionRuntimeEnvironment(
    bundleRoot: bundleRoot,
    identityScope: identityScope,
  );
  final slot = await NativeSlot.open(row, bundleRoot, {
    reconstructionRuntimeBootstrapSymbol,
  });
  if (slot.sourceEvidence['source_sha256'] !=
      reconstructionRuntimeBootstrapSourceSha256) {
    throw StateError('Original normal runtime loader source identity changed');
  }
  final arena = Arena(ReconstructionSystemMemory(DynamicLibrary.process()));
  try {
    final rc = slot.library.lookupFunction<_BootstrapNative, _BootstrapDart>(
      reconstructionRuntimeBootstrapSymbol,
    )(arena.path(bundleRoot));
                                                                              
                                                                           
    if (rc != 0 && rc != 3) {
      throw StateError('Original normal runtime preload returned $rc');
    }
    return {
      'classification': 'NATIVE_LOADER_INITIALIZATION_ONLY',
      'native_entry': reconstructionRuntimeBootstrapSymbol,
      'return_code': rc,
      'source_sha256': reconstructionRuntimeBootstrapSourceSha256,
      'binary_sha256': slot.binarySha256,
      'process_environment': environment,
      'process_environment_context': 'REFERENCE_RUNTIME_CONFIGURED',
      'full_pipeline_complete': false,
    };
  } finally {
    arena.dispose();
  }
}
