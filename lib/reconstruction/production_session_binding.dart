import 'frame_transport.dart';
import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'dense_binding_contract.dart' show DenseFrame, DenseOptions, VersionN, VersionD, InitN, InitD, pwOriginalRetainedDenseHeaderSha256;
import 'native_downstream.dart' show Arena, NativeSlot;
import 'vendor/dart_pipeline/lib/reconstruction_geometry_ffi.dart' show ReconstructionNativeMemory, ReconstructionSystemMemory;
import 'vendor/dart_texture_integration_v1/lib/reconstruction_texture_ffi.dart' show reconstructionTextureAbsolutePath;

                                                                                            
const pwFullPipelineSessionHeaderSha256 =
    'ad7e203ec0629e1cbccee623e0eb5bf8dd5a2964ee9917d6fbbd7174b5283ea8';
const pwFullPipelineSessionSymbols = <String>{
  'pw_full_pipeline_session_abi_version',
  'pw_full_pipeline_session_build',
  'pw_full_pipeline_dense_options_init',
};
typedef SessionBuildN = Int32 Function(Pointer<DenseFrame>, Int32,
    Pointer<Float>, Int32, Uint64, Pointer<DenseOptions>, Pointer<Float>, Uint64,
    Pointer<Int32>, Uint64, Pointer<Char>, Uint64);
typedef SessionBuildD = int Function(Pointer<DenseFrame>, int,
    Pointer<Float>, int, int, Pointer<DenseOptions>, Pointer<Float>, int,
    Pointer<Int32>, int, Pointer<Char>, int);

                                                                             
                                                                                 
final class ProductionSessionFrame {
  const ProductionSessionFrame({required this.frameId, required this.frameIdF64Bits,
    required this.fx, required this.fy, required this.cx, required this.cy,
    required this.imageW, required this.imageH, required this.qWxyz,
    required this.translation, required this.jpegPath});
  final double frameId, fx, fy, cx, cy, imageW, imageH;
  final String frameIdF64Bits, jpegPath;
  final List<double> qWxyz, translation;
}

final class ProductionSessionTableBytes {
  ProductionSessionTableBytes(this.camerasF32, this.neighborsI32, this.receipt);
                                                                              
  final Uint8List camerasF32, neighborsI32;
  final Map<String, Object?> receipt;
}

                                                                                 
                                                                    
final class ProductionSessionBinding {
  ProductionSessionBinding._(this.slot, this.memory, this.headerSha256) {
    if (!slot.symbols.containsAll(pwFullPipelineSessionSymbols)) {
      throw StateError('Actual additive session definitions are required');
    }
    if (slot.library.lookupFunction<VersionN, VersionD>(
        'pw_full_pipeline_session_abi_version')() != 1) {
      throw StateError('Additive session ABI version mismatch');
    }
  }
  final String headerSha256;
  final NativeSlot slot;
  final ReconstructionNativeMemory memory;

  static Future<ProductionSessionBinding> open(Map<String, dynamic> nativeRow,
      String bundleRoot, {ReconstructionNativeMemory? memory}) async {
    final header = nativeRow['header_sha256'];
    if (header != pwFullPipelineSessionHeaderSha256 && header != pwOriginalRetainedDenseHeaderSha256) {
      throw StateError('Exact additive session header identity mismatch');
    }
    final slot = await NativeSlot.open(nativeRow, bundleRoot, pwFullPipelineSessionSymbols);
    return ProductionSessionBinding._(slot,
        memory ?? ReconstructionSystemMemory(DynamicLibrary.process()), header as String);
  }

  ProductionSessionTableBytes build({required List<ProductionSessionFrame> frames,
      required Uint8List sparseXyzF32}) {
    if (Endian.host != Endian.little) {
      throw UnsupportedError('Production raw session transport requires little endian');
    }
    final count = checkedFrameCount(frames.length);
    if ( sparseXyzF32.isEmpty || sparseXyzF32.length % 12 != 0 ||
        sparseXyzF32.length ~/ 12 > 0x7fffffff) {
      throw StateError('Exact actual-count frames and real finite sparse f32 triples required');
    }
    final sparseView = ByteData.sublistView(sparseXyzF32);
    for (var i = 0; i < sparseXyzF32.length; i += 4) {
      if (!sparseView.getFloat32(i, Endian.little).isFinite) {
        throw StateError('Nonfinite sparse xyz');
      }
    }
    final arena = Arena(memory);
    try {
      final nativeFrames = arena.bytes(count * sizeOf<DenseFrame>()).cast<DenseFrame>();
      for (var i = 0; i < frames.length; ++i) {
        final row = frames[i];
        final values = [row.frameId, row.fx, row.fy, row.cx, row.cy,
          row.imageW, row.imageH, ...row.qWxyz, ...row.translation];
        if (values.any((v) => !v.isFinite) || row.qWxyz.length != 4 ||
            row.translation.length != 3 || row.qWxyz.every((v) => v == 0) ||
            row.fx <= 0 || row.fy <= 0 || row.imageW <= 0 || row.imageH <= 0 ||
            (i > 0 && row.frameId <= frames[i - 1].frameId)) {
          throw StateError('Invalid real refined pose/intrinsics/frame order');
        }
        final bits = ByteData(8)..setFloat64(0, row.frameId, Endian.big);
        final identity = bits.getUint64(0, Endian.big).toRadixString(16).padLeft(16, '0');
        if (identity != row.frameIdF64Bits) {
          throw StateError('Production original frame binary64 identity mismatch');
        }
        reconstructionTextureAbsolutePath(row.jpegPath);
        if (row.jpegPath.contains('\u0000')) throw StateError('NUL in real JPEG path');
        final frame = (nativeFrames + i).ref;
        frame.frameId = row.frameId; frame.fx = row.fx; frame.fy = row.fy;
        frame.cx = row.cx; frame.cy = row.cy; frame.imageW = row.imageW; frame.imageH = row.imageH;
        for (var n = 0; n < 4; n++) { frame.q[n] = row.qWxyz[n]; }
        for (var n = 0; n < 3; n++) { frame.t[n] = row.translation[n]; }
        frame.jpeg = arena.path(row.jpegPath);
      }
      final options = arena.bytes(sizeOf<DenseOptions>()).cast<DenseOptions>();
      final init = slot.library.lookupFunction<InitN, InitD>(
          'pw_full_pipeline_dense_options_init')(options);
      if (init != 0) throw StateError('Selected native options init failed: $init');
      final sparse = arena.bytes(sparseXyzF32.length);
      sparse.asTypedList(sparseXyzF32.length).setAll(0, sparseXyzF32);
      final cameras = arena.bytes(count * 36 * 4);
      final neighbors = arena.bytes(count * 9 * 4);
      final error = arena.bytes(512);
      final rc = slot.library.lookupFunction<SessionBuildN, SessionBuildD>(
          'pw_full_pipeline_session_build')(nativeFrames, count, sparse.cast<Float>(),
          sparseXyzF32.length ~/ 12, sparseXyzF32.length ~/ 4, options,
          cameras.cast<Float>(), count * 36, neighbors.cast<Int32>(), count * 9,
          error.cast<Char>(), 512);
      final errorBytes = error.asTypedList(512);
      final end = errorBytes.indexOf(0);
      if (end < 0) throw StateError('Native session error unterminated');
      if (rc != 0) {
        throw StateError('Session table stopped: rc=$rc ${utf8.decode(errorBytes.sublist(0, end))}');
      }
      final cameraBytes = Uint8List.fromList(cameras.asTypedList(count * 36 * 4));
      final neighborBytes = Uint8List.fromList(neighbors.asTypedList(count * 9 * 4));
      final cameraView = ByteData.sublistView(cameraBytes);
      final neighborView = ByteData.sublistView(neighborBytes);
      for (var n = 0; n < count * 36; n++) {
        if (!cameraView.getFloat32(n * 4, Endian.little).isFinite) {
          throw StateError('Native session camera output nonfinite');
        }
      }
      for (var n = 0; n < count * 9; n++) {
        final value = neighborView.getInt32(n * 4, Endian.little);
        if (value < 0 || value >= count) throw StateError('Native session neighbor out of range');
      }
      return ProductionSessionTableBytes(cameraBytes, neighborBytes, {
        'native_entry': 'pw_full_pipeline_session_build', 'return_code': rc,
        'session_abi_version': 1, 'dense_run_abi_version': headerSha256 == pwOriginalRetainedDenseHeaderSha256 ? 2 : 1,
        'header_sha256': headerSha256,
        'binary_sha256': slot.binarySha256,
        'actual_defined_symbols': slot.symbols.toList()..sort(),
        'frames': count, 'sparse_points': sparseXyzF32.length ~/ 12,
        'cameras_count': count * 36, 'neighbors_count': count * 9,
        'byte_transport': 'native_raw_little_endian',
        'algorithm': 'unchanged_build_session_table_selected_defaults',
      });
    } finally {
      arena.dispose();
    }
  }
}
