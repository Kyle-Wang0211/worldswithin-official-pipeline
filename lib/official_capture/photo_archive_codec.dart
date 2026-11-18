import 'dart:io';

                                    
   
                                                                         
                                                                               
abstract interface class PhotoArchiveCodec {
  bool get isSupported;

  Future<void> encodeJpeg({
    required File sourceJpeg,
    required File destinationJxl,
  });

  Future<void> reconstructJpeg({
    required File sourceJxl,
    required File destinationJpeg,
  });

                                                                         
  void requestCancellation();
}

                                                                              
final class PhotoArchiveCancelled implements Exception {
  const PhotoArchiveCancelled();
}
