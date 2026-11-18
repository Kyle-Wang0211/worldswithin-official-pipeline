#pragma once
#include <webgpu/webgpu.h>
#include <cstdlib>
#include <cstring>
#include <string>
                                                                                    
inline std::string PwCUploadCapsFragment(WGPUDevice device,uint32_t adapter_type) {
 const char* optin=std::getenv("RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD");
 if(!optin||std::strcmp(optin,"1"))return {};
 const bool extended=wgpuDeviceHasFeature(device,WGPUFeatureName_BufferMapExtendedUsages);
 const bool integrated=adapter_type==uint32_t(WGPUAdapterType_IntegratedGPU);
 return std::string(",\"BufferMapExtendedUsages\":")+(extended?"1":"0")+
        ",\"C_upload_actual_integrated\":"+(integrated?"1":"0")+
        ",\"C_upload_eligible_first_write\":"+(extended&&integrated?"1":"0")+
        ",\"C_upload_capability_source\":\"actual_wgpuDeviceHasFeature_after_initialization\"";
}
