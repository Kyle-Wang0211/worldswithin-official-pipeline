#include "fusion_existing_owner.h"
#include "owner_pins.h"
#include <mutex>
#include <atomic>
#include <cstring>
#include <string>
#include <new>
namespace {
std::mutex mu;bool registered=false;PwExistingOwnerRegisterV1 config{};PwOwnerSnapshot snapshot{};
uint64_t borrows=0;thread_local std::string error;
struct Borrow {std::atomic<uint32_t>refs{1};};
void retain(void*p){static_cast<Borrow*>(p)->refs.fetch_add(1);}
void release(void*p){if(!p)return;auto*b=static_cast<Borrow*>(p);if(b->refs.fetch_sub(1)!=1)return;std::lock_guard<std::mutex>lock(mu);--borrows;delete b;}
bool eq(const char*a,const char*b){return a&&std::strcmp(a,b)==0;}
}
extern "C" const char*reconstruction_fusion_existing_owner_runtime_identity_v1(){return kRuntimeOwnerIdentity;}
extern "C" const char*reconstruction_fusion_existing_owner_last_error_v1(){return error.c_str();}
extern "C" uint64_t reconstruction_fusion_existing_owner_active_borrows_v1(){std::lock_guard<std::mutex>lock(mu);return borrows;}
extern "C" int reconstruction_fusion_existing_owner_register_v1(const PwExistingOwnerRegisterV1*c){
 if(!c||c->struct_bytes!=sizeof(*c)||c->abi_version!=1||c->context_id!=0||c->all_prior_queue_work_completed!=1||!c->app_owner||!c->retain_app_owner||!c->release_app_owner||!c->enter_exclusive_quiescent_phase||!c->leave_exclusive_quiescent_phase||!eq(c->dawn_header_identity,kDawnHeaderIdentity)||!eq(c->runtime_owner_identity,kRuntimeOwnerIdentity)){error="invalid attestation; no implicit initialization/fallback";return 1;}
 std::lock_guard<std::mutex>lock(mu);if(registered){error="owner already registered";return 7;}
                                                                            
                                                                            
 c->retain_app_owner(c->app_owner);
 if(c->enter_exclusive_quiescent_phase(c->app_owner)!=0){c->release_app_owner(c->app_owner);error="App owner refused exclusive quiescent phase";return 7;}
 PwOwnerSnapshot s{};int rc=pw_owner_backend_open_existing(c->context_id,&s);
 if(rc||!s.instance||!s.device||!s.procs||!s.procs_bytes){if(!rc)pw_owner_backend_close_existing(c->context_id,&s);c->leave_exclusive_quiescent_phase(c->app_owner);c->release_app_owner(c->app_owner);error="existing native ORT context missing or incompatible";return 2;}
 config=*c;snapshot=s;registered=true;return 0;
}
extern "C" int reconstruction_fusion_existing_owner_borrow_v1(PwExistingOwnerBorrowV1*out){
 if(!out||out->struct_bytes!=sizeof(*out)||out->abi_version!=1)return 1;
 out->owner=out->instance=out->device=nullptr;out->dawn_procs=nullptr;out->dawn_procs_bytes=0;out->dawn_header_identity=nullptr;out->retain_owner=out->release_owner=nullptr;
 std::lock_guard<std::mutex>lock(mu);if(!registered){error="existing owner not registered";return 2;}
 auto*b=new(std::nothrow)Borrow;if(!b){error="lease allocation failed";return 3;}++borrows;
 out->owner=b;out->instance=snapshot.instance;out->device=snapshot.device;out->dawn_procs=snapshot.procs;out->dawn_procs_bytes=snapshot.procs_bytes;out->dawn_header_identity=kDawnHeaderIdentity;out->retain_owner=retain;out->release_owner=release;return 0;
}
extern "C" int reconstruction_fusion_existing_owner_unregister_v1(){std::lock_guard<std::mutex>lock(mu);if(!registered)return 0;if(borrows){error="outstanding Fusion/CABI owner leases; drain and Destroy0 first";return 7;}
 pw_owner_backend_close_existing(config.context_id,&snapshot);config.leave_exclusive_quiescent_phase(config.app_owner);config.release_app_owner(config.app_owner);config={};snapshot={};registered=false;return 0;}
