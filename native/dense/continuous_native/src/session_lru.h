#pragma once
#include "src/dawn/common/LRUCache.h"
#include "segment_model_identity.h"
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstdint>
namespace reconstruction {
inline unsigned session_lru_mode(const char*s){if(!s||std::string(s)=="0")return 0;if(std::string(s)=="pass")return 1;throw std::runtime_error("SESSION_LRU_DISABLED_OR_PASS");}
inline size_t verify_declared_pass(int width,const std::string&parent,const std::vector<std::string>&models,const std::string&last){size_t n=segment_layout::expected_count(width);if(parent!=segment_layout::expected_rest(width)||models.size()!=n||last!=segment_layout::expected_last(width))throw std::runtime_error("SESSION_LRU_PARENT_COUNT_LAST_MISMATCH");for(size_t i=0;i<n;i++)if(models[i]!=segment_layout::expected_model(width,i))throw std::runtime_error("SESSION_LRU_OLD_OR_WRONG_ORDERED_MODEL");return n;}
struct SegmentKeyHash {size_t operator()(const std::string&k)const{return std::hash<std::string>{}(k);}bool operator()(const std::string&a,const std::string&b)const{return a==b;}};
// Scene/pass-local. Value is initialized Session only: no IO/values/graph owner.
// Frozen identity and count prevent a mismatched capacity26 traversal over27 distinct keys.
template<class Session>struct SessionLru final: dawn::LRUCache<std::string,std::shared_ptr<Session>,SegmentKeyHash>{
 using Ptr=std::shared_ptr<Session>;using Base=dawn::LRUCache<std::string,Ptr,SegmentKeyHash>;
 uint64_t hits=0,misses=0,evictions=0,created=0;bool closed=false,clearing=false;
 const size_t declared_count;const std::vector<std::string>frozen;std::vector<std::weak_ptr<Session>> witnesses;
 explicit SessionLru(size_t count,std::vector<std::string>keys):Base(count),declared_count(count),frozen(std::move(keys)){if((count!=26&&count!=27)||frozen.size()!=count||std::set<std::string>(frozen.begin(),frozen.end()).size()!=count)throw std::runtime_error("SESSION_LRU_DECLARED_CAPACITY_KEYS_MISMATCH");witnesses.reserve(count);}
 void EvictedFromCache(const Ptr&)override{if(!clearing)++evictions;}
 size_t Size(){return this->mCache.Use([](auto c){return c->map.size();});}
 size_t Alive()const{size_t n=0;for(auto&w:witnesses)if(!w.expired())++n;return n;}
 size_t Close(){if(!closed){closed=true;clearing=true;this->Clear();clearing=false;}return Alive();}
 ~SessionLru(){Close();}
 template<class Create>Ptr Get(size_t sid,const std::string&key,bool active_lease,Create create){
  if(active_lease)throw std::runtime_error("SESSION_LRU_ACTIVE_LEASE");
  if(closed)throw std::runtime_error("SESSION_LRU_CLOSED");
  if(sid>=frozen.size()||key!=frozen[sid])throw std::runtime_error("SESSION_LRU_WRONG_MODEL_OR_COUNT");
  auto before=misses;auto r=this->GetOrCreate(key,[&](const std::string&)->dawn::Result<Ptr,std::runtime_error>{++misses;auto p=create();if(!p)throw std::runtime_error("SESSION_LRU_NULL");witnesses.push_back(p);++created;return p;});
  if(r.IsError()){auto error=r.AcquireError();throw std::runtime_error("SESSION_LRU_CREATE_ERROR");}if(before==misses)++hits;return r.AcquireSuccess();
 }
};
}
