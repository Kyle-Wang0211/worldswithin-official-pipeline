#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <array>
#include <mutex>
#include <cstring>
namespace pw_c2 {
class DawnMapOps {
 struct State {
  wgpu::Instance instance;WGPUBuffer buffer;std::atomic<bool> done{false};WGPUMapAsyncStatus status=WGPUMapAsyncStatus_Error;
  State(const wgpu::Instance&i,WGPUBuffer b):instance(i),buffer(b){wgpuBufferAddRef(buffer);}
  ~State(){wgpuBufferRelease(buffer);}
 };
 inline static std::mutex retained_mutex_;
 inline static std::array<State*,64> retained_{};
 const wgpu::Instance& instance_;WGPUBuffer dst_;void*src_;size_t bytes_;
 std::function<void()> flush_;std::unique_ptr<State> state_;wgpu::Future future_{};
 bool started_=false;
 std::chrono::steady_clock::time_point deadline_;
 void Retain()noexcept{
  State* p=state_.release();if(!p)return;
  try{std::lock_guard<std::mutex>g(retained_mutex_);for(auto&slot:retained_)if(!slot){slot=p;return;}}catch(...){}
                                                                                     
                                                                                      
 }
 public:
 DawnMapOps(const wgpu::Instance&i,WGPUBuffer d,void*s,size_t n,std::function<void()>f,std::chrono::milliseconds budget=std::chrono::seconds(30)):instance_(i),dst_(d),src_(s),bytes_(n),flush_(std::move(f)),deadline_(std::chrono::steady_clock::now()+budget){}
 ~DawnMapOps(){
  if(!state_)return;
  if(started_&&!state_->done.load(std::memory_order_acquire)){
   wgpuBufferUnmap(dst_);Retain();return;
  }
  if(wgpuBufferGetMapState(dst_)!=WGPUBufferMapState_Unmapped)wgpuBufferUnmap(dst_);
 }
 void Flush(){flush_();}
 void Begin(){
  state_=std::make_unique<State>(instance_,dst_);WGPUBufferMapCallbackInfo info{};
  info.mode=WGPUCallbackMode_WaitAnyOnly;info.userdata1=state_.get();
  info.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void*one,void*)noexcept{
   auto*s=static_cast<State*>(one);s->status=status;s->done.store(true,std::memory_order_release);
  };
  auto f=wgpuBufferMapAsync(dst_,WGPUMapMode_Write,0,wgpuBufferGetSize(dst_),info);future_.id=f.id;started_=true;
 }
 bool Expired()const{return std::chrono::steady_clock::now()>=deadline_;}
 Poll PollOnce(){
  auto status=instance_.WaitAny(future_,25000000ULL);
  if(status==wgpu::WaitStatus::TimedOut)return Poll::Pending;
  if(status!=wgpu::WaitStatus::Success)throw std::runtime_error("C2 bounded WaitAny failure");
  if(!state_->done.load(std::memory_order_acquire))throw std::runtime_error("C2 callback not observed");
  return state_->status==WGPUMapAsyncStatus_Success?Poll::Success:Poll::Error;
 }
 bool Mapped()const{return wgpuBufferGetMapState(dst_)==WGPUBufferMapState_Mapped;}
 void WriteUnmap(){
  void*p=wgpuBufferGetMappedRange(dst_,0,WGPU_WHOLE_MAP_SIZE);
  if(!p)throw std::runtime_error("C2 mapped range unavailable");
  std::memcpy(p,src_,bytes_);wgpuBufferUnmap(dst_);
  if(wgpuBufferGetMapState(dst_)!=WGPUBufferMapState_Unmapped)throw std::runtime_error("C2 Unmap state not observed");
 }
 void CancelRetain(){
  wgpuBufferUnmap(dst_);
  auto status=instance_.WaitAny(future_,0);
  if(status!=wgpu::WaitStatus::Success||!state_->done.load(std::memory_order_acquire))Retain();
 }
};
}
