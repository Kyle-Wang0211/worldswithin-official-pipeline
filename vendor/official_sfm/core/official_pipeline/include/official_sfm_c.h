// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.




































#ifndef WORLDSWITHIN_OFFICIAL_SFM_C_H
#define WORLDSWITHIN_OFFICIAL_SFM_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

                                                                                                                                                                                   
                                                               
typedef enum aether_sfm_result {
  AETHER_SFM_OK = 0,
  AETHER_SFM_ERR_INVALID_ARG = 1,
  AETHER_SFM_ERR_DB = 2,
  AETHER_SFM_ERR_EXTRACT = 3,
  AETHER_SFM_ERR_NO_INITIAL_PAIR = 4,
  AETHER_SFM_ERR_NOT_REGISTERED = 5,
  AETHER_SFM_ERR_INTERNAL = 6,
  AETHER_SFM_ERR_UNSUPPORTED = 7,                                      
} aether_sfm_result_t;

typedef struct aether_sfm_session aether_sfm_session_t;           

typedef struct aether_sfm_options {
  int max_features;                               
  int image_width;                                   
  int image_height;
  float match_max_ratio;                                                     
  int use_gpu_match;                                                                
  int k_neighbors;                                                        
                                                                               
                                                                            
                                                                              
                                                                               
  int use_gpu_extract;                                                   
                                                                                   
                                                                            
} aether_sfm_options_t;

                                                                                                                                                                       
                                                                         

                                                                          
                                                                     
                                                                    
                                                                          
                                                                            
                                                                         
                                                     

                                                                          
                                                                     
                                                                           
                                                                            
                                                                         
                                                                             
                                                       

                                                                               
                                                                                          
                                                                              
  
                                                                           
                                      
                                                                             
                                                                               
                                                                          
                                                                              
                                                                           
                                                                             
                                                                                  
                                                                             
                                                                        
                                                  
  
                                                                                   
                                                                         
                                                                               
                                                                                 
                                                                                     
  
                           
                                                                          
                                                                          

                                                                              
                                                                           
                                                     

                                                                                                           
                                                                                     
typedef enum pwofficial_finalize_status {
  AETHER_SFM_FINALIZE_IDLE = 0,                       
  AETHER_SFM_FINALIZE_LOCAL_READY = 1,                                     
  AETHER_SFM_FINALIZE_REFINED = 2,                                                 
  AETHER_SFM_FINALIZE_ERROR = 3,                            
} pwofficial_finalize_status_t;

                                                                                    
                                                                            
                                                                         
                                                                    
                                                                             
                                                                         
                                                                           
                                                      
  
                                                                         
                                                                               
                                                                            
                                                                           
                                                                           
                                                                            
                                                                         
                                                                           
                                                 

                                                                              

                                                                                                                       
typedef struct aether_sfm_pose {
  int frame_id;                                           
  int registered;                               
  double qwxyz[4];                                               
  double t[3];                                 
} aether_sfm_pose_t;

                                                                                 
                                                                   

typedef struct aether_sfm_point {
  float x, y, z;
  uint8_t r, g, b;
  uint8_t _pad[2];
} aether_sfm_point_t;

                                                                            
                                                                            
                                                                            
                   

                                                                                                                                             
                                                                             
                                                                                 
                                                                       
                                                                            
                                                                                
                                                                           

                                                                                                  
                                                                         
                                                                           
                                                                     
                                                                            
                                                                            
                                          
typedef struct aether_sfm_track_obs {
  int32_t frame_id;                                                      
  float x, y;                                                         
} aether_sfm_track_obs_t;

                                                           
                                                                       
                                                                            
                                                                              
                                   
                                                        
                                                                      
                                                                         

                                                                         
                                                                               
                                                                               
                                                                      
                                                                             
                                                                               

                                                                         
                                                                              
                                                                              
                                                                      

                                                                   
                                                          
                                                              
                                                                           
                                                                       
                                                                         
                                                                      

                                                                                 
                                                                              
                                                                        
                                                                               
                                                                             

                                                                        
                                                                           
                                                                            
                                                                          
                                                                  


                                                                              
                                                                              
                                                                               
                                                                             
                                                                      

                                                                        
                                                                         
                                                                          
                                                                          
                                                                             
                                                                   
                                                                                
                                                                           
                                                                           
                                                                       
                                                             
                                                    

                                                                         
                                                                          
                                                                              
                                                                           
                                                                                      
                                                                            
                                                                             
                                                                  
                                                                                
                                                                            
                                                                                      
                                                                       

                                                                              
                                                                        
                                               

                                                                         
                                                                           
                                                                              
                                                                         
                                                                              
                                                                               
                                                                              
                                                                             
                                                                         
                                                                             
                                                                           
                                                                           
                                                                         
                                                                   

                                                                     
                                                                         
                                                                     
                                                                                
                                                                         
                                                                                   
                                                                
                                                    

                                                                               
                                                                          
                                                                             
                                                                            
                                        

                                                                             
                                                                                       
                                                                            
                                                                    
                                                                           
                                                                          
                                                                          
                                                                   
                                                           
                                                                               
                                                                             
                                                                              
                                                                             
                                                                       
                                        

                                                                       
                                                                             
                                                                              
                                                                         
                                                              

                                              

                                                                                                                                           
                                                                               
                                                                           
                                                                         
                                                                               
                                                                               

                                                                            
                                                                          
                                                                              
                              

                                                                               
        

                                                               
aether_sfm_result_t pwofficial_add_frame(aether_sfm_session_t* s,
                                         const uint8_t* gray,
                                         int width, int height,
                                         float fx, float fy,
                                         float cx, float cy,
                                                                             
                                                                      
                                                                              
                                                                              
                                                                       
                                         const double pose_qwxyz[4],
                                         const double pose_t[3],
                                         int* out_frame_id);

aether_sfm_result_t pwofficial_arbitrate(aether_sfm_session_t* s,
                                         char* out_json, int out_cap);

void pwofficial_candidate_stats(aether_sfm_session_t* s,
                                int64_t* spatial_first_pairs,
                                int64_t* temporal_fallback_pairs);

aether_sfm_result_t pwofficial_create(const char* db_path,
                                      const aether_sfm_options_t* options,
                                      aether_sfm_session_t** out_session);

void pwofficial_debug_last(aether_sfm_session_t* s, double* extract_ms,
                           double* match_ms, int* n_cand, int* gpu_matches,
                           int* cpu_matches);

aether_sfm_result_t pwofficial_finalize_async(aether_sfm_session_t* s,
                                              char* out_json, int out_cap);

int pwofficial_finalize_status(aether_sfm_session_t* s);

int pwofficial_finalize_progress(aether_sfm_session_t* s, int* stage,
                                 int* round, int* iter, int* max_iter);

void pwofficial_free(aether_sfm_session_t* s);

aether_sfm_result_t pwofficial_get_points(aether_sfm_session_t* s,
                                          aether_sfm_point_t** out_points,
                                          int* out_count);

aether_sfm_result_t pwofficial_get_points_tracked(
    aether_sfm_session_t* s,
    aether_sfm_point_t** out_points,
    int* out_count,
    int32_t** out_obs_offsets,
    aether_sfm_track_obs_t** out_obs,
    int64_t* out_obs_count);

aether_sfm_result_t pwofficial_get_poses(aether_sfm_session_t* s,
                                         aether_sfm_pose_t* out_poses,
                                         int cap, int* out_count);

aether_sfm_result_t pwofficial_get_preview_points(aether_sfm_session_t* s,
                                                  float* out_xyz, int cap,
                                                  int* out_count);

aether_sfm_result_t pwofficial_get_preview_tracked(
    aether_sfm_session_t* s,
    aether_sfm_point_t** out_points,
    int* out_count,
    int32_t** out_obs_offsets,
    aether_sfm_track_obs_t** out_obs,
    int64_t* out_obs_count);

aether_sfm_result_t pwofficial_global_refine(aether_sfm_session_t* s);

int pwofficial_live_repay(aether_sfm_session_t* s, int max_pairs);

void pwofficial_match_fail_stats(aether_sfm_session_t* s,
                                 int64_t* gpu_fail_total,
                                 int64_t* gpu_fail_by_rc,
                                 int64_t* gpu_fail_max_streak,
                                 int64_t* rematch_starved_frames,
                                 int64_t* rematch_candidates,
                                 int64_t* rematch_attempted,
                                 int64_t* rematch_written,
                                 int64_t* rematch_inliers,
                                 int64_t* rematch_failed);

void pwofficial_options_default(aether_sfm_options_t* out);

void pwofficial_points_free(aether_sfm_point_t* points);

aether_sfm_result_t pwofficial_remove_frame(aether_sfm_session_t* s,
                                            int frame_id,
                                            char* out_json, int out_cap);

void pwofficial_repair_stats(aether_sfm_session_t* s, int64_t* repay_calls,
                             int64_t* repay_attempted, int64_t* repay_written,
                             int64_t* repay_inliers, int64_t* repay_failed,
                             int64_t* repay_skipped_thermal,
                             int64_t* gpu_retry_attempts,
                             int64_t* gpu_retry_recovered,
                             int64_t* enrich_budget_stopped);

aether_sfm_result_t pwofficial_run(const char* db_path,
                                   const char* image_path,
                                   const aether_sfm_options_t* options,                
                                   aether_sfm_session_t** out_session,                 
                                   char* out_json, int out_cap);

                                                                                
aether_sfm_result_t pwofficial_write_model(aether_sfm_session_t* s,
                                           const char* out_dir);

aether_sfm_result_t pwofficial_continue_from_model(
    const char* db_path, const char* image_path, const char* model_in_path,
    const char* model_out_path, const aether_sfm_options_t* options,
    aether_sfm_session_t** out_session, char* out_json, int out_cap);

void pwofficial_set_thermal_state(aether_sfm_session_t* s, int state);

void pwofficial_stream_stats(aether_sfm_session_t* s, int64_t* tvg_pairs,
                             int64_t* raw_pairs, int64_t* grow_accepted,
                             int64_t* grow_rejected, int64_t* reproj_filtered,
                             int64_t* tri_filtered,
                             int64_t* grow_reject_cheirality,
                             int64_t* grow_reject_reproj,
                             int64_t* create_reject_cheirality,
                             int64_t* create_reject_tri_angle,
                             int64_t* create_reject_reproj,
                             int64_t* already_assigned,
                             int64_t* merge_needed,
                             int64_t* merge_accepted,
                             int64_t* merge_rejected,
                             int64_t* spatial_considered,
                             int64_t* spatial_attempted,
                             int64_t* spatial_written,
                             int64_t* spatial_inliers,
                             int64_t* spatial_anchor_attempted,
                             int64_t* spatial_anchor_passed,
                             int64_t* spatial_regions_confirmed,
                             int64_t* spatial_expanded_attempted,
                             int64_t* spatial_guided_pairs,
                             int64_t* spatial_guided_inliers,
                             int64_t* spatial_quadratic_attempted,
                             int64_t* spatial_quadratic_written,
                             int64_t* spatial_budget_skipped,
                             int64_t* temporal_detail_pairs,
                             int64_t* temporal_detail_matches,
                             int64_t* temporal_detail_created,
                             int64_t* temporal_detail_grown,
                             int64_t* temporal_detail_reject_cheirality,
                             int64_t* temporal_detail_reject_reproj,
                             int64_t* temporal_detail_reject_tri_angle,
                             int64_t* temporal_detail_conflicts);

void pwofficial_thermal_throttle_stats(aether_sfm_session_t* s,
                                       int64_t* throttled_frames);

void pwofficial_track_obs_free(int32_t* offsets, aether_sfm_track_obs_t* obs);

int pwofficial_telemetry(double* phys_footprint_mb,
                         double* footprint_peak_mb,
                         int* thermal_state);

#ifdef __cplusplus
}               
#endif

#endif                                 
