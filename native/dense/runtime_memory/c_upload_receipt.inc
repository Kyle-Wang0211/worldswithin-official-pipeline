#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <webgpu/webgpu.h>
namespace c_upload_receipt {
struct Row {uint64_t id=0,expected=0,request=0,descriptor=0,buffer=0,device=0,usage=0,first_bytes=0;uint32_t actual_feature=0,cached_feature=0,adapter_type=0,standard_usage=0,size_match=0,selected_map=0,status=0,map_state=0,has_mapwrite=0,mapped=0,first_kind=0,writes=0,direct=0,staging=0,post_unmapped=0,released=0;};
struct State {std::mutex mutex;std::array<Row,64>rows{};uint64_t count=0,attempted=0;};
inline State state;inline std::atomic<bool> active{false};inline std::atomic<uint64_t> lost{0};
inline uint64_t Begin(uint64_t expected,uint64_t request,bool actual,bool cached,uint32_t adapter,bool standard,bool selected,WGPUDevice device)noexcept{
 try{std::lock_guard<std::mutex>lock(state.mutex);++state.attempted;active=true;if(state.count==state.rows.size()){++lost;return 0;}auto&r=state.rows[state.count++];r.id=state.count;r.expected=expected;r.request=request;r.actual_feature=actual;r.cached_feature=cached;r.adapter_type=adapter;r.standard_usage=standard;r.size_match=expected==request;r.selected_map=selected;r.device=reinterpret_cast<uintptr_t>(device);return r.id;}catch(...){++lost;return 0;}}
inline void Complete(uint64_t id,WGPUBuffer b)noexcept{if(!id)return;try{std::lock_guard<std::mutex>lock(state.mutex);auto&r=state.rows[id-1];r.buffer=reinterpret_cast<uintptr_t>(b);r.descriptor=wgpuBufferGetSize(b);r.usage=wgpuBufferGetUsage(b);r.map_state=uint32_t(wgpuBufferGetMapState(b));r.has_mapwrite=(r.usage&WGPUBufferUsage_MapWrite)!=0;r.mapped=r.map_state==uint32_t(WGPUBufferMapState_Mapped);r.status=1;}catch(...){++lost;}}
inline void Failed(uint64_t id)noexcept{if(!id)return;try{std::lock_guard<std::mutex>lock(state.mutex);state.rows[id-1].status=2;}catch(...){++lost;}}
struct Attempt {uint64_t id;~Attempt(){if(id)Failed(id);}void CompleteBuffer(WGPUBuffer b)noexcept{Complete(id,b);id=0;}};
inline void Write(WGPUBuffer b,uint64_t bytes,bool direct)noexcept{if(!active.load())return;try{std::lock_guard<std::mutex>lock(state.mutex);for(size_t i=state.count;i>0;--i){auto&r=state.rows[i-1];if(r.buffer==reinterpret_cast<uintptr_t>(b)&&!r.released&&r.status==1){if(!r.writes){r.first_kind=direct?1:2;r.first_bytes=bytes;}++r.writes;if(direct)++r.direct;else ++r.staging;r.post_unmapped=wgpuBufferGetMapState(b)==WGPUBufferMapState_Unmapped;return;}}}catch(...){++lost;}}
inline void Release(WGPUBuffer b)noexcept{if(!active.load())return;try{std::lock_guard<std::mutex>lock(state.mutex);for(size_t i=state.count;i>0;--i){auto&r=state.rows[i-1];if(r.buffer==reinterpret_cast<uintptr_t>(b)&&!r.released&&r.status==1){r.released=1;return;}}}catch(...){++lost;}}
}
extern "C" __attribute__((visibility("default"))) int pw_upload_receipt_json_v1(char*out,size_t capacity)noexcept{
 if(!out||!capacity)return 2;out[0]=0;try{std::lock_guard<std::mutex>lock(c_upload_receipt::state.mutex);char buf[65536];size_t used=0;bool ok=true;auto append=[&](const char*format,auto...args){if(!ok)return;int n=std::snprintf(buf+used,sizeof(buf)-used,format,args...);if(n<0||size_t(n)>=sizeof(buf)-used){ok=false;return;}used+=size_t(n);};auto&s=c_upload_receipt::state;
 append("{\"schema\":\"RECONSTRUCTION_C_UPLOAD_ACTUAL_V1\",\"source\":\"actual_scoped_Create_and_Upload_buffer_properties\",\"attempted\":%llu,\"count\":%llu,\"lost\":%llu,\"release_is_request_not_retire\":1,\"rows\":[",(unsigned long long)s.attempted,(unsigned long long)s.count,(unsigned long long)c_upload_receipt::lost.load());
 for(size_t i=0;i<s.count;++i){auto&r=s.rows[i];if(i)append("%s",",");append("{\"id\":%llu,\"expected_bytes\":%llu,\"request_bytes\":%llu,\"descriptor_bytes\":%llu,\"buffer\":%llu,\"device\":%llu,\"usage\":%llu,\"actual_device_feature\":%u,\"cached_device_feature\":%u,\"adapter_type\":%u,\"standard_usage\":%u,\"size_match\":%u,\"selected_map\":%u,\"allocation_status\":%u,\"creation_map_state\":%u,\"has_mapwrite\":%u,\"creation_mapped\":%u,\"first_write_kind\":%u,\"first_write_bytes\":%llu,\"writes\":%u,\"direct_writes\":%u,\"staging_submissions\":%u,\"post_write_unmapped\":%u,\"release_requested\":%u}",(unsigned long long)r.id,(unsigned long long)r.expected,(unsigned long long)r.request,(unsigned long long)r.descriptor,(unsigned long long)r.buffer,(unsigned long long)r.device,(unsigned long long)r.usage,r.actual_feature,r.cached_feature,r.adapter_type,r.standard_usage,r.size_match,r.selected_map,r.status,r.map_state,r.has_mapwrite,r.mapped,r.first_kind,(unsigned long long)r.first_bytes,r.writes,r.direct,r.staging,r.post_unmapped,r.released);}
 char rewrite[1024];int rn=pw_c2::registry.Json(rewrite,sizeof(rewrite));if(rn<0||size_t(rn)>=sizeof(rewrite))return 2;append("],\"rewrite\":%s}",rewrite);if(!ok||used+1>capacity)return 2;std::memcpy(out,buf,used+1);return 0;}catch(...){return 3;}}
