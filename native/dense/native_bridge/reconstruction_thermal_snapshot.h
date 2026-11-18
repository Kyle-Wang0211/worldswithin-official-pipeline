#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// Pure OS observation, no sampler thread, autorun, policy, App launch or GPU IO.
// state: NSProcessInfo thermalState 0 nominal, 1 fair, 2 serious, 3 critical.
// monotonic_seconds: CLOCK_MONOTONIC reading; epoch_seconds: CLOCK_REALTIME.
// memory_bytes: iOS os_proc_available_memory; macOS host free-page bytes
// (host diagnostic only, not an iOS process-available-memory claim).
// All pointers required. 0=valid OS snapshot; 22=null; other nonzero=OS failure.
__attribute__((visibility("default"))) int reconstruction_thermal_snapshot_v1(
    int* state, double* monotonic_seconds, double* epoch_seconds,
    unsigned long long* memory_bytes);
#ifdef __cplusplus
}
#endif
