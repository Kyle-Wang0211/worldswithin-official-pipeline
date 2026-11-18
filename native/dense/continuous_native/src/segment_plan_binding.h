#pragma once
#include "portable_sha.h"
#include "segment_model_identity.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
namespace reconstruction::mobile_segment_runtime {
inline std::string hashfile(const std::string&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("segment SHA open");CC_SHA256_CTX c;CC_SHA256_Init(&c);char b[65536];while(f){f.read(b,sizeof b);if(f.gcount())CC_SHA256_Update(&c,b,CC_LONG(f.gcount()));}if(!f.eof())throw std::runtime_error("segment SHA read");unsigned char h[32];CC_SHA256_Final(h,&c);std::string s;for(auto v:h){s+="0123456789abcdef"[v>>4];s+="0123456789abcdef"[v&15];}return s;}
inline std::vector<std::string> fields(const std::string&s){std::vector<std::string>v;std::istringstream f(s);std::string x;while(std::getline(f,x,'\t'))v.push_back(x);return v;}
inline std::mutex& env_mutex(){static std::mutex m;return m;}
inline bool& env_failed(){static bool bad=false;return bad;}
struct ScopedPlan {
 std::unique_lock<std::mutex> lock;std::string previous;bool had_previous=false,armed=false;
 ScopedPlan(int width,const std::string&model):lock(env_mutex(),std::try_to_lock){
  if(!lock.owns_lock())throw std::runtime_error("segmented scene already owns environment");
  if(env_failed())throw std::runtime_error("segment environment failed latch");
  const char* route=std::getenv("RECONSTRUCTION_MATCHED_MVS_ROUTE");
  if(route&&std::string(route)!="whole"&&std::string(route)!="segmented")throw std::runtime_error("RECONSTRUCTION_MATCHED_MVS_ROUTE must be whole or segmented");
  const size_t expected=reconstruction::segment_layout::expected_count(width);
  if(route&&std::string(route)=="whole"){
   const char*whole_parent=width==768?"4bfb139a9fd8f726e6a296826ef41ff5f3d77e0a0e9db8220c7df4ef8c67cdf1":width==1536?"c6ef1789c0897264112ad28bf71e0e1f1c47166ed98559faa43b33b00ca1d680":nullptr;
   if(!whole_parent||hashfile(model)!=whole_parent)throw std::runtime_error("isolated whole exact current parent required");
   const char* cache=std::getenv("RECONSTRUCTION_BENCH_SEGMENT_SESSION_CACHE");if(cache&&std::string(cache)!="0")throw std::runtime_error("whole cannot use segment SessionLRU");
   if(const char*p=std::getenv("RECONSTRUCTION_SEGMENT_PLAN")){previous=p;had_previous=true;}
   if(::unsetenv("RECONSTRUCTION_SEGMENT_PLAN"))throw std::runtime_error("whole plan environment clear");armed=true;
   std::fprintf(stderr,"MATCHED_MVS_ROUTE whole width=%d parent_verified=1 segment_binding_not_used_for_whole=1\n",width);return;
  }
  const char* key=width==768?"RECONSTRUCTION_S6_SEGMENT_PLAN_LOW":"RECONSTRUCTION_S6_SEGMENT_PLAN_HIGH";
  const char*raw=std::getenv(key);if(!raw||raw[0]!='/')throw std::runtime_error(std::string("explicit perpass plan required: ")+key);
  std::string plan=raw;if(std::filesystem::canonical(plan).string()!=plan)throw std::runtime_error("segment plan alias");
  std::ifstream in(plan+".binding.tsv");std::string line;if(!std::getline(in,line))throw std::runtime_error("segment binding missing");auto h=fields(line);
  if(h.size()!=5||h[0]!="RECONSTRUCTION_SEGMENT_PLAN_V1"||h[1]!=std::to_string(width)||h[2]!=reconstruction::segment_layout::expected_rest(width)||h[2]!=hashfile(model)||h[3]!=hashfile(plan)||h[4]!=std::to_string(expected))throw std::runtime_error("segment LOW/HIGH/model/plan identity mismatch");
  std::ifstream pf(plan);std::vector<std::string>modelpaths;std::string last_records;while(std::getline(pf,line)){auto v=fields(line);if(v.size()==3&&v[0]=="MODEL"){if(v[2]!="1")throw std::runtime_error("segment capture flag");modelpaths.push_back(v[1]);}else if(v.size()==3&&v[0]=="LAST"){last_records+=line+char(10);}else throw std::runtime_error("segment exact MODEL/LAST record");}
  if(modelpaths.size()!=expected)throw std::runtime_error("segment perbucket modelcount");
  CC_SHA256_CTX lc;CC_SHA256_Init(&lc);CC_SHA256_Update(&lc,last_records.data(),CC_LONG(last_records.size()));unsigned char lh[32];CC_SHA256_Final(lh,&lc);std::string last_hash;for(auto b:lh){last_hash+="0123456789abcdef"[b>>4];last_hash+="0123456789abcdef"[b&15];}if(last_hash!=reconstruction::segment_layout::expected_last(width))throw std::runtime_error("segment LAST identity mismatch");
  for(size_t i=0;i<expected;++i){if(!std::getline(in,line))throw std::runtime_error("segment binding truncated");auto v=fields(line);if(v.size()!=3||v[0]!="MODEL"||v[1]!=modelpaths[i]||v[2]!=reconstruction::segment_layout::expected_model(width,i)||std::filesystem::canonical(v[1]).string()!=v[1]||hashfile(v[1])!=v[2])throw std::runtime_error("segment model order/hash/path mismatch");}
  if(std::getline(in,line))throw std::runtime_error("segment binding extra records");
  if(const char*p=std::getenv("RECONSTRUCTION_SEGMENT_PLAN")){previous=p;had_previous=true;}
  if(::setenv("RECONSTRUCTION_SEGMENT_PLAN",plan.c_str(),1))throw std::runtime_error("segment environment set");armed=true;
 }
 ~ScopedPlan(){if(armed){int rc=had_previous?::setenv("RECONSTRUCTION_SEGMENT_PLAN",previous.c_str(),1): ::unsetenv("RECONSTRUCTION_SEGMENT_PLAN");if(rc){env_failed()=true;std::fprintf(stderr,"S6_PLAN_ENV_RESTORE_FAILED\n");}}}
 ScopedPlan(const ScopedPlan&)=delete;ScopedPlan&operator=(const ScopedPlan&)=delete;
};
}
