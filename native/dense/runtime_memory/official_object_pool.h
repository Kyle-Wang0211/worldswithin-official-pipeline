#pragma once
                                                                              
                                                                            
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <map>
#include <set>
#include <stdexcept>
#include <limits>
#include "value_allocation_scope.h"
#include "official_object_profile.h"
extern "C" int reconstruction_workspace_phase_active_v2()noexcept;
namespace reconstruction_official_object {
struct Scope {uintptr_t frame=0;size_t value=SIZE_MAX;};
inline thread_local Scope current;
struct Object {uint64_t handle=0;uintptr_t manager=0;uint64_t usage=0;};
struct Frame {size_t profile_id=0;std::set<size_t> values;std::map<size_t,Object> objects;uint64_t creates=0,hits=0,client_addrefs=0,client_releases=0,owner_releases=0;};
using Release=void(*)(uint64_t);
struct Workspace {
 std::mutex mu;uintptr_t device=0;uint64_t loans=0;Release release=nullptr;std::map<size_t,uint64_t> slots;
 static bool enabled(){const char*p=std::getenv("RECONSTRUCTION_CAPTURE_SHARED_WORKSPACE");return p&&p[0]=='1'&&!p[1];}
 static size_t slot(size_t profile,size_t object){if(profile==7){static constexpr size_t map[]={0,1,2,3,4,5,6};if(object>=sizeof(map)/sizeof(map[0]))throw std::runtime_error("newactualphase object mismatch");return map[object];}if(profile==5){static constexpr size_t map[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60};if(object>=sizeof(map)/sizeof(map[0]))throw std::runtime_error("newactualphase object mismatch");return map[object];}if(profile==6){static constexpr size_t map[]={0,1,2,3,4,5,6};if(object>=sizeof(map)/sizeof(map[0]))throw std::runtime_error("newactualphase object mismatch");return map[object];}if(profile==4){static constexpr size_t map[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55};if(object>=sizeof(map)/sizeof(map[0]))throw std::runtime_error("newactualphase object mismatch");return map[object];}throw std::runtime_error("newactualphase profile mismatch");}
 static size_t capacity(size_t i){static constexpr size_t caps[]={63700992,63700992,56623104,28311552,28311552,21233664,21233664,21233664,21233664,21233664,14155776,14155776,7077888,7077888,5308416,5308416,5308416,5308416,3538944,3538944,3538944,2654208,2654208,2211840,1769472,1769472,1769472,1769472,442368,442368,442368,442368,442368,442368,110592,110592,110592,110592,110592,110592,64,64,64,48,48,48,48,48,48,48,48,16,16,16,16,16,16,16,16,16,16};if(i>=sizeof(caps)/sizeof(caps[0]))throw std::runtime_error("newactualphase slot mismatch");return caps[i];}
 template<class Create,class AddRef> uint64_t acquire(size_t profile,size_t object,uintptr_t actual_device,uint64_t usage,Create create,AddRef addref,Release rel){if(!enabled()||(profile!=6&&profile!=7&&profile!=4&&profile!=5))return 0;if(reconstruction_workspace_phase_active_v2()!=1)throw std::runtime_error("workspace requires held complete FNET/REST FIFO scope");const char*submit_drain=std::getenv("RECONSTRUCTION_SUBMIT_DRAIN");if(!submit_drain||submit_drain[0]!='1'||submit_drain[1]||!actual_device||usage!=396)throw std::runtime_error("workspace requires completed same-queue GPU phase");std::lock_guard<std::mutex>l(mu);if(device&&device!=actual_device)throw std::runtime_error("workspace actual Dawn device changed");device=actual_device;const size_t id=slot(profile,object);auto& h=slots[id];if(!h){h=create(capacity(id));if(!h)throw std::runtime_error("workspace physical create failed");release=rel;}addref(h);++loans;return h;}
 void snapshot(uint64_t*n,uint64_t*b,uint64_t*l){std::lock_guard<std::mutex>g(mu);*n=slots.size();*b=0;for(const auto&x:slots)*b+=capacity(x.first);*l=loans;}
 void retire(){std::lock_guard<std::mutex>g(mu);for(const auto&x:slots)release(x.second);slots.clear();device=0;}
};
inline Workspace& workspace(){static auto*p=new Workspace;return*p;}

struct Pool {std::mutex mu;std::map<uintptr_t,Frame> frames;std::map<uint64_t,size_t> handles;Release release=nullptr;
 const ProfileValue* profile(size_t id,size_t profile_id) const {for(const auto&v:GetProfile(profile_id).values)if(v.value==id)return &v;return nullptr;}
 bool known(uintptr_t f){std::lock_guard<std::mutex>l(mu);return frames.count(f);}
 void register_frame(uintptr_t f,const size_t*ids,size_t n,size_t profile_id=0){std::lock_guard<std::mutex>l(mu);auto [it,inserted]=frames.emplace(f,Frame{});if(!inserted)return;it->second.profile_id=profile_id;const auto&profile=GetProfile(profile_id);if(n)it->second.values.insert(ids,ids+n);std::fprintf(stderr,"RECONSTRUCTION_OFFICIAL_POOL_BEGIN frame=%llu values=%zu official_objects=%zu capacity_B=%llu\n",(unsigned long long)f,n,profile.sizes.size(),(unsigned long long)profile.capacity);}
 bool contains(uintptr_t f,size_t v){std::lock_guard<std::mutex>l(mu);auto it=frames.find(f);return it!=frames.end()&&it->second.values.count(v);}
 bool shared(uint64_t h){std::lock_guard<std::mutex>l(mu);return handles.count(h);}
 template<class Create,class AddRef> uint64_t acquire(uintptr_t actual_device,uintptr_t manager,size_t bytes,uint64_t usage,Create create,AddRef addref,Release rel,bool* fresh){
  if(!Enabled()||!current.frame)return 0;
  std::lock_guard<std::mutex>l(mu);auto fi=frames.find(current.frame);if(fi==frames.end()||!fi->second.values.count(current.value))return 0;
  const auto*p=profile(current.value,fi->second.profile_id);if(!p||bytes!=p->bytes||usage!=396)throw std::runtime_error("official pool actual allocation differs from frozen graph value");
  auto&f=fi->second;auto&o=f.objects[p->object];
  if(!o.handle){o.handle=workspace().acquire(f.profile_id,p->object,actual_device,usage,create,addref,rel);if(!o.handle)o.handle=create(p->capacity);if(!o.handle)throw std::runtime_error("official object physical allocation failed");o.manager=manager;o.usage=usage;release=rel;handles[o.handle]++;++f.creates;*fresh=true;}
  else {if(o.manager!=manager||o.usage!=usage)throw std::runtime_error("official object queue/allocator mismatch");++f.hits;*fresh=false;}
  addref(o.handle);++f.client_addrefs;return o.handle;
 }
 bool client_release(uint64_t h){std::lock_guard<std::mutex>l(mu);if(!handles.count(h))return false;for(auto&[_,f]:frames)for(auto&[__,o]:f.objects)if(o.handle==h){++f.client_releases;return true;}return true;}
 void close(uintptr_t id)noexcept{try{std::lock_guard<std::mutex>l(mu);auto it=frames.find(id);if(it==frames.end())return;auto&f=it->second;uint64_t capacity=0;for(auto&[index,o]:f.objects)if(o.handle){capacity+=GetProfile(f.profile_id).sizes[index];release(o.handle);++f.owner_releases;auto h=handles.find(o.handle);if(h!=handles.end()&&!--h->second)handles.erase(h);}
  std::fprintf(stderr,"RECONSTRUCTION_OFFICIAL_POOL_END frame=%llu creates=%llu hits=%llu client_addrefs=%llu client_release_before_frame_end=%llu owner_releases=%llu lazy_capacity_B=%llu\n",(unsigned long long)id,(unsigned long long)f.creates,(unsigned long long)f.hits,(unsigned long long)f.client_addrefs,(unsigned long long)f.client_releases,(unsigned long long)f.owner_releases,(unsigned long long)capacity);frames.erase(it);
 }catch(...){std::fprintf(stderr,"RECONSTRUCTION_OFFICIAL_POOL_CLOSE_ERROR\n");}}
};
inline Pool& pool(){static auto*p=new Pool;return *p;}
}
extern "C" void reconstruction_official_object_value_enter(uintptr_t f,size_t v)noexcept{reconstruction_official_object::current={f,v};}
extern "C" void reconstruction_official_object_value_leave()noexcept{reconstruction_official_object::current={};}
extern "C" bool reconstruction_official_object_frame_known(uintptr_t f)noexcept{return reconstruction_official_object::pool().known(f);}
extern "C" void reconstruction_official_object_frame_register(uintptr_t f,const size_t*v,size_t n){reconstruction_official_object::pool().register_frame(f,v,n);}
extern "C" bool reconstruction_official_object_has_value(uintptr_t f,size_t v)noexcept{return reconstruction_official_object::pool().contains(f,v);}
extern "C" void reconstruction_official_object_frame_close(uintptr_t f)noexcept{reconstruction_official_object::pool().close(f);}
extern "C" bool reconstruction_official_object_buffer_shared(uintptr_t h)noexcept{return reconstruction_official_object::pool().shared(h);}

extern "C" void reconstruction_official_object_frame_register_profile(uintptr_t f,const size_t*v,size_t n,size_t id){reconstruction_official_object::pool().register_frame(f,v,n,id);}

extern "C" int reconstruction_graph_owners_v1(uint64_t*,uint64_t*)noexcept;
extern "C" __attribute__((visibility("default"))) int reconstruction_capture_workspace_snapshot_v1(uint64_t*n,uint64_t*b,uint64_t*l)noexcept{try{if(!n||!b||!l)return 1;reconstruction_official_object::workspace().snapshot(n,b,l);return 0;}catch(...){return 1;}}
extern "C" __attribute__((visibility("default"))) int reconstruction_capture_workspace_retire_v1()noexcept{try{auto&p=reconstruction_official_object::pool();std::lock_guard<std::mutex>g(p.mu);uint64_t k=0,c=0;if(!p.frames.empty()||reconstruction_graph_owners_v1(&k,&c)||k||c)return 2;reconstruction_official_object::workspace().retire();return 0;}catch(...){return 1;}}
