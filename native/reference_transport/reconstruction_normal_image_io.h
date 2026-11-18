#pragma once
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Requires existing reconstruction_glb_runtime_initialize to have succeeded.
 * Synchronous IO-only call. Manifest is97 ordered records; all paths absolute.
 * Output directory and receipt must not exist. Existing parent dirs required.
 * 0=all97 prepared;22=invalid C arguments;1=runtime/Python/IO failure.
 * Failure artifacts are preserved; never retry into the same directory.
 */
RECONSTRUCTION_EXPORT int reconstruction_normal_images_prepare(const char *input_manifest_json_path,
    const char *output_directory, const char *receipt_path);
RECONSTRUCTION_EXPORT const char *reconstruction_normal_images_last_error(void);
#ifdef __cplusplus
}
#endif
