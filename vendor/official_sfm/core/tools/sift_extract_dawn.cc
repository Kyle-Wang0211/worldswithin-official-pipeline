#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#include "sift_extract_dawn.h"
#include "feature_selection_environment_v1.h"
#if defined(AETHER_FEATURE_SELECTION_ENV_OFFICIAL)
#define AETHER_PRECLAMP_INSTR_ENV_OFFICIAL 1
#elif defined(AETHER_FEATURE_SELECTION_ENV_SELFTEST)
#define AETHER_PRECLAMP_INSTR_ENV_SELFTEST 1
#else
#error "preclamp instr: feature-selection environment namespace is absent"
#endif
#include "../official_pipeline/src/official_preclamp_instr_v1.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <utility>

#ifdef AETHER_WGSL_DIR
#include <fstream>
#include <sstream>
#else
#include "aether/shaders/wgsl_sources.h"
#include <string_view>
#endif

                                                                                       
                                                                                           
                                                                                              
                                                                      
static thread_local char g_aether_sed_fail_reason[192] = {0};
extern "C" const char* aether_sed_last_fail_reason() {
    return g_aether_sed_fail_reason;
}
static void sed_set_fail_reason(const char* what) {
    std::snprintf(g_aether_sed_fail_reason, sizeof(g_aether_sed_fail_reason),
                  "%s", what);
}

namespace aether {
namespace tools {

namespace {

                                                                              
                                                                    
                                                                              
                                                                              
std::string load_wgsl(const char* filename) {
                                                                                                                                           
                                                                                                                                                                    
    if (const char* d = std::getenv("OFFICIAL_AETHER_EXTRACT_WGSL_DIR")) {
        std::string dir = d;
        if (dir.size() > 1 && dir[0] == '~' && dir[1] == '/') { if (const char* h = std::getenv("HOME")) dir = std::string(h) + dir.substr(1); }
        std::ifstream of(dir + "/" + filename, std::ios::binary);
        if (of) { std::ostringstream ss; ss << of.rdbuf(); std::fprintf(stderr, "[SiftExtractDawn] WGSL override %s/%s (%zu B)\n", dir.c_str(), filename, (size_t)ss.str().size()); return ss.str(); }
    }
#ifdef AETHER_WGSL_DIR
    const std::string path = std::string(AETHER_WGSL_DIR) + "/" + filename;
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "[SiftExtractDawn] cannot open WGSL: " << path << '\n';
        std::abort();
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
#else
    using namespace aether::shaders;
    const std::string_view f(filename);
    if (f == "sift_gray_to_f32.wgsl")         return std::string(sift_gray_to_f32_wgsl);
    if (f == "sift_gss_blur.wgsl")            return std::string(sift_gss_blur_wgsl);
    if (f == "sift_gss_resample.wgsl")        return std::string(sift_gss_resample_wgsl);
    if (f == "sift_dog_detect.wgsl")          return std::string(sift_dog_detect_wgsl);
                                                                                                  
                                                                                                      
                                                                                                
                                                                                    
    if (f == "sift_dog_detect_compact.wgsl")  return std::string(sift_dog_detect_compact_wgsl);
    if (f == "sift_dog_refine.wgsl")          return std::string(sift_dog_refine_wgsl);
    if (f == "sift_dog_cand_reset.wgsl")      return std::string(sift_dog_cand_reset_wgsl);
    if (f == "sift_nonextrema_suppress.wgsl") return std::string(sift_nonextrema_suppress_wgsl);
    if (f == "sift_suppress_grid_count.wgsl") return std::string(sift_suppress_grid_count_wgsl);
    if (f == "sift_suppress_grid_scan.wgsl") return std::string(sift_suppress_grid_scan_wgsl);
    if (f == "sift_suppress_grid_scatter.wgsl") return std::string(sift_suppress_grid_scatter_wgsl);
    if (f == "sift_suppress_grid.wgsl") return std::string(sift_suppress_grid_wgsl);
    if (f == "sift_affine_shape.wgsl")        return std::string(sift_affine_shape_wgsl);
    if (f == "sift_orientation.wgsl")         return std::string(sift_orientation_wgsl);
    if (f == "sift_orientation_atomic.wgsl")
        return std::string(sift_orientation_atomic_wgsl);
    if (f == "sift_dsp_descriptor.wgsl")      return std::string(sift_dsp_descriptor_wgsl);
    if (f == "sift_dsp_descriptor_f16.wgsl")  return std::string(sift_dsp_descriptor_f16_wgsl);
    if (f == "sift_dsp_descriptor_f16_atomic.wgsl")
        return std::string(sift_dsp_descriptor_f16_atomic_wgsl);
    if (f == "sift_dsp_descriptor_par.wgsl")  return std::string(sift_dsp_descriptor_par_wgsl);
    if (f == "sift_dsp_mean.wgsl")            return std::string(sift_dsp_mean_wgsl);
    std::cerr << "[SiftExtractDawn] unknown WGSL pass: " << filename << '\n';
    std::abort();
#endif
}

                                                                                       
                                                                                                       
                                                                                        
                                                                                   
                                                                                        
                                                                                                
                                                                                       
                                                                                                   
                                                                              
                                                                                                  
                                                                                               
static bool kp_bind_subrange() {
    if (const char* v = std::getenv("OFFICIAL_AETHER_W256_KPBIND")) {
        return !(v[0] == '0' && v[1] == '\0');
    }
    return SiftPyramidDawn::w256_bind_enabled();
}

static DawnKernelHarness::BufBinding w256_kp(const wgpu::Buffer& b,
                                             const SiftPyramidDawn& pyr) {
                                                                                                    
    if (SiftPyramidDawn::splitbuf_enabled()) {
        SiftPyramidDawn::note_kp_bind(3);                                  
        return DawnKernelHarness::BufBinding(b);
    }
                                                                               
    if (SiftPyramidDawn::kpbuf_enabled() && pyr.keypoint_buffer_ready()) {
        SiftPyramidDawn::note_kp_bind(2);                         
        return DawnKernelHarness::BufBinding(pyr.keypoint_buffer());
    }
    const bool sub = kp_bind_subrange();
    SiftPyramidDawn::note_kp_bind(sub ? 1 : 0);
    if (!sub) return DawnKernelHarness::BufBinding(b);
    return DawnKernelHarness::BufBinding(
        b, 0u, static_cast<uint64_t>(pyr.keypoint_region_end()) * 4u);
}

                                                           
                                                                                                        
                                                                                                           
                                                                                                            
                                                                                                   
bool suppress_grid_on() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_SUPPRESS_GRID");
        return v == nullptr || !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

bool orient_atomic_on() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_ORIENT_ATOMIC");
        return v == nullptr || !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

bool desc_atomic_on() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_DESC_ATOMIC");
        return v == nullptr || !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

                                                                                   
                                                                                                               
                                                                                          
                                                                  
                                                                                                 
                                                                                                         
                                                                                                                  
                                                                                            
                                                                                                                    
bool dog_compact_on() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_DOG_COMPACT");
        return v != nullptr && !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

                                                                                                    
                                                                                                                  
                                                                                                           
                                                                                                            
                                                                                                                     
                                                                                       
                                                                                            

                                                                                                
                                                                                               
const char* desc_shader_name(const DawnKernelHarness& harness) {
    const bool want_f16 =
        (harness.has_f16() && std::getenv("SED_FORCE_F32") == nullptr) ||
        std::getenv("SED_F16_DESC") != nullptr;
    if (!want_f16) return "sift_dsp_descriptor.wgsl";
    return desc_atomic_on() ? "sift_dsp_descriptor_f16_atomic.wgsl"
                            : "sift_dsp_descriptor_f16.wgsl";
}

                                                      
std::vector<uint32_t> read_u32(DawnKernelHarness& h, const wgpu::Buffer& b,
                               size_t n) {
    const size_t bytes = n * sizeof(uint32_t);
    wgpu::Buffer st = h.alloc_staging_for_readback(bytes);
    h.copy_to_staging(b, st, bytes);
    std::vector<uint8_t> raw = h.readback(st, bytes);
                                                                                   
                                                                                                 
                                                                           
                                                                                    
                                                                                    
                                                                                             
    if (raw.size() < bytes) {
        sed_set_fail_reason("readback_short_or_timeout");
        throw std::runtime_error(
            "[SiftExtractDawn] GPU readback short/failed (timeout or device "
            "loss) — aborting this frame's extraction");
    }
    std::vector<uint32_t> out(n);
    std::memcpy(out.data(), raw.data(), bytes);
    return out;
}

}              

aether::sfm::FeatureSelectionPolicyDecisionV1
SiftExtractDawn::feature_selection_policy() {
    return aether::sfm::ParseFeatureSelectionPolicyV1(std::getenv(
        detail::kFeatureSelectionPolicyEnvironmentV1));
}

                                                                             
                                                                          
                                                                             
                                                                           
                                                                        
static constexpr int kAetherSedStageCount = 9;
static thread_local double g_aether_sed_stage_ms[kAetherSedStageCount] = {0};

extern "C" void aether_sed_last_stages(double* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    const int n = cap < kAetherSedStageCount ? cap : kAetherSedStageCount;
    for (int i = 0; i < n; ++i) out[i] = g_aether_sed_stage_ms[i];
}

                                                            
  
                                                                            
                                                                              
                                                                             
                                                                               
                                                                               
                                                                            
                                                                          
  
                                                                                  
                                                                       
static thread_local double
    g_aether_sed_stage_gpu_ms[kAetherSedStageCount] = {0};

extern "C" void aether_sed_last_stages_gpu(double* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    const int n = cap < kAetherSedStageCount ? cap : kAetherSedStageCount;
    for (int i = 0; i < n; ++i) out[i] = g_aether_sed_stage_gpu_ms[i];
}

                                                                                                   
                                                                                   
                                                                          
                                                                        
static thread_local double
    g_aether_sed_stage_hb[kAetherSedStageCount][7] = {{0}};

extern "C" void aether_sed_last_stages_hb(double* out, int cap) {
    if (out == nullptr || cap <= 0) return;
    const int n = cap < kAetherSedStageCount * 7 ? cap : kAetherSedStageCount * 7;
    for (int i = 0; i < n; ++i) out[i] = g_aether_sed_stage_hb[i / 7][i % 7];
}

extern "C" void aether_sed_clear_last_stages() {
    for (int i = 0; i < kAetherSedStageCount; ++i) {
        g_aether_sed_stage_ms[i] = 0.0;
        g_aether_sed_stage_gpu_ms[i] = 0.0;
    }
}


extern "C" void aether_pyr_persist_end_frame();

                                                                                  
  
                                                                                                
                                                                                        
                                                                                           
                                                                                      
                                                                           
                                                                                 
                                                                                                          
                                                              
  
                                         
                                                                          
                                                                                       
                                                                                   
                                                                                                
                                                                                                
                                                                                                           
                                                                                                    
                                                                                                            
  
                                                    
                                                                               
                                                                                                       
                                                                                    
                                                                                      
void SiftExtractDawn::warmup(DawnKernelHarness& harness) {
    SiftPyramidDawn::warmup(harness);                                             
    harness.load_compute(load_wgsl("sift_dog_detect.wgsl"));               
    if (suppress_grid_on()) {                                                          
        harness.load_compute(load_wgsl("sift_suppress_grid_count.wgsl"));
        harness.load_compute(load_wgsl("sift_suppress_grid_scan.wgsl"));
        harness.load_compute(load_wgsl("sift_suppress_grid_scatter.wgsl"));
        harness.load_compute(load_wgsl("sift_suppress_grid.wgsl"));
    } else {
        harness.load_compute(load_wgsl("sift_nonextrema_suppress.wgsl"));
    }
    harness.load_compute(load_wgsl("sift_affine_shape.wgsl"));               
    harness.load_compute(load_wgsl(orient_atomic_on()                        
                                       ? "sift_orientation_atomic.wgsl"
                                       : "sift_orientation.wgsl"));
    harness.load_compute(load_wgsl(desc_shader_name(harness)));                 
}

bool SiftExtractDawn::extract(DawnKernelHarness& harness, const uint8_t* gray,
                              int width, int height, int max_features,
                              Result* out) {
    if (out == nullptr) return false;
    out->xy.clear();
    out->octave.clear();
    out->scale.clear();
    out->kp_scale.clear();
    out->kp_orientation.clear();
    out->raw_desc.clear();
    out->stable_ids.clear();
    out->count = 0;
    const aether::sfm::FeatureSelectionPolicyDecisionV1 selection_decision =
        feature_selection_policy();
    out->selection_policy = selection_decision.policy;
    out->selection_policy_reason = selection_decision.reason;
    if (gray == nullptr || width < 2 || height < 2) return false;
    const bool canonical_selection =
        selection_decision.policy ==
        aether::sfm::FeatureSelectionPolicyV1::kCanonicalExact8192;
    const bool coverage_selection =
        selection_decision.policy ==
        aether::sfm::FeatureSelectionPolicyV1::kCoverageExact8192;

                                                                                      
                                                                                    
                                                                                                          
                                                                                        
    DawnKernelHarness::take_device_error(nullptr);
    g_aether_sed_fail_reason[0] = '\0';                                          
    const auto dawn_failed = [](const char* where) {
        std::string err;
        if (!DawnKernelHarness::take_device_error(&err)) {
            return false;
        }
        std::cerr << "[SiftExtractDawn] Dawn device error at " << where << ": "
                  << err << " (fallback)\n";
        sed_set_fail_reason(
            (std::string("dawn_error@") + where + ": " + err).c_str());
        return true;
    };

                                                                              
                                                                            
                                                                              
                                                  
    const bool timing = std::getenv("SED_TIMING") != nullptr;
    const bool observe_timing = timing || harness.gpu_ts_enabled();
    aether_sed_clear_last_stages();
    int sed_idx = 0;
                                                                             
                                                                                
                                                                                
                                                        
    harness.gpu_ts_reset();
    harness.host_breakdown_reset();                           
    uint32_t ts_bounds[kAetherSedStageCount] = {0};
                                                                               
                                                                                
                                                                            
                                                   
    double hb_stage[kAetherSedStageCount][7] = {{0}};                                      
    auto hb_snap = harness.host_breakdown();
    std::chrono::high_resolution_clock::time_point t_prev{};
    if (observe_timing) {
        t_prev = std::chrono::high_resolution_clock::now();
    }
    auto mark = [&](const char* label) {
        if (!observe_timing) return;
        auto now = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(now - t_prev).count();
        if (sed_idx < kAetherSedStageCount) {
            ts_bounds[sed_idx] = harness.gpu_ts_pass_count();             
            const auto& h = harness.host_breakdown();                      
            hb_stage[sed_idx][0] = h.upload_ms - hb_snap.upload_ms;
            hb_stage[sed_idx][1] = h.encode_ms - hb_snap.encode_ms;
            hb_stage[sed_idx][2] = h.submit_wait_ms - hb_snap.submit_wait_ms;
            hb_stage[sed_idx][3] = h.map_ms - hb_snap.map_ms;
            hb_stage[sed_idx][4] = h.create_ms - hb_snap.create_ms;
            hb_stage[sed_idx][5] = double(h.n_submit) - double(hb_snap.n_submit);
            hb_stage[sed_idx][6] = double(h.n_copy) - double(hb_snap.n_copy);
            for (int c = 0; c < 7; ++c)                                      
                g_aether_sed_stage_hb[sed_idx][c] = hb_stage[sed_idx][c];
            hb_snap = h;
            g_aether_sed_stage_ms[sed_idx++] = ms;
        }
        if (timing) {
            std::printf("  [SED] %-22s %.1f ms\n", label, ms);
            std::fflush(stdout);
        }
        t_prev = now;
    };

                                                                      
    SiftPyramidDawn pyr;
    if (!pyr.build(harness, gray, width, height)) {
        std::cerr << "[SiftExtractDawn] pyramid build failed\n";
        sed_set_fail_reason("pyramid_build_failed");
        return false;
    }
    mark("pyramid build");
    std::vector<SiftPyramidDawn::LevelMeta> meta;
    wgpu::Buffer packed = pyr.pack_levels(harness, &meta);
    wgpu::Buffer meta_buf =
        harness.upload(meta.data(), meta.size() * sizeof(SiftPyramidDawn::LevelMeta),
                       wgpu::BufferUsage::Storage);
    if (kp_bind_subrange() &&
        pyr.keypoint_region_end() > SiftPyramidDawn::window_elems()) {
                                                                                              
                                                                                       
        sed_set_fail_reason("w256_keypoint_region_exceeds_256mb");
        return false;
    }
                                                                                                     
                                                                                           
    pyr.sync_keypoint_buffer(harness);
    mark("pack_levels");

    const float base_scale = static_cast<float>(SiftPyramidDawn::base_scale());
    const float peak = static_cast<float>(peak_threshold());

                                                                                                                                          
    uint32_t zero = 0u;
    wgpu::Buffer det_counter = harness.upload(
        &zero, 4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    std::vector<uint32_t> det_init(static_cast<size_t>(kDetectCap) * kKpStride, 0u);
    wgpu::Buffer det_buf = harness.upload(
        det_init.data(), det_init.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
                                                                                                                
                                                                                                             
    const bool compact = dog_compact_on();
    wgpu::Buffer cand_buf, cand_counter;
    if (compact) {
        std::vector<uint32_t> cand_init(static_cast<size_t>(kDetectCap) * 4u, 0u);
        cand_buf = harness.upload(cand_init.data(), cand_init.size() * sizeof(uint32_t),
                                  wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
        cand_counter = harness.upload(&zero, 4,
                                      wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    }

    struct DetParams {
        uint32_t width, height, cap, octave;
        float peak_threshold, edge_threshold, base_scale, oct_resolution;
                                                                                     
        uint32_t off[6];
        uint32_t z0, _pad1;                                              
    };
    wgpu::ComputePipeline pipe_detect =
        harness.load_compute(load_wgsl("sift_dog_detect.wgsl"));
    wgpu::ComputePipeline pipe_detect_c, pipe_refine, pipe_creset;
    if (compact) {
        pipe_detect_c = harness.load_compute(load_wgsl("sift_dog_detect_compact.wgsl"));
        pipe_refine   = harness.load_compute(load_wgsl("sift_dog_refine.wgsl"));
        pipe_creset   = harness.load_compute(load_wgsl("sift_dog_cand_reset.wgsl"));
    }
                                                                            
                                                                              
                                                                  
                                                                            
    harness.begin_batch();
    for (int o = pyr.first_octave(); o <= pyr.last_octave(); ++o) {
        const int ow = pyr.octave_width(o);
        const int oh = pyr.octave_height(o);
        if (ow < 3 || oh < 3) continue;
        DetParams P{static_cast<uint32_t>(ow), static_cast<uint32_t>(oh),
                    kDetectCap, static_cast<uint32_t>(o), peak,
                    static_cast<float>(kEdgeThreshold), base_scale,
                    static_cast<float>(kOctaveResolution),
                    {0, 0, 0, 0, 0, 0}, 0, 0};
        for (int s = SiftPyramidDawn::kOctaveFirstSub;
             s <= SiftPyramidDawn::kOctaveLastSub; ++s)
            P.off[s - SiftPyramidDawn::kOctaveFirstSub] = pyr.level_offset(o, s);
                                                                                       
                                                                                  
                                                                               
                                                                                              
        using BB = DawnKernelHarness::BufBinding;
        const uint32_t n_oct =
            static_cast<uint32_t>(ow) * static_cast<uint32_t>(oh);
        struct Part { int z0, zn, l0, l1; };
        std::vector<Part> parts;
        {
                                                                                          
                                                                                        
            const auto span = [&](int l0, int l1) {
                uint32_t lo = P.off[l0], hi = P.off[l0] + n_oct;
                for (int k = l0; k <= l1; ++k) {
                    lo = std::min(lo, P.off[k]);
                    hi = std::max(hi, P.off[k] + n_oct);
                }
                return hi - lo;
            };
                                                                                                   
            if (SiftPyramidDawn::splitbuf_enabled() ||
                !SiftPyramidDawn::w256_bind_enabled() ||
                span(0, 5) <= SiftPyramidDawn::window_elems()) {
                parts = {{0, 3, 0, 5}};
            } else {
                for (int zc = 1; zc <= 3; ++zc) {
                    if (span(zc - 1, zc + 2) > SiftPyramidDawn::window_elems()) {
                        sed_set_fail_reason("w256_detect_span_exceeds_256mb");
                        return false;
                    }
                    parts.push_back({zc - 1, 1, zc - 1, zc + 2});
                }
            }
        }
        const bool w256 = SiftPyramidDawn::w256_bind_enabled();
        for (const Part& pt : parts) {
            DetParams Q = P;
            uint32_t lo = P.off[pt.l0], hi = P.off[pt.l0] + n_oct;
            for (int k = pt.l0; k <= pt.l1; ++k) {
                lo = std::min(lo, P.off[k]);
                hi = std::max(hi, P.off[k] + n_oct);
            }
            if (!w256) lo = 0u;                                              
            const bool split = SiftPyramidDawn::splitbuf_enabled();
            if (split) lo = 0u;                                            
            for (int k = 0; k < 6; ++k)
                Q.off[k] = (k >= pt.l0 && k <= pt.l1) ? P.off[k] - lo : 0u;
            Q.z0 = static_cast<uint32_t>(pt.z0);
            wgpu::Buffer p_buf =
                harness.upload(&Q, sizeof(Q), wgpu::BufferUsage::Uniform);
            if (std::getenv("W256_DIAG"))
                std::fprintf(stderr, "[W256] detect o=%d z0=%d zn=%d bind=%.1fMB\n",
                             o, pt.z0, pt.zn, (hi - lo) * 4.0 / 1048576.0);
                                                                                                 
            const auto bind_gss = [&]() {
                return (w256 && !split) ? BB(packed, static_cast<uint64_t>(lo) * 4u,
                                             static_cast<uint64_t>(hi - lo) * 4u)
                                        : BB(packed);
            };
                                                                                      
                                                                                    
                                                                
            const auto bind_hi = [&]() {
                return split ? BB(pyr.packed_hi_buffer())
                             : (w256 ? BB(packed, static_cast<uint64_t>(lo) * 4u,
                                          static_cast<uint64_t>(hi - lo) * 4u)
                                     : BB(packed));
            };
            const uint32_t gx = static_cast<uint32_t>((ow + 7) / 8);
            const uint32_t gy = static_cast<uint32_t>((oh + 7) / 8);
            const uint32_t gz = static_cast<uint32_t>(pt.zn);
            if (!compact) {
                harness.dispatch_batched(pipe_detect,
                    {bind_gss(), BB(det_counter), BB(det_buf), BB(p_buf), bind_hi()},
                    gx, gy, gz);
            } else {
                                                                                                     
                                                                                                                   
                harness.dispatch_batched(pipe_creset, {BB(cand_counter)}, 1u, 1u, 1u);
                                                                                                      
                                                                                    
                harness.dispatch_batched(pipe_detect_c,
                    {bind_gss(), BB(p_buf), bind_hi(), BB(cand_buf), BB(cand_counter)},
                    gx, gy, gz);
                                                                                             
                                                                                             
                harness.dispatch_batched(pipe_refine,
                    {bind_gss(), BB(det_counter), BB(det_buf), BB(p_buf), bind_hi(),
                     BB(cand_buf), BB(cand_counter)},
                    (kDetectCap + 63u) / 64u, 1u, 1u);
                static bool compact_logged = false;
                if (!compact_logged) {
                    compact_logged = true;
                    std::fprintf(stderr,
                                 "[COMPACT] on=1 cand_cap=%u B段工作组=%u "
                                 "(A 段 %ux%ux%u)\n",
                                 kDetectCap, (kDetectCap + 63u) / 64u, gx, gy, gz);
                }
            }
        }
    }
    harness.end_batch();
    uint32_t n_detect = read_u32(harness, det_counter, 1)[0];
    mark("detect (all oct)");
                                                                                                   
                                                                           
    if (dawn_failed("pyramid/detect")) {
        return false;
    }
    if (n_detect > kDetectCap) {
        std::cerr << "[SiftExtractDawn] detect overflow " << n_detect << " > "
                  << kDetectCap << " (fallback)\n";
        char why[96];
        std::snprintf(why, sizeof(why), "detect_overflow n=%u cap=%u",
                      n_detect, kDetectCap);
        sed_set_fail_reason(why);
        return false;
    }
                                                                                            
                                                                                       
                                                                                 
                                                                                    
                                                                                      
                                                   
      
                                                                                   
                                                                                               
                                                                                   
    if (n_detect == 0) {
        out->count = 0;
        aether_preclamp_instr_v1::DiscardPending(
            aether_preclamp_instr_v1::PendingDiscardReason::kZeroCandidate);
        return true;
    }

                                                                                                                                           
    std::vector<uint32_t> keep_init(n_detect, 1u);
    wgpu::Buffer keep_buf = harness.upload(
        keep_init.data(), keep_init.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    {
        struct SupParams {
            uint32_t count;
            float tol;
        } sp{n_detect, static_cast<float>(kSuppressTol)};
        wgpu::Buffer sp_buf =
            harness.upload(&sp, sizeof(sp), wgpu::BufferUsage::Uniform);
                                                                                  
                                                                                                      
                                                                                      
                                                                                        
        if (suppress_grid_on()) {                                                                         
            constexpr uint32_t kCell = 16u;
            const uint32_t ncx = (static_cast<uint32_t>(width) + kCell - 1u) / kCell;
            const uint32_t ncy = (static_cast<uint32_t>(height) + kCell - 1u) / kCell;
            const uint32_t ncells = ncx * ncy;
            struct GridParams {
                uint32_t count; float tol; uint32_t width, height;
                uint32_t cell, ncx, ncy, ncells;
            } gp{n_detect, static_cast<float>(kSuppressTol),
                 static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                 kCell, ncx, ncy, ncells};
            wgpu::Buffer gp_buf =
                harness.upload(&gp, sizeof(gp), wgpu::BufferUsage::Uniform);
            std::vector<uint32_t> zeros(static_cast<size_t>(ncells) + 1u, 0u);
            wgpu::Buffer cell_count = harness.upload(
                zeros.data(), zeros.size() * sizeof(uint32_t),
                wgpu::BufferUsage::Storage);
            wgpu::Buffer cell_cursor = harness.upload(
                zeros.data(), zeros.size() * sizeof(uint32_t),
                wgpu::BufferUsage::Storage);
            wgpu::Buffer cell_start = harness.alloc(
                zeros.size() * sizeof(uint32_t), wgpu::BufferUsage::Storage);
            wgpu::Buffer cell_items = harness.alloc(
                static_cast<size_t>(n_detect) * sizeof(uint32_t),
                wgpu::BufferUsage::Storage);
            wgpu::ComputePipeline pipe_cnt =
                harness.load_compute(load_wgsl("sift_suppress_grid_count.wgsl"));
            wgpu::ComputePipeline pipe_scan =
                harness.load_compute(load_wgsl("sift_suppress_grid_scan.wgsl"));
            wgpu::ComputePipeline pipe_scat =
                harness.load_compute(load_wgsl("sift_suppress_grid_scatter.wgsl"));
            wgpu::ComputePipeline pipe_grid =
                harness.load_compute(load_wgsl("sift_suppress_grid.wgsl"));
            const uint32_t wgs = (n_detect + 63u) / 64u;
            harness.begin_batch();
            harness.dispatch_batched(pipe_cnt, {det_buf, cell_count, gp_buf}, wgs, 1u);
            harness.dispatch_batched(pipe_scan, {cell_count, cell_start, gp_buf}, 1u, 1u);
            harness.dispatch_batched(pipe_scat,
                                     {det_buf, cell_start, cell_cursor, cell_items, gp_buf},
                                     wgs, 1u);
            harness.dispatch_batched(pipe_grid,
                                     {det_buf, cell_start, cell_items, keep_buf, gp_buf},
                                     wgs, 1u);
            harness.end_batch();
        } else {
            wgpu::ComputePipeline pipe_sup =
                harness.load_compute(load_wgsl("sift_nonextrema_suppress.wgsl"));
            harness.dispatch(pipe_sup, {det_buf, keep_buf, sp_buf},
                             (n_detect + 63u) / 64u);
        }
    }

                                                                              
                                                                                
                                                                        
                                                                                     
                                                                                                
    const size_t det_bytes = static_cast<size_t>(n_detect) * kKpStride * sizeof(uint32_t);
    const size_t keep_bytes = static_cast<size_t>(n_detect) * sizeof(uint32_t);
    const size_t keep_off = (det_bytes + 255u) & ~static_cast<size_t>(255u);                            
    std::vector<uint32_t> det_recs(static_cast<size_t>(n_detect) * kKpStride);
    std::vector<uint32_t> keep(n_detect);
    {
        wgpu::Buffer st = harness.alloc_staging_for_readback(keep_off + keep_bytes);
        harness.begin_batch();
        harness.copy_region_batched(det_buf, 0, st, 0, det_bytes);
        harness.copy_region_batched(keep_buf, 0, st, keep_off, keep_bytes);
        harness.end_batch();
        std::vector<uint8_t> raw = harness.readback(st, keep_off + keep_bytes);
        if (raw.size() < keep_off + keep_bytes) {
            sed_set_fail_reason("readback_short_or_timeout");
            throw std::runtime_error("readback short (det+keep)");
        }
        std::memcpy(det_recs.data(), raw.data(), det_bytes);
        std::memcpy(keep.data(), raw.data() + keep_off, keep_bytes);
    }
    mark("suppress+readback");

    std::vector<uint32_t> aff_in;                                         
    aff_in.reserve(static_cast<size_t>(n_detect) * kKpStride);
    for (uint32_t i = 0; i < n_detect; ++i) {
        if (!keep[i]) continue;
        const uint32_t* r = det_recs.data() + static_cast<size_t>(i) * kKpStride;
        for (uint32_t w = 0; w < kKpStride; ++w) aff_in.push_back(r[w]);
    }
    uint32_t n_kept = static_cast<uint32_t>(aff_in.size() / kKpStride);
    if (dawn_failed("suppress")) {
        return false;
    }
    if (n_kept == 0) {
        out->count = 0;
        aether_preclamp_instr_v1::DiscardPending(
            aether_preclamp_instr_v1::PendingDiscardReason::kZeroCandidate);
        return true;
    }

                                                                                                          
                                                                                
                                                                                               
                                                                                              
                                                                                  
                                                                                        
                                                                         
                                                                                        
                                                                                         
                                                                                       
                                                                                       
                                                                                              
    std::vector<uint32_t> aff_in_full;
    uint32_t n_kept_full = n_kept;
    long long prune_validate_os = LLONG_MIN;                                   
    bool did_prune = false;
    {
        static const bool prune_on = [] {                          
            const char* v = std::getenv("OFFICIAL_AETHER_PRECLAMP_PRUNE");
            return v == nullptr || !(v[0] == '0' && v[1] == '\0');
        }();
        if (prune_on && !canonical_selection && !coverage_selection &&
            max_features > 0 && n_kept > static_cast<uint32_t>(max_features)) {
            constexpr long long kOsRes = 1000;
            std::vector<std::pair<long long, uint32_t>> hist;
            for (uint32_t i = 0; i < n_kept; ++i) {
                int o, s;
                std::memcpy(&o, &aff_in[static_cast<size_t>(i) * kKpStride + 5], 4);
                std::memcpy(&s, &aff_in[static_cast<size_t>(i) * kKpStride + 6], 4);
                const long long os = static_cast<long long>(o) * kOsRes + s;
                bool found = false;
                for (auto& e : hist) {
                    if (e.first == os) { ++e.second; found = true; break; }
                }
                if (!found) hist.emplace_back(os, 1u);
            }
            std::sort(hist.begin(), hist.end(),
                      [](const auto& a, const auto& b) { return a.first > b.first; });
            long long guard_os = LLONG_MIN;                                        
            unsigned long long cum = 0;
            for (size_t j = 0; j < hist.size(); ++j) {
                cum += hist[j].second;
                if (cum >= static_cast<unsigned long long>(max_features)) {
                    if (j + 1 < hist.size()) {
                        prune_validate_os = hist[j].first;
                        guard_os = hist[j + 1].first;
                    }
                    break;
                }
            }
            if (guard_os != LLONG_MIN) {
                std::vector<uint32_t> pruned;
                pruned.reserve(aff_in.size());
                for (uint32_t i = 0; i < n_kept; ++i) {
                    int o, s;
                    std::memcpy(&o, &aff_in[static_cast<size_t>(i) * kKpStride + 5], 4);
                    std::memcpy(&s, &aff_in[static_cast<size_t>(i) * kKpStride + 6], 4);
                    const long long os = static_cast<long long>(o) * kOsRes + s;
                    if (os >= guard_os) {
                        const uint32_t* r =
                            aff_in.data() + static_cast<size_t>(i) * kKpStride;
                        for (uint32_t w = 0; w < kKpStride; ++w) pruned.push_back(r[w]);
                    }
                }
                const uint32_t n_pruned =
                    static_cast<uint32_t>(pruned.size() / kKpStride);
                if (n_pruned < n_kept) {
                    aff_in_full.swap(aff_in);                                      
                    aff_in.swap(pruned);
                    n_kept = n_pruned;
                    did_prune = true;
                } else {
                    prune_validate_os = LLONG_MIN;
                }
            } else {
                prune_validate_os = LLONG_MIN;
            }
        }
    }

                                                                                 
                                                                                   
                                                                                          
    wgpu::Buffer ori_out_buf, ori_counter, dbg_buf;
    uint32_t n_oriented = 0;
    std::vector<uint32_t> ori_all;
    for (int prune_attempt = 0;; ++prune_attempt) {
    const bool prune_first_attempt = (prune_attempt == 0);

                                                                                                                              
    wgpu::Buffer aff_in_buf =
        harness.upload(aff_in.data(), aff_in.size() * sizeof(uint32_t),
                       wgpu::BufferUsage::Storage);
    std::vector<float> ell_init(static_cast<size_t>(n_kept) * 5, 0.0f);
    wgpu::Buffer ell_buf = harness.upload(
        ell_init.data(), ell_init.size() * sizeof(float),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    {
        struct AffParams {
            uint32_t count;
            int first_octave, last_octave, oct_res;
            float base_scale;
            int oct_first_sub, oct_last_sub;
            uint32_t levels_per_oct;
        } P{n_kept, 0, pyr.last_octave(), kOctaveResolution, base_scale,
            SiftPyramidDawn::kOctaveFirstSub, SiftPyramidDawn::kOctaveLastSub,
            static_cast<uint32_t>(SiftPyramidDawn::kLevelsPerOctave)};
        wgpu::Buffer p_buf =
            harness.upload(&P, sizeof(P), wgpu::BufferUsage::Uniform);
        wgpu::ComputePipeline pipe_aff =
            harness.load_compute(load_wgsl("sift_affine_shape.wgsl"));
                                                                                 
                                                                                        
                                                                 
        {
            using BB = DawnKernelHarness::BufBinding;
            harness.dispatch(pipe_aff,
                             {w256_kp(packed, pyr), BB(meta_buf), BB(aff_in_buf),
                              BB(ell_buf), BB(p_buf)},
                             std::min(n_kept, 65535u),
                             (n_kept + 65534u) / 65535u, 1u);
        }
    }
    std::vector<uint32_t> ell_raw =
        read_u32(harness, ell_buf, static_cast<size_t>(n_kept) * 5);
    if (prune_first_attempt) mark("affine+readback");
    if (dawn_failed("affine")) {
        return false;
    }

                                                                               
                                                                                
                                                                             
                                                                            
    std::vector<uint32_t> ori_in(static_cast<size_t>(n_kept) * kKpStride, 0u);
    bool nan_seen = false;
    for (uint32_t i = 0; i < n_kept; ++i) {
        const uint32_t* d = aff_in.data() + static_cast<size_t>(i) * kKpStride;
        const uint32_t* e = ell_raw.data() + static_cast<size_t>(i) * 5;
        float a11, a12, a21, a22;
        std::memcpy(&a11, &e[0], 4);
        std::memcpy(&a12, &e[1], 4);
        std::memcpy(&a21, &e[2], 4);
        std::memcpy(&a22, &e[3], 4);
        if (std::isnan(a11) || std::isnan(a12) || std::isnan(a21) ||
            std::isnan(a22)) {
            nan_seen = true;
            break;
        }
        uint32_t* r = ori_in.data() + static_cast<size_t>(i) * kKpStride;
        r[0] = d[0];      
        r[1] = d[1];      
        std::memcpy(&r[2], &a11, 4);
        std::memcpy(&r[3], &a12, 4);
        std::memcpy(&r[4], &a21, 4);
        std::memcpy(&r[5], &a22, 4);
        r[6] = d[5];                           
        r[7] = d[6];                      
    }
    if (nan_seen) {
        std::cerr << "[SiftExtractDawn] NaN affine ellipse (fallback)\n";
        sed_set_fail_reason("nan_affine");
        return false;
    }

                                                                                                                                     
    wgpu::Buffer ori_in_buf =
        harness.upload(ori_in.data(), ori_in.size() * sizeof(uint32_t),
                       wgpu::BufferUsage::Storage);
    ori_counter = harness.upload(
        &zero, 4, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    std::vector<uint32_t> ori_out_init(static_cast<size_t>(kOrientCap) * kKpStride,
                                       0u);
    ori_out_buf = harness.upload(
        ori_out_init.data(), ori_out_init.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
    std::vector<float> dbg_init(static_cast<size_t>(kOrientCap) * 2, 0.0f);
    wgpu::BufferUsage dbg_usage = wgpu::BufferUsage::Storage;
    if (canonical_selection || coverage_selection) {
        dbg_usage |= wgpu::BufferUsage::CopySrc;
    }
    dbg_buf =
        harness.upload(dbg_init.data(), dbg_init.size() * sizeof(float),
                       dbg_usage);
    {
        struct OriParams {
            uint32_t count;
            int fo, lo, res;
            float base;
            int ofs, ols;
            uint32_t lpo;
        } P{n_kept, 0, pyr.last_octave(), kOctaveResolution, base_scale,
            SiftPyramidDawn::kOctaveFirstSub, SiftPyramidDawn::kOctaveLastSub,
            static_cast<uint32_t>(SiftPyramidDawn::kLevelsPerOctave)};
        wgpu::Buffer p_buf =
            harness.upload(&P, sizeof(P), wgpu::BufferUsage::Uniform);
                                                                                     
                                                                   
        wgpu::ComputePipeline pipe_ori = harness.load_compute(
            load_wgsl(orient_atomic_on() ? "sift_orientation_atomic.wgsl"
                                         : "sift_orientation.wgsl"));
                                                                              
        {
            using BB = DawnKernelHarness::BufBinding;
            harness.dispatch(pipe_ori,
                             {w256_kp(packed, pyr), BB(meta_buf), BB(ori_in_buf),
                              BB(ori_out_buf), BB(ori_counter), BB(dbg_buf),
                              BB(p_buf)},
                             std::min(n_kept, 65535u),
                             (n_kept + 65534u) / 65535u, 1u);
        }
    }
    n_oriented = read_u32(harness, ori_counter, 1)[0];
    if (std::getenv("W256_DIAG")) {
        std::fprintf(stderr,
                     "[W256] n_detect=%u n_kept=%u n_oriented=%u "
                     "kp_region=%.1fMB packed_total=%.1fMB (bind cap 256MB)\n",
                     static_cast<unsigned>(n_detect), static_cast<unsigned>(n_kept),
                     static_cast<unsigned>(n_oriented),
                     pyr.keypoint_region_end() * 4.0 / 1048576.0,
                     pyr.packed_total_elems() * 4.0 / 1048576.0);
    }
    if (prune_first_attempt) mark("orient (1->K)");
    if (dawn_failed("orientation")) {
        return false;
    }
    if (std::getenv("SED_DEBUG")) {
        std::cerr << "[SiftExtractDawn] n_detect=" << n_detect
                  << " n_kept(suppress)=" << n_kept
                  << " n_oriented=" << n_oriented << "\n";
    }
    if (n_oriented > kOrientCap) {
        std::cerr << "[SiftExtractDawn] orient overflow " << n_oriented << " > "
                  << kOrientCap << " (fallback)\n";
        char why[96];
        std::snprintf(why, sizeof(why), "orient_overflow n=%u cap=%u",
                      n_oriented, kOrientCap);
        sed_set_fail_reason(why);
        return false;
    }
    if (n_oriented == 0) {
        if (did_prune) {
                                                                                         
                                                    
            std::cerr << "[PRECLAMP-PRUNE] pruned set yielded 0 orientations "
                         "— rerunning full set\n";
            aff_in.swap(aff_in_full);
            n_kept = n_kept_full;
            did_prune = false;
            continue;
        }
        aether_preclamp_instr_v1::DiscardPending(
            aether_preclamp_instr_v1::PendingDiscardReason::kZeroCandidate);
        return true;
    }
    ori_all = read_u32(
        harness, ori_out_buf, static_cast<size_t>(n_oriented) * kKpStride);
    if (!did_prune || prune_validate_os == LLONG_MIN) break;
    {
                                                                                       
                                                                                             
                                                                                                  
        constexpr long long kOsResV = 1000;
        unsigned long long coarse_ori = 0;
        for (uint32_t i = 0; i < n_oriented; ++i) {
            int o, s;
            std::memcpy(&o, &ori_all[static_cast<size_t>(i) * kKpStride + 6], 4);
            std::memcpy(&s, &ori_all[static_cast<size_t>(i) * kKpStride + 7], 4);
            if (static_cast<long long>(o) * kOsResV + s >= prune_validate_os)
                ++coarse_ori;
        }
        if (coarse_ori >= static_cast<unsigned long long>(max_features)) break;
        std::cerr << "[PRECLAMP-PRUNE] validation failed coarse_ori="
                  << coarse_ori << " < max_features=" << max_features
                  << " — rerunning full set\n";
        aff_in.swap(aff_in_full);
        n_kept = n_kept_full;
        did_prune = false;
    }
    }                        
    aether_preclamp_instr_v1::BeginLegacyClamp(n_oriented);
                                                                            
                                                                          
                                                                               
    const int requested_max_features = max_features;
    if (canonical_selection) {
        max_features = 0;
    }

                                                                                                                                 
                                                                                 
                                                                                  
                                                                                     
                                                                                  
                                                                                
                                                                         
                                                                                            
    uint32_t n_desc = n_oriented;
    wgpu::Buffer desc_in_buf = ori_out_buf;
    if (max_features > 0 && n_oriented > static_cast<uint32_t>(max_features)) {
        std::vector<int> idx(n_oriented);
        for (uint32_t i = 0; i < n_oriented; ++i) idx[i] = static_cast<int>(i);
        auto oct_of = [&](int i) {
            int o; std::memcpy(&o, &ori_all[static_cast<size_t>(i) * kKpStride + 6], 4); return o;
        };
        auto scl_of = [&](int i) {
            int s; std::memcpy(&s, &ori_all[static_cast<size_t>(i) * kKpStride + 7], 4); return s;
        };
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            if (oct_of(a) != oct_of(b)) return oct_of(a) > oct_of(b);
            return scl_of(a) > scl_of(b);
        });
                                                                                 
                                                
        constexpr int kMaxOctaveResolution = 1000;
        int prev_os = INT32_MAX;
        std::vector<uint32_t> clamped;
        clamped.reserve(static_cast<size_t>(max_features + 64) * kKpStride);
        uint32_t kept = 0;
        for (uint32_t n = 0; n < n_oriented; ++n) {
            const int i = idx[n];
            const uint32_t* r = ori_all.data() + static_cast<size_t>(i) * kKpStride;
            for (uint32_t w = 0; w < kKpStride; ++w) clamped.push_back(r[w]);
            ++kept;
            const int os = oct_of(i) * kMaxOctaveResolution + scl_of(i);
            if (os != prev_os && kept >= static_cast<uint32_t>(max_features)) break;
            prev_os = os;
        }
        n_desc = kept;
        desc_in_buf = harness.upload(clamped.data(),
                                     clamped.size() * sizeof(uint32_t),
                                     wgpu::BufferUsage::Storage);
                                                                           
        ori_all.assign(clamped.begin(), clamped.end());
    }
    mark("clamp (pre-desc)");
                                                                                         
                                                                                  
                                                                                     
                                                                                    
                                                                                               
                                                                                                       
                                                                                  
                                                    
    static const bool q5a_audit = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_Q5A_AUDIT");
        return v != nullptr && v[0] == '1' && v[1] == '\0';
    }();
    if (q5a_audit) {
        constexpr long long kOsRes = 1000;
        long long os_min = 0;
        bool have_min = false;
        for (uint32_t i = 0; i < n_desc; ++i) {
            int o, s;
            std::memcpy(&o, &ori_all[static_cast<size_t>(i) * kKpStride + 6], 4);
            std::memcpy(&s, &ori_all[static_cast<size_t>(i) * kKpStride + 7], 4);
            const long long os = static_cast<long long>(o) * kOsRes + s;
            if (!have_min || os < os_min) { os_min = os; have_min = true; }
        }
        uint32_t prunable = 0;
        for (uint32_t i = 0; i < n_kept; ++i) {
            int o, s;
            std::memcpy(&o, &aff_in[static_cast<size_t>(i) * kKpStride + 5], 4);
            std::memcpy(&s, &aff_in[static_cast<size_t>(i) * kKpStride + 6], 4);
            const long long os = static_cast<long long>(o) * kOsRes + s;
            if (have_min && os < os_min) ++prunable;
        }
                                                                                               
                                                                                   
                                                                                        
                                                                                    
                                                                                                     
                                                                                         
        std::vector<std::pair<long long, uint32_t>> hist;
        for (uint32_t i = 0; i < n_kept; ++i) {
            int o, s;
            std::memcpy(&o, &aff_in[static_cast<size_t>(i) * kKpStride + 5], 4);
            std::memcpy(&s, &aff_in[static_cast<size_t>(i) * kKpStride + 6], 4);
            const long long os = static_cast<long long>(o) * kOsRes + s;
            bool found = false;
            for (auto& e : hist) {
                if (e.first == os) { ++e.second; found = true; break; }
            }
            if (!found) hist.emplace_back(os, 1u);
        }
        std::sort(hist.begin(), hist.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
                                                                                                   
                                                                                            
                                                                                                   
                                                                                                    
        long long os_safe = LLONG_MIN;
        unsigned long long cum = 0;
        for (size_t j = 0; j < hist.size(); ++j) {
            cum += hist[j].second;
            if (max_features > 0 &&
                cum >= static_cast<unsigned long long>(max_features)) {
                if (j + 1 < hist.size()) os_safe = hist[j + 1].first;
                break;
            }
        }
        uint32_t prunable_safe = 0;
        if (os_safe != LLONG_MIN) {
            for (const auto& e : hist) {
                if (e.first < os_safe) prunable_safe += e.second;
            }
        }
                                                                                           
                                                                                     
                                                                                             
        std::vector<uint32_t> ori_full = read_u32(
            harness, ori_out_buf, static_cast<size_t>(n_oriented) * kKpStride);
        std::vector<std::pair<long long, uint32_t>> ohist;
        for (uint32_t i = 0; i < n_oriented; ++i) {
            int o, s;
            std::memcpy(&o, &ori_full[static_cast<size_t>(i) * kKpStride + 6], 4);
            std::memcpy(&s, &ori_full[static_cast<size_t>(i) * kKpStride + 7], 4);
            const long long os = static_cast<long long>(o) * kOsRes + s;
            bool found = false;
            for (auto& e : ohist) {
                if (e.first == os) { ++e.second; found = true; break; }
            }
            if (!found) ohist.emplace_back(os, 1u);
        }
        unsigned long long cc = 0, co = 0;
        int shown = 0, shrink_groups = 0;
        for (const auto& e : hist) {
            uint32_t oi = 0;
            for (const auto& g : ohist) {
                if (g.first == e.first) { oi = g.second; break; }
            }
            cc += e.second;
            co += oi;
            if (oi < e.second) ++shrink_groups;
            if (shown < 12) {
                std::fprintf(stderr,
                             "[Q5A-GROUP] os=%-6lld cand=%-6u ori=%-6u "
                             "cum_cand=%-7llu cum_ori=%-7llu %s\n",
                             e.first, e.second, oi, cc, co,
                             oi < e.second ? "<-- ori<cand (k=0 存在)" : "");
                ++shown;
            }
        }
                                                                                    
                                                                                                    
        unsigned long long pc = 0, po = 0;
        int prefix_violations = 0;
        for (const auto& e : hist) {
            uint32_t oi = 0;
            for (const auto& g : ohist) {
                if (g.first == e.first) { oi = g.second; break; }
            }
            pc += e.second;
            po += oi;
            if (po < pc) ++prefix_violations;
        }
        std::fprintf(stderr,
                     "[Q5A-AUDIT] groups_with_ori_lt_cand=%d/%zu "
                     "prefix_violations=%d\n",
                     shrink_groups, hist.size(), prefix_violations);
        std::fprintf(stderr,
                     "[Q5A-AUDIT] n_detect=%u n_kept=%u n_oriented=%u "
                     "n_desc=%u max_feat=%d groups=%zu | posthoc_os_min=%lld "
                     "prunable_max=%u (%.2f%%) | conservative_os=%lld "
                     "prunable_safe=%u (%.2f%%)\n",
                     n_detect, n_kept, n_oriented, n_desc, max_features,
                     hist.size(), os_min, prunable,
                     n_kept ? 100.0 * prunable / n_kept : 0.0, os_safe,
                     prunable_safe,
                     n_kept ? 100.0 * prunable_safe / n_kept : 0.0);
    }
    aether_preclamp_instr_v1::UpdateLegacyClampResult(n_desc);
    max_features = requested_max_features;

    if (canonical_selection || coverage_selection) {
        const std::vector<uint32_t> full_orientation_sidecar =
            read_u32(harness, dbg_buf,
                     static_cast<size_t>(n_oriented) * 2);
        std::vector<uint32_t> coverage_orientation_sidecar;
        std::span<const uint32_t> orientation_sidecar =
            full_orientation_sidecar;
        if (coverage_selection) {
            const std::vector<uint32_t> full_oriented_rows =
                read_u32(harness, ori_out_buf,
                         static_cast<size_t>(n_oriented) * kKpStride);
            const auto sidecar_gather_status =
                aether::sfm::GatherSelectedSidecarByRowsV1(
                    full_oriented_rows, ori_all, full_orientation_sidecar,
                    &coverage_orientation_sidecar);
            if (sidecar_gather_status !=
                aether::sfm::SelectedSidecarGatherStatusV1::kOk) {
                std::cerr << "[SiftExtractDawn] coverage sidecar gather failed status="
                          << static_cast<uint32_t>(sidecar_gather_status)
                          << " (fallback)\n";
                return false;
            }
            orientation_sidecar = coverage_orientation_sidecar;
        }
        std::vector<aether::sfm::CanonicalFeatureCandidateV1> candidates;
        const auto build_status =
            aether::sfm::BuildCanonicalCandidatesFromSidecarV1(
                ori_all, orientation_sidecar, aff_in, &candidates);
        if (build_status !=
            aether::sfm::CanonicalCandidateBuildStatusV1::kOk) {
            std::cerr << "[SiftExtractDawn] canonical sidecar invalid status="
                      << static_cast<uint32_t>(build_status)
                      << " (fallback)\n";
            return false;
        }

        aether::sfm::CanonicalFeatureSelectionV1 selection;
        const uint32_t canonical_cap =
            requested_max_features > 0
                ? static_cast<uint32_t>(requested_max_features)
                : static_cast<uint32_t>(candidates.size());
        const auto select_status = coverage_selection
            ? aether::sfm::SelectCoverageFeaturesV1(
                  candidates, canonical_cap, static_cast<uint32_t>(width),
                  static_cast<uint32_t>(height), &selection)
            : aether::sfm::SelectCanonicalFeaturesV1(
                  candidates, canonical_cap, &selection);
        if (select_status !=
            aether::sfm::CanonicalFeatureSelectionStatusV1::kOk) {
            std::cerr << "[SiftExtractDawn] feature selector failed status="
                      << static_cast<uint32_t>(select_status)
                      << " (fallback)\n";
            return false;
        }

        std::vector<uint32_t> canonical_rows;
        const auto gather_status = aether::sfm::GatherCanonicalRowsV1(
            ori_all, kKpStride, selection.input_indices, &canonical_rows);
        if (gather_status !=
            aether::sfm::CanonicalRowGatherStatusV1::kOk) {
            std::cerr << "[SiftExtractDawn] canonical row gather failed status="
                      << static_cast<uint32_t>(gather_status)
                      << " (fallback)\n";
            return false;
        }
        n_desc = static_cast<uint32_t>(selection.input_indices.size());
        out->stable_ids = std::move(selection.stable_ids);
        ori_all = std::move(canonical_rows);
        if (n_desc > 0) {
            desc_in_buf = harness.upload(
                ori_all.data(), ori_all.size() * sizeof(uint32_t),
                wgpu::BufferUsage::Storage);
        }

                                                                             
                                                                               
                                                                             
                                                                         
        if (observe_timing) {
            const auto now = std::chrono::high_resolution_clock::now();
            g_aether_sed_stage_ms[6] +=
                std::chrono::duration<double, std::milli>(now - t_prev)
                    .count();
            const auto& h = harness.host_breakdown();
            hb_stage[6][0] += h.upload_ms - hb_snap.upload_ms;
            hb_stage[6][1] += h.encode_ms - hb_snap.encode_ms;
            hb_stage[6][2] += h.submit_wait_ms - hb_snap.submit_wait_ms;
            hb_stage[6][3] += h.map_ms - hb_snap.map_ms;
            hb_stage[6][4] += h.create_ms - hb_snap.create_ms;
            hb_snap = h;
            t_prev = now;
        }
        if (n_desc == 0) {
            out->xy.clear();
            out->octave.clear();
            out->scale.clear();
            out->kp_scale.clear();
            out->kp_orientation.clear();
            out->raw_desc.clear();
            out->count = 0;
            return true;
        }
    }

                                                                                                                                         
    const uint32_t n_oriented_full = n_oriented;
    (void)n_oriented_full;
    n_oriented = n_desc;                                                            
    struct DescParams {
        uint32_t count;
        int fo, lo, res;
        float base;
        int ofs, ols;
        uint32_t lpo;
        float dmin, dstep;
        uint32_t p0, p1;
    } DP{n_oriented,
         0,
         pyr.last_octave(),
         kOctaveResolution,
         base_scale,
         SiftPyramidDawn::kOctaveFirstSub,
         SiftPyramidDawn::kOctaveLastSub,
         static_cast<uint32_t>(SiftPyramidDawn::kLevelsPerOctave),
         kDspMinScale,
         (kDspMaxScale - kDspMinScale) / kDspNumScales,
         0u,
         0u};
    wgpu::Buffer rd_buf;
    {
        std::vector<float> rd_init(static_cast<size_t>(n_oriented) * 128, 0.0f);
        rd_buf = harness.upload(rd_init.data(), rd_init.size() * sizeof(float),
                                wgpu::BufferUsage::Storage |
                                    wgpu::BufferUsage::CopySrc);
        wgpu::Buffer p_buf =
            harness.upload(&DP, sizeof(DP), wgpu::BufferUsage::Uniform);

                                                                            
                                                                                      
                                                                                  
                                                                                  
                                                                            
                                                                                  
                                                                         
                                                                               
        if (std::getenv("SED_PARALLEL_DESC") != nullptr) {
            const uint32_t n_sd =
                static_cast<uint32_t>(n_oriented) * kDspNumScales;
            std::vector<float> sd_init(static_cast<size_t>(n_sd) * 128, 0.0f);
            wgpu::Buffer scale_desc = harness.upload(
                sd_init.data(), sd_init.size() * sizeof(float),
                wgpu::BufferUsage::Storage);
                                                                               
                                                                               
                                                                            
            const uint32_t groups_x = 32768u;
            const uint32_t groups_y = (n_sd + groups_x - 1u) / groups_x;
            DP.p0 = groups_x;
            wgpu::Buffer p_buf2 =
                harness.upload(&DP, sizeof(DP), wgpu::BufferUsage::Uniform);
            wgpu::ComputePipeline pipe_par =
                harness.load_compute(load_wgsl("sift_dsp_descriptor_par.wgsl"));
            {
                using BB = DawnKernelHarness::BufBinding;
                harness.dispatch(pipe_par,
                                 {w256_kp(packed, pyr), BB(meta_buf),
                                  BB(desc_in_buf), BB(scale_desc), BB(p_buf2)},
                                 groups_x, groups_y, 1u);
            }
            struct MeanParams { uint32_t count; } MP{n_oriented};
            wgpu::Buffer mp_buf =
                harness.upload(&MP, sizeof(MP), wgpu::BufferUsage::Uniform);
            wgpu::ComputePipeline pipe_mean =
                harness.load_compute(load_wgsl("sift_dsp_mean.wgsl"));
            {
                                                                             
                                                    
                const uint32_t mg = (n_oriented * 128u + 63u) / 64u;
                harness.dispatch(pipe_mean, {scale_desc, rd_buf, mp_buf},
                                 std::min(mg, 65535u),
                                 (mg + 65534u) / 65535u, 1u);
            }
        } else {
                                                                          
                                                                          
                                                                                   
                                                                               
                                                                                      
                                                                                   
                                                                                  
                                                                           
                                                                                       
                                                                                 
                                                                                           
                                                                                            
            const char* desc_shader = desc_shader_name(harness);
            wgpu::ComputePipeline pipe_desc =
                harness.load_compute(load_wgsl(desc_shader));
                                                                               
            {
                using BB = DawnKernelHarness::BufBinding;
                harness.dispatch(pipe_desc,
                                 {w256_kp(packed, pyr), BB(meta_buf),
                                  BB(desc_in_buf), BB(rd_buf), BB(p_buf)},
                                 std::min(n_oriented, 65535u),
                                 (n_oriented + 65534u) / 65535u, 1u);
            }
        }
    }
    mark("descriptor");

                                                                                             
    const size_t rbytes = static_cast<size_t>(n_oriented) * 128 * sizeof(float);
    wgpu::Buffer rd_st = harness.alloc_staging_for_readback(rbytes);
    harness.copy_to_staging(rd_buf, rd_st, rbytes);
    std::vector<uint8_t> rd_raw = harness.readback(rd_st, rbytes);

                                                                                
                                                                              
    const std::vector<uint32_t>& ori_recs = ori_all;
    mark("desc readback");

                                                                                           
    out->count = static_cast<int>(n_oriented);
    out->xy.resize(static_cast<size_t>(n_oriented) * 2);
    out->octave.resize(n_oriented);
    out->scale.resize(n_oriented);
    out->kp_scale.resize(n_oriented);
    out->kp_orientation.resize(n_oriented);
    out->raw_desc.resize(static_cast<size_t>(n_oriented) * 128);
    std::memcpy(out->raw_desc.data(), rd_raw.data(), rbytes);
    bool desc_nan = false;
    for (uint32_t i = 0; i < n_oriented; ++i) {
        const uint32_t* r = ori_recs.data() + static_cast<size_t>(i) * kKpStride;
        float x, y;
        std::memcpy(&x, &r[0], 4);
        std::memcpy(&y, &r[1], 4);
        out->xy[2 * i] = x;
        out->xy[2 * i + 1] = y;
                                                                              
                                                                                 
                                                                    
                                                                      
                                                                          
        float a11, a12, a21, a22;
        std::memcpy(&a11, &r[2], 4);
        std::memcpy(&a12, &r[3], 4);
        std::memcpy(&a21, &r[4], 4);
        std::memcpy(&a22, &r[5], 4);
        const float scale_x = std::sqrt(a11 * a11 + a21 * a21);
        const float scale_y = std::sqrt(a12 * a12 + a22 * a22);
        out->kp_scale[i] = (scale_x + scale_y) / 2.0f;
        out->kp_orientation[i] = std::atan2(a21, a11);
        int o, s;
        std::memcpy(&o, &r[6], 4);
        std::memcpy(&s, &r[7], 4);
        out->octave[i] = o;
        out->scale[i] = s;
    }
                                                                                 
                                                                  
    for (size_t i = 0; i < out->raw_desc.size(); ++i) {
        if (std::isnan(out->raw_desc[i])) {
            desc_nan = true;
            break;
        }
    }
    if (desc_nan) {
        std::cerr << "[SiftExtractDawn] NaN descriptor (fallback)\n";
        sed_set_fail_reason("nan_descriptor");
        return false;
    }
                                                                                               
                                                                            
    if (dawn_failed("orient/descriptor")) {
        return false;
    }

                                                                           
                                                                               
                                                                        
                                                                            
    static const bool ts_diag = [] {                                        
        const char* v = std::getenv("OFFICIAL_AETHER_GPU_TS_DIAG");
        return v != nullptr && v[0] == '1' && v[1] == '\0';
    }();
    if (ts_diag) {
        std::fprintf(stderr,
                     "[GPU-TS-DIAG] sed gpu_ts_enabled=%d pass_count=%u "
                     "bounds=%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                     harness.gpu_ts_enabled() ? 1 : 0,
                     harness.gpu_ts_pass_count(), ts_bounds[0], ts_bounds[1],
                     ts_bounds[2], ts_bounds[3], ts_bounds[4], ts_bounds[5],
                     ts_bounds[6], ts_bounds[7], ts_bounds[8]);
    }
                                                                                            
                                                                                       
    if (harness.gpu_ts_enabled() || harness.host_breakdown().n_dispatch > 0) {
        std::vector<uint64_t> pass_ns;
        const bool ts_resolved = harness.gpu_ts_resolve(&pass_ns);
        const bool ts_final =
            ts_resolved &&
            harness.gpu_ts_finalize_frame(ts_bounds, kAetherSedStageCount);
        if (ts_diag) {
            std::fprintf(stderr,
                         "[GPU-TS-DIAG] sed resolve=%d finalize=%d "
                         "pass_ns=%zu\n",
                         ts_resolved ? 1 : 0, ts_final ? 1 : 0, pass_ns.size());
        }
                                                                                     
                                                                                   
        if (ts_final || harness.host_breakdown().n_dispatch > 0) {
            uint32_t lo = 0;
            for (int s = 0; s < kAetherSedStageCount; ++s) {
                const uint32_t hi =
                    ts_bounds[s] < pass_ns.size()
                        ? ts_bounds[s]
                        : static_cast<uint32_t>(pass_ns.size());
                double ns = 0.0;
                for (uint32_t p = lo; p < hi; ++p) ns += double(pass_ns[p]);
                g_aether_sed_stage_gpu_ms[s] = ns / 1.0e6;
                lo = hi;
            }
            if (timing) {
                static const char* kStage[kAetherSedStageCount] = {
                    "pyramid", "pack", "detect", "suppress", "affine",
                    "orient",  "clamp", "descriptor", "desc-rb"};
                const auto& H = harness.host_breakdown();
                std::printf(
                    "  [SED-SPLIT] passes=%zu dispatches=%u uploads=%u "
                    "readbacks=%u pipeline_compiles=%u (%.1f ms)\n",
                    pass_ns.size(), H.n_dispatch, H.n_upload, H.n_readback,
                    H.n_create, H.create_ms);
                std::printf("  %-12s %8s %8s %8s %8s %8s %8s %8s %10s\n",
                            "stage", "host", "gpu", "compile", "upload",
                            "encode", "wait", "map", "own-loop");
                for (int s = 0; s < kAetherSedStageCount; ++s) {
                    const double host = g_aether_sed_stage_ms[s];
                    const double gpu = g_aether_sed_stage_gpu_ms[s];
                                                                              
                                                                             
                    const double harness_cpu = hb_stage[s][0] + hb_stage[s][1] +
                                               hb_stage[s][2] + hb_stage[s][3] +
                                               hb_stage[s][4];
                    std::printf(
                        "  %-12s %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f %10.1f\n",
                        kStage[s], host, gpu, hb_stage[s][4], hb_stage[s][0],
                        hb_stage[s][1], hb_stage[s][2], hb_stage[s][3],
                        host - harness_cpu);
                }
                std::fflush(stdout);
            }
        }
    }
    return true;
}

}                    
}                     
