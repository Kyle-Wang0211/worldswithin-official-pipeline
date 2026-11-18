                                                         
                                                                       
                                          
library;

import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

typedef _PruneC = Int32 Function(Pointer<Utf8>, Pointer<Utf8>, Int32, Int32);
typedef _PruneD = int Function(Pointer<Utf8>, Pointer<Utf8>, int, int);
typedef _ResealC = Int32 Function(Pointer<Utf8>, Pointer<Utf8>);
typedef _ResealD = int Function(Pointer<Utf8>, Pointer<Utf8>);
typedef _XyC = Int32 Function(Pointer<Utf8>, Pointer<Utf8>, Int32);
typedef _XyD = int Function(Pointer<Utf8>, Pointer<Utf8>, int);
typedef _DigestC = Int32 Function(
  Pointer<Utf8>,
  Pointer<Utf8>,
  Pointer<Utf8>,
  Int32,
);
typedef _DigestD = int Function(
  Pointer<Utf8>,
  Pointer<Utf8>,
  Pointer<Utf8>,
  int,
);

bool get databasePruneSupported => Platform.isIOS;

                                                       
                                             
Future<int> pruneDescriptorsFile(
  String source,
  String output, {
  bool stripKeypointAffine = true,
  bool dropRawMatches = false,
}) => Isolate.run(() {
  final lib = DynamicLibrary.process();
  final fn = lib.lookupFunction<_PruneC, _PruneD>(
    'pw_sqlite_prune_descriptors_file',
  );
  final s = source.toNativeUtf8();
  final o = output.toNativeUtf8();
  try {
    return fn(s, o, stripKeypointAffine ? 1 : 0, dropRawMatches ? 1 : 0);
  } finally {
    calloc.free(s);
    calloc.free(o);
  }
});

                                                  
                              
Future<String?> keypointsXySha256(String databasePath) => Isolate.run(() {
  final lib = DynamicLibrary.process();
  final fn = lib.lookupFunction<_XyC, _XyD>('pw_sqlite_keypoints_xy_sha256');
  final p = databasePath.toNativeUtf8();
  final out = calloc<Uint8>(65);
  try {
    if (fn(p, out.cast<Utf8>(), 65) != 0) return null;
    return out.cast<Utf8>().toDartString();
  } finally {
    calloc.free(p);
    calloc.free(out);
  }
});

                                       
                                       
Future<int> resealArkitPoseDigests(String databasePath, String sidecarPath) =>
    Isolate.run(() {
      final lib = DynamicLibrary.process();
      final fn = lib.lookupFunction<_ResealC, _ResealD>(
        'pw_sqlite_reseal_arkit_pose_digests',
      );
      final d = databasePath.toNativeUtf8();
      final s = sidecarPath.toNativeUtf8();
      try {
        return fn(d, s);
      } finally {
        calloc.free(d);
        calloc.free(s);
      }
    });

                                           
Future<String?> tableContentSha256(String databasePath, String table) =>
    Isolate.run(() {
      final lib = DynamicLibrary.process();
      final fn = lib.lookupFunction<_DigestC, _DigestD>(
        'pw_sqlite_table_content_sha256',
      );
      final p = databasePath.toNativeUtf8();
      final t = table.toNativeUtf8();
      final out = calloc<Uint8>(65);
      try {
        final rc = fn(p, t, out.cast<Utf8>(), 65);
        if (rc != 0) return null;
        return out.cast<Utf8>().toDartString();
      } finally {
        calloc.free(p);
        calloc.free(t);
        calloc.free(out);
      }
    });
