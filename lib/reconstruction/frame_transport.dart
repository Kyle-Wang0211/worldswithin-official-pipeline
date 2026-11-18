import 'dart:io';
import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'package:crypto/crypto.dart';

const officialNoiseGeneratorSha256 =
    'd8f756b5f482c0c5bd1b77a4176cda3928f160b0eacc879f4b7840b4a70458e1';

const originalRetainedProfile = 'original_numpy_opencv_retained_v2';
const originalRetainedLegacyProfile = 'original_numpy_opencv_retained97_v1';
const wholeRetainedTransportProfile = 'whole_mixed_precision_gpu_fusion';
const legacyFixtureFrameCount = 97;
const productionMinimumFrameCount = 1;
                                                                                
const officialNoiseBankCapacity = 150;
const officialLowNoiseFrameBytes = 552960;
const officialHighNoiseFrameBytes = 2211840;
const legacy97LowNoiseBankSha256 =
    'be0a6856547538a5a10537e9c64390e8db7e9eac7e0a39ca563b01ff7ce27984';
const legacy97HighNoiseBankSha256 =
    '77b9df4766b323048b02a8dfecd1fd46d375a5be8c4492d8d8c6aed5c4c1b6b1';
const legacy97NoiseProducerSha256 =
    'aa09d2db3c79d940d0f741fae4dcd0523bf02d8ff69145cf98d5ed47aff4703f';

                                                                        
const officialLowNoiseBankSha256 =
    'ee6b0b1a1f863c11e2bc323563758132f5558a6ccef4d143265232d1e70c9197';
const officialHighNoiseBankSha256 =
    '311e6909cb6fcd87c6102430ca78310c8f8d523ca48b0fe74e44f8c52bb28bf7';
const officialNoiseProducerSha256 =
    '142881cc7ced969d9b187598788b8faade2c25aad59d086fdcbdbffd38cb8a0c';

bool variableFrameTransport(Map<String, dynamic> request) =>
    request['dense_producer_profile'] == originalRetainedProfile ||
    request['dense_producer_profile'] == wholeRetainedTransportProfile;
int checkedFrameCount(int count) {
  if (count < productionMinimumFrameCount) {
    throw StateError(
      'Positive actual production frame count required; no frame selection is permitted',
    );
  }
  return count;
}

int frameTransportCount(Map<String, dynamic> request, {int? actualCount}) {
  if (!variableFrameTransport(request)) {
    throw StateError('Explicit actual-count dense producer profile required');
  }
  final declared = request['ordered_frames'];
  if (declared != null && declared is! int) {
    throw StateError('Integer actual ordered frame count required');
  }
  if (actualCount == null && declared == null) {
    throw StateError('Explicit actual ordered frame count required');
  }
  final count = checkedFrameCount(actualCount ?? declared as int);
  if (declared != null && declared != count) {
    throw StateError('Declared and actual frame count mismatch');
  }
  return count;
}

void validateFrameTransportTemplate(Map<String, dynamic> recipe) {
  if (!variableFrameTransport(recipe) ||
      recipe['frame_count_policy'] != 'actual_positive_count' ||
      recipe.containsKey('ordered_frames')) {
    throw StateError('Count-independent actual-frame transport recipe required');
  }
}

                                                                      
int frameTransportTemplateActualCount(
  Map<String, dynamic> recipe, {
  required int actualCount,
}) {
  validateFrameTransportTemplate(recipe);
  return frameTransportCount({
    'dense_producer_profile': recipe['dense_producer_profile'],
    'ordered_frames': actualCount,
  }, actualCount: actualCount);
}

void validateFrameTransportRows(
  Map<String, dynamic> request,
  List<Map<String, dynamic>> rows,
) {
  final count = frameTransportCount(request);
  if (rows.length != count)
    throw StateError('Actual ordered frame count mismatch');
  for (var i = 0; i < count; i++) {
    if (rows[i]['pack_index'] != i || rows[i]['original_index'] != i)
      throw StateError('Actual ordered frame indices changed');
  }
}

int lowNoiseBytes(int count) =>
    checkedFrameCount(count) * officialLowNoiseFrameBytes;
int highNoiseBytes(int count) =>
    checkedFrameCount(count) * officialHighNoiseFrameBytes;

                                                                                
                                                                         
Future<Map<String, dynamic>> copyOfficialNoisePrefix({
  required File source,
  required File destination,
  required int count,
  required bool high,
  required String expectedBankSha256,
}) async {
  final n = checkedFrameCount(count);
  final block = high ? officialHighNoiseFrameBytes : officialLowNoiseFrameBytes;
  final expected = high
      ? officialHighNoiseBankSha256
      : officialLowNoiseBankSha256;
  Future<String> hash(File f) async =>
      (await sha256.bind(f.openRead()).first).toString();
  if (await FileSystemEntity.type(source.path, followLinks: false) !=
          FileSystemEntityType.file ||
      await source.resolveSymbolicLinks() != source.path)
    throw StateError('Canonical regular official noise bank required');
  if (expectedBankSha256 != expected ||
      await source.length() != block * officialNoiseBankCapacity ||
      await hash(source) != expected)
    throw StateError('Official noise bank identity mismatch');
  if (await FileSystemEntity.type(destination.path, followLinks: false) !=
      FileSystemEntityType.notFound)
    throw StateError('Fresh request-owned noise prefix required');
  await destination.parent.create(recursive: true);
  final copiedFrames = n < officialNoiseBankCapacity ? n : officialNoiseBankCapacity;
  final sink = destination.openWrite();
  try {
    await sink.addStream(source.openRead(0, copiedFrames * block));
    await sink.flush();
  } finally {
    await sink.close();
  }
  final prefixHash = (await sha256.bind(source.openRead(0, copiedFrames * block)).first)
      .toString();
  if (n > copiedFrames) {
    final libraryPath = '${File(Platform.resolvedExecutable).parent.path}/Frameworks/libreconstructionOfficialNoise.dylib';
    if (await hash(File(libraryPath)) != officialNoiseGeneratorSha256) {
      throw StateError("Official noise generator bytes changed");
    }
    final library = DynamicLibrary.open(libraryPath);
    final append = library.lookupFunction<
        Int32 Function(Pointer<Utf8>, Uint32, Uint64, Uint64),
        int Function(Pointer<Utf8>, int, int, int)>(
      'reconstruction_official_noise_append_v1',
    );
    final path = destination.path.toNativeUtf8();
    try {
      final result = append(path, high ? 1536 : 768, copiedFrames, n - copiedFrames);
      if (result != 0) {
        final error = library.lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
          'reconstruction_official_noise_last_error_v1',
        );
        throw StateError('Original noise generation failed. ${error().toDartString()}');
      }
    } finally {
      malloc.free(path);
    }
  }
  final outputHash = await hash(destination);
  if (await destination.length() != n * block ||
      (n <= copiedFrames && outputHash != prefixHash) ||
      await hash(source) != expected)
    throw StateError('Official noise prefix byte identity changed');
  if (n == legacyFixtureFrameCount && prefixHash !=
      (high ? legacy97HighNoiseBankSha256 : legacy97LowNoiseBankSha256)) {
    throw StateError('Legacy97 noise prefix byte identity changed');
  }
  return {
    'path': destination.path,
    'bytes': n * block,
    'sha256': outputHash,
    'source_bank_sha256': expected,
    'source_bank_frames': officialNoiseBankCapacity,
    'ordered_frames': n,
    'frame_bytes': block,
    'source_offset': 0,
    'seed_base': 0,
    'seed_rule': 'torch.manual_seed(0 + frame_index)',
    'draw_order': 'stage2_then_stage3_randn_like',
    'copy_kind': n <= officialNoiseBankCapacity
        ? 'exact_first_N_frame_blocks'
        : 'original_seed_bank_with_generated_tail',
    if (n > officialNoiseBankCapacity)
      'generator_sha256': officialNoiseGeneratorSha256,
  };
}

void validateNoisePrefixIdentity(Map<String, dynamic> noise, int count) {
  final streams = noise['prefix_streams'];
  if (noise['kind'] != 'seeded_cpu_gaussian_prefix' ||
      noise['ordered_frames'] != count ||
      streams is! Map ||
      noise['source_producer_sha256'] != officialNoiseProducerSha256)
    throw StateError('Source-recorded actual noise prefix required');
  for (final high in [false, true]) {
    final row = streams[high ? 'high' : 'low'];
    if (row is! Map ||
        row['sha256'] is! String ||
        !RegExp(r'^[0-9a-f]{64}$').hasMatch(row['sha256'] as String) ||
        row['ordered_frames'] != count ||
        row['source_bank_frames'] != officialNoiseBankCapacity ||
        row['source_bank_sha256'] !=
            (high ? officialHighNoiseBankSha256 : officialLowNoiseBankSha256) ||
        row['frame_bytes'] !=
            (high ? officialHighNoiseFrameBytes : officialLowNoiseFrameBytes) ||
        row['bytes'] != (high ? highNoiseBytes(count) : lowNoiseBytes(count)) ||
        row['sha256'] != noise[high ? 'high_sha256' : 'low_sha256'] ||
        row['source_offset'] != 0 ||
        row['seed_base'] != 0 ||
        row['seed_rule'] != 'torch.manual_seed(0 + frame_index)' ||
        row['draw_order'] != 'stage2_then_stage3_randn_like' ||
        row['copy_kind'] != (count <= officialNoiseBankCapacity
            ? 'exact_first_N_frame_blocks'
            : 'original_seed_bank_with_generated_tail') ||
        (count > officialNoiseBankCapacity && row['generator_sha256'] != officialNoiseGeneratorSha256))
      throw StateError('Official per-frame seed/draw/prefix identity mismatch');
    if (count == legacyFixtureFrameCount &&
        row['sha256'] != (high ? legacy97HighNoiseBankSha256 : legacy97LowNoiseBankSha256)) {
      throw StateError('Legacy97 noise prefix byte identity changed');
    }
    if (count == officialNoiseBankCapacity &&
        row['sha256'] != row['source_bank_sha256'])
      throw StateError('Full authenticated bank bytes must remain identical');
  }
}

int cameraTableBytes(int count) => checkedFrameCount(count) * 36 * 4;
int neighborTableBytes(int count) => checkedFrameCount(count) * 9 * 4;
int originalImageTableBytes(int count) =>
    checkedFrameCount(count) * 576 * 768 * 2;
int mergedCerBytes(int count) => checkedFrameCount(count) * 1536 * 1152 * 4;
