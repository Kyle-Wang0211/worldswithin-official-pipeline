                                           
                            
   
                                        
                                                  
library;

import 'dart:io';

import 'aux_archive_manifest.dart';
import 'aux_archive_transaction.dart';
import 'database_archive_codec.dart';

                                                 
class SidecarSession {
  SidecarSession._(this.directory, this._temporary);

  final Directory directory;
  final Directory? _temporary;

                               
  bool get materialized => _temporary != null;

  Future<void> dispose() async {
    final temporary = _temporary;
    if (temporary == null) return;
    try {
      if (await temporary.exists()) await temporary.delete(recursive: true);
    } on FileSystemException {
                             
    }
  }
}

class AuxArchiveResolver {
  const AuxArchiveResolver({required this.codec});

  final DatabaseArchiveCodec codec;

                                                     
                                      
  Future<SidecarSession?> openSidecars(Directory captureDirectory) async {
    final live = Directory('${captureDirectory.path}/photos_highres');
    try {
      if (await live.exists()) {
        await for (final entity in live.list(followLinks: false)) {
          if (entity is File && entity.path.endsWith('.json')) {
            return SidecarSession._(live, null);
          }
        }
      }
    } on FileSystemException {
                
    }

    final manifest = await AuxArchiveManifest.read(captureDirectory);
    final bundle = manifest?.bundles[kAuxSidecarBundleId];
    if (bundle == null) return null;
    final entries = await restoreAuxBundle(
      captureDirectory: captureDirectory,
      bundle: bundle,
      codec: codec,
    );
    if (entries == null) return null;

    Directory? scratch;
    try {
      scratch = await Directory.systemTemp.createTemp('pw_sidecars_');
      for (final entry in entries) {
        final name = entry.relativePath.split('/').last;
        await File('${scratch.path}/$name')
            .writeAsBytes(entry.bytes, flush: true);
      }
      return SidecarSession._(scratch, scratch);
    } catch (_) {
      final created = scratch;
      if (created != null) {
        try {
          if (await created.exists()) await created.delete(recursive: true);
        } catch (_) {}
      }
      return null;
    }
  }
}
