                      
  
                          
                           
                                                                   
                                               
                                                           
                  
  
                                                          
                                                           
           
  
           
                                            
                                         
                                          
                                       
                         
  
                                      
  
                                                

import 'dart:typed_data';

import 'package:image/image.dart' as img;

                                     
   
                                                 
                                                
                       
Uint8List encodeSanitizedJpg(img.Image image, {int quality = 88}) {
  _stripMetadata(image);
  return Uint8List.fromList(img.encodeJpg(image, quality: quality));
}

                 
   
                                          
                                     
void _stripMetadata(img.Image image) {
  final exif = image.exif;
  exif.imageIfd.data.clear();
  exif.thumbnailIfd.data.clear();
  exif.exifIfd.data.clear();
  exif.gpsIfd.data.clear();
  exif.interopIfd.data.clear();
}
