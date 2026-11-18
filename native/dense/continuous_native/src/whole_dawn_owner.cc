#include "actual_frame_count.h"
#include "../../memory_policy.h"
// Execution/ownership only. All MVS/Fusion numeric work calls unchanged ABIs.
#include "whole_dawn_owner.h"
#include "owner_pins.h"
#include "onnxruntime_c_api.h"
#include "core/providers/webgpu/webgpu_context.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <new>
namespace {
enum Phase{Created=0,MvsLive=1,MvsReturned=2,FusionLive=3,FusionReturned=4,Failed=5};
uint64_t memory_cap(){return reconstruction_memory_policy::limit();}
uint64_t now(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
struct PwWholeDawnOwnerV19{
 PwFusionPostCreateV1 c{};std::mutex mu;std::atomic<uint32_t>refs{1};Phase phase=Created;
 bool app_phase=false,registered=false;OrtEnv*env=nullptr;uint64_t ticket=0;PwOwnerSnapshot held_context{};
 void*device=nullptr,*instance=nullptr;PwExistingOwnerBorrowV1 borrow{};ReconstructionFusionContext*fusion=nullptr;
 uint32_t count=0,commits[2]{},destroyed=0;bool callback_bad=false;PwPostBudgetSnapshotV1 latest{};
 reconstruction_continuous_progress progress=nullptr;void*user=nullptr;
};
namespace{
void hold(void*p){static_cast<PwWholeDawnOwnerV19*>(p)->refs.fetch_add(1);}
void drop(void*p){static_cast<PwWholeDawnOwnerV19*>(p)->refs.fetch_sub(1);}
int exclusive(void*p){auto*o=static_cast<PwWholeDawnOwnerV19*>(p);return o->app_phase&&o->phase==MvsReturned?0:7;}
void noleave(void*){}
int sample(PwWholeDawnOwnerV19*o){
 if(!reconstruction_memory_policy::valid())return 6;const uint64_t cap=memory_cap();
 PwPostBudgetSnapshotV1 s{};s.struct_bytes=sizeof(s);s.abi_version=1;
 if(o->c.fresh_budget_sample(o->c.app_owner,&s))return 6;
 auto n=now();if(s.struct_bytes!=sizeof(s)||s.abi_version!=1||(s.valid_flags&7)!=7||s.reserved||s.monotonic_ns>n||n-s.monotonic_ns>1000000000ULL||s.phys_footprint_B>=cap||!s.os_available_B)return 6;
 o->latest=s;return 0;
}
int budget(void*p,uint64_t objects,uint64_t payload){
 auto*o=static_cast<PwWholeDawnOwnerV19*>(p);if(sample(o))return 1;const uint64_t cap=memory_cap();
 const uint64_t cpu=uint64_t(o->c.width)*o->c.height*(o->c.audit_perpixel_counts?14:10);
 uint64_t extra=objects;if(extra>cap||payload>cap-extra)return 1;extra+=payload;if(cpu>cap-extra)return 1;extra+=cpu;
 return extra>o->latest.os_available_B||extra>cap-o->latest.phys_footprint_B;
}
int close_native(PwWholeDawnOwnerV19*o){
 if(o->borrow.owner){o->borrow.release_owner(o->borrow.owner);o->borrow={};}
 if(o->registered){int r=reconstruction_fusion_existing_owner_unregister_v1();if(r)return r;o->registered=false;}
 if(o->held_context.device)pw_owner_backend_close_existing(0,&o->held_context);
 if(o->env){OrtGetApiBase()->GetApi(ORT_API_VERSION)->ReleaseEnv(o->env);o->env=nullptr;}
 return 0;
}
void free_owner(PwWholeDawnOwnerV19*o){if(o->app_phase)o->c.leave_post_exclusive(o->c.app_owner);o->c.release_app_owner(o->c.app_owner);delete o;}
int progress(void*p,int phase,int pass,int ref,int count,double ms){
 auto*o=static_cast<PwWholeDawnOwnerV19*>(p);
 if(count!=int(o->count)||pass<0||pass>1){o->callback_bad=true;return 1;}
 if(phase==5){
  // Original MVS initialized its own context first, including cache/queue
  // configuration. Acquire an extra reference now: never DefaultContext first.
  PwOwnerSnapshot snapshot{};int rc=pw_owner_backend_open_existing(0,&snapshot);
  if(rc){o->callback_bad=true;return 1;}
  if(!o->held_context.device){o->held_context=snapshot;o->device=snapshot.device;o->instance=snapshot.instance;}
  else{bool same=snapshot.device==o->device&&snapshot.instance==o->instance;pw_owner_backend_close_existing(0,&snapshot);if(!same){o->callback_bad=true;return 1;}}
 }
 if(phase==2){if(ref!=int(o->commits[pass])||ref>=count){o->callback_bad=true;return 1;}++o->commits[pass];}
 if(phase==3){if(pass!=int(o->destroyed)||o->commits[pass]!=o->count){o->callback_bad=true;return 1;}++o->destroyed;}
 if(sample(o)){o->callback_bad=true;return 1;}
 return o->progress?o->progress(o->user,phase,pass,ref,count,ms):0;
}
}
extern "C" int reconstruction_whole_dawn_create_v19(const PwFusionPostCreateV1*c,PwWholeDawnOwnerV19**out){
 if(!out)return 1;
 *out=nullptr;
 if(!c||c->struct_bytes!=sizeof(*c)||c->abi_version!=1||c->width!=1536||c->height!=1152||c->audit_perpixel_counts>1||!c->app_owner||!c->retain_app_owner||!c->release_app_owner||!c->enter_post_exclusive||!c->leave_post_exclusive||!c->fresh_budget_sample||!c->dawn_header_identity||std::strcmp(c->dawn_header_identity,kDawnHeaderIdentity)||!c->runtime_owner_identity||std::strcmp(c->runtime_owner_identity,kRuntimeOwnerIdentity))return 1;
 auto*o=new(std::nothrow)PwWholeDawnOwnerV19;if(!o)return 3;o->c=*c;c->retain_app_owner(c->app_owner);
 if(c->enter_post_exclusive(c->app_owner)){free_owner(o);return 7;}o->app_phase=true;
 int r=sample(o);if(!r&&onnxruntime::webgpu::WebGpuContextFactory::HasAnyContextForPostOwner())r=7;
 if(!r)r=reconstruction_fusion_post_mvs_enter_v1(&o->ticket);
 // Retain only the official Env here. Device/BufferManager creation must remain
 // in original MVS Initialize after configure_once and its exact cache options.
 if(!r){const auto*a=OrtGetApiBase()->GetApi(ORT_API_VERSION);auto*s=a->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"reconstruction-whole-owner-env-only",&o->env);if(s){a->ReleaseStatus(s);r=3;}}
 if(r){if(o->env){int z=close_native(o);if(z){o->phase=Failed;*out=o;return z;}}if(o->ticket){int z=reconstruction_fusion_post_mvs_leave_v1(o->ticket,1);if(z){o->phase=Failed;*out=o;return z;}o->ticket=0;}free_owner(o);return r;}
 *out=o;return 0;
}
extern "C" int reconstruction_whole_dawn_mvs_v19(PwWholeDawnOwnerV19*o,PwWholeOriginalMvsFnV19 fn,const reconstruction_continuous_mvs_request*r,char*error,size_t n){
 if(!o||!fn||!r||r->abi_version!=1||r->struct_bytes!=sizeof(*r)||!reconstruction::actual_frame_count_supported(r->count))return 1;
 std::lock_guard<std::mutex>lock(o->mu);if(o->phase!=Created||!o->ticket)return 7;
 if(sample(o))return 6;
 o->count=r->count;o->progress=r->progress;o->user=r->user;o->phase=MvsLive;
 auto request=*r;request.progress=progress;request.user=o;
 int rc=fn(&request,error,n);
 if(rc||o->callback_bad||!o->held_context.device||o->commits[0]!=o->count||o->commits[1]!=o->count||o->destroyed!=2){o->phase=Failed;return rc?rc:7;}
 // The original ABI emits both phase3 only after RAII session/lease teardown.
 // Whole-owner's official context ref remains, so MVS and Fusion reuse the
 // identical actual Dawn device. No Factory clear or Context pointer cast.
 int z=reconstruction_fusion_post_mvs_leave_v1(o->ticket,1);if(z){o->phase=Failed;return z;}o->ticket=0;o->phase=MvsReturned;return 0;
}
extern "C" int reconstruction_whole_dawn_fusion_v19(PwWholeDawnOwnerV19*o,const ReconstructionFusionInputV1*i){
 if(!o||!i)return 1;
 std::lock_guard<std::mutex>lock(o->mu);if(o->phase!=MvsReturned||o->ticket)return 7;
 if(i->frame_count!=o->count||i->reference_count!=o->count||i->CER_all_frames_complete!=1)return 1;
 if(sample(o))return 6;
 PwExistingOwnerRegisterV1 reg{sizeof(reg),1,0,1,o,hold,drop,exclusive,noleave,kDawnHeaderIdentity,kRuntimeOwnerIdentity};
 int rc=reconstruction_fusion_existing_owner_register_v1(&reg);if(rc)return rc;o->registered=true;
 o->borrow.struct_bytes=sizeof(o->borrow);o->borrow.abi_version=1;rc=reconstruction_fusion_existing_owner_borrow_v1(&o->borrow);
 if(!rc&&(o->borrow.device!=o->device||o->borrow.instance!=o->instance))rc=7;
 if(!rc){auto&b=o->borrow;ReconstructionFusionCreateV1 c{sizeof(c),1,b.dawn_procs,b.dawn_procs_bytes,b.dawn_header_identity,b.instance,b.device,o,hold,drop,budget,o->c.width,o->c.height,o->c.audit_perpixel_counts,o->c.wait_timeout_ms};rc=reconstruction_fusion_create_v1(&c,&o->fusion);}
 if(rc){o->phase=Failed;return rc;}o->phase=FusionLive;
 rc=reconstruction_fusion_run_v1(o->fusion,i);if(rc){reconstruction_fusion_cancel_v1(o->fusion);reconstruction_fusion_drain_v1(o->fusion);o->phase=Failed;return rc;}
 int z=reconstruction_fusion_destroy_v1(o->fusion);if(z){o->phase=Failed;return z;}o->fusion=nullptr;o->phase=FusionReturned;return 0;
}
extern "C" int reconstruction_whole_dawn_destroy_v19(PwWholeDawnOwnerV19*o){
 if(!o)return 1;
 std::unique_lock<std::mutex>lock(o->mu);
 if(o->phase==Created&&o->ticket){int rc=reconstruction_fusion_post_mvs_leave_v1(o->ticket,1);if(rc)return rc;o->ticket=0;}
 if(o->phase==MvsLive||o->ticket)return 7;
 if(o->fusion){int rc=reconstruction_fusion_destroy_v1(o->fusion);if(rc)return rc;o->fusion=nullptr;}
 int r=close_native(o);if(r)return r;if(o->refs.load()!=1)return 7;
 lock.unlock();free_owner(o);return 0;
}

extern "C" int reconstruction_whole_dawn_frame_count_supported_v1(uint32_t count){return reconstruction::actual_frame_count_supported(count)?1:0;}
