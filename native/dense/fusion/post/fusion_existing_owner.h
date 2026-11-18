#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#if defined(__GNUC__)
#define PW_OWNER_API __attribute__((visibility("default")))
#else
#define PW_OWNER_API
#endif
typedef void (*PwOwnerRef)(void*);
typedef int (*PwOwnerEnter)(void*);
typedef struct {
 uint32_t struct_bytes,abi_version;
 int32_t context_id;
 uint32_t all_prior_queue_work_completed;
 void* app_owner;
 PwOwnerRef retain_app_owner,release_app_owner;
 PwOwnerEnter enter_exclusive_quiescent_phase;
 PwOwnerRef leave_exclusive_quiescent_phase;
 const char* dawn_header_identity;
 const char* runtime_owner_identity;
} PwExistingOwnerRegisterV1;
typedef struct {
 uint32_t struct_bytes,abi_version;
 void *owner,*instance,*device;
 const void* dawn_procs;
 size_t dawn_procs_bytes;
 const char* dawn_header_identity;
 PwOwnerRef retain_owner,release_owner;
} PwExistingOwnerBorrowV1;
PW_OWNER_API int reconstruction_fusion_existing_owner_register_v1(const PwExistingOwnerRegisterV1*);
PW_OWNER_API int reconstruction_fusion_existing_owner_borrow_v1(PwExistingOwnerBorrowV1*);
PW_OWNER_API int reconstruction_fusion_existing_owner_unregister_v1(void);
PW_OWNER_API uint64_t reconstruction_fusion_existing_owner_active_borrows_v1(void);
PW_OWNER_API const char* reconstruction_fusion_existing_owner_runtime_identity_v1(void);
PW_OWNER_API const char* reconstruction_fusion_existing_owner_last_error_v1(void);
#ifdef __cplusplus
}
                                                                              
struct PwOwnerSnapshot {void *instance,*device;const void*procs;size_t procs_bytes;};
int pw_owner_backend_open_existing(int32_t,PwOwnerSnapshot*) noexcept;
void pw_owner_backend_close_existing(int32_t,PwOwnerSnapshot*) noexcept;
#endif
