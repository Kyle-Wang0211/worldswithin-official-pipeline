                                            
   
                                                         
                                            
                                   
                                                    
   
                                      
library pw_sidecar;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:crypto/crypto.dart';

const _magic = 'PWSC1';

class SidecarEntry {
  final String path;
  final Uint8List bytes;
  SidecarEntry(this.path, this.bytes);
}

Uint8List _deflate(List<int> d) =>
    Uint8List.fromList(ZLibCodec(level: 9).encode(d));

Uint8List _inflate(List<int> d) => Uint8List.fromList(zlib.decode(d));

void _writeU32(BytesBuilder b, int v) =>
    b.add([v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF]);

int _readU32(Uint8List b, int at) =>
    b[at] | (b[at + 1] << 8) | (b[at + 2] << 16) | (b[at + 3] << 24);

                               
Uint8List packSidecars(List<SidecarEntry> entries) {
  final sorted = [...entries]..sort((a, b) => a.path.compareTo(b.path));
  final builder = BytesBuilder()..add(ascii.encode(_magic));
  _writeU32(builder, sorted.length);
  for (final entry in sorted) {
    final pathBytes = utf8.encode(entry.path);
    _writeU32(builder, pathBytes.length);
    builder.add(pathBytes);
    _writeU32(builder, entry.bytes.length);
    builder.add(sha256.convert(entry.bytes).bytes);
    final packed = _deflate(entry.bytes);
    _writeU32(builder, packed.length);
    builder.add(packed);
  }
  return builder.toBytes();
}

                                   
List<SidecarEntry> unpackSidecars(Uint8List archive) {
  var pos = 0;
  if (ascii.decode(Uint8List.sublistView(archive, 0, 5)) != _magic) {
    throw const FormatException('bad magic');
  }
  pos = 5;
  final count = _readU32(archive, pos);
  pos += 4;
  final out = <SidecarEntry>[];
  for (var i = 0; i < count; i++) {
    final pathLen = _readU32(archive, pos);
    pos += 4;
    final path = utf8.decode(archive.sublist(pos, pos + pathLen));
    pos += pathLen;
    final originalLength = _readU32(archive, pos);
    pos += 4;
    final expected = archive.sublist(pos, pos + 32);
    pos += 32;
    final compLen = _readU32(archive, pos);
    pos += 4;
    final restored = _inflate(archive.sublist(pos, pos + compLen));
    pos += compLen;
    if (restored.length != originalLength) {
      throw FormatException('length mismatch: $path');
    }
    final actual = sha256.convert(restored).bytes;
    for (var b = 0; b < 32; b++) {
      if (actual[b] != expected[b]) {
        throw FormatException('sha256 mismatch: $path — refusing to emit');
      }
    }
    out.add(SidecarEntry(path, restored));
  }
  return out;
}
