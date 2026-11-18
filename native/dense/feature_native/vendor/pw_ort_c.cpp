#include "pw_ort_c.h"
#include "../../memory_policy.h"
#include "s1_memory_probe.h"
#include "reconstruction_s1_memory.h"
#include <cerrno>
#include <onnxruntime_c_api.h>
#include <dlfcn.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
static_assert(ORT_API_VERSION==29,"frozen ORT API revision");
namespace {
using MemoryConfigure=int (*)(const PwS1MemoryLimits*);using MemorySnapshot=int (*)(PwS1MemorySnapshot*,size_t);
MemoryConfigure runtime_memory_configure=nullptr;MemorySnapshot runtime_memory_snapshot=nullptr;
PwS1MemoryLimits runtime_memory_limits{RECONSTRUCTION_S1_MEMORY_ABI,sizeof(PwS1MemoryLimits),2000000000ULL,2000000000ULL,2000000000ULL};
static_assert(sizeof(PwS1MemoryLimits)==32 && sizeof(PwS1MemorySnapshot)==168,"matching frozen memory diagnostic ABI");
const OrtApi* api=nullptr; void* lib=nullptr; std::string runtime_path,version; std::mutex init_mu;
void check(OrtStatus* s){if(s){std::string e=api->GetErrorMessage(s);api->ReleaseStatus(s);throw std::runtime_error(e);}}
void textcopy(const std::string& s,char* b,int n){if(b&&n>0)std::snprintf(b,(size_t)n,"%s",s.c_str());}
std::string quote(const std::string& s){std::string o="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){o+='\\';o+=(char)c;}else if(c<32){char b[7];std::snprintf(b,7,"\\u%04x",c);o+=b;}else o+=(char)c;}return o+'"';}
size_t countof(const std::vector<int64_t>& sh){size_t n=1;if(sh.empty())throw std::runtime_error("scalar inputs unsupported");for(auto d:sh){if(d<=0||uint64_t(d)>SIZE_MAX/n)throw std::runtime_error("invalid/overflow tensor dimension");n*=size_t(d);}return n;}
struct Meta {std::string name;std::vector<int64_t> shape;ONNXTensorElementDataType dtype=ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED;};
struct RuntimeContext {
 RuntimeContext(){if(pw_s1_memory_start()!=0)throw std::runtime_error("S1 memory observer/guard initialization failed");}
 OrtEnv* env=nullptr;std::string logs;std::mutex log_mu;size_t errors=0;
 ~RuntimeContext(){if(env)api->ReleaseEnv(env);pw_s1_memory_stop();}
};
std::weak_ptr<RuntimeContext> shared_context;

}
// Shared owner keeps the official session allocator valid for every exported tensor.
struct DeviceSession {
 std::shared_ptr<RuntimeContext> context;OrtSession* session=nullptr;OrtSessionOptions* options=nullptr;OrtMemoryInfo* gpu_mem=nullptr;OrtAllocator* gpu_allocator=nullptr;
 ~DeviceSession(){if(gpu_allocator)api->ReleaseAllocator(gpu_allocator);if(gpu_mem)api->ReleaseMemoryInfo(gpu_mem);if(session)api->ReleaseSession(session);if(options)api->ReleaseSessionOptions(options);}
};
struct DeviceTensor {
 std::shared_ptr<DeviceSession> owner;OrtValue* value=nullptr;
 ~DeviceTensor(){if(value)api->ReleaseValue(value);}
};
struct pw_ort_model {
 std::shared_ptr<RuntimeContext> context;OrtEnv* env=nullptr;OrtSessionOptions* options=nullptr;OrtSession* session=nullptr;OrtMemoryInfo* mem=nullptr;OrtAllocator* allocator=nullptr;
 std::shared_ptr<DeviceSession> device;std::vector<std::shared_ptr<DeviceTensor>> gpu_inputs,gpu_outputs;std::string cache_mode;size_t gpu_runs=0,gpu_binds=0,gpu_uploads=0,explicit_downloads=0,completion_waits=0,completion_download_bytes=0;double last_enqueue_host_ms=0,last_completion_wait_host_ms=0;
 struct FailureResources {
  bool uncertain=false;
  OrtIoBinding* binding=nullptr; OrtRunOptions* options=nullptr;
  OrtValue** results=nullptr; size_t result_count=0;
  OrtValue* host_alias=nullptr; OrtValue* gpu_alias=nullptr;
  std::shared_ptr<DeviceTensor> upload;
  uint32_t sentinel_storage=0;
 } failure;
 bool failed=false; char failure_error[2048]{};
 std::vector<Meta> ins,outs;std::vector<std::vector<float>> inputs,outputs;std::vector<OrtValue*> values;std::string error,available,logs,profile,profile_prefix;std::mutex log_mu; double load_ms=0,run_ms=0;size_t runs=0;bool profile_ended=false;
 ~pw_ort_model(){for(size_t i=0;i<values.size();++i)if(values[i]&&(i>=gpu_inputs.size()||!gpu_inputs[i]))api->ReleaseValue(values[i]);gpu_outputs.clear();gpu_inputs.clear();if(!device){if(session)api->ReleaseSession(session);if(options)api->ReleaseSessionOptions(options);}if(mem)api->ReleaseMemoryInfo(mem);}
};
namespace {
// One serialized production lane; first failure owns the real model for process
// lifetime. Pointer/field stores and fixed error copies cannot allocate.
pw_ort_model* failed_model_slot=nullptr;
void fixedcopy(char* out,size_t n,const char* text) noexcept {
 if(!out||!n)return;if(!text)text="";size_t k=0;while(k+1<n&&text[k])++k;std::memcpy(out,text,k);out[k]=0;
}
void fail_model(pw_ort_model* m,const char* message) noexcept {
 m->failed=true;if(!failed_model_slot)failed_model_slot=m;
 fixedcopy(m->failure_error,sizeof(m->failure_error),message?message:"unknown native feature failure");
}
void release_run_resources(pw_ort_model* m) noexcept {
 auto& f=m->failure;
 if(f.options){api->ReleaseRunOptions(f.options);f.options=nullptr;}
 if(f.results){for(size_t i=0;i<f.result_count;++i)if(f.results[i])api->ReleaseValue(f.results[i]);m->allocator->Free(m->allocator,f.results);f.results=nullptr;f.result_count=0;}
 if(f.binding){api->ReleaseIoBinding(f.binding);f.binding=nullptr;}
}
}
namespace {
void logger(void* p,OrtLoggingLevel severity,const char* category,const char*,const char*,const char* msg){auto* m=(RuntimeContext*)p;std::lock_guard<std::mutex> g(m->log_mu);if(severity>=ORT_LOGGING_LEVEL_ERROR)++m->errors;if(m->logs.size()<65536)m->logs+=std::to_string(int(severity))+":"+(category?category:"")+":"+(msg?msg:"")+"\n";}
int indexof(const std::vector<Meta>& a,const char* name){if(!name)return -1;for(size_t i=0;i<a.size();++i)if(a[i].name==name)return int(i);return -1;}
size_t element_bytes(ONNXTensorElementDataType type){if(type==ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)return 4;if(type==ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16)return 2;throw std::runtime_error("tensor dtype must be float32 or float16");}
size_t tensor_bytes(const std::vector<int64_t>& shape,ONNXTensorElementDataType type){size_t n=countof(shape),unit=element_bytes(type);if(n>SIZE_MAX/unit)throw std::runtime_error("tensor byte size overflow");return n*unit;}
std::vector<int64_t> shapeinfo(OrtTensorTypeAndShapeInfo* ti,ONNXTensorElementDataType* dtype=nullptr){ONNXTensorElementDataType type;check(api->GetTensorElementType(ti,&type));(void)element_bytes(type);if(dtype)*dtype=type;size_t n;check(api->GetDimensionsCount(ti,&n));std::vector<int64_t> sh(n);check(api->GetDimensions(ti,sh.data(),n));return sh;}
void require_shape(const Meta& meta,const std::vector<int64_t>& sh){if(sh.size()!=meta.shape.size())throw std::runtime_error("tensor rank mismatch");for(size_t k=0;k<sh.size();++k)if(meta.shape[k]>0&&sh[k]!=meta.shape[k])throw std::runtime_error("tensor dimension mismatch");}
std::string error_with_memory(const char* message){return std::string(message?message:"unknown error")+" memory_diagnostic="+pw_s1_memory_json();}
void metadata(pw_ort_model* m,bool input){size_t n;check(input?api->SessionGetInputCount(m->session,&n):api->SessionGetOutputCount(m->session,&n));auto& metas=input?m->ins:m->outs;for(size_t i=0;i<n;++i){char* name=nullptr;check(input?api->SessionGetInputName(m->session,i,m->allocator,&name):api->SessionGetOutputName(m->session,i,m->allocator,&name));Meta meta;meta.name=name;m->allocator->Free(m->allocator,name);OrtTypeInfo* t=nullptr;check(input?api->SessionGetInputTypeInfo(m->session,i,&t):api->SessionGetOutputTypeInfo(m->session,i,&t));try{const OrtTensorTypeAndShapeInfo* ti=nullptr;check(api->CastTypeInfoToTensorInfo(t,&ti));if(!ti)throw std::runtime_error("non-tensor metadata");meta.shape=shapeinfo(const_cast<OrtTensorTypeAndShapeInfo*>(ti),&meta.dtype);api->ReleaseTypeInfo(t);}catch(...){api->ReleaseTypeInfo(t);throw;}metas.push_back(std::move(meta));}}
int getshape(const std::vector<Meta>& metas,const char* name,int32_t* dst,int cap){int i=indexof(metas,name);if(i<0)return -1;auto& s=metas[i].shape;if(!dst||cap<int(s.size()))return -1;for(size_t k=0;k<s.size();++k){if(s[k]<INT32_MIN||s[k]>INT32_MAX)return -1;dst[k]=int32_t(s[k]);}return int(s.size());}
std::string env(const char* k){const char* v=std::getenv(k);return v?v:"";}
uint64_t limit_from_env(const char* key,uint64_t fallback,uint64_t maximum=2000000000ULL){const char* v=std::getenv(key);if(!v||!*v)return fallback;for(const char* p=v;*p;++p)if(*p<'0'||*p>'9')throw std::runtime_error("memory budget requires ASCII decimal digits only");errno=0;char* end=nullptr;auto n=std::strtoull(v,&end,10);if(errno||*end||n==0||n>maximum)throw std::runtime_error(std::string(key)+": memory budget must be 1.."+std::to_string(maximum)+" bytes");return n;}

}
extern "C" {
const char* pw_s1_runtime_snapshot_json(){thread_local std::string result;if(!runtime_memory_snapshot){result="{\"status\":\"unavailable\",\"reason\":\"not_configured\"}";return result.c_str();}PwS1MemorySnapshot m{};m.abi=RECONSTRUCTION_S1_MEMORY_ABI;m.struct_size=sizeof(m);int rc=runtime_memory_snapshot(&m,sizeof(m));if(rc!=0){result="{\"status\":\"query_failed\",\"rc\":"+std::to_string(rc)+"}";return result.c_str();}auto value=[&](uint64_t v,uint32_t bit){return (m.valid_bits&bit)?std::to_string(v):std::string("null");};std::ostringstream out;out<<"{\"status\":"<<quote(m.sequence?"latest_allocator_boundary":"no_boundary_recorded")<<",\"abi\":"<<m.abi<<",\"struct_size\":"<<m.struct_size<<",\"valid_bits\":"<<m.valid_bits<<",\"sequence\":"<<m.sequence<<",\"monotonic_ns\":"<<m.monotonic_ns<<",\"event\":"<<m.event<<",\"request_B\":"<<m.request_B<<",\"device_identity\":"<<(m.device_identity?std::to_string(m.device_identity):"null")<<",\"managed_estimated_B\":"<<value(m.managed_estimated_B,1)<<",\"buffer_estimated_B\":"<<value(m.buffer_estimated_B,1)<<",\"texture_estimated_B\":"<<value(m.texture_estimated_B,1)<<",\"metal_device_allocated_B\":"<<value(m.metal_device_allocated_B,2)<<",\"process_phys_footprint_B\":"<<value(m.process_phys_footprint_B,4)<<",\"process_resident_B\":"<<value(m.process_resident_B,8)<<",\"allocation_requests\":"<<m.allocation_requests<<",\"cache_hits\":"<<m.cache_hits<<",\"releases\":"<<m.releases<<",\"refreshes\":"<<m.refreshes<<",\"rejections\":"<<m.rejections<<",\"rejection_domain\":"<<m.rejection_domain<<",\"peak_managed_estimated_B\":"<<value(m.peak_managed_estimated_B,1)<<",\"peak_metal_device_allocated_B\":"<<value(m.peak_metal_device_allocated_B,2)<<",\"peak_process_phys_footprint_B\":"<<value(m.peak_process_phys_footprint_B,4)<<",\"managed_limit_B\":"<<runtime_memory_limits.managed_limit_B<<",\"metal_limit_B\":"<<runtime_memory_limits.metal_allocated_limit_B<<",\"process_limit_B\":"<<runtime_memory_limits.process_footprint_limit_B<<",\"scope\":\"latest allocator boundary, not fresh GPU synchronization; unavailable counters=null; diagnostic budgets not population defaults\"}";result=out.str();return result.c_str();}
int pw_ort_set_runtime(const char* path,char* err,int len){std::lock_guard<std::mutex> guard(init_mu);try{if(!reconstruction_memory_policy::valid())throw std::runtime_error("unknown explicit memory policy");if(!path||path[0]!='/')throw std::runtime_error("absolute Bench libreconstructionQMvsOrt.dylib path required");if(api){if(runtime_path!=path)throw std::runtime_error("runtime already frozen to different path");return 0;}void* handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);if(!handle)throw std::runtime_error(dlerror());using GetBase=const OrtApiBase* (ORT_API_CALL*)();auto basefn=(GetBase)dlsym(handle,"OrtGetApiBase");using Register=int(*)() noexcept;auto reg=(Register)dlsym(handle,"ReconstructionRegisterExperimentalSchema");if(!basefn||!reg)throw std::runtime_error("missing runtime or P32 schema export");auto base=basefn();std::string ver=base->GetVersionString();if(ver!="1.29.0")throw std::runtime_error("runtime version differs from frozen 1.29.0: "+ver);const OrtApi* candidate=base->GetApi(29);if(!candidate)throw std::runtime_error("GetApi(29) failed");auto memory_configure=(MemoryConfigure)dlsym(handle,"reconstruction_s1_memory_configure");auto memory_snapshot=(MemorySnapshot)dlsym(handle,"reconstruction_s1_memory_snapshot");if(!memory_configure||!memory_snapshot)throw std::runtime_error("missing S1 allocator budget/snapshot runtime exports");runtime_memory_limits.managed_limit_B=reconstruction_memory_policy::observe_only()?reconstruction_memory_policy::kObserveLimit:limit_from_env("RECONSTRUCTION_S1_MANAGED_LIMIT_BYTES",2000000000ULL,UINT64_MAX);runtime_memory_limits.metal_allocated_limit_B=reconstruction_memory_policy::observe_only()?reconstruction_memory_policy::kObserveLimit:limit_from_env("RECONSTRUCTION_S1_METAL_LIMIT_BYTES",2000000000ULL,UINT64_MAX);runtime_memory_limits.process_footprint_limit_B=reconstruction_memory_policy::observe_only()?reconstruction_memory_policy::kObserveLimit:limit_from_env("RECONSTRUCTION_S1_FOOTPRINT_CEILING_BYTES",2000000000ULL);if(memory_configure(&runtime_memory_limits)!=0)throw std::runtime_error("S1 allocator limits configuration rejected before model load");runtime_memory_configure=memory_configure;runtime_memory_snapshot=memory_snapshot;if(reg()!=0)throw std::runtime_error("ReconstructionRegisterExperimentalSchema failed");lib=handle;api=candidate;runtime_path=path;version=ver;return 0;}catch(const std::exception& e){textcopy(e.what(),err,len);return -1;}}
const char* pw_ort_version(){return api?version.c_str():"uninitialized; set exact runtime first";}
pw_ort_model* pw_ort_load(const char* path,int backend,int precision,int threads,char* err,int len){if(failed_model_slot){if(err&&len>0)fixedcopy(err,size_t(len),"failed process forbids model retry");return nullptr;}try{if(!api)throw std::runtime_error("pw_ort_set_runtime required");if(!path||backend!=1||precision!=1||threads!=0)throw std::runtime_error("requires model path, WebGPU=1, float32 precision=1, production default threads=0");auto m=std::make_unique<pw_ort_model>();auto start=std::chrono::steady_clock::now();check(api->GetAllocatorWithDefaultOptions(&m->allocator));bool verbose=env("PWDENSE_ORT_VERBOSE")=="1";{
 std::lock_guard<std::mutex> guard(init_mu);m->context=shared_context.lock();
 if(!m->context){m->context=std::make_shared<RuntimeContext>();check(api->CreateEnvWithCustomLogger(logger,m->context.get(),verbose?ORT_LOGGING_LEVEL_VERBOSE:ORT_LOGGING_LEVEL_WARNING,"pw_ort_s1_probe",&m->context->env));shared_context=m->context;}
 m->env=m->context->env;
}check(api->CreateSessionOptions(&m->options));check(api->SetSessionGraphOptimizationLevel(m->options,ORT_ENABLE_BASIC));
check(api->SetIntraOpNumThreads(m->options,1));check(api->SetInterOpNumThreads(m->options,1));
check(api->AddSessionConfigEntry(m->options,"ep.webgpuexecutionprovider.validationMode","disabled"));
check(api->AddSessionConfigEntry(m->options,"ep.webgpuexecutionprovider.maxNumPendingDispatches","16"));
m->cache_mode=env("RECONSTRUCTION_S1_STORAGE_BUFFER_CACHE_MODE");if(m->cache_mode.empty())m->cache_mode="disabled";
if(m->cache_mode!="disabled"&&m->cache_mode!="bucket"&&m->cache_mode!="simple"&&m->cache_mode!="lazyRelease")throw std::runtime_error("unsupported official storage buffer cache mode");
check(api->AddSessionConfigEntry(m->options,"ep.webgpuexecutionprovider.storageBufferCacheMode",m->cache_mode.c_str()));
check(api->AddSessionConfigEntry(m->options,"ep.webgpuexecutionprovider.enableGraphCapture","0"));
check(api->AddSessionConfigEntry(m->options,"ep.webgpuexecutionprovider.enableInt64","1"));
check(api->AddSessionConfigEntry(m->options,"session.disable_cpu_ep_fallback","1"));if(!env("PWDENSE_ORT_PROFILE").empty())throw std::runtime_error("GPU handoff candidate forbids whole-graph GPU timestamp profiling; use separate registered profiling arm");char** providers=nullptr;int np=0;check(api->GetAvailableProviders(&providers,&np));bool have_webgpu=false;for(int i=0;i<np;++i){if(i)m->available+=",";m->available+=providers[i];if(std::string(providers[i])=="WebGpuExecutionProvider"||std::string(providers[i])=="WebGPUExecutionProvider")have_webgpu=true;}check(api->ReleaseAvailableProviders(providers,np));if(!have_webgpu)throw std::runtime_error("runtime does not list WebGPU EP");check(api->SessionOptionsAppendExecutionProvider(m->options,"WebGPU",nullptr,nullptr,0));char memory_error[1024];if(pw_s1_memory_preflight("BEFORE_SESSION_CREATE",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);check(api->CreateSession(m->env,path,m->options,&m->session));if(pw_s1_memory_preflight("AFTER_SESSION_CREATE",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);metadata(m.get(),true);metadata(m.get(),false);check(api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&m->mem));m->device=std::make_shared<DeviceSession>();m->device->context=m->context;m->device->session=m->session;m->device->options=m->options;
check(api->CreateMemoryInfo("WebGPU_Buffer",OrtDeviceAllocator,0,OrtMemTypeDefault,&m->device->gpu_mem));
check(api->CreateAllocator(m->session,m->device->gpu_mem,&m->device->gpu_allocator));
m->inputs.resize(m->ins.size());m->values.resize(m->ins.size(),nullptr);m->gpu_inputs.resize(m->ins.size());m->gpu_outputs.resize(m->outs.size());m->outputs.resize(m->outs.size());m->load_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return m.release();}catch(const std::exception& e){textcopy(std::string(e.what())+" runtime_memory="+pw_s1_runtime_snapshot_json(),err,len);return nullptr;}}
void pw_ort_destroy(pw_ort_model* m){if(m&&(m->failed||m->failure.uncertain))return;delete m;}
int pw_ort_num_inputs(const pw_ort_model* m){return m?int(m->ins.size()):-1;}
int pw_ort_num_outputs(const pw_ort_model* m){return m?int(m->outs.size()):-1;}
const char* pw_ort_input_name(const pw_ort_model* m,int i){return m&&i>=0&&i<int(m->ins.size())?m->ins[i].name.c_str():nullptr;}
const char* pw_ort_output_name(const pw_ort_model* m,int i){return m&&i>=0&&i<int(m->outs.size())?m->outs[i].name.c_str():nullptr;}
int pw_ort_input_shape(const pw_ort_model* m,const char* n,int32_t* s,int cap){return m?getshape(m->ins,n,s,cap):-1;}
int pw_ort_set_input(pw_ort_model* m,const char* name,const float* data,const int32_t* shape,int ndim,size_t count){if(!m||failed_model_slot||m->failed)return -1;try{int i=indexof(m->ins,name);if(i<0||!data||!shape||ndim<=0)throw std::runtime_error("invalid input/name");if(m->ins[i].dtype!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)throw std::runtime_error("float input API requires FLOAT metadata; no implicit half conversion");std::vector<int64_t> sh(shape,shape+ndim);if(countof(sh)!=count||sh.size()!=m->ins[i].shape.size())throw std::runtime_error("input count/rank mismatch");for(size_t k=0;k<sh.size();++k)if(m->ins[i].shape[k]>0&&sh[k]!=m->ins[i].shape[k])throw std::runtime_error("input dimension mismatch");if(m->values[i]&&!m->gpu_inputs[i])api->ReleaseValue(m->values[i]);m->values[i]=nullptr;m->gpu_inputs[i].reset();char memory_error[1024];if(count>SIZE_MAX/sizeof(float))throw std::runtime_error("input byte size overflow");size_t extra=m->inputs[i].capacity()<count?count*sizeof(float):0;if(pw_s1_memory_preflight("BEFORE_HOST_INPUT_COPY",extra,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);m->inputs[i].assign(data,data+count);if(pw_s1_memory_preflight("AFTER_HOST_INPUT_COPY",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);check(api->CreateTensorWithDataAsOrtValue(m->mem,m->inputs[i].data(),count*sizeof(float),sh.data(),sh.size(),ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&m->values[i]));m->error.clear();return 0;}catch(const std::exception& e){m->error=error_with_memory(e.what());return -1;}}
int pw_ort_run(pw_ort_model* m,double* ms){if(!m||failed_model_slot||m->failed)return -1;std::vector<OrtValue*> results(m->outs.size(),nullptr);OrtRunOptions* run_options=nullptr;try{for(const auto& output:m->outs)if(output.dtype!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)throw std::runtime_error("legacy host run requires FLOAT outputs; use run_gpu for FLOAT16 features");char memory_error[1024];if(pw_s1_memory_preflight("BEFORE_RUN",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);check(api->CreateRunOptions(&run_options));pw_s1_memory_active_run(run_options,[](void* p)->int{OrtStatus* status=api->RunOptionsSetTerminate((OrtRunOptions*)p);if(!status)return 0;api->ReleaseStatus(status);return -1;});for(auto* v:m->values)if(!v)throw std::runtime_error("missing input");for(auto& out:m->outputs)out.clear();for(auto& out:m->gpu_outputs)out.reset();std::vector<const char*> in,out;for(auto& a:m->ins)in.push_back(a.name.c_str());for(auto& a:m->outs)out.push_back(a.name.c_str());size_t prior_errors=0;{std::lock_guard<std::mutex> g(m->context->log_mu);prior_errors=m->context->errors;}auto start=std::chrono::steady_clock::now();check(api->Run(m->session,run_options,in.data(),m->values.data(),in.size(),out.data(),out.size(),results.data()));pw_s1_memory_clear_run();api->ReleaseRunOptions(run_options);run_options=nullptr;if(pw_s1_memory_preflight("AFTER_RUN",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);{std::lock_guard<std::mutex> g(m->context->log_mu);if(m->context->errors!=prior_errors)throw std::runtime_error("ORT runtime reported error during Run; outputs rejected");}for(size_t i=0;i<results.size();++i){const OrtMemoryInfo* mi=nullptr;check(api->GetTensorMemoryInfo(results[i],&mi));OrtMemoryInfoDeviceType device;api->MemoryInfoGetDeviceType(mi,&device);if(device!=OrtMemoryInfoDeviceType_CPU)throw std::runtime_error("output not host accessible; refusing packed/device read");OrtTensorTypeAndShapeInfo* ti=nullptr;check(api->GetTensorTypeAndShape(results[i],&ti));std::vector<int64_t> sh;try{ONNXTensorElementDataType actual;sh=shapeinfo(ti,&actual);if(actual!=m->outs[i].dtype||actual!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)throw std::runtime_error("legacy output dtype mismatch");require_shape(m->outs[i],sh);(void)tensor_bytes(sh,actual);api->ReleaseTensorTypeAndShapeInfo(ti);}catch(...){api->ReleaseTensorTypeAndShapeInfo(ti);throw;}size_t n=countof(sh);float* data=nullptr;check(api->GetTensorMutableData(results[i],(void**)&data));m->outputs[i].assign(data,data+n);for(float v:m->outputs[i])if(!std::isfinite(v))throw std::runtime_error("nonfinite ORT output rejected");m->outs[i].shape=std::move(sh);}if(pw_s1_memory_preflight("AFTER_HOST_OUTPUT_COPY",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);if(indexof(m->outs,"depth")>=0){bool any_nonzero=false;for(const auto& values:m->outputs)for(float v:values)any_nonzero=any_nonzero||(v!=0.0f);if(!any_nonzero)throw std::runtime_error("all-zero MVS outputs rejected as invalid execution");}for(auto* v:results)api->ReleaseValue(v);m->run_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(ms)*ms=m->run_ms;++m->runs;m->error.clear();return 0;}catch(const std::exception& e){pw_s1_memory_clear_run();if(run_options)api->ReleaseRunOptions(run_options);for(auto* v:results)if(v)api->ReleaseValue(v);for(auto& v:m->outputs)v.clear();for(auto& v:m->gpu_outputs)v.reset();m->error=error_with_memory(e.what());return -1;}}
int pw_ort_output_shape(const pw_ort_model* m,const char* n,int32_t* s,int cap){return m?getshape(m->outs,n,s,cap):-1;}
int pw_ort_get_output(const pw_ort_model* model,const char* n,float* dst,size_t count){if(!model||!dst||failed_model_slot||model->failed)return -1;auto* m=const_cast<pw_ort_model*>(model);OrtValue* host=nullptr;try{int i=indexof(m->outs,n);if(i<0||countof(m->outs[i].shape)!=count)throw std::runtime_error("output count/name mismatch");if(m->outs[i].dtype!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)throw std::runtime_error("float output API requires FLOAT metadata; no implicit half conversion");(void)tensor_bytes(m->outs[i].shape,m->outs[i].dtype);if(m->gpu_outputs[i]){char memory_error[1024];if(pw_s1_memory_preflight("BEFORE_EXPLICIT_GPU_DOWNLOAD",0,memory_error,sizeof(memory_error))!=0)throw std::runtime_error(memory_error);check(api->CreateTensorWithDataAsOrtValue(m->mem,dst,count*sizeof(float),m->outs[i].shape.data(),m->outs[i].shape.size(),ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&host));const OrtValue* src[]={m->gpu_outputs[i]->value};OrtValue* out[]={host};check(api->CopyTensors(m->env,src,out,nullptr,1));api->ReleaseValue(host);host=nullptr;++m->explicit_downloads;for(size_t k=0;k<count;++k)if(!std::isfinite(dst[k]))throw std::runtime_error("nonfinite GPU output rejected");}else{if(m->outputs[i].size()!=count||!count)throw std::runtime_error("output unavailable");std::memcpy(dst,m->outputs[i].data(),count*sizeof(float));}m->error.clear();return 0;}catch(const std::exception&e){if(host)api->ReleaseValue(host);m->error=error_with_memory(e.what());return -1;}}
int pw_ort_end_profile(pw_ort_model* m,char* buf,int len){if(!m)return -1;try{if(m->profile_prefix.empty())throw std::runtime_error("profiling not enabled");if(!m->profile_ended){char* path=nullptr;check(api->SessionEndProfiling(m->session,m->allocator,&path));m->profile=path;m->allocator->Free(m->allocator,path);m->profile_ended=true;}textcopy(m->profile,buf,len);return len>int(m->profile.size())?0:-1;}catch(const std::exception& e){m->error=error_with_memory(e.what());return -1;}}
int pw_ort_info(const pw_ort_model* m,char* buf,int len){if(!m)return -1;std::string log_snapshot;size_t errors=0;{std::lock_guard<std::mutex> g(m->context->log_mu);log_snapshot=m->context->logs;errors=m->context->errors;}std::ostringstream o;o<<"{\"ort_version\":"<<quote(version)<<",\"api_version\":29,\"runtime_path\":"<<quote(runtime_path)<<",\"requested_provider\":\"WebGPU\",\"effective_node_assignment\":\"CPU fallback disabled by session option; Metal execution requires phone first-gold\",\"cpu_fallback_possible\":false,\"enable_int64\":true,\"graph_capture\":false,\"storage_buffer_cache_mode\":"<<quote(m->cache_mode)<<",\"memory_budget_scope\":\"explicit production observation or Bench2GB admission; not an absolute CPU peak guarantee; managed and Metal are independent diagnostic counters\",\"physical_admission_policy_B\":"<<runtime_memory_limits.process_footprint_limit_B<<",\"effective_process_limit_B\":"<<runtime_memory_limits.process_footprint_limit_B<<",\"effective_managed_diagnostic_limit_B\":"<<runtime_memory_limits.managed_limit_B<<",\"effective_metal_diagnostic_limit_B\":"<<runtime_memory_limits.metal_allocated_limit_B<<",\"gpu_completion_policy\":\"per GPU-bound run: official ordered CopyTensors sentinel: F32=4B/F16=2B CPU payload,16B GPU staging copy per wait; waits MapAsync/WaitAny; not zero downloads\",\"completion_waits\":"<<m->completion_waits<<",\"completion_download_bytes\":"<<m->completion_download_bytes<<",\"completion_staging_copy_bytes\":"<<(m->completion_waits*16)<<",\"completion_staging_normalization_B\":16"<<",\"last_enqueue_host_ms\":"<<m->last_enqueue_host_ms<<",\"last_completion_wait_host_ms\":"<<m->last_completion_wait_host_ms<<",\"gpu_timing_scope\":\"host wall measurements only; not GPU kernel durations\",\"gpu_handoff_runs\":"<<m->gpu_runs<<",\"gpu_binds\":"<<m->gpu_binds<<",\"gpu_uploads\":"<<m->gpu_uploads<<",\"explicit_gpu_downloads\":"<<m->explicit_downloads<<",\"gpu_output_validation\":\"shape/device; finite validation on explicit download\",\"shared_env\":true,\"intra_op_threads\":1,\"inter_op_threads\":1,\"native_backend\":\"Metal expected on iOS through shipped Dawn EP; not yet observed\",\"available_providers\":"<<quote(m->available)<<",\"graph_optimization\":\"ORT_ENABLE_BASIC\",\"tensor_layout\":\"dense typed ONNX order; external float APIs remain F32-only\",\"load_ms\":"<<m->load_ms<<",\"last_run_host_outputs_ms\":"<<m->run_ms<<",\"runs\":"<<m->runs<<",\"profile_prefix\":"<<quote(m->profile_prefix)<<",\"profile_path\":"<<quote(m->profile)<<",\"logs\":"<<quote(log_snapshot)<<",\"memory_diagnostic\":"<<pw_s1_memory_json()<<"}";auto s=o.str();textcopy(s,buf,len);return len>int(s.size())?0:-1;}
const char* pw_ort_last_error(const pw_ort_model* m){return m?(m->failed?m->failure_error:m->error.c_str()):"null model";}
}

namespace {
std::vector<int64_t> tensor_shape(OrtValue* value,ONNXTensorElementDataType* dtype=nullptr){OrtTensorTypeAndShapeInfo* ti=nullptr;check(api->GetTensorTypeAndShape(value,&ti));try{auto sh=shapeinfo(ti,dtype);api->ReleaseTensorTypeAndShapeInfo(ti);return sh;}catch(...){api->ReleaseTensorTypeAndShapeInfo(ti);throw;}}
void require_gpu(OrtValue* value){const OrtMemoryInfo* mi=nullptr;check(api->GetTensorMemoryInfo(value,&mi));OrtMemoryInfoDeviceType type;api->MemoryInfoGetDeviceType(mi,&type);int id=-1;check(api->MemoryInfoGetId(mi,&id));if(type!=OrtMemoryInfoDeviceType_GPU||id!=0)throw std::runtime_error("official WebGPU float32/float16 device tensor required");(void)tensor_shape(value);}
void validate_input(pw_ort_model* m,int i,const std::vector<int64_t>& sh,size_t count){if(i<0||countof(sh)!=count||sh.size()!=m->ins[i].shape.size())throw std::runtime_error("GPU input name/count/rank mismatch");for(size_t k=0;k<sh.size();++k)if(m->ins[i].shape[k]>0&&sh[k]!=m->ins[i].shape[k])throw std::runtime_error("GPU input dimension mismatch");}
// Official opaque-buffer alias: borrow retained first output, expose one element with actual dtype.
// Logical CPU payload F32=4B/F16=2B; pinned Download normalizes staging copy to16B and waits MapAsync.
// No pointer identity heuristic and no custom allocator; output owner stays live throughout.
void wait_gpu_completion(pw_ort_model* m,OrtValue* output){
 require_gpu(output);ONNXTensorElementDataType dtype;auto output_shape=tensor_shape(output,&dtype);(void)tensor_bytes(output_shape,dtype);size_t payload_bytes=element_bytes(dtype);void* opaque_buffer=nullptr;check(api->GetTensorMutableData(output,&opaque_buffer));if(!opaque_buffer)throw std::runtime_error("completion sentinel requires live GPU output buffer");
 char err[1024];if(pw_s1_memory_preflight("BEFORE_GPU_COMPLETION_SENTINEL",payload_bytes,err,sizeof(err))!=0)throw std::runtime_error(err);
 auto& f=m->failure;f.sentinel_storage=0;const int64_t one[]={1};auto begin=std::chrono::steady_clock::now();
 check(api->CreateTensorWithDataAsOrtValue(m->device->gpu_mem,opaque_buffer,payload_bytes,one,1,dtype,&f.gpu_alias));
 check(api->CreateTensorWithDataAsOrtValue(m->mem,&f.sentinel_storage,payload_bytes,one,1,dtype,&f.host_alias));
 const OrtValue* src[]={f.gpu_alias};OrtValue* dst[]={f.host_alias};
 f.uncertain=true;check(api->CopyTensors(m->env,src,dst,nullptr,1));
 f.uncertain=false;
 api->ReleaseValue(f.host_alias);f.host_alias=nullptr;api->ReleaseValue(f.gpu_alias);f.gpu_alias=nullptr;
 m->last_completion_wait_host_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();++m->completion_waits;m->completion_download_bytes+=payload_bytes;
}

void assign_gpu_input(pw_ort_model* m,int i,std::shared_ptr<DeviceTensor> value){require_gpu(value->value);ONNXTensorElementDataType actual;auto sh=tensor_shape(value->value,&actual);validate_input(m,i,sh,countof(sh));if(actual!=m->ins[i].dtype)throw std::runtime_error("GPU handoff dtype mismatch; no implicit conversion");(void)tensor_bytes(sh,actual);if(m->values[i]&&!m->gpu_inputs[i])api->ReleaseValue(m->values[i]);m->gpu_inputs[i]=std::move(value);m->values[i]=m->gpu_inputs[i]->value;std::vector<float>().swap(m->inputs[i]);}
}
extern "C" {
int pw_ort_set_input_gpu(pw_ort_model* m,const char* name,const float* data,const int32_t* shape,int ndim,size_t count){
 if(!m||failed_model_slot||m->failed)return -1;auto& f=m->failure;
 try{
  if(!data||!shape||ndim<=0)throw std::runtime_error("invalid GPU upload arguments");
  int i=indexof(m->ins,name);std::vector<int64_t> sh(shape,shape+ndim);validate_input(m,i,sh,count);
  if(m->ins[i].dtype!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)throw std::runtime_error("float GPU upload requires FLOAT metadata; no implicit half conversion");
  if(count>SIZE_MAX/sizeof(float))throw std::runtime_error("GPU upload byte overflow");
  char err[1024];if(pw_s1_memory_preflight("BEFORE_EXPLICIT_GPU_UPLOAD",count*sizeof(float),err,sizeof(err))!=0)throw std::runtime_error(err);
  f.upload=std::make_shared<DeviceTensor>();f.upload->owner=m->device;
  check(api->CreateTensorAsOrtValue(m->device->gpu_allocator,sh.data(),sh.size(),ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&f.upload->value));
  check(api->CreateTensorWithDataAsOrtValue(m->mem,const_cast<float*>(data),count*sizeof(float),sh.data(),sh.size(),ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&f.host_alias));
  const OrtValue* src[]={f.host_alias};OrtValue* dst[]={f.upload->value};
  f.uncertain=true;check(api->CopyTensors(m->env,src,dst,nullptr,1));f.uncertain=false;
  api->ReleaseValue(f.host_alias);f.host_alias=nullptr;assign_gpu_input(m,i,std::move(f.upload));
  if(pw_s1_memory_preflight("AFTER_EXPLICIT_GPU_UPLOAD",0,err,sizeof(err))!=0)throw std::runtime_error(err);
  ++m->gpu_uploads;m->error.clear();return 0;
 }catch(const std::exception&e){fail_model(m,e.what());}catch(...){fail_model(m,"unknown GPU upload exception");}
 if(!f.uncertain){if(f.host_alias){api->ReleaseValue(f.host_alias);f.host_alias=nullptr;}f.upload.reset();}
 return -1;
}
int pw_ort_bind_feature_output(pw_ort_model* source,pw_ort_model* target,const char* source_name,const char* target_name){if(!source||!target||failed_model_slot||source->failed||target->failed)return -1;try{if(source==target||source->context!=target->context)throw std::runtime_error("handoff requires distinct sessions sharing frozen Env/Dawn context");int s=indexof(source->outs,source_name),t=indexof(target->ins,target_name);if(s<0||t<0||!source->gpu_outputs[s])throw std::runtime_error("GPU output unavailable; explicit pw_ort_run_gpu required");assign_gpu_input(target,t,source->gpu_outputs[s]);++target->gpu_binds;target->error.clear();return 0;}catch(const std::exception&e){target->error=error_with_memory(e.what());return -1;}}
int pw_ort_bind_gpu_input(pw_ort_model* source,pw_ort_model* target,const char* source_name,const char* target_name){if(!source||!target||failed_model_slot||source->failed||target->failed)return -1;try{if(source==target||source->context!=target->context)throw std::runtime_error("RGB handoff requires distinct sessions sharing frozen Env/Dawn context");int s=indexof(source->ins,source_name),t=indexof(target->ins,target_name);if(s<0||t<0||!source->gpu_inputs[s])throw std::runtime_error("GPU input unavailable; explicit pw_ort_set_input_gpu required");assign_gpu_input(target,t,source->gpu_inputs[s]);++target->gpu_binds;target->error.clear();return 0;}catch(const std::exception&e){target->error=error_with_memory(e.what());return -1;}}
int pw_ort_clear_input(pw_ort_model* m,const char* name){if(!m||failed_model_slot||m->failed)return -1;int i=indexof(m->ins,name);if(i<0){m->error="unknown input";return -1;}if(m->values[i]&&!m->gpu_inputs[i])api->ReleaseValue(m->values[i]);m->values[i]=nullptr;m->gpu_inputs[i].reset();std::vector<float>().swap(m->inputs[i]);m->error.clear();return 0;}
int pw_ort_run_gpu(pw_ort_model* m,double* ms){
 if(!m||failed_model_slot||m->failed)return -1;auto& f=m->failure;
 try{
  char err[1024];if(pw_s1_memory_preflight("BEFORE_GPU_BOUND_RUN",0,err,sizeof(err))!=0)throw std::runtime_error(err);
  for(auto* value:m->values)if(!value)throw std::runtime_error("missing GPU-bound run input");
  for(auto& output:m->outputs)std::vector<float>().swap(output);
  for(auto& output:m->gpu_outputs)output.reset();
  check(api->CreateIoBinding(m->session,&f.binding));
  for(size_t i=0;i<m->ins.size();++i)check(api->BindInput(f.binding,m->ins[i].name.c_str(),m->values[i]));
  for(auto& out:m->outs)check(api->BindOutputToDevice(f.binding,out.name.c_str(),m->device->gpu_mem));
  check(api->CreateRunOptions(&f.options));
  pw_s1_memory_active_run(f.options,[](void* p)->int{OrtStatus* status=api->RunOptionsSetTerminate((OrtRunOptions*)p);if(!status)return 0;api->ReleaseStatus(status);return -1;});
  size_t prior_errors;{std::lock_guard<std::mutex> g(m->context->log_mu);prior_errors=m->context->errors;}
  auto start=std::chrono::steady_clock::now();
  f.uncertain=true;check(api->SynchronizeBoundInputs(f.binding));
  check(api->RunWithBinding(m->session,f.options,f.binding));check(api->SynchronizeBoundOutputs(f.binding));
  {std::lock_guard<std::mutex> g(m->context->log_mu);if(m->context->errors!=prior_errors)throw std::runtime_error("ORT runtime reported error during GPU run; outputs rejected");}
  check(api->GetBoundOutputValues(f.binding,m->allocator,&f.results,&f.result_count));
  if(f.result_count!=m->outs.size())throw std::runtime_error("GPU bound output count mismatch");
  for(size_t i=0;i<f.result_count;++i){
   require_gpu(f.results[i]);ONNXTensorElementDataType actual;auto sh=tensor_shape(f.results[i],&actual);
   if(actual!=m->outs[i].dtype)throw std::runtime_error("GPU output actual dtype differs from session metadata");
   require_shape(m->outs[i],sh);(void)tensor_bytes(sh,actual);
   auto value=std::make_shared<DeviceTensor>();value->owner=m->device;value->value=f.results[i];f.results[i]=nullptr;
   m->gpu_outputs[i]=std::move(value);m->outs[i].shape=std::move(sh);
  }
  if(m->gpu_outputs.empty()||!m->gpu_outputs[0])throw std::runtime_error("no GPU output for completion sentinel");
  m->last_enqueue_host_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  wait_gpu_completion(m,m->gpu_outputs[0]->value);
  // Only the successful original CopyTensors completion sentinel permits cleanup.
  pw_s1_memory_clear_run();release_run_resources(m);
  {std::lock_guard<std::mutex> g(m->context->log_mu);if(m->context->errors!=prior_errors)throw std::runtime_error("ORT runtime reported error during GPU completion wait; outputs rejected");}
  if(pw_s1_memory_preflight("AFTER_GPU_BOUND_RUN",0,err,sizeof(err))!=0)throw std::runtime_error(err);
  m->run_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(ms)*ms=m->run_ms;++m->runs;++m->gpu_runs;m->error.clear();return 0;
 }catch(const std::exception&e){fail_model(m,e.what());}catch(...){fail_model(m,"unknown GPU run exception");}
 if(!f.uncertain){pw_s1_memory_clear_run();release_run_resources(m);for(auto& output:m->gpu_outputs)output.reset();}
 return -1;
}
}

extern "C" {
int pw_ort_input_dtype(const pw_ort_model* m,const char* name){if(!m)return -1;int i=indexof(m->ins,name);return i<0?-1:int(m->ins[i].dtype);}
int pw_ort_output_dtype(const pw_ort_model* m,const char* name){if(!m)return -1;int i=indexof(m->outs,name);return i<0?-1:int(m->outs[i].dtype);}
int pw_ort_tensor_bytes(const pw_ort_model* m,int input,const char* name,size_t* bytes){if(!m||!bytes||(input!=0&&input!=1))return -1;try{const auto& metas=input?m->ins:m->outs;int i=indexof(metas,name);if(i<0)return -1;*bytes=tensor_bytes(metas[i].shape,metas[i].dtype);return 0;}catch(...){return -1;}}
}
