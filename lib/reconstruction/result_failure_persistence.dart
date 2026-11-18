import 'dart:convert';
import 'dart:io';

typedef ReconstructionResultWriter = Future<void> Function(File file, String contents);

Future<void> _writeResult(File file, String contents) async {
  await file.writeAsString(contents, flush: true);
}

                                                                          
                                                                              
                                                                           
Future<Map<String, Object?>> saveReconstructionResultSafely(
  File file,
  Map<String, Object?> value, {
  required bool terminal,
  ReconstructionResultWriter? writer,
}) async {
  try {
    await (writer ?? _writeResult)(file, jsonEncode(value));
  } catch (error) {
    if (!terminal) {
      throw FileSystemException(
        'RECONSTRUCTION checkpoint persistence failed before next stage: $error',
        file.path,
      );
    }
    value['result_persistence_error'] = error.toString();
    value['result_persisted'] = false;
    value['full_pipeline_complete'] = false;
    if (value['exit_code'] == 0) {
      value['exit_code'] = 38;
      value['stopped_reason'] = 'Terminal result persistence failed: $error';
    }
                                                                              
                                                                               
  }
  return value;
}
