// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.
#ifndef AETHER_CPP_TOOLS_DAWN_HISTOGRAM_SINK_H
#define AETHER_CPP_TOOLS_DAWN_HISTOGRAM_SINK_H

#include <cstdint>

                                                                 
  
                                                                                        
                                                                                
                                                                                                  
                                                                                           
                                                                                                          
                                                                    
namespace aether {
namespace tools {

                                                                            
                                           
void* dawn_histogram_sink_platform();

                                                      
                                                                                                             
void dawn_histogram_get(const char* name, double* out_ms, uint32_t* out_n);

                                                                  
                                                                  
extern const char kDawnHistMslLibrary[];                                            
extern const char kDawnHistPipelineState[];                                                          

}                    
}                     
#endif
