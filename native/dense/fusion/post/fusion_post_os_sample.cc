#include "fusion_post_owner.h"
#include <chrono>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#include <os/proc.h>
#endif
extern "C" int reconstruction_fusion_post_sample_os_v1(PwPostBudgetSnapshotV1*s){
 if(!s||s->struct_bytes!=sizeof(*s)||s->abi_version!=1)return 1;
#if defined(__APPLE__)
 task_vm_info_data_t info{};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
 if(task_info(mach_task_self(),TASK_VM_INFO,reinterpret_cast<task_info_t>(&info),&count)!=KERN_SUCCESS||count<TASK_VM_INFO_REV1_COUNT)return 6;
 auto avail=os_proc_available_memory();if(!avail)return 6;
 *s={sizeof(*s),1,PW_POST_SAMPLE_PHYS|PW_POST_SAMPLE_OS_AVAILABLE,0,info.phys_footprint,avail,0,0,uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())};return 0;
#else
                                                                           
 return 2;
#endif
}
