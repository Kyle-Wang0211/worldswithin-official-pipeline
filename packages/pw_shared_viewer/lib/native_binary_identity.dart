import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';

const _registryRelativePath = 'reconstruction_production/DISTRIBUTION_IDENTITIES.json';
const _compiledRegistrySha256 = String.fromEnvironment(
  'PW_TESTFLIGHT_NATIVE_IDENTITIES_SHA256',
);
final _shaPattern = RegExp(r'^[0-9a-f]{64}$');

                                                                 
                                                                                     
class ReconstructionCanonicalNativeIdentity {
  const ReconstructionCanonicalNativeIdentity(this.canonicalSha256, this.canonicalLength);

  final String canonicalSha256;
  final int canonicalLength;

  @override
  bool operator ==(Object other) =>
      other is ReconstructionCanonicalNativeIdentity &&
      canonicalSha256 == other.canonicalSha256 &&
      canonicalLength == other.canonicalLength;

  @override
  int get hashCode => Object.hash(canonicalSha256, canonicalLength);
}

ReconstructionCanonicalNativeIdentity reconstructionCanonicalNativeIdentity(List<int> input) {
  final bytes = Uint8List.fromList(input);
  if (bytes.length < 32) throw StateError('Truncated Mach-O header');
  final view = ByteData.sublistView(bytes);
  int u32(int at) => view.getUint32(at, Endian.little);
  int u64(int at) {
                                                                               
                                                                        
    if (u32(at + 4) != 0) throw StateError('Unsupported Mach-O file range');
    return u32(at);
  }

  if (u32(0) != 0xfeedfacf || u32(4) != 0x100000c) {
    throw StateError('Only thin little-endian arm64 Mach-O is supported');
  }
  final commandCount = u32(16);
  final commandBytes = u32(20);
  final commandsEnd = 32 + commandBytes;
  if (commandsEnd > bytes.length || commandCount > commandBytes ~/ 8) {
    throw StateError('Invalid Mach-O load-command bounds');
  }
  var cursor = 32;
  ({int offset, int end, int sizeField})? linkedit;
  ({int offset, int length, int sizeField})? signature;
  final segments = <({int offset, int end})>[];
  var encryptionSeen = false;
  for (var index = 0; index < commandCount; index++) {
    if (cursor + 8 > commandsEnd) throw StateError('Truncated load command');
    final command = u32(cursor);
    final size = u32(cursor + 4);
    if (size < 8 || size % 8 != 0 || cursor + size > commandsEnd) {
      throw StateError('Invalid Mach-O load-command size');
    }
    if (command == 0x19) {
      if (size < 72) throw StateError('Truncated segment command');
      final name = bytes.sublist(cursor + 8, cursor + 24);
      final offset = u64(cursor + 40);
      final length = u64(cursor + 48);
      final sections = u32(cursor + 64);
      if (size != 72 + 80 * sections || offset + length > bytes.length) {
        throw StateError('Invalid segment bounds');
      }
      if (length != 0) segments.add((offset: offset, end: offset + length));
      if (ascii.decode(name, allowInvalid: true) ==
          '__LINKEDIT\x00\x00\x00\x00\x00\x00') {
        if (linkedit != null) throw StateError('Duplicate __LINKEDIT segment');
        if (u64(cursor + 32) != ((length + 16383) ~/ 16384) * 16384) {
          throw StateError('Unsupported __LINKEDIT virtual size');
        }
        linkedit = (
          offset: offset,
          end: offset + length,
          sizeField: cursor + 48,
        );
      }
    } else if (command == 0x1d) {
      if (size != 16 || signature != null) {
        throw StateError('Invalid code-signature command');
      }
      signature = (
        offset: u32(cursor + 8),
        length: u32(cursor + 12),
        sizeField: cursor + 12,
      );
    } else if (command == 0x2c) {
      if (size != 24 || encryptionSeen) {
        throw StateError('Invalid encryption command');
      }
      encryptionSeen = true;
      if (u32(cursor + 16) != 0 ||
          u32(cursor + 20) != 0 ||
          u32(cursor + 8) + u32(cursor + 12) > bytes.length) {
        throw StateError('Encrypted or malformed native image');
      }
    }
    cursor += size;
  }
  if (cursor != commandsEnd || linkedit == null || signature == null) {
    throw StateError('Missing or inconsistent Mach-O commands');
  }
  segments.sort((a, b) => a.offset.compareTo(b.offset));
  for (var index = 1; index < segments.length; index++) {
    if (segments[index - 1].end > segments[index].offset) {
      throw StateError('Overlapping Mach-O segments');
    }
  }
  if (signature.length == 0 ||
      signature.offset < commandsEnd ||
      signature.offset + signature.length != bytes.length ||
      signature.offset < linkedit.offset ||
      signature.offset + signature.length != linkedit.end) {
    throw StateError(
      'Code signature must be the exact trailing __LINKEDIT range',
    );
  }
  final canonical = bytes.sublist(0, signature.offset);
  canonical.fillRange(linkedit.sizeField, linkedit.sizeField + 8, 0);
  ByteData.sublistView(canonical).setUint64(
    linkedit.sizeField - 16,
    ((signature.offset - linkedit.offset + 16383) ~/ 16384) * 16384,
    Endian.little,
  );
  canonical.fillRange(signature.sizeField, signature.sizeField + 4, 0);
  return ReconstructionCanonicalNativeIdentity(
    sha256.convert(canonical).toString(),
    signature.offset,
  );
}

Future<void> verifyReconstructionNativeBinary(
  File file,
  String expectedSha256, {
  int? expectedBytes,
}) =>
    ReconstructionNativeIdentityRegistry(
      expectedRegistrySha256: _compiledRegistrySha256,
    ).verify(file, expectedSha256, expectedBytes: expectedBytes);

                                                                                  
class ReconstructionNativeIdentityRegistry {
  const ReconstructionNativeIdentityRegistry({required this.expectedRegistrySha256});

  final String expectedRegistrySha256;

  Future<void> verify(
    File file,
    String expectedSha256, {
    int? expectedBytes,
  }) async {
    try {
      final data = await file.readAsBytes();
      if (sha256.convert(data).toString() == expectedSha256 &&
          (expectedBytes == null || data.length == expectedBytes)) return;
      if (!_shaPattern.hasMatch(expectedSha256) ||
          !_shaPattern.hasMatch(expectedRegistrySha256)) {
        throw StateError('Invalid native identity pin');
      }
      final canonicalPath = await _canonicalRegular(file);
      var ancestor = File(canonicalPath).parent.path;
      while (!Directory(ancestor)
          .uri
          .pathSegments
          .where((part) => part.isNotEmpty)
          .last
          .endsWith('.app')) {
        final parent = Directory(ancestor).parent.path;
        if (parent == ancestor) {
          throw StateError('Native image has no application registry');
        }
        ancestor = parent;
      }
      final registryFile =
          File.fromUri(Directory(ancestor).uri.resolve(_registryRelativePath));
      await _canonicalRegular(registryFile);
      final registryBytes = await registryFile.readAsBytes();
      if (sha256.convert(registryBytes).toString() != expectedRegistrySha256) {
        throw StateError('Native identity registry digest mismatch');
      }
      final registry = jsonDecode(utf8.decode(registryBytes));
      if (registry is! Map<String, dynamic> ||
          registry['schema'] != 'RECONSTRUCTION_TESTFLIGHT_NATIVE_IDENTITIES_V1' ||
          registry['entries'] is! Map<String, dynamic>) {
        throw StateError('Unsupported native identity registry');
      }
      final relative = canonicalPath
          .substring(ancestor.length + 1)
          .replaceAll(Platform.pathSeparator, '/');
      final row = (registry['entries'] as Map<String, dynamic>)[relative];
      if (row is! Map<String, dynamic> ||
          row['baseline_sha256'] != expectedSha256 ||
          row['baseline_bytes'] is! int ||
          row['baseline_bytes'] <= 0 ||
          (expectedBytes != null && row['baseline_bytes'] != expectedBytes) ||
          row['canonical_length'] is! int ||
          row['canonical_length'] <= 0 ||
          row['canonical_sha256'] is! String ||
          !_shaPattern.hasMatch(row['canonical_sha256'])) {
        throw StateError('Native identity baseline mismatch');
      }
      final identity = reconstructionCanonicalNativeIdentity(data);
      if (identity.canonicalSha256 != row['canonical_sha256'] ||
          identity.canonicalLength != row['canonical_length']) {
        throw StateError('Native canonical identity mismatch');
      }
    } on FileSystemException catch (error) {
      throw StateError('Native identity file error: $error');
    } on FormatException catch (error) {
      throw StateError('Invalid native identity registry: $error');
    }
  }

  Future<String> _canonicalRegular(File file) async {
    final absolute = file.absolute.uri.normalizePath().toFilePath();
    if (absolute != file.absolute.path ||
        await FileSystemEntity.type(absolute, followLinks: false) !=
            FileSystemEntityType.file ||
        await file.resolveSymbolicLinks() != absolute) {
      throw StateError('Native identity paths must be canonical regular files');
    }
    return absolute;
  }
}
