import 'package:flutter/foundation.dart';
import 'controller.dart';

                                                                             
                                                                               
                                                       
class SharedViewerRetainedSession {
  SharedViewerRetainedSession({required this.controller});
  final SharedViewerController controller;
  bool get retentionSupported => false;
  bool get isReady => false;
  bool get hasPresentationLease => false;
  bool get isDisposed => _disposed;
  Object? get error => null;
  bool _disposed = false;
  void addListener(VoidCallback listener) {}
  void removeListener(VoidCallback listener) {}
  Future<void> dispose() async {
    _disposed = true;
  }
}
