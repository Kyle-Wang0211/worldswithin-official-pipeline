#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int pw_s1_memory_start(void);
void pw_s1_memory_stop(void);
int pw_s1_memory_preflight(const char* phase, size_t additional_host_bytes, char* error, int error_size);
void pw_s1_memory_active_run(void* options, int (*cancel)(void*));
void pw_s1_memory_clear_run(void);
int pw_s1_memory_guard_tripped(void);
const char* pw_s1_memory_json(void);
const char* pw_s1_runtime_snapshot_json(void);
#ifdef __cplusplus
}
#endif
