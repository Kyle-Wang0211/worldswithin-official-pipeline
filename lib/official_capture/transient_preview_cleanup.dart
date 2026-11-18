import 'dart:io';

import 'photo_archive_policy.dart';

                                                                             
   
                                                                              
                                                                     
Future<bool> removeTransientCapturePreviews(
  Directory captureDirectory, {
  bool Function()? canContinue,
}) async {
  if (await PhotoArchivePolicy.readCompatible(captureDirectory) == null) {
    return false;
  }
  final previewsPath = '${captureDirectory.path}/previews';
  try {
    final type = await FileSystemEntity.type(previewsPath, followLinks: false);
    if (canContinue != null && !canContinue()) return false;
    if (type == FileSystemEntityType.notFound) return false;
    if (type == FileSystemEntityType.link) {
      await Link(previewsPath).delete();
      return true;
    }
    if (type != FileSystemEntityType.directory) return false;
    await Directory(previewsPath).delete(recursive: true);
    return true;
  } on FileSystemException {
    return false;
  }
}
