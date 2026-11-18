#pragma once
#include "fusion_post_owner.h"
#include "reconstruction_continuous_mvs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PwWholeDawnOwnerV19 PwWholeDawnOwnerV19;
typedef int(*PwWholeOriginalMvsFnV19)(const reconstruction_continuous_mvs_request*,char*,size_t);
// Same accepted Config callbacks, now explicitly exclusive for the whole MVS
// and Fusion phases. Must be held by the one process/App scheduler.
PW_OWNER_API int reconstruction_whole_dawn_create_v19(const PwFusionPostCreateV1*,PwWholeDawnOwnerV19**);
// Invokes the pinned original synchronous MVS ABI1 itself. No caller-completed
// Boolean is accepted. Input pointers/config/module refs remain caller-owned.
PW_OWNER_API int reconstruction_whole_dawn_mvs_v19(PwWholeDawnOwnerV19*,PwWholeOriginalMvsFnV19,const reconstruction_continuous_mvs_request*,char*,size_t);
PW_OWNER_API int reconstruction_whole_dawn_fusion_v19(PwWholeDawnOwnerV19*,const ReconstructionFusionInputV1*);
PW_OWNER_API int reconstruction_whole_dawn_destroy_v19(PwWholeDawnOwnerV19*);
#ifdef __cplusplus
}
#endif
