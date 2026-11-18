                                                  
  
                                             
                                                   
                                                           
                                                  
                                             
                    
  
                                                  
                                    
  
                                                   
                                                     
  
                                                 
                                                   
                                                 
                          
  
                                                
                                                 
                                                           
  
                                                              
                        

                                                   
                                                  
String photoSlotBaseName({
  required int cellIdx,
  required int slotIdx,
  required String frameId,
}) {
  return 'cell_${cellIdx}_slot_${slotIdx}_$frameId';
}

                                       
   
                                                          
                                                       
                                          
                                                            
                                                
   
                                                  
                                           
                                                 
                            
int maxFrameSeqInNames(Iterable<String> fileNames) {
  var maxSeq = 0;
  for (final name in fileNames) {
    final n = frameSeqInName(name);
    if (n != null && n > maxSeq) maxSeq = n;
  }
  return maxSeq;
}

                                
   
                                                     
                                                             
                                                     
                                                              
                             
                                                  
                             
   
                               
int? frameSeqInName(String fileName) {
  final m = RegExp(r'_(?:tap|cap)-(\d+)(?:\.[^.]*)?$').firstMatch(fileName);
  if (m == null) return null;
  return int.tryParse(m.group(1)!);
}
