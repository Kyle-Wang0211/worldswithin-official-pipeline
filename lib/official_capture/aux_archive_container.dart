                                              
                                         
                                 
   
           
                              
                
                                                                   
   
                                         
                                      
library;

import 'dart:convert';
import 'dart:typed_data';

const List<int> _magic = <int>[0x50, 0x57, 0x53, 0x43, 0x31, 0, 0, 0];

class AuxContainerEntry {
  const AuxContainerEntry({required this.relativePath, required this.bytes});

  final String relativePath;
  final Uint8List bytes;
}

class AuxContainerFormatException implements Exception {
  const AuxContainerFormatException(this.message);
  final String message;
  @override
  String toString() => 'AuxContainerFormatException: $message';
}

Uint8List encodeAuxContainer(List<AuxContainerEntry> entries) {
  final sorted = <AuxContainerEntry>[...entries]
    ..sort((left, right) => left.relativePath.compareTo(right.relativePath));
  final encodedPaths = <Uint8List>[
    for (final entry in sorted)
      Uint8List.fromList(utf8.encode(entry.relativePath)),
  ];
  var total = _magic.length + 4;
  for (var index = 0; index < sorted.length; index++) {
    total += 2 + encodedPaths[index].length + 4 + sorted[index].bytes.length;
  }
  final out = Uint8List(total);
  final view = ByteData.view(out.buffer);
  out.setRange(0, _magic.length, _magic);
  var offset = _magic.length;
  view.setUint32(offset, sorted.length, Endian.little);
  offset += 4;
  for (var index = 0; index < sorted.length; index++) {
    final path = encodedPaths[index];
    final content = sorted[index].bytes;
    if (path.length > 0xffff) {
      throw const AuxContainerFormatException('path too long');
    }
    view.setUint16(offset, path.length, Endian.little);
    offset += 2;
    out.setRange(offset, offset + path.length, path);
    offset += path.length;
    view.setUint32(offset, content.length, Endian.little);
    offset += 4;
    out.setRange(offset, offset + content.length, content);
    offset += content.length;
  }
  return out;
}

List<AuxContainerEntry> decodeAuxContainer(Uint8List raw) {
  if (raw.length < _magic.length + 4) {
    throw const AuxContainerFormatException('truncated header');
  }
  for (var index = 0; index < _magic.length; index++) {
    if (raw[index] != _magic[index]) {
      throw const AuxContainerFormatException('bad magic');
    }
  }
  final view = ByteData.view(raw.buffer, raw.offsetInBytes, raw.length);
  var offset = _magic.length;
  final count = view.getUint32(offset, Endian.little);
  offset += 4;
  final entries = <AuxContainerEntry>[];
  for (var index = 0; index < count; index++) {
    if (offset + 2 > raw.length) {
      throw const AuxContainerFormatException('truncated path length');
    }
    final pathLength = view.getUint16(offset, Endian.little);
    offset += 2;
    if (offset + pathLength + 4 > raw.length) {
      throw const AuxContainerFormatException('truncated path');
    }
    final path = utf8.decode(raw.sublist(offset, offset + pathLength));
    offset += pathLength;
    final contentLength = view.getUint32(offset, Endian.little);
    offset += 4;
    if (offset + contentLength > raw.length) {
      throw const AuxContainerFormatException('truncated content');
    }
    entries.add(
      AuxContainerEntry(
        relativePath: path,
        bytes: Uint8List.sublistView(raw, offset, offset + contentLength),
      ),
    );
    offset += contentLength;
  }
  if (offset != raw.length) {
    throw const AuxContainerFormatException('trailing bytes');
  }
  return entries;
}
