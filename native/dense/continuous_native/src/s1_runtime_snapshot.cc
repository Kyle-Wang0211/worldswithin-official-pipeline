// Metadata-only reuse of typed_half_v3 runtime allocator snapshot serialization.
#include "reconstruction_s1_memory.h"
#include "../../memory_policy.h"
#include "s1_memory_probe.h"
#include <sstream>
#include <string>
namespace {
auto runtime_memory_snapshot=&reconstruction_s1_memory_snapshot;
std::string quote(const std::string&s){return "\""+s+"\"";}
}
extern "C" {
const char* pw_s1_runtime_snapshot_json(){const auto cap=reconstruction_memory_policy::limit();thread_local std::string result;if(!runtime_memory_snapshot){result="{\"status\":\"unavailable\",\"reason\":\"not_configured\"}";return result.c_str();}PwS1MemorySnapshot m{};m.abi=RECONSTRUCTION_S1_MEMORY_ABI;m.struct_size=sizeof(m);int rc=runtime_memory_snapshot(&m,sizeof(m));if(rc!=0){result="{\"status\":\"query_failed\",\"rc\":"+std::to_string(rc)+"}";return result.c_str();}auto value=[&](uint64_t v,uint32_t bit){return (m.valid_bits&bit)?std::to_string(v):std::string("null");};std::ostringstream out;out<<"{\"status\":"<<quote(m.sequence?"latest_allocator_boundary":"no_boundary_recorded")<<",\"abi\":"<<m.abi<<",\"struct_size\":"<<m.struct_size<<",\"valid_bits\":"<<m.valid_bits<<",\"sequence\":"<<m.sequence<<",\"monotonic_ns\":"<<m.monotonic_ns<<",\"event\":"<<m.event<<",\"request_B\":"<<m.request_B<<",\"device_identity\":"<<(m.device_identity?std::to_string(m.device_identity):"null")<<",\"managed_estimated_B\":"<<value(m.managed_estimated_B,1)<<",\"buffer_estimated_B\":"<<value(m.buffer_estimated_B,1)<<",\"texture_estimated_B\":"<<value(m.texture_estimated_B,1)<<",\"metal_device_allocated_B\":"<<value(m.metal_device_allocated_B,2)<<",\"process_phys_footprint_B\":"<<value(m.process_phys_footprint_B,4)<<",\"process_resident_B\":"<<value(m.process_resident_B,8)<<",\"allocation_requests\":"<<m.allocation_requests<<",\"cache_hits\":"<<m.cache_hits<<",\"releases\":"<<m.releases<<",\"refreshes\":"<<m.refreshes<<",\"rejections\":"<<m.rejections<<",\"rejection_domain\":"<<m.rejection_domain<<",\"peak_managed_estimated_B\":"<<value(m.peak_managed_estimated_B,1)<<",\"peak_metal_device_allocated_B\":"<<value(m.peak_metal_device_allocated_B,2)<<",\"peak_process_phys_footprint_B\":"<<value(m.peak_process_phys_footprint_B,4)<<",\"managed_limit_B\":"<<cap<<",\"metal_limit_B\":"<<cap<<",\"process_limit_B\":"<<cap<<",\"scope\":\"latest allocator boundary, not fresh GPU synchronization; unavailable counters=null; diagnostic budgets not population defaults\"}";result=out.str();return result.c_str();}
__attribute__((visibility("default"))) void pw_full_pipeline_s1_memory_stop(){pw_s1_memory_stop();}
}
