#include <chrono>
#include <cstdio>
#pragma once
#include "onnxruntime_c_api.h"
#include "s1_memory_probe.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include <utility>
namespace pw_segment_v9 {
inline void check(const OrtApi*a,OrtStatus*s){if(s){std::string m=a->GetErrorMessage(s);a->ReleaseStatus(s);throw std::runtime_error(m);}}
// Non-owning RunOptions registration. clear_run synchronizes with the sampler mutex;
// its return precedes destruction of this scope and the caller-owned RunOptions.
struct ActiveRunScope {
 const OrtApi*api;OrtRunOptions*run;bool registered=false;
 static int cancel(void*p) noexcept {
  auto*self=static_cast<ActiveRunScope*>(p);
  OrtStatus*status=self->api->RunOptionsSetTerminate(self->run);
  if(status){self->api->ReleaseStatus(status);return -1;}return 0;
 }
 ActiveRunScope(const OrtApi*a,const OrtRunOptions*r):api(a),run(const_cast<OrtRunOptions*>(r)){
  if(!api||!run)throw std::logic_error("segment activeRun requires real RunOptions");
  char error[1024];if(pw_s1_memory_preflight("S6_SEGMENT_BEFORE_RUN",0,error,sizeof(error)))throw std::runtime_error(error);
  registered=true;
  try{pw_s1_memory_active_run(this,&cancel);}catch(...){pw_s1_memory_clear_run();registered=false;throw;}
  if(pw_s1_memory_guard_tripped()){pw_s1_memory_clear_run();registered=false;throw std::runtime_error("S6_SEGMENT_GUARD_TRIPPED_BEFORE_RUN");}
 }
 ~ActiveRunScope(){if(registered)pw_s1_memory_clear_run();}
 ActiveRunScope(const ActiveRunScope&)=delete;ActiveRunScope&operator=(const ActiveRunScope&)=delete;
};
inline void budget_boundary(const char*phase){char e[1024];if(pw_s1_memory_preflight(phase,0,e,sizeof(e)))throw std::runtime_error(e);}
struct OwnedBoundary {
 const OrtApi*api=nullptr;OrtValue*value=nullptr;std::shared_ptr<void> allocator_owner;
 OwnedBoundary(const OrtApi*a,OrtValue*v,std::shared_ptr<void>o):api(a),value(v),allocator_owner(std::move(o)){}
 OwnedBoundary(const OwnedBoundary&)=delete;OwnedBoundary&operator=(const OwnedBoundary&)=delete;
 OwnedBoundary(OwnedBoundary&&v)noexcept:api(v.api),value(std::exchange(v.value,nullptr)),allocator_owner(std::move(v.allocator_owner)){}
 ~OwnedBoundary(){if(value)api->ReleaseValue(value);}
};
struct Resources {
 const OrtApi*api;OrtAllocator*host=nullptr;OrtIoBinding*binding=nullptr;OrtValue**raw=nullptr;size_t count=0;
 std::shared_ptr<void> child_owner,anchor_owner;std::vector<std::shared_ptr<OwnedBoundary>> pending_destinations;bool pending=false;
 explicit Resources(const OrtApi*a):api(a){}
 ~Resources(){if(raw){for(size_t i=0;i<count;++i)if(raw[i])api->ReleaseValue(raw[i]);host->Free(host,raw);}if(binding)api->ReleaseIoBinding(binding);}
};
// At most one failed invocation: caller latches scene failed and stops. Retain
// uncertain GPU resources rather than tearing down their EP allocator.
inline std::vector<std::shared_ptr<Resources>>&failure_capsules(){static auto*p=new std::vector<std::shared_ptr<Resources>>;return*p;}
inline std::vector<OwnedBoundary> run_segment(
 const OrtApi*api,OrtEnv*env,OrtSession*session,const std::shared_ptr<void>&child_owner,const OrtRunOptions*run,
 const std::vector<std::pair<std::string,const OrtValue*>>&inputs,const std::vector<std::string>&outputs,
 const OrtMemoryInfo*gpu_info,OrtAllocator*anchor_allocator,const std::shared_ptr<void>&anchor_owner,
 const std::function<void()>&completion,bool captured=true) {
 if(!api||!env||!session||!child_owner||!gpu_info||!anchor_allocator||!anchor_owner||!completion||!failure_capsules().empty())throw std::logic_error("segment prerequisites/latched failure");
 auto pw_clock=std::chrono::steady_clock::now();auto s=std::make_shared<Resources>(api);s->child_owner=child_owner;s->anchor_owner=anchor_owner;
 try{
  check(api,api->CreateIoBinding(session,&s->binding));
  for(const auto&i:inputs)check(api,api->BindInput(s->binding,i.first.c_str(),i.second));
  for(const auto&o:outputs)check(api,api->BindOutputToDevice(s->binding,o.c_str(),gpu_info));
  {ActiveRunScope active(api,run);s->pending=true;check(api,api->RunWithBinding(session,run,s->binding));check(api,api->SynchronizeBoundOutputs(s->binding));completion();s->pending=false;}
  auto pw_run_done=std::chrono::steady_clock::now();budget_boundary("S6_SEGMENT_AFTER_RUN_COMPLETION");
  check(api,api->GetAllocatorWithDefaultOptions(&s->host));check(api,api->GetBoundOutputValues(s->binding,s->host,&s->raw,&s->count));
  if(s->count!=outputs.size())throw std::runtime_error("segment output count");
  api->ClearBoundOutputs(s->binding);
  if(captured)check(api,api->SessionReleaseCapturedGraph(session,0));
  auto pw_graph_released=std::chrono::steady_clock::now();std::vector<OwnedBoundary>result;result.reserve(s->count);
  for(size_t i=0;i<s->count;++i){
   OrtTensorTypeAndShapeInfo*info=nullptr;check(api,api->GetTensorTypeAndShape(s->raw[i],&info));
   struct Info{const OrtApi*a;OrtTensorTypeAndShapeInfo*p;~Info(){a->ReleaseTensorTypeAndShapeInfo(p);}}g{api,info};
   size_t rank=0;check(api,api->GetDimensionsCount(info,&rank));std::vector<int64_t>dims(rank);check(api,api->GetDimensions(info,dims.data(),rank));
   ONNXTensorElementDataType type;check(api,api->GetTensorElementType(info,&type));OrtValue*dst=nullptr;
   budget_boundary("S6_SEGMENT_BEFORE_BOUNDARY_ALLOC");
   check(api,api->CreateTensorAsOrtValue(anchor_allocator,dims.data(),rank,type,&dst));
   auto keep=std::make_shared<OwnedBoundary>(api,dst,anchor_owner);s->pending_destinations.push_back(keep);
   const OrtValue*src[]={s->raw[i]};OrtValue*to[]={dst};
   budget_boundary("S6_SEGMENT_BEFORE_BOUNDARY_COPY");
   s->pending=true;
   check(api,api->CopyTensors(env,src,to,nullptr,1));completion();s->pending=false;
   budget_boundary("S6_SEGMENT_AFTER_BOUNDARY_COMPLETION");
   api->ReleaseValue(s->raw[i]);s->raw[i]=nullptr;result.emplace_back(api,std::exchange(keep->value,nullptr),anchor_owner);s->pending_destinations.pop_back();
  }
  auto pw_transfer_done=std::chrono::steady_clock::now();std::fprintf(stderr,"SEGMENT_TRANSFER_WALL run_binding_completion_s=%.9f getoutputs_graphrelease_s=%.9f boundary_allocate_copy_wait_s=%.9f outputs=%zu\n",std::chrono::duration<double>(pw_run_done-pw_clock).count(),std::chrono::duration<double>(pw_graph_released-pw_run_done).count(),std::chrono::duration<double>(pw_transfer_done-pw_graph_released).count(),s->count);return result;
 }catch(...){
  if(s->pending){try{completion();s->pending=false;}catch(...){failure_capsules().push_back(s);}}
  throw;
 }
}
}
