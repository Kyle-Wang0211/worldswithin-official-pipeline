                                                                                        
  
                                                                                          
                                                                                           
                                                                                                                   
                                                                   
                                                        
                                                                                                                   
                                                                                                                    
                                                                                                    
                                                                                                                  
                                                                          
#pragma once
#include <cstddef>

namespace aether::dense {

constexpr float kMultiresTh = 0.02f;                            

                                                                                                              
                                                                                
size_t cer_merge(int Wl, int Hl, const float* depth_lo, const float* const conf_lo[3],
                 int Wh, int Hh, const float* depth_hi, const float* const conf_hi[3], float th,
                 float* depth_out, float* const conf_out[3]);

}                            
