                                                         
                        
                                     
                                                           
                                                
                                                           
                                
   
                                                        
                                                      
                                          
                                   
                                                   
                                    
                                                            
                                                        
                        
                                      
                                              
                                                  
              
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:crypto/crypto.dart';
import 'package:flutter/foundation.dart';

import 'database_archive_policy.dart';
import 'database_prune_ffi.dart';

typedef DatabaseRecipeContinueCheck = FutureOr<bool> Function();

class DatabaseRecipeResult {
  const DatabaseRecipeResult({
    this.applicable = false,
    this.committed = false,
    this.deletedBytes = 0,
    this.paused = false,
    this.reason,
  });

  final bool applicable;
  final bool committed;

                           
  final int deletedBytes;
  final bool paused;
  final String? reason;
}

class DatabaseRecipeManifest {
  static const schema = 'pw_database_prune_v3';
  static const fileName = 'official_database_prune.json';

                           
                                          
                                                     
                                                                   
                                                            
                   
  static const preservedTablesAlways = <String>[
    'cameras',
    'images',
    'two_view_geometries',
  ];

  static List<String> preservedTablesFor({required bool dropRawMatches}) => [
    ...preservedTablesAlways,
    if (!dropRawMatches) 'matches',
  ];

  static Future<bool> exists(Directory captureDirectory) =>
      File('${captureDirectory.path}/$fileName').exists();

                                     
                                                 
                                                   
                                                       
  static Future<Map<String, dynamic>?> readAnyVersion(
    Directory captureDirectory,
  ) async {
    try {
      final f = File('${captureDirectory.path}/$fileName');
      if (!await f.exists()) return null;
      final json = jsonDecode(await f.readAsString()) as Map<String, dynamic>;
      return json['schema'] is String ? json : null;
    } catch (_) {
      return null;
    }
  }

  static Future<Map<String, dynamic>?> read(Directory captureDirectory) async {
    try {
      final f = File('${captureDirectory.path}/$fileName');
      if (!await f.exists()) return null;
      final json = jsonDecode(await f.readAsString()) as Map<String, dynamic>;
      if (json['schema'] != schema) return null;
      return json;
    } catch (_) {
      return null;
    }
  }
}

class DatabaseRecipeTransaction {
  const DatabaseRecipeTransaction({this.canContinue});

                                                    
                                         
                                                                 
                                          
                                                   
                                                                     
                                                      
                                   
  static const bool dropRawMatches = true;

                                            
                                                           
                                               
                                   
                                                       
                                                    
                                              
  static const bool enabled = true;

  final DatabaseRecipeContinueCheck? canContinue;

  Future<DatabaseRecipeResult> recipeCapture(Directory captureDirectory) async {
    if (!enabled) return const DatabaseRecipeResult(reason: 'gate_closed');
    return pruneCapture(captureDirectory);
  }

                                             
  Future<DatabaseRecipeResult> pruneCapture(
    Directory captureDirectory, {
    String? outputDbPath,

                                        
    bool? dropRawMatchesOverride,
  }) async {
    final dropMatches = dropRawMatchesOverride ?? dropRawMatches;
    if (!databasePruneSupported) {
      return const DatabaseRecipeResult(reason: 'platform');
    }
    try {
      final source = File(
        '${captureDirectory.path}/${DatabaseArchivePolicy.sourceFileName}',
      );
      if (!await source.exists()) {
        return const DatabaseRecipeResult(reason: 'no_source_db');
      }
      final manifestExists = await DatabaseRecipeManifest.exists(
        captureDirectory,
      );
      Map<String, dynamic>? priorManifest;
      if (manifestExists && outputDbPath == null) {
                                                            
                                      
        priorManifest = await DatabaseRecipeManifest.readAnyVersion(
          captureDirectory,
        );
        final currentSchema =
            priorManifest?['schema'] == DatabaseRecipeManifest.schema;
        final desc = await tableContentSha256(source.path, 'descriptors');
                                               
                                                           
        await _dropStaleSiblings(source);
        if (currentSchema && desc == _sha256OfNothing) {
          return const DatabaseRecipeResult(
            applicable: true,
            committed: true,
            reason: 'already_pruned',
          );
        }
                                                
                                 
      }
                                              
                                                        
                                                         
                                              
                                                        
                                
      if (await File('${source.path}-journal').exists()) {
        return const DatabaseRecipeResult(reason: 'db_not_cold_journal');
      }
      final wal = File('${source.path}-wal');
      if (await wal.exists() && await wal.length() != 0) {
        return const DatabaseRecipeResult(reason: 'db_not_cold_wal');
      }
      if (!await _canContinueNow()) {
        return const DatabaseRecipeResult(paused: true);
      }

      final sourceBytes = await source.length();
      final sourceSha = await _sha256Of(source);

                                                      
                                                       
                                     
      final work = File('${source.path}.work.tmp');
      await _deleteWithSiblings(work);
      await source.copy(work.path);

                                 
      final tables = DatabaseRecipeManifest.preservedTablesFor(
        dropRawMatches: dropMatches,
      );
      final pre = <String, String>{};
      for (final t in tables) {
        final d = await tableContentSha256(work.path, t);
        if (d == null) {
          await _deleteWithSiblings(work);
          return const DatabaseRecipeResult(reason: 'pre_digest_failed');
        }
        pre[t] = d;
      }
                                            
      final preXy = await keypointsXySha256(work.path);
      if (preXy == null) {
        await _deleteWithSiblings(work);
        return const DatabaseRecipeResult(reason: 'pre_xy_digest_failed');
      }
      if (!await _canContinueNow()) {
        await _deleteWithSiblings(work);
        return const DatabaseRecipeResult(paused: true);
      }

      final pruned = File(outputDbPath ?? '${source.path}.pruned.tmp');
      await _deleteWithSiblings(pruned);
      final rc = await pruneDescriptorsFile(
        work.path,
        pruned.path,
        dropRawMatches: dropMatches,
      );
      if (rc != 0 || !await pruned.exists()) {
        await _deleteWithSiblings(work);
        await _deleteWithSiblings(pruned);
        return DatabaseRecipeResult(reason: 'prune_failed_rc$rc');
      }

                                         
      for (final t in tables) {
        final d = await tableContentSha256(pruned.path, t);
        if (d == null || d != pre[t]) {
          await _deleteWithSiblings(work);
          await _deleteWithSiblings(pruned);
          return DatabaseRecipeResult(reason: 'post_digest_mismatch_$t');
        }
      }
      final postXy = await keypointsXySha256(pruned.path);
      if (postXy == null || postXy != preXy) {
        await _deleteWithSiblings(work);
        await _deleteWithSiblings(pruned);
        return const DatabaseRecipeResult(reason: 'keypoint_xy_mismatch');
      }
      if (dropMatches) {
        final m = await tableContentSha256(pruned.path, 'matches');
        if (m != _sha256OfNothing) {
          await _deleteWithSiblings(work);
          await _deleteWithSiblings(pruned);
          return const DatabaseRecipeResult(reason: 'matches_not_empty');
        }
      }
      final emptyDescriptors = await tableContentSha256(
        pruned.path,
        'descriptors',
      );
      if (emptyDescriptors != _sha256OfNothing) {
        await _deleteWithSiblings(work);
        await _deleteWithSiblings(pruned);
        return const DatabaseRecipeResult(reason: 'descriptors_not_empty');
      }
      final prunedBytes = await pruned.length();

                                                          
                                                     
      final sidecar = File('${source.path}$_poseSidecarSuffix');
      if (!await sidecar.exists()) {
        await _deleteWithSiblings(work);
        await _deleteWithSiblings(pruned);
        return const DatabaseRecipeResult(reason: 'pose_sidecar_missing');
      }
      final prunedSidecar = File('${pruned.path}$_poseSidecarSuffix');
      await sidecar.copy(prunedSidecar.path);
      final resealRc = await resealArkitPoseDigests(
        pruned.path,
        prunedSidecar.path,
      );
      if (resealRc != 0) {
        await _deleteWithSiblings(work);
        await _deleteWithSiblings(pruned);
        await _deleteIfPresent(prunedSidecar);
        return DatabaseRecipeResult(reason: 'reseal_failed_rc$resealRc');
      }

                                            
                                                  
                                    
      await _deleteWithSiblings(work);
      await _deleteSiblingsOnly(pruned);
      if (outputDbPath != null) {
                                          
        return DatabaseRecipeResult(
          applicable: true,
          committed: false,
          deletedBytes: sourceBytes - prunedBytes,
        );
      }

                                      
                                                       
                       
      final priorOriginalBytes = priorManifest?['original_db_bytes'];
      final priorOriginalSha = priorManifest?['original_db_sha256'];
      final upgraded = manifestExists;
      final manifest = <String, Object?>{
        'schema': DatabaseRecipeManifest.schema,
        'created_at': DateTime.now().toUtc().toIso8601String(),
        'original_db_bytes': priorOriginalBytes ?? sourceBytes,
        'original_db_sha256': priorOriginalSha ?? sourceSha,
        if (upgraded) 'upgraded_from_bytes': sourceBytes,
        'pruned_db_bytes': prunedBytes,
        'preserved_table_sha256': pre,
        'keypoints_xy_sha256': preXy,
        'deleted':
            'descriptors(匹配中间物;可从归档帧重提,语义档) + '
            'keypoints 仿射形状列 a11/a12/a21/a22(匹配期形状信息,重建只读 x,y)'
            '${dropMatches ? ' + matches 原始匹配表(已蒸馏成 TVG 内点)' : ''}',
        'drop_raw_matches': dropMatches,
        'rationale': 'b1-regen-ceiling-PROVEN: 匹配图不可从 q65 帧再生(轨迹-24%),必须保留',
      };
      return await _commitVerifiedPair(
        captureDirectory: captureDirectory,
        source: source,
        pruned: pruned,
        manifest: manifest,
        sourceBytes: sourceBytes,
        prunedBytes: prunedBytes,
      );
    } catch (e) {
      return DatabaseRecipeResult(reason: 'exception:$e');
    }
  }

                                                                             
                                                                             
  @visibleForTesting
  Future<DatabaseRecipeResult> commitVerifiedPairForTesting({
    required Directory captureDirectory,
    required File source,
    required File pruned,
    required Map<String, Object?> manifest,
    required int sourceBytes,
    required int prunedBytes,
  }) => _commitVerifiedPair(
    captureDirectory: captureDirectory,
    source: source,
    pruned: pruned,
    manifest: manifest,
    sourceBytes: sourceBytes,
    prunedBytes: prunedBytes,
  );

  Future<DatabaseRecipeResult> _commitVerifiedPair({
    required Directory captureDirectory,
    required File source,
    required File pruned,
    required Map<String, Object?> manifest,
    required int sourceBytes,
    required int prunedBytes,
  }) async {
    try {
      final manifestFile = File(
        '${captureDirectory.path}/${DatabaseRecipeManifest.fileName}',
      );
      {
                                               
        final tmp = File('${manifestFile.path}.tmp');
        await tmp.writeAsString(
          const JsonEncoder.withIndent(' ').convert(manifest),
        );
        await tmp.rename(manifestFile.path);
      }
      if (!await _canContinueNow()) {
                                                      
                                                 
                                           
        return const DatabaseRecipeResult(applicable: true, paused: true);
      }
                                             
                                      
      await File(
        '${pruned.path}$_poseSidecarSuffix',
      ).rename('${source.path}$_poseSidecarSuffix');
      await pruned.rename(source.path);
                                                           
                                    
      await _dropStaleSiblings(source);
      return DatabaseRecipeResult(
        applicable: true,
        committed: true,
        deletedBytes: sourceBytes - prunedBytes,                 
      );
    } catch (e) {
      return DatabaseRecipeResult(reason: 'exception:$e');
    }
  }

  Future<bool> _canContinueNow() async =>
      canContinue == null || await canContinue!();
}

                                                              
const _poseSidecarSuffix = '.arkit_pose_v1';

                                   
const _sha256OfNothing =
    'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855';

Future<String> _sha256Of(File file) async {
  final digest = await sha256.bind(file.openRead()).first;
  return digest.toString();
}

                                              
                                          
Future<void> _dropStaleSiblings(File db) async {
  try {
    if (await File('${db.path}-journal').exists()) return;
    final wal = File('${db.path}-wal');
    if (await wal.exists() && await wal.length() != 0) return;
    await _deleteIfPresent(wal);
    await _deleteIfPresent(File('${db.path}-shm'));
  } on FileSystemException {
                             
  }
}

                                     
Future<void> _deleteWithSiblings(File db) async {
  await _deleteIfPresent(db);
  await _deleteSiblingsOnly(db);
}

Future<void> _deleteSiblingsOnly(File db) async {
  for (final suffix in const <String>['-wal', '-shm', '-journal']) {
    await _deleteIfPresent(File('${db.path}$suffix'));
  }
}

Future<void> _deleteIfPresent(File file) async {
  try {
    if (await file.exists()) await file.delete();
  } on FileSystemException {
                          
  }
}
