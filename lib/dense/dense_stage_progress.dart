                                                                                                           
                                                                                              
import 'package:flutter/foundation.dart';

import 'dense_live_cloud.dart';
import 'reconstruction_preview_artifact.dart';

                                                                                
final class DenseProgressSourceTime {
  const DenseProgressSourceTime(this.monoNs, this.wallNs);
  final int monoNs, wallNs;
}

enum DenseStageState { running, done, failed }

final class DenseStageProgress {
  const DenseStageProgress({
    required this.captureDir,
    required this.state,
    this.phase = '',
    this.done = 0,
    this.total = 0,
    this.outPly,
    this.message,
    this.points = 0,
    this.startedAt,
    this.finishedAt,
    this.live,
    this.preview,
    this.sourceTime,
  });
  final String captureDir;
  final DenseStageState state;
  final String phase;
  final DenseProgressSourceTime? sourceTime;
  final int done, total;
  final String? outPly;

                                                                                                   
  final String? message;
  final int points;
  final DateTime? startedAt;
  final DateTime? finishedAt;

                                                                                                                 
                                                                                                      
  final DenseLiveCloud? live;
  final ReconstructionPreviewArtifact? preview;

  DenseStageProgress copyWith({
    DenseStageState? state,
    String? phase,
    int? done,
    int? total,
    String? outPly,
    String? message,
    int? points,
    DateTime? startedAt,
    DateTime? finishedAt,
    DenseLiveCloud? live,
    ReconstructionPreviewArtifact? preview,
    DenseProgressSourceTime? sourceTime,
  }) => DenseStageProgress(
    captureDir: captureDir,
    state: state ?? this.state,
    phase: phase ?? this.phase,
    done: done ?? this.done,
    total: total ?? this.total,
    outPly: outPly ?? this.outPly,
    message: message ?? this.message,
    points: points ?? this.points,
    startedAt: startedAt ?? this.startedAt,
    finishedAt: finishedAt ?? this.finishedAt,
    live: live ?? this.live,
    preview: preview ?? this.preview,
    sourceTime: sourceTime,
  );

                                                                           
                                                                       
  String get label {
    final frames = total > 0 ? ' · 已处理 $done/$total 张' : '';
    switch (phase) {
      case 'input_preparation':
      case 'dense_native_binding':
      case 'original_python_runtime_binding':
      case 'preparing':
      case 'session':
        return '1/7 整理照片';
      case 'pw_full_pipeline_dense_run':
      case 'reconstruction_production_whole_run_v1':
        return '2/7 准备图像特征';
      case 'whole_features':
        return '2/7 准备图像特征$frames';
      case 'whole_depth_low':
        return '3/7 估算深度 · 初步$frames';
      case 'whole_depth_high':
        return '3/7 估算深度 · 精细$frames';
      case 'mvs768':
      case 'mvs1536':
        return '3/7 估算深度';
      case 'whole_consistency':
      case 'cer':
        return '4/7 筛选与融合 · 筛选深度';
      case 'whole_fusion':
        return '4/7 筛选与融合$frames';
      case 'fusionPly':
        return '4/7 筛选与融合';
      case 'downstream_native_binding':
      case 'tsdf':
      case 'trim':
      case 'meshopt':
      case 'lagrange':
        return '5/7 生成并整理模型';
      case 'texture':
      case 'avVisibility':
      case 'avSfmToAbc':
      case 'avNormal':
        return '6/7 添加颜色与表面细节';
      case 'terminal_glb_format_validation':
      case 'glb':
        return '7/7 保存模型';
      case 'done':
        return '7/7 模型已保存';
      case 'preparation_failed':
      case 'failed':
        return '处理暂时中断，本地素材已保留';
      case 'images':
        return '整理照片$frames';
      case 'infer':
        return '估算深度$frames';
      case 'fuse':
        return '筛选与融合$frames';
      default:
        return '正在处理模型';
    }
  }

                                                                                                          
                                                                                                   
  double? get fraction {
    if (state == DenseStageState.done) return 1;
    if (total <= 0) return null;
    final double start, end;
    switch (phase) {
      case 'session':
        start = 0.0;
        end = 0.02;
      case 'images':
        start = 0.02;
        end = 0.05;
      case 'infer':
        start = 0.05;
        end = 0.95;
      case 'fuse':
        start = 0.95;
        end = 1.0;
      default:
        return null;
    }
                                                                                                 
    return (start * (total - done) + end * done) / total;
  }

  Duration elapsed([DateTime? now]) {
    final s = startedAt;
    if (s == null) return Duration.zero;
    return (finishedAt ?? now ?? DateTime.now()).difference(s);
  }

                                                              
  static String shorten(Object error, {int max = 90}) {
    var s = error.toString().split('\n').first.trim();
    final arrow = s.indexOf(' <- ');
    if (arrow > 0) s = s.substring(0, arrow);
    return s.length > max ? '${s.substring(0, max)}…' : s;
  }
}

String formatDenseDuration(Duration d) {
  final s = d.inSeconds;
  if (s < 60) return '$s 秒';
  final m = s ~/ 60, r = s % 60;
  if (m < 60) return '$m 分 $r 秒';
  return '${m ~/ 60} 小时 ${m % 60} 分';
}

                                                                                                     
final ValueNotifier<DenseStageProgress?> denseStageProgress =
    ValueNotifier<DenseStageProgress?>(null);
