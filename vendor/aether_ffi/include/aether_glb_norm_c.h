// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.
//
// aether_glb_norm — client-side GLB normalizer.
//
// Goal: any GLB the user imports (Polycam / KIRI / Sketchfab download /
// hand-modeled / our own pipeline output) loads in <1 s on iOS /
// Android / HarmonyOS / Web. Today most third-party photogrammetry
// GLBs ship with 30-60 separate primitives × materials × atlases —
// each material costs Filament/Three.js a separate shader compile,
// and Filament's startup time on iPhone is dominated by these per-
// material compiles (5-9 s observed on a 64-prim scan).
//
// What this library does, given any input GLB:
//   1. Parse via cgltf, decode all baseColor PNGs via stb_image.
//   2. Pack the per-prim charts into a single power-of-2 atlas using
//      stb_rect_pack with chart-mean background + 8 px edge dilation
//      (mirrors aether_cpp/src/glb_norm/atlas_merger.cpp; algorithm
//      ported verbatim from server-side worker_object_slam3r_surface_v1
//      /pipeline/atlas_merger.py, the reference implementation).
//   3. If face count > target_face_count, run meshoptimizer's
//      simplify_quadric_decimation to bring it down.
//   4. Emit a single-prim, single-material GLB with explicit
//      metallicFactor=0 (else strict viewers default to 1.0 → black).
//
// Cross-platform: pure C++17, no platform deps. Vendored single-header
// libs (cgltf, stb_image, stb_image_write, stb_rect_pack) plus
// meshoptimizer (a small C++ library). Builds via the existing aether_cpp
// CMake into the same per-arch static lib that ships to iOS / Android /
// HarmonyOS, and via Emscripten to .wasm for the web client.
//
// Memory: caller owns the input bytes; the library streams its work
// through internal buffers and writes the output bytes into a caller-
// provided `aether_glb_norm_buffer_t`. The caller frees with
// aether_glb_norm_buffer_free(). This avoids heap-allocator mismatches
// across the FFI boundary.

#ifndef AETHER_GLB_NORM_C_H
#define AETHER_GLB_NORM_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

                                                                                                                                                                                   
                                                                     
                                                         
typedef enum aether_glb_norm_result {
    AETHER_GLB_NORM_OK = 0,
    AETHER_GLB_NORM_ERR_INVALID_GLB = 1,
    AETHER_GLB_NORM_ERR_NO_MATERIALS = 2,
    AETHER_GLB_NORM_ERR_NO_TEXTURES = 3,
    AETHER_GLB_NORM_ERR_PNG_DECODE = 4,
    AETHER_GLB_NORM_ERR_PACKING_FAILED = 5,
    AETHER_GLB_NORM_ERR_PNG_ENCODE = 6,
    AETHER_GLB_NORM_ERR_OUT_OF_MEMORY = 7,
    AETHER_GLB_NORM_ERR_CANCELLED = 8,
    AETHER_GLB_NORM_ERR_UNSUPPORTED = 9,
    AETHER_GLB_NORM_ERR_INTERNAL = 10,
    AETHER_GLB_NORM_ERR_KTX2_ENCODE = 11,
} aether_glb_norm_result_t;

                                                                                                                                                                 
                                                                        
                                                                        
                                                                        
                                                                         
                                                                 
                                                                          
                                                     
  
                                                                        
                                                                          
                                                                           
                                                                             
                                                                   
  
                                                                         
                                           
typedef enum aether_glb_norm_texture_format {
    AETHER_GLB_NORM_TEX_PNG = 0,
    AETHER_GLB_NORM_TEX_KTX2_ETC1S = 1,
    AETHER_GLB_NORM_TEX_KTX2_UASTC = 2,
} aether_glb_norm_texture_format_t;

                                                                                                                                                                                
typedef struct aether_glb_norm_options {
                                                                     
                                                                    
                                                                      
    int target_atlas_size;

                                                                      
                                                                     
                                                                        
    int max_atlas_size;

                                                                      
                                                            
                                                                 
                                                                      
    uint32_t target_face_count;

                                                                   
                                                               
    int allow_oversize_textures;

                                                 
                                                                          
                                                                       
                                                                            
    aether_glb_norm_texture_format_t texture_format;

                                                                  
                                                                    
                                                      
      
                                                                           
                                                                      
                                                                     
                                                                    
                                                                           
                                                               
                                                                      
                                                                  
      
                                                            
                                                                            
                                                                          
    int compress_geometry;
} aether_glb_norm_options_t;

                                                                    
                                            
void aether_glb_norm_options_default(aether_glb_norm_options_t* out);

                                                                                                                                                   
                                                                     
                                                                  
                           
typedef struct aether_glb_norm_buffer {
    uint8_t* data;
    size_t size;
} aether_glb_norm_buffer_t;

void aether_glb_norm_buffer_free(aether_glb_norm_buffer_t* buf);

                                                                                                                                                       
                                                                     
                                                            
                                                                   
                                                             
                                                                
  
                                                                    
                                                                
                                               
typedef int (*aether_glb_norm_progress_fn)(float fraction,
                                           const char* phase_label,
                                           void* user_data);

                                                                                                                                     
typedef struct aether_glb_norm_stats {
    uint32_t input_primitive_count;
    uint32_t input_material_count;
    uint32_t input_face_count;
    uint32_t output_primitive_count;                           
    uint32_t output_material_count;                            
    uint32_t output_face_count;
    int      output_atlas_size;
    float    elapsed_seconds;
} aether_glb_norm_stats_t;

                                                                                                                                                                           
                                                                        
                                                                      
                                                                      
  
                                                                    
                                                                      
                
  
                                                               
                                                                     
                                             
aether_glb_norm_result_t aether_glb_norm_run(
    const uint8_t* input_glb_bytes,
    size_t input_glb_size,
    const aether_glb_norm_options_t* options,
    aether_glb_norm_progress_fn progress_cb,                  
    void* progress_user_data,                                 
    aether_glb_norm_buffer_t* out_buffer,
    aether_glb_norm_stats_t* out_stats);                      

                                                                     
                                                                 
const char* aether_glb_norm_result_str(aether_glb_norm_result_t code);

#ifdef __cplusplus
}               
#endif

#endif                        
