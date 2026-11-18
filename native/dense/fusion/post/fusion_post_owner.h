#pragma once
#include "fusion_existing_owner.h"
#include "reconstruction_fusion_batch8.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PwFusionPostOwner PwFusionPostOwner;
enum{PW_POST_SAMPLE_PHYS=1,PW_POST_SAMPLE_OS_AVAILABLE=2,PW_POST_SAMPLE_OBSERVER_ACTIVE=4};
typedef struct{
 uint32_t struct_bytes,abi_version,valid_flags,reserved;
 uint64_t phys_footprint_B,os_available_B,metal_B,dawn_backing_B,monotonic_ns;
}PwPostBudgetSnapshotV1;
typedef struct{
 uint32_t struct_bytes,abi_version,width,height,audit_perpixel_counts,wait_timeout_ms;
 void*app_owner;PwOwnerRef retain_app_owner,release_app_owner;
                                                                                
                                                                                  
 PwOwnerEnter enter_post_exclusive;PwOwnerRef leave_post_exclusive;
 int(*fresh_budget_sample)(void*,PwPostBudgetSnapshotV1*);
 const char*dawn_header_identity;const char*runtime_owner_identity;
}PwFusionPostCreateV1;
                                                                               
                                                                           
PW_OWNER_API int reconstruction_fusion_post_sample_os_v1(PwPostBudgetSnapshotV1*);
                                                                             
                                                                                 
PW_OWNER_API int reconstruction_fusion_post_mvs_enter_v1(uint64_t*ticket);
PW_OWNER_API int reconstruction_fusion_post_mvs_leave_v1(uint64_t ticket,uint32_t all_queue_work_completed);
PW_OWNER_API int reconstruction_fusion_post_create_v1(const PwFusionPostCreateV1*,PwFusionPostOwner**);
PW_OWNER_API int reconstruction_fusion_post_run_v1(PwFusionPostOwner*,const ReconstructionFusionInputV1*);
PW_OWNER_API int reconstruction_fusion_post_cancel_v1(PwFusionPostOwner*);
PW_OWNER_API int reconstruction_fusion_post_drain_v1(PwFusionPostOwner*);
PW_OWNER_API int reconstruction_fusion_post_destroy_v1(PwFusionPostOwner*);
PW_OWNER_API int reconstruction_fusion_post_budget_snapshot_v1(PwFusionPostOwner*,PwPostBudgetSnapshotV1*);
#ifdef __cplusplus
}
                                                                            
int pw_post_backend_create_official(void**owner)noexcept;
int pw_post_backend_release_official(void*owner)noexcept;
#endif
