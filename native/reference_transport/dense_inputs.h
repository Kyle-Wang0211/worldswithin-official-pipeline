                                                                                                            
  

                                                 
                                                                                                    
                                                                         
                                                              
                                                                                                   
                                                                                                    
                                                                                
                                                                                                    
                             
#pragma once
#include <cstdint>
#include <vector>

namespace aether::dense {

constexpr int DENSE_NUM_DEPTH = 384;

struct FrameInputs {
    int n_view = 0;
    std::vector<float> pm[3];                                      
    std::vector<float> dv;              
};

                                                                        
void build_frame_inputs(const float* cams, const std::vector<int32_t>& views, FrameInputs& out);

                                                                                       
float fp16_to_fp32(uint16_t x);
                                                                       
uint16_t fp32_to_fp16(float f);

}                            
