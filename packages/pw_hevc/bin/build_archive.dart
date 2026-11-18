                                          
  
                                       
                                           
  
        
                                  
                                                                
                                              
  
                                                                                                    

import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

typedef _CreateC = Pointer<Void> Function(Int32, Int32, Int32, Double, Int64);
typedef _CreateD = Pointer<Void> Function(int, int, int, double, int);
typedef _EncodeC = Int32 Function(Pointer<Void>, Pointer<Uint8>, Pointer<Uint8>,
    Int64, Int64, Pointer<Pointer<Uint8>>, Pointer<Int64>, Pointer<Int32>);
typedef _EncodeD = int Function(Pointer<Void>, Pointer<Uint8>, Pointer<Uint8>,
    int, int, Pointer<Pointer<Uint8>>, Pointer<Int64>, Pointer<Int32>);
typedef _FreeC = Void Function(Pointer<Uint8>);
typedef _FreeD = void Function(Pointer<Uint8>);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyD = void Function(Pointer<Void>);

void main(List<String> args) async {
  final nv12Path = args[0];
  final bundlePath = args[1];
  final w = int.parse(args[2]);
  final h = int.parse(args[3]);
  final gop = int.parse(args[4]);
  final quality = double.parse(args[5]);
  final outDir = Directory(args[6])..createSync(recursive: true);

  final bundle = jsonDecode(File(bundlePath).readAsStringSync());
  final frames = (bundle['frames'] as List)
      .map((f) => f as Map<String, dynamic>)
      .toList()
    ..sort((a, b) =>
        (a['triggerTimestamp'] as num).compareTo(b['triggerTimestamp'] as num));

  final libDir = File(Platform.script.toFilePath()).parent.parent.path;
  final lib = DynamicLibrary.open('$libDir/native/libpw_vt_encoder.dylib');
  final create = lib.lookupFunction<_CreateC, _CreateD>('pw_vt_create');
  final encode = lib.lookupFunction<_EncodeC, _EncodeD>('pw_vt_encode_nv12');
  final freeBuf = lib.lookupFunction<_FreeC, _FreeD>('pw_vt_free');
  final destroy = lib.lookupFunction<_DestroyC, _DestroyD>('pw_vt_destroy');

  final enc = create(w, h, gop, quality, 0);
  if (enc == nullptr) {
    stderr.writeln('编码器创建失败');
    exit(1);
  }

  final ySize = w * h;
  final frameBytes = ySize * 3 ~/ 2;
  final raw = File(nv12Path).openSync();
  final yPtr = calloc<Uint8>(ySize);
  final uvPtr = calloc<Uint8>(ySize ~/ 2);
  final outData = calloc<Pointer<Uint8>>();
  final outLen = calloc<Int64>();
  final outKey = calloc<Int32>();

  final stream = File('${outDir.path}/photos.hevc').openSync(mode: FileMode.write);
  final index = File('${outDir.path}/photos.pwvi').openSync(mode: FileMode.write);
  var offset = 0;
  var gopId = -1;

  for (var i = 0; i < frames.length; i++) {
    final chunk = raw.readSync(frameBytes);
    yPtr.asTypedList(ySize).setAll(0, chunk.sublist(0, ySize));
    uvPtr.asTypedList(ySize ~/ 2).setAll(0, chunk.sublist(ySize));
    final st = encode(enc, yPtr, uvPtr, i * 300, 300, outData, outLen, outKey);
    if (st != 0) {
      stderr.writeln('帧 $i 编码失败: $st');
      exit(1);
    }
    final n = outLen.value;
    final key = outKey.value == 1;
    if (key) gopId++;
                                  
    stream.writeFromSync(outData.value.asTypedList(n));
    stream.flushSync();
    index.writeStringSync('${jsonEncode({
          'frame': i,
          'offset': offset,
          'len': n,
          'keyframe': key,
          'gop': gopId,
          'source': frames[i]['highresFilename'],
          'trigger': frames[i]['triggerTimestamp'],
        })}\n');
    index.flushSync();
    offset += n;
    freeBuf(outData.value);
  }
  stream.closeSync();
  index.closeSync();
  raw.closeSync();
  destroy(enc);

  final streamBytes = File('${outDir.path}/photos.hevc').lengthSync();
  final digest = Process.runSync('shasum',
      ['-a', '256', '${outDir.path}/photos.hevc']).stdout.toString().split(' ')[0];
  final manifest = {
    'schema': 'pw_video_archive_v1',
    'capture_id': bundle['photoBundleOwner'] ?? 'capture',
    'resolution': '${w}x$h',
    'frame_count': frames.length,
    'gop': gop,
    'encoder': 'videotoolbox_hevc',
    'quality': quality,
    'stream_bytes': streamBytes,
    'stream_sha256': digest,
    'index': 'photos.pwvi',
  };
  File('${outDir.path}/manifest.json')
      .writeAsStringSync(const JsonEncoder.withIndent(' ').convert(manifest));
  final indexBytes = File('${outDir.path}/photos.pwvi').lengthSync();
  print(jsonEncode({
    'stream_bytes': streamBytes,
    'index_bytes': indexBytes,
    'manifest_bytes': File('${outDir.path}/manifest.json').lengthSync(),
    'frames': frames.length,
    'gops': gopId + 1,
  }));
}
