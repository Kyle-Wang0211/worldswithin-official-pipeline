                                                                                             
                                                                                              
                                                                            
  
                                                                                                 
                                                                                                 
                                                                  
  
                                                                                  
                                                                                            
                                                                                           
                                                                                           
                                                                                     
                                                                     
  
                                                                                           
                                                                               
                           
#ifndef PW_LOD_SURFACE_H_
#define PW_LOD_SURFACE_H_

#include <CoreVideo/CoreVideo.h>
#include <stdint.h>

#include "../../vendor/aether_lod/include/pw_lod_bench.h"
#include "../../vendor/aether_lod/include/pwlod_viewer.h"

#ifdef __cplusplus
extern "C" {
#endif

                                                                                    
                                                                                         
                                                                                    
                                                     
                                                                                              
                                                                                             
                                                                            
                                                              
                                                          
                                                                             
                                                                            
                                                                              
pwlod_status PwLodSurfaceCreateGpu(pwlod_gpu *out_gpu);

typedef struct PwLodSurfaceRing PwLodSurfaceRing;             

                                                                                          
                                                                                                 
                                                           
PwLodSurfaceRing *PwLodSurfaceRingCreate(WGPUDevice device,
                                         uint32_t width,
                                         uint32_t height,
                                         char *err_buf,
                                         uint32_t err_buf_len);

                                                                                     
                             
const pwlod_target *PwLodSurfaceRingTargets(const PwLodSurfaceRing *ring);

                                                                                   
WGPUTextureFormat PwLodSurfaceRingFormat(void);

                                                                                     
CVPixelBufferRef PwLodSurfaceRingPixelBuffer(const PwLodSurfaceRing *ring, uint32_t index)
    CF_RETURNS_NOT_RETAINED;

                                                                                          
                                                      
void PwLodSurfaceRingDestroy(PwLodSurfaceRing *ring);

#ifdef __cplusplus
}
#endif

#endif                      
