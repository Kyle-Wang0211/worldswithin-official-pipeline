import 'dart:convert';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

                                                                      
                                                                
                                                                           
                                                                       
final class Apple64Statvfs extends Struct {
  @UnsignedLong()
  external int blockSize;
  @UnsignedLong()
  external int fragmentSize;
  @Uint32()
  external int blocks;
  @Uint32()
  external int freeBlocks;
  @Uint32()
  external int availableBlocks;
  @Uint32()
  external int files;
  @Uint32()
  external int freeFiles;
  @Uint32()
  external int availableFiles;
  @UnsignedLong()
  external int filesystemId;
  @UnsignedLong()
  external int flags;
  @UnsignedLong()
  external int nameMax;
}

typedef StatvfsNativeQuery =
    int Function(Pointer<Utf8>, Pointer<Apple64Statvfs>);
typedef _StatvfsC = Int32 Function(Pointer<Utf8>, Pointer<Apple64Statvfs>);

                                                                                
                                                                            
                                                                               
                                                                                
                                                                   
int readAvailableStorageInWorker(
  String path, {
  StatvfsNativeQuery? nativeQuery,
}) {
  final encoded = utf8.encode(path);
                                                                              
  if (!path.startsWith('/') || encoded.contains(0) || encoded.length > 959) {
    throw ArgumentError.value(
      path,
      'path',
      'Expected absolute UTF-8 path <=959 bytes without NUL',
    );
  }
  try {
    final abi = Abi.current();
    if (!(Platform.isIOS || Platform.isMacOS) ||
        ![Abi.iosArm64, Abi.macosArm64, Abi.macosX64].contains(abi) ||
        sizeOf<Apple64Statvfs>() != 64) {
      throw UnsupportedError('Unverified statvfs ABI');
    }
    final query =
        nativeQuery ??
        DynamicLibrary.process().lookupFunction<_StatvfsC, StatvfsNativeQuery>(
          'statvfs',
        );
    return using((arena) {
      final buffer = arena<Apple64Statvfs>();
      final status = query(path.toNativeUtf8(allocator: arena), buffer);
      if (status != 0) throw StateError('statvfs failed');
      final size = buffer.ref.fragmentSize;
      final blocks = buffer.ref.availableBlocks;
                                                                         
                                                                                
      if (size <= 0 ||
          blocks < 0 ||
          size > 0x7fffffffffffffff ||
          blocks > 0x7fffffffffffffff ~/ size) {
        throw StateError('Invalid or overflowing statvfs measurement');
      }
      return blocks * size;
    });
  } catch (error) {
    throw StateError('可用存储空间读取失败，已停止生成，请稍后重试 ($error)');
  }
}
