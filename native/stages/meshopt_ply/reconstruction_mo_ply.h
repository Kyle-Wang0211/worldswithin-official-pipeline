#pragma once
#include <stdint.h>
#if defined(__GNUC__)
#define RECONSTRUCTION_MO_PLY_API __attribute__((visibility("default")))
#else
#define RECONSTRUCTION_MO_PLY_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
                                                                                  
                                                                                      
                                                                              
                                                                                     
RECONSTRUCTION_MO_PLY_API int reconstruction_meshopt_simplify_ply(const char* input_ply,
    const char* output_ply, const char* positions_f32, const char* indices_u32,
    const char* output_positions_f32, const char* output_indices_u32,
    uint64_t* input_vertices, uint64_t* input_faces,
    uint64_t* output_vertices, uint64_t* output_faces);
RECONSTRUCTION_MO_PLY_API const char* reconstruction_mo_ply_last_error(void);
#ifdef __cplusplus
}
#endif
