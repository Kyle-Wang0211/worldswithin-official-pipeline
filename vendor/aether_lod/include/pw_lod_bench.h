                                                  
  
                                                                                
                                                                                  
                                                                                 
                                                                           
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct PwLodProbeSample {
                                                                              
                                             
  int thermal_state;
  double footprint_mb;                                            
  double avail_mb;                                                           
} PwLodProbeSample;

typedef void (*PwLodProbeFn)(PwLodProbeSample* out, void* ctx);

                                                                            
                                                               
                                                                              
                                                                          
const char* pwlod_run(const char* octree_dir, const char* out_dir,
                      const char* args, PwLodProbeFn probe, void* probe_ctx);

#ifdef __cplusplus
}
#endif
