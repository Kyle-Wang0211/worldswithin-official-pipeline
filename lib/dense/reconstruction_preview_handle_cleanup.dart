import 'dart:io';

                                                                           
                                                                              
                                                                             
void closeReconstructionPreviewHandles(Iterable<RandomAccessFile?> handles) {
  for (final handle in handles) {
    try {
      handle?.closeSync();
    } catch (_) {
                                                                             
    }
  }
}
