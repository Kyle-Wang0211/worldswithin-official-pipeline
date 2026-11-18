                                                  
   
                                                 
                              
                                                                              
                                      
                                                            
                                                         
                                                   
               
   
                                       
                       
   
                                             
              
library pwva;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:convert/convert.dart' show AccumulatorSink;
import 'package:crypto/crypto.dart';

                                                          
class EncodedFrame {
  final Uint8List bytes;
  final bool keyframe;
  EncodedFrame(this.bytes, this.keyframe);
}

                                         
abstract class HevcFrameEncoder {
                                              
  EncodedFrame encodeNv12(Uint8List y, Uint8List uv,
      {required int ptsMs, required int durationMs});
  void dispose();
}

                                           
abstract class HevcFrameDecoder {
                                               
  void decodeAu(Uint8List au, Uint8List y, Uint8List uv);
  void dispose();
}

class PwvaIndexEntry {
  final int frame;
  final int offset;
  final int length;
  final bool keyframe;
  final int gop;
  final String? source;
  final double? trigger;
  final String? sourceSha256;
  PwvaIndexEntry(this.frame, this.offset, this.length, this.keyframe, this.gop,
      {this.source, this.trigger, this.sourceSha256});

  Map<String, Object?> toJson() => {
        'frame': frame,
        'offset': offset,
        'len': length,
        'keyframe': keyframe,
        'gop': gop,
        if (source != null) 'source': source,
        if (trigger != null) 'trigger': trigger,
        if (sourceSha256 != null) 'source_sha256': sourceSha256,
      };

  static PwvaIndexEntry fromJson(Map<String, dynamic> j) => PwvaIndexEntry(
      j['frame'], j['offset'], j['len'], j['keyframe'], j['gop'],
      source: j['source'],
      trigger: (j['trigger'] as num?)?.toDouble(),
      sourceSha256: j['source_sha256']);
}

                                     
class PwvaWriter {
  final HevcFrameEncoder _encoder;
  final RandomAccessFile _stream;
  final RandomAccessFile _index;
  final Directory _dir;
  final int width;
  final int height;
  final int gop;
  int _offset = 0;
  int _frame = 0;
  int _gopId = -1;
  bool _closed = false;

  PwvaWriter(this._encoder, Directory dir,
      {required this.width, required this.height, required this.gop})
      : _dir = dir,
        _stream = File('${dir.path}/photos.hevc')
            .openSync(mode: FileMode.writeOnlyAppend),
        _index = File('${dir.path}/photos.pwvi')
            .openSync(mode: FileMode.writeOnlyAppend);

  PwvaIndexEntry addFrameNv12(Uint8List y, Uint8List uv,
      {String? source, double? trigger}) {
    if (_closed) throw StateError('writer closed');
    final encoded = _encoder.encodeNv12(y, uv,
        ptsMs: _frame * 300, durationMs: 300);
    return addEncodedFrame(encoded, source: source, trigger: trigger);
  }

                                            
  PwvaIndexEntry addEncodedFrame(EncodedFrame encoded,
      {String? source, double? trigger, String? sourceSha256}) {
    if (_closed) throw StateError('writer closed');
    if (encoded.keyframe) _gopId++;
    _stream.writeFromSync(encoded.bytes);
    _stream.flushSync();
    final entry = PwvaIndexEntry(_frame, _offset, encoded.bytes.length,
        encoded.keyframe, _gopId,
        source: source, trigger: trigger, sourceSha256: sourceSha256);
    _index.writeStringSync('${jsonEncode(entry.toJson())}\n');
    _index.flushSync();
    _offset += encoded.bytes.length;
    _frame++;
    return entry;
  }

                                                     
                               
  Future<void> finalize({required String captureId, String? extra}) async {
    _stream.closeSync();
    _index.closeSync();
    _closed = true;
                                                   
                                
    final digestSink = AccumulatorSink<Digest>();
    final hashSink = sha256.startChunkedConversion(digestSink);
    final streamFile = File('${_dir.path}/photos.hevc').openSync();
    try {
      const chunk = 4 * 1024 * 1024;
      while (true) {
        final bytes = streamFile.readSync(chunk);
        if (bytes.isEmpty) break;
        hashSink.add(bytes);
      }
    } finally {
      streamFile.closeSync();
      hashSink.close();
    }
    final digest = digestSink.events.single.toString();
    File('${_dir.path}/manifest.json').writeAsStringSync(
        const JsonEncoder.withIndent(' ').convert({
      'schema': 'pw_video_archive_v1',
      'capture_id': captureId,
      'codec': 'hevc',
      'resolution': '${width}x$height',
      'frame_count': _frame,
      'gop': gop,
      'stream_bytes': _offset,
      'stream_sha256': digest,
      'index': 'photos.pwvi',
      if (extra != null) 'extra': extra,
    }));
    _encoder.dispose();
  }
}

                                           
class PwvaReader {
  final Directory _dir;
  final List<PwvaIndexEntry> _entries;
  final int width;
  final int height;
  final HevcFrameDecoder Function(Uint8List keyframeAu) _decoderFactory;

  PwvaReader(Directory dir, this._decoderFactory)
      : _dir = dir,
        _entries = File('${dir.path}/photos.pwvi')
            .readAsLinesSync()
            .map((l) => PwvaIndexEntry.fromJson(jsonDecode(l)))
            .toList(),
        width = int.parse((jsonDecode(
                File('${dir.path}/manifest.json').readAsStringSync())[
            'resolution'] as String)
            .split('x')[0]),
        height = int.parse((jsonDecode(
                File('${dir.path}/manifest.json').readAsStringSync())[
            'resolution'] as String)
            .split('x')[1]);

  int get frameCount => _entries.length;
  List<PwvaIndexEntry> get entries => List.unmodifiable(_entries);

                                                       
  ({Uint8List y, Uint8List uv, int decodedAus}) readFrameNv12(int frame) {
    if (frame < 0 || frame >= _entries.length) {
      throw RangeError.range(frame, 0, _entries.length - 1);
    }
    var key = frame;
    while (key > 0 && !_entries[key].keyframe) {
      key--;
    }
    final stream = File('${_dir.path}/photos.hevc').openSync();
    Uint8List au(PwvaIndexEntry e) {
      stream.setPositionSync(e.offset);
      return stream.readSync(e.length);
    }

    final decoder = _decoderFactory(au(_entries[key]));
    final y = Uint8List(width * height);
    final uv = Uint8List(width * height ~/ 2);
    var count = 0;
    try {
      for (var i = key; i <= frame; i++) {
        decoder.decodeAu(au(_entries[i]), y, uv);
        count++;
      }
    } finally {
      decoder.dispose();
      stream.closeSync();
    }
    return (y: y, uv: uv, decodedAus: count);
  }
}
