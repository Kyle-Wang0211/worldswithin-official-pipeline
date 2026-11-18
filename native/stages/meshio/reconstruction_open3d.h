#pragma once
#include <stddef.h>
#include <stdint.h>
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    size_t cluster_count;
    size_t clusters_kept;
    size_t faces_before;
    size_t faces_removed;
    size_t vertices_after;
    size_t faces_after;
} reconstruction_open3d_clean_stats;
typedef struct reconstruction_open3d_mesh reconstruction_open3d_mesh;
RECONSTRUCTION_EXPORT int reconstruction_open3d_read_ply(const char* input_ply, reconstruction_open3d_mesh** out);
RECONSTRUCTION_EXPORT int reconstruction_open3d_from_raw(const double* xyz, size_t vertex_count,
                        const int32_t* faces, size_t face_count,
                        reconstruction_open3d_mesh** out);
RECONSTRUCTION_EXPORT void reconstruction_open3d_mesh_destroy(reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT size_t reconstruction_open3d_vertex_count(const reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT size_t reconstruction_open3d_face_count(const reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT const double* reconstruction_open3d_vertices(const reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT const int32_t* reconstruction_open3d_faces(const reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT int reconstruction_open3d_write_ply(const char* output_ply, const reconstruction_open3d_mesh* mesh);
RECONSTRUCTION_EXPORT int reconstruction_open3d_has_vertex_colors(const reconstruction_open3d_mesh* mesh);
/* Borrowed original Open3D FP64 colors, same original vertex order; NULL if absent. */
RECONSTRUCTION_EXPORT const double* reconstruction_open3d_vertex_colors(const reconstruction_open3d_mesh* mesh);
/* av_vis_k.py exact selected source: (vertex_colors*255).astype(uint8),
 * otherwise NVx3 uint8 filled with 200. NEVER raw PLY color bytes or Open3D's
 * rounded/clamped ColorToUint8 writer helper. NULL out_rgb/capacity0 queries size.
 * Selected color invariant: original normalized finite byte-RGB in [0,1].
 * A value outside that source invariant fails, rather than inventing a policy. */
RECONSTRUCTION_EXPORT int reconstruction_open3d_av_vis_colors(const reconstruction_open3d_mesh* mesh,
                                        uint8_t* out_rgb, size_t capacity_bytes,
                                        size_t* required_bytes, int* has_vertex_colors);
/* Original o3d_tutorial_clean.py pipeline and Open3D default binary PLY writer. */
RECONSTRUCTION_EXPORT int reconstruction_open3d_clean_ply(const char* input_ply, const char* output_ply,
                         reconstruction_open3d_clean_stats* stats);
/* Official Open3D default PLY read/write only; shared format bridge for geometry. */
RECONSTRUCTION_EXPORT int reconstruction_open3d_rewrite_ply(const char* input_ply, const char* output_ply);
RECONSTRUCTION_EXPORT const char* reconstruction_open3d_last_error(void);
#ifdef __cplusplus
}
#endif
