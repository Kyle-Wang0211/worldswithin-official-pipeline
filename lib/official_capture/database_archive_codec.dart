import 'dart:io';

                                                                           
abstract interface class DatabaseArchiveCodec {
  bool get isSupported;

  Future<void> compress({
    required File sourceDatabase,
    required File destinationArchive,
  });

  Future<void> decompress({
    required File sourceArchive,
    required File destinationDatabase,
  });

                                                                         
  void requestCancellation();
}

                                                                          
final class DatabaseArchiveCancelled implements Exception {
  const DatabaseArchiveCancelled();
}
