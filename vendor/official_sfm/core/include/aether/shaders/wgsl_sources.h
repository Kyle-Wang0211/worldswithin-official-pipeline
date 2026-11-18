// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#ifndef AETHER_SHADERS_WGSL_SOURCES_H
#define AETHER_SHADERS_WGSL_SOURCES_H

                                                                                                                                            
  
                                                                      
                                               
  
                                                                          
                                                                              
                                                                         
                  
  
                                                                         
                                                                      
                                                              
                                                                       
                                                                       
                                                                      
  
                           
                                      
                                
                                                                          
                             
                                                                        
                                                                    
                                                                 
  
                                                                         
                                                                        
        

namespace aether {
namespace shaders {

extern const char splat_render_wgsl[];
extern const char mesh_render_wgsl[];
extern const char project_forward_wgsl[];
extern const char project_visible_wgsl[];
extern const char project_backwards_wgsl[];
extern const char map_gaussian_to_intersects_wgsl[];
extern const char rasterize_wgsl[];
extern const char rasterize_backwards_wgsl[];
extern const char sort_count_wgsl[];
extern const char sort_reduce_wgsl[];
extern const char sort_scan_wgsl[];
extern const char sort_scan_add_wgsl[];
extern const char sort_scatter_wgsl[];
extern const char sort_prep_depth_wgsl[];
extern const char prefix_sum_scan_wgsl[];
extern const char prefix_sum_scan_sums_wgsl[];
extern const char prefix_sum_add_scanned_sums_wgsl[];
                                                                                
                                                            
                                                                  
                                    

                                                                                 
                                                                             
                                                                            
                                                 
extern const char sift_gray_to_f32_wgsl[];
extern const char sift_gss_blur_wgsl[];
extern const char sift_gss_blur_fused_wgsl[];
extern const char sift_gss_resample_wgsl[];
extern const char sift_dog_detect_wgsl[];
                                                                                                      
                                                                  
                                                                
                                                                                                       
                                                                             
extern const char sift_dog_detect_compact_wgsl[];
extern const char sift_dog_refine_wgsl[];
extern const char sift_dog_cand_reset_wgsl[];
extern const char sift_nonextrema_suppress_wgsl[];
extern const char sift_suppress_grid_count_wgsl[];
extern const char sift_suppress_grid_scan_wgsl[];
extern const char sift_suppress_grid_scatter_wgsl[];
extern const char sift_suppress_grid_wgsl[];
extern const char sift_affine_shape_wgsl[];
extern const char sift_orientation_wgsl[];
extern const char sift_orientation_atomic_wgsl[];
extern const char sift_dsp_descriptor_wgsl[];
extern const char sift_dsp_descriptor_f16_wgsl[];
extern const char sift_dsp_descriptor_f16_atomic_wgsl[];
extern const char sift_dsp_descriptor_par_wgsl[];
extern const char sift_dsp_mean_wgsl[];

}                      
}                     

#endif                                  
