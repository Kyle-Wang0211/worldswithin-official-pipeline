                                                                                     
                                                                                       
                                        
  
                                                                                       
                                                                                                        
                                                                                                                 
                                                                                                    
                                                                                                   
                                                                                                
                                                                                                   
                                                                                           
                                                                                           
                                                                                          
                                                                                     
  
        
                                                                                              
                                                                                    
                                                                                
                                                                                             
                                                                                                  
                                                                                                 
                                                                                              
                                                                 
                                                                                    
                                                                                              
                                                                                               
                                                                                               
//     @113c068e9af3 (no distance term); see pwlod_camera for the general ortho_mix rule.
#pragma once

#include <stdint.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWLOD_ABI_VERSION 3
#define PWLOD_TARGET_COUNT 3                                               

typedef enum pwlod_status {
  PWLOD_OK = 0,
  PWLOD_ERR_ARG = 1,                                                                             
  PWLOD_ERR_IO = 2,                                    
  PWLOD_ERR_FORMAT = 3,                                                                   
  PWLOD_ERR_GPU = 4,                                                                     
  PWLOD_ERR_STATE = 5,                                                                                
  PWLOD_ERR_NOMEM = 6
} pwlod_status;

                                                                                         

                                                                                          
                                                                                         
                                                                                           
                                                                                
                                                                                      
typedef struct pwlod_gpu {
  WGPUInstance instance;
  WGPUAdapter adapter;
  WGPUDevice device;
  WGPUQueue queue;
  WGPUBackendType backend;
} pwlod_gpu;

pwlod_status pwlod_gpu_create(const WGPUFeatureName* required_features,
                              uint32_t required_feature_count,
                              pwlod_gpu* out_gpu);
void pwlod_gpu_destroy(pwlod_gpu* gpu);                                                     

                                                                             

typedef struct pwlod_viewer pwlod_viewer;             

typedef enum pwlod_point_size_mode {
  PWLOD_PSIZE_FIXED = 0,                              
  PWLOD_PSIZE_ADAPTIVE = 1,                                                           
  PWLOD_PSIZE_VIEWER = 2                                                                
} pwlod_point_size_mode;

                                                                                        
                                                                                                
                                                                                     
                                                                                            
                                                                                              
                                                                                               
                                                                                                 
                                                                 
                                                                                           
                                                                                     
                                                                                             
typedef struct pwlod_camera {
  double view_proj_row_major[16];                                                        
  double eye_world[3];                                                             
  double focal_px;                                                                
  double orbit_distance;                                       
  double ortho_mix;                                                                              
  uint32_t viewport_width_px;                                       
  uint32_t viewport_height_px;
} pwlod_camera;

typedef struct pwlod_params {
  int64_t point_budget;                                                     
  double target_frame_ms;                                                                            
  pwlod_point_size_mode point_size_mode;                        
  int32_t async_loading;                                                                      
  uint64_t cache_bytes;                                                                             
  float background_rgba[4];                                                 
                                                                                            
  int32_t debug_render_sleep_ms;                                                         
  int32_t debug_publish_before_done;                                                             
} pwlod_params;

typedef struct pwlod_frame_stats {
  uint64_t frame_number;                                               
  uint64_t completed_frame_number;                                                                  
  int64_t points_drawn;
  int32_t nodes_drawn;
  int32_t nodes_loading;                                          
  int32_t uploads_this_frame;                                      
  int64_t dropped_for_cache;                                                         
  double min_node_pixel_size;                                            
  double cpu_ms;                                                                     
  double gpu_ms;                                                                          
                                                                                                
                                                                                               
                                                                                           
                                                                                            
                                                                                                
  double lowest_spacing;
  int32_t source;                                                                       
} pwlod_frame_stats;

                                                                                              
                                                                                                  
                                                                          
typedef struct pwlod_target {
  WGPUTexture texture;
  WGPUSharedTextureMemory memory;
} pwlod_target;

                                                                                              
                                                                                               
                                                                                    
                                                          
typedef void (*pwlod_frame_ready_fn)(void* user,
                                     uint32_t target_index,
                                     const pwlod_frame_stats* stats);

void pwlod_params_default(pwlod_params* out);

                                                                                                
                                                                                          
                                                                                              
                                                                                                 
                                                                                              
                                                                                                
                                                                                                   
                                                                                   
                                                                                           
                                                                                          
                                                                                                
                                                                                    
                                                                                                 
                                                                                               
                                                                                               
                                                                                           
                                                                                                  
                                                                                             
typedef enum pwlod_tone {
  PWLOD_TONE_AGX = 0,
  PWLOD_TONE_ACES = 1,
  PWLOD_TONE_PBR_NEUTRAL = 2,                                            
  PWLOD_TONE_NONE = 3
} pwlod_tone;

typedef enum pwlod_selection_mode {
  PWLOD_SEL_NONE = 0,
  PWLOD_SEL_TINT_OUTSIDE = 1,
  PWLOD_SEL_CULL_OUTSIDE = 2
} pwlod_selection_mode;

typedef struct pwlod_style {
  float point_size;                                                              
  float sprite_px;                                                  
  float disc_radius_px_at_scale1;                                 
  float max_sprite_scale;                      
  pwlod_tone tone;                                   
  float exposure;                                   
  float uncolored_min_y;                                             
  float uncolored_inv_y_span;                                            
  pwlod_selection_mode selection_mode;
  double selection_center[3];                                    
  double selection_size[3];                                                          
  double selection_rot_row_major[9];                      
  uint32_t selection_out_argb;                                           
} pwlod_style;

void pwlod_style_default(pwlod_style* out);

                                              
pwlod_status pwlod_viewer_set_style(pwlod_viewer* viewer, const pwlod_style* style);

pwlod_status pwlod_viewer_create(const pwlod_gpu* gpu, pwlod_viewer** out_viewer);

                                                                                         
                                                                     
pwlod_status pwlod_viewer_load_octree(pwlod_viewer* viewer, const char* octree_dir);

                                                                                                
                                                                                                 
                                                                                         
                                                                                                  
                                                                                              
                                                  
pwlod_status pwlod_viewer_set_points(pwlod_viewer* viewer,
                                     const float* xyz,
                                     const uint8_t* rgb,
                                     uint64_t count,
                                     int32_t colored,
                                     const uint8_t* visibility);

                                                                      
pwlod_status pwlod_viewer_set_params(pwlod_viewer* viewer, const pwlod_params* params);
pwlod_status pwlod_viewer_set_camera(pwlod_viewer* viewer, const pwlod_camera* camera);

                                                                                         
                                                                                                
pwlod_status pwlod_viewer_set_targets(pwlod_viewer* viewer,
                                      const pwlod_target* targets,
                                      uint32_t count,
                                      WGPUTextureFormat format,
                                      uint32_t width_px,
                                      uint32_t height_px);

                                                                                             
                                                                                               
                                                                                                 
                                                                                                   
pwlod_status pwlod_viewer_start(pwlod_viewer* viewer, pwlod_frame_ready_fn on_frame, void* user);

                                                                                            
                                                                                               
                                                                                              
                         
pwlod_status pwlod_viewer_acquire_latest(pwlod_viewer* viewer,
                                         uint32_t* out_target_index,
                                         uint64_t* out_frame_number);

                                                                              
pwlod_status pwlod_viewer_stop(pwlod_viewer* viewer);

                                                      
pwlod_status pwlod_viewer_get_stats(pwlod_viewer* viewer, pwlod_frame_stats* out_stats);

                                                                                              
                                                                                        
pwlod_status pwlod_viewer_render_once(pwlod_viewer* viewer,
                                      const pwlod_target* target,
                                      WGPUTextureFormat format,
                                      uint32_t width_px,
                                      uint32_t height_px,
                                      pwlod_frame_stats* out_stats);

void pwlod_viewer_destroy(pwlod_viewer* viewer);                                   

                                                                                                

typedef struct pwlod_build_report {
  uint64_t ply_points;                                          
  uint64_t tree_points;                                                                      
  uint64_t octree_bin_bytes;                                     
  int32_t nodes;
  double elapsed_ms;
} pwlod_build_report;

                                                                                          
                                                                                          
                                                                                                
                                                                             
pwlod_status pwlod_build_from_ply(const char* ply_path,
                                  const char* out_dir,
                                  const char* chunk_dir,
                                  int32_t memory_budget_mb,
                                  int32_t threads,
                                  pwlod_build_report* report,
                                  char* err_buf,
                                  uint32_t err_buf_len);

                                                                                          
                                                                                              
                                                                                     
                                                                                      
                                                                                 
typedef struct pwlod_verify_report {
  uint64_t tree_points;
  uint64_t octree_bin_bytes;
  int32_t nodes;
  int32_t leaves;
  int32_t leaves_selected;                               
  int64_t byte_gaps;                             
  int64_t byte_overlaps;                         
} pwlod_verify_report;

pwlod_status pwlod_verify_octree(const char* octree_dir, pwlod_verify_report* out_report);

                                                                                            
                                                                                         
                                                                                       

                                                                                   
const char* pwlod_version(void);

#ifdef __cplusplus
}
#endif
