                                                  
  
                                                  
                                          
                                      
                                             
                                  
  
                                   
                                           
                                            
                        
                                        
                                         
                                                   
                           
  
                                                         
                                          
                                                    
                    
                                       
                                           
                 
                                  
                                                            
               
  
                                          
                                                 
                                               
                                           
                                                
                
                                                       
                                               
                                                   
                                        
                                          
  
                                     
                                                              
                                                               
                    
  
                                                            
                                                                
                                                      
                                                 
  
                                                         
                                                 
                                    
                       

import 'dart:typed_data';

                                                       
                                          
enum PhotoCardSfmState {
                                              
  pending(0),

                       
  registered(1),

                          
  disconnected(2),

                                      
  lowParallax(3);

  const PhotoCardSfmState(this.channelValue);
  final int channelValue;
}

                                                     
                                                    
                                              
                                                 
                                  
   
                                                      
                                          
                     
PhotoCardSfmState photoCardSfmState({
  required int frameId,
  required Float64List posesPacked,
  required bool? lowParallax,
}) {
                                                
                                           
                                      
                                              
                                            
  for (var i = 0; i + 8 < posesPacked.length; i += 9) {
    if (posesPacked[i].toInt() != frameId) continue;
    if (posesPacked[i + 1] == 0) return PhotoCardSfmState.pending;
    return PhotoCardSfmState.registered;
  }
  return PhotoCardSfmState.pending;
}

                               
double? medianOf(List<double> values) {
  if (values.isEmpty) return null;
  final sorted = List<double>.from(values)..sort();
  final mid = sorted.length ~/ 2;
  return sorted.length.isOdd
      ? sorted[mid]
      : (sorted[mid - 1] + sorted[mid]) / 2;
}

                                             
                                            
              
   
                                                  
                                             
                                               
                            
const double kFrameYellowInitialDeg = 5.0;

                                     
                                                                  
const double kFrameYellowEnterDeg = 4.0;

                                       
const double kFrameYellowExitDeg = 6.0;

                                      
                                                
                                    
                  
const int kFrameYellowEnterStreak = 2;

                                        
                                     
                                      
                                   
                                    
                                                     
int frameBelowEnterStreak({
  required double sampleDeg,
  required int prevStreak,
  double enterDeg = kFrameYellowEnterDeg,
}) => sampleDeg < enterDeg ? prevStreak + 1 : 0;

                              
   
                                                       
                                                          
                                           
                                                
   
                                                       
                                     
                                                          
                                                          
                                                                
                                              
                                
                                                        
                      
   
                                                  
                                                 
                                               
                                  
bool? frameLowParallaxTrue({
  required double? trueMedianDeg,
  required bool? wasLowParallax,
  int belowEnterStreak = kFrameYellowEnterStreak,
  double initialDeg = kFrameYellowInitialDeg,
  double enterDeg = kFrameYellowEnterDeg,
  double exitDeg = kFrameYellowExitDeg,
  int enterStreak = kFrameYellowEnterStreak,
}) {
  if (trueMedianDeg == null) return null;
  if (wasLowParallax == null) return trueMedianDeg < initialDeg;
  if (wasLowParallax) return trueMedianDeg < exitDeg;
                                                
  return trueMedianDeg < enterDeg && belowEnterStreak >= enterStreak;
}
