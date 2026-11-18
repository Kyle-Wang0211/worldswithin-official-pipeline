                                                                                    
                                                                            
                                                                                      
                                                                                   
  
                                                                                      
                                                                                
                                                                
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace aether::dense {

                                                                              
constexpr int PROC_H = 512;                                             
constexpr int PROC_W = 896;                           
constexpr int NPZ_H = 504;                                               
constexpr int NUM_DEPTH = 384;                      

                                                                                    
                                                       
std::array<float, 9> scaled_K(const std::array<float, 9>& K_npz);

                                                                                   
                                                                          
                                                                                  
                                                                                  
                                                                              
std::array<float, 32> make_proj_stage(const std::array<float, 16>& w2c,
                                      const std::array<float, 9>& K, float scale);

                                                          
constexpr std::array<float, 4> STAGE_SCALES = {0.125f, 0.25f, 0.5f, 1.0f};

                                                                                 
                                                                                   
std::array<float, NUM_DEPTH> depth_values(float depth_min, float depth_max);

                                                                                                                                                                         
  
                                                                                              
                                                                                           
                                                                    

                                                                     
struct FrameObs {
    std::string name;                                                                                  
    std::vector<int> point_ids;
    std::array<float, 3> center;                                
};

                                                                                              
  
                                                                                                       
                                                                                       
  
                                                                                                    
                                                              
std::vector<std::string> covis_select(const std::string& ref,
                                      const std::vector<FrameObs>& pool,
                                      const std::vector<std::array<float, 3>>& points,
                                      int k);

                                                                                                 
                                                                               
std::vector<std::string> nearest(const std::string& ref,
                                 const std::vector<FrameObs>& pool,
                                 int k, double min_base);

                                                                                            
                                                                                                    
                                                                                            
                                                                                                      
std::pair<float, float> drange(const std::vector<float>& obs_pts_world,
                               const std::array<float, 16>& w2c, double s_al);

                                                                                                                                                         

                                                                                  
  
                                                                                                             
                                                                                                
  
                                                                           
  
                                                                                                    
                                                                                                                 
                                                                                      
std::vector<int> select_views(const std::vector<std::array<float, 16>>& w2c,
                              int ref_local, int n_view, float min_base = 0.06f);

                                              
std::array<float, 3> cam_center(const std::array<float, 16>& w2c);

                                                                                         
  
                                                                                                   
                                                               
  
                                                                                         
                                                                                              
                                  
  
                                                                                                               
                                            
std::pair<float, float> metric_depth_range(const std::vector<float>& pts_world,
                                           const std::array<float, 16>& w2c_ref);

}                            
