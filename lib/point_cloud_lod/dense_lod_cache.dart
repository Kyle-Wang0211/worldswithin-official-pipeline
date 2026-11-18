                                                                                                    
                                                                                 
                         
  
                                                                          
                                                                                                    
                                                                                                 
                                                                                                
                                                                                                      
                                                                                                    
                                                                                           
  
                                                                                               
                                                                                                   
                                                                                                 
                                                                                                   
                                                                                                  
                                                                                                    
                   
  
                                                                               
                                                                                                  
                                                                                            
  
                                                                                                       
                                                                                                        
                                                                                                      
                                                                  
                                                                                                  
                                                                                                
                                                    
                                                                                                 
                                                                                                  
  
                                                                                                   
                                                                                                
                                                                                                     
                                                                    
import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart' show AppLifecycleState, WidgetsBinding, WidgetsBindingObserver;
import 'package:path_provider/path_provider.dart';

import '../official_util/device_log.dart';
import 'lod_bridge.dart';

/// Upstream algorithm revision. The separately identified API-contract derivative
                                                                            
const String kLodEngineSha8 = '72ee817f';
const int kLodEngineAbi = 3;
                                                                             
                                                                               
const String kLodEngineBuildIdentity =
    '$kLodEngineSha8+hardening-b8d2389b4d10 abi=$kLodEngineAbi';

enum DenseLodPhase { building, ready, failed }

@immutable
class DenseLodState {
  const DenseLodState._(this.phase, {this.octreeDir, this.message});
  const DenseLodState.building() : this._(DenseLodPhase.building);
  const DenseLodState.ready(String dir) : this._(DenseLodPhase.ready, octreeDir: dir);
  const DenseLodState.failed(String why) : this._(DenseLodPhase.failed, message: why);

  final DenseLodPhase phase;

                                     
  final String? octreeDir;
  final String? message;

  @override
  String toString() => 'DenseLodState($phase${octreeDir != null ? ' $octreeDir' : ''}${message != null ? ' $message' : ''})';
}

                                                                                    
                                                                                              
                                                                                                    
                                                                                                    
                                                                                                 
                                                                    
int? densePlyPoints(String path) {
  try {
    final f = File(path);
    if (!f.existsSync()) return null;
    final raf = f.openSync();
    try {
      final head = raf.readSync(4096);
      final text = latin1.decode(head, allowInvalid: true);
      const marker = 'end_header\n';
      final end = text.indexOf(marker);
      if (!text.startsWith('ply') || end < 0) return null;
      final m = RegExp(r'element vertex (\d+)').firstMatch(text.substring(0, end));
      if (m == null) return null;
      final n = int.parse(m.group(1)!);
      if (n <= 0) return null;
      return raf.lengthSync() == end + marker.length + kDensePlyBytesPerPoint * n ? n : null;
    } finally {
      raf.closeSync();
    }
  } catch (_) {
    return null;
  }
}

                         
bool densePlyComplete(String path) => densePlyPoints(path) != null;

                                                         
const int kDensePlyBytesPerPoint = 15;

class DenseLodCache {
  DenseLodCache({
    LodBridge? bridge,
    Future<Directory?> Function()? cacheRoot,
    DateTime Function() clock = DateTime.now,
  }) : _bridge = bridge ?? LodBridge(),
       _cacheRoot = cacheRoot ?? _defaultRoot,
       _clock = clock;

  static DenseLodCache _instance = DenseLodCache();

                                                                                          
  static DenseLodCache get instance => _instance;

                                                                 
  @visibleForTesting
  static set instanceForTesting(DenseLodCache c) => _instance = c;

  static const String kTreeDirName = 'lod';
  static const String kBuildingDirName = 'lod.building';
  static const String kStampName = 'pw_lod_source.json';

                                                                                
  static const String kLegacyCacheDirName = 'lod';
  static const String kChunksDirName = 'lod_chunks';

  final LodBridge _bridge;

                                                                                                     
  final Future<Directory?> Function() _cacheRoot;
  final DateTime Function() _clock;

  final Map<String, ValueNotifier<DenseLodState>> _states = {};

                                                                                             
                                                                                            
  final Map<String, String> _failedFor = {};
  Future<void>? _queue;                                                    

                                                                                                  
                                                                                             
                                                                                 
  @visibleForTesting
  Future<void> whenIdle() => _queue ?? Future<void>.value();

  static Future<Directory?> _defaultRoot() async {
    try {
      return await getApplicationCacheDirectory();
    } catch (_) {
      return null;
    }
  }

                                                                      
  static String workDirOf(String densePlyPath) => File(densePlyPath).parent.path;

                   
  static String treeDirOf(String densePlyPath) => '${workDirOf(densePlyPath)}/$kTreeDirName';

                                                                                   
                                                 
  static String keyFor(String plyPath) => File(plyPath).parent.path.split(Platform.pathSeparator).last;

                                                                               
  static int? treePointsOf(Directory tree) {
    try {
      final j = jsonDecode(File('${tree.path}/metadata.json').readAsStringSync());
      final p = j is Map ? j['points'] : null;
      return p is int ? p : (p is num && p == p.roundToDouble() ? p.toInt() : null);
    } catch (_) {
      return null;
    }
  }

                                                                                                
                                                                                                 
  static bool isValidTree(Directory tree, int plyPoints) {
    try {
      for (final f in ['metadata.json', 'hierarchy.bin', 'octree.bin']) {
        if (!File('${tree.path}/$f').existsSync()) return false;
      }
      final points = treePointsOf(tree);
      if (points == null || points <= 0 || points != plyPoints) return false;
      return File('${tree.path}/octree.bin').lengthSync() == kPwLodBytesPerPoint * points;
    } catch (_) {
      return false;
    }
  }

                                                                                                 
                                                                                                    
                                                                             
                                                                               
  Future<String?> findValid(String densePlyPath) async {
    try {
      final points = densePlyPoints(densePlyPath);
      if (points == null) return null;
      final tree = Directory(treeDirOf(densePlyPath));
      if (isValidTree(tree, points)) return tree.path;
      if (!tree.existsSync()) {
        final migrated = await _migrateLegacy(densePlyPath, points);
        if (migrated != null) return migrated;
      }
      return null;
    } catch (_) {
      return null;
    }
  }

                                                                                           
                                                                                    
  Future<String?> _migrateLegacy(String densePlyPath, int plyPoints) async {
    final root = await _cacheRoot();
    if (root == null) return null;
    final legacy = Directory('${root.path}/$kLegacyCacheDirName/${keyFor(densePlyPath)}');
    if (!legacy.existsSync()) return null;
    final target = treeDirOf(densePlyPath);
    if (!isValidTree(legacy, plyPoints)) {
      DeviceLog.log(
        'DenseLodCache',
        'legacy tree not migrated (points ${treePointsOf(legacy)} vs PLY $plyPoints or incomplete): ${legacy.path}',
      );
      return null;
    }
    try {
      legacy.renameSync(target);
      DeviceLog.log('DenseLodCache', 'migrated legacy tree ${legacy.path} → $target ($plyPoints pts, rename, no rebuild)');
      return target;
    } catch (e) {
      DeviceLog.log('DenseLodCache', 'legacy tree migration failed (left in place): $e');
      return null;
    }
  }

                                                                                                  
                                                                                       
                                                                                      
  void discardTree(String workDir) {
    final w = Directory(workDir);
    if (!w.existsSync()) return;
    final tree = Directory('$workDir/$kTreeDirName');
    if (tree.existsSync()) {
      final gone = '$workDir/$kTreeDirName.deleting.${_clock().millisecondsSinceEpoch}';
      try {
        tree.renameSync(gone);
        DeviceLog.log('DenseLodCache', 'dense re-run: old tree removed from $workDir');
      } catch (e) {
        DeviceLog.log('DenseLodCache', 'dense re-run: could not move the old tree aside: $e');
      }
    }
    _removeLeftovers(workDir);
                                                                 
    _states.remove('$workDir/official_dense.ply');
    _failedFor.remove('$workDir/official_dense.ply');
  }

                                                                                           
  static void _removeLeftovers(String workDir) {
    try {
      for (final e in Directory(workDir).listSync(followLinks: false)) {
        if (e is! Directory) continue;
        final name = e.path.split(Platform.pathSeparator).last;
        if (name.startsWith(kBuildingDirName) || name.startsWith('$kTreeDirName.deleting.')) {
          e.deleteSync(recursive: true);
        }
      }
    } catch (_) {}
  }

                                                                                                    
                                                                                                     
                                          
  ValueListenable<DenseLodState> watch(String densePlyPath) {
    final existing = _states[densePlyPath];
    if (existing != null) {
      if (existing.value.phase != DenseLodPhase.building) unawaited(_ensure(densePlyPath, existing));
      return existing;
    }
    final n = ValueNotifier<DenseLodState>(const DenseLodState.building());
    _states[densePlyPath] = n;
    unawaited(_ensure(densePlyPath, n));
    return n;
  }

  Future<void> _ensure(String ply, ValueNotifier<DenseLodState> out) {
    final run = (_queue ?? Future<void>.value()).then((_) async {
      try {
        out.value = await _ensureNow(ply, out);
      } catch (e) {
        DeviceLog.log('DenseLodCache', 'unexpected: $e');
        out.value = DenseLodState.failed('$e');
      }
    });
    _queue = run.catchError((Object _) {});
    return run;
  }

  Future<DenseLodState> _ensureNow(String ply, ValueNotifier<DenseLodState> out) async {
    final src = File(ply);
    if (!src.existsSync()) return _failed(ply, 'PLY missing');
    final stat = src.statSync();
                                                                                                  
    final plyPoints = densePlyPoints(ply);
    if (plyPoints == null) return _failed(ply, 'dense PLY incomplete or unreadable (header count does not fill the file)');
    final workDir = workDirOf(ply);
    final key = keyFor(ply);
    final identity = '${stat.size}|${stat.modified.millisecondsSinceEpoch}|$plyPoints';
    final found = await findValid(ply);
    if (found != null) return DenseLodState.ready(found);
    if (_failedFor[ply] == identity) {
      return DenseLodState.failed('previous build of this PLY failed (not retried)');
    }
    out.value = const DenseLodState.building();
    _removeLeftovers(workDir);
    final tree = Directory('$workDir/$kTreeDirName');
    final tmp = Directory('$workDir/$kBuildingDirName');
    final root = await _cacheRoot();
    final chunks = Directory(
      root != null ? '${root.path}/$kChunksDirName/$key' : '$workDir/$kBuildingDirName.chunks',
    );
    if (chunks.existsSync()) chunks.deleteSync(recursive: true);
    final started = _clock();
    final lifecycle = _LifecycleTally.start();
    DeviceLog.log(
      'DenseLodCache',
      'build start $key at ${started.toIso8601String()}: $plyPoints pts, ${stat.size} B, '
          'app ${lifecycle.stateAtStart}',
    );
    final build = await _bridge.buildFromPly(plyPath: ply, outDir: tmp.path, chunkDir: chunks.path);
    try {
      if (chunks.existsSync()) chunks.deleteSync(recursive: true);
    } catch (_) {}
    final c1 = judgeC1(build);
    final c1Ply = build.plyPoints == plyPoints;
    final checks = <LodCheck>[
      c1,
      LodCheck('C1-ply-header', c1Ply, 'dart header $plyPoints vs build ply_points ${build.plyPoints}'),
    ];
    LodVerifyReport? verify;
    if (c1.pass && c1Ply) {
      verify = await _bridge.verifyOctree(octreeDir: tmp.path);
      checks.addAll(judgeVerify(verify, build: build));
    }
    final pass = verify != null && checks.every((c) => c.pass) && isValidTree(tmp, plyPoints);
    final finished = _clock();
    final bg = lifecycle.stop();
                                                                                                 
                                                                                                
                                                                                              
    DeviceLog.log(
      'DenseLodCache',
      'build timing $key: start ${started.toIso8601String()} end ${finished.toIso8601String()} '
          'wall ${finished.difference(started).inMilliseconds} ms, engine ${build.elapsedMs.toStringAsFixed(0)} ms, '
          'peak ${build.peakFootprintMb.toStringAsFixed(0)} MB (baseline ${build.baselineFootprintMb.toStringAsFixed(0)} MB), '
          'background during build: ${bg.wentToBackground ? 'YES' : 'no'} '
          '(${bg.backgroundEntries}×, ${bg.backgroundMs} ms${bg.observed ? '' : ', lifecycle not observable'})',
    );
    final detail = checks.where((c) => !c.pass).map((c) => '${c.name}: ${c.detail}').join('; ');
    if (!pass) {
      try {
        if (tmp.existsSync()) tmp.deleteSync(recursive: true);
      } catch (_) {}
      _failedFor[ply] = identity;
      return _failed(ply, 'tree rejected (${build.status} ${build.error}) $detail');
    }
    File('${tmp.path}/$kStampName').writeAsStringSync(
      const JsonEncoder.withIndent('  ').convert(<String, Object?>{
        'schema': 'pw_lod_source/2',
        'engine': kLodEngineBuildIdentity,
        'points': plyPoints,
        'started': started.toIso8601String(),
        'finished': finished.toIso8601String(),
        'background_during_build': bg.toJson(),
        'build': build.toJson(),
        'verify': verify.toJson(),
        'checks': [for (final c in checks) c.toJson()],
      }),
      flush: true,
    );
    if (tree.existsSync()) {
      final gone = Directory('$workDir/$kTreeDirName.deleting.${finished.millisecondsSinceEpoch}');
      tree.renameSync(gone.path);
      gone.deleteSync(recursive: true);
    }
    tmp.renameSync(tree.path);
    DeviceLog.log(
      'DenseLodCache',
      'build ok $key: ${build.treePoints} pts, ${build.nodes} nodes, ${build.elapsedMs.toStringAsFixed(0)} ms, '
          'peak ${build.peakFootprintMb.toStringAsFixed(0)} MB, leaves ${verify.leavesSelected}/${verify.leaves} → ${tree.path}',
    );
    return DenseLodState.ready(tree.path);
  }

  DenseLodState _failed(String ply, String why) {
    DeviceLog.log('DenseLodCache', 'no tree for $ply (flat display stays): $why');
    return DenseLodState.failed(why);
  }
}

                                                                                                  
                                                                                                  
class _LifecycleTally with WidgetsBindingObserver {
  _LifecycleTally._(this.observed, this.stateAtStart);

  factory _LifecycleTally.start() {
    try {
      final b = WidgetsBinding.instance;
      final t = _LifecycleTally._(true, b.lifecycleState?.name ?? 'unknown');
      if (b.lifecycleState == AppLifecycleState.paused || b.lifecycleState == AppLifecycleState.hidden) {
        t._enter();
      }
      b.addObserver(t);
      return t;
    } catch (_) {
      return _LifecycleTally._(false, 'unknown');
    }
  }

  final bool observed;
  final String stateAtStart;
  int backgroundEntries = 0;
  int backgroundMs = 0;
  DateTime? _since;

  bool get wentToBackground => backgroundEntries > 0;

  void _enter() {
    if (_since != null) return;
    backgroundEntries++;
    _since = DateTime.now();
  }

  void _leave() {
    final t = _since;
    if (t == null) return;
    backgroundMs += DateTime.now().difference(t).inMilliseconds;
    _since = null;
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused || state == AppLifecycleState.hidden) {
      _enter();
    } else if (state == AppLifecycleState.resumed) {
      _leave();
    }
  }

  _LifecycleTally stop() {
    _leave();
    if (observed) {
      try {
        WidgetsBinding.instance.removeObserver(this);
      } catch (_) {}
    }
    return this;
  }

  Map<String, Object> toJson() => <String, Object>{
    'observed': observed,
    'state_at_start': stateAtStart,
    'went_to_background': wentToBackground,
    'background_entries': backgroundEntries,
    'background_ms': backgroundMs,
  };
}
