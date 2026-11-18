#pragma once
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define RECONSTRUCTION_NORMAL_EXPORT __attribute__((visibility("default")))
#else
#define RECONSTRUCTION_NORMAL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// These adapters call the two untouched, pinned official CLI entry bodies.
// SfM JSON must already satisfy the av_vis_k.py / av_sfm_k.py byte gate.
RECONSTRUCTION_NORMAL_EXPORT int reconstruction_av_convert_sfm_to_abc(const char* sfm_json_path, const char* abc_path);
RECONSTRUCTION_NORMAL_EXPORT int reconstruction_av_bake_normal(const char* abc_path, const char* low_obj_path,
                       const char* high_ply_path, const char* images_folder,
                       const char* output_folder);

typedef struct RECONSTRUCTIONNormalFrameInput {
    const char* fused_d16_le_path;  // row-major uint16, exactly width*height*2 bytes
    uint32_t width;
    uint32_t height;
    double intr_row_major[9];      // original K values, exactly promoted to f64
    double extr_row_major[16];     // original w2c E values, exactly promoted to f64
    const char* original_camera_path;
} RECONSTRUCTIONNormalFrameInput;

// Port of the selected av_vis_k.py / av_sfm_k.py product adapters, not a paper
// algorithm. Caller must preserve original Open3D f64 PLY vertex order and color
// bytes. No mesh/image decoder is introduced. Exactly 97 ordered frames required.
// gate_folder receives raw f64/i64 buffers for the preregistered phone byte gate.
// Return 0 means files were generated; it DOES NOT mean the byte gate has passed.
RECONSTRUCTION_NORMAL_EXPORT int reconstruction_av_prepare_sfm_json(const double* vertices_xyz_f64,
                            const uint8_t* vertex_colors_rgb_u8,
                            uint64_t vertex_count,
                            const RECONSTRUCTIONNormalFrameInput* frames,
                            uint32_t frame_count,
                            const char* original_images_folder,
                            const char* sfm_json_path,
                            const char* gate_folder);

#ifdef __cplusplus
}
#endif
