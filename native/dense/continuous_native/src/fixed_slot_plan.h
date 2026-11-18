#pragma once
#include <array>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>
namespace reconstruction::fixed_material {
constexpr int kViews=10,kLevels=3;
struct Move{int source,destination;}; // -1 is the single-level scratch, never a photo bank.
struct Plan{std::array<int,10> desired;std::vector<Move>moves;std::vector<int>miss_slots;size_t hits=0;bool needs_scratch=false;};
inline Plan make_plan(const std::array<int,10>&old,const std::array<int,10>&wanted){Plan p;p.desired=wanted;std::set<int>ids(wanted.begin(),wanted.end());if(ids.size()!=10||*ids.begin()<0)throw std::runtime_error("canonical ten distinct original views");std::map<int,int>photo_slot;for(int i=0;i<10;++i)if(old[i]>=0&&!photo_slot.emplace(old[i],i).second)throw std::runtime_error("duplicate residentphoto");std::map<int,int>edges;for(int dst=0;dst<10;++dst){auto it=photo_slot.find(wanted[dst]);if(it==photo_slot.end())p.miss_slots.push_back(dst);else{++p.hits;if(it->second!=dst)edges.emplace(it->second,dst);}}
 while(true){bool progress=false;for(auto it=edges.begin();it!=edges.end();++it)if(!edges.count(it->second)){p.moves.push_back({it->first,it->second});edges.erase(it);progress=true;break;}if(!progress)break;}
 while(!edges.empty()){int first=edges.begin()->first,current=first;p.moves.push_back({first,-1});p.needs_scratch=true;while(true){auto it=edges.begin();while(it!=edges.end()&&it->second!=current)++it;if(it==edges.end())throw std::runtime_error("invalid transfercycle");int source=it->first;edges.erase(it);if(source==first){p.moves.push_back({-1,current});break;}p.moves.push_back({source,current});current=source;}}
 return p;
}
struct PinState{bool live=false,failed=false;void prepare()const{if(live||failed)throw std::runtime_error("pinnedorfailed inputs cannot mutate");}void acquire(){prepare();live=true;}void release(){if(!live)throw std::runtime_error("double inputlease release");live=false;}void fail(){failed=true;}};
}
