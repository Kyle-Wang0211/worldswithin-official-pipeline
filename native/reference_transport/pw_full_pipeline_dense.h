#pragma once
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32)
#define PW_FULL_PIPELINE_API __declspec(dllexport)
#else
#define PW_FULL_PIPELINE_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Shared C ABI. No JSON, platform camera conversion, model discovery, or fallback.
   All pointer/string storage must remain alive until run returns. Run is blocking;
   Dart must call on a worker isolate. Chunk callbacks may run on a native fusion
   worker; callbacks must be native thread-safe or correctly marshalled to Dart.
   Until that marshalling is implemented, the mainline Dart caller passes both
   progress and chunk as NULL. Callback return != 0 cancels; data is call-local. */
#define PW_FULL_PIPELINE_DENSE_ABI 2
typedef struct pw_full_pipeline_dense_frame {
    double frame_id; /* exact original binary64 identity, never narrowed */
    double fx, fy, cx, cy, image_w, image_h;
    double q_wxyz[4]; /* refined world->camera COLMAP/OpenCV quaternion */
    double t[3];
    const char* jpeg_path; /* materialised input; no synthetic photo generation */
} pw_full_pipeline_dense_frame;

typedef struct pw_full_pipeline_dense_options {
    uint32_t abi_version, struct_size;
    int32_t width, height, nsrc, webgpu;
    int32_t geo_mask_thres, check_all_views, photo_first;
    float geo_pixel_thres, geo_depth_thres, photo_thres[3];
    float sat_tol, min_tri_angle_deg;
    /* Model identities/hashes belong to the caller's immutable run contract. */
    const char* low_model_path;
    const char* high_model_path;
    /* Required exact float32LE noise, sorted frame order. No generated-noise arm.
       97 * ((H/4)*(W/4) + (H/2)*(W/2)) floats, high uses 2W/2H. */
    const char* low_noise_f32_path;
    const char* high_noise_f32_path;
    const char* work_dir; /* new path, parent exists; pipeline creates its pack */
    const char* out_ply; /* must be NULL: selected original producer emits cache */
    const char* fused_depth_dir; /* new path; approved retention IO creates it */
    /* Required exact original precomputed camera/neighbor tables:97*36 floats
       and97*9 int32 values, both in original ascending frame array order.
       This strict profile does not rebuild a missing table with local formulas. */
    const float* cameras_f32;
    const int32_t* neighbors_i32;
    uint64_t cameras_count, neighbors_count;
    /* Exact original fx_official/rgb JPEG bytes, ascending original frame order.
       Separate from inference source photos; no pre-resized RGB substitution. */
    const char* const* original_fusion_jpeg_paths;
    uint64_t original_fusion_jpeg_count;
} pw_full_pipeline_dense_options;

typedef struct pw_full_pipeline_dense_result {
    int32_t status, frames, inferred, fused_frames;
    uint64_t points;
    double low_infer_ms, high_infer_ms, merge_ms, fuse_ms;
    char error[512];
    /* Always the exact requested paths; existing partial artifacts are preserved.
       out_ply is empty for this cache-only producer; verify actual97 cache/order bytes.
       points is original kept_total (pixels), not fabricated projected PLY points.
       This is dense-stage success, never TSDF/normal/texture/GLB success. */
    char out_ply[4096], fused_depth_dir[4096], order_json[4096], work_dir[4096];
} pw_full_pipeline_dense_result;

typedef int32_t (*pw_full_pipeline_dense_progress)(const char* phase, int32_t done,
                                           int32_t total, void* user);
typedef int32_t (*pw_full_pipeline_dense_chunk)(int32_t frame_index, const float* xyz,
                                        const uint8_t* rgb, int32_t points, void* user);
PW_FULL_PIPELINE_API int32_t pw_full_pipeline_dense_abi_version(void);
PW_FULL_PIPELINE_API const char* pw_full_pipeline_dense_producer_profile(void);
/* Sets the existing selected button-38 recipe; caller fills all input/output paths.
   No subset/box/split/raw-high/no-merge/no-fusion configuration is accepted. */
PW_FULL_PIPELINE_API int32_t pw_full_pipeline_dense_options_init(pw_full_pipeline_dense_options* options);
/* Exactly 97 frames sorted by unique increasing frame_id. Sparse xyz N*3 f32.
   All 97 references run at 768x576 + 1536x1152, CER merge then selected fusion.
   Chunk must be NULL: selected original producer runs ordered cache fusion. Returns 0 dense success, 1 cancelled, 2 input/IO, 3 model, 4 fusion.
   Exceptions are contained and returned as 2. Caller owns model/source/runtime
   identity verification and must not modify inputs concurrently. */
PW_FULL_PIPELINE_API int32_t pw_full_pipeline_dense_run(const pw_full_pipeline_dense_frame* frames,
    int32_t frame_count, const float* sparse_xyz, int32_t point_count,
    const pw_full_pipeline_dense_options* options, pw_full_pipeline_dense_progress progress,
    pw_full_pipeline_dense_chunk chunk, void* user, pw_full_pipeline_dense_result* result);
/* Additive session-table ABI, independent of dense run ABI 1. Uses the same
   selected options_init recipe and the unchanged build_session_table. No model,
   noise, image reads or dense inference. Every count is an element count.
   Caller supplies nonaliasing output buffers of exactly 97*36 f32 / 97*9 i32;
   outputs are written only after validation and successful table construction.
   Sparse count must equal point_count*3, with point_count > 0. Frame JPEG paths
   identify real source inputs, remain required, and are not opened here.
   Blocking: call on a worker isolate. Status 0 table success, 2 validation/native
   exception. Error is NUL terminated when error_capacity > 0; require >=512. */
#define PW_FULL_PIPELINE_SESSION_ABI 1
PW_FULL_PIPELINE_API int32_t pw_full_pipeline_session_abi_version(void);
PW_FULL_PIPELINE_API int32_t pw_full_pipeline_session_build(
    const pw_full_pipeline_dense_frame* frames, int32_t frame_count,
    const float* sparse_xyz, int32_t point_count, uint64_t sparse_count,
    const pw_full_pipeline_dense_options* options,
    float* cameras_f32, uint64_t cameras_count,
    int32_t* neighbors_i32, uint64_t neighbors_count,
    char* error, uint64_t error_capacity);
#ifdef __cplusplus
}
#endif
