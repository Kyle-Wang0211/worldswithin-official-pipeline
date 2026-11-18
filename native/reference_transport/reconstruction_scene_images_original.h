#pragma once
#include <stdint.h>
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Image-only ABI1. Synchronous, worker-thread only. Exactly97 borrowed absolute
 * JPEG paths/SHA256 strings in caller-frozen order; manifest bytes also hashed.
 * Output directory must not exist, parent must exist. Creates numbered JPGs
 * only, never CAM files. 0 means97 images,22 invalid input,1 IO/OpenCV failure.
 * completed_images is updated after each actual write; partials stay on failure.
 * Original recorded recipe: COLOR|IGNORE_ORIENTATION,1920x1440,AREA,JPEG95.
 * Official4.11 iOS calls do not attest original A100 JPEG/CPU or byte parity.
 */
#define RECONSTRUCTION_SCENE_IMAGE_ABI 1
RECONSTRUCTION_EXPORT int reconstruction_scene_image_abi_version(void);
RECONSTRUCTION_EXPORT const char *reconstruction_scene_images_opencv_version(void);
RECONSTRUCTION_EXPORT const char *reconstruction_scene_images_build_information(void);
RECONSTRUCTION_EXPORT const char *reconstruction_scene_images_last_error(void);
RECONSTRUCTION_EXPORT int reconstruction_scene_images_prepare(const char *const *ordered_jpeg_paths,
    const char *const *ordered_sha256, uint64_t image_count,
    const char *source_manifest_path, const char *source_manifest_sha256,
    const char *fresh_output_directory, uint64_t *completed_images);
#ifdef __cplusplus
}
#endif
