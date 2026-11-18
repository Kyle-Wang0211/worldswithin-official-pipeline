#pragma once
#include <stdint.h>
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additive camera-only ABI1; process-owned pinned CPython runtime must already
 * be initialized by reconstruction_glb_runtime_initialize. No image producer or inference.
 * Borrowed selected768 camera buffer: exactly97*36 float32LE elements in frozen
 * manifest order. It remains alive and immutable until synchronous return.
 * Manifest and all output paths are absolute. Camera directories/receipt fresh,
 * parents existing; actual real JPEG identity/order is checked by Python IO.
 * Original4032 material reuses uncorrected768-normalized camera bytes; texture
 * cameras use the exact recorded principal-point correction. No JPGs produced.
 * 0=97 camera pairs generated;22=invalid C arguments;1=Python/runtime/IO failure.
 * Failure artifacts are retained. Success is never combined material completion.
 */
#define RECONSTRUCTION_SCENE_CAMERA_ABI 1
RECONSTRUCTION_EXPORT int reconstruction_scene_camera_abi_version(void);
RECONSTRUCTION_EXPORT int reconstruction_scene_cameras_prepare(const float *cameras_f32,
    uint64_t cameras_count, const char *ordered_manifest_json,
    const char *input_root, const char *original_camera_directory,
    const char *corrected_camera_directory, const char *receipt_path);
RECONSTRUCTION_EXPORT const char *reconstruction_scene_cameras_last_error(void);
#ifdef __cplusplus
}
#endif
