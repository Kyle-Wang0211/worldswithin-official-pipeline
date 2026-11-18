#pragma once
#if defined(__GNUC__) || defined(__clang__)
#define RECONSTRUCTION_WHOLE_EXPORT __attribute__((visibility("default")))
#else
#define RECONSTRUCTION_WHOLE_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif
// Existing CPython must already be initialized. Absolute UTF-8 filesystem
// paths, not inline JSON. Calls only production_whole_driver.run_json(req,res).
// 0 success; 22 invalid path; 1 runtime/Python/driver failure; 16 concurrent run.
RECONSTRUCTION_WHOLE_EXPORT int reconstruction_production_whole_run_v1(const char* request_json,
                                                 const char* result_json);
RECONSTRUCTION_WHOLE_EXPORT const char* reconstruction_production_whole_last_error_v1(void);
#ifdef __cplusplus
}
#endif
