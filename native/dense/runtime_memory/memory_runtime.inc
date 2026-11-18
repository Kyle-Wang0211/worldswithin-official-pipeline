#include "reconstruction_s1_memory.h"
#include "memory_budget.h"
#include "dawn/native/DawnNative.h"
#include <mutex>
#include <chrono>
#include <algorithm>
extern "C" unsigned reconstruction_s1_platform_memory(WGPUDevice,uint64_t*,uint64_t*,uint64_t*);
namespace {
std::recursive_mutex s1_memory_mutex;
PwS1MemoryLimits s1_limits{};
PwS1MemorySnapshot s1_last{};
                                                                                       
void S1MemoryEvent(WGPUDevice device,uint64_t request,uint32_t event) {
 std::lock_guard<std::recursive_mutex> lock(s1_memory_mutex);
 auto objects=dawn::native::ComputeEstimatedMemoryUsageInfo(device);
 auto& x=s1_last;x.abi=1;x.struct_size=sizeof(x);x.event=event;++x.sequence;
 x.monotonic_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
 x.device_identity=reinterpret_cast<uintptr_t>(device);x.request_B=request;x.valid_bits=1;
 x.managed_estimated_B=objects.totalUsage;x.buffer_estimated_B=objects.buffersUsage;x.texture_estimated_B=objects.texturesUsage;
 x.metal_device_allocated_B=0;x.process_phys_footprint_B=0;x.process_resident_B=0;
 x.valid_bits|=reconstruction_s1_platform_memory(device,&x.metal_device_allocated_B,&x.process_phys_footprint_B,&x.process_resident_B);
 x.peak_managed_estimated_B=std::max(x.peak_managed_estimated_B,x.managed_estimated_B);
 x.peak_metal_device_allocated_B=std::max(x.peak_metal_device_allocated_B,x.metal_device_allocated_B);
 x.peak_process_phys_footprint_B=std::max(x.peak_process_phys_footprint_B,x.process_phys_footprint_B);
 if(event==3)++x.cache_hits;if(event==4)++x.releases;if(event==5)++x.refreshes;
 if(event!=1)return;
 ++x.allocation_requests;x.rejection_domain=0;
 if(s1_limits.abi!=1)x.rejection_domain=1;
 else if((x.valid_bits&7)!=7)x.rejection_domain=2;
 else if(reconstructions1::would_exceed(x.managed_estimated_B,request,s1_limits.managed_limit_B))x.rejection_domain=3;
 else if(reconstructions1::would_exceed(x.metal_device_allocated_B,request,s1_limits.metal_allocated_limit_B))x.rejection_domain=4;
 else if(reconstructions1::would_exceed(x.process_phys_footprint_B,request,s1_limits.process_footprint_limit_B))x.rejection_domain=5;
 if(x.rejection_domain){++x.rejections;ORT_THROW("RECONSTRUCTION_S1_MEMORY_BUDGET domain=",x.rejection_domain," request_B=",request," managed_estimated_B=",x.managed_estimated_B," metal_device_allocated_B=",x.metal_device_allocated_B," process_phys_footprint_B=",x.process_phys_footprint_B);}
}
wgpu::Buffer S1BudgetCreate(const wgpu::Device& device,const wgpu::BufferDescriptor& desc) {
                                                                                         
 std::lock_guard<std::recursive_mutex> lock(s1_memory_mutex);
 S1MemoryEvent(device.Get(),desc.size,1);
 auto buffer=device.CreateBuffer(&desc);
 S1MemoryEvent(device.Get(),0,2);
 return buffer;
}
}
extern "C" __attribute__((visibility("default"))) int reconstruction_s1_memory_configure(const PwS1MemoryLimits*l){std::lock_guard<std::recursive_mutex>lock(s1_memory_mutex);if(!l||l->abi!=1||l->struct_size!=sizeof(*l)||!l->managed_limit_B||!l->metal_allocated_limit_B||!l->process_footprint_limit_B)return 1;if(s1_last.allocation_requests){return s1_limits.abi==l->abi&&s1_limits.struct_size==l->struct_size&&s1_limits.managed_limit_B==l->managed_limit_B&&s1_limits.metal_allocated_limit_B==l->metal_allocated_limit_B&&s1_limits.process_footprint_limit_B==l->process_footprint_limit_B?0:1;}s1_limits=*l;return 0;}
extern "C" __attribute__((visibility("default"))) int reconstruction_s1_memory_snapshot(PwS1MemorySnapshot*out,size_t bytes){std::lock_guard<std::recursive_mutex>lock(s1_memory_mutex);if(!out||bytes!=sizeof(*out))return 1;*out=s1_last;out->abi=1;out->struct_size=sizeof(*out);return 0;}
