#pragma once
#include "reconstruction_original_dense_raw.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additive selected product transport. The old pack and producer stay intact.
 * All buffers remain caller-owned and immutable through this blocking call.
 * box15: centre3, full side lengths3, row-major local-to-world rotation9.
 * indices: original ascending frame indices, never freshly numbered noise rows.
 * Both directories must be distinct, absolute and fresh. Original numerical
 * outputs are retained separately from the selected pixel-mask cache. */
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fuse_pack_selected_v1(
    const RECONSTRUCTIONOriginalFusionPack *pack, const char *original_directory,
    const char *selected_directory, const double *box15,
    const uint32_t *original_indices, const char *selection_plan_sha256,
    uint64_t *original_kept_total, uint64_t *selected_kept_total);
/* Borrowed selected prequantization f4/RGB rows, synchronous on the calling
 * thread under the existing runtime/GIL owner. Copy before returning. A zero
 * count is valid; nonzero return cancels/fails the enclosing call. */
typedef int32_t (*reconstruction_selected_chunk_fn)(int32_t index, const float *xyz,
    const uint8_t *rgb, int32_t count, void *user);
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fuse_pack_selected_v2(
    const RECONSTRUCTIONOriginalFusionPack *pack, const char *original_directory,
    const char *selected_directory, const double *box15,
    const uint32_t *original_indices, const char *selection_plan_sha256,
    uint64_t *original_kept_total, uint64_t *selected_kept_total,
    reconstruction_selected_chunk_fn callback, void *user);
RECONSTRUCTION_RAW_EXPORT const char *reconstruction_selected_fusion_last_error(void);
#ifdef __cplusplus
}
#endif
