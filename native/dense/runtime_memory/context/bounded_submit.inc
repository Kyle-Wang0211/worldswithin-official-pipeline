                                                                                                 
#include <algorithm>
#include <memory>
#include <string_view>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <cstring>
#include <deque>
namespace {
using PwFifoStatus=onnxruntime::common::Status;
using PwFifoClock=std::chrono::steady_clock;
struct PwBoundedPolicy {
 uint32_t limit=0;bool enabled=false,invalid=false;
 PwBoundedPolicy(){const char*v=std::getenv("RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS");if(v&&*v){if(!std::strcmp(v,"0"))limit=0;else if(!std::strcmp(v,"1"))limit=1;else if(!std::strcmp(v,"2"))limit=2;else if(!std::strcmp(v,"4"))limit=4;else invalid=true;}enabled=limit!=0;}
};
const PwBoundedPolicy& PwGetBoundedPolicy(){static const PwBoundedPolicy p;return p;}
struct PwFifoCompletionState {std::atomic<bool>done{false};wgpu::QueueWorkDoneStatus status=wgpu::QueueWorkDoneStatus::Error;char message[192]{};};
struct PwFifoEntry {uint64_t sequence;wgpu::Instance instance;wgpu::Device device;wgpu::Queue queue;wgpu::Future future;std::shared_ptr<PwFifoCompletionState> state;};
struct PwFifoStats {uint64_t submits=0,waits=0,completed=0,resolved=0,failures=0,timeouts=0,slices=0,ticks=0,events=0,queued=0,peak=0,first_wait_at_submit=0,drains=0,abandoned_unknown=0;uint32_t last_wait=0,last_queue=0;bool failed_latch=false;};
struct PwFifoRegistry {std::recursive_mutex serial;std::mutex stats_mutex;std::deque<PwFifoEntry> entries;PwFifoStats stats;};
                                                                                             
                                                                                              
PwFifoRegistry& PwFifo(){static auto*r=new PwFifoRegistry;return*r;}
std::unique_lock<std::recursive_mutex> PwBoundedSerializeFlush(){auto&r=PwFifo();std::unique_lock<std::recursive_mutex>l(r.serial,std::defer_lock);if(PwGetBoundedPolicy().enabled)l.lock();return l;}
std::string PwBoundedSubmitJson(){const auto&p=PwGetBoundedPolicy();auto&r=PwFifo();std::lock_guard<std::mutex>l(r.stats_mutex);const auto&s=r.stats;std::ostringstream o;o<<"{\"policy_version\":2,\"enabled\":"<<p.enabled<<",\"invalid_policy\":"<<p.invalid<<",\"max_inflight_policy\":"<<p.limit<<",\"scope\":\"runtime_Flush_FIFO_all_contexts\",\"wait_slice_ms\":25,\"wait_deadline_ms\":30000,\"submits\":"<<s.submits<<",\"waits\":"<<s.waits<<",\"completed\":"<<s.completed<<",\"resolved_callbacks\":"<<s.resolved<<",\"failures\":"<<s.failures<<",\"deadline_timeouts\":"<<s.timeouts<<",\"waitany_calls\":"<<s.slices<<",\"DeviceTick_calls\":"<<s.ticks<<",\"ProcessEvents_calls\":"<<s.events<<",\"inflight_or_unknown\":"<<s.queued<<",\"max_inflight_observed\":"<<s.peak<<",\"first_wait_at_submit\":"<<s.first_wait_at_submit<<",\"drains\":"<<s.drains<<",\"teardown_abandoned_unknown\":"<<s.abandoned_unknown<<",\"last_wait_status\":"<<s.last_wait<<",\"last_queue_status\":"<<s.last_queue<<",\"failed_latch\":"<<s.failed_latch<<"}";return o.str();}
PwFifoStatus PwBoundedCheckPolicy(){const auto&p=PwGetBoundedPolicy();if(p.invalid)return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"FIFOv2 accepts RECONSTRUCTION_BENCH_MAX_INFLIGHT_SUBMITS only0/1/2/4 orunset");if(p.enabled){auto&r=PwFifo();std::lock_guard<std::mutex>l(r.stats_mutex);if(r.stats.failed_latch)return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"FIFOv2 failure latched; no further Flush submits permitted");}return PwFifoStatus::OK();}
void PwFifoLatch(){auto&r=PwFifo();std::lock_guard<std::mutex>l(r.stats_mutex);r.stats.failed_latch=true;}
void PwFifoPruneFront(){auto&r=PwFifo();while(!r.entries.empty()&&r.entries.front().state->done.load(std::memory_order_acquire))r.entries.pop_front();std::lock_guard<std::mutex>l(r.stats_mutex);r.stats.queued=r.entries.size();}
PwFifoStatus PwFifoWaitEntry(PwFifoEntry&e,PwFifoClock::time_point deadline){
 auto&r=PwFifo();{std::lock_guard<std::mutex>l(r.stats_mutex);++r.stats.waits;if(!r.stats.first_wait_at_submit)r.stats.first_wait_at_submit=r.stats.submits;}
 wgpu::WaitStatus ws=wgpu::WaitStatus::TimedOut;bool expired=false;
 while(true){auto now=PwFifoClock::now();uint64_t ns=now>=deadline?0:std::min<uint64_t>(25000000ull,std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-now).count());ws=e.instance.WaitAny(e.future,ns);{std::lock_guard<std::mutex>l(r.stats_mutex);++r.stats.slices;r.stats.last_wait=uint32_t(ws);}if(ws!=wgpu::WaitStatus::TimedOut)break;if(PwFifoClock::now()>=deadline){expired=true;break;}}
 const bool done=e.state->done.load(std::memory_order_acquire);const bool good=ws==wgpu::WaitStatus::Success&&done&&e.state->status==wgpu::QueueWorkDoneStatus::Success;
 if(good){e.device.Tick();e.instance.ProcessEvents();}
 {std::lock_guard<std::mutex>l(r.stats_mutex);auto&s=r.stats;if(done){++s.resolved;s.last_queue=uint32_t(e.state->status);}if(good){++s.completed;++s.ticks;++s.events;}else{++s.failures;s.timeouts+=expired;s.failed_latch=true;}}
 if(!good)return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"FIFOv2 completion failed seq=",e.sequence," wait=",uint32_t(ws)," done=",done," queue=",done?uint32_t(e.state->status):UINT32_MAX," deadline=",expired," message=",done?e.state->message:"Dawn owns pending heap callback");
 return PwFifoStatus::OK();
}
                                                                                             
                                                                                                 
PwFifoStatus PwFifoDrainLocked(PwFifoClock::time_point deadline,bool teardown){
 auto&r=PwFifo();{std::lock_guard<std::mutex>l(r.stats_mutex);++r.stats.drains;}
 PwFifoStatus first;
 for(auto&e:r.entries){if(e.state->done.load(std::memory_order_acquire))continue;auto s=PwFifoWaitEntry(e,deadline);if(!s.IsOK()&&first.IsOK())first=s;}
 PwFifoPruneFront();
 if(teardown&&!r.entries.empty()){
  uint64_t unknown=0;for(auto&e:r.entries)unknown+=!e.state->done.load(std::memory_order_acquire);
  {std::lock_guard<std::mutex>l(r.stats_mutex);r.stats.abandoned_unknown+=unknown;r.stats.failed_latch=true;}
                                                                                        
                                                                                         
  r.entries.clear();{std::lock_guard<std::mutex>l(r.stats_mutex);r.stats.queued=0;}
 }
 return first;
}
PwFifoStatus PwBoundedAfterSubmit(const wgpu::Instance&instance,const wgpu::Device&device,const wgpu::Queue&queue,bool force_drain=false){
 const auto&p=PwGetBoundedPolicy();if(!p.enabled)return PwFifoStatus::OK();auto&r=PwFifo();
 try{
  auto state=std::make_shared<PwFifoCompletionState>();
  auto future=queue.OnSubmittedWorkDone(wgpu::CallbackMode::WaitAnyOnly,[state](wgpu::QueueWorkDoneStatus status,wgpu::StringView message)noexcept{state->status=status;auto v=static_cast<std::string_view>(message);size_t n=std::min(v.size(),sizeof(state->message)-1);if(n)std::memcpy(state->message,v.data(),n);state->message[n]=0;state->done.store(true,std::memory_order_release);});
  uint64_t sequence;{std::lock_guard<std::mutex>l(r.stats_mutex);sequence=++r.stats.submits;}
  r.entries.push_back({sequence,instance,device,queue,future,std::move(state)});
  {std::lock_guard<std::mutex>l(r.stats_mutex);r.stats.queued=r.entries.size();r.stats.peak=std::max<uint64_t>(r.stats.peak,r.entries.size());}
  auto deadline=PwFifoClock::now()+std::chrono::seconds(30);
  PwFifoStatus status;
  if(force_drain){PwFifoLatch();status=PwFifoDrainLocked(deadline,false);}
  else if(r.entries.size()>=p.limit){status=PwFifoWaitEntry(r.entries.front(),deadline);PwFifoPruneFront();if(!status.IsOK()){auto drain=PwFifoDrainLocked(deadline,false);if(!drain.IsOK())return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,status.ErrorMessage(),"; drain: ",drain.ErrorMessage());}}
  if(!status.IsOK())return status;
  uint64_t completed;{std::lock_guard<std::mutex>l(r.stats_mutex);completed=r.stats.completed;}
  if(sequence==1||(completed&&completed%64==0)){if(onnxruntime::logging::LoggingManager::HasDefaultLogger()){LOGS_DEFAULT(WARNING)<<"RECONSTRUCTION_BOUNDED_SUBMIT_FIFO "<<PwBoundedSubmitJson();}}
  return PwFifoStatus::OK();
 }catch(...){PwFifoLatch();throw;}
}
void PwFifoDrainOnException()noexcept{if(!PwGetBoundedPolicy().enabled)return;try{PwFifoLatch();auto s=PwFifoDrainLocked(PwFifoClock::now()+std::chrono::seconds(30),false);if(!s.IsOK()&&onnxruntime::logging::LoggingManager::HasDefaultLogger()){LOGS_DEFAULT(ERROR)<<s.ErrorMessage();}}catch(...){}}
void PwFifoShutdownDrain()noexcept{if(!PwGetBoundedPolicy().enabled)return;try{auto guard=PwBoundedSerializeFlush();auto s=PwFifoDrainLocked(PwFifoClock::now()+std::chrono::seconds(30),true);if(!s.IsOK()&&onnxruntime::logging::LoggingManager::HasDefaultLogger()){LOGS_DEFAULT(ERROR)<<"FIFOv2 teardown: "<<s.ErrorMessage();}}catch(...){PwFifoLatch();}}
}

                                                                              
                                                                               
namespace { thread_local bool pw_workspace_phase_active=false; thread_local uint64_t pw_phase_deferred_flushes=0;
bool PwPhaseBoundaryEnabled(){const char*p=std::getenv("RECONSTRUCTION_PHASE_BOUNDARY_DRAIN");return p&&p[0]=='1'&&!p[1];} }
extern "C" __attribute__((visibility("default"))) int reconstruction_workspace_phase_enter_v2()noexcept {
 try {auto&r=PwFifo();r.serial.lock();if(pw_workspace_phase_active){r.serial.unlock();return 1;}
 {std::lock_guard<std::mutex>g(r.stats_mutex);if(r.stats.failed_latch){r.serial.unlock();return 2;}}
 pw_workspace_phase_active=true;return 0;}catch(...){return 3;}
}
extern "C" __attribute__((visibility("default"))) int reconstruction_workspace_phase_active_v2()noexcept {return pw_workspace_phase_active?1:0;}
extern "C" __attribute__((visibility("default"))) int reconstruction_workspace_phase_leave_v2(int success)noexcept {
 if(!pw_workspace_phase_active){return 1;}
 if(!success){PwFifoLatch();}
 int result=0;
 if(PwPhaseBoundaryEnabled()){
                                                                               
  for(const auto&e:PwFifo().entries)if(e.state->done.load(std::memory_order_acquire)&&e.state->status!=wgpu::QueueWorkDoneStatus::Success)result=4;
  {std::lock_guard<std::mutex>g(PwFifo().stats_mutex);if(PwFifo().stats.failed_latch)result=4;}
  auto status=PwFifoDrainLocked(PwFifoClock::now()+std::chrono::seconds(30),false);
  for(const auto&e:PwFifo().entries)if(!e.state->done.load(std::memory_order_acquire)||e.state->status!=wgpu::QueueWorkDoneStatus::Success)result=4;
  if(!status.IsOK())result=4;
  if(result)PwFifoLatch();
  std::fprintf(stderr,"PHASE_BOUNDARY completed=%d deferred_flushes=%llu remaining=%zu success=%d\n",int(!result),(unsigned long long)pw_phase_deferred_flushes,PwFifo().entries.size(),success);
  pw_phase_deferred_flushes=0;
 }
 pw_workspace_phase_active=false;PwFifo().serial.unlock();return result;
}
