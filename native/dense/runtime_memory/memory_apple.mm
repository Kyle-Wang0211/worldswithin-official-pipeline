#include <mach/mach.h>
#import <Metal/Metal.h>
#include "dawn/native/MetalBackend.h"
extern "C" unsigned reconstruction_s1_platform_memory(WGPUDevice device,uint64_t* metal,uint64_t* footprint,uint64_t* resident) {
 unsigned valid=0;
 if(device){id<MTLDevice>d=dawn::native::metal::GetMTLDevice(device);if(d){*metal=[d currentAllocatedSize];valid|=2;}}
 task_vm_info_data_t vm{};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
 if(task_info(mach_task_self(),TASK_VM_INFO,reinterpret_cast<task_info_t>(&vm),&count)==KERN_SUCCESS){*footprint=vm.phys_footprint;*resident=vm.resident_size;valid|=12;}
 return valid;
}
