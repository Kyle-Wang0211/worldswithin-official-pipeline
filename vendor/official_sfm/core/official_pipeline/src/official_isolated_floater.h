                                                                               
  
                                                                                             
                                                                                             
  
                                                                                                    
                                                                                                           
                                                                                                
                                                                                           
                                                                                                
  
                                                                                                        
                                                                         
                                                                                                   
                                                                     
  
                                                                                              
                                                                                           
                                               
                          
                        
                                                                                      
                                                                                                
                                                                                                           
                                                                         
  
                                                                                                 
                                                                                              
                                                                                                  
                                                                                      
  
                                                                                                         
                                                                          
  
                                                                                           
                                                                   
                                                                       
                                                                  
                                                                
                                                             
                                                             
                                                               
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <vector>

#include "official_scale_eps.h"

namespace aether_isolated_floater {

constexpr float kEpsMulDefault = 10.0f;                          
constexpr int kMinPtsDefault = 5;                                      
constexpr float kDmainMulDefault = 4.0f;                                          
constexpr float kCosDefault = 0.7f;                                           
                                                                                                
                                                                                                   
                                                                                 
                                                                                        
                                                                                                      
                                                                                                       
                                               
constexpr int kCosMaxPtsDefault = 20;                                             
constexpr size_t kMinCloudPts = 100;                                                 

using P3 = aether_scale_eps::P3;

struct Result {
  float eps = 0.0f;                                                          
  int clusters = 0;                           
  int killed_clusters = 0;
  int main_size = 0;                                 
  std::vector<uint32_t> kill;                                                 
};

namespace detail {

using Grid = aether_scale_eps::Grid;
using aether_scale_eps::dist2;

inline float EpsMul() {
  static const float v = aether_scale_eps::EnvMul(
      "OFFICIAL_AETHER_ISOLATED_FLOATER_EPS_MUL", kEpsMulDefault);
  return v;
}
inline int MinPts() {
  static const int v = aether_scale_eps::EnvInt(
      "OFFICIAL_AETHER_ISOLATED_FLOATER_MIN_PTS", kMinPtsDefault);
  return v;
}
inline float DmainMul() {
  static const float v = aether_scale_eps::EnvMul(
      "OFFICIAL_AETHER_ISOLATED_FLOATER_DMAIN_MUL", kDmainMulDefault);
  return v;
}
inline int CosMaxPts() {
  static const int v = static_cast<int>(aether_scale_eps::EnvMul(
      "OFFICIAL_AETHER_ISOLATED_FLOATER_COS_MAX_PTS",
      static_cast<float>(kCosMaxPtsDefault)));
  return v;
}

inline float CosThr() {
  static const float v = aether_scale_eps::EnvMul(
      "OFFICIAL_AETHER_ISOLATED_FLOATER_COS", kCosDefault);
  return v;
}

inline float ScaleEps(const std::vector<P3>& pts) {
  const float p90 = aether_scale_eps::NnP90(pts);
  if (!(p90 > 0.0f)) return 0.0f;
  return EpsMul() * p90;
}

}                     

                                                                 
                                                                                          
                                                                             
                                                                                                 
                                                                                             
                                                                               
inline Result Detect(const std::vector<P3>& pts,
                     const std::vector<P3>& view_dirs) {
  Result r;
  const size_t n = pts.size();
  if (n < kMinCloudPts) return r;

                                                                                                        
  const float eps = detail::ScaleEps(pts);
  if (!(eps > 0.0f)) return r;                                                      
  r.eps = eps;
  const bool has_dirs = (view_dirs.size() == n);

                                               
                                                                                              
                                                                                                
                                                                
                                                                                            
                                                                                                    
                                                                                        
                                                                                              
                                                                                     
                                                                                         
                                                                                              
  const float e2 = eps * eps;
  const float ccell = eps * 0.577f;
                                                                                                   
                                                                                              
                                                                                                    
                                              
  float lo[3] = {pts[0].x, pts[0].y, pts[0].z};
  float hi[3] = {pts[0].x, pts[0].y, pts[0].z};
  for (const auto& p : pts) {
    lo[0] = std::min(lo[0], p.x); hi[0] = std::max(hi[0], p.x);
    lo[1] = std::min(lo[1], p.y); hi[1] = std::max(hi[1], p.y);
    lo[2] = std::min(lo[2], p.z); hi[2] = std::max(hi[2], p.z);
  }
  const float bx = hi[0] - lo[0], by = hi[1] - lo[1], bz = hi[2] - lo[2];
  const float diag = std::sqrt(bx * bx + by * by + bz * bz);
                                                                                         
                                                                                                  
                                      
  auto ckey = [](int x, int y, int z) -> uint64_t {
    return (static_cast<uint64_t>(static_cast<uint32_t>(x) & 0x1FFFFFu) << 42) |
           (static_cast<uint64_t>(static_cast<uint32_t>(y) & 0x1FFFFFu) << 21) |
           (static_cast<uint64_t>(static_cast<uint32_t>(z) & 0x1FFFFFu));
  };
  std::unordered_map<uint64_t, uint32_t> slot_of;
  slot_of.reserve(n / 8 + 16);
  std::vector<std::array<int, 3>> cell_xyz;
  std::vector<uint32_t> slot_of_pt(n), cell_cnt;
  for (uint32_t i = 0; i < n; ++i) {
    const int cx = static_cast<int>(std::floor((pts[i].x - lo[0]) / ccell));
    const int cy = static_cast<int>(std::floor((pts[i].y - lo[1]) / ccell));
    const int cz = static_cast<int>(std::floor((pts[i].z - lo[2]) / ccell));
    const uint64_t k = ckey(cx, cy, cz);
    auto it = slot_of.find(k);
    uint32_t s;
    if (it == slot_of.end()) {
      s = static_cast<uint32_t>(cell_xyz.size());
      slot_of.emplace(k, s);
      cell_xyz.push_back({cx, cy, cz});
      cell_cnt.push_back(0);
    } else {
      s = it->second;
    }
    slot_of_pt[i] = s;
    ++cell_cnt[s];
  }
  const size_t ncell = cell_xyz.size();
  std::vector<uint32_t> cell_off(ncell + 1, 0);         
  for (size_t s = 0; s < ncell; ++s) cell_off[s + 1] = cell_off[s] + cell_cnt[s];
  std::vector<uint32_t> cell_pts(n);
  {
    std::vector<uint32_t> w(cell_off.begin(), cell_off.end() - 1);
    for (uint32_t i = 0; i < n; ++i) cell_pts[w[slot_of_pt[i]]++] = i;
  }

  aether_scale_eps::DSU dsu(n);
  for (size_t s = 0; s < ncell; ++s) {                          
    const uint32_t b = cell_off[s], e = cell_off[s + 1];
    for (uint32_t t = b + 1; t < e; ++t) dsu.unite(cell_pts[b], cell_pts[t]);
  }
  for (size_t s = 0; s < ncell; ++s) {                                    
    const int cx = cell_xyz[s][0], cy = cell_xyz[s][1], cz = cell_xyz[s][2];
    for (int dx = 0; dx <= 2; ++dx)
      for (int dy = (dx == 0 ? 0 : -2); dy <= 2; ++dy)
        for (int dz = (dx == 0 && dy == 0) ? 1 : -2; dz <= 2; ++dz) {
          const auto it = slot_of.find(ckey(cx + dx, cy + dy, cz + dz));
          if (it == slot_of.end()) continue;
          const uint32_t s2 = it->second;
          if (dsu.find(cell_pts[cell_off[s]]) ==
              dsu.find(cell_pts[cell_off[s2]])) continue;
          bool linked = false;
          for (uint32_t ai = cell_off[s]; ai < cell_off[s + 1] && !linked; ++ai) {
            const P3& pa = pts[cell_pts[ai]];
            for (uint32_t bi = cell_off[s2]; bi < cell_off[s2 + 1]; ++bi) {
              const uint32_t j = cell_pts[bi];
              if (detail::dist2(pa, pts[j]) <= e2) {
                dsu.unite(cell_pts[ai], j);
                linked = true;
                break;
              }
            }
          }
        }
  }

                          
  std::vector<uint32_t> root(n);
  std::unordered_map<uint32_t, uint32_t> size_of;
  size_of.reserve(n / 4 + 16);
  for (uint32_t i = 0; i < n; ++i) {
    root[i] = dsu.find(i);
    ++size_of[root[i]];
  }
  uint32_t main_root = root[0];
  uint32_t main_sz = 0;
  for (const auto& [rt, sz] : size_of) {
                                                                                              
    if (sz > main_sz || (sz == main_sz && rt < main_root)) {
      main_sz = sz;
      main_root = rt;
    }
  }
  r.main_size = static_cast<int>(main_sz);
  if (main_sz == n) return r;                              

                                                             
                                                                                         
                                                                                          
                                                                                                   
                                                                                        
                                                                                      
                                                                                              
  const float main_cell =
      std::max(eps, diag / static_cast<float>(aether_scale_eps::kEpsMaxRing - 2));
  detail::Grid mg(main_cell);
  for (uint32_t i = 0; i < n; ++i)
    if (root[i] == main_root) mg.add(pts[i], i);

                       
  std::unordered_map<uint32_t, std::vector<uint32_t>> groups;
  for (uint32_t i = 0; i < n; ++i)
    if (root[i] != main_root) groups[root[i]].push_back(i);
  r.clusters = static_cast<int>(groups.size());

  const int min_pts = detail::MinPts();
  const float dmain_thr = detail::DmainMul() * eps;
  const float cos_thr = detail::CosThr();

  for (const auto& [rt, members] : groups) {
                                                                                                   
    float best_d2 = -1.0f;
    uint32_t p_star = aether_scale_eps::kNoIndex;
    uint32_t q_star = aether_scale_eps::kNoIndex;
    for (uint32_t i : members) {
      float d2 = -1.0f;
      const uint32_t q = aether_scale_eps::NearestIndexed(mg, pts, pts[i], &d2);
      if (q == aether_scale_eps::kNoIndex || d2 < 0.0f) continue;
      if (best_d2 < 0.0f || d2 < best_d2) { best_d2 = d2; p_star = i; q_star = q; }
    }
    if (best_d2 < 0.0f) continue;                                                        
    const float dmain = std::sqrt(best_d2);

                                                                         
    float cosv = 0.0f;
    if (has_dirs) {
      const P3& p = pts[p_star];
      const P3& q = pts[q_star];
      float ox = p.x - q.x, oy = p.y - q.y, oz = p.z - q.z;
      const float on = std::sqrt(ox * ox + oy * oy + oz * oz);
      const P3& v = view_dirs[p_star];
      const float vn = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
      if (on > 0.0f && vn > 0.0f) {
        cosv = std::fabs((ox * v.x + oy * v.y + oz * v.z) / (on * vn));
      }
    }

    const int csize = static_cast<int>(members.size());
    const bool tiny_and_far = (csize < min_pts) && (dmain > dmain_thr);
    const bool ray_aligned = (csize < detail::CosMaxPts()) && (cosv > cos_thr);
    if (tiny_and_far || ray_aligned) {
      ++r.killed_clusters;
      for (uint32_t i : members) r.kill.push_back(i);
    }
  }
  std::sort(r.kill.begin(), r.kill.end());                                       
  return r;
}

}                                      
