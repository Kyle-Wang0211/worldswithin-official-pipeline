// backend / ScanRecordStore, extend this model and the view model that

import 'package:flutter/material.dart';


/// Immutable identity of the end-to-end capture stack that owns a scan.

enum CapturePipelineKind { self, official }

extension CapturePipelineKindWire on CapturePipelineKind {
  String get wireName {
    switch (this) {
      case CapturePipelineKind.self:
        return 'self';
      case CapturePipelineKind.official:
        return 'official';
    }
  }

  String get displayLabel {
    switch (this) {
      case CapturePipelineKind.self:
        return '自研';
      case CapturePipelineKind.official:
        return '官方';
    }
  }

  static CapturePipelineKind fromWireName(Object? value) {
    switch (value) {
      case 'self':
        return CapturePipelineKind.self;
      case 'official':
        return CapturePipelineKind.official;
      default:
        throw FormatException('Invalid pipeline_kind: $value');
    }
  }
}

@immutable
class ScanRecord {
  final String id;

  final String? accountNamespace;
  final String name;
  final DateTime createdAt;
  final CapturePipelineKind pipelineKind;

  final String? thumbnailPath;

  final String? finalCoverSha256;
  final String? finalCoverGlbSha256;
  final String? finalCoverCameraVersion;

  final String? artifactPath;

  final String? finalGlbSha256;
  final int? finalGlbBytes;

  final String? captureDir;

  final String? photosDir;

  final String? captureManifestPath;

  final int? photoCount;

  final DateTime? localRawDeletedAt;

  final DateTime? resultViewedAt;
  final bool localRawRetainedForDebug;

  final String? authorHandle;

  final String? caption;

  final String? bundledGlbAsset;

  const ScanRecord({
    required this.id,
    this.accountNamespace,
    required this.name,
    required this.createdAt,
    this.pipelineKind = CapturePipelineKind.self,
    this.thumbnailPath,
    this.finalCoverSha256,
    this.finalCoverGlbSha256,
    this.finalCoverCameraVersion,
    this.artifactPath,
    this.finalGlbSha256,
    this.finalGlbBytes,
    this.captureDir,
    this.photosDir,
    this.captureManifestPath,
    this.photoCount,
    this.localRawDeletedAt,
    this.resultViewedAt,
    this.localRawRetainedForDebug = false,
    this.authorHandle,
    this.caption,
    this.bundledGlbAsset,
  }) : assert((finalGlbSha256 == null) == (finalGlbBytes == null));

  ScanRecord copyWith({
    String? name,
    String? thumbnailPath,
    String? finalCoverSha256,
    String? finalCoverGlbSha256,
    String? finalCoverCameraVersion,
    bool clearThumbnailPath = false,
    bool clearFinalCoverProof = false,
    String? artifactPath,
    String? finalGlbSha256,
    int? finalGlbBytes,
    bool clearArtifactPath = false,
    String? captureDir,
    String? photosDir,
    String? captureManifestPath,
    int? photoCount,
    DateTime? localRawDeletedAt,
    DateTime? resultViewedAt,
    bool clearResultViewedAt = false,
    bool? localRawRetainedForDebug,
    String? caption,
  }) {
    return ScanRecord(
      id: id,
      accountNamespace: accountNamespace,
      name: name ?? this.name,
      createdAt: createdAt,
      pipelineKind: pipelineKind,
      thumbnailPath: clearThumbnailPath
          ? null
          : (thumbnailPath ?? this.thumbnailPath),
      finalCoverSha256:
          clearThumbnailPath || clearArtifactPath || clearFinalCoverProof
          ? null
          : (finalCoverSha256 ?? this.finalCoverSha256),
      finalCoverGlbSha256:
          clearThumbnailPath || clearArtifactPath || clearFinalCoverProof
          ? null
          : (finalCoverGlbSha256 ?? this.finalCoverGlbSha256),
      finalCoverCameraVersion:
          clearThumbnailPath || clearArtifactPath || clearFinalCoverProof
          ? null
          : (finalCoverCameraVersion ?? this.finalCoverCameraVersion),
      artifactPath: clearArtifactPath
          ? null
          : (artifactPath ?? this.artifactPath),
      finalGlbSha256: clearArtifactPath
          ? null
          : (finalGlbSha256 ?? this.finalGlbSha256),
      finalGlbBytes: clearArtifactPath
          ? null
          : (finalGlbBytes ?? this.finalGlbBytes),
      captureDir: captureDir ?? this.captureDir,
      photosDir: photosDir ?? this.photosDir,
      captureManifestPath: captureManifestPath ?? this.captureManifestPath,
      photoCount: photoCount ?? this.photoCount,
      localRawDeletedAt: localRawDeletedAt ?? this.localRawDeletedAt,
      localRawRetainedForDebug:
          localRawRetainedForDebug ?? this.localRawRetainedForDebug,
      authorHandle: authorHandle,
      caption: caption ?? this.caption,
      bundledGlbAsset: bundledGlbAsset,
      resultViewedAt: clearResultViewedAt
          ? null
          : (resultViewedAt ?? this.resultViewedAt),
    );
  }

  bool get hasCompletedArtifact => artifactPath != null;

  ScanProcessingBadge badgeFor(
    DateTime? sparseReadyAt, {
    bool isActivelyReconstructing = false,
  }) {
    if (captureDir == null) return ScanProcessingBadge.none;
    if (isActivelyReconstructing) return ScanProcessingBadge.generating;
    if (sparseReadyAt == null) {
      return isActivelyReconstructing
          ? ScanProcessingBadge.generating
          : ScanProcessingBadge.unfinished;
    }
    final viewed = resultViewedAt;

    if (viewed != null && !viewed.isBefore(sparseReadyAt)) {
      return ScanProcessingBadge.none;
    }
    return ScanProcessingBadge.done;
  }
}

enum ScanProcessingBadge { none, generating, unfinished, done }

final RegExp kUntitledScanNamePattern = RegExp(r'^(?:未命名|Untitled)\((\d+)\)$');

String nextUntitledScanName(Iterable<String> existingNames) {
  final used = <int>{};
  for (final name in existingNames) {
    final m = kUntitledScanNamePattern.firstMatch(name);
    if (m == null) continue;
    final n = int.tryParse(m.group(1)!);
    if (n != null && n > 0) used.add(n);
  }
  var next = 1;
  while (used.contains(next)) {
    next++;
  }
  return '未命名($next)';
}
