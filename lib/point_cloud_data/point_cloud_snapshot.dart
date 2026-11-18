import 'dart:typed_data';

                                                                                
abstract interface class PointCloudSnapshot {
  Float32List get xyz;
  Uint8List get rgb;
  int get pointCount;
}
