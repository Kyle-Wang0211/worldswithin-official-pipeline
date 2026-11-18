#include "dense_s1_adapter.h"
#include "../../memory_policy.h"
#include "segment_plan_binding.h"
#include "bench_noise_input.h"
#include "reconstruction_s1_memory.h"
#include "s1_memory_probe.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
extern "C" uint32_t reconstruction_feature_transport_abi_v1() noexcept;
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <set>
#include <stdexcept>
extern "C" int ReconstructionRegisterExperimentalSchema() noexcept;
namespace aether::dense { namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
std::string model(const char* name){const char* value=std::getenv(name);if(!value||value[0]!='/')throw std::runtime_error(std::string("missing frozen absolute S1 model path: ")+name);return value;}
class Source final:public reconstruction::ResidentInputSource {
 int noise_=-1,width_;size_t frames_;DenseS1Pass::ReadHalf read_;std::vector<float> cams_;std::vector<int32_t> nb_,orig_;
 public:
 std::unique_ptr<reconstruction::mobile_segment_runtime::ScopedPlan> segment_plan;
 Source(int w,const std::vector<float>& c,const std::vector<int32_t>&n,const std::vector<int>&r,int fd,DenseS1Pass::ReadHalf read):width_(w),frames_(r.size()),read_(std::move(read)),cams_(c),nb_(n),orig_(r.begin(),r.end()){
  if((w!=768&&w!=1536)||!frames_||c.size()!=frames_*36||n.size()!=frames_*9||!read_)throw std::runtime_error("S1 full13 original scene/table size");
  std::set<int32_t> ids(orig_.begin(),orig_.end());if(ids.size()!=frames_||*ids.begin()!=0||size_t(*ids.rbegin())!=frames_-1)throw std::runtime_error("S1 full13 exact original row permutation required");
  for(size_t ref=0;ref<frames_;++ref){std::set<int32_t>v{int32_t(ref)};for(int j=0;j<9;++j){int x=n[ref*9+j];if(x<0||size_t(x)>=frames_||!v.insert(x).second)throw std::runtime_error("S1 full13 ref+9 ordered unique source views");}}
  std::string e;size_t bytes=0;if(!BenchNoiseInput::expected_bytes(frames_,w,w*3/4,&bytes,&e)||!BenchNoiseFileInput::validate_fd(fd,bytes,&e))throw std::runtime_error(e);
  int flags=fcntl(fd,F_GETFL);if(flags<0||(flags&O_ACCMODE)!=O_RDONLY)throw std::runtime_error("S1 full13 frozen read-only noise fd required");
  noise_=dup(fd);if(noise_<0)throw std::runtime_error("retain frozen noise fd");
 }
 ~Source(){if(noise_>=0)close(noise_);}
 std::vector<uint16_t> ReadHalf(int view)const override{if(view<0||size_t(view)>=frames_)throw std::runtime_error("S1 image view out of range");auto x=read_(view);if(x.size()!=size_t(width_)*(width_*3/4))throw std::runtime_error("S1 borrowed original half plane size");return x;}
 void ReadNoise(int ref,std::vector<float>&n2,std::vector<float>&n3)const override{if(ref<0||size_t(ref)>=frames_)throw std::runtime_error("S1 noise reference out of range");size_t N=size_t(width_)*(width_*3/4);std::string e;if(!BenchNoiseFileInput::read_frame(noise_,size_t(orig_[ref]),frames_,N/16,N/4,n2,n3,&e))throw std::runtime_error(e);}
 const std::vector<float>& Cameras()const override{return cams_;}
 const std::vector<int32_t>& Neighbors()const override{return nb_;}
};
void configure_once(){static std::mutex m;static bool configured=false;std::lock_guard<std::mutex>g(m);if(configured){if(pw_s1_memory_start()!=0)throw std::runtime_error("continuous sampler restart failed");return;}if(!reconstruction_memory_policy::valid())throw std::runtime_error("unknown explicit memory policy");const auto cap=reconstruction_memory_policy::limit();PwS1MemoryLimits limits{RECONSTRUCTION_S1_MEMORY_ABI,sizeof(PwS1MemoryLimits),cap,cap,cap};if(reconstruction_s1_memory_configure(&limits)!=0)throw std::runtime_error("S1 full13 allocator configuration rejected; must precede any runtime allocation");if(ReconstructionRegisterExperimentalSchema()!=0)throw std::runtime_error("S1 full13 schema registration failed");if(pw_s1_memory_start()!=0)throw std::runtime_error("S1 full13 100ms sampler start failed");configured=true;}
}
DenseS1Pass::DenseS1Pass()=default;DenseS1Pass::~DenseS1Pass()=default;
bool DenseS1Pass::Initialize(int w,const std::vector<float>&c,const std::vector<int32_t>&n,const std::vector<int>&r,int fd,ReadHalf read,std::string*e,double*init_ms){try{auto begin=Clock::now();if(run_)throw std::runtime_error("S1 pass already initialized");configure_once();reconstruction::ResidentSceneConfig cfg;cfg.width=w;cfg.height=w*3/4;cfg.scene_frames=int(r.size());cfg.feature_model=model(w==768?"RECONSTRUCTION_FULL13_S1_FEATURE_LOW":"RECONSTRUCTION_FULL13_S1_FEATURE_HIGH");cfg.rest_model=model(w==768?"RECONSTRUCTION_FULL13_S1_REST_LOW":"RECONSTRUCTION_FULL13_S1_REST_HIGH");cfg.input_source=std::make_shared<Source>(w,c,n,r,fd,std::move(read));cfg.rest_gpu_int64=true;
 const std::string prefix=w==768?"RECONSTRUCTION_S6_MATERIAL_LOW_":"RECONSTRUCTION_S6_MATERIAL_HIGH_";
 const char* index=std::getenv((prefix+"INDEX").c_str());
 if(index){cfg.material_index=index;auto required=[&](const char*k){const char*v=std::getenv((prefix+k).c_str());if(!v||!*v)throw std::runtime_error("incomplete explicit S6 material optin "+prefix+k);return std::string(v);};cfg.material_root=required("ROOT");cfg.material_index_sha=required("INDEX_SHA");cfg.material_manifest_sha=required("MANIFEST_SHA");cfg.material_preprocess_sha=required("PREPROCESS_SHA");}
const char* transfer=std::getenv("PW_GPU_FEATURE_TRANSFER");const bool direct=transfer&&std::strcmp(transfer,"1")==0;
if(direct&&reconstruction_feature_transport_abi_v1()!=1)throw std::runtime_error("direct feature transport capability absent");
if(!index&&!direct)throw std::runtime_error("segmented private native requires explicit perpass S6 material optin");
std::static_pointer_cast<Source>(cfg.input_source)->segment_plan=std::make_unique<reconstruction::mobile_segment_runtime::ScopedPlan>(w,cfg.rest_model);
run_=std::make_unique<reconstruction::PersistentFullMvsGpu>();if(!run_->Initialize(cfg,e))return false;if(init_ms)*init_ms=ms(begin);return true;}catch(const std::exception&x){if(e)*e=x.what();return false;}}
bool DenseS1Pass::Run(int ref,float*const output[4],size_t elements,double*elapsed_ms,std::string*e){try{if(!run_||!output||!elements)throw std::runtime_error("S1 full13 pass/output missing");for(int k=0;k<4;++k)if(!output[k])throw std::runtime_error("S1 full13 null output plane");char text[1024];if(pw_s1_memory_preflight("FULL13_MVS_REF_ENTER",0,text,sizeof(text)))throw std::runtime_error(text);auto begin=Clock::now();reconstruction::ResidentFrameLease lease;if(!run_->RunReference(ref,&lease,e))return false;bool downloaded=run_->DownloadInto(lease,output,elements,e);if(!downloaded)return false;std::string release_error;bool released=run_->Release(&lease,&release_error);if(!released){if(e)*e=release_error;return false;}if(!downloaded)return false;for(int k=0;k<4;++k)for(size_t i=0;i<elements;++i)if(!std::isfinite(output[k][i]))throw std::runtime_error("S1 full13 non-finite LOW/HIGH output before original CER");if(pw_s1_memory_preflight("FULL13_MVS_REF_RETURN",0,text,sizeof(text)))throw std::runtime_error(text);if(elapsed_ms)*elapsed_ms=ms(begin);return true;}catch(const std::exception&x){if(e)*e=x.what();return false;}}
reconstruction::ResidentCounters DenseS1Pass::Counters()const{return run_?run_->Counters():reconstruction::ResidentCounters{};}
}
