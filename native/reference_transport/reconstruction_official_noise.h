#pragma once
#include <stddef.h>
#include <stdint.h>
#include "reconstruction_export.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original Torch 2.12 CPU float32 noise transport. Width is 768 or 1536.
 * Each frame resets seed=frame_index, draws stage2 (h/4,w/4), then stage3
 * (h/2,w/2), and emits unscaled native little-endian float32 C-order bytes.
 * Required floats are 138240 for width768 and 552960 for width1536.
 * Returns 0 on success, otherwise a positive errno code. No C++ exception escapes.
 * Exact byte parity with the pinned Torch generator must be verified separately.
 */
RECONSTRUCTION_EXPORT int reconstruction_official_noise_generate_frame_v1(uint64_t frame_index,
    uint32_t width, float *output, size_t output_floats);
/* Appends frame_count new frames starting at start_frame. The regular file must
 * contain exactly start_frame complete frames, or be new when start_frame=0.
 * Callers must authenticate any existing prefix. Exclusive nonblocking file lock.
 * A failed partial frame is truncated, leaving prior complete frames available.
 * Process termination may leave a partial frame requiring explicit recovery.
 */
RECONSTRUCTION_EXPORT int reconstruction_official_noise_append_v1(const char *absolute_path,
    uint32_t width, uint64_t start_frame, uint64_t frame_count);
RECONSTRUCTION_EXPORT const char *reconstruction_official_noise_last_error_v1(void);
#ifdef __cplusplus
}
#endif
