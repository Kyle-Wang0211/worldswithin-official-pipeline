                                                                                
                                                                             
                                                                                
                                                                                   
                                                                           
#pragma once
#include <cstdint>
#include <vector>

                                                                              
                                                                          
                                                                               
struct FIBITMAP {
  int width = 0;
  int height = 0;
  int channels = 1;
  std::vector<uint8_t> data;
};
