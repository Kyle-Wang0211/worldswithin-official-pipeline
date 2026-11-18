#pragma once
#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
#define RECONSTRUCTION_RAW_EXPORT __attribute__((visibility("default")))
#else
#define RECONSTRUCTION_RAW_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Plane order: depth/conf0/conf1/conf2. Caller owns all buffers until return. */
typedef struct RECONSTRUCTIONOriginalPlane { const float *data; uint32_t width, height; } RECONSTRUCTIONOriginalPlane;
typedef struct RECONSTRUCTIONOriginalMutablePlane { float *data; uint32_t width, height; } RECONSTRUCTIONOriginalMutablePlane;
typedef struct RECONSTRUCTIONOriginalFusionPack {
    const float *banks[4]; /* each frame-major [97][1152][1536] float32 */
    const float *cams_f32; /* original [97][36], never native recomputed K/E */
    const uint8_t *rgb_u8; /* restricted diagnostic predecoded RGB fallback */
    const double *frame_ids_f64; /* metadata only: ordered capture frame IDs */
    uint32_t frame_count, width, height, rgb_width, rgb_height;
    const char *const *original_jpeg_paths; /* preferred97 ordered original absolute JPEG paths; RGB fields may be NULL/0 */
} RECONSTRUCTIONOriginalFusionPack;
/* Synchronous process-owned CPython3.10.21/GIL calls. No runtime initialization.
 * Return0 means call completed, never numeric acceptance or device parity.
 * merge mask count is observational only, no arithmetic/output modification. */
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_merge_frame(const RECONSTRUCTIONOriginalPlane low[4],
    const RECONSTRUCTIONOriginalPlane high[4], const RECONSTRUCTIONOriginalMutablePlane output[4],
    uint32_t frame_index, uint64_t *high_kept_count);
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_filter_pair(const RECONSTRUCTIONOriginalPlane *reference,
    const RECONSTRUCTIONOriginalPlane *source, const double *Kr9, const double *Er16,
    const double *Ks9, const double *Es16, double depth_max, double depth_min,
    uint8_t *geo_mask, float *depth_reprojected, float *x_source, float *y_source);
/* Exact selected original cache producer. Output folder must be fresh.
 * Emits97 d16/col.u8/camera.f64le/metadata records plus original order.json.
 * fused_ply_path MUST be NULL: selected archived source has no fusedPLY producer.
 * Existing original eager D array copies are preserved; mobile RSS not accepted.
 * kept_total is the original main-body tot[2], no projected point substitute. */
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fuse_pack(const RECONSTRUCTIONOriginalFusionPack *pack,
    const char *retained_directory, const char *fused_ply_path, uint64_t *kept_total);
/* Pre-model gate: exact source pins plus real pinned NumPy/cv2/Pillow imports. */
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_dense_dependencies_validate(void);
RECONSTRUCTION_RAW_EXPORT const char *reconstruction_original_dense_raw_last_error(void);
#ifdef __cplusplus
}
#endif
