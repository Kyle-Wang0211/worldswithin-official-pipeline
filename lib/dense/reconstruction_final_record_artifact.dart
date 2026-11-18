import 'dart:io';

import '../me/scan_record_store.dart';
import '../reconstruction/final_artifact_completion.dart';
import '../ui/scan_record.dart';

bool hasReconstructionFinalRecordProof(ScanRecord record) =>
    record.finalGlbSha256 != null || record.finalGlbBytes != null;

String? _recordFilePath(String? reference) {
  if (reference == null) return null;
  final uri = Uri.tryParse(reference);
  if (uri != null && uri.hasScheme && uri.scheme != 'file') return null;
  return uri?.scheme == 'file' ? uri!.toFilePath() : reference;
}

String _platformAlias(String path) {
  if (Platform.isMacOS || Platform.isIOS) {
    if (path.startsWith('/var/')) return '/private$path';
    if (path.startsWith('/tmp/')) return '/private$path';
  }
  return path;
}

bool reconstructionFinalRecordProofMatches(
  ScanRecord record,
  ReconstructionFinalArtifactCompletionProof proof,
) =>
    record.accountNamespace == proof.accountNamespace &&
    record.id == proof.recordId &&
    record.finalGlbSha256 == proof.sha256Hex &&
    record.finalGlbBytes == proof.bytes &&
    _recordFilePath(record.artifactPath) != null &&
    _platformAlias(_recordFilePath(record.artifactPath)!) ==
        proof.publishedGlb &&
    proof.stillMatches();

Future<ReconstructionFinalArtifactCompletionProof> verifyReconstructionFinalRecordArtifact(
  ScanRecordStore store,
  ScanRecord record,
) async {
  await store.ensureLoaded();
  final namespace = store.workspace.namespace;
  if (!store.isActive ||
      namespace != record.accountNamespace ||
      record.artifactPath == null ||
      record.finalGlbSha256 == null ||
      record.finalGlbBytes == null) {
    throw StateError('Completed model has no active persisted identity');
  }
  final root = await store.workspaceDirectory();
  final path = _recordFilePath(record.artifactPath);
  if (path == null) throw StateError('Completed model requires a local file');

  await store.requireOwnedPath(path);
  final proof = await verifyReconstructionFinalArtifactCompletion(
    accountDocuments: Directory(await root.resolveSymbolicLinks()),
    accountNamespace: namespace,
    recordId: record.id,
    artifactPath: await File(path).resolveSymbolicLinks(),
    expectedBytes: record.finalGlbBytes!,
    expectedSha256: record.finalGlbSha256!,
  );
  final current = store.byId(record.id);
  if (!store.isActive ||
      store.workspace.namespace != namespace ||
      current == null ||
      !reconstructionFinalRecordProofMatches(current, proof)) {
    throw StateError('Completed model changed during verification');
  }
  return proof;
}
