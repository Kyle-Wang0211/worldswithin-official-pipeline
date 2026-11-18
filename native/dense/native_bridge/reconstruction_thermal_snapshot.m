#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#include <time.h>
#include <mach/mach.h>
#if TARGET_OS_IPHONE
#include <os/proc.h>
#endif
#include "reconstruction_thermal_snapshot.h"

int reconstruction_thermal_snapshot_v1(int* state,double* monotonic_seconds,
    double* epoch_seconds,unsigned long long* memory_bytes) {
  if(!state||!monotonic_seconds||!epoch_seconds||!memory_bytes)return 22;
  // Unknown/failure remains explicit. Never convert it to a nominal success.
  *state=-1; *monotonic_seconds=0; *epoch_seconds=0; *memory_bytes=0;
  @autoreleasepool {
    NSProcessInfoThermalState thermal=NSProcessInfo.processInfo.thermalState;
    if(thermal<NSProcessInfoThermalStateNominal||thermal>NSProcessInfoThermalStateCritical)return 5;
    struct timespec monotonic,epoch;
    if(clock_gettime(CLOCK_MONOTONIC,&monotonic)||clock_gettime(CLOCK_REALTIME,&epoch))return 5;
    unsigned long long available=0;
#if TARGET_OS_IPHONE
    available=os_proc_available_memory();
#elif TARGET_OS_OSX
    vm_statistics64_data_t vm={0};mach_msg_type_number_t count=HOST_VM_INFO64_COUNT;
    mach_port_t host=mach_host_self();
    kern_return_t rc=host_statistics64(host,HOST_VM_INFO64,(host_info64_t)&vm,&count);
    mach_port_deallocate(mach_task_self(),host);
    if(rc!=KERN_SUCCESS)return 5;
    available=(unsigned long long)vm.free_count*vm_page_size;
#else
    return 5;
#endif
    *state=(int)thermal;
    *monotonic_seconds=(double)monotonic.tv_sec+(double)monotonic.tv_nsec/1000000000.0;
    *epoch_seconds=(double)epoch.tv_sec+(double)epoch.tv_nsec/1000000000.0;
    *memory_bytes=available;
    return 0;
  }
}
