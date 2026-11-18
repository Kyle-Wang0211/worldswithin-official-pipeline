#pragma once
#include <stddef.h>
#include <stdint.h>
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Opaque ownership wrappers around unchanged upstream VDBVolume and extracted vectors. */
typedef struct reconstruction_vdb_volume reconstruction_vdb_volume;
typedef struct reconstruction_vdb_mesh reconstruction_vdb_mesh;

/* Frozen reconstruction settings: float32(0.003), float32(0.04), space_carving=false. */
RECONSTRUCTION_EXPORT int reconstruction_vdb_create(reconstruction_vdb_volume** out);
RECONSTRUCTION_EXPORT void reconstruction_vdb_destroy(reconstruction_vdb_volume* volume);

/* Already-preprocessed, row-major FP64 world XYZ and FP64 original camera center.
 * Caller must preserve original per-view/order and NumPy preprocessing identity.
 * No preprocessing, batching, filtering or sampling occurs in this ABI. */
RECONSTRUCTION_EXPORT int reconstruction_vdb_integrate_xyz(reconstruction_vdb_volume* volume, const double* xyz,
                          size_t point_count, const double origin[3]);

typedef struct {
    size_t width;
    size_t height;
    /* Exact original cache K and E as row-major FP64, expanded from cams.f32. */
    double intrinsics[9];
    double extrinsics[16];
} reconstruction_tsdf_frame;
/* Per-view raw d16 stream: row-major little-endian uint16, exactly width*height*2.
 * Caller invokes in original order.json order. This source-mapped preprocessing
 * adapter is subject to a separate XYZ/mesh phone byte gate against NumPy 1.26.4.
 * No cross-platform arithmetic identity is claimed by this declaration. */
RECONSTRUCTION_EXPORT int reconstruction_vdb_integrate_d16_file(reconstruction_vdb_volume* volume, const char* d16_path,
                               const reconstruction_tsdf_frame* frame, size_t* integrated_points);

/* Reconstruction extraction: official default fill_holes=true; product parameter min_weight=0.45.
 * This is raw upstream extraction, BEFORE mandatory Trimesh processing/export. */
RECONSTRUCTION_EXPORT int reconstruction_vdb_extract_raw(reconstruction_vdb_volume* volume, reconstruction_vdb_mesh** out);
RECONSTRUCTION_EXPORT void reconstruction_vdb_mesh_destroy(reconstruction_vdb_mesh* mesh);
RECONSTRUCTION_EXPORT size_t reconstruction_vdb_mesh_vertex_count(const reconstruction_vdb_mesh* mesh);
RECONSTRUCTION_EXPORT size_t reconstruction_vdb_mesh_face_count(const reconstruction_vdb_mesh* mesh);
RECONSTRUCTION_EXPORT const double* reconstruction_vdb_mesh_vertices(const reconstruction_vdb_mesh* mesh);
RECONSTRUCTION_EXPORT const int32_t* reconstruction_vdb_mesh_faces(const reconstruction_vdb_mesh* mesh);

/* Trimesh is mandatory. No raw mesh PLY-export function is intentionally exposed. */
RECONSTRUCTION_EXPORT const char* reconstruction_tsdf_last_error(void);
#ifdef __cplusplus
}
#endif
