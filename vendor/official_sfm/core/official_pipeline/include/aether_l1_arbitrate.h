                                                                           
                                                                          
                                                                         
                                                                                 
                                                               
  
                                                                        
                                                                         
                                                                        
                                                                           
                                                                           
                                                                    
                                                                    
                                                                     
                                                                          
                                                                           
                                                                 
  
                                                                          
                                                                              
                                                                         
                                                            
                                                                          
                                                                         
                                                                     
                                                                            
                                                                         
                                                                         
                                                                            
                                                                           
                                                                     
                                                                   
                                                                         
                                                                        
                                                                            
                                                                        
                                                                       
                                                                          
                                                                
                                                                         
                                                                          
                                                                               
                                                                      
                            
                                                               
                                                               
                                                                      
  
                                                                           
                                                                            
                                                                         
                                                                         
                                                                              
                                                                             
                                                                    
                                                                     
                                                                        
                                                                            
                                            
                                                                             
                                                                
  
                                                                            
                                                                              
                                                                             
                                                                   
                                                                               
                                                     
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "aether_ghost_mask.h"
#include "aether_l1_plan.h"

namespace aether_l1 {

                                                                         
                                          
constexpr double kTauSup = 0.010;
constexpr double kTauRej = 0.020;
constexpr int kVoteMin = 2;
constexpr int kSeeTol = 0;
constexpr double kFloorTol = 0.010;
constexpr double kNnTol = 0.010;
                                                                    
constexpr int kConvSeeMin = 3;                                              
constexpr double kConvSparseMargin = 0.025;                           
constexpr double kFloorLooseTol = 0.015;                                  
constexpr double kNnVetoTol = 0.015;                                     
constexpr float kConfMin = 0.3f;                                       
constexpr double kDatumWin = 0.06;                                         
constexpr int kMinCellDense = 50;                                         
constexpr double kModeWin = 0.010;                             
constexpr double kMode2Gap = 0.015;                                
constexpr double kHistBin = 0.005;                         
constexpr double kBelowTol = 0.010;                                
constexpr int kCtrlMin = 50;                                                  
constexpr double kNnVoxel = 0.010;                                            
                                                                          
                                                                            
                                                                        
constexpr double kGeoPix = 1.0;
constexpr double kGeoDepRel = 0.01;
constexpr int kGeoN = 1;
                                                                          
                                                                        
                                                                          
                                                                           
                                                                           
                                                                        
constexpr double kCellTrustTol = 0.010;
                                                                             
                                                                              
                                                                            
                                                                              
constexpr double kEvidVoxel = 0.005;

struct ArbStats {
  int n_refs_planned = 0;
  int n_refs_used = 0;                                           
  int n_region = 0;
  int n_band = 0;
  int n_rescue = 0;
  int n_confirm = 0;
  int n_abstain = 0;
  int n_confirm_see = 0;
  int n_confirm_below = 0;
  int n_ray_ok = 0;                                             
  int n_floor_ok = 0;                                       
  int n_struct_ok = 0;                                       
  int n_below_floor = 0;
  int n_mirror_hit = 0;
  int n_lev_cells = 0;
  int n_lev2_cells = 0;
  int n_ctrl_floor = 0;                                 
  int n_ctrl_ray = 0;                                 
  double delta_mm = 0.0;                   
  double delta_ray_mm = 0.0;                      
  bool degraded = false;                                           
  double pass_ms = 0.0;
};

namespace detail_arb {

struct PlanRefBin {
  int32_t frame_id = -1;
  double k[4] = {0};
  double w2c[16] = {0};
};

inline bool ReadAll(const std::string& path, std::vector<uint8_t>* out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::fseek(f, 0, SEEK_END);
  const long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (sz <= 0) {
    std::fclose(f);
    return false;
  }
  out->resize(static_cast<size_t>(sz));
  const size_t r = std::fread(out->data(), 1, out->size(), f);
  std::fclose(f);
  return r == out->size();
}

template <typename T>
inline bool Pull(const std::vector<uint8_t>& b, size_t* off, T* out,
                 size_t count = 1) {
  const size_t bytes = sizeof(T) * count;
  if (*off + bytes > b.size()) return false;
  std::memcpy(out, b.data() + *off, bytes);
  *off += bytes;
  return true;
}

                                                                 
inline int64_t VoxKeyAt(double x, double y, double z, double vox) {
  const int64_t vx = static_cast<int64_t>(std::floor(x / vox));
  const int64_t vy = static_cast<int64_t>(std::floor(y / vox));
  const int64_t vz = static_cast<int64_t>(std::floor(z / vox));
                                                                  
  return ((vx + (1ll << 20)) << 42) | ((vy + (1ll << 20)) << 21) |
         (vz + (1ll << 20));
}
inline int64_t VoxKey(double x, double y, double z) {
  return VoxKeyAt(x, y, z, kNnVoxel);
}

}                         

inline bool RunArbitration(const std::string& db_dir, ArbStats* st,
                           std::string* err) {
  using aether_ghost::detail::Arange;
  using aether_ghost::detail::Histogram;
  using aether_ghost::detail::Median1D;
  const auto fail = [&](const char* m) {
    if (err) *err = m;
    return false;
  };

                                                                                                                                                                                                       
  std::vector<uint8_t> planb;
  if (!detail_arb::ReadAll(db_dir + "/arbitration_plan.bin", &planb))
    return fail("arbitration_plan.bin missing/unreadable");
  size_t off = 0;
  uint32_t magic = 0, ver = 0, H = 0, W = 0, n_refs = 0;
  double plane_n[3] = {0}, plane_d = 0;
  if (!detail_arb::Pull(planb, &off, &magic) || magic != 0x314C3341u)
    return fail("plan magic mismatch");
  if (!detail_arb::Pull(planb, &off, &ver) || ver != 1u)
    return fail("plan version mismatch");
  if (!detail_arb::Pull(planb, &off, plane_n, 3) ||
      !detail_arb::Pull(planb, &off, &plane_d) ||
      !detail_arb::Pull(planb, &off, &H) ||
      !detail_arb::Pull(planb, &off, &W) ||
      !detail_arb::Pull(planb, &off, &n_refs))
    return fail("plan truncated");
  std::vector<detail_arb::PlanRefBin> refs(n_refs);
  for (uint32_t r = 0; r < n_refs; ++r) {
    if (!detail_arb::Pull(planb, &off, &refs[r].frame_id) ||
        !detail_arb::Pull(planb, &off, refs[r].k, 4) ||
        !detail_arb::Pull(planb, &off, refs[r].w2c, 16))
      return fail("plan ref truncated");
  }
  st->n_refs_planned = static_cast<int>(n_refs);
  if (n_refs == 0) return fail("plan has no refs");

  std::vector<uint8_t> ptsb;
  if (!detail_arb::ReadAll(db_dir + "/arbitration_points.bin", &ptsb))
    return fail("arbitration_points.bin missing/unreadable");
  off = 0;
  uint32_t pmagic = 0, pver = 0, n = 0;
  if (!detail_arb::Pull(ptsb, &off, &pmagic) || pmagic != 0x504C3341u ||
      !detail_arb::Pull(ptsb, &off, &pver) || pver != 1u ||
      !detail_arb::Pull(ptsb, &off, &n) || n == 0)
    return fail("points sidecar header invalid");
  std::vector<uint32_t> flag_idx(n);
  std::vector<float> xyz(static_cast<size_t>(n) * 3);
  std::vector<double> sd(n), local_off(n);
  if (!detail_arb::Pull(ptsb, &off, flag_idx.data(), n) ||
      !detail_arb::Pull(ptsb, &off, xyz.data(), static_cast<size_t>(n) * 3) ||
      !detail_arb::Pull(ptsb, &off, sd.data(), n) ||
      !detail_arb::Pull(ptsb, &off, local_off.data(), n))
    return fail("points sidecar truncated");
  st->n_region = static_cast<int>(n);

  std::vector<uint8_t> mask;
  if (!detail_arb::ReadAll(db_dir + "/ghost_mask.bin", &mask))
    return fail("ghost_mask.bin missing/unreadable");
  for (const uint32_t fi : flag_idx)
    if (fi >= mask.size()) return fail("flag index out of mask range");

  std::vector<uint8_t> band(n), cellghost(n);
  for (uint32_t i = 0; i < n; ++i) {
    band[i] = (mask[flag_idx[i]] & aether_ghost::kFlagBand15) ? 1 : 0;
    cellghost[i] = (mask[flag_idx[i]] & aether_ghost::kFlagCellGhost) ? 1 : 0;
  }
  int n_band = 0;
  for (uint32_t i = 0; i < n; ++i) n_band += band[i];
  st->n_band = n_band;

                                                             
  std::vector<int64_t> ckey(n);
  for (uint32_t i = 0; i < n; ++i)
    ckey[i] = aether_ghost::detail::CellKey(
        static_cast<double>(xyz[static_cast<size_t>(i) * 3]),
        static_cast<double>(xyz[static_cast<size_t>(i) * 3 + 2]));

                                                                           
  std::unordered_map<int64_t, std::vector<uint32_t>> cand_of_vox;
  std::unordered_set<int64_t> vox_accept;
  for (uint32_t i = 0; i < n; ++i) {
    if (!band[i]) continue;
    const double px = xyz[static_cast<size_t>(i) * 3];
    const double py = xyz[static_cast<size_t>(i) * 3 + 1];
    const double pz = xyz[static_cast<size_t>(i) * 3 + 2];
    cand_of_vox[detail_arb::VoxKey(px, py, pz)].push_back(i);
    for (int dx = -1; dx <= 1; ++dx)
      for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
          vox_accept.insert(detail_arb::VoxKey(px + dx * kNnVoxel,
                                               py + dy * kNnVoxel,
                                               pz + dz * kNnVoxel));
  }

                                                                                           
  const size_t hw = static_cast<size_t>(H) * W;
  struct RefMap {
    int32_t frame_id = -1;
    const double* k = nullptr;                  
    const double* w2c = nullptr;                  
    std::vector<float> depth;           
    std::vector<uint8_t> ok;                                                 
    std::vector<uint8_t> okc;                                                
                                                                             
                                                                                
                                                                            
                                                                         
  };
  std::vector<RefMap> maps;
  {
    std::vector<uint8_t> raw;
    for (uint32_t r = 0; r < n_refs; ++r) {
      char name[64];
      std::snprintf(name, sizeof(name), "/l1_depth_%d.bin", refs[r].frame_id);
      if (!detail_arb::ReadAll(db_dir + name, &raw)) continue;
      size_t doff = 0;
      char dm[4];
      uint32_t dver = 0, dH = 0, dW = 0;
      int32_t dfid = -1;
      if (!detail_arb::Pull(raw, &doff, dm, 4) ||
          std::memcmp(dm, "L1DP", 4) != 0 ||
          !detail_arb::Pull(raw, &doff, &dver) || dver != 1u ||
          !detail_arb::Pull(raw, &doff, &dfid) ||
          !detail_arb::Pull(raw, &doff, &dH) ||
          !detail_arb::Pull(raw, &doff, &dW) || dH != H || dW != W ||
          dfid != refs[r].frame_id || raw.size() < doff + hw * 8)
        continue;                                               
      RefMap m;
      m.frame_id = refs[r].frame_id;
      m.k = refs[r].k;
      m.w2c = refs[r].w2c;
      m.depth.resize(hw);
      std::memcpy(m.depth.data(), raw.data() + doff, hw * 4);
      const float* conf = reinterpret_cast<const float*>(raw.data() + doff) +
                          hw;
      m.ok.resize(hw);
      for (size_t p = 0; p < hw; ++p)
        m.ok[p] = (m.depth[p] > 0.0f && conf[p] > kConfMin) ? 1 : 0;
      m.okc = m.ok;
      maps.push_back(std::move(m));
    }
  }
  const int n_maps = static_cast<int>(maps.size());
  st->n_refs_used = n_maps;
  if (n_maps == 0) return fail("no usable l1_depth_*.bin found");
  std::vector<int> ref_used_ids;
  for (const RefMap& m : maps) ref_used_ids.push_back(m.frame_id);

                                              
  const auto cam2world = [](const double* w2c, double xc, double yc,
                            double zc, double o[3]) {
    const double a = xc - w2c[3], b = yc - w2c[7], c = zc - w2c[11];
    o[0] = w2c[0] * a + w2c[4] * b + w2c[8] * c;
    o[1] = w2c[1] * a + w2c[5] * b + w2c[9] * c;
    o[2] = w2c[2] * a + w2c[6] * b + w2c[10] * c;
  };
  const auto world2cam = [](const double* w2c, const double p[3],
                            double o[3]) {
    o[0] = w2c[0] * p[0] + w2c[1] * p[1] + w2c[2] * p[2] + w2c[3];
    o[1] = w2c[4] * p[0] + w2c[5] * p[1] + w2c[6] * p[2] + w2c[7];
    o[2] = w2c[8] * p[0] + w2c[9] * p[1] + w2c[10] * p[2] + w2c[11];
  };

                                                                       
                                                                         
                                                                      
                                                                          
                                                                           
                                                                         
                                                                           
  {
    std::vector<std::vector<uint8_t>> geo_ok(maps.size());
    for (size_t a = 0; a < maps.size(); ++a) {
      const RefMap& A = maps[a];
      geo_ok[a].assign(hw, 0);
      const double fxA = A.k[0], fyA = A.k[1], cxA = A.k[2], cyA = A.k[3];
      for (uint32_t v = 0; v < H; ++v) {
        for (uint32_t u = 0; u < W; ++u) {
          const size_t px = static_cast<size_t>(v) * W + u;
          if (!A.ok[px]) continue;
          const double zd = static_cast<double>(A.depth[px]);
          double Pw[3];
          cam2world(A.w2c, (static_cast<double>(u) - cxA) / fxA * zd,
                    (static_cast<double>(v) - cyA) / fyA * zd, zd, Pw);
          int agree = 0;
          for (size_t b = 0; b < maps.size() && agree < kGeoN; ++b) {
            if (b == a) continue;
            const RefMap& B = maps[b];
            double pb[3];
            world2cam(B.w2c, Pw, pb);
            if (pb[2] <= 0.05) continue;
            const double ub = B.k[0] * pb[0] / pb[2] + B.k[2];
            const double vb = B.k[1] * pb[1] / pb[2] + B.k[3];
            const long ubi = static_cast<long>(std::rint(ub));
            const long vbi = static_cast<long>(std::rint(vb));
            if (ubi < 0 || ubi >= static_cast<long>(W) || vbi < 0 ||
                vbi >= static_cast<long>(H))
              continue;
            const size_t pxb = static_cast<size_t>(vbi) * W + ubi;
            const float dB = B.depth[pxb];
            if (!(dB > 0.0f)) continue;
            double Pw2[3];
            cam2world(B.w2c,
                      (static_cast<double>(ubi) - B.k[2]) / B.k[0] * dB,
                      (static_cast<double>(vbi) - B.k[3]) / B.k[1] * dB,
                      static_cast<double>(dB), Pw2);
            double pa[3];
            world2cam(A.w2c, Pw2, pa);
            if (pa[2] <= 0.05) continue;
            const double ua2 = fxA * pa[0] / pa[2] + cxA;
            const double va2 = fyA * pa[1] / pa[2] + cyA;
            const double du = ua2 - static_cast<double>(u);
            const double dvv = va2 - static_cast<double>(v);
            if (du * du + dvv * dvv >= kGeoPix * kGeoPix) continue;
            if (std::fabs(pa[2] - zd) / zd >= kGeoDepRel) continue;
            ++agree;
          }
          geo_ok[a][px] = agree >= kGeoN ? 1 : 0;
        }
      }
    }
    for (size_t a = 0; a < maps.size(); ++a)
      for (size_t p = 0; p < hw; ++p) maps[a].ok[p] &= geo_ok[a][p];
  }

                                                                                                                                             
                                                                            
                                                                             
                                                                       
                                                                           
                                                                           
  const float nanf = std::numeric_limits<float>::quiet_NaN();
  std::vector<float> hres(static_cast<size_t>(n) * n_maps, nanf);
  struct VoxAcc {
    double sx = 0, sy = 0, sz = 0;
    int cnt = 0;
  };
  std::unordered_map<int64_t, VoxAcc> vox_geo, vox_conf;
  for (int r = 0; r < n_maps; ++r) {
    const RefMap& M = maps[static_cast<size_t>(r)];
    const float* depth = M.depth.data();
    const uint8_t* ok = M.ok.data();
    const uint8_t* okc = M.okc.data();
    const double fx = M.k[0], fy = M.k[1], cx = M.k[2], cy = M.k[3];
    const double* w2c = M.w2c;

                                                 
    for (uint32_t v = 0; v < H; ++v) {
      for (uint32_t u = 0; u < W; ++u) {
        const size_t px = static_cast<size_t>(v) * W + u;
        if (!okc[px]) continue;
        const double zd = static_cast<double>(depth[px]);
        double Pw[3];
        cam2world(w2c, (static_cast<double>(u) - cx) / fx * zd,
                  (static_cast<double>(v) - cy) / fy * zd, zd, Pw);
        const double sdd =
            Pw[0] * plane_n[0] + Pw[1] * plane_n[1] + Pw[2] * plane_n[2] +
            plane_d;
        if (ok[px] && std::fabs(sdd) < aether_ghost::kSlab) {
          VoxAcc& a = vox_geo[detail_arb::VoxKeyAt(Pw[0], Pw[1], Pw[2],
                                                   kEvidVoxel)];
          a.sx += Pw[0];
          a.sy += Pw[1];
          a.sz += Pw[2];
          ++a.cnt;
        }
        if (std::fabs(sdd) < 0.10 &&
            vox_accept.count(detail_arb::VoxKey(Pw[0], Pw[1], Pw[2]))) {
          VoxAcc& a = vox_conf[detail_arb::VoxKeyAt(Pw[0], Pw[1], Pw[2],
                                                    kEvidVoxel)];
          a.sx += Pw[0];
          a.sy += Pw[1];
          a.sz += Pw[2];
          ++a.cnt;
        }
      }
    }

                                                                    
    for (uint32_t i = 0; i < n; ++i) {
      const double P[3] = {
          static_cast<double>(xyz[static_cast<size_t>(i) * 3]),
          static_cast<double>(xyz[static_cast<size_t>(i) * 3 + 1]),
          static_cast<double>(xyz[static_cast<size_t>(i) * 3 + 2])};
      double pc[3];
      world2cam(w2c, P, pc);
      if (pc[2] <= 0.05) continue;
      const double u = fx * pc[0] / pc[2] + cx;
      const double v = fy * pc[1] / pc[2] + cy;
      const long ui = static_cast<long>(std::rint(u));                        
      const long vi = static_cast<long>(std::rint(v));
      if (ui < 0 || ui >= static_cast<long>(W) || vi < 0 ||
          vi >= static_cast<long>(H))
        continue;
      const long uc = std::min(std::max(ui, 1l), static_cast<long>(W) - 2);
      const long vc = std::min(std::max(vi, 1l), static_cast<long>(H) - 2);
      double patch[9];
      int nv = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          const size_t px = static_cast<size_t>(vc + dy) * W + (uc + dx);
          if (ok[px]) patch[nv++] = static_cast<double>(depth[px]);
        }
      }
      if (nv < 3) continue;
      std::sort(patch, patch + nv);
      const double dmv = (nv % 2 == 1)
                             ? patch[nv / 2]
                             : (patch[nv / 2 - 1] + patch[nv / 2]) / 2.0;
      if (!(dmv > 0.0)) continue;
                                                                            
      double Pd[3];
      cam2world(w2c, (static_cast<double>(uc) - cx) / fx * dmv,
                (static_cast<double>(vc) - cy) / fy * dmv, dmv, Pd);
      const double h = (P[0] - Pd[0]) * plane_n[0] +
                       (P[1] - Pd[1]) * plane_n[1] +
                       (P[2] - Pd[2]) * plane_n[2];
      hres[static_cast<size_t>(i) * n_maps + r] = static_cast<float>(h);
    }
  }
  maps.clear();
  maps.shrink_to_fit();

                                                                           
  std::unordered_map<int64_t, std::vector<double>> cellvals;                 
  struct DenseNear {                                         
    float x, y, z, sd;
    int64_t cell;
  };
  std::vector<DenseNear> dense_near;
  for (const auto& [key, a] : vox_geo) {
    const double px = a.sx / a.cnt, py = a.sy / a.cnt, pz = a.sz / a.cnt;
    const double sdd =
        px * plane_n[0] + py * plane_n[1] + pz * plane_n[2] + plane_d;
    if (std::fabs(sdd) < aether_ghost::kSlab)
      cellvals[aether_ghost::detail::CellKey(px, pz)].push_back(sdd);
  }
  dense_near.reserve(vox_conf.size());
  for (const auto& [key, a] : vox_conf) {
    const double px = a.sx / a.cnt, py = a.sy / a.cnt, pz = a.sz / a.cnt;
    const double sdd =
        px * plane_n[0] + py * plane_n[1] + pz * plane_n[2] + plane_d;
    dense_near.push_back({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(pz), static_cast<float>(sdd),
                          aether_ghost::detail::CellKey(px, pz)});
  }
  vox_geo.clear();
  vox_conf.clear();

                                                                                   
  const std::vector<double> edges =
      Arange(-aether_ghost::kSlab, aether_ghost::kSlab + kHistBin, kHistBin);
  std::unordered_map<int64_t, double> lev, lev2;
  for (auto& [key, v] : cellvals) {
    if (v.size() < static_cast<size_t>(kMinCellDense)) continue;
    const std::vector<int64_t> h = Histogram(v, edges);
    int64_t hmax = 0;
    for (const int64_t c : h) hmax = std::max(hmax, c);
    if (hmax == 0) continue;
    const double thr = std::max(1.0, 0.3 * static_cast<double>(hmax));
    std::vector<size_t> cand;
    for (size_t j = 0; j < h.size(); ++j)
      if (static_cast<double>(h[j]) >= thr) cand.push_back(j);
    if (cand.empty()) continue;
    size_t mi = 0;
    double mabs = std::numeric_limits<double>::infinity();
    std::vector<double> centers(cand.size());
    for (size_t j = 0; j < cand.size(); ++j) {
      centers[j] = (edges[cand[j]] + edges[cand[j] + 1]) / 2.0;
      if (std::fabs(centers[j]) < mabs) {
        mabs = std::fabs(centers[j]);
        mi = j;
      }
    }
    const double m = centers[mi];
    std::vector<double> sel;
    for (const double x : v)
      if (std::fabs(x - m) <= kModeWin) sel.push_back(x);
    if (sel.size() < static_cast<size_t>(kMinCellDense) / 2) continue;
    lev[key] = Median1D(sel);
                                                                            
    size_t fb = 0;
    int64_t fbh = -1;
    for (size_t j = 0; j < cand.size(); ++j) {
      if (std::fabs(centers[j] - m) < kMode2Gap) continue;
      if (h[cand[j]] > fbh) {
        fbh = h[cand[j]];
        fb = j;
      }
    }
    if (fbh >= 0) {
      const double m2 = centers[fb];
      std::vector<double> sel2;
      for (const double x : v)
        if (std::fabs(x - m2) <= kModeWin) sel2.push_back(x);
      if (sel2.size() >= static_cast<size_t>(kMinCellDense) / 2)
        lev2[key] = Median1D(sel2);
    }
  }
  st->n_lev_cells = static_cast<int>(lev.size());
  st->n_lev2_cells = static_cast<int>(lev2.size());
  cellvals.clear();

                                                                                                     
  std::vector<float> nn2(n, std::numeric_limits<float>::infinity());
  for (const auto& dp : dense_near) {
    const auto lv = lev.find(dp.cell);
    if (lv != lev.end() && !(static_cast<double>(dp.sd) >= lv->second - 0.005))
      continue;                                                              
    for (int dx = -1; dx <= 1; ++dx) {
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dz = -1; dz <= 1; ++dz) {
          const auto it = cand_of_vox.find(detail_arb::VoxKey(
              dp.x + dx * kNnVoxel, dp.y + dy * kNnVoxel,
              dp.z + dz * kNnVoxel));
          if (it == cand_of_vox.end()) continue;
          for (const uint32_t ci : it->second) {
            const float ddx = xyz[static_cast<size_t>(ci) * 3] - dp.x;
            const float ddy = xyz[static_cast<size_t>(ci) * 3 + 1] - dp.y;
            const float ddz = xyz[static_cast<size_t>(ci) * 3 + 2] - dp.z;
            const float d2 = ddx * ddx + ddy * ddy + ddz * ddz;
            if (d2 < nn2[ci]) nn2[ci] = d2;
          }
        }
      }
    }
  }
  dense_near.clear();
  dense_near.shrink_to_fit();

                                                                                                                                   
  std::vector<double> levP(n, std::numeric_limits<double>::quiet_NaN());
  std::vector<double> lev2P(n, std::numeric_limits<double>::quiet_NaN());
  for (uint32_t i = 0; i < n; ++i) {
    const auto lv = lev.find(ckey[i]);
    if (lv != lev.end()) levP[i] = lv->second;
    const auto l2 = lev2.find(ckey[i]);
    if (l2 != lev2.end()) lev2P[i] = l2->second;
  }
  std::vector<double> mean_h(n, std::numeric_limits<double>::quiet_NaN());
  std::vector<int> h_cnt(n, 0);
  for (uint32_t i = 0; i < n; ++i) {
    double s = 0.0;
    int c = 0;
    for (int r = 0; r < n_maps; ++r) {
      const float h = hres[static_cast<size_t>(i) * n_maps + r];
      if (std::isnan(h) || !(std::fabs(static_cast<double>(h)) < kDatumWin))
        continue;
      s += static_cast<double>(h);
      ++c;
    }
    h_cnt[i] = c;
    if (c > 0) mean_h[i] = s / c;
  }
  std::vector<double> ctrl_r, ctrl_h;
  for (uint32_t i = 0; i < n; ++i) {
    if (band[i]) continue;                                            
    if (!std::isnan(levP[i])) {
      ctrl_r.push_back(sd[i] - levP[i]);
      if (h_cnt[i] >= 2) ctrl_h.push_back(mean_h[i]);
    }
  }
  st->n_ctrl_floor = static_cast<int>(ctrl_r.size());
  st->n_ctrl_ray = static_cast<int>(ctrl_h.size());
  double delta = 0.0, delta_ray = 0.0;
  if (static_cast<int>(ctrl_r.size()) < kCtrlMin ||
      static_cast<int>(ctrl_h.size()) < kCtrlMin) {
    st->degraded = true;                                                  
  } else {
    delta = Median1D(ctrl_r);
    delta_ray = Median1D(ctrl_h);
  }
  st->delta_mm = delta * 1000.0;
  st->delta_ray_mm = delta_ray * 1000.0;

                                                                                                                                             
  const double nn_tol2 = kNnTol * kNnTol;
  std::vector<uint8_t> rescued(n, 0), confirmed(n, 0);
                                                                                 
                                                                        
                                                                           
                                                       
  const bool dump = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_L1_DUMP");
    return e && e[0] == '1';
  }();
  std::vector<uint8_t> dump_buf;
  if (dump) dump_buf.resize(static_cast<size_t>(n) * 20, 0);
  if (!st->degraded) {
    for (uint32_t i = 0; i < n; ++i) {
      if (!band[i] && !dump) continue;
      int sup = 0, see = 0, below = 0;
      for (int r = 0; r < n_maps; ++r) {
        const float hf = hres[static_cast<size_t>(i) * n_maps + r];
        if (std::isnan(hf)) continue;
        const double rh = static_cast<double>(hf) - delta_ray;
        if (std::fabs(rh) <= kTauSup) ++sup;
        if (rh >= kTauRej) ++see;
        if (rh <= -kTauRej) ++below;
      }
      const bool has_lev = !std::isnan(levP[i]);
      const double rp = has_lev ? sd[i] - levP[i] - delta : 0.0;
      const bool floor_strict = has_lev && std::fabs(rp) <= kFloorTol;
      const bool has_m2 = !std::isnan(lev2P[i]);
      const bool m2_above = has_m2 && has_lev && lev2P[i] > levP[i];
      const bool on_m2 =
          has_m2 && std::fabs(sd[i] - lev2P[i] - delta) <= kModeWin;
      const bool on_m2_above = on_m2 && m2_above;
      const bool mirror_hit = on_m2 && !m2_above;
      const bool below_floor = has_lev ? (rp < -kBelowTol)
                                       : (sd[i] - local_off[i] < -kBelowTol);
      const bool struct_ok = static_cast<double>(nn2[i]) <= nn_tol2;
      const bool floor_ok = floor_strict || on_m2_above;
      const bool ray_ok = sup >= kVoteMin && see <= kSeeTol;
                                                                            
                                                                        
                                                                            
                                                                      
                                                      
      const bool cell_trusted =
          has_lev && std::fabs(local_off[i] - levP[i] - delta) <= kCellTrustTol;
      const bool resc = cell_trusted && ray_ok && (floor_ok || struct_ok) &&
                        !below_floor && !mirror_hit;
                                                                           
      const bool vouched = floor_ok ||
                           (has_lev && std::fabs(rp) <= kFloorLooseTol) ||
                           static_cast<double>(nn2[i]) <=
                               kNnVetoTol * kNnVetoTol;
                                                                       
      const bool conf_see =
          see >= kConvSeeMin && sup == 0 && !vouched &&
          std::fabs(sd[i] - local_off[i]) >= kConvSparseMargin;
      const bool conf = cell_trusted && !resc && conf_see;
      if (dump) {
        uint8_t* d = &dump_buf[static_cast<size_t>(i) * 20];
        d[0] = static_cast<uint8_t>(std::min(sup, 255));
        d[1] = static_cast<uint8_t>(std::min(see, 255));
        d[2] = static_cast<uint8_t>(std::min(below, 255));
        d[3] = static_cast<uint8_t>(
            (band[i] ? 1 : 0) | (has_lev ? 2 : 0) | (floor_ok ? 4 : 0) |
            (struct_ok ? 8 : 0) | (below_floor ? 16 : 0) |
            (mirror_hit ? 32 : 0) | ((band[i] && resc) ? 64 : 0) |
            ((band[i] && conf) ? 128 : 0));
        const float frp = static_cast<float>(has_lev ? rp : std::nan(""));
        const float fnn = std::sqrt(nn2[i]);
        const float fmh = static_cast<float>(mean_h[i]);
        const float fcb = static_cast<float>(
            has_lev ? local_off[i] - levP[i] - delta : std::nan(""));
        std::memcpy(d + 4, &frp, 4);
        std::memcpy(d + 8, &fnn, 4);
        std::memcpy(d + 12, &fmh, 4);
        std::memcpy(d + 16, &fcb, 4);
      }
      if (!band[i]) continue;
      rescued[i] = resc ? 1 : 0;
      confirmed[i] = conf ? 1 : 0;
      st->n_ray_ok += ray_ok;
      st->n_floor_ok += floor_ok;
      st->n_struct_ok += struct_ok;
      st->n_below_floor += below_floor;
      st->n_mirror_hit += mirror_hit;
      st->n_rescue += resc;
      st->n_confirm += conf;
      st->n_confirm_see += conf;                                              
    }
  }
  if (dump)
    detail::WriteFileAtomic(db_dir + "/l1_debug.bin", dump_buf.data(),
                            dump_buf.size());
  st->n_abstain = st->n_band - st->n_rescue - st->n_confirm;

                                                                                                             
  for (uint32_t i = 0; i < n; ++i) {
    uint8_t& f = mask[flag_idx[i]];
    f &= static_cast<uint8_t>(
        ~(aether_ghost::kFlagL1Rescued | aether_ghost::kFlagL1Confirmed));
    if (rescued[i]) f |= aether_ghost::kFlagL1Rescued;
    if (confirmed[i]) f |= aether_ghost::kFlagL1Confirmed;
  }
  if (!detail::WriteFileAtomic(db_dir + "/ghost_mask.bin", mask.data(),
                               mask.size()))
    return fail("ghost_mask.bin rewrite failed");

  char jb[1536];
  std::string used_ids;
  for (size_t i = 0; i < ref_used_ids.size(); ++i) {
    if (i) used_ids += ",";
    char t[16];
    std::snprintf(t, sizeof(t), "%d", ref_used_ids[i]);
    used_ids += t;
  }
  const int jn = std::snprintf(
      jb, sizeof(jb),
      "{\"version\":1,\"refs_planned\":%d,\"refs_used\":%d,"
      "\"refs_used_ids\":[%s],\"n_region\":%d,\"n_band\":%d,"
      "\"rescue\":%d,\"confirm\":%d,\"abstain\":%d,"
      "\"confirm_see\":%d,\"confirm_below\":%d,"
      "\"ray_ok\":%d,\"floor_ok\":%d,\"struct_ok\":%d,"
      "\"below_floor\":%d,\"mirror_hit\":%d,"
      "\"lev_cells\":%d,\"lev2_cells\":%d,"
      "\"ctrl_floor\":%d,\"ctrl_ray\":%d,"
      "\"delta_mm\":%.3f,\"delta_ray_mm\":%.3f,\"degraded\":%s,"
      "\"policy\":\"abstain_visible\","
      "\"bits\":\"5:l1_rescued,6:l1_confirmed\"}\n",
      st->n_refs_planned, st->n_refs_used, used_ids.c_str(), st->n_region,
      st->n_band, st->n_rescue, st->n_confirm, st->n_abstain,
      st->n_confirm_see, st->n_confirm_below, st->n_ray_ok, st->n_floor_ok,
      st->n_struct_ok, st->n_below_floor, st->n_mirror_hit, st->n_lev_cells,
      st->n_lev2_cells, st->n_ctrl_floor, st->n_ctrl_ray, st->delta_mm,
      st->delta_ray_mm, st->degraded ? "true" : "false");
  if (jn > 0 && jn < static_cast<int>(sizeof(jb)))
    detail::WriteFileAtomic(db_dir + "/ghost_arbitration.json", jb,
                            static_cast<size_t>(jn));
  return true;
}

}                        
