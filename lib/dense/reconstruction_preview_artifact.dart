import 'dart:io';

import '../reconstruction/request.dart' show hashFile;

                                                                               
final class ReconstructionPreviewObserver {
  ReconstructionPreviewObserver(this.callback);
  final void Function(ReconstructionPreviewArtifact)? callback;
  final errors = <String>[];
  void emit(ReconstructionPreviewArtifact artifact) {
    try {
      callback?.call(artifact);
    } catch (error) {
      if (errors.length < 5) errors.add('$error');
    }
  }
}

                                                                       
final class ReconstructionPreviewArtifact {
  const ReconstructionPreviewArtifact({
    required this.path,
    required this.stage,
    required this.role,
    required this.bytes,
    required this.sha256,
  });
  final String path, stage, role, sha256;
  final int bytes;
  String get identity => '$path:$sha256';
  Map<String, Object?> toJson() => {
    'path': path,
    'stage': stage,
    'role': role,
    'bytes': bytes,
    'sha256': sha256,
  };

  static Future<ReconstructionPreviewArtifact> verify(
    Object? value, {
    required String workRoot,
  }) async {
    if (value is! Map) throw const FormatException('Preview metadata required');
    final path = value['path'], stage = value['stage'], role = value['role'];
    final bytes = value['bytes'], sha = value['sha256'];
    if (path is! String ||
        stage is! String ||
        role is! String ||
        bytes is! int ||
        bytes <= 0 ||
        sha is! String ||
        !RegExp(r'^[0-9a-f]{64}$').hasMatch(sha) ||
        !const {
          'sparse',
          'depth',
          'dense',
          'tsdf',
          'trim',
          'meshopt',
          'lagrange',
          'glb',
        }.contains(stage) ||
        (role != 'ply' && role != 'glb') ||
        (role == 'glb') != (stage == 'glb') ||
        !path.startsWith('$workRoot/') ||
        path.split('/').any((part) => part == '..' || part == '.') ||
        !path.endsWith('.$role')) {
      throw const FormatException('Invalid reference_pipeline preview identity');
    }
    if (await Directory(workRoot).resolveSymbolicLinks() != workRoot ||
        await FileSystemEntity.type(path, followLinks: false) !=
            FileSystemEntityType.file ||
        await File(path).resolveSymbolicLinks() != path) {
      throw const FormatException('Noncanonical reference_pipeline preview');
    }
    final file = File(path);
    if (await file.length() != bytes ||
        await hashFile(file) != sha ||
        await file.length() != bytes) {
      throw const FormatException('ReferencePipeline preview bytes changed');
    }
    return ReconstructionPreviewArtifact(
      path: path,
      stage: stage,
      role: role,
      bytes: bytes,
      sha256: sha,
    );
  }
}
