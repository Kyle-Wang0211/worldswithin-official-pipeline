                                                                                 
  
                                                                                                   
                                                                                                
                                                                                             
                                                                                              
                                                                                                     
                                                                                     
  
                                                                                                 
                                                                                       
                                                                                             
                                                                                
                                                 
                                                               
  
                                                         
                                                                                   
                                                                    
                                                                                                    
                              
                                                                                          
                                                                                          
                                                                                              
  
                                                                                    
                                                                                    
                                                             
                                                                             
  
                                                                           
                                                                                  
                                                                                                      
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "official_scale_eps.h"

namespace aether_mirror_ghost {

constexpr float kFloorBinCount = 80.0f;
constexpr float kFloorSignif = 0.015f;                                              
constexpr int kMinCluster = 8;                                                           
constexpr float kMirrorRatio = 0.6f;                                                        

                                            
                                                                                          
                                                                                                          
                       
constexpr float kEpsMulDefault = 10.0f;
constexpr int kEpsSampleMax = aether_scale_eps::kEpsSampleMax;
constexpr int kEpsMaxRing = aether_scale_eps::kEpsMaxRing;

                                           
                                                                                                    
                                                                                      
                                                                        
                                                 
                                                                                          
                                                   
constexpr float kFloorBandMul = 0.5897f;                                                
constexpr float kBelowTolMul = 1.1056f;                                            
constexpr float kMirrorRMul = 2.2112f;                                 
constexpr float kClusterEpsMul = 2.5798f;                           
constexpr float kFloorPadMul = 0.7371f;                                                    

using P3 = aether_scale_eps::P3;

struct Result {
  float floor_y = 0.0f;
  float eps = 0.0f;                                                         
  int candidates = 0;
  int guard = 0;                                                                                      
  int clusters = 0;                                       
  int killed_clusters = 0;
  std::vector<uint32_t> kill;                                         
};

namespace detail {

                                                                           
                                                                                           
                                                
using Grid = aether_scale_eps::Grid;
using aether_scale_eps::dist2;

inline float EpsMul() {
  static const float v =
      aether_scale_eps::EnvMul("OFFICIAL_AETHER_MIRROR_GHOST_EPS_MUL",
                               kEpsMulDefault);
  return v;
}

                                                              
inline float ScaleEps(const std::vector<P3>& pts) {
  const float p90 = aether_scale_eps::NnP90(pts);
  if (!(p90 > 0.0f)) return 0.0f;
  return EpsMul() * p90;
}

}                     

                                                                                             
inline Result Detect(const std::vector<P3>& pts) {
  Result r;
  const size_t n = pts.size();
  if (n < 100) return r;                             

                                                                                                        
  const float eps = detail::ScaleEps(pts);
  if (!(eps > 0.0f)) return r;                                                      
  r.eps = eps;
  const float floor_band = kFloorBandMul * eps;
  const float below_tol = kBelowTolMul * eps;
  const float mirror_r = kMirrorRMul * eps;
  const float cluster_eps = kClusterEpsMul * eps;
  const float floor_pad = kFloorPadMul * eps;

                                                                                             
                                                            
  float ymin = pts[0].y, ymax = pts[0].y;
  for (const auto& p : pts) { ymin = std::min(ymin, p.y); ymax = std::max(ymax, p.y); }
  if (!(ymax > ymin)) return r;
  const int bins = static_cast<int>(kFloorBinCount);
  const float bw = (ymax - ymin) / bins;
  std::vector<int> hist(bins, 0);
  for (const auto& p : pts) {
    int b = static_cast<int>((p.y - ymin) / bw);
    hist[std::min(std::max(b, 0), bins - 1)]++;
  }
  const int th = static_cast<int>(kFloorSignif * static_cast<float>(n));
  int fbin = -1;
  for (int b = 0; b < bins; ++b) {
    if (hist[b] > th) { fbin = b; break; }
  }
  float floor_y;
  if (fbin < 0) {
                                        
    std::vector<float> ys; ys.reserve(n);
    for (const auto& p : pts) ys.push_back(p.y);
    std::nth_element(ys.begin(), ys.begin() + n / 50, ys.end());
    floor_y = ys[n / 50];
  } else {
    const float lo = ymin + fbin * bw - floor_pad;
    const float hi = ymin + (fbin + 1) * bw + floor_pad;
    std::vector<float> band;
    for (const auto& p : pts) if (p.y >= lo && p.y <= hi) band.push_back(p.y);
    if (band.empty()) return r;
    std::nth_element(band.begin(), band.begin() + band.size() / 2, band.end());
    floor_y = band[band.size() / 2];
  }
  r.floor_y = floor_y;

                                                                                        
  std::vector<uint32_t> cand;
  detail::Grid up(mirror_r);
  for (uint32_t i = 0; i < n; ++i) {
    const float y = pts[i].y;
    if (y < floor_y - below_tol) cand.push_back(i);
    else if (std::fabs(y - floor_y) >= floor_band) up.add(pts[i], i);
  }
  r.candidates = static_cast<int>(cand.size());
  if (cand.empty()) return r;

                                                                                      
                                                                                                    
                                                                                                   
                                                                                               
                                                                                               
                                                                                             
    
                                                                                                   
                                                                              
                                                         
                                                                                                  
                                                                                      
                                                                             
                                                                                              
                                                                                                     
  constexpr float kMaxCandidateFrac = 0.02f;
  constexpr float kFloorGapMul = 3.0f;
  if (static_cast<float>(cand.size()) > kMaxCandidateFrac * static_cast<float>(n)) {
    r.guard = 1;
    return r;
  }
  {
    std::vector<float> ys2;
    ys2.reserve(n);
    for (const auto& p : pts) ys2.push_back(p.y);
    std::nth_element(ys2.begin(), ys2.begin() + n / 50, ys2.end());
    const float p2 = ys2[n / 50];
    if (floor_y - p2 > kFloorGapMul * eps) {
      r.guard = 2;
      return r;
    }
  }

                                                                         
  detail::Grid cg(cluster_eps);
  for (uint32_t k = 0; k < cand.size(); ++k) cg.add(pts[cand[k]], k);
  std::vector<int> lbl(cand.size(), -1);
  int ncl = 0;
  for (uint32_t s = 0; s < cand.size(); ++s) {
    if (lbl[s] >= 0) continue;
    std::vector<uint32_t> stack{s};
    lbl[s] = ncl;
    while (!stack.empty()) {
      const uint32_t j = stack.back(); stack.pop_back();
      cg.forNeighbors(pts[cand[j]], [&](uint32_t k2) {
        if (lbl[k2] < 0 &&
            detail::dist2(pts[cand[j]], pts[cand[k2]]) <= cluster_eps * cluster_eps) {
          lbl[k2] = ncl;
          stack.push_back(k2);
        }
      });
    }
    ++ncl;
  }

                                                                                
  const float r2 = mirror_r * mirror_r;
  for (int c = 0; c < ncl; ++c) {
    std::vector<uint32_t> sel;
    for (uint32_t k = 0; k < cand.size(); ++k)
      if (lbl[k] == c) sel.push_back(cand[k]);
    if (static_cast<int>(sel.size()) < kMinCluster) continue;
    ++r.clusters;
    int hit = 0;
    for (uint32_t i : sel) {
      P3 mp{pts[i].x, 2.0f * floor_y - pts[i].y, pts[i].z};
      bool found = false;
      up.forNeighbors(mp, [&](uint32_t j) {
        if (!found && detail::dist2(mp, pts[j]) <= r2) found = true;
      });
      if (found) ++hit;
    }
    if (static_cast<float>(hit) / static_cast<float>(sel.size()) >= kMirrorRatio) {
      ++r.killed_clusters;
      for (uint32_t i : sel) r.kill.push_back(i);
    }
  }
  return r;
}

}                                  
