                                                                                                                   
                                                                                                             
                                                                                                             
                                                                                               
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWDENSE_API __attribute__((visibility("default")))
#define PWDENSE_ABI_VERSION 3

typedef struct pwdense_frame_t {
    int32_t frame_id;                                                                                      
    double fx, fy, cx, cy;                                                                
    double image_w, image_h;                                  
    double q_wxyz[4];                                                                       
    double t[3];                              
    const char* jpeg_path;                                          
} pwdense_frame_t;

typedef struct pwdense_options_t {
    int32_t width, height;                                    
    int32_t nsrc;                                                 
    int32_t webgpu;                                                                
    const char* model_path;                                                                     
    const char* work_dir;                                                                
    const char* out_ply;                                                                                     
    uint64_t noise_seed;
                                                                                                            
                                                                                                               
                                                                       
    int32_t has_box;
    double box_center[3];
    double box_size[3];
    double box_rot[9];
} pwdense_options_t;

                                                                                               
typedef int (*pwdense_progress_fn)(const char* phase, int32_t done, int32_t total, void* user);

typedef struct pwdense_stats_t {
    int32_t frames, inferred, images;
    int32_t frames_selected;                                                                                   
    int32_t box_fallback;                                                                                
    double session_ms, images_ms, ort_session_ms, infer_ms_median, infer_ms_total, fuse_ms;
    uint64_t points;
    double photo_frac, geo_frac, final_frac;
    char error[256];
} pwdense_stats_t;

PWDENSE_API int32_t pwdense_abi_version(void);
PWDENSE_API int32_t pwdense_available(void);                                                                        
PWDENSE_API int32_t pwdense_options_default(pwdense_options_t* o);                                                
PWDENSE_API const char* pwdense_default_model_path(void);                                                                 
                                                                                                          
                                                                                                             
                                                                                                                 
                                                                                                                 
                                                                                                           
                                                                                                              
                                
typedef int (*pwdense_chunk_fn)(int32_t frame_index, const float* xyz, const uint8_t* rgb, int32_t n_points,
                                void* user);

                                                                                              
PWDENSE_API int32_t pwdense_run(const pwdense_frame_t* frames, int32_t n_frames,
                                const float* points_xyz, int32_t n_points,
                                const pwdense_options_t* opts,
                                pwdense_progress_fn progress, void* user,
                                pwdense_stats_t* out_stats);
                                                                                                          
                                                                                                              
                                                                                                  
                                                                                                       
                                                                                                             
                                                                                                                    
                                                                    
typedef struct pwdense_frame_v3_t {
    int32_t frame_id;
    double fx, fy, cx, cy;
    double image_w, image_h;
    double q_wxyz[4];
    double t[3];
    const char* jpeg_path;                                                       
    const char* nv12_path;                                
    int32_t nv12_width, nv12_height;
    int32_t nv12_matrix;
    int32_t reserved0;                        
} pwdense_frame_v3_t;

                                                                                                   
PWDENSE_API int32_t pwdense_run2(const pwdense_frame_t* frames, int32_t n_frames,
                                 const float* points_xyz, int32_t n_points,
                                 const pwdense_options_t* opts,
                                 pwdense_progress_fn progress, pwdense_chunk_fn chunk, void* user,
                                 pwdense_stats_t* out_stats);
                                                                                                   
PWDENSE_API int32_t pwdense_run3(const pwdense_frame_v3_t* frames, int32_t n_frames,
                                 const float* points_xyz, int32_t n_points,
                                 const pwdense_options_t* opts,
                                 pwdense_progress_fn progress, pwdense_chunk_fn chunk, void* user,
                                 pwdense_stats_t* out_stats);

#ifdef __cplusplus
}
#endif
