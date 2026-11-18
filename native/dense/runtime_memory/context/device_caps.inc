#include "c_upload_capability.inc"
#include <algorithm>
#include "reconstruction_s1_device_caps.h"
#include <mutex>
#include <sstream>
#include <cstring>
#include <iomanip>
namespace {
std::mutex pw_caps_mutex;
std::string pw_last_caps;
uint64_t pw_caps_sequence=0;
std::string PwCapsQuote(std::string_view x){std::ostringstream s;s<<'"';for(unsigned char c:x){if(c=='"'||c=='\\')s<<'\\'<<char(c);else if(c<32)s<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<unsigned(c)<<std::dec;else s<<char(c);}s<<'"';return s.str();}
std::string PwRecordCaps(int32_t context_id,WGPUDevice device,const wgpu::AdapterInfo& a,
 bool f16,bool subgroups,bool matrix,bool inside,bool timestamp,uint32_t query,bool profiling,const wgpu::AdapterPropertiesSubgroupMatrixConfigs& matrices){
 std::lock_guard<std::mutex>lock(pw_caps_mutex);std::ostringstream o;
 o<<"{\"event\":\"RECONSTRUCTION_DEVICE_CAPS\",\"abi\":1,\"sequence\":"<<++pw_caps_sequence
  <<",\"context_id\":"<<context_id<<",\"device_handle\":\""<<device<<"\",\"vendor_id\":"<<a.vendorID<<",\"device_id\":"<<a.deviceID
  <<",\"vendor\":"<<PwCapsQuote(static_cast<std::string_view>(a.vendor))<<",\"architecture\":"<<PwCapsQuote(static_cast<std::string_view>(a.architecture))
  <<",\"device\":"<<PwCapsQuote(static_cast<std::string_view>(a.device))<<",\"description\":"<<PwCapsQuote(static_cast<std::string_view>(a.description))
  <<",\"backend_type\":"<<uint32_t(a.backendType)<<",\"adapter_type\":"<<uint32_t(a.adapterType)
  <<",\"subgroup_min_size\":"<<a.subgroupMinSize<<",\"subgroup_max_size\":"<<a.subgroupMaxSize
  <<",\"ShaderF16\":"<<f16<<",\"Subgroups\":"<<subgroups<<",\"ChromiumExperimentalSubgroupMatrix\":"<<matrix
  <<",\"ChromiumExperimentalTimestampQueryInsidePasses\":"<<inside<<",\"TimestampQuery\":"<<timestamp
  <<",\"selected_query_type\":\""<<(query==1?"InsidePasses":query==2?"AtPasses":"None")<<"\",\"selected_query_type_id\":"<<query
  <<",\"profiling_enabled_at_init\":"<<profiling<<",\"source\":\"actual_enabled_device_features_after_initialization\"}";
 auto prefix=o.str();prefix.pop_back();o.str("");o.clear();o<<prefix;
 const size_t serialized=std::min<size_t>(matrices.configCount,64);
 o<<",\"matrix_config_count_reported\":"<<matrices.configCount<<",\"matrix_configs_truncated\":"<<(serialized!=matrices.configCount)<<",\"matrix_configs\":[";
 for(size_t i=0;i<serialized;i++){
  const auto&c=matrices.configs[i];if(i)o<<',';
  auto type_name=[](wgpu::SubgroupMatrixComponentType t){switch(t){case wgpu::SubgroupMatrixComponentType::F32:return "f32";case wgpu::SubgroupMatrixComponentType::F16:return "f16";case wgpu::SubgroupMatrixComponentType::U32:return "u32";case wgpu::SubgroupMatrixComponentType::I32:return "i32";default:return "other";}};
  o<<"{\"componentType\":"<<uint32_t(c.componentType)<<",\"component_type\":"<<PwCapsQuote(type_name(c.componentType))<<",\"resultComponentType\":"<<uint32_t(c.resultComponentType)<<",\"result_component_type\":"<<PwCapsQuote(type_name(c.resultComponentType))<<",\"M\":"<<c.M<<",\"N\":"<<c.N<<",\"K\":"<<c.K<<'}';
 }
 o<<"]"<<PwCUploadCapsFragment(device,uint32_t(a.adapterType))<<"}";
 pw_last_caps=o.str();auto observed=pw_last_caps;observed.pop_back();return observed+",\"bounded_submit\":"+PwBoundedSubmitJson()+",\"a_flush_drain\":"+ReconstructionDrainDrainJson()+"}";
}
}
extern "C" __attribute__((visibility("default"))) int reconstruction_s1_device_caps_json(char*b,size_t cap){try{std::lock_guard<std::mutex>lock(pw_caps_mutex);if(!b||!cap)return 2;b[0]=0;if(pw_last_caps.empty())return 1;auto observed=pw_last_caps;observed.pop_back();observed+=",\"bounded_submit\":"+PwBoundedSubmitJson()+",\"a_flush_drain\":"+ReconstructionDrainDrainJson()+"}";if(observed.size()+1>cap)return 2;std::memcpy(b,observed.c_str(),observed.size()+1);return 0;}catch(...){return 3;}}
