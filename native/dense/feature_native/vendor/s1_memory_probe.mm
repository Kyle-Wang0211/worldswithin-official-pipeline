#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "s1_memory_probe.h"
#include "../../memory_policy.h"
#include <mach/mach.h>
#include <os/proc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <condition_variable>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace {
struct Snapshot {uint64_t footprint=0,rss=0,available=0,metal=0,registry=0;int vm_rc=-1,rss_rc=-1;bool metal_observed=false;double epoch=0;uint64_t monotonic=0;std::string runtime_snapshot="null";};
std::mutex mu,start_mu;std::condition_variable cv;std::atomic<bool> running{false};std::thread sampler;
Snapshot last;uint64_t ceiling=2000000000ULL,min_available=268435456ULL,peak_footprint=0,peak_rss=0,peak_metal=0;
bool tripped=false;int cancel_rc=0,log_fd=-1;std::string reason,phase="LOAD",path;size_t ordinal=0;void* active_options=nullptr;int (*active_cancel)(void*)=nullptr;
id<MTLDevice> metal_device=nil;
uint64_t configured(const char* key,uint64_t fallback) {const char* p=std::getenv(key);if(!p||!*p)return fallback;for(const char* q=p;*q;++q)if(*q<'0'||*q>'9')throw std::runtime_error("memory guard option requires ASCII decimal digits only");char* end=nullptr;errno=0;auto v=std::strtoull(p,&end,10);if(errno||*end||v==0||v>2000000000ULL)throw std::runtime_error("memory guard option must be 1..2000000000 bytes");return v;}
Snapshot collect() {
 Snapshot s;@autoreleasepool {
  s.epoch=NSDate.date.timeIntervalSince1970;s.monotonic=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();s.runtime_snapshot=pw_s1_runtime_snapshot_json();
  task_vm_info_data_t vm={};mach_msg_type_number_t n=TASK_VM_INFO_COUNT;s.vm_rc=task_info(mach_task_self(),TASK_VM_INFO,(task_info_t)&vm,&n);if(s.vm_rc==KERN_SUCCESS)s.footprint=vm.phys_footprint;
  mach_task_basic_info_data_t basic={};n=MACH_TASK_BASIC_INFO_COUNT;s.rss_rc=task_info(mach_task_self(),MACH_TASK_BASIC_INFO,(task_info_t)&basic,&n);if(s.rss_rc==KERN_SUCCESS)s.rss=basic.resident_size;
  s.available=os_proc_available_memory();if(metal_device){s.metal_observed=true;s.metal=metal_device.currentAllocatedSize;s.registry=metal_device.registryID;}
 }return s;
}
std::string quote(const std::string& value) {@autoreleasepool {NSData* data=[NSJSONSerialization dataWithJSONObject:@[[NSString stringWithUTF8String:value.c_str()]] options:0 error:nil];NSString* text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];std::string result=text.UTF8String;return result.substr(1,result.size()-2);}}
std::string json_locked(const char* event) {
 char b[4096];std::snprintf(b,sizeof(b),"{\"event\":%s,\"pid\":%d,\"ordinal\":%zu,\"epoch_s\":%.9f,\"phase\":%s,\"phys_footprint_bytes\":%llu,\"rss_bytes\":%llu,\"os_proc_available_memory_bytes\":%llu,\"vm_query_rc\":%d,\"rss_query_rc\":%d,\"metal_current_allocated_bytes\":%llu,\"metal_counter_observed\":%s,\"metal_registry_id\":%llu,\"footprint_ceiling_bytes\":%llu,\"min_os_available_bytes\":%llu,\"observed_peak_footprint_bytes\":%llu,\"observed_peak_rss_bytes\":%llu,\"observed_peak_metal_bytes\":%llu,\"guard_tripped\":%s,\"guard_reason\":%s,\"cancel_rc\":%d,\"sampler_threads\":1,\"sample_interval_ms\":100,\"log_path\":%s,\"metal_scope\":\"MTLCreateSystemDefaultDevice.currentAllocatedSize; exact Dawn MTLDevice object match unverified\",\"peak_scope\":\"discrete observations, not allocator lifetime peak or hard peak guarantee\"}",quote(event).c_str(),getpid(),ordinal,last.epoch,quote(phase).c_str(),(unsigned long long)last.footprint,(unsigned long long)last.rss,(unsigned long long)last.available,last.vm_rc,last.rss_rc,(unsigned long long)last.metal,last.metal_observed?"true":"false",(unsigned long long)last.registry,(unsigned long long)ceiling,(unsigned long long)min_available,(unsigned long long)peak_footprint,(unsigned long long)peak_rss,(unsigned long long)peak_metal,tripped?"true":"false",quote(reason).c_str(),cancel_rc,quote(path).c_str());std::string result=b;if(result.empty()||result.back()!='}')return "{\"observer_serialization_error\":true}";result.pop_back();result+=",\"os_sample_monotonic_ns\":"+std::to_string(last.monotonic)+",\"runtime_allocator_boundary_snapshot\":"+last.runtime_snapshot+"}";return result;
}
void save_locked(const char* event) {++ordinal;auto line=json_locked(event)+"\n";if(log_fd>=0){ssize_t n=write(log_fd,line.data(),line.size());if(n!=(ssize_t)line.size()){tripped=true;reason="memory_log_write_failed";}if(tripped)fsync(log_fd);}}
void accept_locked(Snapshot s,const char* event) {
 last=s;peak_footprint=std::max(peak_footprint,s.footprint);peak_rss=std::max(peak_rss,s.rss);peak_metal=std::max(peak_metal,s.metal);
 if(!tripped){if(s.vm_rc!=KERN_SUCCESS||s.rss_rc!=KERN_SUCCESS){tripped=true;reason="memory_query_failed";}else if(s.footprint>=ceiling){tripped=true;reason="phys_footprint_ceiling";}else if(s.available<min_available){tripped=true;reason="os_remaining_budget_below_margin";}}
 if(tripped&&active_options&&active_cancel){cancel_rc=active_cancel(active_options);active_cancel=nullptr;}
 save_locked(event);
}
}
extern "C" {
int pw_s1_memory_start() {
 std::lock_guard<std::mutex> start(start_mu);if(running)return 0;
 try {@autoreleasepool {
  if(!reconstruction_memory_policy::valid())throw std::runtime_error("unknown explicit memory policy");ceiling=reconstruction_memory_policy::observe_only()?reconstruction_memory_policy::kObserveLimit:configured("RECONSTRUCTION_S1_FOOTPRINT_CEILING_BYTES",2000000000ULL);min_available=configured("RECONSTRUCTION_S1_MIN_AVAILABLE_BYTES",268435456ULL);
  const char* home=std::getenv("HOME");if(!home||*home!='/')return -1;std::string dir=std::string(home)+"/Documents/reconstruction_s1_memory";if(mkdir(dir.c_str(),0755)!=0&&errno!=EEXIST)return -2;
  path=dir+"/pid_"+std::to_string(getpid())+".jsonl";log_fd=open(path.c_str(),O_CREAT|O_APPEND|O_WRONLY,0644);if(log_fd<0)return -3;
  metal_device=MTLCreateSystemDefaultDevice();tripped=false;reason.clear();phase="LOAD";peak_footprint=peak_rss=peak_metal=0;cancel_rc=0;ordinal=0;
  {std::lock_guard<std::mutex> lock(mu);accept_locked(collect(),"memory_begin");if(tripped){close(log_fd);log_fd=-1;return -4;}}
  running=true;sampler=std::thread([](){while(running){@autoreleasepool {auto s=collect();std::unique_lock<std::mutex> lock(mu);accept_locked(s,"memory_sample");cv.wait_for(lock,std::chrono::milliseconds(100),[]{return !running.load();});}}});return 0;
 }}catch(...){if(log_fd>=0)close(log_fd);log_fd=-1;return -5;}
}
void pw_s1_memory_stop() {
 std::lock_guard<std::mutex> start(start_mu);running=false;cv.notify_all();if(sampler.joinable())sampler.join();std::lock_guard<std::mutex> lock(mu);active_options=nullptr;active_cancel=nullptr;if(log_fd>=0){accept_locked(collect(),"memory_end");fsync(log_fd);close(log_fd);log_fd=-1;}metal_device=nil;
}
int pw_s1_memory_preflight(const char* label,size_t extra,char* error,int cap) {
 auto s=collect();std::lock_guard<std::mutex> lock(mu);phase=label?label:"UNKNOWN";accept_locked(s,"memory_boundary");
 if(!tripped && (extra>ceiling || last.footprint>ceiling-extra)){tripped=true;reason="known_host_copy_would_cross_footprint_ceiling";save_locked("memory_refusal");}
 if(tripped){if(error&&cap>0)std::snprintf(error,cap,"S1_MEMORY_GUARD:%s footprint=%llu ceiling=%llu available=%llu log=%s",reason.c_str(),(unsigned long long)last.footprint,(unsigned long long)ceiling,(unsigned long long)last.available,path.c_str());return -1;}return 0;
}
void pw_s1_memory_active_run(void* options,int (*cancel)(void*)) {std::lock_guard<std::mutex> lock(mu);active_options=options;active_cancel=cancel;if(tripped&&options&&cancel){cancel_rc=cancel(options);active_cancel=nullptr;save_locked("memory_cancel_before_run");}}
void pw_s1_memory_clear_run() {std::lock_guard<std::mutex> lock(mu);active_options=nullptr;active_cancel=nullptr;}
int pw_s1_memory_guard_tripped() {std::lock_guard<std::mutex> lock(mu);return tripped?1:0;}
const char* pw_s1_memory_json() {thread_local std::string result;std::lock_guard<std::mutex> lock(mu);result=json_locked("memory_info");return result.c_str();}
}
