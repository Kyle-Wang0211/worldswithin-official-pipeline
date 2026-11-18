// pwofficial_gpu_match_dawn.cc — worldswithin cross-platform (Dawn/WGSL) GPU
// descriptor matcher: the ONE matcher implementation shared by iOS (Dawn →
// Metal), Android and HarmonyOS (Dawn → Vulkan). Production sibling of the
// shipped Metal TU pwofficial_gpu_match.mm; that TU is the SEMANTIC GOLD
// STANDARD and is not modified by this file (see the dispatch layer
// pwofficial_gpu_match_dispatch.cc for how the two coexist on iOS).
//

// Plain path kernel = the H2 campaign frontier `fusedr128-db` (SR-3D-1
// software-pipelined fused dual-direction subgroup-matrix kernel), taken
// VERBATIM from the WGSL text the harness
// aether_cpp/experiments/portable_frontend_pareto/tools/fair_match_portable_arm.cc
// assembles for kernel "fusedr128-db" (dumped through the harness's own
// assembly path, SHA-256 2e4a7d40b19346852d4fb085d292c8cb160f5c6d6e5b8eb5327662f7af09ae8f),
// with exactly two textual deltas for KNIFE-C chunking:
//   1. Params.pad0 is renamed rowBase and `let rb = U.rowBase + wg.x;`
//      replaces the implicit block index (rowBase == 0 reproduces the
//      frontier bit-for-bit);
//   2. column partials are indexed by the ABSOLUTE row block `rb`.
// That frontier was proven 696/696 byte-identical to native Metal on the two

// Fallback kernel = the harness's `tiled` kernel (dot4U8Packed, one query per
// thread, 64-row shared B tile; SHA-256 of the dumped text
// 1de3662a9afa4edfa2ea87a17507bbd876f74d1d1afea829c481539ce91feea3), same
// rowBase delta. Both kernels are gated by the parity suite (19 frozen
// goldens) and the 696-pair full gate before promotion.
//
// ── Semantics (identical to pwofficial_gpu_match.mm, see its header) ──────
// * best = MAX dot, LOWEST index wins ties; second = max of the remaining
//   multiset; every scan ascends with strict `>`; every merge level combines
//   an ordered lower-index chunk with a higher-index chunk keeping "ours" on
//   ties. Dots are exact integers (< 2^24) in f32 / u32.
// * plain gate (guideMode 0): angular domain, bd = acos(min(best/512², 1)),
//   sd likewise, keep iff bd <= maxDistance(0.7) && bd < maxRatio * sd.
// * guided gate (guideMode 1/2): v1 TWO-PASS structure (one direction per
//   dispatch, mirrored arguments), geometry gate BEFORE top-2 insertion:
//     mode 1 (E/F): symmetric epipolar residual — line2 = M·p1, line1 = Mᵀ·p2,
//                   nom = p2·line2, denom = |line2.xy|² + |line1.xy|²,
//                   accept iff denom > 1e-12 && nom² <= maxResidual·denom;
//     mode 2 (H):   reprojection — h = M·q, reject iff |hz| <= 1e-8, else
//                   accept iff |(hx/hz, hy/hz) − d|² <= maxResidual;
//   distance domain normalized L2 √(2 − 2cos) with the second-best floored
//   by the 131072 dot sentinel (= COLMAP's sentinel distance 512).
//   ⚠️ The guided gate is a near-cancellation and its float algebra is
//   compile-context dependent (pwofficial_gpu_match.mm header: 1,575
//   recombination hypotheses, ZERO bit-reproduce the Metal v1 gate).
//   Bit-parity with Metal is therefore NOT a gate for guided; the accepted
//   criterion is boundary-confined divergence (candidates within ~1e-3
//   relative of the residual threshold, ±1 match/pair scale) + downstream
//   losslessness. Do not "fix" the algebra to chase bits.
// * zero-padding invariant: both descriptor tables (and, when guided, both
//   keypoint tables) are padded to 128-row multiples, zero-filled, on the
//   host; padded rows produce dot == 0 candidates that can never become best
//   nor raise second under strict `>` against best/second initialised to 0.
// * mutual cross-check + ordered pair emission on the host, verbatim.
//
// ── Portable host driver (ported from the Metal TU, standard C++ only) ───
// * KNIFE-C chunked dispatch: env OFFICIAL_AETHER_MATCH_CHUNK_TARGET_MS
                                                                      
                                                                            
                                                                            
                                                                       
                                                                     
                                                                         
                                                                           
                                                                           
                                                                             
                                                                            
                                                                              
                                                                          
                                                                    
                                                                          
                                                                          
                                                                       
                                                              
                                                                           
                                                                           
                                                                        
                                                                             
                                                                           
                                                                           
                                                                             
                                          
                                                                            
                                                                           
                                                                       
                                                                         
                                                                       
                                                   
                                                              
                                                                  
                                                                           
                                                                             
                                                                    
  
                                                                                            
                                                                            
                                                                              
                                                                                 
                                                                          
                                                                     
                                                                             
                                                                         
                                                                           
                                                                            
                                                                      
                                                                
                                                                      
                                                                          
                                     
  
                                                                              
                                                                                                              
                                                                     
                                                                               
                                
                                                               
                                                                   
                                  
                                                                    
                                              
                                                                           
                                                                           
  
                                                                               
                                                                                
                                                                                    

#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <deque>
#include <vector>

#if __has_include("aether/sfm/descriptor_residency_policy_v1.h")
#include "aether/sfm/descriptor_residency_policy_v1.h"
#else
#include "../include/aether/sfm/descriptor_residency_policy_v1.h"
#endif

                                                                                                                                                          
#if defined(PWOFFICIAL_DAWN_OBSERVABLES_EXTERN) && PWOFFICIAL_DAWN_OBSERVABLES_EXTERN
extern "C" double aether_match_gpu_ms;
extern "C" double aether_match_sleep_ms;
extern "C" int aether_match_chunks;
#else
extern "C" double aether_match_gpu_ms = 0.0;
extern "C" double aether_match_sleep_ms = 0.0;
extern "C" int aether_match_chunks = 0;
#endif

                                                                                   
                                                                            
                               
extern "C" __attribute__((weak)) int pwofficial_platform_thermal_state(void);

namespace {

constexpr int kD = 128;
constexpr uint32_t kMmaRows = 128;                                       
constexpr uint32_t kBlockedRows = 64;                                              
constexpr uint32_t kTiledRows = 64;                                    
constexpr int32_t kOutSentinel = INT32_MIN;
constexpr uint64_t kSlot = 256;                                           

inline uint64_t RoundUp(uint64_t v, uint64_t m) { return (v + m - 1) / m * m; }

double NowMs() {
  using namespace std::chrono;
  return duration<double, std::milli>(steady_clock::now().time_since_epoch())
      .count();
}

std::string SV(wgpu::StringView v) {
  if (!v.data) return std::string();
  if (v.length == WGPU_STRLEN) return std::string(v.data);
  return std::string(v.data, v.length);
}

void Log(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  std::fprintf(stderr, "[pwofficial_gpu_match_dawn] %s\n", buf);
}

                                                                                                              
std::mutex gLastErrLock;
char gLastErr[192] = {0};

void StashLastError(const std::string& text) {
  std::lock_guard<std::mutex> lk(gLastErrLock);
  std::snprintf(gLastErr, sizeof(gLastErr), "%s", text.c_str());
}

                                                                                                                                                  
std::atomic<int> gCaptureActive{1};
std::atomic<int> gPreviewFps30{0};
std::atomic<int> gAbPhase{-1};
std::atomic<int> gThermalState{0};                                            

int PlatformThermalState() {
  if (pwofficial_platform_thermal_state != nullptr) {
    return pwofficial_platform_thermal_state();
  }
  return gThermalState.load(std::memory_order_relaxed);
}

                                                                                      
uint64_t CmdWaitTimeoutMs() {
  static const uint64_t v = [] {
    const char* e = getenv("OFFICIAL_AETHER_GPU_MATCH_WAIT_MS");
    if (e != nullptr) {
      const long long ms = atoll(e);
      if (ms > 0) return (uint64_t)ms;
    }
    return (uint64_t)30000;
  }();
  return v;
}

double AbAltValue(const char* key) {
  const char* e = getenv(key);
  return e ? atof(e) : -1.0;
}

                                                                       
                                                                                                         
                                                                                                            
                                                                                      
                                                                                              
                                                                                                             
                                                                                                 
                                                                                                
bool kColChunk = getenv("OFFICIAL_AETHER_MATCH_DAWN_ROWCHUNK") == nullptr;

double ChunkTargetMs() {
  static double v = -1.0;
  if (v < 0.0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_CHUNK_TARGET_MS");
    v = e ? atof(e) : 16.0;
    if (v < 0.0) v = 0.0;
  }
  if (gAbPhase.load(std::memory_order_relaxed) == 1) {
    static const double alt =
        AbAltValue("OFFICIAL_AETHER_MATCH_CHUNK_TARGET_MS_ALT");
    if (alt >= 0.0) return alt;
  }
  if (gPreviewFps30.load(std::memory_order_relaxed) == 1) {
    static double v30 = -1.0;
    if (v30 < 0.0) {
      const char* e30 = getenv("OFFICIAL_AETHER_MATCH_CHUNK_TARGET_MS_FPS30");
      v30 = e30 ? atof(e30) : 24.0;
      if (v30 < 0.0) v30 = 0.0;
    }
    return v30 > v ? v30 : v;
  }
  return v;
}

double ChunkTargetCoolMs() {
  static double v = -1.0;
  if (v < 0.0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_CHUNK_TARGET_COOL_MS");
    v = e ? atof(e) : 16.0;
    if (v < 0.0) v = 0.0;
  }
  const double hot = ChunkTargetMs();
  return v > hot ? v : hot;
}

bool ThermalHot() {
  if (gCaptureActive.load(std::memory_order_relaxed) == 0) return false;
  const int st = PlatformThermalState();
  return st >= 2;                                
}

double ThermalGapPct() {
  if (gCaptureActive.load(std::memory_order_relaxed) == 0) return 0.0;
  const int st = PlatformThermalState();
  if (st == 2) {
    static double v = -1.0;
    if (v < 0.0) {
      const char* e = getenv("OFFICIAL_AETHER_MATCH_GAP_SERIOUS_PCT");
      v = e ? atof(e) : 100.0;
      if (v < 0.0) v = 0.0;
    }
    if (gPreviewFps30.load(std::memory_order_relaxed) == 1) {
      static double v30 = -1.0;
      if (v30 < 0.0) {
        const char* e30 = getenv("OFFICIAL_AETHER_MATCH_GAP_SERIOUS_PCT_FPS30");
        v30 = e30 ? atof(e30) : -1.0;
      }
      return v30 >= 0.0 ? v30 : v * 0.5;
    }
    return v;
  }
  if (st >= 3) {
    static double v = -1.0;
    if (v < 0.0) {
      const char* e = getenv("OFFICIAL_AETHER_MATCH_GAP_CRITICAL_PCT");
      v = e ? atof(e) : 300.0;
      if (v < 0.0) v = 0.0;
    }
    return v;
  }
  return 0.0;
}

bool DescriptorResidencyEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_V1");
    return value && value[0] == '1' && value[1] == '\0';
  }();
  return enabled;
}

uint64_t DescriptorResidencyBudgetBytes() {
  static const uint64_t budget = [] {
    constexpr uint64_t kDefault = UINT64_C(48) * 1024 * 1024;
    const char* value = std::getenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_BYTES");
    if (!value || !value[0]) return kDefault;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (!end || *end != '\0') return kDefault;
    return static_cast<uint64_t>(parsed);
  }();
  return budget;
}

                                                                                                                                                                                                                  

                                                                             
                                                                              
                                                                         
                                                                       
                                                                                    
                                                                                               
                                                            
                                                                                  
                                                                                                      
                                                                                                     
                                                                                                          
                                                                                                               
                                                                                            
                                                                                                      
                                                                                      
                                                                                             
                                                                                   
                                                                                                      
                                                                                       
                                                                                 
                                                                                                    
                                                                 
constexpr char kWgslMmaMetalShape[] = R"WGSL(
enable chromium_experimental_subgroup_matrix;
enable subgroups;

alias Left = subgroup_matrix_left<f32, 8, 8>;
alias Right = subgroup_matrix_right<f32, 8, 8>;
alias Res = subgroup_matrix_result<f32, 8, 8>;

const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 128u;
const BT : u32 = 32u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,

  // (numA0 numB4 maxRatio8 maxDistance12 numWg16 rowBase20 colBase24 colSpan28)。
  colBase : u32,
  colSpan : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<f32>;
@group(0) @binding(1) var<storage, read> B : array<f32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;

// 只看到部分列,行向最优必须跨 dispatch 累加。**每个工作组独占自己那 128 行**
// ⇒ 无跨组竞争,读-改-写即可,不需要像 ColP 那样再来一趟归并。
@group(0) @binding(6) var<storage, read_write> RowP : array<ColPart>;

var<workgroup> Bsh : array<f32, 4096>; // 32 rows x 128 (16 KiB)
var<workgroup> accSh : array<f32, 4096>;   // WGR rows x 32 cols

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

var<workgroup> cpBest : array<f32, 512>;    // kSG(16) x BT(32)
var<workgroup> cpSecond : array<f32, 512>;
var<workgroup> cpIdx : array<i32, 512>;

@compute @workgroup_size(512)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32,
        @builtin(subgroup_id) sg : u32,
        @builtin(subgroup_invocation_id) lane : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let lrow = lane >> 2u;
  let lcg = lane & 3u;
  let gRow = row0 + sg * 8u + lrow;
  let aRow0 = row0 + sg * 8u;
  var aFrag : array<Left, 16>;
  for (var k = 0u; k < 16u; k = k + 1u) {
    aFrag[k] = subgroupMatrixLoad<Left>(&A, aRow0 * 128u + k * 8u, false, 128u);
  }
  var rowBest = 0.0;
  var rowSecond = 0.0;
  var rowBestI = -1;

  if (U.colBase != 0u && lcg == 0u && gRow < U.numA) {
    let rp = RowP[gRow];
    rowBest = rp.best;
    rowSecond = rp.second;
    rowBestI = rp.idx;
  }
  var col0 = U.colBase;
  let colEnd = min(U.colBase + U.colSpan, U.numB);
  loop {
    if (col0 >= colEnd) { break; }
    for (var e = lid; e < BT * 128u; e = e + 512u) {
      let brow = col0 + e / 128u;
      Bsh[e] = select(0.0, B[brow * 128u + (e % 128u)], brow < U.numB);
    }
    workgroupBarrier();


    // 原形态的内层是**16 次串行依赖的 MMA**(每次都等上一次的 acc);交换后变成
    // **4 条独立累加链**,载入次数 / MMA 次数 / store 次数**一次不变**,只多 3 个
    // 累加器(每 lane +6 个 32 位寄存器)。手写 Metal 台架实测 −3.9% ~ −5.8%。
    // 线索来自 llama.cpp 的 mul_mm(ma[4] × mb[2] → c_res[8],8 条独立链);
    // **原生出货核与我们原来一样是单链**,所以这一刀是超越而不是追平。
    var acc0 = Res(0.0);
    var acc1 = Res(0.0);
    var acc2 = Res(0.0);
    var acc3 = Res(0.0);
    for (var k = 0u; k < 16u; k = k + 1u) {
      let b0 = subgroupMatrixLoad<Right>(&Bsh, (0u * 8u) * 128u + k * 8u, true, 128u);
      let b1 = subgroupMatrixLoad<Right>(&Bsh, (1u * 8u) * 128u + k * 8u, true, 128u);
      let b2 = subgroupMatrixLoad<Right>(&Bsh, (2u * 8u) * 128u + k * 8u, true, 128u);
      let b3 = subgroupMatrixLoad<Right>(&Bsh, (3u * 8u) * 128u + k * 8u, true, 128u);
      acc0 = subgroupMatrixMultiplyAccumulate(aFrag[k], b0, acc0);
      acc1 = subgroupMatrixMultiplyAccumulate(aFrag[k], b1, acc1);
      acc2 = subgroupMatrixMultiplyAccumulate(aFrag[k], b2, acc2);
      acc3 = subgroupMatrixMultiplyAccumulate(aFrag[k], b3, acc3);
    }
    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 0u * 8u, acc0, false, 32u);
    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 1u * 8u, acc1, false, 32u);
    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 2u * 8u, acc2, false, 32u);
    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 3u * 8u, acc3, false, 32u);
    workgroupBarrier();

    var pb = 0.0;
    var ps = 0.0;
    var pbi = -1;
    let rbase = (sg * 8u + lrow) * 32u;
    for (var t = 0u; t < 8u; t = t + 1u) {
      let cLoc = lcg * 8u + t;
      let d = accSh[rbase + cLoc];
      if (d > pb) { ps = pb; pb = d; pbi = i32(col0 + cLoc); }
      else if (d > ps) { ps = d; }
    }
    {
      let ob = subgroupShuffleXor(pb, 1u);
      let os = subgroupShuffleXor(ps, 1u);
      let oi = subgroupShuffleXor(pbi, 1u);
      if (ob > pb) { ps = max(os, pb); pb = ob; pbi = oi; }
      else { ps = max(ps, ob); }
    }
    {
      let ob = subgroupShuffleXor(pb, 2u);
      let os = subgroupShuffleXor(ps, 2u);
      let oi = subgroupShuffleXor(pbi, 2u);
      if (ob > pb) { ps = max(os, pb); pb = ob; pbi = oi; }
      else { ps = max(ps, ob); }
    }
    if (lcg == 0u) {
      if (pb > rowBest) { rowSecond = max(rowBest, ps); rowBest = pb; rowBestI = pbi; }
      else { rowSecond = max(rowSecond, pb); }
    }

    var cb = 0.0;
    var cs = 0.0;
    var ci = -1;
    let cbase = sg * 8u * 32u + lane;
    for (var r = 0u; r < 8u; r = r + 1u) {
      let d = accSh[cbase + r * 32u];
      if (d > cb) { cs = cb; cb = d; ci = i32(row0 + sg * 8u + r); }
      else if (d > cs) { cs = d; }
    }
    cpBest[sg * 32u + lane] = cb;
    cpSecond[sg * 32u + lane] = cs;
    cpIdx[sg * 32u + lane] = ci;
    workgroupBarrier();


    // 「512 线程 × 每人 1 个 partial + 4 级蝶形」。
    // Metal v2 在这里用的是串行形态,并注明"并行 shuffle 树试过更慢——串行形态
    // 藏在其他线程的进度后面"。**那个前提在我们这里已经没了**:packed 上传把预取
    // 从 8 次迭代压到 2 次,480 个线程两拍就干完,归并再没东西可藏。
    // 分解实测(隔离台架,删掉本块):现役 6.555 → 4.982,这块值 ~1.5ms。
    // 映射:mcl = lid/16 是列(0..31),ms = lid%16 是 SG 下标;同一列的 16 个线程
    //   落在同一子组的同一半(偶数列 lane 0-15 / 奇数列 lane 16-31),掩码 1/2/4/8
    //   的蝶形不会跨出那一半。**全部 512 线程无条件执行 ⇒ 控制流对子组一致**,
    //   这正是 tint 允许 subgroupShuffleXor 的前提(条件分支里会被拒)。
    // 共享内存读总量不变(512 次 = 32 列 × 16 SG),没有冗余读。
    // 语义:ms 升序 == sg 升序 == 行号升序,`ob > b` 严格大于 ⇒ 平局留自己;
    //   逐级 xor 后 ms==0 那条恰好是"最小行号胜",与串行形态逐字等价。
    let mcl = lid / 16u;
    let ms = lid % 16u;
    var b = cpBest[ms * 32u + mcl];
    var s2 = cpSecond[ms * 32u + mcl];
    var bi = cpIdx[ms * 32u + mcl];
    {
      let ob = subgroupShuffleXor(b, 1u);
      let os = subgroupShuffleXor(s2, 1u);
      let oi = subgroupShuffleXor(bi, 1u);
      if (ob > b) { s2 = max(b, os); b = ob; bi = oi; }
      else { s2 = max(s2, ob); }
    }
    {
      let ob = subgroupShuffleXor(b, 2u);
      let os = subgroupShuffleXor(s2, 2u);
      let oi = subgroupShuffleXor(bi, 2u);
      if (ob > b) { s2 = max(b, os); b = ob; bi = oi; }
      else { s2 = max(s2, ob); }
    }
    {
      let ob = subgroupShuffleXor(b, 4u);
      let os = subgroupShuffleXor(s2, 4u);
      let oi = subgroupShuffleXor(bi, 4u);
      if (ob > b) { s2 = max(b, os); b = ob; bi = oi; }
      else { s2 = max(s2, ob); }
    }
    {
      let ob = subgroupShuffleXor(b, 8u);
      let os = subgroupShuffleXor(s2, 8u);
      let oi = subgroupShuffleXor(bi, 8u);
      if (ob > b) { s2 = max(b, os); b = ob; bi = oi; }
      else { s2 = max(s2, ob); }
    }
    let j = col0 + mcl;
    if (ms == 0u && j < U.numB) {
      ColP[rb * U.numB + j] = ColPart(b, s2, bi);
    }
    col0 = col0 + BT;
  }

  if (lcg == 0u && gRow < U.numA) {
    RowP[gRow] = ColPart(rowBest, rowSecond, rowBestI);
    OutAB[gRow] = gatef(rowBest, rowSecond, rowBestI);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[w * U.numB + c];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

                                                                                                               
                                                                                                             
                                                                                                       
                                                                                     
  
                                                                                                       
                                                                                                                     
                                                                                                           
                                                                                           
                                                                                                                  
  
                                                                                           
                                                                                                        
                                                                                                               
                                                                                                     
                                                                                                                      
                                                                                            
                                                                                      
constexpr char kWgslBlockedPlain[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 64u;
const BT : u32 = 64u;
const KC : u32 = 32u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  colBase : u32,
  colSpan : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<u32>;
@group(0) @binding(1) var<storage, read> B : array<u32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;
@group(0) @binding(6) var<storage, read_write> RowP : array<ColPart>;

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

var<workgroup> S : array<vec4<f32>, 1024>;

@compute @workgroup_size(256)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let tr = lid / 16u;
  let tc = lid % 16u;

  let myRow = row0 + lid;
  var rowBest = 0.0;
  var rowSecond = 0.0;
  var rowBestI = -1;
  if (U.colBase != 0u && lid < WGR && myRow < U.numA) {
    let rp = RowP[myRow];
    rowBest = rp.best;
    rowSecond = rp.second;
    rowBestI = rp.idx;
  }

  var col0 = U.colBase;
  let colEnd = min(U.colBase + U.colSpan, U.numB);
  loop {
    if (col0 >= colEnd) { break; }

    var acc0 = vec4<f32>(0.0);
    var acc1 = vec4<f32>(0.0);
    var acc2 = vec4<f32>(0.0);
    var acc3 = vec4<f32>(0.0);

    for (var kc = 0u; kc < 4u; kc = kc + 1u) {
      workgroupBarrier();
      // 暂存:线程 0..127 负责 A、128..255 负责 B;每线程取同一 w 下连续 4 行的
      // 4 个 u32,写出 4 个**完整** vec4(k = w*4+j,j=0..3)。不对分量做运行时索引写。
      {
        let side = lid / 128u;
        let t = lid % 128u;
        let w = t / 16u;
        let q = t % 16u;
        let base = q * 4u;
        var x0 = 0u; var x1 = 0u; var x2 = 0u; var x3 = 0u;
        if (side == 0u) {
          let g = row0 + base;
          x0 = select(0u, A[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numA);
          x1 = select(0u, A[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numA);
          x2 = select(0u, A[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numA);
          x3 = select(0u, A[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numA);
        } else {
          let g = col0 + base;
          x0 = select(0u, B[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numB);
          x1 = select(0u, B[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numB);
          x2 = select(0u, B[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numB);
          x3 = select(0u, B[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numB);
        }
        let o = side * 512u + (w * 4u) * 16u + q;
        S[o + 0u * 16u] = vec4<f32>(f32(x0 & 255u), f32(x1 & 255u), f32(x2 & 255u), f32(x3 & 255u));
        S[o + 1u * 16u] = vec4<f32>(f32((x0 >> 8u) & 255u), f32((x1 >> 8u) & 255u), f32((x2 >> 8u) & 255u), f32((x3 >> 8u) & 255u));
        S[o + 2u * 16u] = vec4<f32>(f32((x0 >> 16u) & 255u), f32((x1 >> 16u) & 255u), f32((x2 >> 16u) & 255u), f32((x3 >> 16u) & 255u));
        S[o + 3u * 16u] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));
      }
      workgroupBarrier();
      for (var k = 0u; k < KC; k = k + 1u) {
        let a4 = S[k * 16u + tr];
        let b4 = S[512u + k * 16u + tc];
        acc0 = acc0 + a4.x * b4;
        acc1 = acc1 + a4.y * b4;
        acc2 = acc2 + a4.z * b4;
        acc3 = acc3 + a4.w * b4;
      }
    }
    workgroupBarrier();
    S[(tr * 4u + 0u) * 16u + tc] = acc0;
    S[(tr * 4u + 1u) * 16u + tc] = acc1;
    S[(tr * 4u + 2u) * 16u + tc] = acc2;
    S[(tr * 4u + 3u) * 16u + tc] = acc3;
    workgroupBarrier();

    if (lid < WGR) {
      for (var v = 0u; v < 16u; v = v + 1u) {
        let d = S[lid * 16u + v];
        let c0 = i32(col0 + v * 4u);
        if (d.x > rowBest) { rowSecond = rowBest; rowBest = d.x; rowBestI = c0; }
        else if (d.x > rowSecond) { rowSecond = d.x; }
        if (d.y > rowBest) { rowSecond = rowBest; rowBest = d.y; rowBestI = c0 + 1; }
        else if (d.y > rowSecond) { rowSecond = d.y; }
        if (d.z > rowBest) { rowSecond = rowBest; rowBest = d.z; rowBestI = c0 + 2; }
        else if (d.z > rowSecond) { rowSecond = d.z; }
        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }
        else if (d.w > rowSecond) { rowSecond = d.w; }
      }
    } else if (lid < 2u * WGR) {
      let c = lid - WGR;
      let q = c / 4u;
      let m = c % 4u;
      var cb = 0.0;
      var cs = 0.0;
      var ci = -1;
      for (var r = 0u; r < WGR; r = r + 1u) {
        let d = S[r * 16u + q][m];
        if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }
        else if (d > cs) { cs = d; }
      }
      let gc = col0 + c;
      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }
    }
    col0 = col0 + BT;
  }

  if (lid < WGR && myRow < U.numA) {
    RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI);
    OutAB[myRow] = gatef(rowBest, rowSecond, rowBestI);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[w * U.numB + c];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

                                                                                         
                                                                                                           
constexpr char kWgslBlocked84[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 64u;
const BT : u32 = 64u;
const KC : u32 = 32u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  colBase : u32,
  colSpan : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<u32>;
@group(0) @binding(1) var<storage, read> B : array<u32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;
@group(0) @binding(6) var<storage, read_write> RowP : array<ColPart>;

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

var<workgroup> S : array<vec4<f32>, 1024>;

@compute @workgroup_size(128)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let tr = lid / 16u;
  let tc = lid % 16u;

  let myRow = row0 + lid;
  var rowBest = 0.0;
  var rowSecond = 0.0;
  var rowBestI = -1;
  if (U.colBase != 0u && lid < WGR && myRow < U.numA) {
    let rp = RowP[myRow];
    rowBest = rp.best;
    rowSecond = rp.second;
    rowBestI = rp.idx;
  }

  var col0 = U.colBase;
  let colEnd = min(U.colBase + U.colSpan, U.numB);
  loop {
    if (col0 >= colEnd) { break; }

    var acc0 = vec4<f32>(0.0);
    var acc1 = vec4<f32>(0.0);
    var acc2 = vec4<f32>(0.0);
    var acc3 = vec4<f32>(0.0);
    var acc4 = vec4<f32>(0.0);
    var acc5 = vec4<f32>(0.0);
    var acc6 = vec4<f32>(0.0);
    var acc7 = vec4<f32>(0.0);

    for (var kc = 0u; kc < 4u; kc = kc + 1u) {
      workgroupBarrier();
      // 暂存:线程 0..127 负责 A、128..255 负责 B;每线程取同一 w 下连续 4 行的
      // 4 个 u32,写出 4 个**完整** vec4(k = w*4+j,j=0..3)。不对分量做运行时索引写。
      for (var side = 0u; side < 2u; side = side + 1u) {
        let w = lid / 16u;
        let q = lid % 16u;
        let base = q * 4u;
        var x0 = 0u; var x1 = 0u; var x2 = 0u; var x3 = 0u;
        if (side == 0u) {
          let g = row0 + base;
          x0 = select(0u, A[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numA);
          x1 = select(0u, A[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numA);
          x2 = select(0u, A[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numA);
          x3 = select(0u, A[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numA);
        } else {
          let g = col0 + base;
          x0 = select(0u, B[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numB);
          x1 = select(0u, B[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numB);
          x2 = select(0u, B[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numB);
          x3 = select(0u, B[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numB);
        }
        let o = side * 512u + (w * 4u) * 16u + q;
        S[o + 0u * 16u] = vec4<f32>(f32(x0 & 255u), f32(x1 & 255u), f32(x2 & 255u), f32(x3 & 255u));
        S[o + 1u * 16u] = vec4<f32>(f32((x0 >> 8u) & 255u), f32((x1 >> 8u) & 255u), f32((x2 >> 8u) & 255u), f32((x3 >> 8u) & 255u));
        S[o + 2u * 16u] = vec4<f32>(f32((x0 >> 16u) & 255u), f32((x1 >> 16u) & 255u), f32((x2 >> 16u) & 255u), f32((x3 >> 16u) & 255u));
        S[o + 3u * 16u] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));
      }
      workgroupBarrier();
      for (var k = 0u; k < KC; k = k + 1u) {
        let al = S[k * 16u + tr * 2u];
        let ah = S[k * 16u + tr * 2u + 1u];
        let b4 = S[512u + k * 16u + tc];
        acc0 = acc0 + al.x * b4;
        acc1 = acc1 + al.y * b4;
        acc2 = acc2 + al.z * b4;
        acc3 = acc3 + al.w * b4;
        acc4 = acc4 + ah.x * b4;
        acc5 = acc5 + ah.y * b4;
        acc6 = acc6 + ah.z * b4;
        acc7 = acc7 + ah.w * b4;
      }
    }
    workgroupBarrier();
    S[(tr * 8u + 0u) * 16u + tc] = acc0;
    S[(tr * 8u + 1u) * 16u + tc] = acc1;
    S[(tr * 8u + 2u) * 16u + tc] = acc2;
    S[(tr * 8u + 3u) * 16u + tc] = acc3;
    S[(tr * 8u + 4u) * 16u + tc] = acc4;
    S[(tr * 8u + 5u) * 16u + tc] = acc5;
    S[(tr * 8u + 6u) * 16u + tc] = acc6;
    S[(tr * 8u + 7u) * 16u + tc] = acc7;
    workgroupBarrier();

    if (lid < WGR) {
      for (var v = 0u; v < 16u; v = v + 1u) {
        let d = S[lid * 16u + v];
        let c0 = i32(col0 + v * 4u);
        if (d.x > rowBest) { rowSecond = rowBest; rowBest = d.x; rowBestI = c0; }
        else if (d.x > rowSecond) { rowSecond = d.x; }
        if (d.y > rowBest) { rowSecond = rowBest; rowBest = d.y; rowBestI = c0 + 1; }
        else if (d.y > rowSecond) { rowSecond = d.y; }
        if (d.z > rowBest) { rowSecond = rowBest; rowBest = d.z; rowBestI = c0 + 2; }
        else if (d.z > rowSecond) { rowSecond = d.z; }
        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }
        else if (d.w > rowSecond) { rowSecond = d.w; }
      }
    } else if (lid < 2u * WGR) {
      let c = lid - WGR;
      let q = c / 4u;
      let m = c % 4u;
      var cb = 0.0;
      var cs = 0.0;
      var ci = -1;
      for (var r = 0u; r < WGR; r = r + 1u) {
        let d = S[r * 16u + q][m];
        if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }
        else if (d > cs) { cs = d; }
      }
      let gc = col0 + c;
      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }
    }
    col0 = col0 + BT;
  }

  if (lid < WGR && myRow < U.numA) {
    RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI);
    OutAB[myRow] = gatef(rowBest, rowSecond, rowBestI);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[w * U.numB + c];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

                                                                                                             
                                                                                               
                                                                                                 
                                                                       
                                                                                                   
                                                                                
                                                                                                    
                                                                                                   
                                                                               
                                                                                                                           
                                                                                                                          
                                                                                                                           
constexpr char kWgslXpose[] = R"WGSL(
struct XParams { n4 : u32, p1 : u32, p2 : u32, p3 : u32, };
@group(0) @binding(0) var<storage, read> Src : array<u32>;
@group(0) @binding(3) var<uniform> X : XParams;
@group(0) @binding(7) var<storage, read_write> Dst : array<vec4<f32>>;
// 线程 i:q = 行四元组(相邻线程相邻 q ⇒ 写合并),w = 32 个 u32 字之一(k = 4w..4w+3)。
// 源缓冲已按 n4*4 行零补位(见 UploadDesc),越界由构造排除。
@compute @workgroup_size(64)
fn xpose(@builtin(global_invocation_id) gid : vec3<u32>) {
  let n4 = X.n4;
  let i = gid.x;
  if (i >= n4 * 32u) { return; }
  let q = i % n4;
  let w = i / n4;
  let x0 = Src[(4u * q + 0u) * 32u + w];
  let x1 = Src[(4u * q + 1u) * 32u + w];
  let x2 = Src[(4u * q + 2u) * 32u + w];
  let x3 = Src[(4u * q + 3u) * 32u + w];
  let o = (w * 4u) * n4 + q;
  Dst[o + 0u * n4] = vec4<f32>(f32(x0 & 255u), f32(x1 & 255u), f32(x2 & 255u), f32(x3 & 255u));
  Dst[o + 1u * n4] = vec4<f32>(f32((x0 >> 8u) & 255u), f32((x1 >> 8u) & 255u), f32((x2 >> 8u) & 255u), f32((x3 >> 8u) & 255u));
  Dst[o + 2u * n4] = vec4<f32>(f32((x0 >> 16u) & 255u), f32((x1 >> 16u) & 255u), f32((x2 >> 16u) & 255u), f32((x3 >> 16u) & 255u));
  Dst[o + 3u * n4] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));
}
)WGSL";

constexpr char kWgslBlocked84Direct[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 64u;
const BT : u32 = 64u;
const KD : u32 = 128u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  colBase : u32,
  colSpan : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<u32>;
@group(0) @binding(1) var<storage, read> B : array<u32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;
@group(0) @binding(6) var<storage, read_write> RowP : array<ColPart>;
@group(0) @binding(7) var<storage, read> At : array<vec4<f32>>;
@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

var<workgroup> S : array<vec4<f32>, 1024>;

@compute @workgroup_size(128)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let tr = lid / 16u;
  let tc = lid % 16u;
  let rowPad4 = ((U.numA + 63u) / 64u) * 16u;
  let colPad4 = ((U.numB + 127u) / 128u) * 32u;
  let rq = row0 / 4u + tr * 2u;

  let myRow = row0 + lid;
  var rowBest = 0.0;
  var rowSecond = 0.0;
  var rowBestI = -1;
  if (U.colBase != 0u && lid < WGR && myRow < U.numA) {
    let rp = RowP[myRow];
    rowBest = rp.best;
    rowSecond = rp.second;
    rowBestI = rp.idx;
  }

  var col0 = U.colBase;
  let colEnd = min(U.colBase + U.colSpan, U.numB);
  loop {
    if (col0 >= colEnd) { break; }
    let cq = col0 / 4u + tc;

    var acc0 = vec4<f32>(0.0);
    var acc1 = vec4<f32>(0.0);
    var acc2 = vec4<f32>(0.0);
    var acc3 = vec4<f32>(0.0);
    var acc4 = vec4<f32>(0.0);
    var acc5 = vec4<f32>(0.0);
    var acc6 = vec4<f32>(0.0);
    var acc7 = vec4<f32>(0.0);

    var b4 = Bt[cq];
    for (var k = 0u; k < KD; k = k + 1u) {
      let al = At[k * rowPad4 + rq];
      let ah = At[k * rowPad4 + rq + 1u];
      let bn = Bt[min(k + 1u, KD - 1u) * colPad4 + cq];
      acc0 = acc0 + al.x * b4;
      acc1 = acc1 + al.y * b4;
      acc2 = acc2 + al.z * b4;
      acc3 = acc3 + al.w * b4;
      acc4 = acc4 + ah.x * b4;
      acc5 = acc5 + ah.y * b4;
      acc6 = acc6 + ah.z * b4;
      acc7 = acc7 + ah.w * b4;
      b4 = bn;
    }
    workgroupBarrier();
    S[(tr * 8u + 0u) * 16u + tc] = acc0;
    S[(tr * 8u + 1u) * 16u + tc] = acc1;
    S[(tr * 8u + 2u) * 16u + tc] = acc2;
    S[(tr * 8u + 3u) * 16u + tc] = acc3;
    S[(tr * 8u + 4u) * 16u + tc] = acc4;
    S[(tr * 8u + 5u) * 16u + tc] = acc5;
    S[(tr * 8u + 6u) * 16u + tc] = acc6;
    S[(tr * 8u + 7u) * 16u + tc] = acc7;
    workgroupBarrier();

    if (lid < WGR) {
      for (var v = 0u; v < 16u; v = v + 1u) {
        let d = S[lid * 16u + v];
        let c0 = i32(col0 + v * 4u);
        if (d.x > rowBest) { rowSecond = rowBest; rowBest = d.x; rowBestI = c0; }
        else if (d.x > rowSecond) { rowSecond = d.x; }
        if (d.y > rowBest) { rowSecond = rowBest; rowBest = d.y; rowBestI = c0 + 1; }
        else if (d.y > rowSecond) { rowSecond = d.y; }
        if (d.z > rowBest) { rowSecond = rowBest; rowBest = d.z; rowBestI = c0 + 2; }
        else if (d.z > rowSecond) { rowSecond = d.z; }
        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }
        else if (d.w > rowSecond) { rowSecond = d.w; }
      }
    } else if (lid < 2u * WGR) {
      let c = lid - WGR;
      let q = c / 4u;
      let m = c % 4u;
      var cb = 0.0;
      var cs = 0.0;
      var ci = -1;
      for (var r = 0u; r < WGR; r = r + 1u) {
        let d = S[r * 16u + q][m];
        if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }
        else if (d > cs) { cs = d; }
      }
      let gc = col0 + c;
      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }
    }
    col0 = col0 + BT;
  }

  if (lid < WGR && myRow < U.numA) {
    RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI);
    OutAB[myRow] = gatef(rowBest, rowSecond, rowBestI);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[w * U.numB + c];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

                                                                                                     
                                                                                                                
                                                   
constexpr char kWgslBlocked88[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 64u;
const BT : u32 = 64u;
const KC : u32 = 32u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  colBase : u32,
  colSpan : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<u32>;
@group(0) @binding(1) var<storage, read> B : array<u32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;
@group(0) @binding(6) var<storage, read_write> RowP : array<ColPart>;

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

var<workgroup> S : array<vec4<f32>, 1024>;

@compute @workgroup_size(64)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let tr = lid / 8u;
  let tc = lid % 8u;

  let myRow = row0 + lid;
  var rowBest = 0.0;
  var rowSecond = 0.0;
  var rowBestI = -1;
  if (U.colBase != 0u && lid < WGR && myRow < U.numA) {
    let rp = RowP[myRow];
    rowBest = rp.best;
    rowSecond = rp.second;
    rowBestI = rp.idx;
  }

  var col0 = U.colBase;
  let colEnd = min(U.colBase + U.colSpan, U.numB);
  loop {
    if (col0 >= colEnd) { break; }

    var acc0 = vec4<f32>(0.0);
    var acc1 = vec4<f32>(0.0);
    var acc2 = vec4<f32>(0.0);
    var acc3 = vec4<f32>(0.0);
    var acc4 = vec4<f32>(0.0);
    var acc5 = vec4<f32>(0.0);
    var acc6 = vec4<f32>(0.0);
    var acc7 = vec4<f32>(0.0);
    var acd0 = vec4<f32>(0.0);
    var acd1 = vec4<f32>(0.0);
    var acd2 = vec4<f32>(0.0);
    var acd3 = vec4<f32>(0.0);
    var acd4 = vec4<f32>(0.0);
    var acd5 = vec4<f32>(0.0);
    var acd6 = vec4<f32>(0.0);
    var acd7 = vec4<f32>(0.0);

    for (var kc = 0u; kc < 4u; kc = kc + 1u) {
      workgroupBarrier();
      // 暂存:线程 0..127 负责 A、128..255 负责 B;每线程取同一 w 下连续 4 行的
      // 4 个 u32,写出 4 个**完整** vec4(k = w*4+j,j=0..3)。不对分量做运行时索引写。
      for (var pair = 0u; pair < 4u; pair = pair + 1u) {
        let side = pair / 2u;
        let t = lid + (pair % 2u) * 64u;
        let w = t / 16u;
        let q = t % 16u;
        let base = q * 4u;
        var x0 = 0u; var x1 = 0u; var x2 = 0u; var x3 = 0u;
        if (side == 0u) {
          let g = row0 + base;
          x0 = select(0u, A[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numA);
          x1 = select(0u, A[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numA);
          x2 = select(0u, A[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numA);
          x3 = select(0u, A[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numA);
        } else {
          let g = col0 + base;
          x0 = select(0u, B[(g + 0u) * 32u + kc * 8u + w], g + 0u < U.numB);
          x1 = select(0u, B[(g + 1u) * 32u + kc * 8u + w], g + 1u < U.numB);
          x2 = select(0u, B[(g + 2u) * 32u + kc * 8u + w], g + 2u < U.numB);
          x3 = select(0u, B[(g + 3u) * 32u + kc * 8u + w], g + 3u < U.numB);
        }
        let o = side * 512u + (w * 4u) * 16u + q;
        S[o + 0u * 16u] = vec4<f32>(f32(x0 & 255u), f32(x1 & 255u), f32(x2 & 255u), f32(x3 & 255u));
        S[o + 1u * 16u] = vec4<f32>(f32((x0 >> 8u) & 255u), f32((x1 >> 8u) & 255u), f32((x2 >> 8u) & 255u), f32((x3 >> 8u) & 255u));
        S[o + 2u * 16u] = vec4<f32>(f32((x0 >> 16u) & 255u), f32((x1 >> 16u) & 255u), f32((x2 >> 16u) & 255u), f32((x3 >> 16u) & 255u));
        S[o + 3u * 16u] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));
      }
      workgroupBarrier();
      for (var k = 0u; k < KC; k = k + 1u) {
        let al = S[k * 16u + tr * 2u];
        let ah = S[k * 16u + tr * 2u + 1u];
        let b4 = S[512u + k * 16u + tc * 2u];
        let bh = S[512u + k * 16u + tc * 2u + 1u];
        acc0 = acc0 + al.x * b4;
        acc1 = acc1 + al.y * b4;
        acc2 = acc2 + al.z * b4;
        acc3 = acc3 + al.w * b4;
        acc4 = acc4 + ah.x * b4;
        acc5 = acc5 + ah.y * b4;
        acc6 = acc6 + ah.z * b4;
        acc7 = acc7 + ah.w * b4;
        acd0 = acd0 + al.x * bh;
        acd1 = acd1 + al.y * bh;
        acd2 = acd2 + al.z * bh;
        acd3 = acd3 + al.w * bh;
        acd4 = acd4 + ah.x * bh;
        acd5 = acd5 + ah.y * bh;
        acd6 = acd6 + ah.z * bh;
        acd7 = acd7 + ah.w * bh;
      }
    }
    workgroupBarrier();
    S[(tr * 8u + 0u) * 16u + tc * 2u] = acc0;
    S[(tr * 8u + 1u) * 16u + tc * 2u] = acc1;
    S[(tr * 8u + 2u) * 16u + tc * 2u] = acc2;
    S[(tr * 8u + 3u) * 16u + tc * 2u] = acc3;
    S[(tr * 8u + 4u) * 16u + tc * 2u] = acc4;
    S[(tr * 8u + 5u) * 16u + tc * 2u] = acc5;
    S[(tr * 8u + 6u) * 16u + tc * 2u] = acc6;
    S[(tr * 8u + 7u) * 16u + tc * 2u] = acc7;
    S[(tr * 8u + 0u) * 16u + tc * 2u + 1u] = acd0;
    S[(tr * 8u + 1u) * 16u + tc * 2u + 1u] = acd1;
    S[(tr * 8u + 2u) * 16u + tc * 2u + 1u] = acd2;
    S[(tr * 8u + 3u) * 16u + tc * 2u + 1u] = acd3;
    S[(tr * 8u + 4u) * 16u + tc * 2u + 1u] = acd4;
    S[(tr * 8u + 5u) * 16u + tc * 2u + 1u] = acd5;
    S[(tr * 8u + 6u) * 16u + tc * 2u + 1u] = acd6;
    S[(tr * 8u + 7u) * 16u + tc * 2u + 1u] = acd7;
    workgroupBarrier();

    if (lid < WGR) {
      for (var v = 0u; v < 16u; v = v + 1u) {
        let d = S[lid * 16u + v];
        let c0 = i32(col0 + v * 4u);
        if (d.x > rowBest) { rowSecond = rowBest; rowBest = d.x; rowBestI = c0; }
        else if (d.x > rowSecond) { rowSecond = d.x; }
        if (d.y > rowBest) { rowSecond = rowBest; rowBest = d.y; rowBestI = c0 + 1; }
        else if (d.y > rowSecond) { rowSecond = d.y; }
        if (d.z > rowBest) { rowSecond = rowBest; rowBest = d.z; rowBestI = c0 + 2; }
        else if (d.z > rowSecond) { rowSecond = d.z; }
        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }
        else if (d.w > rowSecond) { rowSecond = d.w; }
      }
    }
    workgroupBarrier();
    if (lid < WGR) {
      let c = lid;
      let q = c / 4u;
      let m = c % 4u;
      var cb = 0.0;
      var cs = 0.0;
      var ci = -1;
      for (var r = 0u; r < WGR; r = r + 1u) {
        let d = S[r * 16u + q][m];
        if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }
        else if (d > cs) { cs = d; }
      }
      let gc = col0 + c;
      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }
    }
    col0 = col0 + BT;
  }

  if (lid < WGR && myRow < U.numA) {
    RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI);
    OutAB[myRow] = gatef(rowBest, rowSecond, rowBestI);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[w * U.numB + c];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

constexpr char kWgslMmaFused[] = R"WGSL(
enable chromium_experimental_subgroup_matrix;
enable subgroups;

alias Left = subgroup_matrix_left<f32, 8, 8>;
alias Right = subgroup_matrix_right<f32, 8, 8>;
alias Res = subgroup_matrix_result<f32, 8, 8>;

const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 128u;
const BT : u32 = 32u;

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  pad1 : u32,
  pad2 : u32,
};

struct ColPart {
  best : f32,
  second : f32,
  idx : i32,
};

@group(0) @binding(0) var<storage, read> A : array<f32>;
@group(0) @binding(1) var<storage, read> B : array<f32>;
@group(0) @binding(2) var<storage, read_write> OutAB : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;
@group(0) @binding(4) var<storage, read_write> ColP : array<ColPart>;
@group(0) @binding(5) var<storage, read_write> OutBA : array<i32>;

var<workgroup> Bsh : array<f32, 4096>; // 32 rows x 128 (16 KiB)
var<workgroup> accSh : array<f32, 4096>;   // WGR rows x 32 cols

fn gatef(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(best * INV_SQ_NORM, 1.0));
  let sd = acos(min(second * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

@compute @workgroup_size(512)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32,
        @builtin(subgroup_id) sg : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let aRow0 = row0 + sg * 8u;
  var aFrag : array<Left, 16>;
  for (var k = 0u; k < 16u; k = k + 1u) {
    aFrag[k] = subgroupMatrixLoad<Left>(&A, aRow0 * 128u + k * 8u, false, 128u);
  }
  var rbest = 0.0;
  var rsecond = 0.0;
  var rbi = -1;
    for (var e = lid; e < BT * 128u; e = e + 512u) {
      let brow = 0u + e / 128u;
      Bsh[e] = select(0.0, B[brow * 128u + (e % 128u)], brow < U.numB);
    }

  workgroupBarrier();

  var tile0 = 0u;
  loop {
    if (tile0 >= U.numB) { break; }
    for (var nt = 0u; nt < 4u; nt = nt + 1u) {
      var acc = Res(0.0);
      for (var k = 0u; k < 16u; k = k + 1u) {
        let bF = subgroupMatrixLoad<Right>(&Bsh, (nt * 8u) * 128u + k * 8u, true, 128u);
        acc = subgroupMatrixMultiplyAccumulate(aFrag[k], bF, acc);
      }
      subgroupMatrixStore(&accSh, (sg * 8u) * 32u + nt * 8u, acc, false, 32u);
    }
    workgroupBarrier();
    let nextT = tile0 + BT;
    if (nextT < U.numB && lid >= 160u) {
      for (var e = lid - 160u; e < BT * 128u; e = e + 352u) {
        let brow = nextT + e / 128u;
        Bsh[e] = select(0.0, B[brow * 128u + (e % 128u)], brow < U.numB);
      }
    }

    if (lid < WGR) {

      // 运行时上界让 LLVM 无法展开这个 32 次循环(手写 Metal 那边是编译期常量);
      // 隔离台架(tint 生成的 MSL 逐行对拍手写形态)实测这一改 −0.37ms。
      // **语义等价证明**:预取对越界列写的是精确 0(select(0, B[..], brow<numB)),
      // 而这里是严格 `>` ⇒ rbest 初值 0.0 时 `0.0 > 0.0` 为假,补位列永远不会
      // 成为 best/second,tile0+c 也就永远不会被写进 rbi。逐字节闸复验。
      // guided 核**不能同样处理**:那边多一个 guide_ok(q, PtsD[tile0+c]),
      // 补位下标会越界读点云缓冲,零点积的论证覆盖不到它。
      for (var c = 0u; c < BT; c = c + 1u) {
        let s = accSh[lid * 32u + c];
        if (s > rbest) {
          rsecond = rbest; rbest = s; rbi = i32(tile0 + c);
        } else if (s > rsecond) { rsecond = s; }
      }
    }
    if (lid >= WGR && lid < WGR + BT) {
      let cl = lid - WGR;
      let c = tile0 + cl;
      if (c < U.numB) {
        var best = 0.0; var second = 0.0; var bi = -1;
        for (var r = 0u; r < WGR; r = r + 1u) {
          let s = accSh[r * 32u + cl];
          if (s > best) {
            second = best; best = s; bi = i32(row0 + r);
          } else if (s > second) { second = s; }
        }
        ColP[c * U.numWg + rb] = ColPart(best, second, bi);
      }
    }

    workgroupBarrier();
    tile0 = nextT;
  }
  let row = row0 + lid;
  if (lid < WGR && row < U.numA) {
    OutAB[row] = gatef(rbest, rsecond, rbi);
  }
}

@compute @workgroup_size(64)
fn merge(@builtin(global_invocation_id) gid : vec3<u32>) {
  let c = gid.x;
  if (c >= U.numB) { return; }
  var best = 0.0;
  var second = 0.0;
  var bi = -1;
  for (var w = 0u; w < U.numWg; w = w + 1u) {
    let p = ColP[c * U.numWg + w];
    if (p.best > best) {
      second = max(best, p.second);
      best = p.best;
      bi = p.idx;
    } else {
      second = max(second, p.best);
    }
  }
  OutBA[c] = gatef(best, second, bi);
}
)WGSL";

                                                                       
                                                                       
                                         
constexpr char kWgslGuidedCommon[] = R"WGSL(
struct GParams {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  guideMode : u32,
  rowBase : u32,
  maxResidual : f32,
  pad0 : u32,
};

fn guide_ok(q : vec2<f32>, d : vec2<f32>) -> bool {
  if (U.guideMode == 1u) {
    let p1 = vec3<f32>(q, 1.0);
    let p2 = vec3<f32>(d, 1.0);
    let line2 = vec3<f32>(
        M[0] * p1.x + M[1] * p1.y + M[2],
        M[3] * p1.x + M[4] * p1.y + M[5],
        M[6] * p1.x + M[7] * p1.y + M[8]);
    let line1 = vec3<f32>(
        M[0] * p2.x + M[3] * p2.y + M[6],
        M[1] * p2.x + M[4] * p2.y + M[7],
        M[2] * p2.x + M[5] * p2.y + M[8]);
    let nom = dot(p2, line2);
    let denom = dot(line2.xy, line2.xy) + dot(line1.xy, line1.xy);
    return denom > 1e-12 && nom * nom <= U.maxResidual * denom;
  } else if (U.guideMode == 2u) {
    let hx = M[0] * q.x + M[1] * q.y + M[2];
    let hy = M[3] * q.x + M[4] * q.y + M[5];
    let hz = M[6] * q.x + M[7] * q.y + M[8];
    if (abs(hz) <= 1e-8) {
      return false;
    }
    let delta = vec2<f32>(hx / hz, hy / hz) - d;
    return dot(delta, delta) <= U.maxResidual;
  }
  return true;
}

fn gate_guided(best : f32, second : f32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let secondDot = max(second, 131072.0);
  let bd = sqrt(max(0.0, 2.0 - 2.0 * best * INV_SQ_NORM));
  let sd = sqrt(max(0.0, 2.0 - 2.0 * secondDot * INV_SQ_NORM));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}
)WGSL";

                                                                        
                                                                              
                                                                    
                                                                             
                                                                                
                                                                       
constexpr char kWgslMmaGuidedHead[] = R"WGSL(
enable chromium_experimental_subgroup_matrix;
enable subgroups;

alias Left = subgroup_matrix_left<f32, 8, 8>;
alias Right = subgroup_matrix_right<f32, 8, 8>;
alias Res = subgroup_matrix_result<f32, 8, 8>;

const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
const WGR : u32 = 128u;
const BT : u32 = 32u;
)WGSL";

constexpr char kWgslMmaGuidedBindings[] = R"WGSL(
@group(0) @binding(0) var<storage, read> A : array<f32>;
@group(0) @binding(1) var<storage, read> B : array<f32>;
@group(0) @binding(2) var<storage, read_write> Out : array<i32>;
@group(0) @binding(3) var<uniform> U : GParams;
@group(0) @binding(4) var<storage, read> PtsQ : array<vec2<f32>>;
@group(0) @binding(5) var<storage, read> PtsD : array<vec2<f32>>;
@group(0) @binding(6) var<storage, read> M : array<f32>;

var<workgroup> Bsh : array<f32, 4096>;
var<workgroup> accSh : array<f32, 4096>;
)WGSL";

constexpr char kWgslMmaGuidedMain[] = R"WGSL(
@compute @workgroup_size(512)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32,
        @builtin(subgroup_id) sg : u32) {
  let rb = U.rowBase + wg.x;
  let row0 = rb * WGR;
  let aRow0 = row0 + sg * 8u;
  var aFrag : array<Left, 16>;
  for (var k = 0u; k < 16u; k = k + 1u) {
    aFrag[k] = subgroupMatrixLoad<Left>(&A, aRow0 * 128u + k * 8u, false, 128u);
  }
  var rbest = 0.0;
  var rsecond = 0.0;
  var rbi = -1;
  var q = vec2<f32>(0.0, 0.0);
  if (lid < WGR) { q = PtsQ[row0 + lid]; }
    for (var e = lid; e < BT * 128u; e = e + 512u) {
      let brow = 0u + e / 128u;
      Bsh[e] = select(0.0, B[brow * 128u + (e % 128u)], brow < U.numB);
    }

  workgroupBarrier();

  var tile0 = 0u;
  loop {
    if (tile0 >= U.numB) { break; }
    for (var nt = 0u; nt < 4u; nt = nt + 1u) {
      var acc = Res(0.0);
      for (var k = 0u; k < 16u; k = k + 1u) {
        let bF = subgroupMatrixLoad<Right>(&Bsh, (nt * 8u) * 128u + k * 8u, true, 128u);
        acc = subgroupMatrixMultiplyAccumulate(aFrag[k], bF, acc);
      }
      subgroupMatrixStore(&accSh, (sg * 8u) * 32u + nt * 8u, acc, false, 32u);
    }
    workgroupBarrier();
    let nextT = tile0 + BT;
    if (nextT < U.numB && lid >= 160u) {
      for (var e = lid - 160u; e < BT * 128u; e = e + 352u) {
        let brow = nextT + e / 128u;
        Bsh[e] = select(0.0, B[brow * 128u + (e % 128u)], brow < U.numB);
      }
    }

    if (lid < WGR) {
      let lim = min(BT, U.numB - tile0);
      for (var c = 0u; c < lim; c = c + 1u) {
        if (!guide_ok(q, PtsD[tile0 + c])) { continue; }
        let s = accSh[lid * 32u + c];
        if (s > rbest) {
          rsecond = rbest; rbest = s; rbi = i32(tile0 + c);
        } else if (s > rsecond) { rsecond = s; }
      }
    }

    workgroupBarrier();
    tile0 = nextT;
  }
  let row = row0 + lid;
  if (lid < WGR && row < U.numA) {
    Out[row] = gate_guided(rbest, rsecond, rbi);
  }
}
)WGSL";

                                                                              
                                                                            
                                            
constexpr char kWgslTiledPlain[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144

struct Params {
  numA : u32,
  numB : u32,
  maxRatio : f32,
  maxDistance : f32,
  numWg : u32,
  rowBase : u32,
  pad1 : u32,
  pad2 : u32,
};

@group(0) @binding(0) var<storage, read> A : array<vec4<u32>>;
@group(0) @binding(1) var<storage, read> B : array<vec4<u32>>;
@group(0) @binding(2) var<storage, read_write> Out : array<i32>;
@group(0) @binding(3) var<uniform> U : Params;

fn gate(best : u32, second : u32, bestIndex : i32) -> i32 {
  if (bestIndex < 0) { return -1; }
  let bd = acos(min(f32(best) * INV_SQ_NORM, 1.0));
  let sd = acos(min(f32(second) * INV_SQ_NORM, 1.0));
  if (bd <= U.maxDistance && bd < U.maxRatio * sd) { return bestIndex; }
  return -1;
}

const WG : u32 = 64u;

var<workgroup> Bsh : array<vec4<u32>, 512>; // 64 rows x 8 vec4 words

@compute @workgroup_size(64)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let row = (U.rowBase + wg.x) * WG + lid;
  let valid = row < U.numA;
  var q : array<vec4<u32>, 8>;
  if (valid) {
    for (var w = 0u; w < 8u; w = w + 1u) { q[w] = A[row * 8u + w]; }
  }
  var best = 0u;
  var second = 0u;
  var bestIndex = -1;
  var tile0 = 0u;
  loop {
    if (tile0 >= U.numB) { break; }
    let brow = tile0 + lid;
    if (brow < U.numB) {
      for (var w = 0u; w < 8u; w = w + 1u) {
        Bsh[lid * 8u + w] = B[brow * 8u + w];
      }
    }
    workgroupBarrier();
    let lim = min(WG, U.numB - tile0);
    if (valid) {
      for (var c = 0u; c < lim; c = c + 1u) {
        var s = 0u;
        for (var w = 0u; w < 8u; w = w + 1u) {
          let bv = Bsh[c * 8u + w];
          s += dot4U8Packed(q[w].x, bv.x) + dot4U8Packed(q[w].y, bv.y) +
               dot4U8Packed(q[w].z, bv.z) + dot4U8Packed(q[w].w, bv.w);
        }
        if (s > best) {
          second = best;
          best = s;
          bestIndex = i32(tile0 + c);
        } else if (s > second) {
          second = s;
        }
      }
    }
    workgroupBarrier();
    tile0 = tile0 + WG;
  }
  if (valid) { Out[row] = gate(best, second, bestIndex); }
}
)WGSL";

                                                                             
                                                                          
                                                                        
constexpr char kWgslTiledGuidedHead[] = R"WGSL(
const INV_SQ_NORM : f32 = 0.000003814697265625; // 1/262144
)WGSL";

constexpr char kWgslTiledGuidedBindings[] = R"WGSL(
@group(0) @binding(0) var<storage, read> A : array<vec4<u32>>;
@group(0) @binding(1) var<storage, read> B : array<vec4<u32>>;
@group(0) @binding(2) var<storage, read_write> Out : array<i32>;
@group(0) @binding(3) var<uniform> U : GParams;
@group(0) @binding(4) var<storage, read> PtsQ : array<vec2<f32>>;
@group(0) @binding(5) var<storage, read> PtsD : array<vec2<f32>>;
@group(0) @binding(6) var<storage, read> M : array<f32>;

const WG : u32 = 64u;

var<workgroup> Bsh : array<vec4<u32>, 512>; // 64 rows x 8 vec4 words
)WGSL";

constexpr char kWgslTiledGuidedMain[] = R"WGSL(
@compute @workgroup_size(64)
fn main(@builtin(workgroup_id) wg : vec3<u32>,
        @builtin(local_invocation_index) lid : u32) {
  let row = (U.rowBase + wg.x) * WG + lid;
  let valid = row < U.numA;
  var q : array<vec4<u32>, 8>;
  var qpt = vec2<f32>(0.0, 0.0);
  if (valid) {
    for (var w = 0u; w < 8u; w = w + 1u) { q[w] = A[row * 8u + w]; }
    qpt = PtsQ[row];
  }
  var best = 0.0;
  var second = 0.0;
  var bestIndex = -1;
  var tile0 = 0u;
  loop {
    if (tile0 >= U.numB) { break; }
    let brow = tile0 + lid;
    if (brow < U.numB) {
      for (var w = 0u; w < 8u; w = w + 1u) {
        Bsh[lid * 8u + w] = B[brow * 8u + w];
      }
    }
    workgroupBarrier();
    let lim = min(WG, U.numB - tile0);
    if (valid) {
      for (var c = 0u; c < lim; c = c + 1u) {
        if (!guide_ok(qpt, PtsD[tile0 + c])) { continue; }
        var su = 0u;
        for (var w = 0u; w < 8u; w = w + 1u) {
          let bv = Bsh[c * 8u + w];
          su += dot4U8Packed(q[w].x, bv.x) + dot4U8Packed(q[w].y, bv.y) +
                dot4U8Packed(q[w].z, bv.z) + dot4U8Packed(q[w].w, bv.w);
        }
        let s = f32(su);
        if (s > best) {
          second = best;
          best = s;
          bestIndex = i32(tile0 + c);
        } else if (s > second) {
          second = s;
        }
      }
    }
    workgroupBarrier();
    tile0 = tile0 + WG;
  }
  if (valid) { Out[row] = gate_guided(best, second, bestIndex); }
}
)WGSL";

struct alignas(16) Params {
  uint32_t numA, numB;
  float maxRatio, maxDistance;
                                                                                       
  uint32_t numWg, rowBase, colBase, colSpan;
};
struct alignas(16) GParams {
  uint32_t numA, numB;
  float maxRatio, maxDistance;
  uint32_t guideMode, rowBase;
  float maxResidual;
  uint32_t pad0;
};
static_assert(sizeof(Params) == 32, "Params must be 32 bytes");
static_assert(sizeof(GParams) == 32, "GParams must be 32 bytes");
constexpr uint64_t kColBaseOffset = 24;                    
constexpr uint64_t kColSpanOffset = 28;                    
constexpr uint64_t kRowBaseOffset = 20;                                   

                                                                                                                                                                    

enum class Backend { kNone, kMma, kTiled, kBlocked };

                                                                                                          
                                                                                                              
                                                     
                                                                                                                                    
                                                                                                                               
                                                                                                                       
                                                                                             
                                                                                                     
struct DirectKnobs { bool direct, k44, w128, nopb, f16, w64, keyscan2, ptr, keyscan3, kp; };
static DirectKnobs ResolveDirectKnobs() {
  DirectKnobs k{};
  const bool legacy = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_LEGACY84") != nullptr;
  k.direct = legacy ? (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT") != nullptr) : true;
  k.k44 = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_84") == nullptr &&
          (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_44") != nullptr || !legacy);
  k.w128 = k.k44 && std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_W256") == nullptr &&
           (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_W128") != nullptr || !legacy);
  k.nopb = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PIPEB") == nullptr &&
           (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOPB") != nullptr || !legacy);
  k.f16 = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_F32") == nullptr &&
          (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_F16") != nullptr || !legacy);
                                                                                                                 
                                                                                                                                                                  
                                                                                                                                                
                                                                                                                                              
                                                                                                                                            
                                                                                                                       
  const bool noks = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOKEYSCAN") != nullptr;
  const bool ks1 = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN") != nullptr;
  const bool ks2 = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN2") != nullptr;
  k.keyscan2 = k.w128 && !noks && !ks1 && ks2;
  k.keyscan3 = k.w128 && !noks && !ks1 && !ks2 &&
               (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN3") != nullptr || !legacy);
  const bool anyKeyscan = k.keyscan2 || k.keyscan3 || (k.w128 && std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN") != nullptr);
  k.w64 = anyKeyscan && std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOW64") == nullptr &&
          (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_W64") != nullptr || !legacy);
  k.ptr = k.nopb && std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOPTR") == nullptr &&
          (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PTR") != nullptr || !legacy);
                                                                                                                                                 
                                                                        
  k.kp = k.f16 && k.ptr && std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOKP") == nullptr &&
         (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KP") != nullptr || !legacy);
  return k;
}

static const char* BlockedLabel() {
  const DirectKnobs k = ResolveDirectKnobs();
  if (!k.direct) {
    if (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_88") != nullptr) return "blocked(fma8x8,V3)";
    if (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_44") != nullptr) return "blocked(fma4x4,V3)";
    if (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_PIPE") != nullptr) return "blocked(fma8x4+pipe,V3)";
    if (std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_NOPIPEB") != nullptr) return "blocked(fma8x4,V3)";
    return "blocked(fma8x4+pipeb,V3)";
  }
  static std::string lbl;
  lbl = std::string("blocked(") + (k.k44 ? "fma4x4" : "fma8x4") + "+direct" + (k.w128 ? (k.w64 ? "+w64" : "+w128") : "") +
        (k.nopb ? "+nopb" : "+pipeb") + (k.f16 ? "+f16" : "+f32") + (k.keyscan2 ? "+keyscan2" : "") + (k.keyscan3 ? "+keyscan3" : "") + (k.ptr ? "+ptr" : "") + (k.kp ? "+kp" : "");
  const char* extras[][2] = {
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_G", "+g"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TEX", "+tex"},
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TEXA", "+texa"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANMEM", "+scanmem"},
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_FMA", "+fma"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PSCAN", "+pscan"},
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PACKED", "+packed"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TMAP", "+tmap"},
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TAILB", "+tailb"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TAILB2", "+tailb2"},
    {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PIPEA", "+pipea"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANSEL", "+scansel"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANU4", "+scanu4"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PSCAN2", "+pscan2"}, {"OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN", "+keyscan"},
  };
  for (auto& e : extras) if (std::getenv(e[0]) != nullptr) lbl += e[1];
  if (const char* v = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLL")) lbl += std::string("+unroll") + v;
  if (const char* v = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLLH")) lbl += std::string("+unrollh") + v;
  if (const char* v = std::getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLLHB")) lbl += std::string("+unrollhb") + v;
  lbl += ",V8)";                                            
  return lbl.c_str();
}
                                                                                                            
uint32_t gBlockedRows = kBlockedRows;                                           
inline uint32_t MainRows(Backend b) {
  return b == Backend::kBlocked ? gBlockedRows : kMmaRows;
}
                                                                                             
                                                                                     
                                                                                              
                                                                                        
                                   
inline uint32_t MainTileCols(Backend b) {
  return b == Backend::kBlocked ? 64u : 32u;
}

                                                                        
                                      
std::atomic<int> gDeviceLostRc{0};                                      
std::atomic<int> gErrorType{0};                                             
std::atomic<bool> gTearingDown{false};

struct PoolBuf {
  wgpu::Buffer buf;
  uint64_t cap = 0;
  wgpu::BufferUsage usage = wgpu::BufferUsage::None;
};

struct Ctx {
  wgpu::Instance instance;
  wgpu::Adapter adapter;
  wgpu::Device device;
  wgpu::Queue queue;
  Backend backend = Backend::kNone;
  std::string adapter_name;
  std::string backend_info;
  bool feat_subgroups = false, feat_sgmatrix = false, feat_packed_dot = false;
  bool feat_timestamp = false, sgcfg_f32_8x8x8 = false;
                                                                               
                                                                           
                                                                               
                                                                            
                                                                      
                                                                           
                                                                            
                                                               
                                                                              
  bool sgcfg_f16_f32 = false;
  bool mixed = false;                                  
  bool packed = false;                                                 
  std::vector<uint16_t> scratch16;
  std::vector<uint8_t> padZero;
                                                                                     
                                                                                                   
                                                                                           
                                                                                            
                                                                                              
  bool ts_on = false;
  wgpu::QuerySet ts_qset;
  wgpu::Buffer ts_resolve, ts_map;
  uint32_t ts_slot = 0;
  static constexpr uint32_t kTsSlots = 64;
  uint32_t subgroup_min = 0, subgroup_max = 0;
  uint32_t lim_storage = 0, lim_invocations = 0, lim_size_x = 0;
                                                                         
  wgpu::ComputePipeline p_main, p_merge, p_guided;
  wgpu::ComputePipeline p_xpose;                                              
  bool direct = false;                                                                              
  bool direct_g = false;                                                                                             
  bool direct_tex = false;                                                                       
  bool direct_texa = false;                                                          
  bool direct_packed = false;                                                            
  bool feat_f16 = false;                                                
  bool direct_f16 = false;                                                            
  bool direct_kp = false;                                                                                              
  wgpu::Texture atTex; uint32_t atTexW = 0;                                                 
  wgpu::Texture pbAtTex; uint32_t pbAtTexW = 0;                                             
  wgpu::Texture btTex; uint32_t btTexW = 0;                                                               
  std::vector<wgpu::Texture> pbTex; std::vector<uint32_t> pbTexW;                                      
  bool tried_main = false, tried_guided = false;
                       
  PoolBuf a{}, b{}, outAB{}, outBA{}, colp{}, rowp{}, uni{}, staging{}, ptsA{}, ptsB{},
      matAB{}, matBA{};
  PoolBuf pbA{}, pbB{}, pbOutAB{}, pbOutBA{}, pbColp{}, pbRowp{}, pbUni{}, pbStaging{};
  PoolBuf at{}, bt{}, pbAt{}, pbBt{}, pbXUni{};             
  PoolBuf scr{}, pbScr{};                                      
                  
  std::vector<float> scratchA, scratchB;
  std::vector<int32_t> sentinel, host;
                                                                      
  double ema_main = 0.0;                                                  
  double ema_guided = 0.0;                                
};

std::mutex gMatchCallLock;
std::unique_ptr<Ctx> gCtx;
double gLastInitFailMs = -1e12;
int gInitLogged = 0;

void ClearAllDescriptorResidencyForDeviceError();

std::unique_ptr<Ctx> CreateCtx() {
  auto c = std::make_unique<Ctx>();
                                                                                  
                                                                                           
                                               
  static const char* kAllowT = "allow_unsafe_apis";
  static const char* kDumpT = "dump_shaders";
                                                                                     
                                                                                      
                                                                                  
                                                                                
                                                                                                                  
                                                                                                  
                                                                                                         
                                                                           
  static const char* kNoRobust = "disable_robustness";
  static const char* kNoWgInit = "disable_workgroup_init";
  const bool kWantDump = getenv("OFFICIAL_AETHER_MATCH_DAWN_DUMP") != nullptr;
  const char* allow = kAllowT;
  const char* kDbgToggles[2] = {kAllowT, kDumpT};
  wgpu::DawnTogglesDescriptor toggles{};
  toggles.enabledToggleCount = kWantDump ? 2 : 1;
  toggles.enabledToggles = kWantDump ? kDbgToggles : &allow;
  const wgpu::InstanceFeatureName timed = wgpu::InstanceFeatureName::TimedWaitAny;
  wgpu::InstanceDescriptor idesc{};
  idesc.nextInChain = &toggles;
  idesc.requiredFeatureCount = 1;
  idesc.requiredFeatures = &timed;
  c->instance = wgpu::CreateInstance(&idesc);
  if (!c->instance) {
    Log("instance creation failed (TimedWaitAny unavailable?)");
    return nullptr;
  }
  c->feat_packed_dot = c->instance.HasWGSLLanguageFeature(
      wgpu::WGSLLanguageFeatureName::Packed4x8IntegerDotProduct);

  wgpu::RequestAdapterOptions aopts{};
#if !defined(__APPLE__)
                                                                                               
                                                                                                      
                                                                                                    
                                                                                                                       
  aopts.backendType = wgpu::BackendType::Vulkan;
#endif
  std::string amsg;
  c->instance.WaitAny(
      c->instance.RequestAdapter(
          &aopts, wgpu::CallbackMode::WaitAnyOnly,
          [&](wgpu::RequestAdapterStatus st, wgpu::Adapter a,
              wgpu::StringView m) {
            if (st == wgpu::RequestAdapterStatus::Success) {
              c->adapter = std::move(a);
            } else {
              amsg = SV(m);
            }
          }),
      UINT64_MAX);
  if (!c->adapter) {
    Log("no adapter: %s", amsg.c_str());
    return nullptr;
  }
  c->feat_subgroups = c->adapter.HasFeature(wgpu::FeatureName::Subgroups);
  c->feat_sgmatrix = c->adapter.HasFeature(
      wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix);
  c->feat_timestamp = c->adapter.HasFeature(wgpu::FeatureName::TimestampQuery);
  {
    wgpu::AdapterInfo info{};
    wgpu::AdapterPropertiesSubgroupMatrixConfigs cfgs{};
    if (c->feat_sgmatrix) info.nextInChain = &cfgs;
    c->adapter.GetInfo(&info);
    c->adapter_name = SV(info.device);
                                                                                            
                                                                                          
    if (info.backendType == wgpu::BackendType::Null) {
      Log("adapter is the Null backend — refusing (would silently return garbage)");
      if (const char* home = std::getenv("HOME")) {
        const std::string fp = std::string(home) + "/Documents/matcher_backend.jsonl";
        if (FILE* f = std::fopen(fp.c_str(), "a")) {
          std::fprintf(f, "{\"adapter_refused\":\"Null backend\"}\n");
          std::fclose(f);
        }
      }
      c->adapter = nullptr;
      return nullptr;
    }
    c->subgroup_min = info.subgroupMinSize;
    c->subgroup_max = info.subgroupMaxSize;
    if (c->feat_sgmatrix) {
      for (size_t i = 0; i < cfgs.configCount; ++i) {
        const wgpu::SubgroupMatrixConfig& k = cfgs.configs[i];
        if (k.componentType == wgpu::SubgroupMatrixComponentType::F16 &&
            k.resultComponentType == wgpu::SubgroupMatrixComponentType::F32 &&
            k.M == 8 && k.N == 8 && k.K == 8) {
          c->sgcfg_f16_f32 = true;
        }
        if (k.componentType == wgpu::SubgroupMatrixComponentType::F32 &&
            k.resultComponentType == wgpu::SubgroupMatrixComponentType::F32 &&
            k.M == 8 && k.N == 8 && k.K == 8) {
          c->sgcfg_f32_8x8x8 = true;
        }
      }
    }
  }
  wgpu::Limits alim{};
  c->adapter.GetLimits(&alim);
  c->lim_storage = alim.maxComputeWorkgroupStorageSize;
  c->lim_invocations = alim.maxComputeInvocationsPerWorkgroup;
  c->lim_size_x = alim.maxComputeWorkgroupSizeX;

  const char* force = getenv("OFFICIAL_AETHER_MATCH_DAWN_KERNEL");
  const bool want_mma = !(force && std::strcmp(force, "tiled") == 0);
                                                                                         
                                                                                                     
                                                                                                                
                                                                                                                    
                                                                                  
  const bool want_mixed = !(force && std::strcmp(force, "plain") == 0);
                                                                                                         
                                                                                       
                                                                               
                                                                                                      
                                                                                          
                                                                                                
                                                                                                             
                                                                                                         
                                                                                                       
                                                                                                                 
  const bool apple_gpu = c->adapter_name.find("Apple") != std::string::npos;
  const bool sg32 = (c->subgroup_min == 32 && c->subgroup_max == 32) ||
                    (apple_gpu && c->subgroup_min <= 32 && c->subgroup_max >= 32);
  const bool mma_ok = c->feat_subgroups && c->feat_sgmatrix &&
                      c->sgcfg_f32_8x8x8 && sg32 &&
                      c->lim_invocations >= 512 &&
                      c->lim_size_x >= 512 && c->lim_storage >= 32768;
  std::vector<wgpu::FeatureName> feats;
  wgpu::Limits req{};
  wgpu::DeviceDescriptor dd{};
                                                                                                        
  if (ResolveDirectKnobs().f16 && c->adapter.HasFeature(wgpu::FeatureName::ShaderF16)) {
    feats.push_back(wgpu::FeatureName::ShaderF16);
    c->feat_f16 = true;
  }
                                                                                    
                                                                                                          
                                                                                                    
                                                                                
                                                                                        
                                                                                             
  const bool want_mma_explicit = force && std::strcmp(force, "mma") == 0;
  const bool want_tiled_explicit = force && std::strcmp(force, "tiled") == 0;
  if (!want_mma_explicit && !want_tiled_explicit) {
    c->backend = Backend::kBlocked;
    c->packed = true;
    c->mixed = false;
  } else if (want_mma_explicit && mma_ok) {
    c->backend = Backend::kMma;
    feats.push_back(wgpu::FeatureName::Subgroups);
    feats.push_back(wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_TSGPU") && c->feat_timestamp) {
      c->ts_on = true;
      feats.push_back(wgpu::FeatureName::TimestampQuery);
    }
    if (want_mixed && c->sgcfg_f16_f32 && c->adapter.HasFeature(wgpu::FeatureName::ShaderF16)) {
      c->mixed = true;
      c->packed = getenv("OFFICIAL_AETHER_MATCH_DAWN_UNPACKED") == nullptr;
      feats.push_back(wgpu::FeatureName::ShaderF16);
    }
    req.maxComputeWorkgroupStorageSize = alim.maxComputeWorkgroupStorageSize;
    req.maxComputeInvocationsPerWorkgroup =
        alim.maxComputeInvocationsPerWorkgroup;
    req.maxComputeWorkgroupSizeX = alim.maxComputeWorkgroupSizeX;
    dd.requiredLimits = &req;
  } else if (force && std::strcmp(force, "tiled") == 0) {
    c->backend = Backend::kTiled;
  } else {
                                                                                            
                                                                                                 
                                                                                                   
    c->backend = Backend::kBlocked;
    c->packed = true;
    c->mixed = false;
  }
  dd.requiredFeatureCount = feats.size();
  dd.requiredFeatures = feats.empty() ? nullptr : feats.data();
                                                                            
                                                         
                                                                                        
  const bool no_robust = getenv("OFFICIAL_AETHER_MATCH_DAWN_ROBUST") == nullptr;
  std::vector<const char*> dtoggles;
  if (no_robust) {
    dtoggles.push_back(kNoRobust);
    dtoggles.push_back(kNoWgInit);
  }
  if (kWantDump) dtoggles.push_back(kDumpT);
  wgpu::DawnTogglesDescriptor dtog{};
  if (!dtoggles.empty()) {
    dtog.enabledToggleCount = dtoggles.size();
    dtog.enabledToggles = dtoggles.data();
    dd.nextInChain = &dtog;
  }
  dd.SetDeviceLostCallback(
      wgpu::CallbackMode::AllowSpontaneous,
      [](const wgpu::Device&, wgpu::DeviceLostReason reason,
         wgpu::StringView message) {
        if (gTearingDown.load()) return;
        const int rc = (reason == wgpu::DeviceLostReason::Destroyed ||
                        reason == wgpu::DeviceLostReason::FailedCreation)
                           ? 8
                           : 7;
        gDeviceLostRc.store(rc);
        const std::string m = SV(message);
        static std::atomic<long> gLostCount{0};
        const long k = ++gLostCount;
        if (k <= 5 || (k % 100) == 0) {
          Log("device lost #%ld reason=%u rc=%d: %s", k, (unsigned)reason, rc,
              m.c_str());
        }
        StashLastError(std::string("DawnDeviceLost reason=") +
                       std::to_string((unsigned)reason) + " " + m);
      });
  dd.SetUncapturedErrorCallback(
      [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
        gErrorType.store((int)type);
        const std::string m = SV(message);
        static std::atomic<long> gErrCount{0};
        const long k = ++gErrCount;
        if (k <= 5 || (k % 100) == 0) {
          Log("uncaptured error #%ld type=%u: %s", k, (unsigned)type, m.c_str());
        }
        StashLastError(std::string("DawnError type=") +
                       std::to_string((unsigned)type) + " " + m);
      });
  std::string dmsg;
  c->instance.WaitAny(
      c->adapter.RequestDevice(
          &dd, wgpu::CallbackMode::WaitAnyOnly,
          [&](wgpu::RequestDeviceStatus st, wgpu::Device d, wgpu::StringView m) {
            if (st == wgpu::RequestDeviceStatus::Success) {
              c->device = std::move(d);
            } else {
              dmsg = SV(m);
            }
          }),
      UINT64_MAX);
  if (!c->device && c->backend == Backend::kMma) {
                                                                                                 
                                                                                              
                                                                                                                    
                                                                                                
    Log("MMA device creation failed (%s); degrading to blocked", dmsg.c_str());
    c->backend = Backend::kBlocked;
    c->packed = true;
    c->mixed = false;
    c->ts_on = false;
    dd.requiredFeatureCount = 0;
    dd.requiredFeatures = nullptr;
    dd.requiredLimits = nullptr;
    dmsg.clear();
    c->instance.WaitAny(
        c->adapter.RequestDevice(
            &dd, wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::RequestDeviceStatus st, wgpu::Device d, wgpu::StringView m) {
              if (st == wgpu::RequestDeviceStatus::Success) {
                c->device = std::move(d);
              } else {
                dmsg = SV(m);
              }
            }),
        UINT64_MAX);
  }
  if (!c->device) {
    Log("device creation failed: %s", dmsg.c_str());
    return nullptr;
  }
  if (kWantDump) {
                                                                       
    c->device.SetLoggingCallback(
        [](wgpu::LoggingType, wgpu::StringView msg) {
          std::fprintf(stderr, "%.*s\n", (int)msg.length, msg.data);
        });
  }
  c->queue = c->device.GetQueue();
  gDeviceLostRc.store(0);
  gErrorType.store(0);
  char info[512];
  std::snprintf(info, sizeof(info),
                "backend=%s adapter=\"%s\" subgroups=%d sgmatrix=%d "
                "f32_8x8x8=%d f16_f32=%d mixed=%d subgroup=[%u,%u] packed_dot=%d timestamp=%d "
                "limits[storage=%u invocations=%u sizeX=%u]%s",
                c->backend == Backend::kMma     ? "mma(fusedr128-db,V0)"
                : c->backend == Backend::kBlocked ? BlockedLabel()
                                                  : "tiled(dot4U8Packed,V1/V2)",
                c->adapter_name.c_str(), (int)c->feat_subgroups,
                (int)c->feat_sgmatrix, (int)c->sgcfg_f32_8x8x8,
                (int)c->sgcfg_f16_f32, (int)c->mixed, c->subgroup_min,
                c->subgroup_max, (int)c->feat_packed_dot, (int)c->feat_timestamp,
                c->lim_storage, c->lim_invocations, c->lim_size_x,
                (force && std::strcmp(force, "tiled") == 0) ? " (forced tiled)"
                                                            : "");
  c->backend_info = info;
                                                                                                                 
                                                                                                     
                                                                                              
                                                                                          
                                                                                                              
  if (const char* home = std::getenv("HOME")) {
    const std::string fp = std::string(home) + "/Documents/matcher_backend.jsonl";
    if (FILE* f = std::fopen(fp.c_str(), "a")) {
      std::fprintf(f,
                   "{\"dawn_init\":1,\"kernel\":\"%s\",\"adapter\":\"%s\","
                   "\"subgroups\":%d,\"sgmatrix\":%d,\"f32_8x8x8\":%d,"
                   "\"f16_f32\":%d,\"mixed\":%d,\"subgroup_min\":%u,"
                   "\"subgroup_max\":%u,\"packed_dot\":%d,\"storage\":%u,"
                   "\"invocations\":%u,\"sizeX\":%u}\n",
                   c->backend == Backend::kMma     ? "mma(fusedr128-db,V0)"
                   : c->backend == Backend::kBlocked ? BlockedLabel()
                                                     : "tiled(dot4U8Packed,V1/V2)",
                   c->adapter_name.c_str(), (int)c->feat_subgroups,
                   (int)c->feat_sgmatrix, (int)c->sgcfg_f32_8x8x8,
                   (int)c->sgcfg_f16_f32, (int)c->mixed, c->subgroup_min,
                   c->subgroup_max, (int)c->feat_packed_dot, c->lim_storage,
                   c->lim_invocations, c->lim_size_x);
      std::fclose(f);
    }
  }
  if (gInitLogged < 3) {
    ++gInitLogged;
    Log("%s", info);
  }
  return c;
}

                                                                           
                                          
Ctx* EnsureDawn() {
  if (gCtx && gDeviceLostRc.load() != 0) {
    ClearAllDescriptorResidencyForDeviceError();
    gTearingDown.store(true);
    gCtx.reset();
    gTearingDown.store(false);
    gDeviceLostRc.store(0);
  }
  if (gCtx) return gCtx.get();
  const double now = NowMs();
  if (now - gLastInitFailMs < 250.0) return nullptr;
  gCtx = CreateCtx();
  if (!gCtx) gLastInitFailMs = now;
  return gCtx.get();
}

                                                                                                                                                                        
                                                                         
                                                                         
                                                                          
                                                                       
                                                                  
                                                                        
                                                                                            
                                                                                     
                                                                                 
                                                                                             
                                                                                              
                                                                            
                                                                                             
                                                                               
                                                                            
                                                                                           
                                                                                
                                                                                                      
                                                                                                              
                                                                                           
                                                                                                    
                                                                                 
                                                                                       
                                                                                              
                                                                                         
                                                                                                        
                                                                                    
                                                                              
                                                                                             
                                                                                                    
                                                                                                   
                                                              
void AnchorAlarm(const char* what, const char* detail) {
  std::fprintf(stderr, "[pwofficial_gpu_match_dawn] 🔴 锚点失配:%s %s\n", what,
               detail ? detail : "");
  if (const char* home = std::getenv("HOME")) {
    const std::string fp = std::string(home) + "/Documents/matcher_backend.jsonl";
    if (FILE* f = std::fopen(fp.c_str(), "a")) {
      std::fprintf(f, "{\"anchor_mismatch\":\"%s\",\"detail\":\"%s\"}\n", what,
                   detail ? detail : "");
      std::fclose(f);
    }
  }
}

std::string Stage0Wgsl(const std::string& src) {
  std::string t = src;
                                                                                                            
                                                                                                       
  const std::string from = "    let pr0 = nextT + pe0 / 32u;";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("Stage0Wgsl", "静默退化为 no-op"); return src; }
  t.replace(p, from.size(), "    let pr0 = pe0 / 32u;");
  return t;
}

std::string ExtraBarrierWgsl(const std::string& src, int n) {
  std::string t = src;
                                                                                    
                                                                                   
                                                                                                                 
  const std::string one =
      "    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 3u * 8u, acc3, false, 32u);\n";
  const size_t q = t.find(one);
  if (q == std::string::npos) {
    AnchorAlarm("ExtraBarrierWgsl", "静默退化为 no-op");
    return src;
  }
  std::string add = one;
  for (int i = 0; i < n; ++i) add += "    workgroupBarrier();\n";
  t.replace(q, one.size(), add);
  return t;
}


std::string NoScanWgsl(const std::string& src) {
  std::string t = src;
  auto rep = [&t](const std::string& from, const std::string& to) {
    const size_t p = t.find(from);
    if (p == std::string::npos) {
                                                                                                           
      AnchorAlarm("NoScanWgsl", from.c_str());
      return false;
    }
    t.replace(p, from.size(), to);
    return true;
  };
  bool ok = true;
                                                                                   
                                                                                                
                                                                                                             
                                                                                                      
                                                                                          
                                                                                    
                                                                                             
                                                                                        
  ok &= rep("    for (var t = 0u; t < 8u; t = t + 1u) {",
            "    for (var t = 0u; t < 1u; t = t + 1u) {");
  ok &= rep("    for (var r = 0u; r < 8u; r = r + 1u) {",
            "    for (var r = 0u; r < 1u; r = r + 1u) {");
  if (!ok) { AnchorAlarm("NoScanWgsl", "静默退化为 no-op"); return src; }
  return t;
}

                                                                                               
                                                                                                            
                                                                                                 
                                                                                                             
                                                                        
std::string BLoadHoistWgsl(const std::string& src) {
  std::string t = src;
  const std::string from =
      "    for (var k = 0u; k < 16u; k = k + 1u) {\n"
      "      let b0 = subgroupMatrixLoad<Right>(&Bsh, (0u * 8u) * 128u + k * 8u, true, 128u);\n"
      "      let b1 = subgroupMatrixLoad<Right>(&Bsh, (1u * 8u) * 128u + k * 8u, true, 128u);\n"
      "      let b2 = subgroupMatrixLoad<Right>(&Bsh, (2u * 8u) * 128u + k * 8u, true, 128u);\n"
      "      let b3 = subgroupMatrixLoad<Right>(&Bsh, (3u * 8u) * 128u + k * 8u, true, 128u);\n";
  const std::string to =
      "    let b0 = subgroupMatrixLoad<Right>(&Bsh, (0u * 8u) * 128u, true, 128u);\n"
      "    let b1 = subgroupMatrixLoad<Right>(&Bsh, (1u * 8u) * 128u, true, 128u);\n"
      "    let b2 = subgroupMatrixLoad<Right>(&Bsh, (2u * 8u) * 128u, true, 128u);\n"
      "    let b3 = subgroupMatrixLoad<Right>(&Bsh, (3u * 8u) * 128u, true, 128u);\n"
      "    for (var k = 0u; k < 16u; k = k + 1u) {\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) {
    std::fprintf(stderr,
                 "[pwofficial_gpu_match_dawn] 🔴 BLoadHoistWgsl 锚点未命中\n");
    return src;
  }
  t.replace(p, from.size(), to);
  return t;
}

                                                                                            
                                                                                       
                                                                                                   
                                                                                                           
                                                                                                
                                                                                                                 
std::string HalfLoadWgsl(const std::string& src) {
  std::string t = src;
  auto rep = [&t](const std::string& from, const std::string& to) {
    const size_t p = t.find(from);
    if (p == std::string::npos) {
      AnchorAlarm("HalfLoadWgsl", from.c_str());
      return false;
    }
    t.replace(p, from.size(), to);
    return true;
  };
  bool ok = true;
  ok &= rep("      let b2 = subgroupMatrixLoad<Right>(&Bsh, (2u * 8u) * 128u + k * 8u, true, 128u);\n"
            "      let b3 = subgroupMatrixLoad<Right>(&Bsh, (3u * 8u) * 128u + k * 8u, true, 128u);\n",
            "");
  ok &= rep("      acc2 = subgroupMatrixMultiplyAccumulate(aFrag[k], b2, acc2);\n"
            "      acc3 = subgroupMatrixMultiplyAccumulate(aFrag[k], b3, acc3);\n",
            "      acc2 = subgroupMatrixMultiplyAccumulate(aFrag[15u - k], b0, acc2);\n"
            "      acc3 = subgroupMatrixMultiplyAccumulate(aFrag[15u - k], b1, acc3);\n");
  if (!ok) return src;
  return t;
}

                                                                                             
                                                                                               
                                                                                               
                                                                                                   
std::string Merge1Wgsl(const std::string& src) {
  std::string t = src;
  const std::string from = "  for (var w = 0u; w < U.numWg; w = w + 1u) {";
  const std::string to   = "  for (var w = 0u; w < min(U.numWg, 1u); w = w + 1u) {";
  const size_t p = t.find(from);
  if (p == std::string::npos) {
    AnchorAlarm("Merge1Wgsl", "锚点未命中");
    return src;
  }
  t.replace(p, from.size(), to);
  return t;
}

                                                                                              
                                                                                   
  
                                                                                                        
                                                                                                     
                                                                                    
                                                                                                      
                                                                                                           
std::string BStoreWgsl(const std::string& src) {
  std::string t = src;
  int hit = 0;
  for (int nt = 0; nt < 4; ++nt) {
    char from[160], to[160];
    std::snprintf(from, sizeof(from),
                  "subgroupMatrixLoad<Right>(&Bsh, (%du * 8u) * 128u + k * 8u, true, 128u)",
                  nt);
    std::snprintf(to, sizeof(to),
                  "subgroupMatrixLoad<Right>(&B, (col0 + %du * 8u) * 128u + k * 8u, true, 128u)",
                  nt);
    const size_t p = t.find(from);
    if (p == std::string::npos) continue;
    t.replace(p, std::strlen(from), to);
    ++hit;
  }
  if (hit != 4) {
    AnchorAlarm("BStoreWgsl", "四条 B 载入没全部命中");
    return src;
  }
  return t;
}

                                                                                            
                                                              
                                                                                             
                                                                              
                                                                                         
                                                                                                                 
                                                                                                
                                                                                                   
                                                                                        
                                              
std::string PackedWgsl(const std::string& src) {
  std::string t = src;
  auto rep_all = [&t](const std::string& from, const std::string& to) {
    size_t p = 0; int n = 0;
    while ((p = t.find(from, p)) != std::string::npos) {
      t.replace(p, from.size(), to);
      p += to.size();
      ++n;
    }
    return n;
  };
  auto rep = [&t](const std::string& from, const std::string& to) {
    const size_t p = t.find(from);
    if (p == std::string::npos) return false;
    t.replace(p, from.size(), to);
    return true;
  };
  bool ok = true;
  ok &= rep("var<storage, read> A : array<f16>", "var<storage, read> A : array<u32>");
  ok &= rep("var<storage, read> B : array<f16>", "var<storage, read> B : array<u32>");
                                                                                         
  ok &= (rep_all("e < BT * 128u", "e < BT * 32u") > 0);
  ok &= (rep_all(
             "      Bsh[e] = select(f16(0.0), B[brow * 128u + (e % 128u)], brow < U.numB);",
             "      let pu = select(0u, B[brow * 32u + (e % 32u)], brow < U.numB);\n"
             "      let po = (e / 32u) * 128u + (e % 32u) * 4u;\n"
             "      Bsh[po] = f16(pu & 255u);\n"
             "      Bsh[po + 1u] = f16((pu >> 8u) & 255u);\n"
             "      Bsh[po + 2u] = f16((pu >> 16u) & 255u);\n"
             "      Bsh[po + 3u] = f16(pu >> 24u);") > 0);
  ok &= (rep_all("        Bsh[e] = select(f16(0.0), B[brow * 128u + (e % 128u)], brow < U.numB);",
             "        let pu = select(0u, B[brow * 32u + (e % 32u)], brow < U.numB);\n"
             "        let po = (e / 32u) * 128u + (e % 32u) * 4u;\n"
             "        Bsh[po] = f16(pu & 255u);\n"
             "        Bsh[po + 1u] = f16((pu >> 8u) & 255u);\n"
             "        Bsh[po + 2u] = f16((pu >> 16u) & 255u);\n"
             "        Bsh[po + 3u] = f16(pu >> 24u);") >= 0);
  ok &= (rep_all("      let brow = 0u + e / 128u;", "      let brow = 0u + e / 32u;") >= 0);
  ok &= (rep_all("      let brow = col0 + e / 128u;", "      let brow = col0 + e / 32u;") >= 0);
  ok &= (rep_all("        let brow = nextT + e / 128u;", "        let brow = nextT + e / 32u;") >= 0);
                                                                                   
  ok &= (rep_all(
             "  var aFrag : array<Left, 16>;\n"
             "  for (var k = 0u; k < 16u; k = k + 1u) {\n"
             "    aFrag[k] = subgroupMatrixLoad<Left>(&A, aRow0 * 128u + k * 8u, false, 128u);\n"
             "  }",
             "  var aFrag : array<Left, 16>;\n"
             "  {\n"
             "    let pwv = sg >> 2u;\n"
             "    let pq = (sg & 3u) * 1024u;\n"
             "    let plane = lid % 32u;\n"
             "    for (var w = 0u; w < 4u; w = w + 1u) {\n"
             "      if (w == pwv) {\n"
             "        for (var e = plane; e < 256u; e = e + 32u) {\n"
             "          let u = A[aRow0 * 32u + e];\n"
             "          let o = pq + e * 4u;\n"
             "          Bsh[o] = f16(u & 255u);\n"
             "          Bsh[o + 1u] = f16((u >> 8u) & 255u);\n"
             "          Bsh[o + 2u] = f16((u >> 16u) & 255u);\n"
             "          Bsh[o + 3u] = f16(u >> 24u);\n"
             "        }\n"
             "      }\n"
             "      workgroupBarrier();\n"
             "      if (w == pwv) {\n"
             "        for (var k = 0u; k < 16u; k = k + 1u) {\n"
             "          aFrag[k] = subgroupMatrixLoad<Left>(&Bsh, pq + k * 8u, false, 128u);\n"
             "        }\n"
             "      }\n"
             "      workgroupBarrier();\n"
             "    }\n"
             "  }") > 0);
  if (!ok) { AnchorAlarm("PackedWgsl", "静默退化为 no-op"); return src; }
  return t;
}

                                                                                             
                                                                                               
                                                                                                    
                                                                                                      
                                                                                                          
std::string NoAProWgsl(const std::string& src) {
  std::string t = src;
  const std::string head = "  {\n    let pwv = sg >> 2u;\n";
  const size_t p = t.find(head);
  if (p == std::string::npos) {
    AnchorAlarm("NoAProWgsl", "前奏头未命中");
    return src;
  }
  const std::string tail = "      workgroupBarrier();\n    }\n  }";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) {
    AnchorAlarm("NoAProWgsl", "前奏尾未命中");
    return src;
  }
  const std::string repl =
      "  {\n"
      "    let pq = (sg & 3u) * 1024u;\n"
                                                                                       
                                                                               
      "    if (lid == 0u) { Bsh[0] = f16(A[aRow0 * 32u] & 255u); }\n"
      "    workgroupBarrier();\n"
      "    for (var k = 0u; k < 16u; k = k + 1u) {\n"
      "      aFrag[k] = subgroupMatrixLoad<Left>(&Bsh, pq + k * 8u, false, 128u);\n"
      "    }\n"
      "  }";
  t.replace(p, q + tail.size() - p, repl);
  return t;
}

                                                                                                                          
                                                                                                 
                                                                          
                                                                                                          
                                                                                                      
std::string BlkNoScanWgsl(const std::string& src) {
  std::string t = src;
  int hit = 0;
  auto rep = [&](const std::string& from, const std::string& to) {
    const size_t p = t.find(from);
    if (p == std::string::npos) { AnchorAlarm("BlkNoScanWgsl", from.c_str()); return; }
    t.replace(p, from.size(), to); ++hit;
  };
  rep("      for (var v = 0u; v < 16u; v = v + 1u) {",
      "      for (var v = 0u; v < 1u; v = v + 1u) {");
  rep("      for (var r = 0u; r < WGR; r = r + 1u) {",
      "      for (var r = 0u; r < 1u; r = r + 1u) {");
  return hit == 2 ? t : src;
}

std::string BlkNoStageWgsl(const std::string& src) {
  std::string t = src;
                                                                                                        
                                                                                                         
                                                                                
  const char* heads[] = {"      {\n        let side = lid / 128u;\n",
                         "      for (var side = 0u; side < 2u; side = side + 1u) {\n",
                         "      for (var pair = 0u; pair < 4u; pair = pair + 1u) {\n"};
  std::string head;
  size_t p = std::string::npos;
  for (const char* h : heads) { p = t.find(h); if (p != std::string::npos) { head = h; break; } }
  const std::string tail = "        S[o + 3u * 16u] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));\n      }\n";
  const size_t q = (p == std::string::npos) ? std::string::npos : t.find(tail, p);
  if (p == std::string::npos || q == std::string::npos) {
    AnchorAlarm("BlkNoStageWgsl", "暂存块首尾未命中");
    return src;
  }
  t.replace(p, q + tail.size() - p,
            "      if (lid == 0u) { S[0] = vec4<f32>(f32(A[row0 * 32u] & 255u), f32(B[col0 * 32u] & 255u), 0.0, 0.0); }\n");
  return t;
}

                                                                                                 
                                                                                                                 
std::string BlkPipeWgsl(const std::string& src) {
  std::string t = src;
                                                                                                      
                                                                                  
  const std::string from84 =
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let al = S[k * 16u + tr * 2u];\n"
      "        let ah = S[k * 16u + tr * 2u + 1u];\n"
      "        let b4 = S[512u + k * 16u + tc];\n";
  const size_t p84 = t.find(from84);
  if (p84 != std::string::npos) {
    t.replace(p84, from84.size(),
      "      var al = S[tr * 2u];\n"
      "      var ah = S[tr * 2u + 1u];\n"
      "      var b4 = S[512u + tc];\n"
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let kn = min(k + 1u, KC - 1u);\n"
      "        let aln = S[kn * 16u + tr * 2u];\n"
      "        let ahn = S[kn * 16u + tr * 2u + 1u];\n"
      "        let bn = S[512u + kn * 16u + tc];\n");
    const std::string tail84 = "        acc7 = acc7 + ah.w * b4;\n      }\n";
    const size_t q84 = t.find(tail84, p84);
    if (q84 == std::string::npos) { AnchorAlarm("BlkPipeWgsl", "8x4 循环尾未命中"); return src; }
    t.replace(q84, tail84.size(),
              "        acc7 = acc7 + ah.w * b4;\n"
              "        al = aln; ah = ahn; b4 = bn;\n"
              "      }\n");
    return t;
  }
  const std::string from =
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let a4 = S[k * 16u + tr];\n"
      "        let b4 = S[512u + k * 16u + tc];\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("BlkPipeWgsl", "k 循环头未命中(4x4/8x4 都不是)"); return src; }
  const std::string to =
      "      var a4 = S[tr];\n"
      "      var b4 = S[512u + tc];\n"
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let kn = min(k + 1u, KC - 1u);\n"
      "        let an = S[kn * 16u + tr];\n"
      "        let bn = S[512u + kn * 16u + tc];\n";
  t.replace(p, from.size(), to);
  const std::string tail =
      "        acc3 = acc3 + a4.w * b4;\n"
      "      }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("BlkPipeWgsl", "k 循环尾未命中"); return src; }
  t.replace(q, tail.size(),
            "        acc3 = acc3 + a4.w * b4;\n"
            "        a4 = an; b4 = bn;\n"
            "      }\n");
  return t;
}

                                                                             
                                                                                                 
                                                                                                    
std::string BlkFmaWgsl(const std::string& src) {
  std::string t = src;
  int n = 0;
  for (int i = 0; i < 8; ++i) {
    for (const char* src_name : {"a4", "al", "ah"}) {
      for (const char* comp : {"x", "y", "z", "w"}) {
        char from[96], to[96];
        std::snprintf(from, sizeof(from), "acc%d = acc%d + %s.%s * b4;", i, i, src_name, comp);
        std::snprintf(to, sizeof(to), "acc%d = fma(vec4<f32>(%s.%s), b4, acc%d);", i, src_name, comp, i);
        for (size_t p = t.find(from); p != std::string::npos; p = t.find(from, p + 1)) {
          t.replace(p, std::strlen(from), to); ++n;
        }
      }
    }
  }
  if (n == 0) { AnchorAlarm("BlkFmaWgsl", "一条 FMA 都没命中"); return src; }
  return t;
}

                                                                                                  
                                                                                                         
                                                                                                          
                                                                                               
                                                                                                                       
                                                                                        
std::string DirectTo44Wgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub subs[] = {
    {"@compute @workgroup_size(128)\nfn main(", "@compute @workgroup_size(256)\nfn main("},
    {"  let rq = row0 / 4u + tr * 2u;\n", "  let rq = row0 / 4u + tr;\n"},
    {"    var acc4 = vec4<f32>(0.0);\n    var acc5 = vec4<f32>(0.0);\n    var acc6 = vec4<f32>(0.0);\n    var acc7 = vec4<f32>(0.0);\n", ""},
    {"      let al = At[k * rowPad4 + rq];\n      let ah = At[k * rowPad4 + rq + 1u];\n", "      let al = At[k * rowPad4 + rq];\n"},
    {"      acc4 = acc4 + ah.x * b4;\n      acc5 = acc5 + ah.y * b4;\n      acc6 = acc6 + ah.z * b4;\n      acc7 = acc7 + ah.w * b4;\n", ""},
    {"    S[(tr * 8u + 0u) * 16u + tc] = acc0;\n    S[(tr * 8u + 1u) * 16u + tc] = acc1;\n    S[(tr * 8u + 2u) * 16u + tc] = acc2;\n    S[(tr * 8u + 3u) * 16u + tc] = acc3;\n    S[(tr * 8u + 4u) * 16u + tc] = acc4;\n    S[(tr * 8u + 5u) * 16u + tc] = acc5;\n    S[(tr * 8u + 6u) * 16u + tc] = acc6;\n    S[(tr * 8u + 7u) * 16u + tc] = acc7;\n",
     "    S[(tr * 4u + 0u) * 16u + tc] = acc0;\n    S[(tr * 4u + 1u) * 16u + tc] = acc1;\n    S[(tr * 4u + 2u) * 16u + tc] = acc2;\n    S[(tr * 4u + 3u) * 16u + tc] = acc3;\n"},
  };
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectTo44Wgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  return t;
}

                                                                                                                           
                                                                                                           
                                                                                                        
                                                                                                            
                                                                                                         
                                                                                                    
std::string DirectToGlobalScratchWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
                                                                                                                       
                                                                                                                                          
  const Sub common[] = {
    {"@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;\n",
     "@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;\n"
     "@group(0) @binding(9) var<storage, read_write> Scr : array<vec4<f32>>;\n"},
    {"var<workgroup> S : array<vec4<f32>, 1024>;\n", ""},
    {"        let d = S[lid * 16u + v];\n", "        let d = Scr[sb + lid * 16u + v];\n"},
    {"        let d = S[r * 16u + q][m];\n", "        let d = Scr[sb + r * 16u + q][m];\n"},
  };
  for (const Sub& sb : common) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectToGlobalScratchWgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
                                        
  {
    const char* rq84 = "  let rq = row0 / 4u + tr * 2u;\n";
    const char* rq44 = "  let rq = row0 / 4u + tr;\n";
    size_t p = t.find(rq84); const char* hit = rq84;
    if (p == std::string::npos) { p = t.find(rq44); hit = rq44; }
    if (p == std::string::npos) { AnchorAlarm("DirectToGlobalScratchWgsl", "rq 行未命中"); return src; }
    t.insert(p + std::strlen(hit), "  let sb = wg.x * 1024u;\n");
  }
                                  
  {
    const char* blk84 =
        "    workgroupBarrier();\n    S[(tr * 8u + 0u) * 16u + tc] = acc0;\n    S[(tr * 8u + 1u) * 16u + tc] = acc1;\n    S[(tr * 8u + 2u) * 16u + tc] = acc2;\n    S[(tr * 8u + 3u) * 16u + tc] = acc3;\n    S[(tr * 8u + 4u) * 16u + tc] = acc4;\n    S[(tr * 8u + 5u) * 16u + tc] = acc5;\n    S[(tr * 8u + 6u) * 16u + tc] = acc6;\n    S[(tr * 8u + 7u) * 16u + tc] = acc7;\n    workgroupBarrier();\n";
    const char* to84 =
        "    storageBarrier();\n    Scr[sb + (tr * 8u + 0u) * 16u + tc] = acc0;\n    Scr[sb + (tr * 8u + 1u) * 16u + tc] = acc1;\n    Scr[sb + (tr * 8u + 2u) * 16u + tc] = acc2;\n    Scr[sb + (tr * 8u + 3u) * 16u + tc] = acc3;\n    Scr[sb + (tr * 8u + 4u) * 16u + tc] = acc4;\n    Scr[sb + (tr * 8u + 5u) * 16u + tc] = acc5;\n    Scr[sb + (tr * 8u + 6u) * 16u + tc] = acc6;\n    Scr[sb + (tr * 8u + 7u) * 16u + tc] = acc7;\n    storageBarrier();\n";
    const char* blk44 =
        "    workgroupBarrier();\n    S[(tr * 4u + 0u) * 16u + tc] = acc0;\n    S[(tr * 4u + 1u) * 16u + tc] = acc1;\n    S[(tr * 4u + 2u) * 16u + tc] = acc2;\n    S[(tr * 4u + 3u) * 16u + tc] = acc3;\n    workgroupBarrier();\n";
    const char* to44 =
        "    storageBarrier();\n    Scr[sb + (tr * 4u + 0u) * 16u + tc] = acc0;\n    Scr[sb + (tr * 4u + 1u) * 16u + tc] = acc1;\n    Scr[sb + (tr * 4u + 2u) * 16u + tc] = acc2;\n    Scr[sb + (tr * 4u + 3u) * 16u + tc] = acc3;\n    storageBarrier();\n";
    size_t p = t.find(blk84);
    if (p != std::string::npos) t.replace(p, std::strlen(blk84), to84);
    else {
      p = t.find(blk44);
      if (p == std::string::npos) { AnchorAlarm("DirectToGlobalScratchWgsl", "结果交换块未命中(8x4/4x4 都不是)"); return src; }
      t.replace(p, std::strlen(blk44), to44);
    }
  }
  if (t.find("S[") != std::string::npos) { AnchorAlarm("DirectToGlobalScratchWgsl", "残留 S[ 引用"); return src; }
  return t;
}

                                                                                                           
                                                                                                     
                                                                                                                       
                                                                                                     
std::string DirectToTexWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub head = {"@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;\n",
                    "@group(0) @binding(8) var BtT : texture_2d<f32>;\n"};
  {
    const size_t p = t.find(head.from);
    if (p == std::string::npos) { AnchorAlarm("DirectToTexWgsl", head.from); return src; }
    t.replace(p, std::strlen(head.from), head.to);
  }
                                                                       
  const Sub pipeb[] = {
    {"    var b4 = Bt[cq];\n", "    var b4 = textureLoad(BtT, vec2<i32>(i32(cq), 0), 0);\n"},
    {"      let bn = Bt[min(k + 1u, KD - 1u) * colPad4 + cq];\n",
     "      let bn = textureLoad(BtT, vec2<i32>(i32(cq), i32(min(k + 1u, KD - 1u))), 0);\n"},
  };
  const Sub nopb = {"      let b4 = Bt[k * colPad4 + cq];\n",
                    "      let b4 = textureLoad(BtT, vec2<i32>(i32(cq), i32(k)), 0);\n"};
  if (t.find(pipeb[0].from) != std::string::npos) {
    for (const Sub& sb : pipeb) {
      const size_t p = t.find(sb.from);
      if (p == std::string::npos) { AnchorAlarm("DirectToTexWgsl", sb.from); return src; }
      t.replace(p, std::strlen(sb.from), sb.to);
    }
  } else {
    const size_t p = t.find(nopb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectToTexWgsl", "循环形态未命中(pipeb/nopb 都不是)"); return src; }
    t.replace(p, std::strlen(nopb.from), nopb.to);
  }
  if (t.find("Bt[") != std::string::npos) { AnchorAlarm("DirectToTexWgsl", "残留 Bt[ 引用"); return src; }
  return t;
}

                                                                                                                        
wgpu::TextureView EnsureBtTexture(Ctx& c, wgpu::Texture& tex, uint32_t& texW, uint32_t colPad4) {
  if (!tex || texW != colPad4) {
    if (tex) tex.Destroy();
    wgpu::TextureDescriptor d{};
    d.dimension = wgpu::TextureDimension::e2D;
    d.size = {colPad4, 128u, 1u};
    d.format = wgpu::TextureFormat::RGBA32Float;
    d.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    tex = c.device.CreateTexture(&d);
    texW = tex ? colPad4 : 0;
  }
  return tex ? tex.CreateView() : wgpu::TextureView();
}
void EncodeBtToTexture(wgpu::CommandEncoder& enc, wgpu::Buffer bt, wgpu::Texture tex, uint32_t colPad4) {
  wgpu::TexelCopyBufferInfo src{};
  src.buffer = bt;
  src.layout.offset = 0;
  src.layout.bytesPerRow = colPad4 * 16u;                                                          
  src.layout.rowsPerImage = 128u;
  wgpu::TexelCopyTextureInfo dst{};
  dst.texture = tex;
  wgpu::Extent3D ext{colPad4, 128u, 1u};
  enc.CopyBufferToTexture(&src, &dst, &ext);
}

                                                                                                            
                                                               
std::string DirectNoPipeBWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub subs[] = {
    {"    var b4 = Bt[cq];\n    for (var k = 0u; k < KD; k = k + 1u) {\n",
     "    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n"},
    {"      let bn = Bt[min(k + 1u, KD - 1u) * colPad4 + cq];\n", ""},
    {"      b4 = bn;\n", ""},
  };
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectNoPipeBWgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  return t;
}
                                                                                                                                  
                                              
std::string DirectPipeAWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
                                                                                             
  const Sub pipeb[] = {
    {"    var b4 = Bt[cq];\n    for (var k = 0u; k < KD; k = k + 1u) {\n      let al = At[k * rowPad4 + rq];\n",
     "    var b4 = Bt[cq];\n    var al = At[rq];\n    for (var k = 0u; k < KD; k = k + 1u) {\n      let aln = At[min(k + 1u, KD - 1u) * rowPad4 + rq];\n"},
    {"      b4 = bn;\n", "      b4 = bn;\n      al = aln;\n"},
  };
                                                                                                                                      
  const Sub nopb[] = {
    {"    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n      let al = At[k * rowPad4 + rq];\n",
     "    var al = At[rq];\n    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n      let aln = At[min(k + 1u, KD - 1u) * rowPad4 + rq];\n"},
    {"      acc3 = acc3 + al.w * b4;\n    }\n", "      acc3 = acc3 + al.w * b4;\n      al = aln;\n    }\n"},
  };
  const Sub* subs = (t.find(pipeb[0].from) != std::string::npos) ? pipeb : nopb;
  for (int i = 0; i < 2; ++i) {
    const size_t p = t.find(subs[i].from);
    if (p == std::string::npos) { AnchorAlarm("DirectPipeAWgsl", subs[i].from); return src; }
    t.replace(p, std::strlen(subs[i].from), subs[i].to);
  }
  if (t.find("let ah = At[") != std::string::npos) { AnchorAlarm("DirectPipeAWgsl", "只认 4x4 形态(发现 ah)"); return src; }
  return t;
}

                                                                                                                                   
                                                                                                                               
                                                                                             
static void ReplaceAll(std::string& t, const std::string& from, const std::string& to) {
  size_t p = 0;
  while ((p = t.find(from, p)) != std::string::npos) { t.replace(p, from.size(), to); p += to.size(); }
}
std::string DirectToTexAWgsl(const std::string& src) {
  std::string t = src;
  const std::string head = "@group(0) @binding(7) var<storage, read> At : array<vec4<f32>>;\n";
  const size_t p = t.find(head);
  if (p == std::string::npos) { AnchorAlarm("DirectToTexAWgsl", "At 声明未命中"); return src; }
  t.replace(p, head.size(), "@group(0) @binding(7) var AtT : texture_2d<f32>;\n");
  ReplaceAll(t, "At[k * rowPad4 + rq + 1u]", "textureLoad(AtT, vec2<i32>(i32(rq + 1u), i32(k)), 0)");
  ReplaceAll(t, "At[k * rowPad4 + rq]", "textureLoad(AtT, vec2<i32>(i32(rq), i32(k)), 0)");
  ReplaceAll(t, "At[min(k + 1u, KD - 1u) * rowPad4 + rq]", "textureLoad(AtT, vec2<i32>(i32(rq), i32(min(k + 1u, KD - 1u))), 0)");
  ReplaceAll(t, "At[rq]", "textureLoad(AtT, vec2<i32>(i32(rq), 0), 0)");
  if (t.find("At[") != std::string::npos) { AnchorAlarm("DirectToTexAWgsl", "残留 At[ 引用"); return src; }
  return t;
}

                                                                                                      
                                                                                                               
                                                                                                                             
                                                                                           
std::string DirectScanMemWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub subs[] = {
    {"  let myRow = row0 + lid;\n"
     "  var rowBest = 0.0;\n"
     "  var rowSecond = 0.0;\n"
     "  var rowBestI = -1;\n"
     "  if (U.colBase != 0u && lid < WGR && myRow < U.numA) {\n"
     "    let rp = RowP[myRow];\n"
     "    rowBest = rp.best;\n"
     "    rowSecond = rp.second;\n"
     "    rowBestI = rp.idx;\n"
     "  }\n",
     "  let myRow = row0 + lid;\n"},
    {"    if (lid < WGR) {\n"
     "      for (var v = 0u; v < 16u; v = v + 1u) {\n",
     "    if (lid < WGR) {\n"
     "      var rowBest = 0.0;\n"
     "      var rowSecond = 0.0;\n"
     "      var rowBestI = -1;\n"
     "      if (!(U.colBase == 0u && col0 == 0u) && myRow < U.numA) {\n"
     "        let rp = RowP[myRow];\n"
     "        rowBest = rp.best;\n"
     "        rowSecond = rp.second;\n"
     "        rowBestI = rp.idx;\n"
     "      }\n"
     "      for (var v = 0u; v < 16u; v = v + 1u) {\n"},
    {"        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }\n"
     "        else if (d.w > rowSecond) { rowSecond = d.w; }\n"
     "      }\n"
     "    } else if (lid < 2u * WGR) {\n",
     "        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }\n"
     "        else if (d.w > rowSecond) { rowSecond = d.w; }\n"
     "      }\n"
     "      if (myRow < U.numA) { RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI); }\n"
     "    } else if (lid < 2u * WGR) {\n"},
    {"  if (lid < WGR && myRow < U.numA) {\n"
     "    RowP[myRow] = ColPart(rowBest, rowSecond, rowBestI);\n"
     "    OutAB[myRow] = gatef(rowBest, rowSecond, rowBestI);\n"
     "  }\n",
     "  if (lid < WGR && myRow < U.numA) {\n"
     "    let rp = RowP[myRow];\n"
     "    OutAB[myRow] = gatef(rp.best, rp.second, rp.idx);\n"
     "  }\n"},
  };
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectScanMemWgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  return t;
}

                                                                                                                                                              
std::string DirectNoLoadWgsl(const std::string& src) {
                                                                                                                 
  std::string t = src;
  struct Sub { const char* from; const char* var; const char* hoist; };
  const Sub subs[] = {
                                                          
    {"      let al = vec4<f32>(At[k * rowPad4 + rq]);\n", "al", "    let al_c = vec4<f32>(At[rq]);\n"},
    {"      let ah = vec4<f32>(At[k * rowPad4 + rq + 1u]);\n", "ah", "    let ah_c = vec4<f32>(At[rq + 1u]);\n"},
    {"      let bn = vec4<f32>(Bt[min(k + 1u, KD - 1u) * colPad4 + cq]);\n", "bn", "    let bn_c = vec4<f32>(Bt[colPad4 + cq]);\n"},
    {"      let b4 = vec4<f32>(Bt[k * colPad4 + cq]);\n", "b4", "    let b4_c = vec4<f32>(Bt[cq]);\n"},
    {"      let aln = vec4<f32>(At[min(k + 1u, KD - 1u) * rowPad4 + rq]);\n", "aln", "    let aln_c = vec4<f32>(At[rowPad4 + rq]);\n"},
    {"      let al = At[k * rowPad4 + rq];\n", "al", "    let al_c = At[rq];\n"},
    {"      let ah = At[k * rowPad4 + rq + 1u];\n", "ah", "    let ah_c = At[rq + 1u];\n"},
    {"      let bn = Bt[min(k + 1u, KD - 1u) * colPad4 + cq];\n", "bn", "    let bn_c = Bt[colPad4 + cq];\n"},
    {"      let b4 = Bt[k * colPad4 + cq];\n", "b4", "    let b4_c = Bt[cq];\n"},
    {"      let aln = At[min(k + 1u, KD - 1u) * rowPad4 + rq];\n", "aln", "    let aln_c = At[rowPad4 + rq];\n"},
    {"      let al = textureLoad(AtT, vec2<i32>(i32(rq), i32(k)), 0);\n", "al", "    let al_c = textureLoad(AtT, vec2<i32>(i32(rq), 0), 0);\n"},
    {"      let ah = textureLoad(AtT, vec2<i32>(i32(rq + 1u), i32(k)), 0);\n", "ah", "    let ah_c = textureLoad(AtT, vec2<i32>(i32(rq + 1u), 0), 0);\n"},
    {"      let bn = textureLoad(BtT, vec2<i32>(i32(cq), i32(min(k + 1u, KD - 1u))), 0);\n", "bn", "    let bn_c = textureLoad(BtT, vec2<i32>(i32(cq), 1), 0);\n"},
    {"      let b4 = textureLoad(BtT, vec2<i32>(i32(cq), i32(k)), 0);\n", "b4", "    let b4_c = textureLoad(BtT, vec2<i32>(i32(cq), 0), 0);\n"},
    {"      let aln = textureLoad(AtT, vec2<i32>(i32(rq), i32(min(k + 1u, KD - 1u))), 0);\n", "aln", "    let aln_c = textureLoad(AtT, vec2<i32>(i32(rq), 1), 0);\n"},
  };
  std::string hoisted;
  int hits = 0;
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) continue;
    t.replace(p, std::strlen(sb.from), std::string("      let ") + sb.var + " = " + sb.var + "_c;\n");
    hoisted += sb.hoist; ++hits;
  }
  if (hits < 2) { AnchorAlarm("DirectNoLoadWgsl", "内层载入未命中"); return src; }
  const char* loop = "    for (var k = 0u; k < KD; k = k + 1u) {\n";
  const size_t q = t.find(loop);
  if (q == std::string::npos) { AnchorAlarm("DirectNoLoadWgsl", "k 循环头未命中"); return src; }
  t.insert(q, hoisted);
  return t;
}

                                                                                                                               
                                                                                                                                                  
std::string DirectFmaWgsl(const std::string& src) {
  std::string t = src;
  int hits = 0;
  const char* comps[] = {"x", "y", "z", "w"};
  const char* srcs[] = {"al", "ah", "a4"};
  for (int i = 0; i < 8; ++i) {
    for (const char* a : srcs) {
      for (const char* c : comps) {
        char from[96], to[96];
        std::snprintf(from, sizeof(from), "      acc%d = acc%d + %s.%s * b4;\n", i, i, a, c);
        std::snprintf(to, sizeof(to), "      acc%d = fma(vec4<f32>(%s.%s), b4, acc%d);\n", i, a, c, i);
        const size_t p = t.find(from);
        if (p != std::string::npos) { t.replace(p, std::strlen(from), to); ++hits; }
      }
    }
  }
  if (hits < 4) { AnchorAlarm("DirectFmaWgsl", "FMA 行未命中"); return src; }
  return t;
}
                                                                                                                          
                                                                                                                                
std::string DirectUnrollWgsl(const std::string& src, int n) {
  std::string t = src;
                                                                                                                                                                
                                                                                                                                                                                                          
  {
    const std::string ph = "    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + 1u) {\n"
                           "      let b4 = vec4<f32>(Bt[ib]);\n      let al = vec4<f32>(At[ia]);\n"
                           "      acc0 = acc0 + al.x * b4;\n      acc1 = acc1 + al.y * b4;\n      acc2 = acc2 + al.z * b4;\n      acc3 = acc3 + al.w * b4;\n"
                           "      ia = ia + rowPad4;\n      ib = ib + colPad4;\n    }\n";
    const size_t pp = t.find(ph);
    if (pp != std::string::npos) {
      if (n < 2 || 128 % n != 0) { AnchorAlarm("DirectUnrollWgsl", "n 必须整除 128"); return src; }
      std::string u = "    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + " + std::to_string(n) + "u) {\n";
      for (int i = 0; i < n; ++i) {
        const std::string si = std::to_string(i);
        u += "      let b4_" + si + " = vec4<f32>(Bt[ib]);\n      let al_" + si + " = vec4<f32>(At[ia]);\n";
        u += "      acc0 = acc0 + al_" + si + ".x * b4_" + si + ";\n      acc1 = acc1 + al_" + si + ".y * b4_" + si + ";\n";
        u += "      acc2 = acc2 + al_" + si + ".z * b4_" + si + ";\n      acc3 = acc3 + al_" + si + ".w * b4_" + si + ";\n";
        u += "      ia = ia + rowPad4;\n      ib = ib + colPad4;\n";
      }
      u += "    }\n";
      t.replace(pp, ph.size(), u);
      return t;
    }
  }
  const std::string head = "    for (var k = 0u; k < KD; k = k + 1u) {\n";
  const size_t p = t.find(head);
  if (p == std::string::npos) { AnchorAlarm("DirectUnrollWgsl", "循环头未命中"); return src; }
                                                                                   
  const std::string tail = "      acc3 = acc3 + al.w * b4;\n    }\n";
  const std::string tailF = "      acc3 = fma(vec4<f32>(al.w), b4, acc3);\n    }\n";
  size_t q = t.find(tail, p); size_t tlen = tail.size();
  if (q == std::string::npos) { q = t.find(tailF, p); tlen = tailF.size(); }
  if (q == std::string::npos) { AnchorAlarm("DirectUnrollWgsl", "循环尾未命中(只认 4x4)"); return src; }
  std::string body = t.substr(p + head.size(), q + tlen - 6 - (p + head.size()));                           
  if (body.find("let ah") != std::string::npos || body.find("bn") != std::string::npos) {
    AnchorAlarm("DirectUnrollWgsl", "只认 4x4+NOPB 形态"); return src;
  }
  std::string out;
  char hdr[96];
  std::snprintf(hdr, sizeof(hdr), "    for (var k0 = 0u; k0 < KD; k0 = k0 + %du) {\n", n);
  out += hdr;
  for (int u = 0; u < n; ++u) {
    char kk[32]; std::snprintf(kk, sizeof(kk), "(k0 + %du)", u);
    std::string b = body;
                                                                                                            
    std::string r;
    for (size_t i = 0; i < b.size(); ++i) {
      if (b[i] == 'k' && (i == 0 || !(std::isalnum((unsigned char)b[i-1]) || b[i-1] == '_')) &&
          (i + 1 >= b.size() || !(std::isalnum((unsigned char)b[i+1]) || b[i+1] == '_'))) r += kk;
      else r += b[i];
    }
    out += "      {\n" + r + "      }\n";
  }
  out += "    }\n";
  t.replace(p, q + tlen - p, out);
  return t;
}

                                                                                                                                      
                                                                                  
std::string DirectNoSyncWgsl(const std::string& src) {
  std::string t = src;
  const char* blk44 =
      "    workgroupBarrier();\n    S[(tr * 4u + 0u) * 16u + tc] = acc0;\n    S[(tr * 4u + 1u) * 16u + tc] = acc1;\n    S[(tr * 4u + 2u) * 16u + tc] = acc2;\n    S[(tr * 4u + 3u) * 16u + tc] = acc3;\n    workgroupBarrier();\n";
  const size_t p = t.find(blk44);
  if (p == std::string::npos) { AnchorAlarm("DirectNoSyncWgsl", "4x4 结果交换块未命中"); return src; }
                                                                                                       
  t.replace(p, std::strlen(blk44), "    if (lid == 0u) { S[tc] = acc0 + acc1 + acc2 + acc3; }\n");
  return t;
}

                                                                                                                
                                                                                                                               
                                                                                                                                         
                                                                                                                                                 
std::string DirectTo44W128Wgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub subs[] = {
    {"const WGR : u32 = 64u;\n", "const WGR : u32 = 32u;\n"},
    {"var<workgroup> S : array<vec4<f32>, 1024>;\n", "var<workgroup> S : array<vec4<f32>, 512>;\n"},
    {"@compute @workgroup_size(256)\nfn main(", "@compute @workgroup_size(128)\nfn main("},
    {"  let rowPad4 = ((U.numA + 63u) / 64u) * 16u;\n", "  let rowPad4 = ((U.numA + 31u) / 32u) * 8u;\n"},
    {"    } else if (lid < 2u * WGR) {\n", "    } else if (lid < WGR + 64u) {\n"},
  };
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectTo44W128Wgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  if (t.find("let ah = ") != std::string::npos) { AnchorAlarm("DirectTo44W128Wgsl", "只认 4x4 形态"); return src; }
  return t;
}

                                                                                                                                
                                                                                    
                                                                             
                                                                                                                   
                                                                                                                                        
                                                                                             
std::string DirectPScanWgsl(const std::string& src, bool ext = false) {
  std::string t = src;
  const bool w128 = t.find("const WGR : u32 = 32u;\n") != std::string::npos;
  const bool w256 = t.find("const WGR : u32 = 64u;\n") != std::string::npos &&
                    t.find("@compute @workgroup_size(256)\nfn main(") != std::string::npos;
  if (!w128 && !w256) { AnchorAlarm("DirectPScanWgsl", "只认 4x4 形态(128/WGR32 或 256/WGR64)"); return src; }
  const char* nt = w128 ? "const NT : u32 = 128u;\nconst CSEG : u32 = 2u;\nconst PB : u32 = 512u;\n" : "const NT : u32 = 256u;\nconst CSEG : u32 = 4u;\nconst PB : u32 = 1024u;\n";
  if (ext && !w128) { AnchorAlarm("DirectPScanWgsl", "ext 模式只认 W128(S 512→768 仍在 16 KiB 内)"); return src; }
  {
    const std::string wgr = w128 ? "const WGR : u32 = 32u;\n" : "const WGR : u32 = 64u;\n";
    const size_t p = t.find(wgr);
    t.insert(p + wgr.size(), nt);
  }
  const std::string from_head = "    if (lid < WGR) {\n      for (var v = 0u; v < 16u; v = v + 1u) {\n";
  const std::string from_tail = "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }\n    }\n";
  const size_t p = t.find(from_head);
  const size_t q = t.find(from_tail, p);
  if (p == std::string::npos || q == std::string::npos) { AnchorAlarm("DirectPScanWgsl", "扫描块未命中"); return src; }
  if (ext) {
    const std::string sdecl = "var<workgroup> S : array<vec4<f32>, 512>;\n";
    const size_t ps = t.find(sdecl);
    if (ps == std::string::npos) { AnchorAlarm("DirectPScanWgsl", "ext:S 声明未命中"); return src; }
    t.replace(ps, sdecl.size(), "var<workgroup> S : array<vec4<f32>, 768>;\n");
  }
  const std::string to =
    "    // [PSCAN] 行 partial:4 线程/行,各 16 列(4 个 vec4)\n"
    "    let prow = lid / 4u;\n"
    "    let psub = lid % 4u;\n"
    "    var pb = 0.0;\n    var ps = 0.0;\n    var pi = -1;\n"
    "    for (var v = 0u; v < 4u; v = v + 1u) {\n"
    "      let d = S[prow * 16u + psub * 4u + v];\n"
    "      let c0 = i32(col0 + (psub * 4u + v) * 4u);\n"
    "      if (d.x > pb) { ps = pb; pb = d.x; pi = c0; }\n      else if (d.x > ps) { ps = d.x; }\n"
    "      if (d.y > pb) { ps = pb; pb = d.y; pi = c0 + 1; }\n      else if (d.y > ps) { ps = d.y; }\n"
    "      if (d.z > pb) { ps = pb; pb = d.z; pi = c0 + 2; }\n      else if (d.z > ps) { ps = d.z; }\n"
    "      if (d.w > pb) { ps = pb; pb = d.w; pi = c0 + 3; }\n      else if (d.w > ps) { ps = d.w; }\n"
    "    }\n"
    "    // [PSCAN] 列 partial:线程负责列 lid%64、行段 lid/64(16 行)\n"
    "    let pcol = lid % 64u;\n"
    "    let pseg = lid / 64u;\n"
    "    let pq = pcol / 4u;\n"
    "    let pm = pcol % 4u;\n"
    "    var cb = 0.0;\n    var cs = 0.0;\n    var ci = -1;\n"
    "    for (var r = pseg * 16u; r < pseg * 16u + 16u; r = r + 1u) {\n"
    "      let d = S[r * 16u + pq][pm];\n"
    "      if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }\n      else if (d > cs) { cs = d; }\n"
    "    }\n"
    + std::string(ext ? "" : "    workgroupBarrier();\n") +
    (ext ? "    S[PB + lid] = vec4<f32>(pb, ps, bitcast<f32>(pi), 0.0);\n"
           "    S[PB + NT + lid] = vec4<f32>(cb, cs, bitcast<f32>(ci), 0.0);\n"
         : "    S[lid] = vec4<f32>(pb, ps, bitcast<f32>(pi), 0.0);\n"
           "    S[NT + lid] = vec4<f32>(cb, cs, bitcast<f32>(ci), 0.0);\n") +
    "    workgroupBarrier();\n"
    "    if (lid < WGR) {\n"
    "      for (var sg = 0u; sg < 4u; sg = sg + 1u) {\n"
    + std::string(ext ? "        let pp = S[PB + lid * 4u + sg];\n" : "        let pp = S[lid * 4u + sg];\n") +
    "        let b2 = pp.x;\n        let s2 = pp.y;\n        let i2 = bitcast<i32>(pp.z);\n"
    "        if (b2 > rowBest) { rowSecond = max(rowBest, s2); rowBest = b2; rowBestI = i2; }\n"
    "        else { rowSecond = max(rowSecond, b2); }\n"
    "      }\n"
    "    } else if (lid < WGR + 64u) {\n"
    "      let c = lid - WGR;\n"
    "      var mb = 0.0;\n      var ms = 0.0;\n      var mi = -1;\n"
    "      for (var sg = 0u; sg < CSEG; sg = sg + 1u) {\n"
    + std::string(ext ? "        let pp = S[PB + NT + sg * 64u + c];\n" : "        let pp = S[NT + sg * 64u + c];\n") +
    "        let b2 = pp.x;\n        let s2 = pp.y;\n        let i2 = bitcast<i32>(pp.z);\n"
    "        if (b2 > mb) { ms = max(mb, s2); mb = b2; mi = i2; }\n"
    "        else { ms = max(ms, b2); }\n"
    "      }\n"
    "      let gc = col0 + c;\n"
    "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(mb, ms, mi); }\n"
    "    }\n";
  t.replace(p, q + from_tail.size() - p, to);
                                                                                                                    
  return t;
}

                                                                                                               
                                                                                                                            
                                                                                                                                
                                                                                                
constexpr char kWgslXposePacked[] = R"WGSL(
struct XParams { n4 : u32, p1 : u32, p2 : u32, p3 : u32, };
@group(0) @binding(0) var<storage, read> Src : array<u32>;
@group(0) @binding(3) var<uniform> X : XParams;
@group(0) @binding(7) var<storage, read_write> Dst : array<u32>;
@compute @workgroup_size(64)
fn xpose(@builtin(global_invocation_id) gid : vec3<u32>) {
  let n4 = X.n4;
  let i = gid.x;
  if (i >= n4 * 32u) { return; }
  let q = i % n4;
  let w = i / n4;
  let x0 = Src[(4u * q + 0u) * 32u + w];
  let x1 = Src[(4u * q + 1u) * 32u + w];
  let x2 = Src[(4u * q + 2u) * 32u + w];
  let x3 = Src[(4u * q + 3u) * 32u + w];
  let o = (w * 4u) * n4 + q;
  Dst[o + 0u * n4] = (x0 & 255u) | ((x1 & 255u) << 8u) | ((x2 & 255u) << 16u) | ((x3 & 255u) << 24u);
  Dst[o + 1u * n4] = ((x0 >> 8u) & 255u) | (((x1 >> 8u) & 255u) << 8u) | (((x2 >> 8u) & 255u) << 16u) | (((x3 >> 8u) & 255u) << 24u);
  Dst[o + 2u * n4] = ((x0 >> 16u) & 255u) | (((x1 >> 16u) & 255u) << 8u) | (((x2 >> 16u) & 255u) << 16u) | (((x3 >> 16u) & 255u) << 24u);
  Dst[o + 3u * n4] = (x0 >> 24u) | ((x1 >> 24u) << 8u) | ((x2 >> 24u) << 16u) | ((x3 >> 24u) << 24u);
}
)WGSL";
std::string DirectPackedWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  const Sub decl[] = {
    {"@group(0) @binding(7) var<storage, read> At : array<vec4<f32>>;\n", "@group(0) @binding(7) var<storage, read> At : array<u32>;\n"},
    {"@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;\n", "@group(0) @binding(8) var<storage, read> Bt : array<u32>;\n"},
  };
  for (const Sub& sb : decl) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectPackedWgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  const Sub loads[] = {
    {"At[k * rowPad4 + rq + 1u]", "vec4<f32>(unpack4xU8(At[k * rowPad4 + rq + 1u]))"},
    {"At[k * rowPad4 + rq]", "vec4<f32>(unpack4xU8(At[k * rowPad4 + rq]))"},
    {"At[min(k + 1u, KD - 1u) * rowPad4 + rq]", "vec4<f32>(unpack4xU8(At[min(k + 1u, KD - 1u) * rowPad4 + rq]))"},
    {"At[rq]", "vec4<f32>(unpack4xU8(At[rq]))"},
    {"Bt[min(k + 1u, KD - 1u) * colPad4 + cq]", "vec4<f32>(unpack4xU8(Bt[min(k + 1u, KD - 1u) * colPad4 + cq]))"},
    {"Bt[k * colPad4 + cq]", "vec4<f32>(unpack4xU8(Bt[k * colPad4 + cq]))"},
    {"Bt[cq]", "vec4<f32>(unpack4xU8(Bt[cq]))"},
  };
  int hits = 0;
  for (const Sub& sb : loads) {
    size_t p = 0;
    while ((p = t.find(sb.from, p)) != std::string::npos) {
                                              
      if (p >= 11 && t.compare(p - 11, 11, "unpack4xU8(") == 0) { p += std::strlen(sb.from); continue; }
      t.replace(p, std::strlen(sb.from), sb.to); p += std::strlen(sb.to); ++hits;
    }
  }
  if (hits < 2) { AnchorAlarm("DirectPackedWgsl", "载入未命中"); return src; }
  return t;
}

                                                                                                                               
                                                                                                                            
                                                                                                                                    
constexpr char kWgslXposeF16[] = R"WGSL(
enable f16;
struct XParams { n4 : u32, p1 : u32, p2 : u32, p3 : u32, };
@group(0) @binding(0) var<storage, read> Src : array<u32>;
@group(0) @binding(3) var<uniform> X : XParams;
@group(0) @binding(7) var<storage, read_write> Dst : array<vec4<f16>>;
@compute @workgroup_size(64)
fn xpose(@builtin(global_invocation_id) gid : vec3<u32>) {
  let n4 = X.n4;
  let i = gid.x;
  if (i >= n4 * 32u) { return; }
  let q = i % n4;
  let w = i / n4;
  let x0 = Src[(4u * q + 0u) * 32u + w];
  let x1 = Src[(4u * q + 1u) * 32u + w];
  let x2 = Src[(4u * q + 2u) * 32u + w];
  let x3 = Src[(4u * q + 3u) * 32u + w];
  let o = (w * 4u) * n4 + q;
  Dst[o + 0u * n4] = vec4<f16>(f16(x0 & 255u), f16(x1 & 255u), f16(x2 & 255u), f16(x3 & 255u));
  Dst[o + 1u * n4] = vec4<f16>(f16((x0 >> 8u) & 255u), f16((x1 >> 8u) & 255u), f16((x2 >> 8u) & 255u), f16((x3 >> 8u) & 255u));
  Dst[o + 2u * n4] = vec4<f16>(f16((x0 >> 16u) & 255u), f16((x1 >> 16u) & 255u), f16((x2 >> 16u) & 255u), f16((x3 >> 16u) & 255u));
  Dst[o + 3u * n4] = vec4<f16>(f16(x0 >> 24u), f16(x1 >> 24u), f16(x2 >> 24u), f16(x3 >> 24u));
}
)WGSL";
std::string DirectF16Wgsl(const std::string& src) {
  std::string t = "enable f16;\n" + src;
  struct Sub { const char* from; const char* to; };
  const Sub decl[] = {
    {"@group(0) @binding(7) var<storage, read> At : array<vec4<f32>>;\n", "@group(0) @binding(7) var<storage, read> At : array<vec4<f16>>;\n"},
    {"@group(0) @binding(8) var<storage, read> Bt : array<vec4<f32>>;\n", "@group(0) @binding(8) var<storage, read> Bt : array<vec4<f16>>;\n"},
  };
  for (const Sub& sb : decl) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectF16Wgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  const char* loads[] = {"At[k * rowPad4 + rq + 1u]", "At[k * rowPad4 + rq]", "At[min(k + 1u, KD - 1u) * rowPad4 + rq]", "At[rq]",
                         "Bt[min(k + 1u, KD - 1u) * colPad4 + cq]", "Bt[k * colPad4 + cq]", "Bt[cq]"};
  int hits = 0;
  for (const char* ld : loads) {
    const std::string from = ld; const std::string to = "vec4<f32>(" + from + ")";
    size_t p = 0;
    while ((p = t.find(from, p)) != std::string::npos) {
      if (p >= 10 && t.compare(p - 10, 10, "vec4<f32>(") == 0) { p += from.size(); continue; }
      t.replace(p, from.size(), to); p += to.size(); ++hits;
    }
  }
  if (hits < 2) { AnchorAlarm("DirectF16Wgsl", "载入未命中"); return src; }
  return t;
}

                                                                                                            
                                                                                                                                        
                                                                                                                         
                                                                                                                                          
std::string DirectTMapWgsl(const std::string& src) {
  std::string t = src;
  const bool w128 = t.find("@compute @workgroup_size(128)\nfn main(") != std::string::npos;
  const char* nr = w128 ? "8u" : "16u";                                                               
  const std::string from = "  let tr = lid / 16u;\n  let tc = lid % 16u;\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("DirectTMapWgsl", "tr/tc 映射未命中"); return src; }
  t.replace(p, from.size(), std::string("  let tr = lid % ") + nr + ";\n  let tc = lid / " + nr + ";\n");
  return t;
}

                                                                                                                                             
                                                                                                                                               
                                                                                                                                          
std::string DirectUnrollHWgsl(const std::string& src, int n, bool onlyB = false) {
  std::string t = src;
  const std::string head = "    for (var k = 0u; k < KD; k = k + 1u) {\n";
  const size_t p = t.find(head);
  if (p == std::string::npos) { AnchorAlarm("DirectUnrollHWgsl", "循环头未命中"); return src; }
  const std::string tail = "      acc3 = acc3 + al.w * b4;\n    }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("DirectUnrollHWgsl", "循环尾未命中(只认 4x4 未 fma 化)"); return src; }
  const std::string body = t.substr(p + head.size(), q + tail.size() - 6 - (p + head.size()));
  if (body.find("bn") != std::string::npos || body.find("aln") != std::string::npos || body.find("let ah") != std::string::npos) {
    AnchorAlarm("DirectUnrollHWgsl", "只认 4x4+NOPB(无预取)形态"); return src;
  }
  std::vector<std::string> loads, comps;
  size_t i = 0;
  while (i < body.size()) {
    size_t e = body.find('\n', i); if (e == std::string::npos) e = body.size();
    const std::string line = body.substr(i, e - i + 1);
    if (line.rfind("      let al = ", 0) == 0 || line.rfind("      let b4 = ", 0) == 0) loads.push_back(line); else comps.push_back(line);
    i = e + 1;
  }
  if (loads.size() != 2) { AnchorAlarm("DirectUnrollHWgsl", "载入行数不是 2"); return src; }
  auto subst_k = [](const std::string& b, const std::string& kk) {
    std::string r;
    for (size_t j = 0; j < b.size(); ++j) {
      if (b[j] == 'k' && (j == 0 || !(std::isalnum((unsigned char)b[j-1]) || b[j-1] == '_')) &&
          (j + 1 >= b.size() || !(std::isalnum((unsigned char)b[j+1]) || b[j+1] == '_'))) r += kk; else r += b[j];
    }
    return r;
  };
  auto rename = [](std::string l, const char* from, const std::string& to) {
    size_t pos = 0; while ((pos = l.find(from, pos)) != std::string::npos) { l.replace(pos, std::strlen(from), to); pos += to.size(); } return l;
  };
  std::string out;
  char hdr[96]; std::snprintf(hdr, sizeof(hdr), "    for (var k0 = 0u; k0 < KD; k0 = k0 + %du) {\n", n);
  out += hdr;
                                                                                                                                                                  
  for (int u = 0; u < n; ++u) {
    char kk[32]; std::snprintf(kk, sizeof(kk), "(k0 + %du)", u);
    for (const std::string& l : loads) {
      if (onlyB && l.rfind("      let al = ", 0) == 0) continue;
      std::string x = subst_k(l, kk);
      x = rename(x, "let al = ", "let al_" + std::to_string(u) + " = ");
      x = rename(x, "let b4 = ", "let b4_" + std::to_string(u) + " = ");
      out += x;
    }
  }
  for (int u = 0; u < n; ++u) {
    char kk[32]; std::snprintf(kk, sizeof(kk), "(k0 + %du)", u);
    if (onlyB) {
      for (const std::string& l : loads) {
        if (l.rfind("      let al = ", 0) != 0) continue;
        std::string x = subst_k(l, kk);
        out += rename(x, "let al = ", "let al_" + std::to_string(u) + " = ");
      }
    }
    for (const std::string& l : comps) {
      std::string x = rename(l, "al.", "al_" + std::to_string(u) + ".");
      x = rename(x, "b4;", "b4_" + std::to_string(u) + ";");
      out += x;
    }
  }
  out += "    }\n";
  t.replace(p, q + tail.size() - p, out);
  return t;
}

                                                                                                                       
                                                                                                         
                                                                                                                                       
                                                            
std::string DirectTailBWgsl(const std::string& src) {
  std::string t = src;
  const char* heads[] = {
    "    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = vec4<f32>(Bt[k * colPad4 + cq]);\n",
    "    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n",
  };
  const char* firsts[] = { "vec4<f32>(Bt[cq])", "Bt[cq]" };
  const char* nexts[]  = { "vec4<f32>(Bt[min(k + 1u, KD - 1u) * colPad4 + cq])", "Bt[min(k + 1u, KD - 1u) * colPad4 + cq]" };
  int which = -1; size_t p = std::string::npos;
  for (int i = 0; i < 2; ++i) { p = t.find(heads[i]); if (p != std::string::npos) { which = i; break; } }
  if (which < 0) { AnchorAlarm("DirectTailBWgsl", "只认 4x4+NOPB 的循环头"); return src; }
  t.replace(p, std::strlen(heads[which]),
            std::string("    var b4 = ") + firsts[which] + ";\n    for (var k = 0u; k < KD; k = k + 1u) {\n");
  const std::string tail = "      acc3 = acc3 + al.w * b4;\n    }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("DirectTailBWgsl", "循环尾未命中"); return src; }
  t.replace(q, tail.size(), std::string("      acc3 = acc3 + al.w * b4;\n      b4 = ") + nexts[which] + ";\n    }\n");
  return t;
}

                                                                                                                                             
                                                                                                                                                                 
std::string DirectTailB2Wgsl(const std::string& src) {
  std::string t = src;
  const char* heads[] = {
    "    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = vec4<f32>(Bt[k * colPad4 + cq]);\n",
    "    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n",
  };
  const char* firsts[] = { "vec4<f32>(Bt[cq])", "Bt[cq]" };
  const char* nexts[]  = { "vec4<f32>(Bt[(k + 1u) * colPad4 + cq])", "Bt[(k + 1u) * colPad4 + cq]" };
  const char* alast[]  = { "vec4<f32>(At[(KD - 1u) * rowPad4 + rq])", "At[(KD - 1u) * rowPad4 + rq]" };
  int which = -1; size_t p = std::string::npos;
  for (int i = 0; i < 2; ++i) { p = t.find(heads[i]); if (p != std::string::npos) { which = i; break; } }
  if (which < 0) { AnchorAlarm("DirectTailB2Wgsl", "只认 4x4+NOPB 的循环头"); return src; }
  t.replace(p, std::strlen(heads[which]),
            std::string("    var b4 = ") + firsts[which] + ";\n    for (var k = 0u; k < KD - 1u; k = k + 1u) {\n");
  const std::string tail = "      acc3 = acc3 + al.w * b4;\n    }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("DirectTailB2Wgsl", "循环尾未命中"); return src; }
                                                               
  const std::string body = t.substr(p, q - p);
  const size_t la = body.find("      let al = ");
  const size_t le = body.find('\n', la);
  if (la == std::string::npos || le == std::string::npos) { AnchorAlarm("DirectTailB2Wgsl", "al 载入行未命中"); return src; }
  std::string alLine = body.substr(la, le - la + 1);
                                              
  {
    std::string r; for (size_t j = 0; j < alLine.size(); ++j) {
      if (alLine[j] == 'k' && (j == 0 || !(std::isalnum((unsigned char)alLine[j-1]) || alLine[j-1] == '_')) &&
          (j + 1 >= alLine.size() || !(std::isalnum((unsigned char)alLine[j+1]) || alLine[j+1] == '_'))) r += "(KD - 1u)"; else r += alLine[j];
    }
    alLine = r;
  }
  const std::string fmas =
    "      acc0 = acc0 + al.x * b4;\n      acc1 = acc1 + al.y * b4;\n      acc2 = acc2 + al.z * b4;\n      acc3 = acc3 + al.w * b4;\n";
  std::string epilogue = "    {\n" + alLine + fmas + "    }\n";
  t.replace(q, tail.size(), std::string("      acc3 = acc3 + al.w * b4;\n      b4 = ") + nexts[which] + ";\n    }\n" + epilogue);
  (void)alast;
  return t;
}

                                                                                                                                                        
                                                                                                                        
std::string DirectScanSelWgsl(const std::string& src) {
  std::string t = src;
  const std::string from = "        let d = S[r * 16u + q][m];\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("DirectScanSelWgsl", "列扫描分量索引未命中"); return src; }
  t.replace(p, from.size(),
    "        let dv = S[r * 16u + q];\n"
    "        let d = select(select(dv.x, dv.y, m == 1u), select(dv.z, dv.w, m == 3u), m >= 2u);\n");
  return t;
}

                                                                                                                    
std::string DirectNoScanRowWgsl(const std::string& src) {
  std::string t = src; const std::string from = "      for (var v = 0u; v < 16u; v = v + 1u) {\n";
  const size_t p = t.find(from); if (p == std::string::npos) { AnchorAlarm("DirectNoScanRowWgsl", "行扫描未命中"); return src; }
  t.replace(p, from.size(), "      for (var v = 0u; v < 1u; v = v + 1u) {\n"); return t;
}
std::string DirectNoScanColWgsl(const std::string& src) {
  std::string t = src; const std::string from = "      for (var r = 0u; r < WGR; r = r + 1u) {\n";
  const size_t p = t.find(from); if (p == std::string::npos) { AnchorAlarm("DirectNoScanColWgsl", "列扫描未命中"); return src; }
  t.replace(p, from.size(), "      for (var r = 0u; r < 1u; r = r + 1u) {\n"); return t;
}

                                                                                                                                                                   
                                                                                                                                                                             
                                                                                               
std::string DirectScanU4Wgsl(const std::string& src) {
  std::string t = src;
              
  {
    const std::string from =
      "      for (var r = 0u; r < WGR; r = r + 1u) {\n"
      "        let d = S[r * 16u + q][m];\n"
      "        if (d > cb) { cs = cb; cb = d; ci = i32(row0 + r); }\n"
      "        else if (d > cs) { cs = d; }\n"
      "      }\n";
    const size_t p = t.find(from);
    if (p == std::string::npos) { AnchorAlarm("DirectScanU4Wgsl", "列扫描循环未命中"); return src; }
    std::string to = "      for (var r = 0u; r < WGR; r = r + 4u) {\n";
    for (int i = 0; i < 4; ++i) { char b[96]; std::snprintf(b, sizeof(b), "        let d%d = S[(r + %du) * 16u + q][m];\n", i, i); to += b; }
    for (int i = 0; i < 4; ++i) {
      char b[256];
      std::snprintf(b, sizeof(b), "        if (d%d > cb) { cs = cb; cb = d%d; ci = i32(row0 + r + %du); }\n        else if (d%d > cs) { cs = d%d; }\n", i, i, i, i, i);
      to += b;
    }
    to += "      }\n";
    t.replace(p, from.size(), to);
  }
              
  {
    const std::string head = "      for (var v = 0u; v < 16u; v = v + 1u) {\n        let d = S[lid * 16u + v];\n        let c0 = i32(col0 + v * 4u);\n";
    const size_t p = t.find(head);
    if (p == std::string::npos) { AnchorAlarm("DirectScanU4Wgsl", "行扫描循环头未命中"); return src; }
    const std::string tail = "        if (d.w > rowBest) { rowSecond = rowBest; rowBest = d.w; rowBestI = c0 + 3; }\n        else if (d.w > rowSecond) { rowSecond = d.w; }\n      }\n";
    const size_t q = t.find(tail, p);
    if (q == std::string::npos) { AnchorAlarm("DirectScanU4Wgsl", "行扫描循环尾未命中"); return src; }
                                                                                   
    const std::string body = t.substr(p + head.size(), q + tail.size() - 8 - (p + head.size()));
    std::string to = "      for (var v = 0u; v < 16u; v = v + 4u) {\n";
    for (int i = 0; i < 4; ++i) { char b[96]; std::snprintf(b, sizeof(b), "        let e%d = S[lid * 16u + v + %du];\n", i, i); to += b; }
    for (int i = 0; i < 4; ++i) {
      char b[96]; std::snprintf(b, sizeof(b), "        let c%d = i32(col0 + (v + %du) * 4u);\n", i, i); to += b;
      std::string x = body;
                                  
      std::string y; for (size_t j = 0; j < x.size(); ++j) {
        if (x.compare(j, 2, "d.") == 0) { y += "e" + std::to_string(i) + "."; ++j; }
        else if (x.compare(j, 2, "c0") == 0 && (j + 2 >= x.size() || !std::isalnum((unsigned char)x[j+2]))) { y += "c" + std::to_string(i); ++j; }
        else y += x[j];
      }
      to += y;
    }
    to += "      }\n";
    t.replace(p, q + tail.size() - p, to);
  }
  return t;
}

                                                                                                                                     
                                                                                                                                           
                                                                                                                                         
                                                                                                                                            
std::string DirectW64Wgsl(const std::string& src) {
  if (src.find("const WGR : u32 = 32u;\n") == std::string::npos || src.find("const BT : u32 = 64u;\n") == std::string::npos) {
    AnchorAlarm("DirectW64Wgsl", "只认 W128(WGR=32,BT=64)"); return src;
  }
  struct Sub { const char* from; const char* to; };
  static const Sub subs[] = {
    {"const BT : u32 = 64u;\n", "const BT : u32 = 32u;\n"},
    {"var<workgroup> S : array<vec4<f32>, 512>;\n", "var<workgroup> S : array<vec4<f32>, 256>;\n"},
    {"@compute @workgroup_size(128)\nfn main(", "@compute @workgroup_size(64)\nfn main("},
    {"  let tr = lid / 16u;\n  let tc = lid % 16u;\n", "  let tr = lid / 8u;\n  let tc = lid % 8u;\n"},
    {"    S[(tr * 4u + 0u) * 16u + tc] = acc0;\n    S[(tr * 4u + 1u) * 16u + tc] = acc1;\n    S[(tr * 4u + 2u) * 16u + tc] = acc2;\n    S[(tr * 4u + 3u) * 16u + tc] = acc3;\n",
     "    S[(tr * 4u + 0u) * 8u + tc] = acc0;\n    S[(tr * 4u + 1u) * 8u + tc] = acc1;\n    S[(tr * 4u + 2u) * 8u + tc] = acc2;\n    S[(tr * 4u + 3u) * 8u + tc] = acc3;\n"},
    {"      for (var v = 0u; v < 16u; v = v + 1u) {\n        let d = S[lid * 16u + v];\n", "      for (var v = 0u; v < 8u; v = v + 1u) {\n        let d = S[lid * 8u + v];\n"},
    {"    } else if (lid < WGR + 64u) {\n", "    } else if (lid < WGR + 32u) {\n"},
    {"        let d = S[r * 16u + q][m];\n", "        let d = S[r * 8u + q][m];\n"},
  };
  std::string t = src;
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectW64Wgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  return t;
}
                                                                                                                             
                                                                                                                                                                        
                                                                                                       
std::string DirectPtrWgsl(const std::string& src) {
  struct Sub { const char* from; const char* to; };
  static const Sub subs[] = {
    {"    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = vec4<f32>(Bt[k * colPad4 + cq]);\n      let al = vec4<f32>(At[k * rowPad4 + rq]);\n",
     "    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = vec4<f32>(Bt[ib]);\n      let al = vec4<f32>(At[ia]);\n"},
    {"    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[k * colPad4 + cq];\n      let al = At[k * rowPad4 + rq];\n",
     "    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + 1u) {\n      let b4 = Bt[ib];\n      let al = At[ia];\n"},
  };
  const std::string tail_from = "      acc3 = acc3 + al.w * b4;\n    }\n";
  const std::string tail_to = "      acc3 = acc3 + al.w * b4;\n      ia = ia + rowPad4;\n      ib = ib + colPad4;\n    }\n";
  for (const Sub& sb : subs) {
    const size_t p = src.find(sb.from);
    if (p == std::string::npos) continue;
    std::string t = src;
    t.replace(p, std::strlen(sb.from), sb.to);
    const size_t q = t.find(tail_from, p);
    if (q == std::string::npos) { AnchorAlarm("DirectPtrWgsl", "循环尾未命中"); return src; }
    t.replace(q, tail_from.size(), tail_to);
    return t;
  }
  AnchorAlarm("DirectPtrWgsl", "只认 NOPB 形态的 GEMM 循环");
  return src;
}
                                                                                                                         
                                                                                                                                     
                                                                                                                                                         
                                                                                                                                 
constexpr char kWgslXposeKP[] = R"WGSL(
struct XParams { n4 : u32, p1 : u32, p2 : u32, p3 : u32, };
@group(0) @binding(0) var<storage, read> Src : array<u32>;
@group(0) @binding(3) var<uniform> X : XParams;
@group(0) @binding(7) var<storage, read_write> Dst : array<vec4<u32>>;
fn pk(a : u32, b : u32) -> u32 { return pack2x16float(vec2<f32>(f32(a), f32(b))); }
@compute @workgroup_size(64)
fn xpose(@builtin(global_invocation_id) gid : vec3<u32>) {
  let n4 = X.n4;
  let i = gid.x;
  if (i >= n4 * 32u) { return; }
  let q = i % n4;
  let w = i / n4;
  let x0 = Src[(4u * q + 0u) * 32u + w];
  let x1 = Src[(4u * q + 1u) * 32u + w];
  let x2 = Src[(4u * q + 2u) * 32u + w];
  let x3 = Src[(4u * q + 3u) * 32u + w];
  // k = 4w + j,j = 字节序;kp = 2w + (j >> 1),半 = j & 1
  let o = (w * 2u) * n4 + q;
  Dst[o] = vec4<u32>(pk(x0 & 255u, x1 & 255u), pk(x2 & 255u, x3 & 255u),
                     pk((x0 >> 8u) & 255u, (x1 >> 8u) & 255u), pk((x2 >> 8u) & 255u, (x3 >> 8u) & 255u));
  Dst[o + n4] = vec4<u32>(pk((x0 >> 16u) & 255u, (x1 >> 16u) & 255u), pk((x2 >> 16u) & 255u, (x3 >> 16u) & 255u),
                          pk(x0 >> 24u, x1 >> 24u), pk(x2 >> 24u, x3 >> 24u));
}
)WGSL";
std::string DirectKPWgsl(const std::string& src) {
  std::string t = src;
  struct Sub { const char* from; const char* to; };
  static const Sub subs[] = {
    {"@group(0) @binding(7) var<storage, read> At : array<vec4<f16>>;\n", "@group(0) @binding(7) var<storage, read> At : array<vec4<u32>>;\n"},
    {"@group(0) @binding(8) var<storage, read> Bt : array<vec4<f16>>;\n", "@group(0) @binding(8) var<storage, read> Bt : array<vec4<u32>>;\n"},
    {"    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + 1u) {\n"
     "      let b4 = vec4<f32>(Bt[ib]);\n      let al = vec4<f32>(At[ia]);\n"
     "      acc0 = acc0 + al.x * b4;\n      acc1 = acc1 + al.y * b4;\n      acc2 = acc2 + al.z * b4;\n      acc3 = acc3 + al.w * b4;\n"
     "      ia = ia + rowPad4;\n      ib = ib + colPad4;\n    }\n",
     "    var ia = rq;\n    var ib = cq;\n    for (var k = 0u; k < KD; k = k + 2u) {\n"
     "      let wb = Bt[ib];\n      let wa = At[ia];\n"
     "      let b4 = vec4<f32>(unpack2x16float(wb.x), unpack2x16float(wb.y));\n"
     "      let al = vec4<f32>(unpack2x16float(wa.x), unpack2x16float(wa.y));\n"
     "      acc0 = acc0 + al.x * b4;\n      acc1 = acc1 + al.y * b4;\n      acc2 = acc2 + al.z * b4;\n      acc3 = acc3 + al.w * b4;\n"
     "      let b4b = vec4<f32>(unpack2x16float(wb.z), unpack2x16float(wb.w));\n"
     "      let alb = vec4<f32>(unpack2x16float(wa.z), unpack2x16float(wa.w));\n"
     "      acc0 = acc0 + alb.x * b4b;\n      acc1 = acc1 + alb.y * b4b;\n      acc2 = acc2 + alb.z * b4b;\n      acc3 = acc3 + alb.w * b4b;\n"
     "      ia = ia + rowPad4;\n      ib = ib + colPad4;\n    }\n"},
  };
  for (const Sub& sb : subs) {
    const size_t p = t.find(sb.from);
    if (p == std::string::npos) { AnchorAlarm("DirectKPWgsl", sb.from); return src; }
    t.replace(p, std::strlen(sb.from), sb.to);
  }
  if (t.find("At[") != t.rfind("At[") || t.find("Bt[") != t.rfind("Bt[")) { AnchorAlarm("DirectKPWgsl", "残留其它 At/Bt 读"); return src; }
  return t;
}
                                                                                                                                     
                                                                                                                     
                                                                                                                                                        
                                                                                                                        
                                                                                                                            
                                                                                                                                
                                                                                                                                 
                                                                                                                              
                                                                                      
                                                                                                                                                    
                                                                                                                      
                                                                                                                                                                     
                                                                                                                              
                                                                                                                            
                                                                                                                                               
                                                                                                                              
std::string DirectKeyScanWgsl(const std::string& src, bool vec = false, bool noselect = false) {
  std::string t = src;
  if (t.find("const WGR : u32 = 32u;\n") == std::string::npos) { AnchorAlarm("DirectKeyScanWgsl", "只认 WGR=32"); return src; }
  const bool w64 = t.find("const BT : u32 = 32u;\n") != std::string::npos;                                                   
  const unsigned TCN = w64 ? 8u : 16u;                                                  
  const unsigned CB = w64 ? 5u : 6u;                                                              
  const unsigned CMASK = w64 ? 31u : 63u;
  const unsigned NCOL = w64 ? 32u : 64u;
  const unsigned COLOFF = w64 ? 512u : 1024u;                                                   
  {
    const std::string sd = w64 ? "var<workgroup> S : array<vec4<f32>, 256>;\n" : "var<workgroup> S : array<vec4<f32>, 512>;\n";
    const size_t p = t.find(sd);
    if (p == std::string::npos) { AnchorAlarm("DirectKeyScanWgsl", "S 声明未命中"); return src; }
    t.replace(p, sd.size(), w64 ? "var<workgroup> P : array<u32, 1024>;\n" : "var<workgroup> P : array<u32, 2048>;\n");
  }
  const std::string from_head = w64 ? "    workgroupBarrier();\n    S[(tr * 4u + 0u) * 8u + tc] = acc0;\n" : "    workgroupBarrier();\n    S[(tr * 4u + 0u) * 16u + tc] = acc0;\n";
  const std::string from_tail = "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }\n    }\n";
  const size_t p = t.find(from_head);
  const size_t q = (p == std::string::npos) ? std::string::npos : t.find(from_tail, p);
  if (p == std::string::npos || q == std::string::npos) { AnchorAlarm("DirectKeyScanWgsl", "扫描块未命中"); return src; }
  std::string to;
  to += "    workgroupBarrier();\n    {\n      let cbase = tc * 4u;\n      let rbase = tr * 4u;\n";
  const char* comp[] = {"x", "y", "z", "w"};
  const char* accs[] = {"acc0", "acc1", "acc2", "acc3"};
  if (vec) {
    char v[3000];
    std::snprintf(v, sizeof(v),
      "      let colBits = vec4<u32>(%uu - cbase, %uu - cbase, %uu - cbase, %uu - cbase);\n"
      "      let u0 = vec4<u32>(acc0);\n      let u1 = vec4<u32>(acc1);\n      let u2 = vec4<u32>(acc2);\n      let u3 = vec4<u32>(acc3);\n"
      "      let r0 = (u0 << vec4<u32>(%uu)) | colBits;\n      let r1 = (u1 << vec4<u32>(%uu)) | colBits;\n"
      "      let r2 = (u2 << vec4<u32>(%uu)) | colBits;\n      let r3 = (u3 << vec4<u32>(%uu)) | colBits;\n"
      "      // 行 top-2:按列转置后 4 行同时折叠\n"
      "      let tX = vec4<u32>(r0.x, r1.x, r2.x, r3.x);\n      let tY = vec4<u32>(r0.y, r1.y, r2.y, r3.y);\n"
      "      let tZ = vec4<u32>(r0.z, r1.z, r2.z, r3.z);\n      let tW = vec4<u32>(r0.w, r1.w, r2.w, r3.w);\n"
      "      let ra = max(tX, tY);\n      let rb = min(tX, tY);\n      let rc = max(tZ, tW);\n      let rd = min(tZ, tW);\n"
      "      let rbest = max(ra, rc);\n      let rsec = max(min(ra, rc), max(rb, rd));\n"
      "      P[((rbase + 0u) * %uu + tc) * 2u] = rbest.x;\n      P[((rbase + 0u) * %uu + tc) * 2u + 1u] = rsec.x;\n"
      "      P[((rbase + 1u) * %uu + tc) * 2u] = rbest.y;\n      P[((rbase + 1u) * %uu + tc) * 2u + 1u] = rsec.y;\n"
      "      P[((rbase + 2u) * %uu + tc) * 2u] = rbest.z;\n      P[((rbase + 2u) * %uu + tc) * 2u + 1u] = rsec.z;\n"
      "      P[((rbase + 3u) * %uu + tc) * 2u] = rbest.w;\n      P[((rbase + 3u) * %uu + tc) * 2u + 1u] = rsec.w;\n"
      "      // 列键:行位 31-rbase-i;列 top-2 直接按分量折叠(4 列同时)\n"
      "      let c0 = (u0 << vec4<u32>(5u)) | vec4<u32>(31u - rbase);\n      let c1 = (u1 << vec4<u32>(5u)) | vec4<u32>(30u - rbase);\n"
      "      let c2 = (u2 << vec4<u32>(5u)) | vec4<u32>(29u - rbase);\n      let c3 = (u3 << vec4<u32>(5u)) | vec4<u32>(28u - rbase);\n"
      "      let ca = max(c0, c1);\n      let cb_ = min(c0, c1);\n      let cc = max(c2, c3);\n      let cd = min(c2, c3);\n"
      "      let cbest = max(ca, cc);\n      let csec = max(min(ca, cc), max(cb_, cd));\n"
      "      P[%uu + ((cbase + 0u) * 8u + tr) * 2u] = cbest.x;\n      P[%uu + ((cbase + 0u) * 8u + tr) * 2u + 1u] = csec.x;\n"
      "      P[%uu + ((cbase + 1u) * 8u + tr) * 2u] = cbest.y;\n      P[%uu + ((cbase + 1u) * 8u + tr) * 2u + 1u] = csec.y;\n"
      "      P[%uu + ((cbase + 2u) * 8u + tr) * 2u] = cbest.z;\n      P[%uu + ((cbase + 2u) * 8u + tr) * 2u + 1u] = csec.z;\n"
      "      P[%uu + ((cbase + 3u) * 8u + tr) * 2u] = cbest.w;\n      P[%uu + ((cbase + 3u) * 8u + tr) * 2u + 1u] = csec.w;\n",
      CMASK, CMASK - 1u, CMASK - 2u, CMASK - 3u, CB, CB, CB, CB,
      TCN, TCN, TCN, TCN, TCN, TCN, TCN, TCN,
      COLOFF, COLOFF, COLOFF, COLOFF, COLOFF, COLOFF, COLOFF, COLOFF);
    to += v;
  }
                                                                     
  for (int i = 0; i < 4 && !vec; ++i) {
    to += "      {\n";
    for (int j = 0; j < 4; ++j) {
      char b[256];
      if (noselect) std::snprintf(b, sizeof(b), "        let k%d = (u32(%s.%s) << %uu) | (%uu - (cbase + %du));\n", j, accs[i], comp[j], CB, CMASK, j);
      else std::snprintf(b, sizeof(b), "        let k%d = select((u32(%s.%s) << %uu) | (%uu - (cbase + %du)), 0u, %s.%s == 0.0);\n", j, accs[i], comp[j], CB, CMASK, j, accs[i], comp[j]);
      to += b;
    }
    to += "        var b = k0;\n        var s = 0u;\n"
          "        s = max(s, min(b, k1)); b = max(b, k1);\n"
          "        s = max(s, min(b, k2)); b = max(b, k2);\n"
          "        s = max(s, min(b, k3)); b = max(b, k3);\n";
    char w[160];
    std::snprintf(w, sizeof(w), "        P[((rbase + %du) * %uu + tc) * 2u] = b;\n        P[((rbase + %du) * %uu + tc) * 2u + 1u] = s;\n      }\n", i, TCN, i, TCN);
    to += w;
  }
                                                                         
  for (int j = 0; j < 4 && !vec; ++j) {
    to += "      {\n";
    for (int i = 0; i < 4; ++i) {
      char b[256];
      if (noselect) std::snprintf(b, sizeof(b), "        let k%d = (u32(%s.%s) << 5u) | (31u - (rbase + %du));\n", i, accs[i], comp[j], i);
      else std::snprintf(b, sizeof(b), "        let k%d = select((u32(%s.%s) << 5u) | (31u - (rbase + %du)), 0u, %s.%s == 0.0);\n", i, accs[i], comp[j], i, accs[i], comp[j]);
      to += b;
    }
    to += "        var b = k0;\n        var s = 0u;\n"
          "        s = max(s, min(b, k1)); b = max(b, k1);\n"
          "        s = max(s, min(b, k2)); b = max(b, k2);\n"
          "        s = max(s, min(b, k3)); b = max(b, k3);\n";
    char w[160];
    std::snprintf(w, sizeof(w), "        P[%uu + ((cbase + %du) * 8u + tr) * 2u] = b;\n        P[%uu + ((cbase + %du) * 8u + tr) * 2u + 1u] = s;\n      }\n", COLOFF, j, COLOFF, j);
    to += w;
  }
  char tail[1400];
  std::snprintf(tail, sizeof(tail),
        "    }\n    workgroupBarrier();\n"
        "    if (lid < WGR) {\n"
        "      var b = 0u;\n      var s = 0u;\n"
        "      for (var t = 0u; t < %uu; t = t + 1u) {\n"
        "        let bk = P[(lid * %uu + t) * 2u];\n"
        "        let sk = P[(lid * %uu + t) * 2u + 1u];\n"
        "        s = max(max(s, sk), min(b, bk));\n"
        "        b = max(b, bk);\n"
        "      }\n"
        "      let b2 = f32(b >> %uu);\n      let s2 = f32(s >> %uu);\n"
        "      let i2 = select(i32(col0 + (%uu - (b & %uu))), -1, %s);\n"
        "      if (b2 > rowBest) { rowSecond = max(rowBest, s2); rowBest = b2; rowBestI = i2; }\n"
        "      else { rowSecond = max(rowSecond, b2); }\n"
        "    } else if (lid < WGR + %uu) {\n"
        "      let c = lid - WGR;\n"
        "      var b = 0u;\n      var s = 0u;\n"
        "      for (var t = 0u; t < 8u; t = t + 1u) {\n"
        "        let bk = P[%uu + (c * 8u + t) * 2u];\n"
        "        let sk = P[%uu + (c * 8u + t) * 2u + 1u];\n", TCN, TCN, TCN, CB, CB, CMASK, CMASK, (vec || noselect) ? "(b >> CBITSu) == 0u" : "b == 0u", NCOL, COLOFF, COLOFF);
  {
    std::string tl = tail;
    const std::string cb = std::to_string(CB);
    for (size_t q = tl.find("CBITS"); q != std::string::npos; q = tl.find("CBITS")) tl.replace(q, 5, cb);
    to += tl;
  }
  to += ""
        "        s = max(max(s, sk), min(b, bk));\n"
        "        b = max(b, bk);\n"
        "      }\n"
        "      let cb = f32(b >> 5u);\n      let cs = f32(s >> 5u);\n"
        "      let ci = select(i32(row0 + (31u - (b & 31u))), -1, " + std::string((vec || noselect) ? "(b >> 5u) == 0u" : "b == 0u") + ");\n"
        "      let gc = col0 + c;\n"
        "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }\n"
        "    }\n";
  t.replace(p, q + from_tail.size() - p, to);
  if (t.find("S[") != std::string::npos) { AnchorAlarm("DirectKeyScanWgsl", "残留 S[ 引用"); return src; }
  return t;
}

std::string BlkNoLoadWgsl(const std::string& src) {
  std::string t = src;
                                                                                                   
                                                                                                                  
  const std::string fromPipeB =
      "        let al = S[k * 16u + tr * 2u];\n"
      "        let ah = S[k * 16u + tr * 2u + 1u];\n"
      "        let bn = S[512u + min(k + 1u, KC - 1u) * 16u + tc];\n";
  const size_t pb = t.find(fromPipeB);
  if (pb != std::string::npos) {
    t.replace(pb, fromPipeB.size(),
        "        let fk = f32(k);\n"
        "        let al = vec4<f32>(fk, fk + 1.0, fk + 2.0, fk + 3.0);\n"
        "        let ah = vec4<f32>(fk + 4.0, fk + 5.0, fk + 6.0, fk + 7.0);\n"
        "        let bn = vec4<f32>(fk * 0.5, fk, fk * 1.5, fk * 2.0);\n");
    return t;
  }
  const std::string from =
      "        let al = S[k * 16u + tr * 2u];\n"
      "        let ah = S[k * 16u + tr * 2u + 1u];\n"
      "        let b4 = S[512u + k * 16u + tc];\n";
  const size_t p = t.find(from);
  if (p != std::string::npos) {
    t.replace(p, from.size(),
        "        let fk = f32(k);\n"
        "        let al = vec4<f32>(fk, fk + 1.0, fk + 2.0, fk + 3.0);\n"
        "        let ah = vec4<f32>(fk + 4.0, fk + 5.0, fk + 6.0, fk + 7.0);\n"
        "        let b4 = vec4<f32>(fk * 0.5, fk, fk * 1.5, fk * 2.0);\n");
    return t;
  }
                        
  const std::string from44 =
      "        let a4 = S[k * 16u + tr];\n"
      "        let b4 = S[512u + k * 16u + tc];\n";
  const size_t q = t.find(from44);
  if (q == std::string::npos) { AnchorAlarm("BlkNoLoadWgsl", "内层载入未命中(4x4/8x4 都不是)"); return src; }
  t.replace(q, from44.size(),
      "        let fk = f32(k);\n"
      "        let a4 = vec4<f32>(fk, fk + 1.0, fk + 2.0, fk + 3.0);\n"
      "        let b4 = vec4<f32>(fk * 0.5, fk, fk * 1.5, fk * 2.0);\n");
  return t;
}

                                                                                                                     
std::string BlkXBarWgsl(const std::string& src, int n) {
  std::string t = src;
                                                                                            
  std::string from =
      "        acc7 = acc7 + ah.w * b4;\n"
      "        b4 = bn;\n"
      "      }\n";
  size_t p = t.find(from);
  if (p == std::string::npos) {
    from = "        acc7 = acc7 + ah.w * b4;\n      }\n";
    p = t.find(from);
  }
  if (p == std::string::npos) { AnchorAlarm("BlkXBarWgsl", "8x4 内层尾未命中(PIPEB/裸 都不是)"); return src; }
  std::string add = from;
  for (int i = 0; i < n; ++i) add += "      workgroupBarrier();\n";
  t.replace(p, from.size(), add);
  return t;
}

                                                                                                        
                                                                                                                      
                                                                                                               
                                                                                                                          
std::string BlkGpfWgsl(const std::string& src) {
  std::string t = src;
                                                                                
  const char* heads[] = {
      "      for (var side = 0u; side < 2u; side = side + 1u) {\n        let w = lid / 16u;\n        let q = lid % 16u;\n",
      "      for (var pair = 0u; pair < 4u; pair = pair + 1u) {\n        let side = pair / 2u;\n        let t = lid + (pair % 2u) * 64u;\n        let w = t / 16u;\n        let q = t % 16u;\n"};
  size_t p = std::string::npos; int which = -1;
  for (int h = 0; h < 2; ++h) { p = t.find(heads[h]); if (p != std::string::npos) { which = h; break; } }
  if (which < 0) { AnchorAlarm("BlkGpfWgsl", "暂存头未命中"); return src; }
  const std::string tail = "        S[o + 3u * 16u] = vec4<f32>(f32(x0 >> 24u), f32(x1 >> 24u), f32(x2 >> 24u), f32(x3 >> 24u));\n      }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("BlkGpfWgsl", "暂存尾未命中"); return src; }
  const int npair = which == 0 ? 2 : 4;
                                                                       
  std::string body;
  body += "      // [GPF] 从上一步预取好的寄存器写线程组\n";
  for (int pr = 0; pr < npair; ++pr) {
    char b[1200];
    const char* sidx = which == 0 ? "%d" : "(%d / 2)";
    (void)sidx;
    const int side = which == 0 ? pr : pr / 2;
    const int tofs = which == 0 ? 0 : (pr % 2) * 64;
    std::snprintf(b, sizeof(b),
      "      {\n"
      "        let t = lid + %du;\n"
      "        let w = t / 16u;\n"
      "        let q = t %% 16u;\n"
      "        let o = %du + (w * 4u) * 16u + q;\n"
      "        S[o + 0u * 16u] = vec4<f32>(f32(px%d.x & 255u), f32(px%d.y & 255u), f32(px%d.z & 255u), f32(px%d.w & 255u));\n"
      "        S[o + 1u * 16u] = vec4<f32>(f32((px%d.x >> 8u) & 255u), f32((px%d.y >> 8u) & 255u), f32((px%d.z >> 8u) & 255u), f32((px%d.w >> 8u) & 255u));\n"
      "        S[o + 2u * 16u] = vec4<f32>(f32((px%d.x >> 16u) & 255u), f32((px%d.y >> 16u) & 255u), f32((px%d.z >> 16u) & 255u), f32((px%d.w >> 16u) & 255u));\n"
      "        S[o + 3u * 16u] = vec4<f32>(f32(px%d.x >> 24u), f32(px%d.y >> 24u), f32(px%d.z >> 24u), f32(px%d.w >> 24u));\n"
      "      }\n",
      tofs, side * 512, pr,pr,pr,pr, pr,pr,pr,pr, pr,pr,pr,pr, pr,pr,pr,pr);
    body += b;
  }
  t.replace(p, q + tail.size() - p, body);
                                                      
  std::string pre;
  for (int pr = 0; pr < npair; ++pr) {
    const int side = which == 0 ? pr : pr / 2;
    const int tofs = which == 0 ? 0 : (pr % 2) * 64;
    char b[900];
    std::snprintf(b, sizeof(b),
      "      {\n"
      "        let t = lid + %du;\n"
      "        let w = t / 16u;\n"
      "        let base = (t %% 16u) * 4u;\n"
      "        let g = %s + base;\n"
      "        px%d = vec4<u32>(\n"
      "          select(0u, %s[(g + 0u) * 32u + kcn * 8u + w], g + 0u < %s),\n"
      "          select(0u, %s[(g + 1u) * 32u + kcn * 8u + w], g + 1u < %s),\n"
      "          select(0u, %s[(g + 2u) * 32u + kcn * 8u + w], g + 2u < %s),\n"
      "          select(0u, %s[(g + 3u) * 32u + kcn * 8u + w], g + 3u < %s));\n"
      "      }\n",
      tofs, side == 0 ? "row0" : "col0", pr,
      side == 0 ? "A" : "B", side == 0 ? "U.numA" : "U.numB",
      side == 0 ? "A" : "B", side == 0 ? "U.numA" : "U.numB",
      side == 0 ? "A" : "B", side == 0 ? "U.numA" : "U.numB",
      side == 0 ? "A" : "B", side == 0 ? "U.numA" : "U.numB");
    pre += b;
  }
                                                                                           
  std::string decl;
  for (int pr = 0; pr < npair; ++pr) { char b[64]; std::snprintf(b, sizeof(b), "    var px%d = vec4<u32>(0u);\n", pr); decl += b; }
  const std::string loop_head = "    for (var kc = 0u; kc < 4u; kc = kc + 1u) {\n      workgroupBarrier();\n";
  const size_t lp = t.find(loop_head);
  if (lp == std::string::npos) { AnchorAlarm("BlkGpfWgsl", "段循环头未命中"); return src; }
  std::string pre0 = pre; { size_t z; while ((z = pre0.find("kcn")) != std::string::npos) pre0.replace(z, 3, "0u"); }
  t.replace(lp, loop_head.size(), decl + "    {\n" + pre0 + "    }\n" + loop_head);
                                                   
  const std::string bar2 = "      workgroupBarrier();\n      for (var k = 0u; k < KC; k = k + 1u) {\n";
  const size_t bp = t.find(bar2, lp);
  if (bp == std::string::npos) { AnchorAlarm("BlkGpfWgsl", "第二 barrier 未命中"); return src; }
  std::string pre1 = pre; { size_t z; while ((z = pre1.find("kcn")) != std::string::npos) pre1.replace(z, 3, "kcx"); }
  t.replace(bp, bar2.size(),
            "      workgroupBarrier();\n"
            "      if (kc + 1u < 4u) {\n        let kcx = kc + 1u;\n" + pre1 + "      }\n"
            "      for (var k = 0u; k < KC; k = k + 1u) {\n");
  return t;
}

                                                                                                              
                                                                                                       
                                                                                                     
std::string BlkStgWgsl(const std::string& src) {
  std::string t = src;
  const std::string from = "        let w = lid / 16u;\n        let q = lid % 16u;\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("BlkStgWgsl", "8x4 暂存映射未命中"); return src; }
  t.replace(p, from.size(), "        let w = lid % 8u;\n        let q = lid / 8u;\n");
  return t;
}

                                                                                
                                                                                                                     
                                                                                                       
                                                                                                       
                                                                 
                                                                                                           
                                                                                                     
std::string BlkRScanWgsl(const std::string& src) {
  std::string t = src;
  const std::string head = "    workgroupBarrier();\n    S[(tr * 8u + 0u) * 16u + tc] = acc0;\n";
  const size_t p = t.find(head);
  if (p == std::string::npos) { AnchorAlarm("BlkRScanWgsl", "acc 存储头未命中"); return src; }
  const std::string tail = "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }\n    }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("BlkRScanWgsl", "列扫描尾未命中"); return src; }
  std::string body;
  body +=
    "    workgroupBarrier();\n"
    "    // ── 行向 partial(本线程 8 行 × 4 列)──\n";
  for (int i = 0; i < 8; ++i) {
    char b[700];
    std::snprintf(b, sizeof(b),
      "    {\n"
      "      let d = acc%d;\n"
      "      var pb = 0.0; var ps = 0.0; var pi = -1;\n"
      "      let cb0 = i32(col0 + tc * 4u);\n"
      "      if (d.x > pb) { ps = pb; pb = d.x; pi = cb0; } else if (d.x > ps) { ps = d.x; }\n"
      "      if (d.y > pb) { ps = pb; pb = d.y; pi = cb0 + 1; } else if (d.y > ps) { ps = d.y; }\n"
      "      if (d.z > pb) { ps = pb; pb = d.z; pi = cb0 + 2; } else if (d.z > ps) { ps = d.z; }\n"
      "      if (d.w > pb) { ps = pb; pb = d.w; pi = cb0 + 3; } else if (d.w > ps) { ps = d.w; }\n"
      "      S[(tr * 8u + %du) * 16u + tc] = vec4<f32>(pb, ps, bitcast<f32>(pi), 0.0);\n"
      "    }\n", i, i);
    body += b;
  }
  body +=
    "    workgroupBarrier();\n"
    "    if (lid < WGR) {\n"
    "      for (var v = 0u; v < 16u; v = v + 1u) {\n"
    "        let d = S[lid * 16u + v];\n"
    "        let ob = d.x; let os = d.y; let oi = bitcast<i32>(d.z);\n"
    "        if (ob > rowBest) { rowSecond = max(os, rowBest); rowBest = ob; rowBestI = oi; }\n"
    "        else { rowSecond = max(rowSecond, ob); }\n"
    "      }\n"
    "    }\n"
    "    workgroupBarrier();\n"
    "    // ── 列向 partial(本线程 4 列 × 8 行)──\n";
  for (int j = 0; j < 4; ++j) {
    const char* comp = "xyzw";
    std::string b =
      "    {\n"
      "      var pb = 0.0; var ps = 0.0; var pi = -1;\n"
      "      let rb0 = i32(row0 + tr * 8u);\n";
    for (int i = 0; i < 8; ++i) {
      char l[200];
      std::snprintf(l, sizeof(l),
        "      if (acc%d.%c > pb) { ps = pb; pb = acc%d.%c; pi = rb0 + %d; } else if (acc%d.%c > ps) { ps = acc%d.%c; }\n",
        i, comp[j], i, comp[j], i, i, comp[j], i, comp[j]);
      b += l;
    }
    char st[160];
    std::snprintf(st, sizeof(st), "      S[(tc * 4u + %du) * 8u + tr] = vec4<f32>(pb, ps, bitcast<f32>(pi), 0.0);\n    }\n", j);
    b += st;
    body += b;
  }
  body +=
    "    workgroupBarrier();\n"
    "    if (lid >= WGR && lid < 2u * WGR) {\n"
    "      let c = lid - WGR;\n"
    "      var cb = 0.0; var cs = 0.0; var ci = -1;\n"
    "      for (var r8 = 0u; r8 < 8u; r8 = r8 + 1u) {\n"
    "        let d = S[c * 8u + r8];\n"
    "        let ob = d.x; let os = d.y; let oi = bitcast<i32>(d.z);\n"
    "        if (ob > cb) { cs = max(os, cb); cb = ob; ci = oi; }\n"
    "        else { cs = max(cs, ob); }\n"
    "      }\n"
    "      let gc = col0 + c;\n"
    "      if (gc < U.numB) { ColP[rb * U.numB + gc] = ColPart(cb, cs, ci); }\n"
    "    }\n";
  t.replace(p, q + tail.size() - p, body);
  return t;
}

                                                                                            
                                                                                                               
                                              
std::string BlkPipeBWgsl(const std::string& src) {
  std::string t = src;
  const std::string from =
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let al = S[k * 16u + tr * 2u];\n"
      "        let ah = S[k * 16u + tr * 2u + 1u];\n"
      "        let b4 = S[512u + k * 16u + tc];\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("BlkPipeBWgsl", "8x4 循环头未命中"); return src; }
  t.replace(p, from.size(),
      "      var b4 = S[512u + tc];\n"
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let al = S[k * 16u + tr * 2u];\n"
      "        let ah = S[k * 16u + tr * 2u + 1u];\n"
      "        let bn = S[512u + min(k + 1u, KC - 1u) * 16u + tc];\n");
  const std::string tail = "        acc7 = acc7 + ah.w * b4;\n      }\n";
  const size_t q = t.find(tail, p);
  if (q == std::string::npos) { AnchorAlarm("BlkPipeBWgsl", "8x4 循环尾未命中"); return src; }
  t.replace(q, tail.size(), "        acc7 = acc7 + ah.w * b4;\n        b4 = bn;\n      }\n");
  return t;
}

std::string BlkUnrollWgsl(const std::string& src) {
  std::string t = src;
  const std::string from =
      "      for (var k = 0u; k < KC; k = k + 1u) {\n"
      "        let a4 = S[k * 16u + tr];\n"
      "        let b4 = S[512u + k * 16u + tc];\n"
      "        acc0 = acc0 + a4.x * b4;\n"
      "        acc1 = acc1 + a4.y * b4;\n"
      "        acc2 = acc2 + a4.z * b4;\n"
      "        acc3 = acc3 + a4.w * b4;\n"
      "      }\n";
  const size_t p = t.find(from);
  if (p == std::string::npos) { AnchorAlarm("BlkUnrollWgsl", "k 循环未命中"); return src; }
  std::string to;
  for (int k = 0; k < 32; ++k) {
    char buf[512];
    std::snprintf(buf, sizeof(buf),
        "      {\n"
        "        let a4 = S[%du + tr];\n"
        "        let b4 = S[%du + tc];\n"
        "        acc0 = acc0 + a4.x * b4;\n"
        "        acc1 = acc1 + a4.y * b4;\n"
        "        acc2 = acc2 + a4.z * b4;\n"
        "        acc3 = acc3 + a4.w * b4;\n"
        "      }\n", k * 16, 512 + k * 16);
    to += buf;
  }
  t.replace(p, from.size(), to);
  return t;
}

                                                                                               
                                                                           
                                                                                        
                                                                                                                  
                                                                                                         
                                                                                                                    
                                                                                            
                                                                                                    
                                                                                                         
                                                                                                           
                                                                                                                    
                                                   
                                                                                                  
                                                                                                                
                                                         
                                                                                 
std::string PrefetchWgsl(const std::string& src) {
  std::string t = src;
                                                                    
  const std::string stage =
      "    for (var e = lid; e < BT * 32u; e = e + 512u) {\n"
      "      let brow = col0 + e / 32u;\n"
      "      let pu = select(0u, B[brow * 32u + (e % 32u)], brow < U.numB);\n"
      "      let po = (e / 32u) * 128u + (e % 32u) * 4u;\n"
      "      Bsh[po] = f16(pu & 255u);\n"
      "      Bsh[po + 1u] = f16((pu >> 8u) & 255u);\n"
      "      Bsh[po + 2u] = f16((pu >> 16u) & 255u);\n"
      "      Bsh[po + 3u] = f16(pu >> 24u);\n"
      "    }\n"
      "    workgroupBarrier();\n";
  const size_t sp = t.find(stage);
  if (sp == std::string::npos) {
                                                                                                                
                                                                                                     
                                                                              
    std::fprintf(stderr,
                 "[pwofficial_gpu_match_dawn] PrefetchWgsl 锚点失配(stage),"
                 "变换未生效 —— 内核结构可能已改,请核对锚点\n");
    { AnchorAlarm("PrefetchWgsl", "静默退化为 no-op"); return src; }
  }
                                   
  t.erase(sp, stage.size());
                                                      
                                                                                   
                                                                                                     
                                                                    
  const std::string loop_head =
      "  var col0 = U.colBase;\n"
      "  let colEnd = min(U.colBase + U.colSpan, U.numB);\n"
      "  loop {\n";
  const size_t lp = t.find(loop_head);
  if (lp == std::string::npos) {
    std::fprintf(stderr, "[pwofficial_gpu_match_dawn] PrefetchWgsl 锚点失配(loop_head),变换未生效\n");
    { AnchorAlarm("PrefetchWgsl", "静默退化为 no-op"); return src; }
  }
  std::string pre0 = stage;
  { const size_t q = pre0.find("col0 + e / 32u"); pre0.replace(q, 14, "U.colBase + e / 32u"); }
  t.insert(lp, pre0);
                                                                       
                                                                                             
                                                                                       
                                                                                                               
                                                                                                               
                                                                                             
                                                                                                 
  const bool late = getenv("OFFICIAL_AETHER_MATCH_DAWN_PFLATE") != nullptr;
  const std::string after_gemm =
      late ? std::string("    cpIdx[sg * 32u + lane] = ci;\n    workgroupBarrier();\n")
           : std::string(
      "    subgroupMatrixStore(&accSh, (sg * 8u) * 32u + 3u * 8u, acc3, false, 32u);\n"
      "    workgroupBarrier();\n");
  const size_t ag = t.find(after_gemm);
  if (ag == std::string::npos) {
    std::fprintf(stderr, "[pwofficial_gpu_match_dawn] PrefetchWgsl 锚点失配(after_gemm),变换未生效\n");
    { AnchorAlarm("PrefetchWgsl", "静默退化为 no-op"); return src; }
  }
  t.insert(ag + after_gemm.size(),
      "    // 提前发出下一块的 device load(只进寄存器,不碰 Bsh)\n"
      "    let nextT = col0 + BT;\n"
      "    let pe0 = lid;\n"
      "    let pe1 = lid + 512u;\n"
      "    let pr0 = nextT + pe0 / 32u;\n"
      "    let pr1 = nextT + pe1 / 32u;\n"
      "    // 🔴 WGSL 的 select **两个操作数都会求值** ⇒ 条件为假时 B[...] 照样读。\n"
      "    // 最后一块 nextT == numB,行号最多超出 15 行 ⇒ 读到 B 尾后约 2 KiB。\n"
      "    // disable_robustness 开着,在 Metal 上无害(值被外层 select 丢弃),但这是 UB,\n"
      "    // **Vulkan 后端可能触发校验层或崩溃** —— 三端一套下是硬伤。先把下标钳到界内,\n"
      "    // 界内地址逐字节不变(只影响最后一块的越界行,那些行本来就被丢弃)。\n"
      "    let sr0 = select(0u, pr0, pr0 < U.numB);\n"
      "    let sr1 = select(0u, pr1, pr1 < U.numB);\n"
      "    let pv0 = select(0u, B[sr0 * 32u + (pe0 % 32u)], pr0 < U.numB);\n"
      "    let pv1 = select(0u, B[sr1 * 32u + (pe1 % 32u)], pr1 < U.numB);\n");
                                                                            
  const std::string tail = "    col0 = col0 + BT;\n";
  const size_t tp = t.rfind(tail);
  if (tp == std::string::npos) { AnchorAlarm("PrefetchWgsl", "静默退化为 no-op"); return src; }
  t.replace(tp, tail.size(),
      "    // 用得最晚:此时 load 已在飞行中被扫描+归并掩藏\n"
      "    let po0 = (pe0 / 32u) * 128u + (pe0 % 32u) * 4u;\n"
      "    Bsh[po0] = f16(pv0 & 255u);\n"
      "    Bsh[po0 + 1u] = f16((pv0 >> 8u) & 255u);\n"
      "    Bsh[po0 + 2u] = f16((pv0 >> 16u) & 255u);\n"
      "    Bsh[po0 + 3u] = f16(pv0 >> 24u);\n"
      "    let po1 = (pe1 / 32u) * 128u + (pe1 % 32u) * 4u;\n"
      "    Bsh[po1] = f16(pv1 & 255u);\n"
      "    Bsh[po1 + 1u] = f16((pv1 >> 8u) & 255u);\n"
      "    Bsh[po1 + 2u] = f16((pv1 >> 16u) & 255u);\n"
      "    Bsh[po1 + 3u] = f16(pv1 >> 24u);\n"
      "    workgroupBarrier();\n"
      "    col0 = nextT;\n");
  return t;
}

std::string ColpTransposeWgsl(const std::string& src) {
  std::string t = src;
  auto rep = [&t](const std::string& from, const std::string& to) {
    const size_t p = t.find(from);
    if (p == std::string::npos) return false;
    t.replace(p, from.size(), to);
    return true;
  };
  bool ok = true;
  ok &= rep("        ColP[c * U.numWg + rb] = ColPart(best, second, bi);",
            "        ColP[rb * U.numB + c] = ColPart(best, second, bi);");
  ok &= rep("    let p = ColP[c * U.numWg + w];",
            "    let p = ColP[w * U.numB + c];");
  if (!ok) { AnchorAlarm("ColpTransposeWgsl", "静默退化为 no-op"); return src; }
  return t;
}

std::string MixedWgsl(const char* src) {
  std::string t(src);
  auto sub = [&t](const std::string& from, const std::string& to) {
    for (size_t p = t.find(from); p != std::string::npos;
         p = t.find(from, p + to.size())) {
      t.replace(p, from.size(), to);
    }
  };
  sub("enable subgroups;", "enable subgroups;\nenable f16;");
  sub("subgroup_matrix_left<f32", "subgroup_matrix_left<f16");
  sub("subgroup_matrix_right<f32", "subgroup_matrix_right<f16");
  sub("var<storage, read> A : array<f32>", "var<storage, read> A : array<f16>");
  sub("var<storage, read> B : array<f32>", "var<storage, read> B : array<f16>");
  sub("var<workgroup> Bsh : array<f32,", "var<workgroup> Bsh : array<f16,");
  sub("select(0.0, B[", "select(f16(0.0), B[");
  return t;
}

wgpu::ShaderModule CompileWgsl(Ctx& c, const std::string& src,
                               const char* what) {
  wgpu::ShaderSourceWGSL s{};
  s.code = src.c_str();
  wgpu::ShaderModuleDescriptor d{};
  d.nextInChain = &s;
  wgpu::ShaderModule m = c.device.CreateShaderModule(&d);
  if (!m) return nullptr;
  bool ok = true;
  std::string err;
  c.instance.WaitAny(
      m.GetCompilationInfo(
          wgpu::CallbackMode::WaitAnyOnly,
          [&](wgpu::CompilationInfoRequestStatus,
              const wgpu::CompilationInfo* ci) {
            if (!ci) return;
            for (size_t i = 0; i < ci->messageCount; ++i) {
              if (ci->messages[i].type == wgpu::CompilationMessageType::Error) {
                ok = false;
                if (err.size() < 400) {
                  err += SV(ci->messages[i].message);
                  err += " | ";
                }
              }
            }
          }),
      UINT64_MAX);
  if (!ok) {
    Log("WGSL compile failed (%s): %s", what, err.c_str());
    StashLastError(std::string("WGSL compile failed: ") + what);
    return nullptr;
  }
  return m;
}

wgpu::ComputePipeline MakePipeline(Ctx& c, wgpu::ShaderModule m,
                                   const char* entry) {
  wgpu::ComputePipelineDescriptor pd{};
  pd.compute.module = m;
  pd.compute.entryPoint = entry;
  return c.device.CreateComputePipeline(&pd);
}

bool EnsureMainPipelines(Ctx& c) {
  if (c.p_main && (c.backend == Backend::kTiled || c.p_merge)) return true;
  if (c.tried_main) return false;
  c.tried_main = true;
  if (c.backend == Backend::kMma) {
                                                                      
                                                                            
    const bool shape = c.mixed &&
                       getenv("OFFICIAL_AETHER_MATCH_DAWN_OLDSHAPE") == nullptr;
    std::string mixed_src =
        c.mixed ? MixedWgsl(shape ? kWgslMmaMetalShape : kWgslMmaFused)
                : std::string();
    if (c.mixed && !shape &&
        getenv("OFFICIAL_AETHER_MATCH_DAWN_COLPC") == nullptr) {
      mixed_src = ColpTransposeWgsl(mixed_src);                                          
    }
    if (c.packed) mixed_src = PackedWgsl(mixed_src);
    if (c.packed && shape &&
        getenv("OFFICIAL_AETHER_MATCH_DAWN_NOPREFETCH") == nullptr) {
      mixed_src = PrefetchWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_NOSCAN") != nullptr) {
      mixed_src = NoScanWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_HALFLOAD") != nullptr) {
      mixed_src = HalfLoadWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_NOAPRO") != nullptr) {
      mixed_src = NoAProWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_BSTORE") != nullptr) {
      mixed_src = BStoreWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_MERGE1") != nullptr) {
      mixed_src = Merge1Wgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_BLOAD") != nullptr) {
      mixed_src = BLoadHoistWgsl(mixed_src);
    }
    if (c.mixed && getenv("OFFICIAL_AETHER_MATCH_DAWN_STAGE0") != nullptr) {
      mixed_src = Stage0Wgsl(mixed_src);
    }
    if (const char* nb = getenv("OFFICIAL_AETHER_MATCH_DAWN_XBARRIER")) {
      if (c.mixed) mixed_src = ExtraBarrierWgsl(mixed_src, atoi(nb));
    }
                                                                                              
                                                                                           
                                                                                                       

    wgpu::ShaderModule m = CompileWgsl(
        c, c.mixed ? mixed_src.c_str() : kWgslMmaFused,
        c.mixed ? "mma fused mixed" : "mma fused");
    if (!m) return false;
    c.p_main = MakePipeline(c, m, "main");
    c.p_merge = MakePipeline(c, m, "merge");
    return c.p_main && c.p_merge;
  }
  if (c.backend == Backend::kBlocked) {
    const DirectKnobs kn = ResolveDirectKnobs();
    if (kn.direct) {
                                                                                                       
      std::string dsrc = kWgslBlocked84Direct;
      const bool dbg = getenv("OFFICIAL_AETHER_MATCH_DAWN_WGSL_DUMP") != nullptr;
      if (dbg) std::fprintf(stderr, "[direct-chain] base len=%zu\n", dsrc.size());
      if (kn.k44) {
        dsrc = DirectTo44Wgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] after 44 len=%zu\n", dsrc.size());
        if (kn.w128) {
          const std::string x = DirectTo44W128Wgsl(dsrc);
          if (x != dsrc) gBlockedRows = 32;
          if (dbg) std::fprintf(stderr, "[direct-chain] W128 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
          dsrc = x;
        }
      }
      if (kn.w64) {
        const std::string x = DirectW64Wgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] W64 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (kn.keyscan2 || kn.keyscan3 || getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_KEYSCAN") != nullptr) {
        const std::string x = DirectKeyScanWgsl(dsrc, kn.keyscan2, kn.keyscan3);
        if (dbg) std::fprintf(stderr, "[direct-chain] KEYSCAN in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PSCAN2") != nullptr) {
        const std::string x = DirectPScanWgsl(dsrc, true);
        if (dbg) std::fprintf(stderr, "[direct-chain] PSCAN2 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PSCAN") != nullptr) {
        const std::string x = DirectPScanWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] PSCAN in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TMAP") != nullptr) {
        const std::string x = DirectTMapWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] TMAP in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANU4") != nullptr) {
        const std::string x = DirectScanU4Wgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] SCANU4 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANSEL") != nullptr) {
        const std::string x = DirectScanSelWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] SCANSEL in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_SCANMEM") != nullptr) {
        const std::string x = DirectScanMemWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] SCANMEM in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (kn.nopb) {
        const std::string x = DirectNoPipeBWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOPB in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PIPEA") != nullptr) {
        const std::string x = DirectPipeAWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] PIPEA in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_G") != nullptr) {
        const std::string g = DirectToGlobalScratchWgsl(dsrc);
        c.direct_g = (g != dsrc);                                                                                 
        if (dbg) std::fprintf(stderr, "[direct-chain] G in=%zu out=%zu applied=%d\n", dsrc.size(), g.size(), (int)c.direct_g);
        dsrc = g;
      }
      if (kn.f16) {
        if (!c.feat_f16) { if (dbg) std::fprintf(stderr, "[direct-chain] F16 skipped: adapter lacks ShaderF16\n"); }
        else {
          const std::string x = DirectF16Wgsl(dsrc);
          c.direct_f16 = (x != dsrc);
          if (dbg) std::fprintf(stderr, "[direct-chain] F16 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)c.direct_f16);
          dsrc = x;
        }
      }
      if (!c.direct_f16 && getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_PACKED") != nullptr) {
        const std::string x = DirectPackedWgsl(dsrc);
        c.direct_packed = (x != dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] PACKED in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)c.direct_packed);
        dsrc = x;
      }
      if (!c.direct_packed && getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TEXA") != nullptr) {
        const std::string x = DirectToTexAWgsl(dsrc);
        c.direct_texa = (x != dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] TEXA in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)c.direct_texa);
        dsrc = x;
      }
      if (!c.direct_packed && getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TEX") != nullptr) {
        const std::string x = DirectToTexWgsl(dsrc);
        c.direct_tex = (x != dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] TEX in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)c.direct_tex);
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TAILB2") != nullptr) {
        const std::string x = DirectTailB2Wgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] TAILB2 in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_TAILB") != nullptr) {
        const std::string x = DirectTailBWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] TAILB in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_FMA") != nullptr) {
        const std::string x = DirectFmaWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] FMA in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (const char* ub = getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLLHB")) {
        const std::string x = DirectUnrollHWgsl(dsrc, std::max(2, atoi(ub)), true);
        if (dbg) std::fprintf(stderr, "[direct-chain] UNROLLHB%s in=%zu out=%zu applied=%d\n", ub, dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (const char* uh = getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLLH")) {
        const std::string x = DirectUnrollHWgsl(dsrc, std::max(2, atoi(uh)));
        if (dbg) std::fprintf(stderr, "[direct-chain] UNROLLH%s in=%zu out=%zu applied=%d\n", uh, dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (kn.ptr) {
        const std::string x = DirectPtrWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] PTR in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (const char* un = getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_UNROLL")) {
        const std::string x = DirectUnrollWgsl(dsrc, std::max(2, atoi(un)));
        if (dbg) std::fprintf(stderr, "[direct-chain] UNROLL%s in=%zu out=%zu applied=%d\n", un, dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      c.direct_kp = false;
      if (kn.kp) {
        if (!c.direct_f16) { if (dbg) std::fprintf(stderr, "[direct-chain] KP skipped: needs F16 storage\n"); }
        else {
          const std::string x = DirectKPWgsl(dsrc);
          c.direct_kp = (x != dsrc);
          if (dbg) std::fprintf(stderr, "[direct-chain] KP in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)c.direct_kp);
          dsrc = x;
        }
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOLOAD") != nullptr) {           
        const std::string x = DirectNoLoadWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOLOAD(probe) in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOSYNC") != nullptr) {           
        const std::string x = DirectNoSyncWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOSYNC(probe) in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOSCANROW") != nullptr) {           
        const std::string x = DirectNoScanRowWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOSCANROW(probe) in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOSCANCOL") != nullptr) {           
        const std::string x = DirectNoScanColWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOSCANCOL(probe) in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_DIRECT_NOSCAN") != nullptr) {                                                         
        const std::string x = BlkNoScanWgsl(dsrc);
        if (dbg) std::fprintf(stderr, "[direct-chain] NOSCAN(probe) in=%zu out=%zu applied=%d\n", dsrc.size(), x.size(), (int)(x != dsrc));
        dsrc = x;
      }
                                                                                                                               
                                                                                                                                    
                                                                                                          
      if (const char* wf0 = getenv("OFFICIAL_AETHER_MATCH_DAWN_WGSL_FILE")) {
                                                                                                                                                  
        std::string wfs = wf0;
        if (wfs.size() > 1 && wfs[0] == '~' && wfs[1] == '/') { if (const char* h = getenv("HOME")) wfs = std::string(h) + wfs.substr(1); }
        const char* wf = wfs.c_str();
        if (FILE* f = std::fopen(wf, "rb")) {
          std::string file;
          char buf[4096]; size_t n;
          while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) file.append(buf, n);
          std::fclose(f);
          if (dbg) std::fprintf(stderr, "[direct-chain] WGSL_FILE %s len=%zu (replaces generated len=%zu)\n", wf, file.size(), dsrc.size());
          if (!file.empty()) dsrc = file;
        } else if (dbg) {
          std::fprintf(stderr, "[direct-chain] WGSL_FILE %s open failed\n", wf);
        }
      }
      if (getenv("OFFICIAL_AETHER_MATCH_DAWN_WGSL_DUMP") != nullptr) std::fprintf(stderr, "===WGSL_BEGIN===\n%s\n===WGSL_END===\n", dsrc.c_str());
      wgpu::ShaderModule m = CompileWgsl(c, dsrc.c_str(), "blocked direct");
      if (!m) return false;
      wgpu::ShaderModule mx = CompileWgsl(c, c.direct_kp ? kWgslXposeKP : (c.direct_f16 ? kWgslXposeF16 : (c.direct_packed ? kWgslXposePacked : kWgslXpose)), "xpose");
      if (!mx) return false;
      c.p_main = MakePipeline(c, m, "main");
      c.p_merge = MakePipeline(c, m, "merge");
      c.p_xpose = MakePipeline(c, mx, "xpose");
      c.direct = c.p_main && c.p_merge && c.p_xpose;
      return c.direct;
    }
                                                                                                             
                                                                                                               
                                                                                                        
                                                                                                      
                                                                                             
    std::string blk = getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_88") != nullptr
                          ? std::string(kWgslBlocked88)
                      : getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_44") != nullptr
                          ? std::string(kWgslBlockedPlain) : std::string(kWgslBlocked84);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_GPF") != nullptr) blk = BlkGpfWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_STG") != nullptr) blk = BlkStgWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_RSCAN") != nullptr) blk = BlkRScanWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_PIPE") != nullptr) blk = BlkPipeWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_NOPIPEB") == nullptr &&
        getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_PIPE") == nullptr &&
        getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_88") == nullptr &&
        getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_44") == nullptr) {
      blk = BlkPipeBWgsl(blk);                      
    }
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_FMA") != nullptr) blk = BlkFmaWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_NOLOAD") != nullptr) blk = BlkNoLoadWgsl(blk);
    if (const char* nb = getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_XBAR")) blk = BlkXBarWgsl(blk, atoi(nb));
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_UNROLL") != nullptr) blk = BlkUnrollWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_NOSCAN") != nullptr) blk = BlkNoScanWgsl(blk);
    if (getenv("OFFICIAL_AETHER_MATCH_DAWN_BLK_NOSTAGE") != nullptr) blk = BlkNoStageWgsl(blk);
    wgpu::ShaderModule m = CompileWgsl(c, blk.c_str(), "blocked plain");
    if (!m) return false;
    c.p_main = MakePipeline(c, m, "main");
    c.p_merge = MakePipeline(c, m, "merge");
    return c.p_main && c.p_merge;
  }
  wgpu::ShaderModule m = CompileWgsl(c, kWgslTiledPlain, "tiled plain");
  if (!m) return false;
  c.p_main = MakePipeline(c, m, "main");
  return (bool)c.p_main;
}

bool EnsureGuidedPipeline(Ctx& c) {
  if (c.p_guided) return true;
  if (c.tried_guided) return false;
  c.tried_guided = true;
  std::string src;
  if (c.backend == Backend::kMma) {
    src = std::string(kWgslMmaGuidedHead) + kWgslGuidedCommon +
          kWgslMmaGuidedBindings + kWgslMmaGuidedMain;
  } else {
    src = std::string(kWgslTiledGuidedHead) + kWgslGuidedCommon +
          kWgslTiledGuidedBindings + kWgslTiledGuidedMain;
  }
                                                                             
                                                                           
                                                
                                                                           
                                                                             
                                                                            
                                                                     
  if (c.mixed) src = MixedWgsl(src.c_str());
  if (c.packed) src = PackedWgsl(src);
  wgpu::ShaderModule m = CompileWgsl(c, src, c.mixed ? "guided mixed" : "guided");
  if (!m) return false;
  c.p_guided = MakePipeline(c, m, "main");
  return (bool)c.p_guided;
}

                                                                                                                                                                                                            
wgpu::Buffer PoolGet(Ctx& c, PoolBuf& p, uint64_t need,
                     wgpu::BufferUsage usage) {
  need = RoundUp(std::max<uint64_t>(need, 16), kSlot);
  if (!p.buf || p.cap < need || p.usage != usage) {
    if (p.buf) p.buf.Destroy();
    wgpu::BufferDescriptor d{};
    d.usage = usage;
    d.size = need;
    p.buf = c.device.CreateBuffer(&d);
    p.usage = usage;
    p.cap = p.buf ? need : 0;
  }
  return p.buf;
}

constexpr wgpu::BufferUsage kUsageDesc =
    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
constexpr wgpu::BufferUsage kUsageOut = wgpu::BufferUsage::Storage |
                                        wgpu::BufferUsage::CopyDst |
                                        wgpu::BufferUsage::CopySrc;
constexpr wgpu::BufferUsage kUsageColp = wgpu::BufferUsage::Storage;
constexpr wgpu::BufferUsage kUsageUni =
    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
constexpr wgpu::BufferUsage kUsageStaging =
    wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

                                                                       
uint64_t DescBytes(const Ctx& c, uint32_t n, uint32_t npad) {
  if (c.backend == Backend::kBlocked) return (uint64_t)npad * kD;                          
  if (c.backend != Backend::kMma) return (uint64_t)n * kD;
  if (c.packed) return (uint64_t)npad * kD;                                   
  return (uint64_t)npad * kD * (c.mixed ? sizeof(uint16_t) : sizeof(float));
}

                                                                               
                                                                                
                                                                 
void UploadDesc(Ctx& c, wgpu::Buffer buf, uint64_t off, const uint8_t* d,
                uint32_t n, uint32_t npad, std::vector<float>& scratch) {
  if ((c.backend == Backend::kMma && c.packed) || c.backend == Backend::kBlocked) {
                                                                                                          
    c.queue.WriteBuffer(buf, off, d, (uint64_t)n * kD);
    if (npad > n) {
      const size_t padBytes = (size_t)(npad - n) * kD;
      if (c.padZero.size() < padBytes) c.padZero.assign(padBytes, 0);
      c.queue.WriteBuffer(buf, off + (uint64_t)n * kD, c.padZero.data(), padBytes);
    }
    return;
  }
  if (c.backend == Backend::kMma && c.mixed) {
                                                                              
    c.scratch16.assign((size_t)npad * kD, 0);
    const size_t live16 = (size_t)n * kD;
    for (size_t i = 0; i < live16; ++i) {
      const _Float16 v = (_Float16)(float)d[i];
      uint16_t bits;
      std::memcpy(&bits, &v, sizeof(bits));
      c.scratch16[i] = bits;
    }
    c.queue.WriteBuffer(buf, off,
                        reinterpret_cast<const uint8_t*>(c.scratch16.data()),
                        (uint64_t)npad * kD * sizeof(uint16_t));
    return;
  }
  if (c.backend == Backend::kMma) {
    scratch.assign((size_t)npad * kD, 0.0f);
    const size_t live = (size_t)n * kD;
    for (size_t i = 0; i < live; ++i) scratch[i] = (float)d[i];
    c.queue.WriteBuffer(buf, off, reinterpret_cast<const uint8_t*>(scratch.data()),
                        (uint64_t)npad * kD * sizeof(float));
  } else {
    c.queue.WriteBuffer(buf, off, d, (uint64_t)n * kD);
  }
}

void FillSentinel(Ctx& c, wgpu::Buffer buf, uint64_t off, uint32_t n) {
  if (c.sentinel.size() < n) c.sentinel.assign(n, kOutSentinel);
  c.queue.WriteBuffer(buf, off, reinterpret_cast<const uint8_t*>(c.sentinel.data()),
                      (uint64_t)n * sizeof(int32_t));
}

wgpu::BindGroupEntry BE(uint32_t binding, wgpu::Buffer b, uint64_t off,
                        uint64_t size) {
  wgpu::BindGroupEntry e{};
  e.binding = binding;
  e.buffer = b;
  e.offset = off;
  e.size = size;
  return e;
}

wgpu::BindGroup MakeBG(Ctx& c, const wgpu::ComputePipeline& p,
                       const std::vector<wgpu::BindGroupEntry>& entries) {
  wgpu::BindGroupDescriptor d{};
  d.layout = p.GetBindGroupLayout(0);
  d.entryCount = entries.size();
  d.entries = entries.data();
  return c.device.CreateBindGroup(&d);
}

                                                                                                                                                  
int WaitFuture(Ctx& c, wgpu::Future f, const char* what) {
  const uint64_t ms = CmdWaitTimeoutMs();
  const wgpu::WaitStatus ws = c.instance.WaitAny(f, ms * 1000000ull);
  if (ws == wgpu::WaitStatus::TimedOut) {
    static std::atomic<long> gTimeoutCount{0};
    const long k = ++gTimeoutCount;
    if (k <= 5 || (k % 100) == 0) {
      Log("%s wait TIMEOUT #%ld after %llu ms (rc=7)", what, k,
          (unsigned long long)ms);
    }
    StashLastError(std::string("host-side wait timeout after ") +
                   std::to_string((unsigned long long)ms) + " ms (" + what + ")");
    ClearAllDescriptorResidencyForDeviceError();
    return 7;
  }
  if (ws != wgpu::WaitStatus::Success) {
    StashLastError(std::string("WaitAny error (") + what + ")");
    return 7;
  }
  return 0;
}

                                                                            
                                                                               
                                                  
int SubmitAndWait(Ctx& c, wgpu::CommandBuffer cb, double* gpu_ms) {
  gErrorType.store(0);
  const double t0 = NowMs();
  c.queue.Submit(1, &cb);
  wgpu::QueueWorkDoneStatus st = wgpu::QueueWorkDoneStatus::Error;
  wgpu::Future f = c.queue.OnSubmittedWorkDone(
      wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::QueueWorkDoneStatus s, wgpu::StringView) { st = s; });
  const int rc = WaitFuture(c, f, "submit");
  if (gpu_ms) *gpu_ms = NowMs() - t0;
  if (rc != 0) return rc;
  const int lost = gDeviceLostRc.load();
  if (lost != 0) {
    ClearAllDescriptorResidencyForDeviceError();
    return lost;
  }
  if (st != wgpu::QueueWorkDoneStatus::Success) {
    static std::atomic<long> gCmdErrCount{0};
    const long k = ++gCmdErrCount;
    if (k <= 5 || (k % 100) == 0) {
      Log("queue work done status=%u (rc=7) #%ld", (unsigned)st, k);
    }
    StashLastError("OnSubmittedWorkDone status != Success");
    ClearAllDescriptorResidencyForDeviceError();
    return 7;
  }
  const int et = gErrorType.load();
  if (et != 0) {
    return et == (int)wgpu::ErrorType::OutOfMemory ? 5 : 7;
  }
  return 0;
}

int ReadbackStaging(Ctx& c, wgpu::Buffer staging, uint64_t bytes, void* dst) {
  bool mapped = false;
  std::string msg;
  wgpu::Future f = staging.MapAsync(
      wgpu::MapMode::Read, 0, (size_t)bytes, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::MapAsyncStatus s, wgpu::StringView m) {
        mapped = s == wgpu::MapAsyncStatus::Success;
        if (!mapped) msg = SV(m);
      });
  const int rc = WaitFuture(c, f, "map");
  if (rc != 0) return rc;
  if (!mapped) {
    StashLastError("MapAsync failed: " + msg);
    return 7;
  }
  const void* p = staging.GetConstMappedRange(0, (size_t)bytes);
  if (!p) {
    staging.Unmap();
    StashLastError("GetConstMappedRange returned null");
    return 7;
  }
  std::memcpy(dst, p, (size_t)bytes);
  staging.Unmap();
  return 0;
}

                                                                                                                                  
                                                                         
                                                                            
                                                                        
struct Stage {
                                                                                   
                                                                                
                                                                                                   
  bool col_chunkable = false;
  uint32_t tile_cols = 32;                                               
  uint32_t total_groups = 0;
  uint32_t nDb = 0;                                        
  wgpu::Buffer uni;
  uint64_t uniOff = 0;
  std::function<void(wgpu::ComputePassEncoder&, uint32_t, uint32_t)> encode;
};

                                                                                                          
void TsEnsure(Ctx& c) {
  if (!c.ts_on || c.ts_qset) return;
  wgpu::QuerySetDescriptor qd{};
  qd.type = wgpu::QueryType::Timestamp;
  qd.count = Ctx::kTsSlots;
  c.ts_qset = c.device.CreateQuerySet(&qd);
  wgpu::BufferDescriptor rd{};
  rd.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
  rd.size = (uint64_t)Ctx::kTsSlots * 8;
  c.ts_resolve = c.device.CreateBuffer(&rd);
  wgpu::BufferDescriptor md{};
  md.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
  md.size = rd.size;
  c.ts_map = c.device.CreateBuffer(&md);
}

                                                                                      
wgpu::ComputePassEncoder BeginPassTs(Ctx& c, wgpu::CommandEncoder& enc) {
  if (!c.ts_on) return enc.BeginComputePass();
  TsEnsure(c);
  if (c.ts_slot + 2 > Ctx::kTsSlots) return enc.BeginComputePass();
  wgpu::PassTimestampWrites tw{};
  tw.querySet = c.ts_qset;
  tw.beginningOfPassWriteIndex = c.ts_slot;
  tw.endOfPassWriteIndex = c.ts_slot + 1;
  c.ts_slot += 2;
  wgpu::ComputePassDescriptor pd{};
  pd.timestampWrites = &tw;
  return enc.BeginComputePass(&pd);
}

                                                                       
double TsDrainMs(Ctx& c) {
  if (!c.ts_on || !c.ts_qset || c.ts_slot == 0) return -1.0;
  const uint32_t used = c.ts_slot;
  c.ts_slot = 0;
  wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
  enc.ResolveQuerySet(c.ts_qset, 0, used, c.ts_resolve, 0);
  enc.CopyBufferToBuffer(c.ts_resolve, 0, c.ts_map, 0, (uint64_t)used * 8);
  wgpu::CommandBuffer cb = enc.Finish();
  if (SubmitAndWait(c, cb, nullptr) != 0) return -1.0;
  bool done = false;
  wgpu::Future f = c.ts_map.MapAsync(
      wgpu::MapMode::Read, 0, (size_t)used * 8, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::MapAsyncStatus, wgpu::StringView) { done = true; });
  if (WaitFuture(c, f, "ts-map") != 0) return -1.0;
  double total_ns = 0.0;
  if (done) {
    const uint64_t* p =
        static_cast<const uint64_t*>(c.ts_map.GetConstMappedRange(0, (size_t)used * 8));
    if (p) {
      for (uint32_t i = 0; i + 1 < used; i += 2) {
        if (p[i + 1] > p[i]) total_ns += (double)(p[i + 1] - p[i]);
      }
    }
    c.ts_map.Unmap();
  }
  return total_ns / 1e6;
}

int RunStages(Ctx& c, std::vector<Stage>& stages,
              const std::function<void(wgpu::CommandEncoder&)>& finish,
              double* ema,
              const std::function<void(wgpu::CommandEncoder&)>& pre = nullptr) {
                                                                                                                                                      
  const double chunkTargetMs = ChunkTargetMs();
  if (chunkTargetMs <= 0.0) {
                                                                    
    wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
    if (pre) pre(enc);
    for (Stage& s : stages) {
      const uint32_t zero = 0;
      c.queue.WriteBuffer(s.uni, s.uniOff + kRowBaseOffset,
                          reinterpret_cast<const uint8_t*>(&zero), 4);
      wgpu::ComputePassEncoder pass = BeginPassTs(c, enc);
      s.encode(pass, 0u, s.total_groups);
      pass.End();
    }
    finish(enc);
    wgpu::CommandBuffer cb = enc.Finish();
    const int rc0 = SubmitAndWait(c, cb, nullptr);
    if (rc0 == 0 && c.ts_on) {
      const double ts = TsDrainMs(c);
      if (ts >= 0.0) aether_match_gpu_ms = ts;                               
    }
    return rc0;
  }
                                                                                               
                                                                                                                                             
                                                                                                                                                          
  static const bool kFinishFold = getenv("OFFICIAL_AETHER_MATCH_DAWN_NOFINISHFOLD") == nullptr;
  bool folded = false;
  if (kColChunk && stages.size() == 1 && stages[0].col_chunkable) {
    Stage& s = stages[0];
    uint32_t c0 = 0;
    while (c0 < s.nDb) {
      const double target = ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
      const double unit = *ema;                             
      uint32_t span = 1024;                        
      if (unit > 0.0) {
        const double perCol = unit * (double)s.total_groups / 1024.0;
        const double ideal = target / (perCol > 1e-9 ? perCol : 1e-9);
        span = ideal < 32.0 ? 32u : (uint32_t)std::min(ideal, 1e9);
      }
      span = ((span + s.tile_cols - 1u) / s.tile_cols) * s.tile_cols;                                    
      if (span > s.nDb - c0) span = s.nDb - c0;
      c.queue.WriteBuffer(s.uni, s.uniOff + kColBaseOffset,
                          reinterpret_cast<const uint8_t*>(&c0), 4);
      c.queue.WriteBuffer(s.uni, s.uniOff + kColSpanOffset,
                          reinterpret_cast<const uint8_t*>(&span), 4);
      wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
      if (pre && c0 == 0u) pre(enc);
      wgpu::ComputePassEncoder pass = BeginPassTs(c, enc);
      s.encode(pass, 0u, s.total_groups);
      pass.End();
      const bool lastChunk = (c0 + span >= s.nDb);
      if (kFinishFold && lastChunk) { finish(enc); folded = true; }
      wgpu::CommandBuffer cb = enc.Finish();
      double gpuMs = 0.0;
      const int rc = SubmitAndWait(c, cb, &gpuMs);
      if (rc != 0) return rc;
      if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
      ++aether_match_chunks;
      if (!folded && gpuMs > 0.0 && gpuMs < 10000.0) {                                                                       
        const double u =
            gpuMs / ((double)s.total_groups * ((double)span / 1024.0));
        const double prev = *ema;
        *ema = prev <= 0.0 ? u : prev * 0.7 + u * 0.3;
      }
      const double gapPct = ThermalGapPct();
      if (gapPct > 0.0 && gpuMs > 0.0) {
        double gapMs = gpuMs * gapPct / 100.0;
        if (gapMs > 250.0) gapMs = 250.0;
        aether_match_sleep_ms += gapMs;
        std::this_thread::sleep_for(
            std::chrono::microseconds((long long)(gapMs * 1000.0)));
      }
      c0 += span;
    }
                                                                                       
    const uint32_t zero = 0u;
    c.queue.WriteBuffer(s.uni, s.uniOff + kColBaseOffset,
                        reinterpret_cast<const uint8_t*>(&zero), 4);
    c.queue.WriteBuffer(s.uni, s.uniOff + kColSpanOffset,
                        reinterpret_cast<const uint8_t*>(&s.nDb), 4);
  } else
  for (Stage& s : stages) {
    uint32_t tg0 = 0;
    while (tg0 < s.total_groups) {
      const double target = ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
      const double unit = *ema;
      uint32_t want = 8;                                             
      if (unit > 0.0) {
        const double perTg = unit * ((double)s.nDb / 1024.0);
        const double ideal = target / (perTg > 1e-6 ? perTg : 1e-6);
        want = ideal < 1.0 ? 1u : (uint32_t)std::min(ideal, 1e9);
      }
      const uint32_t groups =
          want < s.total_groups - tg0 ? want : s.total_groups - tg0;
      c.queue.WriteBuffer(s.uni, s.uniOff + kRowBaseOffset,
                          reinterpret_cast<const uint8_t*>(&tg0), 4);
      wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
      wgpu::ComputePassEncoder pass = BeginPassTs(c, enc);
      s.encode(pass, tg0, groups);
      pass.End();
      wgpu::CommandBuffer cb = enc.Finish();
      double gpuMs = 0.0;
      const int rc = SubmitAndWait(c, cb, &gpuMs);
      if (rc != 0) return rc;
      if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
      ++aether_match_chunks;
      if (gpuMs > 0.0 && gpuMs < 10000.0) {
        const double u = gpuMs / ((double)groups * ((double)s.nDb / 1024.0));
        const double prev = *ema;
        *ema = prev <= 0.0 ? u : prev * 0.7 + u * 0.3;
      }
      const double gapPct = ThermalGapPct();
      if (gapPct > 0.0 && gpuMs > 0.0) {
        double gapMs = gpuMs * gapPct / 100.0;
        if (gapMs > 250.0) gapMs = 250.0;
        aether_match_sleep_ms += gapMs;
        std::this_thread::sleep_for(
            std::chrono::microseconds((long long)(gapMs * 1000.0)));
      }
      tg0 += groups;
    }
  }
  if (folded) {
    if (c.ts_on) {
      const double ts = TsDrainMs(c);
      if (ts >= 0.0) aether_match_gpu_ms = ts;
    }
    return 0;
  }
  wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
  finish(enc);
  wgpu::CommandBuffer cb = enc.Finish();
  const int rcF = SubmitAndWait(c, cb, nullptr);
  if (rcF == 0 && c.ts_on) {
    const double ts = TsDrainMs(c);
    if (ts >= 0.0) aether_match_gpu_ms = ts;                                     
  }
  return rcF;
}

                                                                                          
using aether::sfm::DescriptorFormatV1;
using aether::sfm::DescriptorResidencyKeyHashV1;
using aether::sfm::DescriptorResidencyKeyV1;
using aether::sfm::DescriptorResidencyMetadataV1;
using aether::sfm::DescriptorResidencyPolicyV1;

struct ResidencySession {
  explicit ResidencySession(uint64_t budget) : policy(budget) {}
  DescriptorResidencyPolicyV1 policy;
  std::unordered_map<DescriptorResidencyKeyV1, wgpu::Buffer,
                     DescriptorResidencyKeyHashV1>
      buffers;
  uint64_t upload_bytes = 0;
  uint64_t allocation_failures = 0;
  uint64_t device_resets = 0;
                                                                                                                         
                                                                                                                                        
                                                                                                                                              
  struct Xposed { wgpu::Buffer buf; uint32_t npad = 0; uint32_t layout = 0; };
  std::unordered_map<DescriptorResidencyKeyV1, Xposed, DescriptorResidencyKeyHashV1> xposed;
  std::deque<DescriptorResidencyKeyV1> xorder;
  uint64_t xres_hits = 0, xres_misses = 0;
};

std::unordered_map<uint64_t, std::unique_ptr<ResidencySession>> gResidency;

ResidencySession* ResidencyFor(uint64_t nonce) {
  auto found = gResidency.find(nonce);
  if (found != gResidency.end()) return found->second.get();
  auto inserted = gResidency.emplace(
      nonce, std::make_unique<ResidencySession>(DescriptorResidencyBudgetBytes()));
  return inserted.first->second.get();
}

void EraseResident(ResidencySession* s,
                   const std::vector<DescriptorResidencyKeyV1>& keys) {
  if (!s) return;
  for (const auto& k : keys) {
    auto it = s->buffers.find(k);
    if (it != s->buffers.end()) {
      if (it->second) it->second.Destroy();
      s->buffers.erase(it);
    }
    auto xt = s->xposed.find(k);
    if (xt != s->xposed.end()) {
      if (xt->second.buf) xt->second.buf.Destroy();
      s->xposed.erase(xt);
    }
  }
}

void ClearAllDescriptorResidencyForDeviceError() {
  for (auto& kv : gResidency) {
    kv.second->policy.ClearAll();
    kv.second->buffers.clear();
    ++kv.second->device_resets;
  }
}

                                                                            
                                                          
wgpu::Buffer ResidentBuffer(Ctx& c, uint64_t nonce, uint32_t frame,
                            uint32_t generation, const uint8_t* desc,
                            uint32_t n, uint32_t npad, std::vector<float>& scratch) {
  if (!DescriptorResidencyEnabled() || nonce == 0 || !desc || n == 0 ||
      npad < n) {
    return nullptr;
  }
  ResidencySession* s = ResidencyFor(nonce);
  const DescriptorResidencyKeyV1 key{nonce, frame, generation};
  const uint64_t bytes = DescBytes(c, n, npad);
                                                                        
                                                                          
                                            
  const DescriptorResidencyMetadataV1 md{
      n, c.backend == Backend::kMma ? DescriptorFormatV1::kHalf
                                    : DescriptorFormatV1::kRawU8,
      bytes};
  auto access = s->policy.Access(key, md);
  EraseResident(s, access.evicted);
  if (access.hit) {
    auto found = s->buffers.find(key);
    if (found != s->buffers.end() && found->second) return found->second;
    EraseResident(s, s->policy.InvalidateFrame(nonce, frame));
    access = s->policy.Access(key, md);
    EraseResident(s, access.evicted);
  }
  if (!access.admitted) return nullptr;
  wgpu::BufferDescriptor d{};
  d.usage = kUsageDesc;
  d.size = RoundUp(std::max<uint64_t>(bytes, 16), 4);
  wgpu::Buffer buf = c.device.CreateBuffer(&d);
  if (!buf) {
    ++s->allocation_failures;
    EraseResident(s, s->policy.InvalidateFrame(nonce, frame));
    return nullptr;
  }
  UploadDesc(c, buf, 0, desc, n, npad, scratch);
  s->upload_bytes += bytes;
  s->buffers[key] = buf;
  return buf;
}

                                                                                                                                                    
                                                             
int CrossCheck(const int32_t* mAB, int nA, const int32_t* mBA, int nB,
               uint32_t* out_pairs, int max_pairs, int* out_num) {
  for (int i = 0; i < nA; ++i) {
    if (mAB[i] == kOutSentinel) {
      StashLastError("output incomplete (A->B sentinel survived)");
      return 7;
    }
  }
  for (int j = 0; j < nB; ++j) {
    if (mBA[j] == kOutSentinel) {
      StashLastError("output incomplete (B->A sentinel survived)");
      return 7;
    }
  }
  int n_out = 0;
  for (int i = 0; i < nA; ++i) {
    const int j = mAB[i];
    if (j >= 0 && j < nB && mBA[j] == i) {
      if (out_pairs != nullptr) {
        if (n_out >= max_pairs) break;                             
        out_pairs[2 * n_out] = (uint32_t)i;
        out_pairs[2 * n_out + 1] = (uint32_t)j;
      }
      ++n_out;
    }
  }
  if (out_num) *out_num = n_out;
  return 0;
}

#if defined(PWOFFICIAL_DAWN_HOST_TEST) && PWOFFICIAL_DAWN_HOST_TEST
std::vector<int32_t> gDbgAB, gDbgBA;
double gDbgUploadMs = 0.0, gDbgSubmitMs = 0.0, gDbgReadbackMs = 0.0;
#endif

                                                                                                                                                                                                    
int MatchPairsImpl(const uint8_t* dA, int nA, const uint8_t* dB, int nB,
                   double max_ratio, uint32_t* out_pairs, int max_pairs,
                   int* out_num, uint64_t nonce = 0, uint32_t frameA = 0,
                   uint32_t genA = 0, uint32_t frameB = 0, uint32_t genB = 0) {
  if (out_num) *out_num = 0;
  if (!dA || !dB || nA <= 0 || nB <= 0) return 1;
  if (out_pairs != nullptr && max_pairs <= 0) return 1;
  Ctx* cp = EnsureDawn();
  if (!cp) return 2;
  Ctx& c = *cp;
  if (!EnsureMainPipelines(c)) return 2;
  const double tStart = NowMs();
  std::function<void(wgpu::CommandEncoder&)> preXpose;                                        

  const uint32_t nAu = (uint32_t)nA, nBu = (uint32_t)nB;
  const uint32_t nApad = (uint32_t)RoundUp(nAu, MainRows(c.backend));
  const uint32_t nBpad = (uint32_t)RoundUp(nBu, 128);
  const uint64_t aBytes = DescBytes(c, nAu, nApad);
  const uint64_t bBytes = DescBytes(c, nBu, nBpad);

  wgpu::Buffer aBuf = ResidentBuffer(c, nonce, frameA, genA, dA, nAu, nApad,
                                     c.scratchA);
  wgpu::Buffer bBuf = ResidentBuffer(c, nonce, frameB, genB, dB, nBu, nBpad,
                                     c.scratchB);
  const bool aRes = (bool)aBuf, bRes = (bool)bBuf;
  if (!aBuf) aBuf = PoolGet(c, c.a, aBytes, kUsageDesc);
  if (!bBuf) bBuf = PoolGet(c, c.b, bBytes, kUsageDesc);
  if (!aBuf || !bBuf) return 5;
  if (!aRes) UploadDesc(c, aBuf, 0, dA, nAu, nApad, c.scratchA);
  if (!bRes) UploadDesc(c, bBuf, 0, dB, nBu, nBpad, c.scratchB);

  const uint64_t outABBytes = (uint64_t)nAu * 4, outBABytes = (uint64_t)nBu * 4;
  wgpu::Buffer outAB = PoolGet(c, c.outAB, outABBytes, kUsageOut);
  wgpu::Buffer outBA = PoolGet(c, c.outBA, outBABytes, kUsageOut);
  wgpu::Buffer uni = PoolGet(c, c.uni, 4 * kSlot, kUsageUni);                                     
  wgpu::Buffer staging =
      PoolGet(c, c.staging, outABBytes + outBABytes, kUsageStaging);
  if (!outAB || !outBA || !uni || !staging) return 6;
  FillSentinel(c, outAB, 0, nAu);
  FillSentinel(c, outBA, 0, nBu);

  float maxRatio = (float)max_ratio;
  if (maxRatio <= 0.0f) maxRatio = 0.8f;
  const float maxDistance = 0.7f;                                             

  std::vector<Stage> stages;
  std::function<void(wgpu::CommandEncoder&)> finish;
  wgpu::BindGroup bgMain, bgMerge, bgBA;
  const double tUpload = NowMs();
  if (c.backend == Backend::kMma || c.backend == Backend::kBlocked) {
    const uint32_t numWg = nApad / MainRows(c.backend);
    const uint64_t colBytes = (uint64_t)nBu * numWg * 12;
    wgpu::Buffer colp = PoolGet(c, c.colp, colBytes, kUsageColp);
    if (!colp) return 6;
                                                                                                
    const uint64_t rowBytes = (uint64_t)nApad * 12;
    wgpu::Buffer rowp = PoolGet(c, c.rowp, rowBytes, kUsageColp);
    if (!rowp) return 6;
    const Params p{nAu, nBu, maxRatio, maxDistance, numWg, 0u, 0u, nBu};
    c.queue.WriteBuffer(uni, 0, reinterpret_cast<const uint8_t*>(&p), sizeof(p));
    std::vector<wgpu::BindGroupEntry> mainEntries = {
        BE(0, aBuf, 0, aBytes), BE(1, bBuf, 0, bBytes),
        BE(2, outAB, 0, outABBytes), BE(3, uni, 0, sizeof(Params)),
        BE(4, colp, 0, colBytes), BE(6, rowp, 0, rowBytes)};
    wgpu::BindGroup bgXA, bgXB;
    bool xposeA = true, xposeB = true;                                          
    if (c.direct) {
                                                                                                              
                                                                                              
      mainEntries.erase(mainEntries.begin(), mainEntries.begin() + 2);
                                                                                             
      const uint64_t rowB = c.direct_f16 ? 256ull : (c.direct_packed ? 128ull : 512ull);                                   
      const uint64_t atBytes = rowB * nApad, btBytes = rowB * nBpad;
                                                                                                                                           
      static const bool kXres = getenv("OFFICIAL_AETHER_MATCH_DAWN_NOXRES") == nullptr;
      const uint32_t xlayout = (c.direct_f16 ? 1u : 0u) | (c.direct_kp ? 2u : 0u) | (c.direct_packed ? 4u : 0u);
      ResidencySession* rs = (kXres && nonce != 0 && DescriptorResidencyEnabled() && !c.direct_tex && !c.direct_texa)
                                 ? ResidencyFor(nonce) : nullptr;
      auto xlookup = [&](bool res, uint32_t frame, uint32_t gen, uint32_t npad, uint64_t bytes, PoolBuf& pool,
                         wgpu::BufferUsage usage, bool& needX) -> wgpu::Buffer {
        needX = true;
        if (rs && res) {
          const DescriptorResidencyKeyV1 key{nonce, frame, gen};
          auto it = rs->xposed.find(key);
          if (it != rs->xposed.end() && it->second.buf && it->second.npad == npad && it->second.layout == xlayout) {
            ++rs->xres_hits; needX = false; return it->second.buf;
          }
          ++rs->xres_misses;
          wgpu::BufferDescriptor d{};
          d.usage = kUsageColp;
          d.size = RoundUp(std::max<uint64_t>(bytes, 16), 4);
          wgpu::Buffer nb = c.device.CreateBuffer(&d);
          if (nb) {
            if (it != rs->xposed.end()) { if (it->second.buf) it->second.buf.Destroy(); rs->xposed.erase(it); }
            else rs->xorder.push_back(key);
            rs->xposed[key] = ResidencySession::Xposed{nb, npad, xlayout};
            while (rs->xorder.size() > 48) {
              const DescriptorResidencyKeyV1 old = rs->xorder.front(); rs->xorder.pop_front();
              auto ot = rs->xposed.find(old);
              if (ot != rs->xposed.end() && ot->second.buf.Get() != nb.Get()) { if (ot->second.buf) ot->second.buf.Destroy(); rs->xposed.erase(ot); }
            }
            return nb;
          }
        }
        return PoolGet(c, pool, bytes, usage);
      };
      wgpu::Buffer at = xlookup(aRes, frameA, genA, nApad, atBytes, c.at,
                                c.direct_texa ? (kUsageColp | wgpu::BufferUsage::CopySrc) : kUsageColp, xposeA);
      wgpu::Buffer bt = xlookup(bRes, frameB, genB, nBpad, btBytes, c.bt,
                                c.direct_tex ? (kUsageColp | wgpu::BufferUsage::CopySrc) : kUsageColp, xposeB);
      if (!at || !bt) return 6;
      if (rs && ((rs->xres_hits + rs->xres_misses) % 256u) == 0u) {
        if (const char* home = getenv("HOME")) {
          const std::string fp = std::string(home) + "/Documents/matcher_backend.jsonl";
          if (FILE* f = std::fopen(fp.c_str(), "a")) {
            std::fprintf(f, "{\"xres\":{\"hits\":%llu,\"misses\":%llu,\"entries\":%zu}}\n",
                         (unsigned long long)rs->xres_hits, (unsigned long long)rs->xres_misses, rs->xposed.size());
            std::fclose(f);
          }
        }
      }
      const uint32_t xa[4] = {nApad / 4u, 0u, 0u, 0u};
      const uint32_t xb[4] = {nBpad / 4u, 0u, 0u, 0u};
      c.queue.WriteBuffer(uni, kSlot, reinterpret_cast<const uint8_t*>(xa), 16);
      c.queue.WriteBuffer(uni, 2 * kSlot, reinterpret_cast<const uint8_t*>(xb), 16);
      if (c.direct_texa) {
        wgpu::TextureView tv = EnsureBtTexture(c, c.atTex, c.atTexW, nApad / 4u);
        if (!tv) return 6;
        wgpu::BindGroupEntry te{};
        te.binding = 7;
        te.textureView = tv;
        mainEntries.push_back(te);
      } else {
        mainEntries.push_back(BE(7, at, 0, atBytes));
      }
      if (c.direct_tex) {
        wgpu::TextureView tv = EnsureBtTexture(c, c.btTex, c.btTexW, nBpad / 4u);
        if (!tv) return 6;
        wgpu::BindGroupEntry te{};
        te.binding = 8;
        te.textureView = tv;
        mainEntries.push_back(te);
      } else {
        mainEntries.push_back(BE(8, bt, 0, btBytes));
      }
      if (c.direct_g) {
        const uint64_t scrBytes = 16384ull * numWg;                            
        wgpu::Buffer scr = PoolGet(c, c.scr, scrBytes, kUsageColp);
        if (!scr) return 6;
        mainEntries.push_back(BE(9, scr, 0, scrBytes));
      }
      bgXA = MakeBG(c, c.p_xpose, {BE(0, aBuf, 0, aBytes), BE(3, uni, kSlot, 16),
                                   BE(7, at, 0, atBytes)});
      bgXB = MakeBG(c, c.p_xpose, {BE(0, bBuf, 0, bBytes), BE(3, uni, 2 * kSlot, 16),
                                   BE(7, bt, 0, btBytes)});
      if (!bgXA || !bgXB) return 6;
    }
    bgMain = MakeBG(c, c.p_main, mainEntries);
    bgMerge = MakeBG(c, c.p_merge,
                     {BE(3, uni, 0, sizeof(Params)), BE(4, colp, 0, colBytes),
                      BE(5, outBA, 0, outBABytes)});
    if (!bgMain || !bgMerge) return 6;
    static const bool kXfold = getenv("OFFICIAL_AETHER_MATCH_DAWN_NOXFOLD") == nullptr;
    if (c.direct && (xposeA || xposeB) && kXfold && kColChunk && !c.direct_tex && !c.direct_texa && ChunkTargetMs() > 0.0) {
      preXpose = [&, xposeA, xposeB, bgXA, bgXB, nApad, nBpad](wgpu::CommandEncoder& enc) {
        wgpu::ComputePassEncoder pass = enc.BeginComputePass();
        pass.SetPipeline(c.p_xpose);
        if (xposeA) { pass.SetBindGroup(0, bgXA); pass.DispatchWorkgroups((nApad / 4u * 32u + 63u) / 64u); }
        if (xposeB) { pass.SetBindGroup(0, bgXB); pass.DispatchWorkgroups((nBpad / 4u * 32u + 63u) / 64u); }
        pass.End();
      };
    } else if (c.direct && (xposeA || xposeB)) {
                                                                                                                       
      wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
      wgpu::ComputePassEncoder pass = enc.BeginComputePass();
      pass.SetPipeline(c.p_xpose);
      if (xposeA) { pass.SetBindGroup(0, bgXA); pass.DispatchWorkgroups((nApad / 4u * 32u + 63u) / 64u); }
      if (xposeB) { pass.SetBindGroup(0, bgXB); pass.DispatchWorkgroups((nBpad / 4u * 32u + 63u) / 64u); }
      pass.End();
      if (c.direct_tex) EncodeBtToTexture(enc, c.bt.buf, c.btTex, nBpad / 4u);
      if (c.direct_texa) EncodeBtToTexture(enc, c.at.buf, c.atTex, nApad / 4u);
      wgpu::CommandBuffer cb = enc.Finish();
      const int rcx = SubmitAndWait(c, cb, nullptr);
      if (rcx != 0) return rcx;
    }
    Stage s;
    s.col_chunkable = true;                                                     
    s.tile_cols = MainTileCols(c.backend);
    s.total_groups = numWg;
    s.nDb = nBu;
    s.uni = uni;
    s.uniOff = 0;
    s.encode = [&](wgpu::ComputePassEncoder& pass, uint32_t, uint32_t groups) {
      pass.SetPipeline(c.p_main);
      pass.SetBindGroup(0, bgMain);
      pass.DispatchWorkgroups(groups);
    };
    stages.push_back(s);
    finish = [&](wgpu::CommandEncoder& enc) {
      wgpu::ComputePassEncoder pass = enc.BeginComputePass();
      pass.SetPipeline(c.p_merge);
      pass.SetBindGroup(0, bgMerge);
      pass.DispatchWorkgroups((nBu + 63) / 64);
      pass.End();
      enc.CopyBufferToBuffer(outAB, 0, staging, 0, outABBytes);
      enc.CopyBufferToBuffer(outBA, 0, staging, outABBytes, outBABytes);
    };
  } else {
    const Params pab{nAu, nBu, maxRatio, maxDistance, 0u, 0u, 0u, 0u};
    const Params pba{nBu, nAu, maxRatio, maxDistance, 0u, 0u, 0u, 0u};
    c.queue.WriteBuffer(uni, 0, reinterpret_cast<const uint8_t*>(&pab), sizeof(pab));
    c.queue.WriteBuffer(uni, kSlot, reinterpret_cast<const uint8_t*>(&pba),
                        sizeof(pba));
    bgMain = MakeBG(c, c.p_main,
                    {BE(0, aBuf, 0, aBytes), BE(1, bBuf, 0, bBytes),
                     BE(2, outAB, 0, outABBytes), BE(3, uni, 0, sizeof(Params))});
    bgBA = MakeBG(c, c.p_main,
                  {BE(0, bBuf, 0, bBytes), BE(1, aBuf, 0, aBytes),
                   BE(2, outBA, 0, outBABytes), BE(3, uni, kSlot, sizeof(Params))});
    if (!bgMain || !bgBA) return 6;
    Stage sab, sba;
    sab.total_groups = (nAu + kTiledRows - 1) / kTiledRows;
    sab.nDb = nBu;
    sab.uni = uni;
    sab.uniOff = 0;
    sab.encode = [&](wgpu::ComputePassEncoder& pass, uint32_t, uint32_t groups) {
      pass.SetPipeline(c.p_main);
      pass.SetBindGroup(0, bgMain);
      pass.DispatchWorkgroups(groups);
    };
    sba.total_groups = (nBu + kTiledRows - 1) / kTiledRows;
    sba.nDb = nAu;
    sba.uni = uni;
    sba.uniOff = kSlot;
    sba.encode = [&](wgpu::ComputePassEncoder& pass, uint32_t, uint32_t groups) {
      pass.SetPipeline(c.p_main);
      pass.SetBindGroup(0, bgBA);
      pass.DispatchWorkgroups(groups);
    };
    stages.push_back(sab);
    stages.push_back(sba);
    finish = [&](wgpu::CommandEncoder& enc) {
      enc.CopyBufferToBuffer(outAB, 0, staging, 0, outABBytes);
      enc.CopyBufferToBuffer(outBA, 0, staging, outABBytes, outBABytes);
    };
  }
  int rc = RunStages(c, stages, finish, &c.ema_main, preXpose);
  if (rc != 0) return rc;
  const double tSubmit = NowMs();
  c.host.resize((size_t)nAu + nBu);
  rc = ReadbackStaging(c, staging, outABBytes + outBABytes, c.host.data());
  if (rc != 0) return rc;
  const int32_t* mAB = c.host.data();
  const int32_t* mBA = c.host.data() + nAu;
#if defined(PWOFFICIAL_DAWN_HOST_TEST) && PWOFFICIAL_DAWN_HOST_TEST
  gDbgAB.assign(mAB, mAB + nAu);
  gDbgBA.assign(mBA, mBA + nBu);
  gDbgUploadMs = tUpload - tStart;
  gDbgSubmitMs = tSubmit - tUpload;
  gDbgReadbackMs = NowMs() - tSubmit;
#else
  (void)tStart;
  (void)tUpload;
  (void)tSubmit;
#endif
  return CrossCheck(mAB, nA, mBA, nB, out_pairs, max_pairs, out_num);
}

                                                                                                                                                                      
int MatchGuidedImpl(const uint8_t* dA, int nA, const float* xyA,
                    const uint8_t* dB, int nB, const float* xyB,
                    double max_ratio, const float* matrixAB,
                    const float* matrixBA, uint32_t guideMode,
                    float maxResidual, uint32_t* out_pairs, int max_pairs,
                    int* out_num) {
  if (out_num) *out_num = 0;
  if (!dA || !dB || nA <= 0 || nB <= 0) return 1;
  if (out_pairs != nullptr && max_pairs <= 0) return 1;
  if (guideMode == 0u || guideMode > 2u) return 1;
  if (!xyA || !xyB || !matrixAB || !matrixBA || maxResidual <= 0.0f) return 1;
  Ctx* cp = EnsureDawn();
  if (!cp) return 2;
  Ctx& c = *cp;
  if (!EnsureGuidedPipeline(c)) return 2;
  const double tStart = NowMs();

  const uint32_t nAu = (uint32_t)nA, nBu = (uint32_t)nB;
  const uint32_t nApad = (uint32_t)RoundUp(nAu, 128);
  const uint32_t nBpad = (uint32_t)RoundUp(nBu, 128);
  const uint64_t aBytes = DescBytes(c, nAu, nApad);
  const uint64_t bBytes = DescBytes(c, nBu, nBpad);
  wgpu::Buffer aBuf = PoolGet(c, c.a, aBytes, kUsageDesc);
  wgpu::Buffer bBuf = PoolGet(c, c.b, bBytes, kUsageDesc);
  if (!aBuf || !bBuf) return 5;
  UploadDesc(c, aBuf, 0, dA, nAu, nApad, c.scratchA);
  UploadDesc(c, bBuf, 0, dB, nBu, nBpad, c.scratchB);

                                                                           
                                                                       
  const uint64_t ptsABytes = (uint64_t)nApad * 2 * sizeof(float);
  const uint64_t ptsBBytes = (uint64_t)nBpad * 2 * sizeof(float);
  wgpu::Buffer ptsA = PoolGet(c, c.ptsA, ptsABytes, kUsageDesc);
  wgpu::Buffer ptsB = PoolGet(c, c.ptsB, ptsBBytes, kUsageDesc);
  wgpu::Buffer matAB = PoolGet(c, c.matAB, 48, kUsageDesc);
  wgpu::Buffer matBA = PoolGet(c, c.matBA, 48, kUsageDesc);
  if (!ptsA || !ptsB || !matAB || !matBA) return 6;
  {
    std::vector<float>& sa = c.scratchA;
    sa.assign((size_t)nApad * 2, 0.0f);
    std::memcpy(sa.data(), xyA, (size_t)nAu * 2 * sizeof(float));
    c.queue.WriteBuffer(ptsA, 0, reinterpret_cast<const uint8_t*>(sa.data()),
                        ptsABytes);
    std::vector<float>& sb = c.scratchB;
    sb.assign((size_t)nBpad * 2, 0.0f);
    std::memcpy(sb.data(), xyB, (size_t)nBu * 2 * sizeof(float));
    c.queue.WriteBuffer(ptsB, 0, reinterpret_cast<const uint8_t*>(sb.data()),
                        ptsBBytes);
    float m[12] = {0};
    std::memcpy(m, matrixAB, 9 * sizeof(float));
    c.queue.WriteBuffer(matAB, 0, reinterpret_cast<const uint8_t*>(m), 48);
    std::memcpy(m, matrixBA, 9 * sizeof(float));
    c.queue.WriteBuffer(matBA, 0, reinterpret_cast<const uint8_t*>(m), 48);
  }
  const uint64_t outABBytes = (uint64_t)nAu * 4, outBABytes = (uint64_t)nBu * 4;
  wgpu::Buffer outAB = PoolGet(c, c.outAB, outABBytes, kUsageOut);
  wgpu::Buffer outBA = PoolGet(c, c.outBA, outBABytes, kUsageOut);
  wgpu::Buffer uni = PoolGet(c, c.uni, 4 * kSlot, kUsageUni);                                     
  wgpu::Buffer staging =
      PoolGet(c, c.staging, outABBytes + outBABytes, kUsageStaging);
  if (!outAB || !outBA || !uni || !staging) return 6;
  FillSentinel(c, outAB, 0, nAu);
  FillSentinel(c, outBA, 0, nBu);

  float maxRatio = (float)max_ratio;
  if (maxRatio <= 0.0f) maxRatio = 0.8f;
  const float maxDistance = 0.7f;
  const GParams pab{nAu, nBu, maxRatio, maxDistance, guideMode, 0u, maxResidual, 0u};
  const GParams pba{nBu, nAu, maxRatio, maxDistance, guideMode, 0u, maxResidual, 0u};
  c.queue.WriteBuffer(uni, 0, reinterpret_cast<const uint8_t*>(&pab), sizeof(pab));
  c.queue.WriteBuffer(uni, kSlot, reinterpret_cast<const uint8_t*>(&pba), sizeof(pba));
  wgpu::BindGroup bgAB = MakeBG(
      c, c.p_guided,
      {BE(0, aBuf, 0, aBytes), BE(1, bBuf, 0, bBytes), BE(2, outAB, 0, outABBytes),
       BE(3, uni, 0, sizeof(GParams)), BE(4, ptsA, 0, ptsABytes),
       BE(5, ptsB, 0, ptsBBytes), BE(6, matAB, 0, 48)});
  wgpu::BindGroup bgBA = MakeBG(
      c, c.p_guided,
      {BE(0, bBuf, 0, bBytes), BE(1, aBuf, 0, aBytes), BE(2, outBA, 0, outBABytes),
       BE(3, uni, kSlot, sizeof(GParams)), BE(4, ptsB, 0, ptsBBytes),
       BE(5, ptsA, 0, ptsABytes), BE(6, matBA, 0, 48)});
  if (!bgAB || !bgBA) return 6;
  const uint32_t rows = c.backend == Backend::kMma ? kMmaRows : kTiledRows;
  std::vector<Stage> stages;
  Stage sab, sba;
  sab.total_groups = (nAu + rows - 1) / rows;
  sab.nDb = nBu;
  sab.uni = uni;
  sab.uniOff = 0;
  sab.encode = [&](wgpu::ComputePassEncoder& pass, uint32_t, uint32_t groups) {
    pass.SetPipeline(c.p_guided);
    pass.SetBindGroup(0, bgAB);
    pass.DispatchWorkgroups(groups);
  };
  sba.total_groups = (nBu + rows - 1) / rows;
  sba.nDb = nAu;
  sba.uni = uni;
  sba.uniOff = kSlot;
  sba.encode = [&](wgpu::ComputePassEncoder& pass, uint32_t, uint32_t groups) {
    pass.SetPipeline(c.p_guided);
    pass.SetBindGroup(0, bgBA);
    pass.DispatchWorkgroups(groups);
  };
  stages.push_back(sab);
  stages.push_back(sba);
  auto finish = [&](wgpu::CommandEncoder& enc) {
    enc.CopyBufferToBuffer(outAB, 0, staging, 0, outABBytes);
    enc.CopyBufferToBuffer(outBA, 0, staging, outABBytes, outBABytes);
  };
  const double tUpload = NowMs();
  int rc = RunStages(c, stages, finish, &c.ema_guided);
  if (rc != 0) return rc;
  const double tSubmit = NowMs();
  c.host.resize((size_t)nAu + nBu);
  rc = ReadbackStaging(c, staging, outABBytes + outBABytes, c.host.data());
  if (rc != 0) return rc;
  const int32_t* mAB = c.host.data();
  const int32_t* mBA = c.host.data() + nAu;
#if defined(PWOFFICIAL_DAWN_HOST_TEST) && PWOFFICIAL_DAWN_HOST_TEST
  gDbgAB.assign(mAB, mAB + nAu);
  gDbgBA.assign(mBA, mBA + nBu);
  gDbgUploadMs = tUpload - tStart;
  gDbgSubmitMs = tSubmit - tUpload;
  gDbgReadbackMs = NowMs() - tSubmit;
#else
  (void)tStart;
  (void)tUpload;
  (void)tSubmit;
#endif
  return CrossCheck(mAB, nA, mBA, nB, out_pairs, max_pairs, out_num);
}

                                                                                                                                                                                                    
                                                                             
                                                                              
                                                                            
                                                                            
                                                                             
                                                                           
         
int ProbeBatchImpl(const uint8_t* dA, int nA, const uint8_t* const* dBs,
                   const int* nBs, int n_cands, double max_ratio,
                   int* out_counts) {
  if (!dA || !dBs || !nBs || !out_counts || nA <= 0 || n_cands <= 0) return 1;
  for (int k = 0; k < n_cands; ++k) {
    if (!dBs[k] || nBs[k] <= 0) return 1;
    out_counts[k] = 0;
  }
  Ctx* cp = EnsureDawn();
  if (!cp) return 2;
  Ctx& c = *cp;
  if (!EnsureMainPipelines(c)) return 2;
  const bool mma = c.backend == Backend::kMma || c.backend == Backend::kBlocked;
  const uint32_t nAu = (uint32_t)nA;
  const uint32_t nApad = (uint32_t)RoundUp(nAu, MainRows(c.backend));
  const uint32_t numWg = nApad / MainRows(c.backend);
  const uint64_t aBytes = DescBytes(c, nAu, nApad);
                                                                                           
  const uint64_t rowPbBytes = (uint64_t)nApad * 12;
  const uint64_t outABBytes = (uint64_t)nAu * 4;
  const uint64_t outABSlot = RoundUp(outABBytes, kSlot);
  const uint32_t uniPerCand = mma ? 1u : 2u;

  struct Cand {
    uint32_t nB, nBpad;
    uint64_t bOff, bBytes, outBAOff, outBABytes, colOff, colBytes, uniOff,
        stgOff;
    uint64_t btOff = 0, btBytes = 0;             
    wgpu::BindGroup bgMain, bgMerge, bgBA;
    wgpu::BindGroup bgX;                                      
    double units;
  };
  std::vector<Cand> cands((size_t)n_cands);
  uint64_t bTotal = 0, outBATotal = 0, colTotal = 0, stgTotal = 0, btTotal = 0;
  for (int k = 0; k < n_cands; ++k) {
    Cand& cd = cands[(size_t)k];
    cd.nB = (uint32_t)nBs[k];
    cd.nBpad = (uint32_t)RoundUp(cd.nB, 128);
    cd.bBytes = DescBytes(c, cd.nB, cd.nBpad);
    cd.bOff = bTotal;
    bTotal += RoundUp(cd.bBytes, kSlot);
    cd.outBABytes = (uint64_t)cd.nB * 4;
    cd.outBAOff = outBATotal;
    outBATotal += RoundUp(cd.outBABytes, kSlot);
    cd.colBytes = mma ? (uint64_t)cd.nB * numWg * 12 : 16;
    cd.colOff = colTotal;
    colTotal += RoundUp(cd.colBytes, kSlot);
    cd.uniOff = (uint64_t)k * uniPerCand * kSlot;
    cd.stgOff = stgTotal;
    stgTotal += outABBytes + cd.outBABytes;
    cd.btBytes = (c.direct_f16 ? 256ull : (c.direct_packed ? 128ull : 512ull)) * cd.nBpad;
    cd.btOff = btTotal;
    btTotal += RoundUp(cd.btBytes, kSlot);
    cd.units = mma ? (double)numWg * ((double)cd.nB / 1024.0)
                   : (double)((nAu + 63) / 64) * ((double)cd.nB / 1024.0) +
                         (double)((cd.nB + 63) / 64) * ((double)nAu / 1024.0);
  }
  wgpu::Buffer aBuf = PoolGet(c, c.pbA, aBytes, kUsageDesc);
  wgpu::Buffer bBuf = PoolGet(c, c.pbB, bTotal, kUsageDesc);
  if (!aBuf || !bBuf) return 5;
  UploadDesc(c, aBuf, 0, dA, nAu, nApad, c.scratchA);
  for (int k = 0; k < n_cands; ++k) {
    const Cand& cd = cands[(size_t)k];
    UploadDesc(c, bBuf, cd.bOff, dBs[k], cd.nB, cd.nBpad, c.scratchB);
  }
  wgpu::Buffer outAB = PoolGet(c, c.pbOutAB, outABSlot * (uint64_t)n_cands, kUsageOut);
  wgpu::Buffer outBA = PoolGet(c, c.pbOutBA, outBATotal, kUsageOut);
  wgpu::Buffer colp = PoolGet(c, c.pbColp, colTotal, kUsageColp);
  wgpu::Buffer rowpPb = PoolGet(c, c.pbRowp, rowPbBytes, kUsageColp);
  if (!rowpPb) return 6;
                                                                                           
  const uint64_t atPbBytes = (c.direct_f16 ? 256ull : (c.direct_packed ? 128ull : 512ull)) * nApad;
  wgpu::Buffer atPb, btPb, xuni;
  wgpu::BindGroup bgXA;
  if (c.direct) {
    atPb = PoolGet(c, c.pbAt, atPbBytes,
                   c.direct_texa ? (kUsageColp | wgpu::BufferUsage::CopySrc) : kUsageColp);
    btPb = PoolGet(c, c.pbBt, btTotal,
                   c.direct_tex ? (kUsageColp | wgpu::BufferUsage::CopySrc) : kUsageColp);
    if (c.direct_tex) { c.pbTex.resize((size_t)n_cands); c.pbTexW.resize((size_t)n_cands, 0u); }
    xuni = PoolGet(c, c.pbXUni, (uint64_t)(n_cands + 1) * kSlot, kUsageUni);
    if (!atPb || !btPb || !xuni) return 6;
    const uint32_t xa[4] = {nApad / 4u, 0u, 0u, 0u};
    c.queue.WriteBuffer(xuni, 0, reinterpret_cast<const uint8_t*>(xa), 16);
    bgXA = MakeBG(c, c.p_xpose, {BE(0, aBuf, 0, aBytes), BE(3, xuni, 0, 16),
                                 BE(7, atPb, 0, atPbBytes)});
    if (!bgXA) return 6;
  }
  wgpu::Buffer uni = PoolGet(c, c.pbUni, (uint64_t)n_cands * uniPerCand * kSlot,
                             kUsageUni);
  wgpu::Buffer staging = PoolGet(c, c.pbStaging, stgTotal, kUsageStaging);
  if (!outAB || !outBA || !colp || !uni || !staging) return 6;

  float maxRatio = (float)max_ratio;
  if (maxRatio <= 0.0f) maxRatio = 0.8f;
  const float maxDistance = 0.7f;
  for (int k = 0; k < n_cands; ++k) {
    Cand& cd = cands[(size_t)k];
    FillSentinel(c, outAB, (uint64_t)k * outABSlot, nAu);
    FillSentinel(c, outBA, cd.outBAOff, cd.nB);
    if (mma) {
                                                                                        
                                                                                             
                                                                                     
                                                                                        
      const Params p{nAu, cd.nB, maxRatio, maxDistance, numWg, 0u, 0u, cd.nB};
      c.queue.WriteBuffer(uni, cd.uniOff, reinterpret_cast<const uint8_t*>(&p),
                          sizeof(p));
      std::vector<wgpu::BindGroupEntry> me = {
          BE(0, aBuf, 0, aBytes), BE(1, bBuf, cd.bOff, cd.bBytes),
          BE(2, outAB, (uint64_t)k * outABSlot, outABBytes),
          BE(3, uni, cd.uniOff, sizeof(Params)), BE(4, colp, cd.colOff, cd.colBytes),
          BE(6, rowpPb, 0, rowPbBytes)};
      if (c.direct) {
        me.erase(me.begin(), me.begin() + 2);                                         
        const uint32_t xb[4] = {cd.nBpad / 4u, 0u, 0u, 0u};
        c.queue.WriteBuffer(xuni, (uint64_t)(k + 1) * kSlot,
                            reinterpret_cast<const uint8_t*>(xb), 16);
        if (c.direct_texa) {
          wgpu::TextureView tv = EnsureBtTexture(c, c.pbAtTex, c.pbAtTexW, nApad / 4u);
          if (!tv) return 6;
          wgpu::BindGroupEntry ta{};
          ta.binding = 7;
          ta.textureView = tv;
          me.push_back(ta);
        } else {
          me.push_back(BE(7, atPb, 0, atPbBytes));
        }
        if (c.direct_tex) {
          wgpu::TextureView tv = EnsureBtTexture(c, c.pbTex[(size_t)k], c.pbTexW[(size_t)k], cd.nBpad / 4u);
          if (!tv) return 6;
          wgpu::BindGroupEntry te{};
          te.binding = 8;
          te.textureView = tv;
          me.push_back(te);
        } else {
          me.push_back(BE(8, btPb, cd.btOff, cd.btBytes));
        }
        if (c.direct_g) {
          const uint64_t scrBytes = 16384ull * numWg;
          wgpu::Buffer scr = PoolGet(c, c.pbScr, scrBytes, kUsageColp);
          if (!scr) return 6;
          me.push_back(BE(9, scr, 0, scrBytes));
        }
        cd.bgX = MakeBG(c, c.p_xpose,
                        {BE(0, bBuf, cd.bOff, cd.bBytes),
                         BE(3, xuni, (uint64_t)(k + 1) * kSlot, 16),
                         BE(7, btPb, cd.btOff, cd.btBytes)});
        if (!cd.bgX) return 6;
      }
      cd.bgMain = MakeBG(c, c.p_main, me);
      cd.bgMerge = MakeBG(c, c.p_merge,
                          {BE(3, uni, cd.uniOff, sizeof(Params)),
                           BE(4, colp, cd.colOff, cd.colBytes),
                           BE(5, outBA, cd.outBAOff, cd.outBABytes)});
      if (!cd.bgMain || !cd.bgMerge) return 6;
    } else {
      const Params pab{nAu, cd.nB, maxRatio, maxDistance, 0u, 0u, 0u, 0u};
      const Params pba{cd.nB, nAu, maxRatio, maxDistance, 0u, 0u, 0u, 0u};
      c.queue.WriteBuffer(uni, cd.uniOff, reinterpret_cast<const uint8_t*>(&pab),
                          sizeof(pab));
      c.queue.WriteBuffer(uni, cd.uniOff + kSlot,
                          reinterpret_cast<const uint8_t*>(&pba), sizeof(pba));
      cd.bgMain = MakeBG(c, c.p_main,
                         {BE(0, aBuf, 0, aBytes), BE(1, bBuf, cd.bOff, cd.bBytes),
                          BE(2, outAB, (uint64_t)k * outABSlot, outABBytes),
                          BE(3, uni, cd.uniOff, sizeof(Params))});
      cd.bgBA = MakeBG(c, c.p_main,
                       {BE(0, bBuf, cd.bOff, cd.bBytes), BE(1, aBuf, 0, aBytes),
                        BE(2, outBA, cd.outBAOff, cd.outBABytes),
                        BE(3, uni, cd.uniOff + kSlot, sizeof(Params))});
      if (!cd.bgMain || !cd.bgBA) return 6;
    }
  }

  auto runGroup = [&](int c0, int c1) -> int {
    wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = enc.BeginComputePass();
    if (c.direct && c0 == 0) {
      pass.SetPipeline(c.p_xpose);
      pass.SetBindGroup(0, bgXA);
      pass.DispatchWorkgroups((nApad / 4u * 32u + 63u) / 64u);
    }
    if (c.direct && (c.direct_tex || c.direct_texa)) {
                                                                                                                                
      for (int k = c0; k < c1; ++k) {
        const Cand& cd = cands[(size_t)k];
        pass.SetPipeline(c.p_xpose);
        pass.SetBindGroup(0, cd.bgX);
        pass.DispatchWorkgroups((cd.nBpad / 4u * 32u + 63u) / 64u);
      }
      pass.End();
      if (c.direct_texa && c0 == 0) EncodeBtToTexture(enc, atPb, c.pbAtTex, nApad / 4u);
      for (int k = c0; k < c1 && c.direct_tex; ++k) {
        const Cand& cd = cands[(size_t)k];
        wgpu::TexelCopyBufferInfo src{};
        src.buffer = btPb;
        src.layout.offset = cd.btOff;
        src.layout.bytesPerRow = cd.nBpad * 4u;
        src.layout.rowsPerImage = 128u;
        wgpu::TexelCopyTextureInfo dst{};
        dst.texture = c.pbTex[(size_t)k];
        wgpu::Extent3D ext{cd.nBpad / 4u, 128u, 1u};
        enc.CopyBufferToTexture(&src, &dst, &ext);
      }
      pass = enc.BeginComputePass();
    }
    for (int k = c0; k < c1; ++k) {
      const Cand& cd = cands[(size_t)k];
      if (mma) {
        if (c.direct && !(c.direct_tex || c.direct_texa)) {
          pass.SetPipeline(c.p_xpose);
          pass.SetBindGroup(0, cd.bgX);
          pass.DispatchWorkgroups((cd.nBpad / 4u * 32u + 63u) / 64u);
        }
        pass.SetPipeline(c.p_main);
        pass.SetBindGroup(0, cd.bgMain);
        pass.DispatchWorkgroups(numWg);
        pass.SetPipeline(c.p_merge);
        pass.SetBindGroup(0, cd.bgMerge);
        pass.DispatchWorkgroups((cd.nB + 63) / 64);
      } else {
        pass.SetPipeline(c.p_main);
        pass.SetBindGroup(0, cd.bgMain);
        pass.DispatchWorkgroups((nAu + 63) / 64);
        pass.SetBindGroup(0, cd.bgBA);
        pass.DispatchWorkgroups((cd.nB + 63) / 64);
      }
    }
    pass.End();
    for (int k = c0; k < c1; ++k) {
      const Cand& cd = cands[(size_t)k];
      enc.CopyBufferToBuffer(outAB, (uint64_t)k * outABSlot, staging, cd.stgOff,
                             outABBytes);
      enc.CopyBufferToBuffer(outBA, cd.outBAOff, staging, cd.stgOff + outABBytes,
                             cd.outBABytes);
    }
    wgpu::CommandBuffer cb = enc.Finish();
    double gpuMs = 0.0;
    const int rc = SubmitAndWait(c, cb, &gpuMs);
    if (rc != 0) return rc;
    if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
    ++aether_match_chunks;
                                                                             
                                                                          
    const double gapPct = ThermalGapPct();
    if (gapPct > 0.0 && gpuMs > 0.0) {
      double gapMs = gpuMs * gapPct / 100.0;
      if (gapMs > 250.0) gapMs = 250.0;
      aether_match_sleep_ms += gapMs;
      std::this_thread::sleep_for(
          std::chrono::microseconds((long long)(gapMs * 1000.0)));
    }
    return 0;
  };
  const double chunkTargetMs = ChunkTargetMs();
  double estCap = 0.0;
  if (chunkTargetMs > 0.0) {
    const double target = ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
    estCap = c.ema_main > 0.0 ? target / c.ema_main : 0.0;
  }
  int g0 = 0;
  double gUnits = 0.0;
  for (int k = 0; k < n_cands; ++k) {
    const double cu = cands[(size_t)k].units;
    if (k > g0 && estCap > 0.0 && gUnits + cu > estCap) {
      const int rc = runGroup(g0, k);
      if (rc != 0) return rc;
      g0 = k;
      gUnits = 0.0;
    }
    gUnits += cu;
  }
  if (g0 < n_cands) {
    const int rc = runGroup(g0, n_cands);
    if (rc != 0) return rc;
  }
  c.host.resize((size_t)(stgTotal / 4));
  const int rrc = ReadbackStaging(c, staging, stgTotal, c.host.data());
  if (rrc != 0) return rrc;
  for (int k = 0; k < n_cands; ++k) {
    const Cand& cd = cands[(size_t)k];
    const int32_t* mAB = c.host.data() + cd.stgOff / 4;
    const int32_t* mBA = mAB + nAu;
    int n_out = 0;
    const int rc = CrossCheck(mAB, nA, mBA, (int)cd.nB, nullptr, 0, &n_out);
    if (rc != 0) return rc;
    out_counts[k] = n_out;
  }
  return 0;
}

}              

                                                                                                                                                                                              

extern "C" int pwdawn_gpu_match_last_error(char* buf, int cap) {
  if (!buf || cap <= 0) return 0;
  std::lock_guard<std::mutex> lk(gLastErrLock);
  const int n = std::snprintf(buf, (size_t)cap, "%s", gLastErr);
  return n < 0 ? 0 : (n < cap ? n : cap - 1);
}

extern "C" void pwdawn_match_set_ab_phase(int phase) {
  gAbPhase.store(phase, std::memory_order_relaxed);
}
extern "C" void pwdawn_gpu_match_set_capture_active(int active) {
  gCaptureActive.store(active ? 1 : 0, std::memory_order_relaxed);
}
extern "C" void pwdawn_gpu_match_set_preview_fps30(int on) {
  gPreviewFps30.store(on ? 1 : 0, std::memory_order_relaxed);
}
extern "C" void pwdawn_gpu_match_set_thermal_state(int state) {
  gThermalState.store((state >= 0 && state <= 3) ? state : 0,
                      std::memory_order_relaxed);
}
extern "C" int pwdawn_gpu_match_get_capture_active(void) {
  return gCaptureActive.load(std::memory_order_relaxed);
}

#if !(defined(PWOFFICIAL_DAWN_OBSERVABLES_EXTERN) && PWOFFICIAL_DAWN_OBSERVABLES_EXTERN)
                                                                             
                                          
extern "C" __attribute__((used, visibility("default"))) int
aether_gpu_match_get_capture_active(void) {
  return gCaptureActive.load(std::memory_order_relaxed);
}
#endif

                                                                            
                                                                             
extern "C" int pwdawn_gpu_match_backend_info(char* buf, int cap) {
  if (!buf || cap <= 0) return 0;
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  Ctx* c = EnsureDawn();
  if (!c) {
    buf[0] = 0;
    return 0;
  }
  const int n = std::snprintf(buf, (size_t)cap, "%s", c->backend_info.c_str());
  return n < 0 ? 0 : (n < cap ? n : cap - 1);
}

extern "C" int pwdawn_gpu_match_gemm_pairs(const uint8_t* dA, int nA,
                                           const uint8_t* dB, int nB,
                                           double max_ratio,
                                           uint32_t* out_pairs, int max_pairs,
                                           int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  return MatchPairsImpl(dA, nA, dB, nB, max_ratio, out_pairs, max_pairs,
                        out_num_matches);
}

extern "C" int pwdawn_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame_a, uint32_t generation_a,
    const uint8_t* dA, int nA, uint32_t frame_b, uint32_t generation_b,
    const uint8_t* dB, int nB, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  return MatchPairsImpl(dA, nA, dB, nB, max_ratio, out_pairs, max_pairs,
                        out_num_matches, session_nonce, frame_a, generation_a,
                        frame_b, generation_b);
}

extern "C" void pwdawn_gpu_match_descriptor_residency_invalidate(
    uint64_t session_nonce, uint32_t frame_ordinal) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  auto found = gResidency.find(session_nonce);
  if (found == gResidency.end()) return;
  EraseResident(found->second.get(),
                found->second->policy.InvalidateFrame(session_nonce, frame_ordinal));
}

extern "C" void pwdawn_gpu_match_descriptor_residency_clear_session(
    uint64_t session_nonce) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  auto found = gResidency.find(session_nonce);
  if (found == gResidency.end()) return;
  found->second->policy.ClearSession(session_nonce);
  for (auto& kv : found->second->buffers) {
    if (kv.second) kv.second.Destroy();
  }
  found->second->buffers.clear();
  gResidency.erase(found);
}

extern "C" int pwdawn_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements, uint64_t* upload_bytes,
    uint64_t* resident_bytes, uint64_t* resident_entries,
    uint64_t* allocation_failures, uint64_t* device_resets) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  uint64_t values[9] = {0};
  auto found = gResidency.find(session_nonce);
  if (found != gResidency.end()) {
    const auto stats = found->second->policy.stats();
    values[0] = stats.hits;
    values[1] = stats.misses;
    values[2] = stats.evictions;
    values[3] = stats.stale_replacements;
    values[4] = found->second->upload_bytes;
    values[5] = found->second->policy.resident_bytes();
    values[6] = found->second->policy.size();
    values[7] = found->second->allocation_failures;
    values[8] = found->second->device_resets;
  }
  uint64_t* outputs[9] = {hits,           misses,           evictions,
                          stale_replacements, upload_bytes, resident_bytes,
                          resident_entries, allocation_failures, device_resets};
  for (size_t i = 0; i < 9; ++i) {
    if (outputs[i]) *outputs[i] = values[i];
  }
  return found == gResidency.end() ? 0 : 1;
}

extern "C" int pwdawn_gpu_match_probe_batch(const uint8_t* dA, int nA,
                                            const uint8_t* const* dBs,
                                            const int* nBs, int n_cands,
                                            double max_ratio, int* out_counts) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  return ProbeBatchImpl(dA, nA, dBs, nBs, n_cands, max_ratio, out_counts);
}

extern "C" int pwdawn_gpu_match_gemm_pairs_guided(
    const uint8_t* dA, int nA, const float* xyA, const uint8_t* dB, int nB,
    const float* xyB, double max_ratio, const float* matrixAB,
    const float* matrixBA, int guide_mode, float max_residual,
    uint32_t* out_pairs, int max_pairs, int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  if (guide_mode < 0) return 1;
  return MatchGuidedImpl(dA, nA, xyA, dB, nB, xyB, max_ratio, matrixAB,
                         matrixBA, (uint32_t)guide_mode, max_residual,
                         out_pairs, max_pairs, out_num_matches);
}

#if defined(PWOFFICIAL_DAWN_HOST_TEST) && PWOFFICIAL_DAWN_HOST_TEST
                                                                          
                                                                           
                                             
extern "C" void pwdawn_gpu_match_debug_last_dirmaps(const int32_t** ab, int* na,
                                                    const int32_t** ba, int* nb) {
  if (ab) *ab = gDbgAB.data();
  if (na) *na = (int)gDbgAB.size();
  if (ba) *ba = gDbgBA.data();
  if (nb) *nb = (int)gDbgBA.size();
}
extern "C" void pwdawn_gpu_match_debug_last_timing(double* upload_ms,
                                                   double* submit_ms,
                                                   double* readback_ms) {
  if (upload_ms) *upload_ms = gDbgUploadMs;
  if (submit_ms) *submit_ms = gDbgSubmitMs;
  if (readback_ms) *readback_ms = gDbgReadbackMs;
}
#endif

                                                                                                                         
                                                                                                                                     
                                                 
extern "C" int pwdawn_probe_alu_shape(const char* shape, uint32_t workgroups, uint32_t iters, double* out_ms, double* out_gfma);
extern "C" int pwdawn_probe_alu_peak(uint32_t workgroups, uint32_t iters, double* out_ms, double* out_gfma) {
  return pwdawn_probe_alu_shape("chain16", workgroups, iters, out_ms, out_gfma);
}
                                                                                                                        
                                                                                          
                                                                                                         
                                                                                                            
                                                               
                                                                          
extern "C" int pwdawn_probe_alu_shape(const char* shape, uint32_t workgroups, uint32_t iters, double* out_ms, double* out_gfma) {
  Ctx* cp = EnsureDawn();
  if (!cp) return 2;
  Ctx& c = *cp;
  const std::string sh = shape ? shape : "chain16";
  std::string body; double fmaPerIter = 32.0;
  if (sh == "gemm44" || sh == "gemm44ld") {
    body = sh == "gemm44"
      ? "    let fk = f32(i);\n    let al = vec4<f32>(fk, fk + 1.0, fk + 2.0, fk + 3.0) * 0.001;\n    let b4 = vec4<f32>(fk * 0.5, fk, fk * 1.5, fk * 2.0) * 0.001;\n"
      : "    let al = Src[(i & 63u) * 2u + (lid & 1u)];\n    let b4 = Src[128u + (i & 63u) * 4u + (lid & 3u)];\n";
    body += "    a0 = a0 + al.x * b4;\n    a1 = a1 + al.y * b4;\n    a2 = a2 + al.z * b4;\n    a3 = a3 + al.w * b4;\n";
    fmaPerIter = 16.0;
  } else if (sh == "gemm84") {
    body = "    let fk = f32(i);\n    let al = vec4<f32>(fk, fk + 1.0, fk + 2.0, fk + 3.0) * 0.001;\n    let ah = al + 4.0;\n    let b4 = vec4<f32>(fk * 0.5, fk, fk * 1.5, fk * 2.0) * 0.001;\n"
           "    a0 = a0 + al.x * b4;\n    a1 = a1 + al.y * b4;\n    a2 = a2 + al.z * b4;\n    a3 = a3 + al.w * b4;\n"
           "    a4 = a4 + ah.x * b4;\n    a5 = a5 + ah.y * b4;\n    a6 = a6 + ah.z * b4;\n    a7 = a7 + ah.w * b4;\n";
    fmaPerIter = 32.0;
  } else {
    body = "    a0 = fma(a0, b, cc); a1 = fma(a1, b, cc); a2 = fma(a2, b, cc); a3 = fma(a3, b, cc);\n"
           "    a0 = fma(a0, b, cc); a1 = fma(a1, b, cc); a2 = fma(a2, b, cc); a3 = fma(a3, b, cc);\n";
    fmaPerIter = 32.0;
  }
  const bool eight = sh == "gemm84";
  std::string src =
    "struct P { iters : u32, p1 : u32, p2 : u32, p3 : u32, };\n"
    "@group(0) @binding(0) var<uniform> U : P;\n"
    "@group(0) @binding(1) var<storage, read_write> Out : array<f32>;\n"
    "@group(0) @binding(2) var<storage, read> Src : array<vec4<f32>>;\n"
    "@compute @workgroup_size(256)\n"
    "fn main(@builtin(global_invocation_id) gid : vec3<u32>, @builtin(local_invocation_index) lid : u32) {\n"
    "  var a0 = vec4<f32>(f32(lid) * 0.001, 1.0, 2.0, 3.0);\n  var a1 = a0 + 1.0;\n  var a2 = a0 + 2.0;\n  var a3 = a0 + 3.0;\n";
  if (eight) src += "  var a4 = a0 + 4.0;\n  var a5 = a0 + 5.0;\n  var a6 = a0 + 6.0;\n  var a7 = a0 + 7.0;\n";
  src += "  let b = vec4<f32>(0.999, 1.001, 0.998, 1.002);\n  let cc = vec4<f32>(0.0000001);\n"
         "  for (var i = 0u; i < U.iters; i = i + 1u) {\n" + body + "  }\n"
         "  var sum = a0 + a1 + a2 + a3;\n";
  if (eight) src += "  sum = sum + a4 + a5 + a6 + a7;\n";
  src += "  Out[gid.x] = sum.x + sum.y + sum.z + sum.w + Src[lid & 3u].x * 0.0;\n}\n";
  wgpu::ShaderModule m = CompileWgsl(c, src.c_str(), "alu ladder");
  if (!m) return 3;
  wgpu::ComputePipeline pipe = MakePipeline(c, m, "main");
  if (!pipe) return 3;
  wgpu::BufferDescriptor ud{}; ud.usage = kUsageUni; ud.size = 256;
  wgpu::Buffer uni = c.device.CreateBuffer(&ud);
  wgpu::BufferDescriptor od{}; od.usage = wgpu::BufferUsage::Storage; od.size = (uint64_t)workgroups * 256u * 4u;
  wgpu::Buffer outb = c.device.CreateBuffer(&od);
  wgpu::BufferDescriptor sd{}; sd.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst; sd.size = 512u * 16u;
  wgpu::Buffer srcb = c.device.CreateBuffer(&sd);
  if (!uni || !outb || !srcb) return 6;
  {
    std::vector<float> init(512 * 4);
    for (size_t k = 0; k < init.size(); ++k) init[k] = 0.001f * (float)(k % 97);
    c.queue.WriteBuffer(srcb, 0, reinterpret_cast<const uint8_t*>(init.data()), init.size() * 4);
  }
  const uint32_t pv[4] = {iters, 0u, 0u, 0u};
  c.queue.WriteBuffer(uni, 0, reinterpret_cast<const uint8_t*>(pv), 16);
  wgpu::BindGroup bg = MakeBG(c, pipe, {BE(0, uni, 0, 16), BE(1, outb, 0, od.size), BE(2, srcb, 0, sd.size)});
  if (!bg) return 6;
  double best = 1e30;
  for (int rep = 0; rep < 3; ++rep) {
    wgpu::CommandEncoder enc = c.device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = enc.BeginComputePass();
    pass.SetPipeline(pipe);
    pass.SetBindGroup(0, bg);
    pass.DispatchWorkgroups(workgroups);
    pass.End();
    wgpu::CommandBuffer cb = enc.Finish();
    const double t0 = NowMs();
    const int rc = SubmitAndWait(c, cb, nullptr);
    const double dt = NowMs() - t0;
    if (rc != 0) return rc;
    if (dt < best) best = dt;
  }
  const double fmas = (double)workgroups * 256.0 * (double)iters * fmaPerIter;
  if (out_ms) *out_ms = best;
  if (out_gfma) *out_gfma = fmas / (best * 1e-3) / 1e9;
  return 0;
}
