#pragma once
#include "reconstruction_original_dense_raw.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additive path-only transport; dense options/header ABI2 is unchanged.
 * Caller owns five NUL-terminated UTF-8 strings until synchronous return.
 * capture/output/manifest/receipt must be absolute paths; manifest SHA is
 * exactly64 lowercase ASCII hex bytes. Runtime is the already-initialized sole
 * process-owned CPython3.10.21. This function never initializes/finalizes it.
 * Caller must keep one Python job active across preparation and reconstruction;
 * the Python wrapper's RLock serializes this producer's argv/environment changes.
 * GIL alone does not protect process globals while NumPy releases it.
 * Return0 only after exact Python int97;22=invalid C arguments;1=runtime/Python
 * failure. Read existing reconstruction_original_dense_raw_last_error on the SAME native
 * thread immediately after failure; pointer ownership/lifetime is unchanged.
 */
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fixture_prepare(
    const char *capture_root_utf8,
    const char *output_root_utf8,
    const char *manifest_path_utf8,
    const char *manifest_sha256_ascii,
    const char *receipt_path_utf8);
#ifdef __cplusplus
}
#endif
