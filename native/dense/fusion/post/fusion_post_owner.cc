#include "fusion_post_owner.h"
#include "owner_pins.h"
#include <mutex>
#include <atomic>
#include <new>
#include <cstring>
#include <chrono>
#include <algorithm>
namespace{std::mutex phase_mu;uint64_t sequence=0,mvs_ticket=0;bool post_live=false;
uint64_t now_ns(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
constexpr uint64_t hardcap=2000000000ULL;
}
struct PwFusionPostOwner{
 PwFusionPostCreateV1 c{};std::atomic<uint32_t>refs{1};std::mutex api_mu;
 void*native=nullptr;bool phase=false,registered=false;PwExistingOwnerBorrowV1 borrow{};ReconstructionFusionContext*fusion=nullptr;PwPostBudgetSnapshotV1 latest{};
};
namespace{
void hold(void*p){static_cast<PwFusionPostOwner*>(p)->refs.fetch_add(1);}
void drop(void*p){static_cast<PwFusionPostOwner*>(p)->refs.fetch_sub(1);}
int already_exclusive(void*p){return static_cast<PwFusionPostOwner*>(p)->phase?0:7;}
void no_phase_release(void*){}
int sample(PwFusionPostOwner*p){PwPostBudgetSnapshotV1 s{};s.struct_bytes=sizeof(s);s.abi_version=1;if(p->c.fresh_budget_sample(p->c.app_owner,&s))return PW_FUSION_BUDGET;
 auto n=now_ns();if(s.struct_bytes!=sizeof(s)||s.abi_version!=1||(s.valid_flags&7)!=7||s.reserved||s.monotonic_ns>n||n-s.monotonic_ns>1000000000ULL||s.phys_footprint_B>=hardcap||!s.os_available_B)return PW_FUSION_BUDGET;p->latest=s;return 0;}
int budget(void*opaque,uint64_t objects,uint64_t payload){auto*p=static_cast<PwFusionPostOwner*>(opaque);if(sample(p))return 1;
                                                                             
                                                                            
 const uint64_t cpu=uint64_t(p->c.width)*p->c.height*(p->c.audit_perpixel_counts?14:10);
 uint64_t extra=objects;if(extra>hardcap||payload>hardcap-extra)return 1;extra+=payload;if(cpu>hardcap-extra)return 1;extra+=cpu;
 return extra>p->latest.os_available_B||extra>hardcap-p->latest.phys_footprint_B;
}
int unregister_if_possible(PwFusionPostOwner*p){
 if(p->borrow.owner){p->borrow.release_owner(p->borrow.owner);p->borrow={};}
 if(p->registered){int rc=reconstruction_fusion_existing_owner_unregister_v1();if(rc)return rc;p->registered=false;}
 if(p->native){int rc=pw_post_backend_release_official(p->native);if(rc)return rc;p->native=nullptr;}
 return 0;
}
void release_phase(PwFusionPostOwner*p){if(p->phase){p->c.leave_post_exclusive(p->c.app_owner);p->phase=false;}p->c.release_app_owner(p->c.app_owner);{std::lock_guard<std::mutex>lock(phase_mu);post_live=false;}delete p;}
}
extern "C" int reconstruction_fusion_post_mvs_enter_v1(uint64_t*out){if(!out)return 1;*out=0;std::lock_guard<std::mutex>lock(phase_mu);if(post_live||mvs_ticket)return 7;mvs_ticket=++sequence;*out=mvs_ticket;return 0;}
extern "C" int reconstruction_fusion_post_mvs_leave_v1(uint64_t ticket,uint32_t completed){std::lock_guard<std::mutex>lock(phase_mu);if(!ticket||ticket!=mvs_ticket||completed!=1)return 7;mvs_ticket=0;return 0;}
extern "C" int reconstruction_fusion_post_create_v1(const PwFusionPostCreateV1*c,PwFusionPostOwner**out){
 if(!out)return 1;
 *out=nullptr;
 if(!c||c->struct_bytes!=sizeof(*c)||c->abi_version!=1||c->width!=1536||c->height!=1152||c->audit_perpixel_counts>1||!c->app_owner||!c->retain_app_owner||!c->release_app_owner||!c->enter_post_exclusive||!c->leave_post_exclusive||!c->fresh_budget_sample||!c->dawn_header_identity||std::strcmp(c->dawn_header_identity,kDawnHeaderIdentity)||!c->runtime_owner_identity||std::strcmp(c->runtime_owner_identity,kRuntimeOwnerIdentity))return 1;
 {std::lock_guard<std::mutex>lock(phase_mu);if(post_live||mvs_ticket)return 7;post_live=true;}
 auto*p=new(std::nothrow)PwFusionPostOwner;if(!p){std::lock_guard<std::mutex>lock(phase_mu);post_live=false;return 3;}p->c=*c;c->retain_app_owner(c->app_owner);
 if(c->enter_post_exclusive(c->app_owner)){release_phase(p);return 7;}p->phase=true;
 uint64_t plane=uint64_t(c->width)*c->height*4;int rc=budget(p,(c->audit_perpixel_counts?44:42)*plane+64*768+104,24*plane+64*768)?6:0;
 if(!rc)rc=pw_post_backend_create_official(&p->native);
 if(!rc){PwExistingOwnerRegisterV1 r{sizeof(r),1,0,1,p,hold,drop,already_exclusive,no_phase_release,kDawnHeaderIdentity,kRuntimeOwnerIdentity};rc=reconstruction_fusion_existing_owner_register_v1(&r);if(!rc)p->registered=true;}
 if(!rc){p->borrow.struct_bytes=sizeof(p->borrow);p->borrow.abi_version=1;rc=reconstruction_fusion_existing_owner_borrow_v1(&p->borrow);}
 if(!rc){auto&b=p->borrow;ReconstructionFusionCreateV1 h{sizeof(h),1,b.dawn_procs,b.dawn_procs_bytes,b.dawn_header_identity,b.instance,b.device,p,hold,drop,budget,c->width,c->height,c->audit_perpixel_counts,c->wait_timeout_ms};rc=reconstruction_fusion_create_v1(&h,&p->fusion);}
 if(rc&&!p->fusion){int close=unregister_if_possible(p);if(close){*out=p;return close;}if(p->refs.load()!=1){*out=p;return 7;}release_phase(p);return rc;}
 *out=p;return rc;
}
extern "C" int reconstruction_fusion_post_run_v1(PwFusionPostOwner*p,const ReconstructionFusionInputV1*i){if(!p)return 1;std::lock_guard<std::mutex>lock(p->api_mu);return p->fusion?reconstruction_fusion_run_v1(p->fusion,i):1;}
extern "C" int reconstruction_fusion_post_cancel_v1(PwFusionPostOwner*p){if(!p)return 1;return p->fusion?reconstruction_fusion_cancel_v1(p->fusion):0;}
extern "C" int reconstruction_fusion_post_drain_v1(PwFusionPostOwner*p){if(!p)return 1;std::lock_guard<std::mutex>lock(p->api_mu);return p->fusion?reconstruction_fusion_drain_v1(p->fusion):0;}
extern "C" int reconstruction_fusion_post_budget_snapshot_v1(PwFusionPostOwner*p,PwPostBudgetSnapshotV1*out){if(!p||!out||out->struct_bytes!=sizeof(*out)||out->abi_version!=1)return 1;std::lock_guard<std::mutex>lock(p->api_mu);int rc=sample(p);if(!rc)*out=p->latest;return rc;}
extern "C" int reconstruction_fusion_post_destroy_v1(PwFusionPostOwner*p){if(!p)return 1;std::unique_lock<std::mutex>lock(p->api_mu);if(p->fusion){int rc=reconstruction_fusion_destroy_v1(p->fusion);if(rc)return rc;p->fusion=nullptr;}int rc=unregister_if_possible(p);if(rc)return rc;if(p->refs.load()!=1)return 7;lock.unlock();release_phase(p);return 0;}
