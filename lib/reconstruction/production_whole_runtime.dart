import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'execution_identity_scope.dart';
import 'memory_policy.dart';

import 'native_downstream.dart' show Arena;
import 'production_runtime_binding.dart' show applyProductionRuntimeEnvironment;
import 'request.dart' show hashFile, ReconstructionIoTiming;

import 'package:worldswithin_flutter/reconstruction/vendor/dart_pipeline/lib/reconstruction_geometry_ffi.dart'
    show ReconstructionSystemMemory;
import 'package:worldswithin_flutter/reconstruction/vendor/dart_pipeline/lib/reconstruction_pipeline.dart'
    show checkedRelativePath;

typedef _BindN = Int32 Function(Pointer<Char>, Pointer<Char>, UintPtr);
typedef _BindD = int Function(Pointer<Char>, Pointer<Char>, int);
typedef _StartN = Int32 Function(Pointer<Char>, Uint32, Uint64);
typedef _StartD = int Function(Pointer<Char>, int, int);
typedef _BoundaryN = Int32 Function(Pointer<Char>);
typedef _BoundaryD = int Function(Pointer<Char>);
typedef _StopN = Int32 Function();
typedef _StopD = int Function();

const _intervalMs = 100;
const _traceName = 'whole_app_m0.jsonl';

                                                                              
                                                                             
final _ownedObservers =
    <String, ({_StopD stop, _BoundaryD boundary, Map<String, dynamic> metadata})>{};

void _observerEvent(String event, Map<String, dynamic> metadata,
    [Map<String, Object?> details = const {}]) {
  ReconstructionIoTiming.diagnostic('RECONSTRUCTION_OBSERVER_EVENT_V1', event, {
    'trace_path': metadata['m0_trace_path'],
    'native_run_id': metadata['m0_run_id'],
    'cap_B': metadata['m0_cap_B'],
    'start_rc': metadata['m0_start_return_code'],
    'stop_rc': metadata['m0_stop_return_code'],
    ...details,
  });
}

String _observerOwnershipKey(Map<String, dynamic> metadata) {
  final observer = _map(metadata['observer_identity'], 'owned M0');
  return jsonEncode([
    metadata['m0_pid'],
    metadata['m0_run_id'],
    metadata['m0_trace_path'],
    metadata['m0_cap_B'],
    observer['absolute_path'],
    observer['sha256'],
  ]);
}

                                                                               
final class WholeProductionRuntimeFailure implements Exception {
  WholeProductionRuntimeFailure(
    this.stage,
    this.returnCode,
    this.message, {
    this.observerStopReturnCode,
    this.metadata,
  });
  final String stage;
  final int? returnCode;
  final String message;
  final int? observerStopReturnCode;
  final Map<String, dynamic>? metadata;
  @override
  String toString() =>
      'WholeProductionRuntimeFailure(stage=$stage, '
      'rc=$returnCode, observer_stop_rc=$observerStopReturnCode): $message';
}

Map<String, dynamic> _map(Object? row, String label) {
  if (row is! Map) throw StateError('Missing whole runtime $label');
  return Map<String, dynamic>.from(row);
}

Future<Directory> _root(String path) async {
  if (!path.startsWith('/') || path.contains('\u0000')) {
    throw StateError('Absolute canonical directory required');
  }
  final root = Directory(path);
  if (await root.resolveSymbolicLinks() != path) {
    throw StateError('Noncanonical whole runtime directory: $path');
  }
  return root;
}

Future<Map<String, dynamic>> _identity(
  Map<String, dynamic> row,
  Directory root,
  String label,
) async {
  final relative = checkedRelativePath(row['path'] as String);
  final file = File('${root.path}/$relative');
  final sha = row['sha256'];
  if (row['bytes'] is! int ||
      (row['bytes'] as int) <= 0 ||
      sha is! String ||
      !RegExp(r'^[0-9a-f]{64}$').hasMatch(sha) ||
      await FileSystemEntity.type(file.path, followLinks: false) !=
          FileSystemEntityType.file ||
      await file.resolveSymbolicLinks() != file.path ||
      await file.length() != row['bytes'] ||
      await hashFile(file) != sha) {
    throw StateError(
      'Whole runtime $label canonical bytes/SHA mismatch: $relative',
    );
  }
  final receipt = row['receipt'];
  if (receipt != null && receipt is! Map) {
    throw StateError('Whole runtime $label receipt requires path/bytes/sha256');
  }
  return {
    'path': relative,
    'absolute_path': file.path,
    'bytes': row['bytes'],
    'sha256': sha,
    if (receipt != null)
      'receipt': await _identity(
        _map(receipt, '$label receipt'),
        root,
        '$label receipt',
      ),
  };
}

String _text(Pointer<Char> pointer, int bound) {
  final bytes = pointer.cast<Uint8>().asTypedList(bound);
  final end = bytes.indexOf(0);
  if (end < 0) return utf8.decode(bytes, allowMalformed: true);
  return utf8.decode(bytes.sublist(0, end), allowMalformed: true);
}

Future<Map<String, dynamic>> _firstRow(File trace, int capBytes) async {
                                                                            
  final input = await trace.open();
  try {
    final bytes = await input.read(16384);
    final end = bytes.indexOf(10);
    if (end < 0) throw StateError('Missing bounded M0 start row');
    final row = _map(
      jsonDecode(utf8.decode(bytes.sublist(0, end))),
      'M0 start',
    );
    final run = row['run_id'];
    if (row['schema'] != 'reconstruction.m0.memory/1' ||
        row['event'] != 'start' ||
        row['seq'] != 1 ||
        row['pid'] != pid ||
        run is! String ||
        !RegExp('^$pid-[0-9]+\u0024').hasMatch(run) ||
        row['interval_ms'] != _intervalMs ||
        row['cap_B'] != capBytes ||
        row['receipt_rc'] != 0) {
      throw StateError('M0 start identity/context mismatch');
    }
    return row;
  } finally {
    await input.close();
  }
}

void _worker() {
  if (Isolate.current.debugName == 'main') {
    throw StateError('Whole production runtime requires reconstruction worker');
  }
}

                                                                           
List<int> releaseOwnedWholeProductionObservers() {
  _worker();
  final owners = _ownedObservers.values.toList(growable: false);
  _ownedObservers.clear();
  final results = <int>[];
  for (final owner in owners) {
    _observerEvent('stop_requested', owner.metadata, {'reason': 'worker_exit'});
    final rc = owner.stop();
    owner.metadata['m0_stop_return_code'] = rc;
    _observerEvent('stop_returned', owner.metadata, {'reason': 'worker_exit'});
    results.add(rc);
  }
  return results;
}

                                                                            
                                                                           
                                                                              
                                                                                 
Future<Map<String, dynamic>> initializeWholeProductionRuntime({
  required Map<String, dynamic> profile,
  required String bundleRoot,
  required String runRoot,
  ReconstructionExecutionIdentityScope identityScope =
      ReconstructionExecutionIdentityScope.production,
}) async {
  _worker();
  requireWholeMemoryPolicy(profile, identityScope);
  final capBytes = wholeMemoryCapBytes(identityScope);
  final bundle = await _root(bundleRoot);
  final run = await _root(runRoot);
  final bindings = _map(profile['bindings'], 'bindings');
  final runtime = await _identity(
    _map(bindings['runtime'], 'runtime'),
    bundle,
    'runtime',
  );
  final producer = await _identity(
    _map(bindings['producer'], 'producer'),
    bundle,
    'producer',
  );
  final observer = await _identity(_map(bindings['M0'], 'M0'), bundle, 'M0');
  final trace = File('${run.path}/$_traceName');
  if (await FileSystemEntity.type(trace.path, followLinks: false) !=
      FileSystemEntityType.notFound) {
    throw StateError('M0 requires fresh trace; existing evidence retained');
  }
  final environment = applyProductionRuntimeEnvironment(
    bundleRoot: bundle.path,
    identityScope: identityScope,
  );
  final library = DynamicLibrary.open(observer['absolute_path'] as String);
  final start = library.lookupFunction<_StartN, _StartD>(
    'reconstruction_m0_observer_start',
  );
  final boundary = library.lookupFunction<_BoundaryN, _BoundaryD>(
    'reconstruction_m0_observer_boundary',
  );
  final stop = library.lookupFunction<_StopN, _StopD>('reconstruction_m0_observer_stop');
  final arena = Arena(ReconstructionSystemMemory(DynamicLibrary.process()));
  var ownsObserver = false;
  final metadata = <String, dynamic>{
    'm0_trace_path': trace.path,
    'm0_interval_ms': _intervalMs,
    'm0_cap_B': capBytes,
    'runtime_identity': runtime,
    'producer_identity': producer,
    'observer_identity': observer,
    'process_environment': environment,
    'native_entry': 'pw_feature_runtime_bind_v1',
    'classification': 'WHOLE_PRODUCTION_INITIALIZATION_ONLY',
    'full_pipeline_complete': false,
  };
  try {
    _observerEvent('start_requested', metadata);
    final startRc = start(arena.path(trace.path), _intervalMs, capBytes);
    metadata['m0_start_return_code'] = startRc;
    _observerEvent('start_returned', metadata, {'ownership_acquired': startRc == 0});
    if (startRc != 0) {
      throw WholeProductionRuntimeFailure(
        'observer_start',
        startRc,
        'No sampler ownership acquired',
        metadata: metadata,
      );
    }
    ownsObserver = true;
    final row = await _firstRow(trace, capBytes);
    metadata['m0_run_id'] = row['run_id'];
    metadata['m0_pid'] = row['pid'];
                                                                               
    final producerLibrary = DynamicLibrary.open(
      producer['absolute_path'] as String,
    );
    final bind = producerLibrary.lookupFunction<_BindN, _BindD>(
      'pw_feature_runtime_bind_v1',
    );
    final error = arena.bytes(4096).cast<Char>();
    final rc = bind(
      arena.path(runtime['absolute_path'] as String),
      error,
      4096,
    );
    metadata['return_code'] = rc;
    if (rc != 0) {
      throw WholeProductionRuntimeFailure(
        'runtime_bind',
        rc,
        _text(error, 4096),
        metadata: metadata,
      );
    }
    final boundaryRc = boundary(arena.path('/whole_runtime_initialized'));
    metadata['m0_initialize_boundary_return_code'] = boundaryRc;
    if (boundaryRc != 0) {
      throw WholeProductionRuntimeFailure(
        'observer_boundary',
        boundaryRc,
        'Whole-App observation boundary rejected',
        metadata: metadata,
      );
    }
    _ownedObservers[_observerOwnershipKey(metadata)] = (
      stop: stop,
      boundary: boundary,
      metadata: metadata,
    );
    _observerEvent('initialized', metadata);
    return metadata;
  } catch (error, stack) {
    _observerEvent('initialization_failed', metadata, {
      'error': error.toString(),
      'stack': stack.toString(),
    });
    if (ownsObserver) {
      _observerEvent('stop_requested', metadata, {'reason': 'initialization_failed'});
    }
    final stopRc = ownsObserver ? stop() : null;
    if (stopRc != null) {
      metadata['m0_stop_return_code'] = stopRc;
      _observerEvent('stop_returned', metadata, {'reason': 'initialization_failed'});
    }
    if (error is WholeProductionRuntimeFailure) {
      throw WholeProductionRuntimeFailure(
        error.stage,
        error.returnCode,
        error.message,
        observerStopReturnCode: stopRc,
        metadata: metadata,
      );
    }
    throw WholeProductionRuntimeFailure(
      'initialization',
      null,
      error.toString(),
      observerStopReturnCode: stopRc,
      metadata: metadata,
    );
  } finally {
    arena.dispose();
  }
}

                                                                               
                                                                                
Future<Map<String, dynamic>> stopWholeProductionObserver({
  required Map<String, dynamic> metadata,
  required Map<String, dynamic> profile,
  required String bundleRoot,
  ReconstructionExecutionIdentityScope identityScope =
      ReconstructionExecutionIdentityScope.production,
}) async {
  _worker();
  requireWholeMemoryPolicy(profile, identityScope);
  final capBytes = wholeMemoryCapBytes(identityScope);
  if (metadata['m0_cap_B'] != capBytes) {
    throw StateError('M0 stop policy differs from execution scope');
  }
  final bundle = await _root(bundleRoot);
  final bindings = _map(profile['bindings'], 'bindings');
  final observer = await _identity(_map(bindings['M0'], 'M0'), bundle, 'M0');
  final previous = _map(metadata['observer_identity'], 'owned M0');
  if (observer['absolute_path'] != previous['absolute_path'] ||
      observer['sha256'] != previous['sha256'] ||
      metadata['m0_pid'] != pid ||
      metadata['m0_start_return_code'] != 0 ||
      metadata['m0_stop_return_code'] != null) {
    throw StateError('M0 stop ownership mismatch or already stopped');
  }
  final path = metadata['m0_trace_path'] as String;
  final trace = File(path);
  final ownershipKey = _observerOwnershipKey(metadata);
  final owner = _ownedObservers[ownershipKey];
  if (owner == null) {
    throw StateError('M0 stop requires its active worker ownership');
  }
  final arena = Arena(ReconstructionSystemMemory(DynamicLibrary.process()));
                                                                               
  _ownedObservers.remove(ownershipKey);
  int? boundaryRc;
  late int stopRc;
  _observerEvent('stop_requested', metadata, {'reason': 'pipeline_finish_or_failure'});
  try {
    if (!path.startsWith('/') ||
        await trace.resolveSymbolicLinks() != path ||
        (await _firstRow(trace, capBytes))['run_id'] != metadata['m0_run_id']) {
      throw StateError('M0 current trace/run identity mismatch');
    }
    boundaryRc = owner.boundary(arena.path('/whole_pipeline_finished'));
    metadata['m0_finish_boundary_return_code'] = boundaryRc;
    _observerEvent('finish_boundary_returned', metadata, {'boundary_rc': boundaryRc});
  } catch (error, stack) {
    _observerEvent('stop_validation_failed', metadata, {
      'error': error.toString(),
      'stack': stack.toString(),
    });
    rethrow;
  } finally {
    try {
      stopRc = owner.stop();
      metadata['m0_stop_return_code'] = stopRc;
      _observerEvent('stop_returned', metadata, {'reason': 'pipeline_finish_or_failure'});
    } finally {
      arena.dispose();
    }
  }
  final result = <String, dynamic>{
    ...metadata,
    'm0_trace_bytes': await trace.length(),
    'm0_trace_sha256': await hashFile(trace),
  };
  if (boundaryRc != 0 || stopRc != 0) {
    throw WholeProductionRuntimeFailure(
      stopRc != 0 ? 'observer_stop' : 'observer_finish_boundary',
      stopRc != 0 ? stopRc : boundaryRc,
      'boundary_rc=$boundaryRc; stop_rc=$stopRc',
      observerStopReturnCode: stopRc,
      metadata: result,
    );
  }
  return result;
}
