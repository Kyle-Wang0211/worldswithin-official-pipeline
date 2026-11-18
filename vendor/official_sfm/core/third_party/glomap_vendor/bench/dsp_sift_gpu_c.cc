                                                                            
  
                                                                     
                                                                          
                                                                             
                                                                                
                                                                        
                                                                              
                                                                              
                                                                             
                     
  
                                                                           
                                                                              
                                                                                  
                                                                             

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>                                                     
#include <cstdlib>           
#include <cstring>
#include <chrono>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

#include "dawn_kernel_harness.h"
#include "dawn_histogram_sink.h"
#include "sift_extract_dawn.h"
#if defined(AETHER_FEATURE_SELECTION_ENV_OFFICIAL)
#define AETHER_PRECLAMP_INSTR_ENV_OFFICIAL 1
#elif defined(AETHER_FEATURE_SELECTION_ENV_SELFTEST)
#define AETHER_PRECLAMP_INSTR_ENV_SELFTEST 1
#else
#error "preclamp instr: feature-selection environment namespace is absent"
#endif
#include "official_preclamp_instr_v1.h"

extern "C" {

                                                                                
int aether_dsp_sift_extract_threaded(const uint8_t* gray, int width, int height,
                                     int max_features, int num_threads,
                                     float* out_xy, uint8_t* out_desc,
                                     int out_cap, int* out_count);
                                                                            
                                                                           
int aether_dsp_sift_extract_threaded_v2(const uint8_t* gray, int width,
                                        int height, int max_features,
                                        int num_threads, float* out_xy,
                                        uint8_t* out_desc, float* out_scales,
                                        float* out_orientations, int out_cap,
                                        int* out_count);
void aether_sed_clear_last_stages();
void aether_sed_last_stages_gpu(double* out, int cap);

}               

namespace {

constexpr int kCanonicalGpuUnavailable = 2;

                                             
                                                                             
                                                                            
                                                                          
                                                                             
                                                                            
                                                                                 
                                                                                
                                                   
std::mutex g_gpu_harness_mu;
aether::tools::DawnKernelHarness* g_gpu_harness = nullptr;
bool g_gpu_harness_init_failed = false;
                                                                                              
int g_gpu_harness_rebuilds = 0;
#if defined(AETHER_PRECLAMP_INSTR_TEST_HOOKS)
thread_local bool g_force_next_gpu_fallback_for_test = false;
#endif

                                                                  
                                     
aether::tools::DawnKernelHarness* acquire_gpu_harness() {
    if (g_gpu_harness_init_failed) return nullptr;
                                                                                           
                                                                                                    
                                                                                        
                                                                                     
                                                                                                      
                                                                                    
                  
    if (g_gpu_harness != nullptr && !g_gpu_harness->healthy()) {
        g_gpu_harness = nullptr;                              
        ++g_gpu_harness_rebuilds;
                                                                                        
                                                                                                    
                                                                     
        if (g_gpu_harness_rebuilds > 6) {
            g_gpu_harness_init_failed = true;
            return nullptr;
        }
    }
    if (g_gpu_harness == nullptr) {
        auto* h = new aether::tools::DawnKernelHarness();
        if (!h->init()) {
            delete h;
            g_gpu_harness_init_failed = true;
            return nullptr;
        }
        g_gpu_harness = h;
    }
    return g_gpu_harness;
}

                                                                              
                                                                            
void finish_descriptor(const float* raw128, uint8_t* out128) {
    double l1 = 0.0;
    for (int i = 0; i < 128; ++i) l1 += std::abs(static_cast<double>(raw128[i]));
    std::array<float, 128> root{};
    if (l1 > 0.0) {
        for (int i = 0; i < 128; ++i)
            root[i] = static_cast<float>(
                std::sqrt(std::abs(static_cast<double>(raw128[i])) / l1));
    }
    std::array<uint8_t, 128> tmp{};
    for (int i = 0; i < 128; ++i) {
        const float v = std::round(512.0f * root[i]);
        int iv = static_cast<int>(v);
        iv = std::max(0, std::min(255, iv));
        tmp[i] = static_cast<uint8_t>(iv);
    }
                                                   
    static const int q[8] = {0, 7, 6, 5, 4, 3, 2, 1};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 8; ++k)
                out128[8 * (j + 4 * i) + q[k]] = tmp[8 * (j + 4 * i) + k];
}

}              

extern "C" {

#if defined(AETHER_PRECLAMP_INSTR_TEST_HOOKS)
__attribute__((visibility("hidden"))) void
aether_preclamp_instr_force_next_gpu_fallback_for_test() {
    g_force_next_gpu_fallback_for_test = true;
}
#endif

                                                                            
                                                                             
                                                                            
                                                                 
  
                                                                       
                                                                                
                                                                           
                                                                            
                                                                            
                                                                              
                                                                        
                                                                                
int aether_dsp_sift_extract_gpu_v2(const uint8_t* gray, int width, int height,
                                   int max_features, int num_threads,
                                   float* out_xy, uint8_t* out_desc,
                                   float* out_scales, float* out_orientations,
                                   int out_cap, int* out_count) {
    aether_preclamp_instr_v1::ClearPendingAtGpuEntry();
    aether::tools::DawnKernelHarness::gpu_ts_clear_caller_thread_frame();
    aether_sed_clear_last_stages();
    if (out_count) *out_count = 0;
    (void)num_threads;
    if (gray == nullptr || width <= 0 || height <= 0 || out_cap <= 0) return 1;

    const auto selection_policy =
        aether::tools::SiftExtractDawn::feature_selection_policy();
    const bool deterministic_selection_requested =
        selection_policy.policy !=
        aether::sfm::FeatureSelectionPolicyV1::kLegacyColmapGroup;

    auto fallback = [&]() -> int {
        aether_preclamp_instr_v1::DiscardPending(
            aether_preclamp_instr_v1::PendingDiscardReason::
                kGpuFailureOrFallback);
        aether::tools::DawnKernelHarness::gpu_ts_clear_caller_thread_frame();
        aether_sed_clear_last_stages();
        if (deterministic_selection_requested) return kCanonicalGpuUnavailable;
                                                                                 
                                                                                       
                                                                                               
                                                                             
                                                                          
                                                                                          
                                                                                      
                                                                                                         
                                                                                   
                                                                  
        {
                                                                                               
                                                                                                
                                                                                   
            const int fallback_mode = [] {
                const char* e =
                    std::getenv("OFFICIAL_AETHER_EXTRACT_CPU_FALLBACK");
                if (e && e[0] == '1' && e[1] == '\0') return 1;
                if (e && e[0] == '0' && e[1] == '\0') return 0;
                return 2;                             
            }();
            const long long px =
                static_cast<long long>(width) * static_cast<long long>(height);
            const bool allowed =
                fallback_mode == 1 || (fallback_mode == 2 && px <= 4000000ll);
            if (!allowed) {
                std::fprintf(stderr,
                             "[DspSiftGpu] GPU extract failed; CPU fallback "
                             "blocked for %dx%d (OOM guard, status 2 -> "
                             "frame drop)\n",
                             width, height);
                return kCanonicalGpuUnavailable;
            }
        }
        return aether_dsp_sift_extract_threaded_v2(
            gray, width, height, max_features, num_threads, out_xy, out_desc,
            out_scales, out_orientations, out_cap, out_count);
    };

#if defined(AETHER_PRECLAMP_INSTR_TEST_HOOKS)
    if (g_force_next_gpu_fallback_for_test) {
        g_force_next_gpu_fallback_for_test = false;
        return fallback();
    }
#endif

    try {
                                                                             
                                                                                 
        std::lock_guard<std::mutex> gpu_lock(g_gpu_harness_mu);
                                                                                       
                                                                                                
                                                                                       
                                                                                               
        for (int attempt = 0; attempt < 2; ++attempt) {
        aether::tools::DawnKernelHarness* harness_ptr = acquire_gpu_harness();
        if (harness_ptr == nullptr) return fallback();
        aether::tools::DawnKernelHarness& harness = *harness_ptr;

        const int max_num0 = max_features > 0 ? max_features : 8192;
        aether::tools::SiftExtractDawn extractor;
        aether::tools::SiftExtractDawn::Result res;
                                                                          
                                                                                   
                                                                                
                                                                              
                                                                                 
        if (!extractor.extract(harness, gray, width, height, max_num0, &res)) {
            if (attempt == 0) {
                harness.mark_unhealthy();                                 
                continue;
            }
            return fallback();
        }
        if (res.count == 0) {
            aether_preclamp_instr_v1::DiscardPending(
                aether_preclamp_instr_v1::PendingDiscardReason::kZeroCandidate);
            if (out_count) *out_count = 0;
            harness.gpu_ts_stash_frame_for_caller_thread();
            return 0;
        }
        const bool deterministic_selection =
            res.selection_policy !=
            aether::sfm::FeatureSelectionPolicyV1::kLegacyColmapGroup;
        if (deterministic_selection_requested != deterministic_selection ||
            selection_policy.policy != res.selection_policy) {
            return fallback();
        }
        if (deterministic_selection) {
            aether_preclamp_instr_v1::DiscardPending(
                aether_preclamp_instr_v1::PendingDiscardReason::
                    kCanonicalRoute);
            if (res.stable_ids.size() !=
                static_cast<size_t>(res.count)) {
                return fallback();
            }
            for (size_t rank = 0; rank < res.stable_ids.size(); ++rank) {
                if (res.stable_ids[rank] != rank) {
                    return fallback();
                }
            }
        }

        const int max_num =
            max_features > 0 ? max_features : 8192;

                                                                                    
                                                                                
                                                                              
                                                      
        const int K = res.count;
        std::vector<int> idx(K);
        if (deterministic_selection) {
                                                                             
                                                                      
            std::iota(idx.begin(), idx.end(), 0);
        } else {
            for (int i = 0; i < K; ++i) idx[i] = i;
            std::sort(idx.begin(), idx.end(), [&](int a, int b) {
                if (res.octave[a] != res.octave[b])
                    return res.octave[a] > res.octave[b];
                return res.scale[a] > res.scale[b];
            });
        }

                                                                        
        const int kMaxOctaveResolution = 1000;
        int prev_os = INT32_MAX;
        int emitted = 0;
        for (int n = 0; n < K && emitted < out_cap; ++n) {
            const int i = idx[n];
            if (out_xy) {
                out_xy[2 * emitted] = res.xy[2 * i] + 0.5f;                         
                out_xy[2 * emitted + 1] = res.xy[2 * i + 1] + 0.5f;
            }
                                                                              
                                                                               
            if (out_scales) out_scales[emitted] = res.kp_scale[i];
            if (out_orientations) out_orientations[emitted] = res.kp_orientation[i];
            if (out_desc) {
                finish_descriptor(res.raw_desc.data() +
                                      static_cast<size_t>(i) * 128,
                                  out_desc + static_cast<size_t>(emitted) * 128);
            }
            ++emitted;

            const int os = res.octave[i] * kMaxOctaveResolution + res.scale[i];
            if (os != prev_os && emitted >= max_num) break;
            prev_os = os;
        }
        if (out_count) *out_count = emitted;
        if (!deterministic_selection_requested && !deterministic_selection) {
            double gpu_stage_ms[9] = {0.0};
            aether_sed_last_stages_gpu(gpu_stage_ms, 9);
            double gpu_ms = 0.0;
            bool valid_gpu_ms = true;
            for (double stage_ms : gpu_stage_ms) {
                valid_gpu_ms = valid_gpu_ms && std::isfinite(stage_ms) &&
                               stage_ms >= 0.0;
                gpu_ms += stage_ms;
            }
            valid_gpu_ms = valid_gpu_ms && gpu_ms > 0.0;
            (void)aether_preclamp_instr_v1::SealAcceptedLegacyGpuResult(
                static_cast<uint32_t>(res.count),
                valid_gpu_ms
                    ? aether_preclamp_instr_v1::FieldStatus::kValid
                    : aether_preclamp_instr_v1::FieldStatus::kUnavailable,
                valid_gpu_ms ? static_cast<float>(gpu_ms) : 0.0f);
        }
                                                                                
                                                                                 
                                                                                
                                                                               
                                                                                 
        if (const char* home = std::getenv("HOME")) {
            const std::string path =
                std::string(home) + "/Documents/gpu_sift_feat.log";
            if (FILE* lf = std::fopen(path.c_str(), "a")) {
                std::fprintf(
                    lf,
                    "feat_raw=%d kept=%d %dx%d selector_policy=%u "
                    "selector_reason=%u\n",
                    K, emitted, width, height,
                    static_cast<uint32_t>(res.selection_policy),
                    static_cast<uint32_t>(res.selection_policy_reason));
                std::fclose(lf);
            }
        }
        harness.gpu_ts_stash_frame_for_caller_thread();
        return 0;
        }                                                   
        return fallback();                                                      
    } catch (...) {
                                                                                       
                                                                  
        if (g_gpu_harness != nullptr) g_gpu_harness->mark_unhealthy();
        return fallback();
    }
}

                                                                             
                                                         
int aether_dsp_sift_extract_gpu(const uint8_t* gray, int width, int height,
                                int max_features, int num_threads, float* out_xy,
                                uint8_t* out_desc, int out_cap, int* out_count) {
    return aether_dsp_sift_extract_gpu_v2(
        gray, width, height, max_features, num_threads, out_xy, out_desc,
                       nullptr,                      nullptr, out_cap,
        out_count);
}

                                                                     
                                                                             
                                                                
  
                                                                                                           
                                  
                                                                                        
                                                                                              
                                                     
                                                                                                   
                                                           
                                                                                                 
int aether_dsp_sift_extract_gpu_warmup(double* out_init_ms,
                                       double* out_compile_ms,
                                       double* out_msl_ms, unsigned* out_msl_n,
                                       double* out_pso_ms, unsigned* out_pso_n) {
    if (out_init_ms) *out_init_ms = 0.0;
    if (out_compile_ms) *out_compile_ms = 0.0;
    if (out_msl_ms) *out_msl_ms = 0.0;
    if (out_msl_n) *out_msl_n = 0u;
    if (out_pso_ms) *out_pso_ms = 0.0;
    if (out_pso_n) *out_pso_n = 0u;
    try {
        std::lock_guard<std::mutex> gpu_lock(g_gpu_harness_mu);
        const auto t0 = std::chrono::steady_clock::now();
        aether::tools::DawnKernelHarness* harness_ptr = acquire_gpu_harness();
        const auto t1 = std::chrono::steady_clock::now();
        if (out_init_ms) {
            *out_init_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        }
        if (harness_ptr == nullptr) return -1;
        const size_t before = harness_ptr->pipeline_cache_size();
        aether::tools::SiftExtractDawn::warmup(*harness_ptr);
        const auto t2 = std::chrono::steady_clock::now();
        if (out_compile_ms) {
            *out_compile_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        }
        harness_ptr->dawn_histogram(aether::tools::kDawnHistMslLibrary, out_msl_ms, out_msl_n);
        harness_ptr->dawn_histogram(aether::tools::kDawnHistPipelineState, out_pso_ms, out_pso_n);
        return static_cast<int>(harness_ptr->pipeline_cache_size() - before);
    } catch (...) {
                                                                               
        return -1;
    }
}

}               
