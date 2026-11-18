                                                                                              
  
                                                                                          
                                               
                                                 
                                                                                          
                                                                                               
                                                                                                
                                                     
  
                                                                         
                                                                                               
                                                                                             
                                                                                       
                                                                                              
                          
                                                                                             
                                                                                             
                                                                                              
  
                                                                                              
                                                                                            
                                                            
#pragma once
#include <cstdint>
#include <vector>

namespace aether::dense {

                                                                                             
                                                                                        
                                                                                 
                                                                                                     
                                                                                                       
                                                                                                         
extern int g_fuse_matmul_mode;

struct FuseCamera {
    double K[9];                                                           
    double E[16];                                                                                 
    float depth_min;                                              
    float depth_max;                                              
    double invK[9];                           
    double invE[16];                          
};

                                                                                                           
bool fuse_camera_finalize(FuseCamera& c);

                                                                                                   
bool lapack_inv(int n, const double* A_rowmajor, double* out_rowmajor);

struct FuseParams {
    float photo_thres[3] = {0.3f, 0.5f, 0.5f};                                                       
    float geo_pixel_thres = 1.0f;                                            
    float geo_depth_thres = 0.01f;                                           
    int   geo_mask_thres  = 3;                                                                              
                                                                                                                          
                                                                                                                            
                                                                                                                           
                                                                                                                           
                                                                                                                        
                                                                                                                    
                                                                                                                     
                                                                                                      
                                                                                                                    
                                                                                                                          
                                                                                                                         
                             
    int   check_all_views = 0;
    float sat_tol = 0.0f;
    float min_tri_angle_deg = 0.0f;
                                                                                                                        
                                                                                                                    
                                                                                                                        
                                                                                       
    int   photo_first = 0;
};

                                                                                
struct GeoProbe {
    std::vector<float> x_src, y_src, sampled, depth_reproj_raw, x_reproj, y_reproj;
    std::vector<double> x_src64, y_src64;                                                                       
};

                                                                                    
                                                                                
void check_geometric_consistency(int W, int H, const float* depth_ref, const FuseCamera& cam_ref,
                                 const float* depth_src, const FuseCamera& cam_src,
                                 float geo_pixel_thres, float geo_depth_thres,
                                 std::vector<uint8_t>& mask, std::vector<float>& depth_reproj,
                                 GeoProbe* probe = nullptr, const uint8_t* active = nullptr);
                                                                                                                 
                                                                                                                  
                                  

struct FrameFusion {
    std::vector<uint8_t> final_mask;         
    std::vector<int32_t> geo_sum;            
    std::vector<double>  d_avg;                                             
    std::vector<float>   xyz;                                                                          
    std::vector<double>  xyz64;                                                                        
    std::vector<uint8_t> rgb;                
    double photo_frac = 0, geo_frac = 0, final_frac = 0;
};

                                                                                            
                                                                                                               
                                                          
void fuse_frame(int W, int H, const float* depth_ref, const float* const conf[3], const uint8_t* rgb,
                const FuseCamera& cam_ref, const std::vector<const float*>& depth_srcs,
                const std::vector<const FuseCamera*>& cam_srcs, const FuseParams& p, FrameFusion& out);

                                                                                                             
bool fuse_view_sees(int W, int H, const float* depth_ref, const FuseCamera& cam_ref, const FuseCamera& cam_src);

                                                                                            
bool write_ply(const char* path, const std::vector<float>& xyz, const std::vector<uint8_t>& rgb);

}                            
