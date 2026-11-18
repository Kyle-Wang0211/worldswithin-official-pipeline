#pragma once
#include <array>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace pw_c2 {
struct Token {uintptr_t buffer=0,device=0;uint64_t generation=0;size_t bytes=0;};
struct Record {Token token;bool busy=false,failed=false,covered=false;};
inline bool Enabled(){const char*v=std::getenv("RECONSTRUCTION_BENCH_UMA_INPUT_REWRITE_MAP");return v&&std::strcmp(v,"1")==0;}
struct Counters{uint64_t registered=0,attempted=0,success=0,waits=0,timeouts=0,map_failures=0,wait_failures=0,bytes=0,coverage=0;};
class Registry {
 std::mutex mutex_;std::array<Record,64> rows_{};uint64_t next_=0;Counters counters_;
 public:
 void Register(uintptr_t b,uintptr_t d,size_t n){std::lock_guard<std::mutex>g(mutex_);if(!b||!d||!n)throw std::runtime_error("C2 invalid registration");for(auto&r:rows_)if(!r.token.buffer||r.token.buffer==b){if(r.busy)throw std::runtime_error("C2 active generation replacement");r={{b,d,++next_,n},false,false,false};++counters_.registered;return;}throw std::runtime_error("C2 registry capacity");}
 bool RegisterIfEligible(uintptr_t b,uintptr_t d,size_t n,bool enabled,bool feature,bool uma){if(!enabled||!feature||!uma)return false;Register(b,d,n);return true;}
 void Attempt(){std::lock_guard<std::mutex>g(mutex_);++counters_.attempted;}
 void Wait(){std::lock_guard<std::mutex>g(mutex_);++counters_.waits;}
 void Timeout(){std::lock_guard<std::mutex>g(mutex_);++counters_.timeouts;}
 void MapFailure(){std::lock_guard<std::mutex>g(mutex_);++counters_.map_failures;}
 void WaitFailure(){std::lock_guard<std::mutex>g(mutex_);++counters_.wait_failures;}
 Counters Stats(){std::lock_guard<std::mutex>g(mutex_);return counters_;}
 int Json(char*out,size_t cap){const auto c=Stats();return std::snprintf(out,cap,"{\"schema\":\"RECONSTRUCTION_C2_REWRITE_ACTUAL_V1\",\"opt_in\":%u,\"registered_generations\":%llu,\"map_async_attempted\":%llu,\"write_unmap_success\":%llu,\"wait_calls\":%llu,\"timeouts\":%llu,\"map_failures\":%llu,\"wait_failures\":%llu,\"direct_bytes\":%llu,\"unique_rewritten_generations\":%llu}",unsigned(Enabled()),(unsigned long long)c.registered,(unsigned long long)c.attempted,(unsigned long long)c.success,(unsigned long long)c.waits,(unsigned long long)c.timeouts,(unsigned long long)c.map_failures,(unsigned long long)c.wait_failures,(unsigned long long)c.bytes,(unsigned long long)c.coverage);}
 void Retire(uintptr_t b){std::lock_guard<std::mutex>g(mutex_);for(auto&r:rows_)if(r.token.buffer==b){r.token={};r.failed=true;return;}}
 Token Begin(uintptr_t b,uintptr_t d,size_t n,bool enabled,bool feature,bool uma,bool mapwrite,bool unmapped){if(!enabled||!feature||!uma||!mapwrite)return {};std::lock_guard<std::mutex>g(mutex_);for(auto&r:rows_)if(r.token.buffer==b){if(r.token.device!=d||r.token.bytes!=n||!unmapped||r.busy||r.failed)throw std::runtime_error("C2 identity/generation/inflight/state rejected");r.busy=true;return r.token;}return {};}
 bool Valid(const Token&t){std::lock_guard<std::mutex>g(mutex_);for(const auto&r:rows_)if(r.token.buffer==t.buffer&&r.token.generation==t.generation&&r.token.device==t.device&&r.token.bytes==t.bytes)return r.busy&&!r.failed;return false;}
 void End(const Token&t,bool ok){std::lock_guard<std::mutex>g(mutex_);for(auto&r:rows_)if(r.token.buffer==t.buffer&&r.token.generation==t.generation){r.busy=false;r.failed=!ok;if(ok){++counters_.success;counters_.bytes+=t.bytes;if(!r.covered){r.covered=true;++counters_.coverage;}}return;}}
};
inline Registry registry;
enum class Poll {Pending,Success,Error};
                                                                              
                                                                                   
template<class Ops> bool Execute(Registry& r,const Token&t,Ops&ops){
 if(!t.buffer)return false;
 try{
  if(!r.Valid(t))throw std::runtime_error("C2 stale generation before flush");
  ops.Flush();
  if(!r.Valid(t))throw std::runtime_error("C2 stale generation before map");
  ops.Begin();r.Attempt();auto result=Poll::Pending;
  while(result==Poll::Pending&&!ops.Expired()){r.Wait();try{result=ops.PollOnce();}catch(...){r.WaitFailure();throw;}}
  if(result==Poll::Pending){r.Timeout();r.Wait();ops.CancelRetain();throw std::runtime_error("C2 map deadline; callback capsule retained");}
  if(result!=Poll::Success){r.MapFailure();throw std::runtime_error("C2 MapAsync rejected; no CPU write or fallback");}
  if(!r.Valid(t)||!ops.Mapped())throw std::runtime_error("C2 generation/state changed before CPU write");
  ops.WriteUnmap();r.End(t,true);return true;
 }catch(...){r.End(t,false);throw;}
}
}
