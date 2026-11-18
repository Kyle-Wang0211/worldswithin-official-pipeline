#pragma once
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define RECONSTRUCTION_AV_NUMPY_EXPORT __attribute__((visibility("default")))
#else
#define RECONSTRUCTION_AV_NUMPY_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Field layout matches the existing RECONSTRUCTIONNormalFrameInput/NormalFrame ABI.
 * All paths, FP64 arrays, and the frame table remain caller-owned until return. */
typedef struct RECONSTRUCTIONOriginalVisibilityFrame {
    const char* fused_d16_le_path;
    uint32_t width;
    uint32_t height;
    double intr_row_major[9];
    double extr_row_major[16];
    const char* original_camera_path;
} RECONSTRUCTIONOriginalVisibilityFrame;

/* Synchronous IO/GIL bridge, using the existing process-owned pinned interpreter.
 * XYZ and optional color values are the ordered original Open3D FP64 arrays.
 * has_vertex_colors is exactly 0 or 1; absent colors use the original NumPy
 * fallback. Frame order is the existing 97-frame order, without sorting.
 * SFm JSON and raw gate outputs must be fresh. No Python object retains caller
 * buffers after return. Return 0 means generated, never byte-gate acceptance. */
RECONSTRUCTION_AV_NUMPY_EXPORT int reconstruction_av_prepare_sfm_original_numpy(
    const double* vertices_xyz_f64, const double* vertex_colors_rgb_f64,
    uint64_t vertex_count, int32_t has_vertex_colors,
    const RECONSTRUCTIONOriginalVisibilityFrame* frames, uint32_t frame_count,
    const char* original_images_folder, const char* sfm_json_path,
    const char* gate_folder);
RECONSTRUCTION_AV_NUMPY_EXPORT const char* reconstruction_av_original_numpy_last_error(void);

#ifdef __cplusplus
}
#endif
