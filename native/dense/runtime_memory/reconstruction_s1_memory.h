#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RECONSTRUCTION_S1_MEMORY_ABI 1u
                                                                             
                                                                                    
                                                                       
typedef struct {
 uint32_t abi,struct_size;
 uint64_t managed_limit_B,metal_allocated_limit_B,process_footprint_limit_B;
} PwS1MemoryLimits;
typedef struct {
 uint32_t abi,struct_size,valid_bits,event;
 uint64_t sequence,monotonic_ns,device_identity,request_B;
 uint64_t managed_estimated_B,buffer_estimated_B,texture_estimated_B;
 uint64_t metal_device_allocated_B,process_phys_footprint_B,process_resident_B;
 uint64_t allocation_requests,cache_hits,releases,refreshes,rejections;
 uint64_t peak_managed_estimated_B,peak_metal_device_allocated_B,peak_process_phys_footprint_B;
 uint32_t rejection_domain,reserved;
} PwS1MemorySnapshot;
                                                                                        
int reconstruction_s1_memory_configure(const PwS1MemoryLimits* limits);
                                                                                       
int reconstruction_s1_memory_snapshot(PwS1MemorySnapshot* out,size_t bytes);
#ifdef __cplusplus
}
#endif
