import 'dart:io';

                                                               
abstract final class DatabaseArchivePreprocess {
  static const rawV1 = 'raw_v1';
  static const trackDeltaV1 = 'track_delta_v1';

  static bool isSupported(String value) =>
      value == rawV1 || value == trackDeltaV1;
}

                                                                        
abstract interface class DatabaseArchivePreprocessor {
  bool get isSupported;

  Future<void> transformTrackDelta({
    required File sourceDatabase,
    required File destinationDatabase,
  });

  Future<void> restoreTrackDelta({
    required File sourceDatabase,
    required File destinationDatabase,
  });

                                                                         
  void requestCancellation();
}
