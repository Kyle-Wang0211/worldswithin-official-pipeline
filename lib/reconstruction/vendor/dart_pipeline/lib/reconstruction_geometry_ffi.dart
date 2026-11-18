import 'dart:convert';
import 'dart:ffi';

import 'reconstruction_pipeline.dart';

                                                                      
typedef ReconstructionMeshoptNative = Int32 Function(
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Char>,
);
typedef ReconstructionMeshoptDart = int Function(
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Char>,
);
typedef ReconstructionLagrangeNative = Int32 Function(
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Uint64>,
  Pointer<Uint64>,
  Pointer<Uint64>,
  Pointer<Uint64>,
);
typedef ReconstructionLagrangeDart = int Function(
  Pointer<Char>,
  Pointer<Char>,
  Pointer<Uint64>,
  Pointer<Uint64>,
  Pointer<Uint64>,
  Pointer<Uint64>,
);
typedef ReconstructionGeometryErrorNative = Pointer<Char> Function();
typedef ReconstructionGeometryErrorDart = Pointer<Char> Function();

const reconstructionGeometryHeaderSha256 =
    '05b7bf2af15c5dfc785fd0752e0ca525aaddd7937772c689379351eef9c9dbaa';
const reconstructionGeometryLibrarySha256 =
    '70c780fa8f0810cd9bb3f6c2d03adbb6166e999c095fb4981bfe9f74ddd42e0b';
const reconstructionGeometrySymbols = <String>{
  'reconstruction_meshopt_simplify_files',
  'reconstruction_lagrange_fill_ply',
  'reconstruction_geometry_last_error',
};

                                                                             
                                                                                 
final class ReconstructionGeometryEvidence {
  const ReconstructionGeometryEvidence({
    required this.headerSha256,
    required this.binarySha256,
    required this.sourceInputManifestSha256,
    required this.actualDefinedSymbols,
  });
  final String headerSha256;
  final String binarySha256;
  final String sourceInputManifestSha256;
  final Set<String> actualDefinedSymbols;
  void validate() {
    if (headerSha256 != reconstructionGeometryHeaderSha256 ||
        binarySha256 != reconstructionGeometryLibrarySha256 ||
        sourceInputManifestSha256 != reconstructionSourceInputManifestSha256 ||
        !actualDefinedSymbols.containsAll(reconstructionGeometrySymbols)) {
      throw StateError('Frozen geometry C ABI/source/binary identity rejected');
    }
  }
}

                                                                           
                                                                             
final class ReconstructionGeometryBindings {
  ReconstructionGeometryBindings._(this.meshopt, this.lagrange, this.lastError);
  factory ReconstructionGeometryBindings.bind(
    DynamicLibrary library,
    ReconstructionGeometryEvidence evidence,
  ) {
    evidence.validate();
    return ReconstructionGeometryBindings._(
      library.lookupFunction<ReconstructionMeshoptNative, ReconstructionMeshoptDart>(
        'reconstruction_meshopt_simplify_files',
      ),
      library.lookupFunction<ReconstructionLagrangeNative, ReconstructionLagrangeDart>(
        'reconstruction_lagrange_fill_ply',
      ),
      library.lookupFunction<ReconstructionGeometryErrorNative, ReconstructionGeometryErrorDart>(
        'reconstruction_geometry_last_error',
      ),
    );
  }

                                                                             
                                                                   
  factory ReconstructionGeometryBindings.testCallbacks({
    required Pointer<NativeFunction<ReconstructionMeshoptNative>> meshopt,
    required Pointer<NativeFunction<ReconstructionLagrangeNative>> lagrange,
    required Pointer<NativeFunction<ReconstructionGeometryErrorNative>> lastError,
  }) => ReconstructionGeometryBindings._(
    meshopt.asFunction<ReconstructionMeshoptDart>(),
    lagrange.asFunction<ReconstructionLagrangeDart>(),
    lastError.asFunction<ReconstructionGeometryErrorDart>(),
  );
  final ReconstructionMeshoptDart meshopt;
  final ReconstructionLagrangeDart lagrange;
  final ReconstructionGeometryErrorDart lastError;
}

abstract interface class ReconstructionNativeMemory {
  Pointer<Uint8> allocate(int bytes);
  void release(Pointer<Uint8> pointer);
}

typedef _MallocNative = Pointer<Void> Function(UintPtr);
typedef _MallocDart = Pointer<Void> Function(int);
typedef _FreeNative = Void Function(Pointer<Void>);
typedef _FreeDart = void Function(Pointer<Void>);

                                                                          
                                                                           
final class ReconstructionSystemMemory implements ReconstructionNativeMemory {
  ReconstructionSystemMemory(DynamicLibrary runtime)
    : _malloc = runtime.lookupFunction<_MallocNative, _MallocDart>('malloc'),
      _free = runtime.lookupFunction<_FreeNative, _FreeDart>('free');
  final _MallocDart _malloc;
  final _FreeDart _free;
  @override
  Pointer<Uint8> allocate(int bytes) {
    if (bytes <= 0) throw ArgumentError.value(bytes, 'bytes');
    final pointer = _malloc(bytes).cast<Uint8>();
    if (pointer == nullptr) throw StateError('Native allocation failed');
    return pointer;
  }

  @override
  void release(Pointer<Uint8> pointer) => _free(pointer.cast<Void>());
}

final class _CallArena {
  _CallArena(this.memory);
  final ReconstructionNativeMemory memory;
  final _owned = <Pointer<Uint8>>[];
  Pointer<Uint8> bytes(int length) {
    final p = memory.allocate(length);
    _owned.add(p);
    p.asTypedList(length).fillRange(0, length, 0);
    return p;
  }

  Pointer<Char> path(String value) {
    if (!value.startsWith('/') ||
        value.length < 2 ||
        value.contains('\x00') ||
        value.contains('\\') ||
        value
            .split('/')
            .skip(1)
            .any((p) => p.isEmpty || p == '.' || p == '..')) {
      throw ArgumentError(
        'Native IO paths must be canonical absolute nonempty paths',
      );
    }
    final encoded = utf8.encode(value);
    final p = bytes(encoded.length + 1);
    p.asTypedList(encoded.length).setAll(0, encoded);
    return p.cast<Char>();
  }

  void dispose() {
    for (final p in _owned.reversed) {
      memory.release(p);
    }
    _owned.clear();
  }
}

String reconstructionBorrowedUtf8(Pointer<Char> pointer, {int maxBytes = 4096}) {
  if (pointer == nullptr) return '';
  if (maxBytes <= 0) throw ArgumentError.value(maxBytes, 'maxBytes');
  final bytes = <int>[];
  final view = pointer.cast<Uint8>();
  for (var i = 0; i < maxBytes; i++) {
    final value = (view + i).value;
    if (value == 0) return utf8.decode(bytes);
    bytes.add(value);
  }
  throw StateError('Native error exceeds the registered diagnostic bound');
}

final class ReconstructionGeometryCallResult {
  const ReconstructionGeometryCallResult(
    this.entryReturnCode,
    this.error, {
    this.inputVertices,
    this.inputFacets,
    this.outputVertices,
    this.outputFacets,
  });
  final int entryReturnCode;
  final String error;
  final int? inputVertices;
  final int? inputFacets;
  final int? outputVertices;
  final int? outputFacets;
}

                                                                                
                                                                               
final class ReconstructionGeometryFfi {
  ReconstructionGeometryFfi(this.bindings, this.memory);
  final ReconstructionGeometryBindings bindings;
  final ReconstructionNativeMemory memory;

  ReconstructionGeometryCallResult simplifyFiles({
    required String inputPositionsF32,
    required String inputIndicesU32,
    required String outputPositionsF32,
    required String outputIndicesU32,
  }) {
    final arena = _CallArena(memory);
    try {
      final rc = bindings.meshopt(
        arena.path(inputPositionsF32),
        arena.path(inputIndicesU32),
        arena.path(outputPositionsF32),
        arena.path(outputIndicesU32),
      );
      return ReconstructionGeometryCallResult(rc, reconstructionBorrowedUtf8(bindings.lastError()));
    } finally {
      arena.dispose();
    }
  }

  ReconstructionGeometryCallResult fillPly({
    required String inputPly,
    required String outputPly,
  }) {
    final arena = _CallArena(memory);
    try {
      final input = arena.path(inputPly), output = arena.path(outputPly);
      final counts = arena.bytes(4 * sizeOf<Uint64>()).cast<Uint64>();
      final rc = bindings.lagrange(
        input,
        output,
        counts,
        counts + 1,
        counts + 2,
        counts + 3,
      );
      final error = reconstructionBorrowedUtf8(bindings.lastError());
                                                                            
                                                                  
      return ReconstructionGeometryCallResult(
        rc,
        error,
        inputVertices: rc == 0 ? counts.value : null,
        inputFacets: rc == 0 ? (counts + 1).value : null,
        outputVertices: rc == 0 ? (counts + 2).value : null,
        outputFacets: rc == 0 ? (counts + 3).value : null,
      );
    } finally {
      arena.dispose();
    }
  }
}
