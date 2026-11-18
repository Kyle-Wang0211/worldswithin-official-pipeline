                                                                                              
namespace {
struct ReconstructionDrainPolicy {bool enabled=false,invalid=false;ReconstructionDrainPolicy(){const char*v=std::getenv("RECONSTRUCTION_SUBMIT_DRAIN");if(v&&*v){enabled=!std::strcmp(v,"1");invalid=!enabled&&std::strcmp(v,"0");}}};
const ReconstructionDrainPolicy& ReconstructionDrainPolicyValue(){static const ReconstructionDrainPolicy p;return p;}
struct ReconstructionDrainState {std::atomic<bool>done{false};wgpu::QueueWorkDoneStatus queue=wgpu::QueueWorkDoneStatus::Error;};
struct ReconstructionDrainRetained {wgpu::Instance instance;wgpu::Device device;wgpu::Queue queue;std::shared_ptr<ReconstructionDrainState>state;wgpu::Future future;};
struct ReconstructionDrainStats {std::mutex mu;uint64_t calls=0,success=0,failures=0,serial=0,instance=0,device=0;uint32_t wait=UINT32_MAX,queue=UINT32_MAX;bool done=false,retained=false;};
ReconstructionDrainStats&ReconstructionDrainObservation(){static auto*s=new ReconstructionDrainStats;return*s;}
std::unique_ptr<ReconstructionDrainRetained>&ReconstructionDrainFailureRetention(){static auto*p=new std::unique_ptr<ReconstructionDrainRetained>;return*p;}
std::string ReconstructionDrainDrainJson(){auto&s=ReconstructionDrainObservation();std::lock_guard<std::mutex>g(s.mu);std::ostringstream o;o<<"{\"schema\":\"reconstruction.submit_drain/1\",\"enabled\":"<<ReconstructionDrainPolicyValue().enabled<<",\"invalid\":"<<ReconstructionDrainPolicyValue().invalid<<",\"scope\":\"each_Flush_same_context_queue_not_ref_or_node_A\",\"calls\":"<<s.calls<<",\"success\":"<<s.success<<",\"failures\":"<<s.failures<<",\"submit_serial\":"<<s.serial<<",\"instance\":"<<s.instance<<",\"device\":"<<s.device<<",\"wait_raw\":"<<s.wait<<",\"queue_raw\":"<<s.queue<<",\"callback\":"<<s.done<<",\"retained\":"<<s.retained<<",\"owner_lease_proven\":false}";return o.str();}
PwFifoStatus ReconstructionDrainValidatePolicy(){const auto&p=ReconstructionDrainPolicyValue();if(p.invalid)return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"Submit drain drain accepts unset/0/1 only");if(p.enabled&&(!PwGetBoundedPolicy().enabled||PwGetBoundedPolicy().limit!=4))return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"Submit drain drain requires unchanged FIFO4 serial scope");return PwFifoStatus::OK();}
template<class CaptureState> PwFifoStatus ReconstructionDrainValidateCapture(CaptureState state){if(state!=CaptureState::Default&&state!=CaptureState::Capturing&&state!=CaptureState::Replaying)return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"Submit drain drain unknown capture state");return PwFifoStatus::OK();}
                                                                                 
PwFifoStatus ReconstructionDrainRejectReadyError(){for(const auto&e:PwFifo().entries)if(e.state->done.load(std::memory_order_acquire)&&e.state->status!=wgpu::QueueWorkDoneStatus::Success){PwFifoLatch();return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"Submit drain drain retains prior ready-error FIFO entry");}return PwFifoStatus::OK();}
PwFifoStatus ReconstructionDrainFreshCompletion(const wgpu::Instance&instance,const wgpu::Device&device,const wgpu::Queue&queue){
 auto e=std::make_unique<ReconstructionDrainRetained>();e->instance=instance;e->device=device;e->queue=queue;e->state=std::make_shared<ReconstructionDrainState>();auto state=e->state;
 auto&s=ReconstructionDrainObservation();{std::lock_guard<std::mutex>g(s.mu);++s.calls;s.instance=reinterpret_cast<uintptr_t>(instance.Get());s.device=reinterpret_cast<uintptr_t>(device.Get());s.done=false;s.wait=s.queue=UINT32_MAX;}{std::lock_guard<std::mutex>g(PwFifo().stats_mutex);std::lock_guard<std::mutex>obs(s.mu);s.serial=PwFifo().stats.submits;}
 e->future=queue.OnSubmittedWorkDone(wgpu::CallbackMode::WaitAnyOnly,[state,instance,device,queue](wgpu::QueueWorkDoneStatus status,wgpu::StringView)noexcept{(void)instance;(void)device;(void)queue;state->queue=status;state->done.store(true,std::memory_order_release);});
 const auto deadline=PwFifoClock::now()+std::chrono::seconds(30);auto wait=wgpu::WaitStatus::TimedOut;
 do{auto now=PwFifoClock::now();auto ns=now>=deadline?0:std::min<uint64_t>(25000000ULL,std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-now).count());wait=instance.WaitAny(e->future,ns);if(wait!=wgpu::WaitStatus::TimedOut)break;}while(PwFifoClock::now()<deadline);
 const bool done=state->done.load(std::memory_order_acquire);const bool ok=wait==wgpu::WaitStatus::Success&&done&&state->queue==wgpu::QueueWorkDoneStatus::Success;
 {std::lock_guard<std::mutex>g(s.mu);s.wait=uint32_t(wait);s.done=done;s.queue=done?uint32_t(state->queue):UINT32_MAX;s.success+=ok;s.failures+=!ok;s.retained=!ok;}
 if(!ok){PwFifoLatch();ReconstructionDrainFailureRetention()=std::move(e);return ORT_MAKE_STATUS(ONNXRUNTIME,FAIL,"Submit drain fresh queue completion failed; keep handles and do not Refresh");}
 return PwFifoStatus::OK();
}
}
