                                    
   
                                     
                                                                    
                                                                          
                                                                   
                                                               
                                                      
                                                               
                                                                      
                                                           
                                                           
   
                                              
                                           
                                        
   
                                                   
                                                
                                             
                    
library;

import 'dart:async';
import 'dart:io';

import 'package:crypto/crypto.dart';

import 'aux_archive_container.dart';
import 'aux_archive_manifest.dart';
import 'database_archive_codec.dart';
import 'database_archive_transaction.dart'
    show databaseArchiveFilesEqual, databaseArchiveSha256;

typedef AuxArchiveContinueCheck = FutureOr<bool> Function();

const String kAuxSidecarBundleId = 'sidecars_v1';
const String kAuxDiagLogBundleId = 'diag_log_v1';
const String kAuxSidecarArchiveFileName = 'photos_highres_sidecars.zpaq';
const String kAuxDiagLogArchiveFileName = 'sfm_match_fail.jsonl.zpaq';
const String kAuxDiagLogSourceName = 'sfm_match_fail.jsonl';
const String _sidecarDirectoryName = 'photos_highres';

class AuxArchiveRunResult {
  const AuxArchiveRunResult({
    this.applicable = true,
    this.reason,
    this.committedBundles = const <String>[],
    this.deletedBytes = 0,
    this.archiveBytes = 0,
    this.interrupted = false,
    this.failed = false,
  });

  final bool applicable;
  final String? reason;
  final List<String> committedBundles;

                               
  final int deletedBytes;

                                                   
  final int archiveBytes;
  final bool interrupted;
  final bool failed;
}

class AuxArchiveTransaction {
  const AuxArchiveTransaction({required this.codec, this.canContinue});

  final DatabaseArchiveCodec codec;
  final AuxArchiveContinueCheck? canContinue;

  Future<AuxArchiveRunResult> archiveCapture(Directory captureDirectory) async {
    if (!codec.isSupported) {
      return const AuxArchiveRunResult(applicable: false, reason: 'codec');
    }
    if (!await _hasDurableFinalArtifacts(captureDirectory)) {
      return const AuxArchiveRunResult(applicable: false, reason: 'not_ready');
    }
    var manifest = await AuxArchiveManifest.read(captureDirectory);
    final committed = <String>[];
    var deleted = 0;
    var archived = 0;
    try {
      for (final bundle in <Future<_BundleOutcome> Function()>[
        () => _runSidecars(captureDirectory, manifest),
        () => _runDiagLog(captureDirectory, manifest),
      ]) {
        await _requireMayContinue();
        final outcome = await bundle();
        if (outcome.bundle == null) continue;
        manifest = (manifest ?? const AuxArchiveManifest(bundles: {}))
            .withBundle(outcome.id!, outcome.bundle!);
        await manifest.writeAtomic(captureDirectory);
                                       
        deleted += await _deleteSources(outcome.sources);
        archived += outcome.bundle!.archiveBytes;
        committed.add(outcome.id!);
      }
    } on DatabaseArchiveCancelled {
      return const AuxArchiveRunResult(interrupted: true);
    } catch (_) {
      return AuxArchiveRunResult(
        failed: true,
        committedBundles: committed,
        deletedBytes: deleted,
        archiveBytes: archived,
      );
    }
    return AuxArchiveRunResult(
      committedBundles: committed,
      deletedBytes: deleted,
      archiveBytes: archived,
    );
  }

                                                                    

  Future<_BundleOutcome> _runSidecars(
    Directory captureDirectory,
    AuxArchiveManifest? manifest,
  ) async {
    final directory = Directory(
      '${captureDirectory.path}/$_sidecarDirectoryName',
    );
    final present = <File>[];
    if (await directory.exists()) {
      await for (final entity in directory.list(followLinks: false)) {
        if (entity is File && entity.path.endsWith('.json')) {
          present.add(entity);
        }
      }
    }
    present.sort((left, right) => left.path.compareTo(right.path));
    final existing = manifest?.bundles[kAuxSidecarBundleId];
    if (present.isEmpty) return const _BundleOutcome.none();

                                         
                                  
    final entries = <AuxContainerEntry>[];
    final onDisk = <String>{
      for (final file in present)
        '$_sidecarDirectoryName/${_baseName(file.path)}',
    };
    if (existing != null) {
      for (final entry in await _restoreBundle(captureDirectory, existing)) {
        if (!onDisk.contains(entry.relativePath)) entries.add(entry);
      }
    }
    for (final file in present) {
      entries.add(
        AuxContainerEntry(
          relativePath: '$_sidecarDirectoryName/${_baseName(file.path)}',
          bytes: await file.readAsBytes(),
        ),
      );
    }
    return _commitBundle(
      captureDirectory: captureDirectory,
      id: kAuxSidecarBundleId,
      archiveFileName: kAuxSidecarArchiveFileName,
      entries: entries,
      sources: present,
    );
  }

                                                               

  Future<_BundleOutcome> _runDiagLog(
    Directory captureDirectory,
    AuxArchiveManifest? manifest,
  ) async {
    final source = File('${captureDirectory.path}/$kAuxDiagLogSourceName');
    var length = 0;
    try {
      length = await source.exists() ? await source.length() : 0;
    } on FileSystemException {
      length = 0;
    }
    if (length <= 0) return const _BundleOutcome.none();

                                              
                                            
                                 
    final entries = <AuxContainerEntry>[];
    final existing = manifest?.bundles[kAuxDiagLogBundleId];
    if (existing != null) {
      entries.addAll(await _restoreBundle(captureDirectory, existing));
    }
    entries.add(
      AuxContainerEntry(
        relativePath: _nextDiagGenerationPath(entries),
        bytes: await source.readAsBytes(),
      ),
    );
    return _commitBundle(
      captureDirectory: captureDirectory,
      id: kAuxDiagLogBundleId,
      archiveFileName: kAuxDiagLogArchiveFileName,
      entries: entries,
      sources: <File>[source],
    );
  }

  static String _nextDiagGenerationPath(List<AuxContainerEntry> existing) {
    if (existing.isEmpty) return kAuxDiagLogSourceName;
    var next = 1;
    for (final entry in existing) {
      final suffix = entry.relativePath.split('.').last;
      final parsed = int.tryParse(suffix);
      if (parsed != null && parsed >= next) next = parsed + 1;
    }
    return '$kAuxDiagLogSourceName.$next';
  }

                                                   

  Future<_BundleOutcome> _commitBundle({
    required Directory captureDirectory,
    required String id,
    required String archiveFileName,
    required List<AuxContainerEntry> entries,
    required List<File> sources,
  }) async {
    final archive = File('${captureDirectory.path}/$archiveFileName');
    final containerTemporary = File('${archive.path}.container.tmp');
    final archiveTemporary = File('${archive.path}.tmp');
    final verifyTemporary = File('${archive.path}.verify.tmp');
    final temporaries = <File>[
      containerTemporary,
      archiveTemporary,
      verifyTemporary,
    ];
    await _deleteAll(temporaries);
    try {
      final container = encodeAuxContainer(entries);
      final containerSha = sha256.convert(container).toString();
      await containerTemporary.writeAsBytes(container, flush: true);
      await _requireMayContinue();

      await codec.compress(
        sourceDatabase: containerTemporary,
        destinationArchive: archiveTemporary,
      );
      if (!await archiveTemporary.exists() ||
          await archiveTemporary.length() == 0) {
        throw const FileSystemException('ZPAQ encoder produced no output');
      }
      await _requireMayContinue();

      await codec.decompress(
        sourceArchive: archiveTemporary,
        destinationDatabase: verifyTemporary,
      );
      if (!await databaseArchiveFilesEqual(
        containerTemporary,
        verifyTemporary,
      )) {
        throw const FileSystemException(
          'ZPAQ reconstruction differs from the container bytes',
        );
      }
                                      
      final restored = decodeAuxContainer(await verifyTemporary.readAsBytes());
      if (restored.length != entries.length) {
        throw const FileSystemException('restored entry count mismatch');
      }
      final expected = <String, String>{
        for (final entry in entries)
          entry.relativePath: sha256.convert(entry.bytes).toString(),
      };
      final files = <AuxArchiveFile>[];
      var sourceBytes = 0;
      for (final entry in restored) {
        final digest = sha256.convert(entry.bytes).toString();
        if (expected[entry.relativePath] != digest) {
          throw FileSystemException(
            'entry digest mismatch',
            entry.relativePath,
          );
        }
        sourceBytes += entry.bytes.length;
        files.add(
          AuxArchiveFile(
            relativePath: entry.relativePath,
            bytes: entry.bytes.length,
            sha256: digest,
          ),
        );
      }
      await _requireMayContinue();

      final archiveBytes = await archiveTemporary.length();
      if (await archive.exists()) await archive.delete();
      await archiveTemporary.rename(archive.path);
      await _deleteAll(temporaries);
      return _BundleOutcome(
        id: id,
        bundle: AuxArchiveBundle(
          archiveFileName: archiveFileName,
          archiveBytes: archiveBytes,
          archiveSha256: await databaseArchiveSha256(archive),
          containerBytes: container.length,
          containerSha256: containerSha,
          sourceBytes: sourceBytes,
          files: files,
          verifiedAt: DateTime.now().toUtc().toIso8601String(),
        ),
        sources: sources,
      );
    } catch (_) {
      await _deleteAll(temporaries);
      rethrow;
    }
  }

                                       
                       
  Future<List<AuxContainerEntry>> _restoreBundle(
    Directory captureDirectory,
    AuxArchiveBundle bundle,
  ) async {
    final entries = await restoreAuxBundle(
      captureDirectory: captureDirectory,
      bundle: bundle,
      codec: codec,
    );
    if (entries == null) {
      throw const FileSystemException('existing aux archive failed to restore');
    }
    return entries;
  }

  Future<int> _deleteSources(List<File> sources) async {
    var deleted = 0;
    for (final file in sources) {
      try {
        if (!await file.exists()) continue;
        final bytes = await file.length();
        await _requireMayContinue();
        await file.delete();
        deleted += bytes;
      } on FileSystemException {
                                       
      }
    }
    return deleted;
  }

  Future<void> _requireMayContinue() async {
    final check = canContinue;
    if (check != null && !await check()) {
      throw const DatabaseArchiveCancelled();
    }
  }
}

class _BundleOutcome {
  const _BundleOutcome({
    required this.id,
    required this.bundle,
    required this.sources,
  });

  const _BundleOutcome.none()
    : id = null,
      bundle = null,
      sources = const <File>[];

  final String? id;
  final AuxArchiveBundle? bundle;
  final List<File> sources;
}

                                              
Future<List<AuxContainerEntry>?> restoreAuxBundle({
  required Directory captureDirectory,
  required AuxArchiveBundle bundle,
  required DatabaseArchiveCodec codec,
}) async {
  if (!codec.isSupported) return null;
  final archive = File('${captureDirectory.path}/${bundle.archiveFileName}');
  final temporary = File('${archive.path}.restore.tmp');
  try {
    if (!await archive.exists()) return null;
    if (await archive.length() != bundle.archiveBytes) return null;
    if (await databaseArchiveSha256(archive) != bundle.archiveSha256) {
      return null;
    }
    if (await temporary.exists()) await temporary.delete();
    await codec.decompress(
      sourceArchive: archive,
      destinationDatabase: temporary,
    );
    final bytes = await temporary.readAsBytes();
    if (bytes.length != bundle.containerBytes) return null;
    if (sha256.convert(bytes).toString() != bundle.containerSha256) return null;
    final entries = decodeAuxContainer(bytes);
    final expected = <String, AuxArchiveFile>{
      for (final file in bundle.files) file.relativePath: file,
    };
    if (entries.length != expected.length) return null;
    for (final entry in entries) {
      final want = expected[entry.relativePath];
      if (want == null || want.bytes != entry.bytes.length) return null;
      if (sha256.convert(entry.bytes).toString() != want.sha256) return null;
    }
    return entries;
  } catch (_) {
    return null;
  } finally {
    try {
      if (await temporary.exists()) await temporary.delete();
    } catch (_) {}
  }
}

String _baseName(String path) => path.split(Platform.pathSeparator).last;

Future<void> _deleteAll(Iterable<File> files) async {
  for (final file in files) {
    try {
      if (await file.exists()) await file.delete();
    } on FileSystemException {
                                  
    }
  }
}

Future<bool> _hasDurableFinalArtifacts(Directory captureDirectory) async {
  for (final name in const <String>[
    'official_sfm_sparse.ply',
    'official_sfm_sparse_meta.json',
  ]) {
    final file = File('${captureDirectory.path}/$name');
    try {
      if (!await file.exists() || await file.length() == 0) return false;
    } on FileSystemException {
      return false;
    }
  }
  return true;
}
