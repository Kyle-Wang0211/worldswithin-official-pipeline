                                                                                           
                                                                                                              
                                                                                                 
                                                                                      
  
                                                                                                              
                                                                                                              
                                                                                                     
                                                                                                              
                                                                                                            
                                                                                                            
                                                                                          
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "dense_fuse.h"
#include "dense_images.h"
#include "dense_fuse_pack.h"
#include "dense_session.h"
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
#include "dense_view_segments.h"
#endif

namespace aether::dense {

struct DenseJob {
    std::vector<SessionFrame> frames;                                            
    std::vector<std::string> jpeg_paths;                                                
                                                                                                               
                                                                                                             
                                                                               
    std::vector<FrameSource> sources;
    std::vector<float> points;                                                               
    SessionParams session;
    FuseParams fuse;
    std::string model_path;                                        
                                                                                                                  
                                                                                                               
                                                                                                     
                                                                                                         
    std::string feat_model_path, rest_model_path;
    bool webgpu = true;
    std::string work_dir;                                                                                     
    std::string out_ply;
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
                                                                              
                                                                              
    std::string full_pipeline_fused_depth_dir;
    std::vector<std::string> original_fusion_jpeg_paths;
#endif
    uint64_t noise_seed = 0x5EEDDEE5ULL;
                                                                                                                       
                                                                                                                        
                                                                                                                      
    bool has_box = false;
    double box_c[3] = {0, 0, 0}, box_s[3] = {0, 0, 0};
    double box_rot[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
                                                                                                                        
                                                                                                                         
                                                                                                                               
                                                                                                                          
                                                                                                                       
                                                                                                                      
                                                                                                                        
    std::string hi_model_path;
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
                                                                            
                                                                          
    std::vector<DenseViewSegmentPlan> view_segment_plan, view_segment_plan_hi;
    int view_segment_intra_op_threads = 1;
    bool view_segment_native_output_locations = false;
    bool view_segment_disable_cpu_mem_arena = false;
    bool view_segment_disable_graph_optimizations = false;
#endif
                                                 
    int ref_begin = 0, ref_end = -1;                                                                           
    const float* ext_noise = nullptr;                                                                                         
                                                                                                                              
                                                                                                                          
                                                                                                       
    const SessionTable* table_hook = nullptr;
    const float* ext_noise_hi = nullptr;                                                                                          
                                                                                   
                                                                                  
                                                                             
    int ext_noise_fd = -1, ext_noise_hi_fd = -1;

    const float* ref_depth = nullptr;                                                                                               
};

struct DenseStats {
    int NF = 0, inferred = 0, images = 0;
    int frames_selected = 0; bool box_fallback = false;                                                     
    double session_ms = 0, images_ms = 0, ort_session_ms = 0, infer_ms_median = 0, infer_ms_total = 0, fuse_ms = 0;
                                                                                                                
                                                                                                               
                                                                                                               
                                                                                                       
    int image_threads = 0; double fuse_wait_ms = 0;
                                                                                                        
                                                                                                 
    int fuse_threads = 0; size_t fuse_peak_outstanding = 0;
                                                                                                              
                                                                                                             
    double feat_ms_total = 0; int feat_count = 0;
                                                                                                                         
                                                                             
    double ort_hi_session_ms = 0, infer_hi_ms_median = 0, infer_hi_ms_total = 0, merge_ms = 0;
    size_t mr_pixels = 0, mr_hi_kept = 0;
                                                                      
    size_t parity_pixels = 0, parity_bad1 = 0, parity_nonfinite = 0; double parity_worst_rel = 0;
    FusePackStats fuse;
    std::string error;
};

                                                                                                          
                                                                    
                                                                                                               
                                                                                                      
                                                         
typedef int (*dense_chunk_fn)(int frame_index, const float* xyz, const uint8_t* rgb, int n_points, void* user);

                                                                           
                                                                                                 
int dense_run(const DenseJob& job, dense_progress_fn progress, dense_chunk_fn chunk, void* user, DenseStats* stats);

                                                                                                    
inline int dense_run(const DenseJob& job, dense_progress_fn progress, void* user, DenseStats* stats) {
    return dense_run(job, progress, (dense_chunk_fn) nullptr, user, stats);
}

}                            
