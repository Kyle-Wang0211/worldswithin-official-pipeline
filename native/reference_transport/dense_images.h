                                                                                                
  
                                                                         
                                                                                      
                                                                                          
                               
                                                                                                        
                                                                                                  
                                                                                            
                                                                                           
                                                                                      
                                                                                                     
                                                                                       
                                                                                                         
                                                                                                    
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace aether::dense {

struct RgbImage {
    int w = 0, h = 0;
    std::vector<uint8_t> rgb;                      
};

                                                                                        
bool decode_jpeg_rgb_file(const std::string& path, RgbImage& out);
bool decode_jpeg_rgb_mem(const uint8_t* bytes, size_t n, RgbImage& out);

                                                                                
void pil_resize_bilinear_rgb(const RgbImage& in, int W, int H, RgbImage& out);

                                                                                                                       
                                                                                                                          
                                                                                                                              
void pil_resize_bilinear_l(const std::vector<uint8_t>& in, int w, int h, int W, int H, std::vector<uint8_t>& out);

                                                      
void pil_rgb_to_l(const RgbImage& rgb, std::vector<uint8_t>& gray);

                                                                                                    
void gray_to_f16(const std::vector<uint8_t>& gray, std::vector<uint16_t>& f16);

                                                                                                        
                                                                                                      
                                                                                       
                                                                                             
                                                                                                   
                                                                                                         
                                                                                                        
                                                                                                              
                                                                                                        
                                                                                                        
                                                                                               
enum class FrameSourceKind : int32_t { Jpeg = 0, Nv12 = 1 };
struct FrameSource {
    FrameSourceKind kind = FrameSourceKind::Jpeg;
    std::string path;                                                              
    int width = 0, height = 0;                                             
    int nv12_matrix = 0;                                                                                                                  
};
                                                                                                              
                                                                                                           
                                                                             
bool load_frame_rgb(const FrameSource& src, RgbImage& out, std::string* err = nullptr);

}                            
