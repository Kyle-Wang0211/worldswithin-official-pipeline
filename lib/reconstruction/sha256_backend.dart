import 'dart:ffi';
import 'dart:io';
import 'package:crypto/crypto.dart';
import 'package:ffi/ffi.dart';

                                                                           
                                                                              
final class _Sha256Context extends Struct {
  @Array(2)
  external Array<Uint32> count;
  @Array(8)
  external Array<Uint32> hash;
  @Array(16)
  external Array<Uint32> wbuf;
}

typedef _InitNative = Int32 Function(Pointer<_Sha256Context>);
typedef _Init = int Function(Pointer<_Sha256Context>);
typedef _UpdateNative = Int32 Function(
    Pointer<_Sha256Context>, Pointer<Uint8>, Uint32);
typedef _Update = int Function(Pointer<_Sha256Context>, Pointer<Uint8>, int);
typedef _FinalNative = Int32 Function(Pointer<Uint8>, Pointer<_Sha256Context>);
typedef _Final = int Function(Pointer<Uint8>, Pointer<_Sha256Context>);

final _commonCrypto = _CommonCrypto.load();
String get reconstructionSha256Backend => _commonCrypto == null
    ? 'dart_crypto_sha256'
    : 'apple_commoncrypto_sha256';

final class _CommonCrypto {
  _CommonCrypto(DynamicLibrary library)
      : init = library.lookupFunction<_InitNative, _Init>('CC_SHA256_Init'),
        update = library.lookupFunction<_UpdateNative, _Update>('CC_SHA256_Update'),
        finish = library.lookupFunction<_FinalNative, _Final>('CC_SHA256_Final');

  final _Init init;
  final _Update update;
  final _Final finish;

  static _CommonCrypto? load() {
    if (!const bool.fromEnvironment('RECONSTRUCTION_NATIVE_SHA256', defaultValue: true) ||
        !(Platform.isIOS || Platform.isMacOS)) return null;
    try {
                                                                              
                                                                          
      return _CommonCrypto(DynamicLibrary.open('/usr/lib/libSystem.B.dylib'));
    } on ArgumentError {
                                                                               
      return null;
    }
  }

  Future<String> digest(Stream<List<int>> stream) async {
    final arena = Arena(calloc);
    try {
      final context = arena<_Sha256Context>();
      final buffer = arena<Uint8>(65536);
      final output = arena<Uint8>(32);
      if (init(context) != 1) throw StateError('CC_SHA256_Init failed');
      final scratch = buffer.asTypedList(65536);
      await for (final chunk in stream) {
                                                                            
                                                                 
        for (var offset = 0; offset < chunk.length; offset += scratch.length) {
          final remaining = chunk.length - offset;
          final length = remaining < scratch.length ? remaining : scratch.length;
          scratch.setRange(0, length, chunk, offset);
          if (update(context, buffer, length) != 1) {
            throw StateError('CC_SHA256_Update failed');
          }
        }
      }
      if (finish(output, context) != 1) {
        throw StateError('CC_SHA256_Final failed');
      }
      return output.asTypedList(32)
          .map((byte) => byte.toRadixString(16).padLeft(2, '0')).join();
    } finally {
      arena.releaseAll();
    }
  }
}

                                                                         
                                                                              
Future<String> reconstructionSha256Stream(Stream<List<int>> stream) async {
  final backend = _commonCrypto;
  if (backend != null) return backend.digest(stream);
  return (await sha256.bind(stream).first).toString();
}
