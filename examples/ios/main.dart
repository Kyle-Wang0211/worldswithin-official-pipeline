import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:pw_shared_viewer/pw_shared_viewer.dart';

import 'package:worldswithin_flutter/dense/dense_stage_progress.dart';
import 'package:worldswithin_flutter/dense/native_dense_stage_launcher.dart';
import 'package:worldswithin_flutter/dense/reconstruction_capture_preparer_adapter.dart';
import 'package:worldswithin_flutter/dense/reconstruction_final_record_artifact.dart';
import 'package:worldswithin_flutter/dense/reconstruction_production_launcher.dart';
import 'package:worldswithin_flutter/dense/reconstruction_production_package.dart';
import 'package:worldswithin_flutter/dense/reconstruction_shared_mainline_runner.dart';
import 'package:worldswithin_flutter/me/scan_record_store.dart';
import 'package:worldswithin_flutter/me/work_identity.dart';
import 'package:worldswithin_flutter/official_aether_sfm_ffi.dart' show AetherProcessEnv;
import 'package:worldswithin_flutter/official_capture/dense_stage.dart';
import 'package:worldswithin_flutter/official_capture/photo_archive_runtime.dart';
import 'package:worldswithin_flutter/official_capture/sfm_resume.dart';
import 'package:worldswithin_flutter/point_cloud_lod/dense_lod_cache.dart' show densePlyPoints;
import 'package:worldswithin_flutter/reconstruction/execution_identity_scope.dart';
import 'package:worldswithin_flutter/reconstruction/final_artifact_completion.dart';
import 'package:worldswithin_flutter/ui/scan_record.dart';
import 'package:worldswithin_flutter/ui/shared/product_preview_cover.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(
    MaterialApp(
      title: 'WorldsWithin Official Pipeline',
      theme: ThemeData(
        colorScheme: const ColorScheme.light(
          primary: Colors.black,
          secondary: Colors.black54,
          surface: Colors.white,
        ),
      ),
      home: const LocalReconstructionPage(),
    ),
  );
}

class LocalReconstructionPage extends StatefulWidget {
  const LocalReconstructionPage({super.key});

  @override
  State<LocalReconstructionPage> createState() =>
      _LocalReconstructionPageState();
}

class _LocalReconstructionPageState extends State<LocalReconstructionPage> {
  SharedViewerController? _viewer;
  ReconstructionFinalArtifactCompletionProof? _artifact;
  final _covers = ProductPreviewCoverWriter();
  bool _running = false;
  String _status = 'Place the capture in Files → WorldsWithin → Input.';
  ScanRecordStore? _store;
  String? _runId;

  @override
  void initState() {
    super.initState();
    denseStageProgress.addListener(_showProgress);
    unawaited(_prepareInputFolder());
  }

  Future<void> _prepareInputFolder() async {
    try {
      final documents = await getApplicationDocumentsDirectory();
      await Directory('${documents.path}/Input').create(recursive: true);
    } catch (error) {
      _showStatus('Cannot open local input folder: $error');
    }
  }

  @override
  void dispose() {
    denseStageProgress.removeListener(_showProgress);
    _viewer?.dispose();
    super.dispose();
  }

  void _showStatus(String status) {
    if (mounted) setState(() => _status = status);
  }

  void _showProgress() {
    final progress = denseStageProgress.value;
    if (progress == null || !_running) return;
    _showStatus(
      progress.message ??
          '${progress.phase} ${progress.done}/${progress.total}',
    );
  }

  Future<void> _reconstruct() async {
    if (_running) return;
    setState(() {
      _running = true;
      _artifact = null;
      _status = 'Preparing the local capture…';
    });
    _viewer?.dispose();
    _viewer = null;
    try {
      if (!Platform.isIOS)
        throw UnsupportedError('This entry uses the iOS native runtime.');
      final documents = Directory(
        await (await getApplicationDocumentsDirectory()).resolveSymbolicLinks(),
      );
      final workspace = await Directory(
        '${documents.path}/worldswithin_pipeline',
      ).create(recursive: true);
      final bundle = await _nativeBundle();
      final scope = await ReconstructionExecutionIdentityScope.independentTest(
        bundleRoot: bundle,
        documentsDirectory: workspace,
      );
      final package = await ReconstructionProductionPackage.load(
        bundleRoot: bundle,
        identityScope: scope,
      );
      if (package == null)
        throw StateError('The local runtime package is missing.');
      final store = ScanRecordStore.instance;
      await store.ensureLoaded();
      final id = newCaptureWorkId();
      _store = store;
      _runId = id;
      final capture = await store.captureDirFor(
        id,
        pipelineKind: CapturePipelineKind.official,
      );
      final count = await _copyCapture(
        Directory('${documents.path}/Input'),
        capture,
      );
      await store.insertNew(
        ScanRecord(
          id: id,
          name: id,
          accountNamespace: store.workspace.namespace,
          createdAt: DateTime.now(),
          pipelineKind: CapturePipelineKind.official,
          captureDir: capture.path,
          photosDir: '${capture.path}/photos_highres',
          captureManifestPath:
              File('${capture.path}/official_photo_bundle.json').existsSync()
              ? '${capture.path}/official_photo_bundle.json'
              : null,
          photoCount: count,
        ),
      );

      AetherProcessEnv.set('OFFICIAL_AETHER_PYR_PERSIST', '0');
      await officialArchiveBackgroundRuntime.initialize();
      _showStatus('Reconstructing camera poses…');
      final sparse = await rebuildFromArchivedPhotos(
        capture.path,
        store: store,
      );
      if (!sparse.ok || sparse.fed != count) {
        throw StateError(
          sparse.failure ??
              'SfM did not accept all input photos. Check the camera sidecars.',
        );
      }
      final ply = '${capture.path}/official_sfm_sparse.ply';
      final points = densePlyPoints(ply);
      if (points == null || points == 0)
        throw StateError('SfM produced no sparse points.');
      final launcher = ReconstructionProductionLauncher(
        store: store,
        identityScope: scope,
        inputGatherer: NativeDenseStageLauncher.forCaptureInputs(),
        runner: ReconstructionSharedMainlineRunner(
          identityScope: scope,
          prepareCapture: ReconstructionCapturePreparerAdapter(package).call,
          isAvailable: true,
        ),
        onVerifiedBeforeRetirement: (_) async {
          final record = store.byId(id)!;
          final artifact = await verifyReconstructionFinalRecordArtifact(store, record);
          if (mounted) {
            setState(() {
              _artifact = artifact;
              _viewer = SharedViewerController();
            });
          }
        },
      );
      final started = await launcher.start(
        DenseStageRequest(
          captureDir: capture.path,
          sparsePlyPath: ply,
          pointCount: points,
          recordId: id,
        ),
      );
      if (started.status != DenseStageStatus.started) {
        throw StateError(
          started.message ?? 'Dense reconstruction did not start.',
        );
      }
      await launcher.completion;
      final progress = denseStageProgress.value;
      if (progress?.state != DenseStageState.done) {
        throw StateError(
          progress?.message ?? 'Dense reconstruction did not finish.',
        );
      }
      final record = store.byId(id)!;
      _showStatus(
        record.captureDir == null
            ? 'GLB and cover saved to scans_official.'
            : 'GLB saved. The cover and working copy are still being finalized.',
      );
    } catch (error) {
      _showStatus('Reconstruction stopped: $error');
    } finally {
      if (mounted) setState(() => _running = false);
    }
  }

  Future<void> _openModel(SharedViewerController viewer) async {
    final artifact = _artifact;
    final store = _store;
    final id = _runId;
    if (artifact == null || store == null || id == null) return;
    try {
      await viewer.loadLocalFile(
        artifact.publishedGlb,
        format: SharedModelFormat.glb,
      );
      final record = store.byId(id);
      final capture = record?.captureDir;
      if (!mounted ||
          !identical(_viewer, viewer) ||
          record == null ||
          capture == null)
        return;
      final source = ProductPreviewCoverSource(
        accountNamespace: record.accountNamespace,
        workId: id,
        captureDir: capture,
        runId: id,
        stage: 'glb',
        artifactIdentity: '${artifact.publishedGlb}:${artifact.sha256Hex}',
      );
      bool isCurrent() =>
          mounted &&
          identical(_viewer, viewer) &&
          store.byId(id)?.finalGlbSha256 == artifact.sha256Hex;
      _covers.select(source);
      final cover = await _covers.capture(
        source: source,
        controller: viewer,
        isCurrent: isCurrent,
        attach: (path) => store.attachPreviewThumbnail(
          expected: record,
          thumbnailPath: path,
          isCurrent: isCurrent,
        ),
      );
      if (cover == null && isCurrent())
        _showStatus('GLB saved. The cover could not be captured.');
    } catch (error) {
      _showStatus('Cannot show the local GLB: $error');
    }
  }

  @override
  Widget build(BuildContext context) {
    final viewer = _viewer;
    return Scaffold(
      appBar: AppBar(title: const Text('WorldsWithin Official Pipeline')),
      body: SafeArea(
        child: Column(
          children: [
            if (viewer != null)
              Expanded(
                child: SharedViewer(
                  key: ObjectKey(viewer),
                  controller: viewer,
                  onReady: () => unawaited(_openModel(viewer)),
                  onError: (error) => _showStatus('Viewer error: $error'),
                ),
              )
            else
              const Spacer(),
            Padding(
              padding: const EdgeInsets.all(16),
              child: Text(_status, textAlign: TextAlign.center),
            ),
            Padding(
              padding: const EdgeInsets.only(bottom: 16),
              child: FilledButton(
                onPressed: _running ? null : _reconstruct,
                child: const Text('Reconstruct local capture'),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

Future<Directory> _nativeBundle() async {
  var candidate = File(Platform.resolvedExecutable).parent;
  while (true) {
    if (await File('${candidate.path}/reconstruction_production/PACKAGE.json').exists()) {
      return Directory(await candidate.resolveSymbolicLinks());
    }
    final parent = candidate.parent;
    if (parent.path == candidate.path) {
      throw StateError(
        'Reconstruction runtime resources are missing from the app bundle.',
      );
    }
    candidate = parent;
  }
}

Future<int> _copyCapture(Directory input, Directory capture) async {
  final photos = Directory('${input.path}/photos_highres');
  if (!await photos.exists() ||
      await photos.resolveSymbolicLinks() != photos.path) {
    throw StateError(
      'Add an Input/photos_highres folder with JPEGs and camera sidecars.',
    );
  }
  final target = Directory('${capture.path}/photos_highres');
  await target.create(recursive: true);
  var count = 0;
  await for (final entry in photos.list(followLinks: false)) {
    if (entry is! File) continue;
    final name = entry.uri.pathSegments.last;
    if (!RegExp(r'\.(?:jpg|json)$', caseSensitive: false).hasMatch(name))
      continue;
    await entry.copy('${target.path}/$name');
    if (RegExp(r'\.jpg$', caseSensitive: false).hasMatch(name)) count++;
  }
  if (count == 0) throw StateError('No JPEGs found in Input/photos_highres.');
  final manifest = File('${input.path}/official_photo_bundle.json');
  if (await FileSystemEntity.type(manifest.path, followLinks: false) ==
      FileSystemEntityType.file) {
    await manifest.copy('${capture.path}/official_photo_bundle.json');
  }
  return count;
}
