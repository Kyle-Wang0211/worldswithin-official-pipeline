                                                                                         
  
                                                                                       
                                                                                        
                                                                                            
                                                                                
                                                                                             
                                                      
  
                                                     
                                                                                                       
                                                                                              
                                                                                            
                                                                                               
                                                                                                   
                                                                        
                                                                                               
                                                                                        
                                                                                
                                                                                               
  
                                                                                                    
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <unordered_map>
#include <vector>

namespace aether_scale_eps {

constexpr int kEpsSampleMax = 30000;                                 
constexpr int kEpsMaxRing = 32;                                                      
constexpr uint32_t kNoIndex = std::numeric_limits<uint32_t>::max();

struct P3 { float x, y, z; };

inline float dist2(const P3& a, const P3& b) {
  const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

                                                  
struct Grid {
  float cell;
  std::unordered_map<uint64_t, std::vector<uint32_t>> m;
  explicit Grid(float c) : cell(c) {}
  static uint64_t key(int ix, int iy, int iz) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(ix)) << 42) ^
           (static_cast<uint64_t>(static_cast<uint32_t>(iy)) << 21) ^
           static_cast<uint64_t>(static_cast<uint32_t>(iz));
  }
  std::array<int, 3> cellOf(const P3& p) const {
    return {static_cast<int>(std::floor(p.x / cell)),
            static_cast<int>(std::floor(p.y / cell)),
            static_cast<int>(std::floor(p.z / cell))};
  }
  void add(const P3& p, uint32_t idx) {
    const auto c = cellOf(p);
    m[key(c[0], c[1], c[2])].push_back(idx);
  }
                                             
  template <typename F>
  void forNeighbors(const P3& p, F&& f) const {
    const auto c = cellOf(p);
    for (int dx = -1; dx <= 1; ++dx)
      for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz) {
          const auto it = m.find(key(c[0] + dx, c[1] + dy, c[2] + dz));
          if (it == m.end()) continue;
          for (uint32_t idx : it->second) f(idx);
        }
  }
};

                                                                                               
                                                                                            
                                                                                          
inline uint32_t NearestIndexed(const Grid& g, const std::vector<P3>& pts,
                               const P3& q, float* out_d2,
                               uint32_t exclude = kNoIndex,
                               int max_ring = kEpsMaxRing) {
  const auto c = g.cellOf(q);
  float best = -1.0f;
  uint32_t best_i = kNoIndex;
  for (int R = 0; R <= max_ring; ++R) {
    for (int ax = -R; ax <= R; ++ax)
      for (int ay = -R; ay <= R; ++ay)
        for (int az = -R; az <= R; ++az) {
                                                                                    
          if (std::max(std::max(std::abs(ax), std::abs(ay)), std::abs(az)) != R)
            continue;
          const auto it = g.m.find(Grid::key(c[0] + ax, c[1] + ay, c[2] + az));
          if (it == g.m.end()) continue;
          for (uint32_t j : it->second) {
            if (j == exclude) continue;
            const float d2 = dist2(q, pts[j]);
            if (best < 0.0f || d2 < best) { best = d2; best_i = j; }
          }
        }
    const float bound = static_cast<float>(R) * g.cell;
    if (best >= 0.0f && best <= bound * bound) break;
  }
  if (out_d2) *out_d2 = best;
  return best_i;
}

                                                                                   
inline float NnP90(const std::vector<P3>& pts) {
  const size_t n = pts.size();
  if (n < 2) return 0.0f;
  float lo[3] = {pts[0].x, pts[0].y, pts[0].z};
  float hi[3] = {pts[0].x, pts[0].y, pts[0].z};
  for (const auto& p : pts) {
    lo[0] = std::min(lo[0], p.x); hi[0] = std::max(hi[0], p.x);
    lo[1] = std::min(lo[1], p.y); hi[1] = std::max(hi[1], p.y);
    lo[2] = std::min(lo[2], p.z); hi[2] = std::max(hi[2], p.z);
  }
  const float dx = hi[0] - lo[0], dy = hi[1] - lo[1], dz = hi[2] - lo[2];
  const float diag = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (!(diag > 0.0f)) return 0.0f;
  const float cell = diag / std::sqrt(static_cast<float>(n));
  if (!(cell > 0.0f)) return 0.0f;

  Grid g(cell);
  for (uint32_t i = 0; i < n; ++i) g.add(pts[i], i);

  const size_t stride = (n + kEpsSampleMax - 1) / kEpsSampleMax;
  std::vector<float> d;
  d.reserve(n / stride + 1);
  for (size_t i = 0; i < n; i += stride) {
    float best = -1.0f;
    NearestIndexed(g, pts, pts[i], &best, static_cast<uint32_t>(i));
    if (best < 0.0f) continue;                                                       
    d.push_back(std::sqrt(best));
  }
  if (d.empty()) return 0.0f;
  size_t k = static_cast<size_t>(0.9 * static_cast<double>(d.size()));
  if (k >= d.size()) k = d.size() - 1;
  std::nth_element(d.begin(), d.begin() + k, d.end());
  return d[k];
}

                                                                                    
inline float EnvMul(const char* name, float def) {
  const char* e = std::getenv(name);
  if (!e || !*e) return def;
  const float f = static_cast<float>(std::atof(e));
  return (f > 0.0f) ? f : def;
}

                                                         
inline int EnvInt(const char* name, int def) {
  const char* e = std::getenv(name);
  if (!e || !*e) return def;
  const int v = std::atoi(e);
  return (v > 0) ? v : def;
}

                                          
struct DSU {
  std::vector<uint32_t> p;
  explicit DSU(size_t n) : p(n) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(n); ++i) p[i] = i;
  }
  uint32_t find(uint32_t x) {
    while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; }
    return x;
  }
  void unite(uint32_t a, uint32_t b) {
    a = find(a); b = find(b);
    if (a != b) p[b] = a;
  }
};

}                               
