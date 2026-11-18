#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Synchronous MVS-only ABI1. All input pointers remain owned/alive by caller.
   phases: 1=before reference, 2=reference files committed, 3=pass destroyed,
   4=paused poll, 5=pass initialized. pass: 0=LOW768,1=HIGH1536.
   callback result: 0=continue,1=stop (rc=2),2=pause (100ms polling).
   A thermal stop is checked between refs, never claimed as a completed run. */
typedef int (*reconstruction_continuous_progress)(void* user,int phase,int pass,int reference,
                                      int count,double elapsed_ms);
typedef struct reconstruction_continuous_mvs_request {
 uint32_t abi_version; uint32_t struct_bytes; uint32_t count; uint32_t reserved;
 const char* feature_low; const char* rest_low;
 const char* feature_high; const char* rest_high;
 const float* cameras_low_f32; /* count*36; original row order */
 const int32_t* neighbors_i32; /* count*9, ref excluded, unique */
 const char* noise_low_f32; const char* noise_high_f32; /* exact original SEEDED banks */
 const char* images_low_f16; const char* images_high_f16; /* both supplied or both null */
 const char* const* original_jpeg_paths; /* count paths; used only without banks */
 const char* output_root; /* fresh absolute directory; never overwrite */
 reconstruction_continuous_progress progress; void* user;
} reconstruction_continuous_mvs_request;
/* rc 0=all LOW+HIGH refs committed MVS-only,2=callback stopped,1=error.
   Output: manifest.json + refs.jsonl + low/high/{depth,conf0,conf1,conf2}.f32.
   Models hashed before execution; per-ref four plane SHA256/finite/timing receipts.
   2e9 allocator settings + 100ms sampler are observations/guards, not OS cap proof. */
int reconstruction_continuous_mvs_run(const reconstruction_continuous_mvs_request*,char* error,size_t error_bytes);
uint32_t reconstruction_continuous_mvs_abi(void);
#ifdef __cplusplus
}
#endif
