#ifndef RECONSTRUCTION_FUSION_BATCH8_H
#define RECONSTRUCTION_FUSION_BATCH8_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#if defined(__GNUC__)
#define PW_FUSION_API __attribute__((visibility("default")))
#else
#define PW_FUSION_API
#endif
typedef struct ReconstructionFusionContext ReconstructionFusionContext;
enum { PW_FUSION_OK=0, PW_FUSION_ARGUMENT=1, PW_FUSION_ABI=2, PW_FUSION_GPU=3, PW_FUSION_PENDING=4, PW_FUSION_IO=5, PW_FUSION_BUDGET=6, PW_FUSION_BUSY=7, PW_FUSION_CANCELLED=8 };
typedef struct {
 uint32_t struct_bytes, abi_version;
 const void* dawn_procs; size_t dawn_procs_bytes; const char* dawn_header_identity;
 void* instance; void* device; void* owner;
 void (*retain_owner)(void*); void (*release_owner)(void*);
                                                                                      
                                                                                      
 int (*budget_check)(void*,uint64_t requested_objects_B,uint64_t outstanding_payload_B);
 uint32_t width,height; uint32_t audit_perpixel_counts; uint32_t wait_timeout_ms;
} ReconstructionFusionCreateV1;
typedef struct {
 uint32_t ref; float dmax,dmin;
 uint32_t source_count; const uint32_t* ascending_qualified_sources;
} ReconstructionFusionReferenceV1;
typedef struct {
 uint32_t struct_bytes,abi_version,frame_count,reference_count;
 const ReconstructionFusionReferenceV1* original_order_refs;
 void* data_owner; void (*retain_data)(void*); void (*release_data)(void*);
 /* Each callback fills caller-owned bounded storage synchronously; source bank is
    immutable and all CER frames completed. Exact SHA/order/camera prep is upstream. */
 int (*read_depth)(void*,uint32_t source,float* dst,size_t elements);
 int (*read_photo)(void*,uint32_t ref,uint32_t* dst,size_t elements);
 int (*read_pair91)(void*,uint32_t ref,uint32_t source,float* dst91);
                                                                                 
                                                                              
 int (*emit)(void*,uint32_t ref,const uint16_t* d16,const uint32_t* counts_or_null,size_t elements,uint32_t kept,const uint32_t histogram11[11]);
 uint32_t CER_all_frames_complete; const char* immutable_input_manifest_sha256;
} ReconstructionFusionInputV1;
typedef struct {
 uint32_t struct_bytes,abi_version;uint64_t requested_objects_B,object_peak_B,upload_bytes,dispatches,window_fences,credit_waits,map_waits;uint32_t completed_refs,pending_windows,data_lease_held;
} ReconstructionFusionStatsV1;
PW_FUSION_API const char* reconstruction_fusion_shader_sha256_v1(void);
PW_FUSION_API const char* reconstruction_fusion_dawn_header_identity_v1(void);
PW_FUSION_API int reconstruction_fusion_create_v1(const ReconstructionFusionCreateV1*,ReconstructionFusionContext** out);
PW_FUSION_API int reconstruction_fusion_run_v1(ReconstructionFusionContext*,const ReconstructionFusionInputV1*);
PW_FUSION_API int reconstruction_fusion_cancel_v1(ReconstructionFusionContext*);
                                                                              
                                                                            
PW_FUSION_API int reconstruction_fusion_drain_v1(ReconstructionFusionContext*);
                                                                          
PW_FUSION_API int reconstruction_fusion_destroy_v1(ReconstructionFusionContext*);
PW_FUSION_API int reconstruction_fusion_stats_v1(ReconstructionFusionContext*,ReconstructionFusionStatsV1*);
#ifdef __cplusplus
}
#endif
#endif
