#pragma once
#include <stdint.h>

#if defined(_WIN32)
#define RECONSTRUCTION_SELECTION_API __declspec(dllexport)
#else
#define RECONSTRUCTION_SELECTION_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Product selection transport only. Existing f32 world points are promoted to
// double and classified by the unchanged177 BoxFilter predicate. box15 contains
// centre3, FULL side lengths3, row-major local->world rotation9.
//
// Returns 0 on success, nonzero on invalid input. count must be positive and
// mask_count must equal count. Inputs must be finite, full sizes positive, and
// pointer spans valid/non-overlapping for writable output/error versus inputs.
// Any rejected input leaves out_mask and input bytes untouched. No sorting,
// epsilon, clamping, inverse, rotation normalization, or geometry generation.
// error may be null only when error_capacity is zero. Safe diagnostics are
// NUL-terminated when capacity permits; invalid/aliased diagnostic spans are
// never written. Caller owns valid live allocations for all declared spans.
RECONSTRUCTION_SELECTION_API int reconstruction_selection_contains_f32(const float* xyz, uint64_t count,
    const double* box15, uint8_t* out_mask, uint64_t mask_count,
    char* error, uint64_t error_capacity);

#ifdef __cplusplus
}
#endif
