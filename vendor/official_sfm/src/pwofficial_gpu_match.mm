                                                                                
                                                                             
                                                                          
  
                                                                                                                
                                                                      
                                                                              
                                                                      
                                 
                                                                            
                                                                            
                                                                          
                                                                      
                                                                             
                                                                           
                                                                            
                                                                      
                                                                  
                                                                            
                                                                        
                                                                             
                                                                       
                                                                          
                                                                           
                
                                                                           
                                                                        
                                                                            
                                                                           
                                                                             
                                                                             
                                                                          
                      
                                                                           
                                                                              
                                                                       
                                                                      
                                                                         
                                                                     
                                                                                
                                                                            
                                                                         
                                                                     
                                                                      
                                                                            
                                                                            
                                                                          
                                                                             
                                                                         
                                                                            
                                                                       
                                                                           
                                                
                                                                
                                                                            
                                                                           
                                                                          
                                                                          
                                                                         
                                                                         
                                                                            
                                                                           
                                                                          
                                                                     
                                                                       
                                                                         
                                                                    
                                                      
  
                                                                                                          
// Adapted from the research harness's GpuMatch.m (iosapp/Sources,
                                                                              
                                                  
  
                                                                           
                                                                               
                                                                             
                                                                          
                                                                            
                                                                      
                                                                       
                                                                           
                                                                          
  
                                                                            
                                                                        
                                                                 

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <atomic>
#include <chrono>                                                          
#include <condition_variable>                                                   
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unistd.h>
#include <vector>                                                         

#if __has_include("aether/sfm/descriptor_residency_policy_v1.h")
#include "aether/sfm/descriptor_residency_policy_v1.h"
#else
#include "../include/aether/sfm/descriptor_residency_policy_v1.h"
#endif

static void ClearAllDescriptorResidencyForDeviceError(void);

                                                                                                          
                                                        
                                                                          
                                                                     
                                                                     
                                                                         
                                                                            
                                                            
static std::mutex gLastErrLock;
static char gLastErr[192] = {0};

static void stashLastError(NSError* error) {
  NSString* text =
      error ? [NSString stringWithFormat:@"%@ code=%ld %@", error.domain,
                                         (long)error.code,
                                         error.localizedDescription ?: @""]
            : @"(nil error)";
  std::lock_guard<std::mutex> lk(gLastErrLock);
  strlcpy(gLastErr, text.UTF8String ?: "(utf8 failed)", sizeof(gLastErr));
}

                                                                          
                                                                              
extern "C" int aether_gpu_match_last_error(char* buf, int cap) {
  if (!buf || cap <= 0) return 0;
  std::lock_guard<std::mutex> lk(gLastErrLock);
  const size_t n = strlcpy(buf, gLastErr, (size_t)cap);
  return (int)(n < (size_t)cap ? n : (size_t)cap - 1);
}

                                                                  
                                                                  
                                                                        
                                                                        
                                                                        
                                                                    
                                    
                                                                       
                                                                          
                                                                        
                                                                      
static void NoteCmdError(id<MTLCommandBuffer> cmd) {
  static std::atomic<long> gCmdErrCount{0};
  const long k = ++gCmdErrCount;
  if (k <= 5 || (k % 100) == 0) {
    NSLog(@"[pwofficial_gpu_match] command buffer error #%ld (rc=7): %@", k,
          cmd.error);
  }
  stashLastError(cmd.error);
  ClearAllDescriptorResidencyForDeviceError();
}

                                                                                                                      
                                                                                
                                                                                            
                                                                                                   
                                                                                                 
                                                                                     
                                                                                         
                                                                                              
                                                                            
static uint64_t CmdWaitTimeoutMs(void) {
                                                                                          
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

static int WaitCmdWithTimeout(id<MTLCommandBuffer> cmd, uint64_t timeout_ms) {
                                                                                            
  struct WaitState {
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
  };
  auto state = std::make_shared<WaitState>();
                                                                                
  [cmd addCompletedHandler:^(id<MTLCommandBuffer>) {
    std::lock_guard<std::mutex> lk(state->mu);
    state->done = true;
    state->cv.notify_all();
  }];
  [cmd commit];
  {
    std::unique_lock<std::mutex> lk(state->mu);
    if (!state->cv.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                            [&] { return state->done; })) {
                                                                                      
                                                                                            
      static std::atomic<long> gCmdTimeoutCount{0};
      const long k = ++gCmdTimeoutCount;
      if (k <= 5 || (k % 100) == 0) {
        NSLog(@"[pwofficial_gpu_match] command buffer wait TIMEOUT #%ld after "
              @"%llu ms (rc=7): status=%ld",
              k, (unsigned long long)timeout_ms, (long)cmd.status);
      }
      stashLastError([NSError
          errorWithDomain:MTLCommandBufferErrorDomain
                     code:MTLCommandBufferErrorTimeout
                 userInfo:@{
                   NSLocalizedDescriptionKey : [NSString
                       stringWithFormat:@"host-side wait timeout after %llu ms",
                                        (unsigned long long)timeout_ms]
                 }]);
      ClearAllDescriptorResidencyForDeviceError();
      return 7;
    }
  }
                                                                                    
  if (cmd.status == MTLCommandBufferStatusError || cmd.error != nil) {
    NSError* err = cmd.error;
    NoteCmdError(cmd);                                          
    if (err != nil &&
        [err.domain isEqualToString:MTLCommandBufferErrorDomain]) {
      switch (err.code) {
        case MTLCommandBufferErrorAccessRevoked:
                                                                                
                                                                                 
          return 8;
        case MTLCommandBufferErrorNotPermitted:
                                                                                              
                                                                                              
          return 8;
        case MTLCommandBufferErrorTimeout:
                                                              
          return 7;
        default:
          break;
      }
    }
    return 7;                                                             
  }
  return 0;
}

                                                                                                
                                                                                
                                                                             
                                                                         
                                                                        
                                                              
                                                    
                                                                          
                                                                             
                                                                        
                                                                            
                                                                           
                                                                       
static const char* kGemmKernelSrc = R"MSL(
#include <metal_stdlib>
#include <metal_simdgroup_matrix>
using namespace metal;

constant uint kD  = 128u;
constant uint kKT = 16u;
constant uint kSG = 16u;
constant uint kMB = kSG * 8u;
constant uint kBN = 16u;
constant uint kNT = kBN / 8u;
constant uint kTGT = kSG * 32u;
constant float kInvSqNorm = 1.0f / 262144.0f;  // 1 / kSqSiftDescriptorNorm (512^2)

kernel void pw_match_gemm(device const half*  A          [[buffer(0)]],
                          device const half*  B          [[buffer(1)]],
                          device int*         out         [[buffer(2)]],
                          constant uint&      numA        [[buffer(3)]],
                          constant uint&      numB        [[buffer(4)]],
                          constant float&     maxRatio    [[buffer(5)]],
                          constant float&     maxDistance [[buffer(6)]],
                          device const float2* pointsA     [[buffer(7)]],
                          device const float2* pointsB     [[buffer(8)]],
                          device const float*  guideMatrix [[buffer(9)]],
                          constant uint&       guideMode   [[buffer(10)]],
                          constant float&      maxResidual [[buffer(11)]],
                          constant uint&       rowBase     [[buffer(12)]],
                          threadgroup half*   Bsh         [[threadgroup(0)]],
                          threadgroup float*  acc         [[threadgroup(1)]],
                          uint tgid  [[threadgroup_position_in_grid]],
                          uint lid   [[thread_index_in_threadgroup]],
                          uint sgid  [[simdgroup_index_in_threadgroup]]) {
  // rowBase: first row-block of this chunk. Chunked dispatch splits one
  // logical (numA x numB) pass into several small command buffers so a
  // single dispatch never occupies the GPU long enough to starve the
  // ARKit camera/render pipeline (rc=7 pathology). Row blocks are
  // independent — per-row results are identical for any chunking.
  const uint row0 = (rowBase + tgid) * kMB;
  if (row0 >= numA) { return; }
  const uint rows = min(kMB, numA - row0);

  const uint aRow0 = row0 + sgid * 8u;
  simdgroup_matrix<half, 8, 8> aFrag[kKT];
  for (uint k = 0; k < kKT; ++k) {
    simdgroup_load(aFrag[k], A + aRow0 * kD + k * 8u, kD, ulong2(0, 0));
  }

  // Track the two LARGEST dot products per row (COLMAP: best = max dot,
  // init 0 so only positive dots become candidates; index -1 if none).
  threadgroup float bestT[kMB];
  threadgroup float secondT[kMB];
  threadgroup int   biT[kMB];
  for (uint r = lid; r < kMB; r += kTGT) { bestT[r] = 0.0f; secondT[r] = 0.0f; biT[r] = -1; }
  threadgroup_barrier(mem_flags::mem_threadgroup);

  for (uint col0 = 0; col0 < numB; col0 += kBN) {
    const uint cols = min(kBN, numB - col0);

    for (uint e = lid; e < kBN * kD; e += kTGT) {
      uint r = e / kD, d = e % kD;
      Bsh[e] = (r < cols) ? B[(col0 + r) * kD + d] : half(0);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint nt = 0; nt < kNT; ++nt) {
      simdgroup_matrix<float, 8, 8> c = make_filled_simdgroup_matrix<float, 8, 8>(0.0f);
      for (uint k = 0; k < kKT; ++k) {
        simdgroup_matrix<half, 8, 8> bF;
        simdgroup_load(bF, Bsh + (nt * 8u) * kD + k * 8u, kD, ulong2(0, 0),
                       true);
        simdgroup_multiply_accumulate(c, aFrag[k], bF, c);
      }
      simdgroup_store(c, acc + (sgid * 8u) * kBN + nt * 8u, kBN, ulong2(0, 0));
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    {
      const uint row = lid / 4u;
      const uint cg  = lid % 4u;
      const uint per = kBN / 4u;
      float pb = 0.0f, ps = 0.0f; int pbi = -1;  // best/second DOT (max)
      if (row < rows) {
        for (uint t = 0; t < per; ++t) {
          const uint c = cg * per + t;
          if (col0 + c >= numB) break;
          bool geometryOK = true;
          if (guideMode == 1u) {
            const float2 q = pointsA[row0 + row];
            const float2 d = pointsB[col0 + c];
            const float3 p1 = float3(q, 1.0f);
            const float3 p2 = float3(d, 1.0f);
            const float3 line2 = float3(
                guideMatrix[0] * p1.x + guideMatrix[1] * p1.y + guideMatrix[2],
                guideMatrix[3] * p1.x + guideMatrix[4] * p1.y + guideMatrix[5],
                guideMatrix[6] * p1.x + guideMatrix[7] * p1.y + guideMatrix[8]);
            const float3 line1 = float3(
                guideMatrix[0] * p2.x + guideMatrix[3] * p2.y + guideMatrix[6],
                guideMatrix[1] * p2.x + guideMatrix[4] * p2.y + guideMatrix[7],
                guideMatrix[2] * p2.x + guideMatrix[5] * p2.y + guideMatrix[8]);
            const float nom = dot(p2, line2);
            const float denom = dot(line2.xy, line2.xy) +
                                dot(line1.xy, line1.xy);
            geometryOK = denom > 1e-12f &&
                         nom * nom <= maxResidual * denom;
          } else if (guideMode == 2u) {
            const float2 q = pointsA[row0 + row];
            const float2 d = pointsB[col0 + c];
            const float hx = guideMatrix[0] * q.x + guideMatrix[1] * q.y +
                             guideMatrix[2];
            const float hy = guideMatrix[3] * q.x + guideMatrix[4] * q.y +
                             guideMatrix[5];
            const float hz = guideMatrix[6] * q.x + guideMatrix[7] * q.y +
                             guideMatrix[8];
            if (abs(hz) <= 1e-8f) {
              geometryOK = false;
            } else {
              const float2 delta = float2(hx / hz, hy / hz) - d;
              geometryOK = dot(delta, delta) <= maxResidual;
            }
          }
          if (!geometryOK) continue;
          const float dot = acc[row * kBN + c];
          if (dot > pb) { ps = pb; pb = dot; pbi = (int)(col0 + c); }
          else if (dot > ps) { ps = dot; }
        }
      }
      for (ushort off = 1; off <= 2; off <<= 1) {
        const float ob = simd_shuffle_xor(pb, off);
        const float os = simd_shuffle_xor(ps, off);
        const int   oi = simd_shuffle_xor(pbi, off);
        if (ob > pb) { ps = max(os, pb); pb = ob; pbi = oi; }
        else         { ps = max(ps, ob); }
      }
      if (cg == 0u && row < rows) {
        float bb = bestT[row], ss = secondT[row]; int bi = biT[row];
        if (pb > bb) { ss = max(bb, ps); bb = pb; bi = pbi; }
        else         { ss = max(ss, pb); }
        bestT[row] = bb; secondT[row] = ss; biT[row] = bi;
      }
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
  }

  if (lid < kMB && lid < rows) {
    const int bi = biT[lid];
    if (bi < 0) { out[row0 + lid] = -1; return; }
    // COLMAP FindBestMatchesOneWayBruteForce (sift.cc:801-816):
    //   best_dist = acos(min(best_dot/512^2, 1));  reject if best_dist > max_distance
    //   second_dist = acos(min(second_dot/512^2, 1));
    //   reject if best_dist >= max_ratio * second_dist  (>= keeps best==second out)
    float bd;
    float sd;
    if (guideMode == 0u) {
      bd = acos(min(bestT[lid] * kInvSqNorm, 1.0f));
      sd = acos(min(secondT[lid] * kInvSqNorm, 1.0f));
    } else {
      // COLMAP's guided CPU path applies the thresholds in normalized L2.
      // With fixed-norm 512 SIFT descriptors, L2^2 / 512^2 = 2 - 2*cos.
      // A filtered candidate has COLMAP's sentinel distance 512, so it also
      // serves as the second-best baseline when only one candidate lies in the
      // geometry band.
      const float secondDot = max(secondT[lid], 131072.0f);
      bd = sqrt(max(0.0f, 2.0f - 2.0f * bestT[lid] * kInvSqNorm));
      sd = sqrt(max(0.0f, 2.0f - 2.0f * secondDot * kInvSqNorm));
    }
    const bool keep = (bd <= maxDistance) && (bd < maxRatio * sd);
    out[row0 + lid] = keep ? bi : -1;
  }
}
)MSL";

                                                                                      
                                                                      
                                                                        
                                                                            
                                                                          
                                                                     
                                                                      
                                                                        
                                                                          
                                                                          
                                                                          
                                                                          
                                                                 
                                                                                                  
                                                                                                 
  
                                                                                                   
                                                                                                           
                                                                                                           
  
                                                                                             
                                                                                             
                                                                                                    
static std::atomic<int> gAbPhase{-1};                                          
extern "C" void aether_match_set_ab_phase(int phase) {
  gAbPhase.store(phase, std::memory_order_relaxed);
}
                                                                        
static double AbAltValue(const char* key) {
  const char* e = getenv(key);
  return e ? atof(e) : -1.0;
}


                                                                                       
                         
                                                                                              
                                                                   
                                                                                            
                                                                        
                                                                                                    
                                                                                   
                                                                                 
                                                                                                      
                                                                                    
                                                        
                                                                 
                                                                                      
static std::atomic<int> gPreviewFps30{0};
static double ChunkTargetMs(void) {
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
                                                                      
                                                                             
                                                                            
                                                            
static double ChunkTargetCoolMs(void) {
  static double v = -1.0;
  if (v < 0.0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_CHUNK_TARGET_COOL_MS");
    v = e ? atof(e) : 16.0;
    if (v < 0.0) v = 0.0;
  }
  const double hot = ChunkTargetMs();
  return v > hot ? v : hot;
}
                                                                                                                  
                                                                        
                                                                            
                                                                         
                                                                          
                                                                          
                                                                            
                                                                           
                                                                          
                                                                            
                             
static std::atomic<int> gCaptureActive{1};
extern "C" void aether_gpu_match_set_capture_active(int active) {
  gCaptureActive.store(active ? 1 : 0, std::memory_order_relaxed);
}
                                                                                                                   
                                                                                                 
                                                                                                  
                                                                                                 
                                                                                        
                                                                                       
                                                                            
                                                                                       
extern "C" void aether_gpu_match_set_preview_fps30(int on) {
  gPreviewFps30.store(on ? 1 : 0, std::memory_order_relaxed);
}
                                                                      
                                                                            
                                                                             
                                                                          
                                                                            
                                        
                                                                        
                                                                          
                                                              
extern "C" __attribute__((used, visibility("default"))) int
aether_gpu_match_get_capture_active(void) {
  return gCaptureActive.load(std::memory_order_relaxed);
}
static bool ThermalHot(void) {
  if (gCaptureActive.load(std::memory_order_relaxed) == 0) return false;
  if (@available(iOS 11.0, macOS 10.10.3, *)) {
    const NSProcessInfoThermalState st =
        NSProcessInfo.processInfo.thermalState;
    return st == NSProcessInfoThermalStateSerious ||
           st == NSProcessInfoThermalStateCritical;
  }
  return false;
}
                                                                   
                                                                         
                                                                           
                                                                                         
                                                                                          
                                                                                   
                                                                                    
                                                         
extern "C" double aether_match_gpu_ms = 0.0;
extern "C" double aether_match_sleep_ms = 0.0;
extern "C" int aether_match_chunks = 0;

static double ThermalGapPct(void) {
  if (gCaptureActive.load(std::memory_order_relaxed) == 0) return 0.0;
  if (@available(iOS 11.0, macOS 10.10.3, *)) {
    const NSProcessInfoThermalState st =
        NSProcessInfo.processInfo.thermalState;
    if (st == NSProcessInfoThermalStateSerious) {
      static double v = -1.0;
      if (v < 0.0) {
        const char* e = getenv("OFFICIAL_AETHER_MATCH_GAP_SERIOUS_PCT");
        v = e ? atof(e) : 100.0;
        if (v < 0.0) v = 0.0;
      }
                                                                             
      if (gPreviewFps30.load(std::memory_order_relaxed) == 1) {
        static double v30 = -1.0;
        if (v30 < 0.0) {
          const char* e30 =
              getenv("OFFICIAL_AETHER_MATCH_GAP_SERIOUS_PCT_FPS30");
          v30 = e30 ? atof(e30) : -1.0;
        }
        return v30 >= 0.0 ? v30 : v * 0.5;
      }
      return v;
    }
    if (st == NSProcessInfoThermalStateCritical) {
      static double v = -1.0;
      if (v < 0.0) {
        const char* e = getenv("OFFICIAL_AETHER_MATCH_GAP_CRITICAL_PCT");
        v = e ? atof(e) : 300.0;
        if (v < 0.0) v = 0.0;
      }
      return v;
    }
  }
  return 0.0;
}
                                                                            
                                                                       
                                                                      
                                                                    
static std::atomic<double> gMsPerTgKCol{0.0};
static std::atomic<double> gMsPerTgKColV2{0.0};

                                          
static id<MTLDevice> gDev;
static id<MTLCommandQueue> gQueue;
static id<MTLComputePipelineState> gGemm;

static BOOL ensureMetal(void) {
  if (gGemm) return YES;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    gDev = MTLCreateSystemDefaultDevice();
    if (!gDev) return;
    gQueue = [gDev newCommandQueue];
    NSError* e = nil;
    id<MTLLibrary> lib = [gDev
        newLibraryWithSource:[NSString stringWithUTF8String:kGemmKernelSrc]
                     options:nil
                       error:&e];
    if (!lib) {
      NSLog(@"[pwofficial_gpu_match] kernel compile failed: %@", e);
      return;
    }
    id<MTLFunction> f = [lib newFunctionWithName:@"pw_match_gemm"];
    if (f) gGemm = [gDev newComputePipelineStateWithFunction:f error:&e];
    if (!gGemm) NSLog(@"[pwofficial_gpu_match] pipeline failed: %@", e);
  });
  return gGemm != nil;
}

                                                                              
                                                                    
                                                                             
                                                                           
                                                
static int matchPairsImpl(const uint8_t* dA, int nA, const float* xyA,
                          const uint8_t* dB, int nB, const float* xyB,
                          double max_ratio, const float* matrixAB,
                          const float* matrixBA, uint32_t guideMode,
                          float maxResidual, uint32_t* out_pairs,
                          int max_pairs, int* out_num_matches) {
  @autoreleasepool {
    if (out_num_matches) *out_num_matches = 0;
    if (!dA || !dB || nA <= 0 || nB <= 0) return 1;
    if (out_pairs != nullptr && max_pairs <= 0) return 1;
    if (guideMode > 2u) return 1;
    if (guideMode != 0u &&
        (!xyA || !xyB || !matrixAB || !matrixBA || maxResidual <= 0.0f)) {
      return 1;
    }
    if (!ensureMetal()) return 2;
    const int D = 128;
                                                                         
                                                                  
    NSUInteger nApad = (((NSUInteger)nA + 127) / 128) * 128;
    NSUInteger nBpad = (((NSUInteger)nB + 127) / 128) * 128;
    id<MTLBuffer> aBuf = [gDev newBufferWithLength:nApad * D * sizeof(__fp16)
                                           options:MTLResourceStorageModeShared];
    id<MTLBuffer> bBuf = [gDev newBufferWithLength:nBpad * D * sizeof(__fp16)
                                           options:MTLResourceStorageModeShared];
    if (!aBuf || !bBuf) return 5;
    __fp16* af = (__fp16*)aBuf.contents;
    for (NSUInteger i = 0; i < (NSUInteger)nA * D; ++i) af[i] = (__fp16)dA[i];
    for (NSUInteger i = (NSUInteger)nA * D; i < nApad * D; ++i) af[i] = 0;
    __fp16* bf = (__fp16*)bBuf.contents;
    for (NSUInteger i = 0; i < (NSUInteger)nB * D; ++i) bf[i] = (__fp16)dB[i];
    for (NSUInteger i = (NSUInteger)nB * D; i < nBpad * D; ++i) bf[i] = 0;
    id<MTLBuffer> outAB = [gDev newBufferWithLength:(NSUInteger)nA * sizeof(int)
                                            options:MTLResourceStorageModeShared];
    id<MTLBuffer> outBA = [gDev newBufferWithLength:(NSUInteger)nB * sizeof(int)
                                            options:MTLResourceStorageModeShared];
    if (!outAB || !outBA) return 6;
    static const float kDummyPoints[2] = {0.0f, 0.0f};
    static const float kDummyMatrix[9] = {1.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f,
                                          0.0f, 0.0f, 1.0f};
    const float* pointsA = guideMode == 0u ? kDummyPoints : xyA;
    const float* pointsB = guideMode == 0u ? kDummyPoints : xyB;
    const float* matrixAtoB = guideMode == 0u ? kDummyMatrix : matrixAB;
    const float* matrixBtoA = guideMode == 0u ? kDummyMatrix : matrixBA;
    const NSUInteger pointsALength =
        guideMode == 0u ? sizeof(kDummyPoints)
                        : (NSUInteger)nA * 2 * sizeof(float);
    const NSUInteger pointsBLength =
        guideMode == 0u ? sizeof(kDummyPoints)
                        : (NSUInteger)nB * 2 * sizeof(float);
    id<MTLBuffer> pointsABuf =
        [gDev newBufferWithBytes:pointsA
                          length:pointsALength
                         options:MTLResourceStorageModeShared];
    id<MTLBuffer> pointsBBuf =
        [gDev newBufferWithBytes:pointsB
                          length:pointsBLength
                         options:MTLResourceStorageModeShared];
    id<MTLBuffer> matrixABBuf =
        [gDev newBufferWithBytes:matrixAtoB
                          length:9 * sizeof(float)
                         options:MTLResourceStorageModeShared];
    id<MTLBuffer> matrixBABuf =
        [gDev newBufferWithBytes:matrixBtoA
                          length:9 * sizeof(float)
                         options:MTLResourceStorageModeShared];
    if (!pointsABuf || !pointsBBuf || !matrixABBuf || !matrixBABuf) return 6;
                                                                            
                                                                               
                                                                   
                                                                          
    float maxRatio = (float)max_ratio;
    if (maxRatio <= 0.0f) maxRatio = 0.8f;
    float maxDistance = 0.7f;                                                      
    const NSUInteger bshLen = 16 * 128 * sizeof(__fp16);
    const NSUInteger accLen = 128 * 16 * sizeof(float);
                                                                               
    auto encChunk = [&](id<MTLCommandBuffer> cmd, id<MTLBuffer> Q,
                        id<MTLBuffer> Db, id<MTLBuffer> O, id<MTLBuffer> Qxy,
                        id<MTLBuffer> Dbxy, id<MTLBuffer> M, uint32_t nQ,
                        uint32_t nDb, uint32_t rowBase, NSUInteger groups) {
      id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
      [enc setComputePipelineState:gGemm];
      [enc setBuffer:Q offset:0 atIndex:0];
      [enc setBuffer:Db offset:0 atIndex:1];
      [enc setBuffer:O offset:0 atIndex:2];
      [enc setBytes:&nQ length:4 atIndex:3];
      [enc setBytes:&nDb length:4 atIndex:4];
      [enc setBytes:&maxRatio length:4 atIndex:5];
      [enc setBytes:&maxDistance length:4 atIndex:6];
      [enc setBuffer:Qxy offset:0 atIndex:7];
      [enc setBuffer:Dbxy offset:0 atIndex:8];
      [enc setBuffer:M offset:0 atIndex:9];
      [enc setBytes:&guideMode length:4 atIndex:10];
      [enc setBytes:&maxResidual length:4 atIndex:11];
      [enc setBytes:&rowBase length:4 atIndex:12];
      [enc setThreadgroupMemoryLength:bshLen atIndex:0];
      [enc setThreadgroupMemoryLength:accLen atIndex:1];
      [enc dispatchThreadgroups:MTLSizeMake(groups, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(512, 1, 1)];
      [enc endEncoding];
    };
    const double chunkTargetMs = ChunkTargetMs();
    if (chunkTargetMs <= 0.0) {
                                                                          
                                                             
      id<MTLCommandBuffer> cmd = [gQueue commandBuffer];
      encChunk(cmd, aBuf, bBuf, outAB, pointsABuf, pointsBBuf, matrixABBuf,
               (uint32_t)nA, (uint32_t)nB, 0u,
               ((NSUInteger)nA + 127) / 128);          
      encChunk(cmd, bBuf, aBuf, outBA, pointsBBuf, pointsABuf, matrixBABuf,
               (uint32_t)nB, (uint32_t)nA, 0u,
               ((NSUInteger)nB + 127) / 128);          
                                                                                      
                                                                                        
      const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
      if (wrc != 0) return wrc;
    } else {
                                                                          
                                                                           
                                                                            
                                                                     
      auto runDirection = [&](id<MTLBuffer> Q, id<MTLBuffer> Db,
                              id<MTLBuffer> O, id<MTLBuffer> Qxy,
                              id<MTLBuffer> Dbxy, id<MTLBuffer> M, uint32_t nQ,
                              uint32_t nDb) -> int {
        const NSUInteger totalGroups = ((NSUInteger)nQ + 127) / 128;
        NSUInteger tg0 = 0;
        while (tg0 < totalGroups) {
                                                                    
                                                         
          const double target =
              ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
          const double unit = gMsPerTgKCol.load();
          NSUInteger want = 8;                                          
          if (unit > 0.0) {
            const double perTg = unit * ((double)nDb / 1024.0);
            const double ideal = target / (perTg > 1e-6 ? perTg : 1e-6);
            want = ideal < 1.0 ? 1 : (NSUInteger)ideal;
          }
          const NSUInteger groups =
              want < totalGroups - tg0 ? want : totalGroups - tg0;
          id<MTLCommandBuffer> cmd = [gQueue commandBuffer];
          encChunk(cmd, Q, Db, O, Qxy, Dbxy, M, nQ, nDb, (uint32_t)tg0,
                   groups);
                                                                                  
          const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
          if (wrc != 0) return wrc;
          const double gpuMs = (cmd.GPUEndTime - cmd.GPUStartTime) * 1000.0;
          if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
          ++aether_match_chunks;
          if (gpuMs > 0.0 && gpuMs < 10000.0) {
            const double u = gpuMs / ((double)groups * ((double)nDb / 1024.0));
            const double prev = gMsPerTgKCol.load();
            gMsPerTgKCol.store(prev <= 0.0 ? u : prev * 0.7 + u * 0.3);
          }
          const double gapPct = ThermalGapPct();
          if (gapPct > 0.0 && gpuMs > 0.0) {
                                                                    
                                                               
            double gapMs = gpuMs * gapPct / 100.0;
            if (gapMs > 250.0) gapMs = 250.0;
            aether_match_sleep_ms += gapMs;
            usleep((useconds_t)(gapMs * 1000.0));
          }
          tg0 += groups;
        }
        return 0;
      };
      int rc = runDirection(aBuf, bBuf, outAB, pointsABuf, pointsBBuf,
                            matrixABBuf, (uint32_t)nA, (uint32_t)nB);          
      if (rc == 0) {
        rc = runDirection(bBuf, aBuf, outBA, pointsBBuf, pointsABuf,
                          matrixBABuf, (uint32_t)nB, (uint32_t)nA);          
      }
      if (rc != 0) return rc;
    }

                                          
    const int* mAB = (const int*)outAB.contents;
    const int* mBA = (const int*)outBA.contents;
    int n_out = 0;
    for (int i = 0; i < nA; ++i) {
      int j = mAB[i];
      if (j >= 0 && j < nB && mBA[j] == i) {
        if (out_pairs != nullptr) {
          if (n_out >= max_pairs) break;                             
          out_pairs[2 * n_out] = (uint32_t)i;
          out_pairs[2 * n_out + 1] = (uint32_t)j;
        }
        ++n_out;
      }
    }
    if (out_num_matches) *out_num_matches = n_out;
    return 0;
  }
}

                                                                                                                                                                       

                                                                          
                                                                    
static bool MatchV2Enabled(void) {
  static int v = -1;
  if (v < 0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_V2");
    v = (e && atoi(e) == 0) ? 0 : 1;               
  }
  return v != 0;
}
static bool MatchV2ForceHalf(void) {
  static int v = -1;
  if (v < 0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_V2_HALF");
    v = (e && atoi(e) != 0) ? 1 : 0;                          
  }
  return v != 0;
}
                                                                             
                                                                             
                                                                            
static bool MatchV2GuidedFused(void) {
  static int v = -1;
  if (v < 0) {
    const char* e = getenv("OFFICIAL_AETHER_MATCH_V2_GUIDED_FUSED");
    v = (e && atoi(e) != 0) ? 1 : 0;                
  }
  return v != 0;
}

                                                                                                                         
                                                                       
                                                                      
                                                                             
                                                                     
                                                                           
                                                                                 
                                                                         
                                                                            
             
static const char* kGemm2KernelSrc = R"MSL(
#include <metal_stdlib>
#include <metal_simdgroup_matrix>
using namespace metal;

constant bool kPacked    [[function_constant(0)]];
constant uint kGuideMode [[function_constant(1)]];

constant uint kD  = 128u;         // descriptor dim
constant uint kKT = 16u;          // K tiles of 8
constant uint kSG = 16u;          // simdgroups per threadgroup
constant uint kMB = kSG * 8u;     // 128 query rows per threadgroup
constant uint kBN = 32u;          // database columns per tile (v1: 16)
constant uint kNT = kBN / 8u;     // 8x8 col blocks per tile
constant uint kTGT = kSG * 32u;   // 512 threads
constant float kInvSqNorm = 1.0f / 262144.0f;  // 1/512^2 (exact power of two)

// v1's geometry gate (E/F Sampson-style band, H transfer error), open-coded
// verbatim at both tracker sites below. PARITY-CRITICAL: `nom` suffers
// near-cancellation for candidates close to the epipolar line, so single-ulp
// differences in intermediate rounding (fma contraction choices) amplify to
// ~1e-3 relative residual shifts and flip borderline gates. The gate is
// therefore kept TEXTUALLY identical to v1 — same expression trees, same
// device address spaces for points and matrix, loads inside the candidate
// loop — so the Metal compiler makes the same contraction choices it makes
// for v1 (host-verified: guided fixtures cmp byte-identical). Do NOT
// refactor into a helper taking values or hoist the loads.
// PW_GUIDE_OK expands v1's gate body: sets `geometryOK` from (Q, DPT, M).
#define PW_GUIDE_OK(Q, DPT, M)                                            \
  bool geometryOK = true;                                                 \
  if (kGuideMode == 1u) {                                                 \
    const float2 q = (Q);                                                 \
    const float2 d = (DPT);                                               \
    const float3 p1 = float3(q, 1.0f);                                    \
    const float3 p2 = float3(d, 1.0f);                                    \
    const float3 line2 = float3(                                          \
        M[0] * p1.x + M[1] * p1.y + M[2],                                 \
        M[3] * p1.x + M[4] * p1.y + M[5],                                 \
        M[6] * p1.x + M[7] * p1.y + M[8]);                                \
    const float3 line1 = float3(                                          \
        M[0] * p2.x + M[3] * p2.y + M[6],                                 \
        M[1] * p2.x + M[4] * p2.y + M[7],                                 \
        M[2] * p2.x + M[5] * p2.y + M[8]);                                \
    const float nom = dot(p2, line2);                                     \
    const float denom = dot(line2.xy, line2.xy) +                         \
                        dot(line1.xy, line1.xy);                          \
    geometryOK = denom > 1e-12f &&                                        \
                 nom * nom <= maxResidual * denom;                        \
  } else if (kGuideMode == 2u) {                                          \
    const float2 q = (Q);                                                 \
    const float2 d = (DPT);                                               \
    const float hx = M[0] * q.x + M[1] * q.y + M[2];                      \
    const float hy = M[3] * q.x + M[4] * q.y + M[5];                      \
    const float hz = M[6] * q.x + M[7] * q.y + M[8];                      \
    if (abs(hz) <= 1e-8f) {                                               \
      geometryOK = false;                                                 \
    } else {                                                              \
      const float2 delta = float2(hx / hz, hy / hz) - d;                  \
      geometryOK = dot(delta, delta) <= maxResidual;                      \
    }                                                                     \
  }

kernel void pw_match_gemm2(
    device const half*  Ah          [[buffer(0)]],
    device const half*  Bh          [[buffer(1)]],
    device int*         outAB       [[buffer(2)]],
    constant uint&      numA        [[buffer(3)]],
    constant uint&      numB        [[buffer(4)]],
    constant float&     maxRatio    [[buffer(5)]],
    constant float&     maxDistance [[buffer(6)]],
    constant uint&      rowBase     [[buffer(7)]],
    device float*       colBestOut  [[buffer(8)]],
    device float*       colSecondOut[[buffer(9)]],
    device int*         colIdxOut   [[buffer(10)]],
    constant uint&      colStride   [[buffer(11)]],
    device const uint*  Ap          [[buffer(12)]],
    device const uint*  Bp          [[buffer(13)]],
    device const float2* pointsA    [[buffer(14)]],
    device const float2* pointsB    [[buffer(15)]],
    device const float* matAB       [[buffer(16)]],
    device const float* matBA       [[buffer(17)]],
    constant float&     maxResidual [[buffer(18)]],
    uint tgid [[threadgroup_position_in_grid]],
    uint lid  [[thread_index_in_threadgroup]],
    uint sgid [[simdgroup_index_in_threadgroup]],
    uint lane [[thread_index_in_simdgroup]]) {
  // Threadgroup budget: 8 KB + 16 KB + 3*2 KB = 30 KB (< 32 KB limit).
  threadgroup half4 Bsh4[kBN * kD / 4u];       //  8 KB staged B tile
  threadgroup float accSG[kSG * 8u * kBN];     // 16 KB per-SG 8 x kBN scratch
  threadgroup float cpBest[kSG * kBN];         //  2 KB per-SG column partials
  threadgroup float cpSecond[kSG * kBN];       //  2 KB
  threadgroup int   cpIdx[kSG * kBN];          //  2 KB
  threadgroup half* Bsh = (threadgroup half*)Bsh4;

  // rowBase: first row-block of this chunk (KNIFE-C). Row blocks are
  // independent and column partials are indexed by the ABSOLUTE row-block,
  // so per-row and per-column results are identical for any chunking.
  const uint rb   = rowBase + tgid;   // absolute row-block index
  const uint row0 = rb * kMB;
  if (row0 >= numA) { return; }  // dispatch-overshoot safety (uniform)

  const uint lrow = lane >> 2u;                // 0..7: row within simdgroup
  const uint lcg  = lane & 3u;                 // 0..3: column group
  const uint gRow = row0 + sgid * 8u + lrow;   // global (padded) A row

  // Load this simdgroup's 8 query rows into registers (16 x 8x8 half frags).
  const uint aRow0 = row0 + sgid * 8u;
  simdgroup_matrix<half, 8, 8> aFrag[kKT];
  if (!kPacked) {
    for (uint k = 0; k < kKT; ++k) {
      simdgroup_load(aFrag[k], Ah + aRow0 * kD + k * 8u, kD, ulong2(0, 0));
    }
  } else {
    // Packed A: unpack this SG's 8 rows (256 uints) into a Bsh quadrant, in
    // 4 waves of 4 simdgroups (Bsh = 4096 halfs = exactly 4 x (8*128)).
    const uint wave = sgid >> 2u, slot = sgid & 3u;
    threadgroup half* scratch = Bsh + slot * (8u * kD);
    for (uint w = 0; w < 4u; ++w) {
      if (w == wave) {
        device const uint* src = Ap + (aRow0 * kD) / 4u;
        threadgroup half4* s4 = (threadgroup half4*)scratch;
        for (uint e = lane; e < 256u; e += 32u) {
          const uint u = src[e];
          s4[e] = half4(half(u & 0xFFu), half((u >> 8u) & 0xFFu),
                        half((u >> 16u) & 0xFFu), half(u >> 24u));
        }
        simdgroup_barrier(mem_flags::mem_threadgroup);
        for (uint k = 0; k < kKT; ++k) {
          simdgroup_load(aFrag[k], scratch + k * 8u, kD, ulong2(0, 0));
        }
      }
      threadgroup_barrier(mem_flags::mem_threadgroup);
    }
  }

  // Running A->B top-2 for row `gRow`, held by the lcg==0 lane.
  float rowBestD = 0.0f, rowSecondD = 0.0f;
  int   rowBestI = -1;

  for (uint col0 = 0; col0 < numB; col0 += kBN) {
    // Cooperative contiguous B-tile load (host pads B to a 128-multiple,
    // zero-filled, so the tile read never goes OOB).
    if (!kPacked) {
      device const half4* src4 = (device const half4*)(Bh + col0 * kD);
      for (uint e = lid; e < kBN * kD / 4u; e += kTGT) { Bsh4[e] = src4[e]; }
    } else {
      device const uint* src = Bp + (col0 * kD) / 4u;
      for (uint e = lid; e < kBN * kD / 4u; e += kTGT) {
        const uint u = src[e];
        Bsh4[e] = half4(half(u & 0xFFu), half((u >> 8u) & 0xFFu),
                        half((u >> 16u) & 0xFFu), half(u >> 24u));
      }
    }
    // Barrier (A): Bsh ready; also fences last tile's cp readers vs the
    // cp writes below (merge threads re-join here).
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // GEMM: this SG's 8 rows x the 32-column tile, staged per-SG (no
    // cross-SG sharing -> simdgroup_barrier only).
    for (uint nt = 0; nt < kNT; ++nt) {
      simdgroup_matrix<float, 8, 8> c =
          make_filled_simdgroup_matrix<float, 8, 8>(0.0f);
      for (uint k = 0; k < kKT; ++k) {
        simdgroup_matrix<half, 8, 8> bF;
        simdgroup_load(bF, Bsh + (nt * 8u) * kD + k * 8u, kD, ulong2(0, 0),
                       true);
        simdgroup_multiply_accumulate(c, aFrag[k], bF, c);
      }
      simdgroup_store(c, accSG + sgid * (8u * kBN) + nt * 8u, kBN,
                      ulong2(0, 0));
    }
    simdgroup_barrier(mem_flags::mem_threadgroup);

    // ── Row direction (A->B): lane lcg scans 8 consecutive columns
    //    ascending with strict `>` (v1 semantics), then a 2-stage lcg
    //    butterfly (ours-on-tie, ascending groups) reduces to lcg==0.
    {
      threadgroup const float* accRow =
          accSG + sgid * (8u * kBN) + lrow * kBN;
      const uint per = kBN / 4u;
      float pb = 0.0f, ps = 0.0f;
      int pbi = -1;
      for (uint t = 0; t < per; ++t) {
        const uint cLoc = lcg * per + t;
        if (kGuideMode != 0u) {
          // Row gate: q = pointsA[query row], d = pointsB[candidate col],
          // M = matrixAB — v1 pass-1 verbatim.
          PW_GUIDE_OK(pointsA[gRow], pointsB[col0 + cLoc], matAB)
          if (!geometryOK) continue;
        }
        const float d = accRow[cLoc];
        if (d > pb) { ps = pb; pb = d; pbi = (int)(col0 + cLoc); }
        else if (d > ps) { ps = d; }
      }
      for (ushort off = 1; off <= 2; off <<= 1) {
        const float ob = simd_shuffle_xor(pb, off);
        const float os = simd_shuffle_xor(ps, off);
        const int   oi = simd_shuffle_xor(pbi, off);
        if (ob > pb) { ps = max(os, pb); pb = ob; pbi = oi; }
        else         { ps = max(ps, ob); }
      }
      if (lcg == 0u) {
        if (pb > rowBestD) {
          rowSecondD = max(rowBestD, ps); rowBestD = pb; rowBestI = pbi;
        } else {
          rowSecondD = max(rowSecondD, pb);
        }
      }
    }

    // ── Column direction (B->A): each lane owns one column of the tile
    //    (kBN == simd width == 32) and scans this SG's 8 rows ascending with
    //    strict `>` — literally the standalone B->A per-query scan
    //    restricted to an 8-row slice.
    {
      const uint cc = lane;  // requires kBN == 32
      float cb = 0.0f, cs = 0.0f;
      int ci = -1;
      threadgroup const float* accCol = accSG + sgid * (8u * kBN) + cc;
      for (uint r = 0; r < 8u; ++r) {
        if (kGuideMode != 0u) {
          // Column gate: q = pointsB[query col], d = pointsA[candidate row],
          // M = matrixBA — exactly v1's pass-2 with its swapped arguments.
          PW_GUIDE_OK(pointsB[col0 + cc], pointsA[row0 + sgid * 8u + r],
                      matBA)
          if (!geometryOK) continue;
        }
        const float d = accCol[r * kBN];
        if (d > cb) { cs = cb; cb = d; ci = (int)(row0 + sgid * 8u + r); }
        else if (d > cs) { cs = d; }
      }
      cpBest[sgid * kBN + cc] = cb;
      cpSecond[sgid * kBN + cc] = cs;
      cpIdx[sgid * kBN + cc] = ci;
    }

    // Barrier (B): cp complete from all SGs; all Bsh readers done.
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Cross-SG merge, ascending sgid == ascending global row (tie-break
    // preserved). Runs on 32 threads and overlaps with the other 480
    // threads' next-tile Bsh load (they only sync at barrier A). A parallel
    // shuffle-tree merge was probed and measured SLOWER — the serial form
    // hides behind the other threads' progress. Padded columns of the last
    // tile write harmless partials (stride covers them).
    if (lid < kBN) {
      float b = cpBest[lid], s = cpSecond[lid];
      int bi = cpIdx[lid];
      for (uint sg = 1u; sg < kSG; ++sg) {
        const float ob = cpBest[sg * kBN + lid];
        const float os = cpSecond[sg * kBN + lid];
        const int   oi = cpIdx[sg * kBN + lid];
        if (ob > b) { s = max(b, os); b = ob; bi = oi; }
        else        { s = max(s, ob); }
      }
      const uint j = col0 + lid;
      colBestOut[rb * colStride + j] = b;
      colSecondOut[rb * colStride + j] = s;
      colIdxOut[rb * colStride + j] = bi;
    }
  }

  // Final A->B gates — identical expressions to pw_match_gemm v6 (angular
  // acos domain plain; normalized-L2 + 131072 sentinel guided). Padded rows
  // also write (outAB is over-allocated to the padded row count; the CPU
  // cross-check reads only the first numA entries).
  if (lcg == 0u) {
    if (rowBestI < 0) {
      outAB[gRow] = -1;
    } else {
      float bd;
      float sd;
      if (kGuideMode == 0u) {
        bd = acos(min(rowBestD * kInvSqNorm, 1.0f));
        sd = acos(min(rowSecondD * kInvSqNorm, 1.0f));
      } else {
        const float secondDot = max(rowSecondD, 131072.0f);
        bd = sqrt(max(0.0f, 2.0f - 2.0f * rowBestD * kInvSqNorm));
        sd = sqrt(max(0.0f, 2.0f - 2.0f * secondDot * kInvSqNorm));
      }
      const bool keep = (bd <= maxDistance) && (bd < maxRatio * sd);
      outAB[gRow] = keep ? rowBestI : -1;
    }
  }
}

// Reduces the per-row-block column partials over row-blocks in ascending
// order (== ascending global A row), then applies the SAME final gates as
// the standalone B->A pass (v1 second run). One thread per B column.
kernel void pw_match_merge_cols(
    device const float* colBest   [[buffer(0)]],
    device const float* colSecond [[buffer(1)]],
    device const int*   colIdx    [[buffer(2)]],
    device int*         outBA     [[buffer(3)]],
    constant uint&      numB      [[buffer(4)]],
    constant uint&      colStride [[buffer(5)]],
    constant uint&      numBlocks [[buffer(6)]],
    constant float&     maxRatio  [[buffer(7)]],
    constant float&     maxDistance [[buffer(8)]],
    constant uint&      guideMode [[buffer(9)]],
    uint gid [[thread_position_in_grid]]) {
  if (gid >= numB) return;
  float b = 0.0f, s = 0.0f;
  int bi = -1;
  for (uint rbk = 0; rbk < numBlocks; ++rbk) {
    const uint o = rbk * colStride + gid;
    const float ob = colBest[o];
    const float os = colSecond[o];
    const int   oi = colIdx[o];
    if (ob > b) { s = max(b, os); b = ob; bi = oi; }
    else        { s = max(s, ob); }
  }
  if (bi < 0) { outBA[gid] = -1; return; }
  float bd;
  float sd;
  if (guideMode == 0u) {
    bd = acos(min(b * kInvSqNorm, 1.0f));
    sd = acos(min(s * kInvSqNorm, 1.0f));
  } else {
    const float secondDot = max(s, 131072.0f);
    bd = sqrt(max(0.0f, 2.0f - 2.0f * b * kInvSqNorm));
    sd = sqrt(max(0.0f, 2.0f - 2.0f * secondDot * kInvSqNorm));
  }
  const bool keep = (bd <= maxDistance) && (bd < maxRatio * sd);
  outBA[gid] = keep ? bi : -1;
}
)MSL";

                                                                        
                                                               
static id<MTLDevice> gV2Dev;
static id<MTLCommandQueue> gV2Queue;
static id<MTLLibrary> gV2Lib;
static id<MTLComputePipelineState> gV2Fused[2][3];                        
static id<MTLComputePipelineState> gV2Merge;
static id<MTLBuffer> gV2Dummy;                                             

static BOOL ensureMetalV2(void) {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    gV2Dev = MTLCreateSystemDefaultDevice();
    if (!gV2Dev) return;
    gV2Queue = [gV2Dev newCommandQueue];
    NSError* e = nil;
    gV2Lib = [gV2Dev
        newLibraryWithSource:[NSString stringWithUTF8String:kGemm2KernelSrc]
                     options:nil                                       
                       error:&e];
    if (!gV2Lib) {
      NSLog(@"[pwofficial_gpu_match] v2 kernel compile failed: %@", e);
      return;
    }
    id<MTLFunction> fm = [gV2Lib newFunctionWithName:@"pw_match_merge_cols"];
    if (fm) gV2Merge = [gV2Dev newComputePipelineStateWithFunction:fm error:&e];
    if (!gV2Merge) {
      NSLog(@"[pwofficial_gpu_match] v2 merge pipeline failed: %@", e);
      gV2Lib = nil;
      return;
    }
    gV2Dummy = [gV2Dev newBufferWithLength:16
                                   options:MTLResourceStorageModeShared];
  });
  return gV2Lib != nil && gV2Merge != nil && gV2Dummy != nil;
}

                                                                   
static id<MTLComputePipelineState> v2FusedPipeline(bool packed,
                                                   uint32_t guideMode) {
  const int p = packed ? 1 : 0;
  if (gV2Fused[p][guideMode]) return gV2Fused[p][guideMode];
  MTLFunctionConstantValues* fc = [MTLFunctionConstantValues new];
  bool pv = packed;
  [fc setConstantValue:&pv type:MTLDataTypeBool atIndex:0];
  [fc setConstantValue:&guideMode type:MTLDataTypeUInt atIndex:1];
  NSError* e = nil;
  id<MTLFunction> f = [gV2Lib newFunctionWithName:@"pw_match_gemm2"
                                   constantValues:fc
                                            error:&e];
  id<MTLComputePipelineState> pso =
      f ? [gV2Dev newComputePipelineStateWithFunction:f error:&e] : nil;
  if (!pso) {
    NSLog(@"[pwofficial_gpu_match] v2 pipeline (packed=%d guide=%u) "
          @"failed: %@", p, guideMode, e);
    return nil;
  }
  if (pso.maxTotalThreadsPerThreadgroup < 512) {
                                                                            
                                                                         
                          
    NSLog(@"[pwofficial_gpu_match] v2 pipeline (packed=%d guide=%u) "
          @"maxThreads=%lu < 512 — refusing", p, guideMode,
          (unsigned long)pso.maxTotalThreadsPerThreadgroup);
    return nil;
  }
  gV2Fused[p][guideMode] = pso;
  return pso;
}

                                                                    
                                                                          
                                                                     
                                     
static id<MTLBuffer> gV2A, gV2B, gV2OutAB, gV2OutBA, gV2PtsA, gV2PtsB;
static id<MTLBuffer> gV2ColBest, gV2ColSecond, gV2ColIdx;
static id<MTLBuffer> gV2MatAB, gV2MatBA;                                
static NSUInteger gV2ACap, gV2BCap, gV2OutACap, gV2OutBCap, gV2PtsACap,
    gV2PtsBCap, gV2ColCap, gV2MatABCap, gV2MatBACap;

static id<MTLBuffer> v2PoolBuf(id<MTLBuffer> __strong* slot, NSUInteger* cap,
                               NSUInteger need, MTLResourceOptions opt) {
  if (*cap < need || !*slot) {
    *slot = [gV2Dev newBufferWithLength:need options:opt];
    *cap = *slot ? need : 0;
  }
  return *slot;
}

                                                                                                     
                                                                               
                                                                           
using aether::sfm::DescriptorFormatV1;
using aether::sfm::DescriptorResidencyKeyHashV1;
using aether::sfm::DescriptorResidencyKeyV1;
using aether::sfm::DescriptorResidencyMetadataV1;
using aether::sfm::DescriptorResidencyPolicyV1;

static bool DescriptorResidencyEnabled(void) {
  static const bool enabled = [] {
    const char* value = std::getenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_V1");
    return value && value[0] == '1' && value[1] == '\0';
  }();
  return enabled;
}

static uint64_t DescriptorResidencyBudgetBytes(void) {
  static const uint64_t budget = [] {
    constexpr uint64_t kDefault = UINT64_C(48) * 1024 * 1024;
    const char* value =
        std::getenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_BYTES");
    if (!value || !value[0]) return kDefault;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (!end || *end != '\0') return kDefault;
    return static_cast<uint64_t>(parsed);
  }();
  return budget;
}

struct DescriptorResidencyMetalSessionV1 {
  explicit DescriptorResidencyMetalSessionV1(uint64_t budget)
      : policy(budget) {}

  DescriptorResidencyPolicyV1 policy;
  std::unordered_map<DescriptorResidencyKeyV1, id<MTLBuffer>,
                     DescriptorResidencyKeyHashV1>
      buffers;
  uint64_t upload_bytes = 0;
  uint64_t allocation_failures = 0;
  uint64_t device_resets = 0;
};

static std::unordered_map<uint64_t,
                          std::unique_ptr<DescriptorResidencyMetalSessionV1>>
    gDescriptorResidencySessions;

static DescriptorResidencyMetalSessionV1* DescriptorResidencySession(
    uint64_t nonce) {
  auto found = gDescriptorResidencySessions.find(nonce);
  if (found != gDescriptorResidencySessions.end()) return found->second.get();
  auto inserted = gDescriptorResidencySessions.emplace(
      nonce, std::make_unique<DescriptorResidencyMetalSessionV1>(
                 DescriptorResidencyBudgetBytes()));
  return inserted.first->second.get();
}

static void EraseResidentBuffers(
    DescriptorResidencyMetalSessionV1* session,
    const std::vector<DescriptorResidencyKeyV1>& keys) {
  if (!session) return;
  for (const auto& key : keys) session->buffers.erase(key);
}

static id<MTLBuffer> v2ResidentRawU8Buffer(
    uint64_t session_nonce, uint32_t frame_ordinal, uint32_t generation,
    const uint8_t* descriptors, uint32_t descriptor_count,
    NSUInteger padded_rows) {
  if (!DescriptorResidencyEnabled() || session_nonce == 0 || !descriptors ||
      descriptor_count == 0 || padded_rows < descriptor_count) {
    return nil;
  }
  DescriptorResidencyMetalSessionV1* session =
      DescriptorResidencySession(session_nonce);
  const DescriptorResidencyKeyV1 key{session_nonce, frame_ordinal, generation};
  const uint64_t bytes = static_cast<uint64_t>(padded_rows) * 128;
  const DescriptorResidencyMetadataV1 metadata{
      descriptor_count, DescriptorFormatV1::kRawU8, bytes};
  auto access = session->policy.Access(key, metadata);
  EraseResidentBuffers(session, access.evicted);
  if (access.hit) {
    const auto found = session->buffers.find(key);
    if (found != session->buffers.end() && found->second) return found->second;
                                                                               
                                                                             
    EraseResidentBuffers(
        session,
        session->policy.InvalidateFrame(session_nonce, frame_ordinal));
    access = session->policy.Access(key, metadata);
    EraseResidentBuffers(session, access.evicted);
  }
  if (!access.admitted) return nil;

  id<MTLBuffer> buffer =
      [gV2Dev newBufferWithLength:static_cast<NSUInteger>(bytes)
                          options:MTLResourceStorageModeShared];
  if (!buffer) {
    ++session->allocation_failures;
    EraseResidentBuffers(
        session,
        session->policy.InvalidateFrame(session_nonce, frame_ordinal));
    return nil;
  }
  const size_t live_bytes = static_cast<size_t>(descriptor_count) * 128;
  std::memcpy(buffer.contents, descriptors, live_bytes);
  std::memset(static_cast<uint8_t*>(buffer.contents) + live_bytes, 0,
              static_cast<size_t>(bytes) - live_bytes);
  session->upload_bytes += bytes;
  session->buffers[key] = buffer;
  return buffer;
}

static void ClearAllDescriptorResidencyForDeviceError(void) {
  for (auto& [nonce, session] : gDescriptorResidencySessions) {
    (void)nonce;
    session->policy.ClearAll();
    session->buffers.clear();
    ++session->device_resets;
  }
}

                                                                      
                                                                        
                                                                   
                                                                      
static int matchPairsImplV2(const uint8_t* dA, int nA, const float* xyA,
                            const uint8_t* dB, int nB, const float* xyB,
                            double max_ratio, const float* matrixAB,
                            const float* matrixBA, uint32_t guideMode,
                            float maxResidual, uint32_t* out_pairs,
                            int max_pairs, int* out_num_matches,
                            uint64_t residency_session_nonce = 0,
                            uint32_t residency_frame_a = 0,
                            uint32_t residency_generation_a = 0,
                            uint32_t residency_frame_b = 0,
                            uint32_t residency_generation_b = 0) {
  @autoreleasepool {
    if (out_num_matches) *out_num_matches = 0;
    if (!dA || !dB || nA <= 0 || nB <= 0) return 1;
    if (out_pairs != nullptr && max_pairs <= 0) return 1;
    if (guideMode > 2u) return 1;
    if (guideMode != 0u &&
        (!xyA || !xyB || !matrixAB || !matrixBA || maxResidual <= 0.0f)) {
      return 1;
    }
    if (!ensureMetalV2()) return 2;
    const bool packed = !MatchV2ForceHalf();
    id<MTLComputePipelineState> fused = v2FusedPipeline(packed, guideMode);
    if (!fused) return 2;
    const int D = 128;
                                                                          
                                                               
    const NSUInteger nApad = (((NSUInteger)nA + 127) / 128) * 128;
    const NSUInteger nBpad = (((NSUInteger)nB + 127) / 128) * 128;
    id<MTLBuffer> aBuf, bBuf;
    if (!packed) {
      aBuf = v2PoolBuf(&gV2A, &gV2ACap, nApad * D * sizeof(__fp16),
                       MTLResourceStorageModeShared);
      bBuf = v2PoolBuf(&gV2B, &gV2BCap, nBpad * D * sizeof(__fp16),
                       MTLResourceStorageModeShared);
      if (!aBuf || !bBuf) return 5;
      __fp16* af = (__fp16*)aBuf.contents;
      for (NSUInteger i = 0; i < (NSUInteger)nA * D; ++i) af[i] = (__fp16)dA[i];
      for (NSUInteger i = (NSUInteger)nA * D; i < nApad * D; ++i) af[i] = 0;
      __fp16* bf = (__fp16*)bBuf.contents;
      for (NSUInteger i = 0; i < (NSUInteger)nB * D; ++i) bf[i] = (__fp16)dB[i];
      for (NSUInteger i = (NSUInteger)nB * D; i < nBpad * D; ++i) bf[i] = 0;
    } else {
                                                                            
                                                                        
                                                               
      aBuf = v2ResidentRawU8Buffer(
          residency_session_nonce, residency_frame_a,
          residency_generation_a, dA, static_cast<uint32_t>(nA), nApad);
      bBuf = v2ResidentRawU8Buffer(
          residency_session_nonce, residency_frame_b,
          residency_generation_b, dB, static_cast<uint32_t>(nB), nBpad);
      const bool aResident = aBuf != nil;
      const bool bResident = bBuf != nil;
      if (!aBuf) {
        aBuf = v2PoolBuf(&gV2A, &gV2ACap, nApad * D,
                         MTLResourceStorageModeShared);
      }
      if (!bBuf) {
        bBuf = v2PoolBuf(&gV2B, &gV2BCap, nBpad * D,
                         MTLResourceStorageModeShared);
      }
      if (!aBuf || !bBuf) return 5;
      if (!aResident) {
        memcpy(aBuf.contents, dA, (size_t)nA * D);
        memset((uint8_t*)aBuf.contents + (size_t)nA * D, 0,
               (size_t)(nApad - (NSUInteger)nA) * D);
      }
      if (!bResident) {
        memcpy(bBuf.contents, dB, (size_t)nB * D);
        memset((uint8_t*)bBuf.contents + (size_t)nB * D, 0,
               (size_t)(nBpad - (NSUInteger)nB) * D);
      }
    }
                                                                           
                                                                          
    id<MTLBuffer> outAB = v2PoolBuf(&gV2OutAB, &gV2OutACap,
                                    nApad * sizeof(int),
                                    MTLResourceStorageModeShared);
    id<MTLBuffer> outBA = v2PoolBuf(&gV2OutBA, &gV2OutBCap,
                                    (NSUInteger)nB * sizeof(int),
                                    MTLResourceStorageModeShared);
    if (!outAB || !outBA) return 6;
    id<MTLBuffer> ptsA = gV2Dummy;
    id<MTLBuffer> ptsB = gV2Dummy;
    if (guideMode != 0u) {
                                                                           
                                                                           
                                                               
      ptsA = v2PoolBuf(&gV2PtsA, &gV2PtsACap, nApad * 2 * sizeof(float),
                       MTLResourceStorageModeShared);
      ptsB = v2PoolBuf(&gV2PtsB, &gV2PtsBCap, nBpad * 2 * sizeof(float),
                       MTLResourceStorageModeShared);
      if (!ptsA || !ptsB) return 6;
      memcpy(ptsA.contents, xyA, (size_t)nA * 2 * sizeof(float));
      memset((uint8_t*)ptsA.contents + (size_t)nA * 2 * sizeof(float), 0,
             (size_t)(nApad - (NSUInteger)nA) * 2 * sizeof(float));
      memcpy(ptsB.contents, xyB, (size_t)nB * 2 * sizeof(float));
      memset((uint8_t*)ptsB.contents + (size_t)nB * 2 * sizeof(float), 0,
             (size_t)(nBpad - (NSUInteger)nB) * 2 * sizeof(float));
    }
    const NSUInteger numBlocksA = nApad / 128;
    const NSUInteger colBytes = numBlocksA * nBpad * 4;
    if (gV2ColCap < colBytes) {
      gV2ColBest = [gV2Dev newBufferWithLength:colBytes
                                       options:MTLResourceStorageModePrivate];
      gV2ColSecond = [gV2Dev newBufferWithLength:colBytes
                                         options:MTLResourceStorageModePrivate];
      gV2ColIdx = [gV2Dev newBufferWithLength:colBytes
                                      options:MTLResourceStorageModePrivate];
      gV2ColCap = (gV2ColBest && gV2ColSecond && gV2ColIdx) ? colBytes : 0;
      if (gV2ColCap == 0) return 6;
    }
    static const float kIdentity[9] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f};
    const float* matAtoB = guideMode == 0u ? kIdentity : matrixAB;
    const float* matBtoA = guideMode == 0u ? kIdentity : matrixBA;
                                                                             
                                                                            
                                     
    id<MTLBuffer> matABBuf = v2PoolBuf(&gV2MatAB, &gV2MatABCap,
                                       9 * sizeof(float),
                                       MTLResourceStorageModeShared);
    id<MTLBuffer> matBABuf = v2PoolBuf(&gV2MatBA, &gV2MatBACap,
                                       9 * sizeof(float),
                                       MTLResourceStorageModeShared);
    if (!matABBuf || !matBABuf) return 6;
    memcpy(matABBuf.contents, matAtoB, 9 * sizeof(float));
    memcpy(matBABuf.contents, matBtoA, 9 * sizeof(float));
    float maxRatio = (float)max_ratio;
    if (maxRatio <= 0.0f) maxRatio = 0.8f;
    float maxDistance = 0.7f;                                                      
    const uint32_t numAU = (uint32_t)nA, numBU = (uint32_t)nB;
    const uint32_t colStride = (uint32_t)nBpad;
    const uint32_t numBlocksU = (uint32_t)numBlocksA;

    auto encFused = [&](id<MTLCommandBuffer> cmd, uint32_t rowBase,
                        NSUInteger groups) {
      id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
      [enc setComputePipelineState:fused];
      [enc setBuffer:aBuf offset:0 atIndex:0];
      [enc setBuffer:bBuf offset:0 atIndex:1];
      [enc setBuffer:outAB offset:0 atIndex:2];
      [enc setBytes:&numAU length:4 atIndex:3];
      [enc setBytes:&numBU length:4 atIndex:4];
      [enc setBytes:&maxRatio length:4 atIndex:5];
      [enc setBytes:&maxDistance length:4 atIndex:6];
      [enc setBytes:&rowBase length:4 atIndex:7];
      [enc setBuffer:gV2ColBest offset:0 atIndex:8];
      [enc setBuffer:gV2ColSecond offset:0 atIndex:9];
      [enc setBuffer:gV2ColIdx offset:0 atIndex:10];
      [enc setBytes:&colStride length:4 atIndex:11];
      [enc setBuffer:aBuf offset:0 atIndex:12];                           
      [enc setBuffer:bBuf offset:0 atIndex:13];
      [enc setBuffer:ptsA offset:0 atIndex:14];
      [enc setBuffer:ptsB offset:0 atIndex:15];
      [enc setBuffer:matABBuf offset:0 atIndex:16];
      [enc setBuffer:matBABuf offset:0 atIndex:17];
      [enc setBytes:&maxResidual length:4 atIndex:18];
      [enc dispatchThreadgroups:MTLSizeMake(groups, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(512, 1, 1)];
      [enc endEncoding];
    };
    auto encMerge = [&](id<MTLCommandBuffer> cmd) {
      id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
      [enc setComputePipelineState:gV2Merge];
      [enc setBuffer:gV2ColBest offset:0 atIndex:0];
      [enc setBuffer:gV2ColSecond offset:0 atIndex:1];
      [enc setBuffer:gV2ColIdx offset:0 atIndex:2];
      [enc setBuffer:outBA offset:0 atIndex:3];
      [enc setBytes:&numBU length:4 atIndex:4];
      [enc setBytes:&colStride length:4 atIndex:5];
      [enc setBytes:&numBlocksU length:4 atIndex:6];
      [enc setBytes:&maxRatio length:4 atIndex:7];
      [enc setBytes:&maxDistance length:4 atIndex:8];
      [enc setBytes:&guideMode length:4 atIndex:9];
      [enc dispatchThreadgroups:MTLSizeMake(((NSUInteger)nB + 255) / 256, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
      [enc endEncoding];
    };

    const double chunkTargetMs = ChunkTargetMs();
    if (chunkTargetMs <= 0.0) {
                                                                        
      id<MTLCommandBuffer> cmd = [gV2Queue commandBuffer];
      encFused(cmd, 0u, numBlocksA);
      encMerge(cmd);
                                                                                      
                                                    
      const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
      if (wrc != 0) return wrc;
    } else {
                                                                          
                                                                        
                                                                          
      const NSUInteger totalGroups = numBlocksA;
      NSUInteger tg0 = 0;
      while (tg0 < totalGroups) {
        const double target =
            ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
        const double unit = gMsPerTgKColV2.load();
        NSUInteger want = 8;                                          
        if (unit > 0.0) {
          const double perTg = unit * ((double)numBU / 1024.0);
          const double ideal = target / (perTg > 1e-6 ? perTg : 1e-6);
          want = ideal < 1.0 ? 1 : (NSUInteger)ideal;
        }
        const NSUInteger groups =
            want < totalGroups - tg0 ? want : totalGroups - tg0;
        id<MTLCommandBuffer> cmd = [gV2Queue commandBuffer];
        encFused(cmd, (uint32_t)tg0, groups);
                                                                                
        const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
        if (wrc != 0) return wrc;
        const double gpuMs = (cmd.GPUEndTime - cmd.GPUStartTime) * 1000.0;
        if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
        ++aether_match_chunks;
        if (gpuMs > 0.0 && gpuMs < 10000.0) {
          const double u = gpuMs / ((double)groups * ((double)numBU / 1024.0));
          const double prev = gMsPerTgKColV2.load();
          gMsPerTgKColV2.store(prev <= 0.0 ? u : prev * 0.7 + u * 0.3);
        }
        const double gapPct = ThermalGapPct();
        if (gapPct > 0.0 && gpuMs > 0.0) {
          double gapMs = gpuMs * gapPct / 100.0;
          if (gapMs > 250.0) gapMs = 250.0;
          aether_match_sleep_ms += gapMs;
          usleep((useconds_t)(gapMs * 1000.0));
        }
        tg0 += groups;
      }
      id<MTLCommandBuffer> cmd = [gV2Queue commandBuffer];
      encMerge(cmd);
                                                                                      
                                                    
      const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
      if (wrc != 0) return wrc;
    }

                                                                 
    const int* mAB = (const int*)outAB.contents;
    const int* mBA = (const int*)outBA.contents;
    int n_out = 0;
    for (int i = 0; i < nA; ++i) {
      int j = mAB[i];
      if (j >= 0 && j < nB && mBA[j] == i) {
        if (out_pairs != nullptr) {
          if (n_out >= max_pairs) break;                             
          out_pairs[2 * n_out] = (uint32_t)i;
          out_pairs[2 * n_out + 1] = (uint32_t)j;
        }
        ++n_out;
      }
    }
    if (out_num_matches) *out_num_matches = n_out;
    return 0;
  }
}

                                                                                                                                                                                
                                                                        
                                                                    
static std::mutex gMatchCallLock;

                                                              
                                                                             
                                                                        
extern "C" int aether_gpu_match_gemm_pairs(const uint8_t* dA, int nA,
                                           const uint8_t* dB, int nB,
                                           double max_ratio,
                                           uint32_t* out_pairs, int max_pairs,
                                           int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  if (MatchV2Enabled()) {
    return matchPairsImplV2(dA, nA, nullptr, dB, nB, nullptr, max_ratio,
                            nullptr, nullptr, 0u, 0.0f, out_pairs, max_pairs,
                            out_num_matches);
  }
  return matchPairsImpl(dA, nA, nullptr, dB, nB, nullptr, max_ratio, nullptr,
                        nullptr, 0u, 0.0f, out_pairs, max_pairs,
                        out_num_matches);
}

                                                                           
                                                                             
                                                                       
                                                                            
extern "C" int aether_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame_a, uint32_t generation_a,
    const uint8_t* dA, int nA, uint32_t frame_b, uint32_t generation_b,
    const uint8_t* dB, int nB, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  if (MatchV2Enabled()) {
    return matchPairsImplV2(
        dA, nA, nullptr, dB, nB, nullptr, max_ratio, nullptr, nullptr, 0u,
        0.0f, out_pairs, max_pairs, out_num_matches, session_nonce, frame_a,
        generation_a, frame_b, generation_b);
  }
  return matchPairsImpl(dA, nA, nullptr, dB, nB, nullptr, max_ratio, nullptr,
                        nullptr, 0u, 0.0f, out_pairs, max_pairs,
                        out_num_matches);
}

extern "C" void aether_gpu_match_descriptor_residency_invalidate(
    uint64_t session_nonce, uint32_t frame_ordinal) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  const auto found = gDescriptorResidencySessions.find(session_nonce);
  if (found == gDescriptorResidencySessions.end()) return;
  EraseResidentBuffers(
      found->second.get(),
      found->second->policy.InvalidateFrame(session_nonce, frame_ordinal));
}

extern "C" void aether_gpu_match_descriptor_residency_clear_session(
    uint64_t session_nonce) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  const auto found = gDescriptorResidencySessions.find(session_nonce);
  if (found == gDescriptorResidencySessions.end()) return;
  found->second->policy.ClearSession(session_nonce);
  found->second->buffers.clear();
  gDescriptorResidencySessions.erase(found);
}

extern "C" int aether_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements,
    uint64_t* upload_bytes, uint64_t* resident_bytes,
    uint64_t* resident_entries, uint64_t* allocation_failures,
    uint64_t* device_resets) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  uint64_t values[9] = {0};
  const auto found = gDescriptorResidencySessions.find(session_nonce);
  if (found != gDescriptorResidencySessions.end()) {
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
  uint64_t* outputs[9] = {hits,
                          misses,
                          evictions,
                          stale_replacements,
                          upload_bytes,
                          resident_bytes,
                          resident_entries,
                          allocation_failures,
                          device_resets};
  for (size_t i = 0; i < 9; ++i) {
    if (outputs[i]) *outputs[i] = values[i];
  }
  return found == gDescriptorResidencySessions.end() ? 0 : 1;
}

                                                                                    
                                                                           
                                                                            
                                                                            
                                                                           
                                                                         
                                                                             
                                                                              
                                                                                 
                     
  
                                                                           
                                                                                
                                                                           
                                                                             
                                                                           
                                                                             
                                                                     
  
                                                                         
                                                                            
                                                                               
                                                                       
                                                                            
                                                                             
                                                                                
                               
                                                                        
                                                                           
                                                                          
                                                                    
static id<MTLBuffer> gPbA, gPbB, gPbOutAB, gPbOutBA;
static id<MTLBuffer> gPbColBest, gPbColSecond, gPbColIdx;
static NSUInteger gPbACap, gPbBCap, gPbOutACap, gPbOutBCap, gPbColCap;

static int probeBatchImplV2(const uint8_t* dA, int nA,
                            const uint8_t* const* dBs, const int* nBs,
                            int n_cands, double max_ratio, int* out_counts) {
  @autoreleasepool {
    if (!dA || !dBs || !nBs || !out_counts || nA <= 0 || n_cands <= 0) {
      return 1;
    }
    for (int c = 0; c < n_cands; ++c) {
      if (!dBs[c] || nBs[c] <= 0) return 1;
      out_counts[c] = 0;
    }
    if (!MatchV2Enabled()) return 2;
    if (!ensureMetalV2()) return 2;
    const bool packed = !MatchV2ForceHalf();
    id<MTLComputePipelineState> fused = v2FusedPipeline(packed, 0u);
    if (!fused) return 2;
    const int D = 128;
    const NSUInteger elem = packed ? 1 : sizeof(__fp16);
    const NSUInteger nApad = (((NSUInteger)nA + 127) / 128) * 128;
    const NSUInteger numBlocksA = nApad / 128;
                                                                             
                                                                              
                                              
    std::vector<NSUInteger> nBpadv((size_t)n_cands);
    std::vector<NSUInteger> bOff((size_t)n_cands), outBAOff((size_t)n_cands),
        colOff((size_t)n_cands);
    NSUInteger bTotal = 0, outBATotal = 0, colTotal = 0;
    for (int c = 0; c < n_cands; ++c) {
      const NSUInteger nBpad = (((NSUInteger)nBs[c] + 127) / 128) * 128;
      nBpadv[(size_t)c] = nBpad;
      bOff[(size_t)c] = bTotal;
      outBAOff[(size_t)c] = outBATotal;
      colOff[(size_t)c] = colTotal;
      bTotal += nBpad * D * elem;
      outBATotal += nBpad * sizeof(int);
      colTotal += numBlocksA * nBpad * sizeof(float);
    }
    id<MTLBuffer> aBuf = v2PoolBuf(&gPbA, &gPbACap, nApad * D * elem,
                                   MTLResourceStorageModeShared);
    id<MTLBuffer> bBuf = v2PoolBuf(&gPbB, &gPbBCap, bTotal,
                                   MTLResourceStorageModeShared);
    if (!aBuf || !bBuf) return 5;
    if (packed) {
      memcpy(aBuf.contents, dA, (size_t)nA * D);
      memset((uint8_t*)aBuf.contents + (size_t)nA * D, 0,
             (size_t)(nApad - (NSUInteger)nA) * D);
      for (int c = 0; c < n_cands; ++c) {
        uint8_t* dst = (uint8_t*)bBuf.contents + bOff[(size_t)c];
        memcpy(dst, dBs[c], (size_t)nBs[c] * D);
        memset(dst + (size_t)nBs[c] * D, 0,
               (size_t)(nBpadv[(size_t)c] - (NSUInteger)nBs[c]) * D);
      }
    } else {
      __fp16* af = (__fp16*)aBuf.contents;
      for (NSUInteger i = 0; i < (NSUInteger)nA * D; ++i) af[i] = (__fp16)dA[i];
      for (NSUInteger i = (NSUInteger)nA * D; i < nApad * D; ++i) af[i] = 0;
      for (int c = 0; c < n_cands; ++c) {
        __fp16* bf = (__fp16*)((uint8_t*)bBuf.contents + bOff[(size_t)c]);
        for (NSUInteger i = 0; i < (NSUInteger)nBs[c] * D; ++i) {
          bf[i] = (__fp16)dBs[c][i];
        }
        for (NSUInteger i = (NSUInteger)nBs[c] * D;
             i < nBpadv[(size_t)c] * D; ++i) {
          bf[i] = 0;
        }
      }
    }
    id<MTLBuffer> outAB =
        v2PoolBuf(&gPbOutAB, &gPbOutACap,
                  (NSUInteger)n_cands * nApad * sizeof(int),
                  MTLResourceStorageModeShared);
    id<MTLBuffer> outBA = v2PoolBuf(&gPbOutBA, &gPbOutBCap, outBATotal,
                                    MTLResourceStorageModeShared);
    if (!outAB || !outBA) return 6;
    if (gPbColCap < colTotal) {
      gPbColBest = [gV2Dev newBufferWithLength:colTotal
                                       options:MTLResourceStorageModePrivate];
      gPbColSecond = [gV2Dev newBufferWithLength:colTotal
                                         options:MTLResourceStorageModePrivate];
      gPbColIdx = [gV2Dev newBufferWithLength:colTotal
                                      options:MTLResourceStorageModePrivate];
      gPbColCap = (gPbColBest && gPbColSecond && gPbColIdx) ? colTotal : 0;
      if (gPbColCap == 0) return 6;
    }
    float maxRatio = (float)max_ratio;
    if (maxRatio <= 0.0f) maxRatio = 0.8f;
    float maxDistance = 0.7f;                                             
    const float maxResidual = 0.0f;
    const uint32_t guideMode = 0u;
    const uint32_t rowBase = 0u;
    const uint32_t numAU = (uint32_t)nA;

    auto encFusedCand = [&](id<MTLComputeCommandEncoder> enc, int c) {
      const uint32_t numBU = (uint32_t)nBs[c];
      const uint32_t colStride = (uint32_t)nBpadv[(size_t)c];
      [enc setComputePipelineState:fused];
      [enc setBuffer:aBuf offset:0 atIndex:0];
      [enc setBuffer:bBuf offset:bOff[(size_t)c] atIndex:1];
      [enc setBuffer:outAB offset:(NSUInteger)c * nApad * sizeof(int)
              atIndex:2];
      [enc setBytes:&numAU length:4 atIndex:3];
      [enc setBytes:&numBU length:4 atIndex:4];
      [enc setBytes:&maxRatio length:4 atIndex:5];
      [enc setBytes:&maxDistance length:4 atIndex:6];
      [enc setBytes:&rowBase length:4 atIndex:7];
      [enc setBuffer:gPbColBest offset:colOff[(size_t)c] atIndex:8];
      [enc setBuffer:gPbColSecond offset:colOff[(size_t)c] atIndex:9];
      [enc setBuffer:gPbColIdx offset:colOff[(size_t)c] atIndex:10];
      [enc setBytes:&colStride length:4 atIndex:11];
      [enc setBuffer:aBuf offset:0 atIndex:12];                           
      [enc setBuffer:bBuf offset:bOff[(size_t)c] atIndex:13];
      [enc setBuffer:gV2Dummy offset:0 atIndex:14];
      [enc setBuffer:gV2Dummy offset:0 atIndex:15];
      [enc setBuffer:gV2Dummy offset:0 atIndex:16];
      [enc setBuffer:gV2Dummy offset:0 atIndex:17];
      [enc setBytes:&maxResidual length:4 atIndex:18];
      [enc dispatchThreadgroups:MTLSizeMake(numBlocksA, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(512, 1, 1)];
    };
    auto encMergeCand = [&](id<MTLComputeCommandEncoder> enc, int c) {
      const uint32_t numBU = (uint32_t)nBs[c];
      const uint32_t colStride = (uint32_t)nBpadv[(size_t)c];
      const uint32_t numBlocksU = (uint32_t)numBlocksA;
      [enc setComputePipelineState:gV2Merge];
      [enc setBuffer:gPbColBest offset:colOff[(size_t)c] atIndex:0];
      [enc setBuffer:gPbColSecond offset:colOff[(size_t)c] atIndex:1];
      [enc setBuffer:gPbColIdx offset:colOff[(size_t)c] atIndex:2];
      [enc setBuffer:outBA offset:outBAOff[(size_t)c] atIndex:3];
      [enc setBytes:&numBU length:4 atIndex:4];
      [enc setBytes:&colStride length:4 atIndex:5];
      [enc setBytes:&numBlocksU length:4 atIndex:6];
      [enc setBytes:&maxRatio length:4 atIndex:7];
      [enc setBytes:&maxDistance length:4 atIndex:8];
      [enc setBytes:&guideMode length:4 atIndex:9];
      [enc dispatchThreadgroups:MTLSizeMake(((NSUInteger)nBs[c] + 255) / 256,
                                            1, 1)
          threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
    };
                                                                             
                                                                            
                                                                          
                                                                             
                                                                            
                                                                        
                                                                           
                                                                      
                                                                          
                                                                           
                                                                             
    auto runGroup = [&](int c_begin, int c_end, double groupUnits) -> int {
      id<MTLCommandBuffer> cmd = [gV2Queue commandBuffer];
      id<MTLComputeCommandEncoder> enc1 = [cmd
          computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
      for (int c = c_begin; c < c_end; ++c) encFusedCand(enc1, c);
      [enc1 endEncoding];
      id<MTLComputeCommandEncoder> enc2 = [cmd
          computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
      for (int c = c_begin; c < c_end; ++c) encMergeCand(enc2, c);
      [enc2 endEncoding];
      const int wrc = WaitCmdWithTimeout(cmd, CmdWaitTimeoutMs());
      if (wrc != 0) return wrc;
      const double gpuMs = (cmd.GPUEndTime - cmd.GPUStartTime) * 1000.0;
      if (gpuMs > 0.0 && gpuMs < 10000.0) aether_match_gpu_ms += gpuMs;
      ++aether_match_chunks;
                                                                           
                                                                          
                                                                           
                                                                             
      (void)groupUnits;
      const double gapPct = ThermalGapPct();
      if (gapPct > 0.0 && gpuMs > 0.0) {
        double gapMs = gpuMs * gapPct / 100.0;
        if (gapMs > 250.0) gapMs = 250.0;
        aether_match_sleep_ms += gapMs;
        usleep((useconds_t)(gapMs * 1000.0));
      }
      return 0;
    };
                                                                           
                                                                             
                                                                            
                                         
                                                                             
                           
    const double chunkTargetMs = ChunkTargetMs();
    double estCap = 0.0;                                                   
    if (chunkTargetMs > 0.0) {
      const double target = ThermalHot() ? chunkTargetMs : ChunkTargetCoolMs();
      const double unit = gMsPerTgKColV2.load();
      estCap = unit > 0.0 ? target / unit : 0.0;                     
    }
    int group_begin = 0;
    double groupUnits = 0.0;
    for (int c = 0; c < n_cands; ++c) {
      const double candUnits =
          (double)numBlocksA * ((double)nBs[c] / 1024.0);
      if (c > group_begin && estCap > 0.0 && groupUnits + candUnits > estCap) {
        const int wrc = runGroup(group_begin, c, groupUnits);
        if (wrc != 0) return wrc;
        group_begin = c;
        groupUnits = 0.0;
      }
      groupUnits += candUnits;
    }
    if (group_begin < n_cands) {
      const int wrc = runGroup(group_begin, n_cands, groupUnits);
      if (wrc != 0) return wrc;
    }

                                                                             
                                                                      
    for (int c = 0; c < n_cands; ++c) {
      const int* mAB =
          (const int*)((const uint8_t*)outAB.contents +
                       (NSUInteger)c * nApad * sizeof(int));
      const int* mBA = (const int*)((const uint8_t*)outBA.contents +
                                    outBAOff[(size_t)c]);
      const int nB = nBs[c];
      int n_out = 0;
      for (int i = 0; i < nA; ++i) {
        const int j = mAB[i];
        if (j >= 0 && j < nB && mBA[j] == i) ++n_out;
      }
      out_counts[c] = n_out;
    }
    return 0;
  }
}

                                                                             
                                                        
extern "C" int aether_gpu_match_probe_batch(const uint8_t* dA, int nA,
                                            const uint8_t* const* dBs,
                                            const int* nBs, int n_cands,
                                            double max_ratio,
                                            int* out_counts) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
  return probeBatchImplV2(dA, nA, dBs, nBs, n_cands, max_ratio, out_counts);
}

                                                                         
                                                                             
                                                                             
                                                                 
extern "C" int aether_gpu_match_gemm_pairs_guided(
    const uint8_t* dA, int nA, const float* xyA, const uint8_t* dB, int nB,
    const float* xyB, double max_ratio, const float* matrixAB,
    const float* matrixBA, int guide_mode, float max_residual,
    uint32_t* out_pairs, int max_pairs, int* out_num_matches) {
  std::lock_guard<std::mutex> lk(gMatchCallLock);
                                                                            
                                                                          
                                                    
  if (MatchV2Enabled() && MatchV2GuidedFused()) {
    return matchPairsImplV2(dA, nA, xyA, dB, nB, xyB, max_ratio, matrixAB,
                            matrixBA, (uint32_t)guide_mode, max_residual,
                            out_pairs, max_pairs, out_num_matches);
  }
  return matchPairsImpl(dA, nA, xyA, dB, nB, xyB, max_ratio, matrixAB,
                        matrixBA, (uint32_t)guide_mode, max_residual,
                        out_pairs, max_pairs, out_num_matches);
}
