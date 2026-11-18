#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Adapter entry points for the frozen, selected button-38 recipe. */
int reconstruction_meshopt_simplify_files(const char* input_positions_f32,
                              const char* input_indices_u32,
                              const char* output_positions_f32,
                              const char* output_indices_u32);
int reconstruction_lagrange_fill_ply(const char* input_ply, const char* output_ply,
                          uint64_t* input_vertices, uint64_t* input_facets,
                          uint64_t* output_vertices, uint64_t* output_facets);
const char* reconstruction_geometry_last_error(void);
#ifdef __cplusplus
}
#endif
