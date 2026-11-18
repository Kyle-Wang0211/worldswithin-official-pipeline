import 'package:uuid/uuid.dart';

/// Unmodified upstream RFC UUIDv4 implementation. Existing IDs are never
                                                                       
String newCaptureWorkId() => 'cap_${const Uuid().v4()}';

                                                                          
                                                                 
String workNameKey(String name) {
  final trimmed = name.trim();
  final unnamed = RegExp(r'^(?:未命名|Untitled)\(([0-9]+)\)$').firstMatch(trimmed);
  if (unnamed != null) return 'Untitled(${unnamed.group(1)})';
  if (trimmed == '导入的模型') return 'Imported model';
  return trimmed;
}

class DuplicateWorkNameException extends StateError {
  DuplicateWorkNameException() : super('Work name already exists');
}

                                                                           
                                                                               
String availableWorkName(String desired, Iterable<String> existing) {
  final used = existing.map(workNameKey).toSet();
  final name = desired.trim();
  if (!used.contains(workNameKey(name))) return name;
  for (var number = 2; ; number++) {
    final suffix = ' ($number)';
    var end = name.length < 256 - suffix.length
        ? name.length
        : 256 - suffix.length;
    if (end > 0 &&
        name.codeUnitAt(end - 1) >= 0xd800 &&
        name.codeUnitAt(end - 1) <= 0xdbff) {
      end--;
    }
    final base = name.substring(0, end);
    final candidate = '$base$suffix';
    if (!used.contains(workNameKey(candidate))) return candidate;
  }
}
