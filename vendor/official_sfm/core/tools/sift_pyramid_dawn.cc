#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#include "sift_pyramid_dawn.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <cstring>
#include <iostream>
#include <string>

#ifdef AETHER_WGSL_DIR
                                                                            
                                             
#include <fstream>
#include <sstream>
#else
                                                                     
#include "aether/shaders/wgsl_sources.h"
#include <string_view>
#endif

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
        std::cerr << "[SiftPyramidDawn] cannot open WGSL: " << path << '\n';
        std::abort();
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
#else
    using namespace aether::shaders;
    const std::string_view fn(filename);
    if (fn == "sift_gray_to_f32.wgsl")   return std::string(sift_gray_to_f32_wgsl);
    if (fn == "sift_gss_blur.wgsl")      return std::string(sift_gss_blur_wgsl);
    if (fn == "sift_gss_resample.wgsl")  return std::string(sift_gss_resample_wgsl);
    if (fn == "sift_gss_blur_fused.wgsl") return std::string(sift_gss_blur_fused_wgsl);
    std::cerr << "[SiftPyramidDawn] unknown WGSL: " << filename << '\n';
    std::abort();
#endif
}

                                                                             
                                                                             
                                                                               

                                                                                  
                                                                                          
                                                                                     
                                                                                    
                                                                                                  
                                                                                           
                                               
namespace {
struct PersistBuf {
    int slot = -1;
    WGPUDevice dev = nullptr;
    size_t size = 0;
    wgpu::Buffer buf;
};
                                                                                    
thread_local std::vector<PersistBuf> g_pyr_persist;
                                                                                                 
                                                                             
                                                                                    
                                                                                   
                                                
bool pyr_persist_on() {
                                                                                              
                                                                  
                                                                                                        
                                                                            
                                                              
                                                                           
                                                                                                   
                                                                             
                                                                                            
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_PYR_PERSIST");
        return v == nullptr || !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}
wgpu::Buffer pyr_persist_alloc(aether::tools::DawnKernelHarness& h, int slot,
                               size_t size, wgpu::BufferUsage usage) {
    if (!pyr_persist_on()) return h.alloc(size, usage);
    const WGPUDevice dev = h.device().Get();
    for (auto& p : g_pyr_persist) {
        if (p.slot == slot && p.dev == dev && p.size == size) return p.buf;
    }
                                                                                                       
    if (slot != 3) {
        g_pyr_persist.erase(
            std::remove_if(g_pyr_persist.begin(), g_pyr_persist.end(),
                           [&](const PersistBuf& p) { return p.slot == slot; }),
            g_pyr_persist.end());
    }
    PersistBuf nb; nb.slot = slot; nb.dev = dev; nb.size = size;
    nb.buf = h.alloc(size, usage);
    g_pyr_persist.push_back(nb);
    return nb.buf;
}
}              

                                                                                                               
extern "C" void aether_pyr_persist_release() { g_pyr_persist.clear(); }
                                                                   
extern "C" void aether_pyr_persist_end_frame() {}

std::vector<float> make_gaussian_taps(double sigma) {
    const int width = static_cast<int>(std::ceil(sigma * 3.0));
    const int size = 2 * width + 1;
    std::vector<double> filt(static_cast<size_t>(size));
    double mass = 1.0;
    filt[static_cast<size_t>(width)] = 1.0;
    for (int i = 1; i <= width; ++i) {
        const double x = static_cast<double>(i) / sigma;
        const double g = std::exp(-0.5 * x * x);
        mass += g + g;
        filt[static_cast<size_t>(width - i)] = g;
        filt[static_cast<size_t>(width + i)] = g;
    }
    std::vector<float> taps(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i) {
        taps[static_cast<size_t>(i)] = static_cast<float>(filt[static_cast<size_t>(i)] / mass);
    }
    return taps;
}

}              

double SiftPyramidDawn::base_scale() {
    return 1.6 * std::pow(2.0, 1.0 / static_cast<double>(kOctaveResolution));
}

double SiftPyramidDawn::level_sigma(int octave, int sublevel) {
    return base_scale() *
           std::pow(2.0, static_cast<double>(octave) +
                             static_cast<double>(sublevel) /
                                 static_cast<double>(kOctaveResolution));
}

SiftPyramidDawn::LevelGeom SiftPyramidDawn::level_geom(int octave,
                                                       int sublevel) const {
    LevelGeom g;
    g.octave = octave;
    g.sublevel = sublevel;
    g.width = width_ >> octave;
    g.height = height_ >> octave;
    g.sigma = level_sigma(octave, sublevel);
    return g;
}

                                                               
  
                                                                                            
                                                                                       
  
                                                                                
                                                                                               
void SiftPyramidDawn::warmup(DawnKernelHarness& harness) {
    harness.load_compute(load_wgsl("sift_gray_to_f32.wgsl"));
    harness.load_compute(load_wgsl("sift_gss_blur.wgsl"));
    harness.load_compute(load_wgsl("sift_gss_resample.wgsl"));
    harness.load_compute(load_wgsl("sift_gss_blur_fused.wgsl"));
}

bool SiftPyramidDawn::build(DawnKernelHarness& harness, const uint8_t* gray,
                            int width, int height) {
    if (width < 2 || height < 2 || gray == nullptr) {
        std::cerr << "[SiftPyramidDawn] invalid dimensions\n";
        return false;
    }
    width_ = width;
    height_ = height;

                                                          
                                                      
    const double r =
        static_cast<double>(std::min(width - 1, height - 1)) /
        static_cast<double>(kMinOctaveSize - 1);
    last_octave_ = static_cast<int>(std::floor(std::log2(r)));
    if (last_octave_ < 0) last_octave_ = 0;

                                                  
    const std::string s0_src = load_wgsl("sift_gray_to_f32.wgsl");
    const std::string s1_src = load_wgsl("sift_gss_blur.wgsl");
    const std::string s1b_src = load_wgsl("sift_gss_resample.wgsl");
    wgpu::ComputePipeline pipe_gray = harness.load_compute(s0_src);
    wgpu::ComputePipeline pipe_blur = harness.load_compute(s1_src);
    wgpu::ComputePipeline pipe_resample = harness.load_compute(s1b_src);
                                                                              
                                                                                  
    static const bool fused_on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_GSS_FUSED");
        return v == nullptr || !(v[0] == '0' && v[1] == '\0');
    }();
    wgpu::ComputePipeline pipe_blur_fused =
        harness.load_compute(load_wgsl("sift_gss_blur_fused.wgsl"));

    const auto wg = [](int n) -> uint32_t {
        return static_cast<uint32_t>((n + 7) / 8);
    };

                                                                                               
                                                                                         
                                                                                           
                                                                
    octaves_.assign(static_cast<size_t>(last_octave_ + 1), {});
    for (int o = 0; o <= last_octave_; ++o) {
        OctaveBuffers& ob = octaves_[static_cast<size_t>(o)];
        ob.width = width_ >> o;
        ob.height = height_ >> o;
    }
                                                                                       
                                                            
                                                                                          
                                                                                                 
                                                                                                          
                                                                                        
    {
        uint64_t req_elems = 0;
        for (int o = 0; o <= last_octave_; ++o) {
            const uint64_t n = static_cast<uint64_t>(width_ >> o) *
                               static_cast<uint64_t>(height_ >> o);
            for (int li = 0; li < kLevelsPerOctave; ++li) {
                req_elems = (req_elems + kAlignElems - 1u) / kAlignElems * kAlignElems;
                req_elems += n;
            }
        }
        wgpu::Limits dev_limits{};
        const uint64_t lim =
            harness.device().GetLimits(&dev_limits) == wgpu::Status::Success
                ? static_cast<uint64_t>(dev_limits.maxStorageBufferBindingSize)
                : 0ull;
        configure_w256_by_limit(req_elems * 4ull, lim);
        note_kp_bind(-1);
    }
    level_offsets_ = level_layout(width_, height_, last_octave_,
                                  &packed_total_elems_, &keypoint_region_end_,
                                  &packed_b_elems_);
    if (splitbuf_enabled()) {
        packed_b_ = pyr_persist_alloc(harness, 8,
            static_cast<size_t>(packed_b_elems_) * sizeof(float),
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                wgpu::BufferUsage::CopyDst);
    }
    packed_buf_ = pyr_persist_alloc(harness, 0,
        static_cast<size_t>(packed_total_elems_) * sizeof(float),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
            wgpu::BufferUsage::CopyDst);

                                               
    const auto level_off = [&](int o, int s) -> uint32_t {
        return level_offsets_[static_cast<size_t>(o) * kLevelsPerOctave +
                              (s - kOctaveFirstSub)];
    };
                                                                                            
                                                                               
    const bool split = splitbuf_enabled();
                                                                                             
    const auto buf_of = [&](int sublevel) -> const wgpu::Buffer& {
        return (split && level_in_hi(sublevel)) ? packed_b_ : packed_buf_;
    };
    const bool w256 = !split && w256_bind_enabled();
    const auto rng = [&](const wgpu::Buffer& b, uint32_t lo, uint32_t hi) {
        if (!w256) return DawnKernelHarness::BufBinding(b);
        return DawnKernelHarness::BufBinding(
            packed_buf_, static_cast<uint64_t>(lo) * 4u,
            static_cast<uint64_t>(hi - lo) * 4u);
    };
                                                                                             
    const auto rel = [&](uint32_t off, uint32_t lo) -> uint32_t {
        return w256 ? (off - lo) : off;
    };
                                                                                                            
    const auto rng_always = [&](uint32_t lo, uint32_t hi) {
        return DawnKernelHarness::BufBinding(
            packed_buf_, static_cast<uint64_t>(lo) * 4u,
            static_cast<uint64_t>(hi - lo) * 4u);
    };
    using BB = DawnKernelHarness::BufBinding;

                                                                     
                                                                          
                                                                               
                                                                                      
                                              
                                                                                 
                                                                                      
                                                                                              
                                                                                               
                                                                        
    const auto cross_sub = kOctaveFirstSub + kKeypointLevels;         
    const auto src_off_for = [&](int o, int s) -> uint32_t {
        if (!split || s != cross_sub) return level_off(o, s - 1);
        const OctaveBuffers& ob = octaves_[static_cast<size_t>(o)];
        const uint32_t n = static_cast<uint32_t>(ob.width) *
                           static_cast<uint32_t>(ob.height);
        const uint32_t tmp = level_off(o, kOctaveLastSub);
        harness.copy_region_batched(
            packed_buf_, static_cast<uint64_t>(level_off(o, s - 1)) * 4u,
            packed_b_, static_cast<uint64_t>(tmp) * 4u,
            static_cast<size_t>(n) * 4u);
        return tmp;
    };
    const auto blur = [&](uint32_t src_off, uint32_t dst_off, uint32_t scratch_off,
                          int w, int h, double sigma_px, uint32_t scratch_n,
                          int dst_sub = kOctaveFirstSub) {
        const wgpu::Buffer& bbuf = buf_of(dst_sub);
        const std::vector<float> taps = make_gaussian_taps(sigma_px);
        const uint32_t n = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
        const uint32_t lo = std::min({src_off, dst_off, scratch_off});
        const uint32_t hi =
            std::max({src_off + n, dst_off + n, scratch_off + scratch_n});
        const uint32_t radius = static_cast<uint32_t>((taps.size() - 1) / 2);
        wgpu::Buffer taps_buf = harness.upload(taps.data(),
                                               taps.size() * sizeof(float),
                                               wgpu::BufferUsage::Storage);
        struct BlurParams {
            uint32_t width, height, radius, axis, src_off, dst_off;
            uint32_t _pad0, _pad1;
        };
        BlurParams ph{static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                      radius, 0u, rel(src_off, lo), rel(scratch_off, lo), 0u, 0u};
        wgpu::Buffer ph_buf = harness.upload(&ph, sizeof(ph),
                                             wgpu::BufferUsage::Uniform);
        harness.dispatch_batched(pipe_blur, {rng(bbuf, lo, hi), BB(taps_buf), BB(ph_buf)},
                                 wg(w), wg(h));
        BlurParams pv{static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                      radius, 1u, rel(scratch_off, lo), rel(dst_off, lo), 0u, 0u};
        wgpu::Buffer pv_buf = harness.upload(&pv, sizeof(pv),
                                             wgpu::BufferUsage::Uniform);
        harness.dispatch_batched(pipe_blur, {rng(bbuf, lo, hi), BB(taps_buf), BB(pv_buf)},
                                 wg(w), wg(h));
    };

                                                                           
                                                                                
                                                                                  
                                                                                  
                                                                              
                                                           
    harness.begin_batch();

                                                                                          
                                                                              
                                           
    {
                                                                        
        const size_t n = static_cast<size_t>(width_) * height_;
        const size_t words = (n + 3) / 4;
        std::vector<uint32_t> packed(words, 0u);
        std::memcpy(packed.data(), gray, n);
        wgpu::Buffer src_u8 =
            harness.upload(packed.data(), words * sizeof(uint32_t),
                           wgpu::BufferUsage::Storage);
        struct GrayParams {
            uint32_t width, height, dst_off, _pad;
        } gp{static_cast<uint32_t>(width_), static_cast<uint32_t>(height_),
             0u                                                                 , 0u};
        const uint32_t l0 = level_off(0, kOctaveFirstSub);
        gp.dst_off = rel(l0, l0);
        wgpu::Buffer gp_buf = harness.upload(&gp, sizeof(gp),
                                             wgpu::BufferUsage::Uniform);
        harness.dispatch_batched(
            pipe_gray,
            {BB(src_u8), rng(packed_buf_, l0, l0 + static_cast<uint32_t>(n)), BB(gp_buf)},
            wg(width_), wg(height_));
    }

                                                                                           
                                                                           
                                                                              
    {
        OctaveBuffers& ob0 = octaves_[0];
        const double sigma = level_sigma(0, kOctaveFirstSub);
        if (sigma > kNominalScale) {
            const double delta =
                std::sqrt(sigma * sigma - kNominalScale * kNominalScale);
                                                                                         
                                                                                                         
            blur(level_off(0, kOctaveFirstSub), level_off(0, kOctaveFirstSub),
                 level_off(0, kOctaveFirstSub + 1), ob0.width, ob0.height,
                 delta                    ,
                 static_cast<uint32_t>(ob0.width) *
                     static_cast<uint32_t>(ob0.height));
        }
    }

    const auto fill_octave = [&](int o) {
        OctaveBuffers& ob = octaves_[static_cast<size_t>(o)];
        const double step = std::pow(2.0, static_cast<double>(o));
        for (int s = kOctaveFirstSub + 1; s <= kOctaveLastSub; ++s) {
            const double sigma = level_sigma(o, s);
            const double prev = level_sigma(o, s - 1);
            const double delta = std::sqrt(sigma * sigma - prev * prev);
            const double sigma_px = delta / step;
            const std::vector<float> taps = make_gaussian_taps(sigma_px);
            const uint32_t radius =
                static_cast<uint32_t>((taps.size() - 1) / 2);
            if (fused_on && radius <= 16u) {
                                                                                
                wgpu::Buffer taps_buf = harness.upload(
                    taps.data(), taps.size() * sizeof(float),
                    wgpu::BufferUsage::Storage);
                struct FusedParams {
                    uint32_t width, height, radius, src_off;
                    uint32_t dst_off, _pad0, _pad1, _pad2;
                } fp{static_cast<uint32_t>(ob.width),
                     static_cast<uint32_t>(ob.height), radius,
                     rel(level_off(o, s - 1), level_off(o, s - 1)), rel(level_off(o, s), level_off(o, s - 1)), 0u, 0u, 0u};
                wgpu::Buffer fp_buf = harness.upload(
                    &fp, sizeof(fp), wgpu::BufferUsage::Uniform);
                harness.dispatch_batched(
                    pipe_blur_fused,
                    {rng(buf_of(s), src_off_for(o, s),
                         level_off(o, s) + static_cast<uint32_t>(ob.width) *
                                               static_cast<uint32_t>(ob.height)),
                     BB(taps_buf), BB(fp_buf)},
                    static_cast<uint32_t>((ob.width + 7) / 8), 1u);
            } else {
                                                                                           
                                                                                                      
                uint32_t scratch_off = 0u; bool have = false; uint32_t scratch_n = 0u;
                                                                                       
                                                                                                   
                                                                                               
                                                                  
                const bool split_last = split && s == kOctaveLastSub;
                if (!split_last && s + 1 <= kOctaveLastSub) { scratch_off = level_off(o, s + 1); have = true; scratch_n = static_cast<uint32_t>(ob.width) * static_cast<uint32_t>(ob.height); }
                else if (!split && o + 1 <= last_octave_) { scratch_off = level_off(o + 1, kOctaveFirstSub); have = true; const OctaveBuffers& nb = octaves_[static_cast<size_t>(o + 1)]; scratch_n = static_cast<uint32_t>(nb.width) * static_cast<uint32_t>(nb.height); }
                if (have) {
                    blur(src_off_for(o, s), level_off(o, s), scratch_off,
                         ob.width, ob.height, sigma_px, scratch_n, s);
                } else {
                    wgpu::Buffer taps_buf = harness.upload(
                        taps.data(), taps.size() * sizeof(float),
                        wgpu::BufferUsage::Storage);
                    struct FusedParams {
                        uint32_t width, height, radius, src_off;
                        uint32_t dst_off, _pad0, _pad1, _pad2;
                    } fp{static_cast<uint32_t>(ob.width),
                         static_cast<uint32_t>(ob.height), radius,
                         rel(level_off(o, s - 1), level_off(o, s - 1)), rel(level_off(o, s), level_off(o, s - 1)), 0u, 0u, 0u};
                    wgpu::Buffer fp_buf = harness.upload(
                        &fp, sizeof(fp), wgpu::BufferUsage::Uniform);
                    harness.dispatch_batched(
                        pipe_blur_fused,
                        {rng(buf_of(s), src_off_for(o, s),
                             level_off(o, s) + static_cast<uint32_t>(ob.width) *
                                                   static_cast<uint32_t>(ob.height)),
                         BB(taps_buf), BB(fp_buf)},
                        static_cast<uint32_t>((ob.width + 7) / 8), 1u);
                }
            }
        }
    };

                                    
    fill_octave(0);

                                                                  
                                                        
                                                                        
                                                                       
                                                                       
                                                            
    for (int o = 1; o <= last_octave_; ++o) {
        OctaveBuffers& ob = octaves_[static_cast<size_t>(o)];
        const int prev_sub = std::min(kOctaveFirstSub + kOctaveResolution,
                                      kOctaveLastSub);        
        OctaveBuffers& prev = octaves_[static_cast<size_t>(o - 1)];
        struct ResampleParams {
            uint32_t src_width, dst_width, dst_height, src_off;
            uint32_t dst_off, _pad0, _pad1, _pad2;
        } rp{static_cast<uint32_t>(prev.width),
             static_cast<uint32_t>(ob.width),
             static_cast<uint32_t>(ob.height),
             0u, 0u                                                        , 0u, 0u, 0u};
        const uint32_t rs_src = level_off(o - 1, prev_sub);
        const uint32_t rs_dst = level_off(o, kOctaveFirstSub);
        rp.src_off = 0u;                                                
        rp.dst_off = 0u;
        if (split) {
                                                                                                 
                                                                                          
                                                                   
            rp.src_off = rs_src;
            rp.dst_off = rs_dst;
        }
        wgpu::Buffer rp_buf = harness.upload(&rp, sizeof(rp),
                                             wgpu::BufferUsage::Uniform);
                                                                                  
                                                                                             
        harness.dispatch_batched(
            pipe_resample,
            {split ? BB(buf_of(prev_sub))
                   : rng_always(rs_src,
                                rs_src + static_cast<uint32_t>(prev.width) *
                                             static_cast<uint32_t>(prev.height)),
             split ? BB(buf_of(kOctaveFirstSub))
                   : rng_always(rs_dst,
                                rs_dst + static_cast<uint32_t>(ob.width) *
                                             static_cast<uint32_t>(ob.height)),
             BB(rp_buf)},
            wg(ob.width), wg(ob.height));
        fill_octave(o);
    }

                                                                             
    harness.end_batch();
    return true;
}

namespace {
                                                                          
uint64_t g_w256_required_bytes = 0;
uint64_t g_w256_limit_bytes = 0;
bool g_w256_configured = false;
}              

void SiftPyramidDawn::configure_w256_by_limit(uint64_t required_bytes,
                                              uint64_t limit_bytes) {
    g_w256_required_bytes = required_bytes;
    g_w256_limit_bytes = limit_bytes;
    g_w256_configured = true;
}

void SiftPyramidDawn::w256_last_decision(uint64_t* required_bytes,
                                         uint64_t* limit_bytes, int* enabled,
                                         int* forced_by_env) {
    if (required_bytes) *required_bytes = g_w256_required_bytes;
    if (limit_bytes) *limit_bytes = g_w256_limit_bytes;
    if (enabled) *enabled = w256_enabled() ? 1 : 0;
    if (forced_by_env)
        *forced_by_env = std::getenv("OFFICIAL_AETHER_W256") != nullptr ? 1 : 0;
}

namespace { int g_last_kp_bind = -1; }
void SiftPyramidDawn::note_kp_bind(int subrange) { g_last_kp_bind = subrange; }
int SiftPyramidDawn::last_kp_bind() { return g_last_kp_bind; }

uint32_t SiftPyramidDawn::window_elems() {
    if (!g_w256_configured || g_w256_limit_bytes == 0) return kWindowElems;
    const uint64_t elems = g_w256_limit_bytes / 4ull;
    return elems > 0xFFFFFFFFull ? 0xFFFFFFFFu
                                 : static_cast<uint32_t>(elems);
}

bool SiftPyramidDawn::splitbuf_enabled() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_SPLITBUF");
        return v != nullptr && !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

bool SiftPyramidDawn::kpbuf_enabled() {
    static const bool on = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_KPBUF");
        return v != nullptr && !(v[0] == '0' && v[1] == '\0');
    }();
    return on;
}

void SiftPyramidDawn::sync_keypoint_buffer(DawnKernelHarness& harness) {
    if (!kpbuf_enabled() || keypoint_region_end_ == 0) return;
    const size_t bytes = static_cast<size_t>(keypoint_region_end_) * 4u;
    if (kp_buf_ == nullptr) {
        kp_buf_ = pyr_persist_alloc(harness, 7, bytes,
                                    wgpu::BufferUsage::Storage |
                                        wgpu::BufferUsage::CopySrc |
                                        wgpu::BufferUsage::CopyDst);
    }
    harness.copy_region(packed_buf_, 0, kp_buf_, 0, bytes);
}

bool SiftPyramidDawn::w256_bind_enabled() {
                                                                                                      
                                                                                                 
    if (splitbuf_enabled()) return false;
    if (const char* v = std::getenv("OFFICIAL_AETHER_W256_BIND")) {
        return !(v[0] == '0' && v[1] == '\0');
    }
    return w256_enabled();
}

bool SiftPyramidDawn::w256_enabled() {
                                                                                
    if (const char* v = std::getenv("OFFICIAL_AETHER_W256")) {
        return !(v[0] == '0' && v[1] == '\0');
    }
                                                                                               
                                                                                             
                                       
    if (!g_w256_configured || g_w256_limit_bytes == 0) return true;
    return g_w256_required_bytes > g_w256_limit_bytes;
}

std::vector<uint32_t> SiftPyramidDawn::level_layout(int width, int height,
                                                    int last_octave,
                                                    uint32_t* total_elems,
                                                    uint32_t* keypoint_region_end,
                                                    uint32_t* hi_elems) {
    if (splitbuf_enabled()) {
                                                                                              
        std::vector<uint32_t> offs(
            static_cast<size_t>(last_octave + 1) * kLevelsPerOctave, 0u);
        uint32_t run[2] = {0u, 0u};
        for (int region = 0; region < 2; ++region) {
            for (int o = 0; o <= last_octave; ++o) {
                const uint32_t n = static_cast<uint32_t>(width >> o) *
                                   static_cast<uint32_t>(height >> o);
                for (int li = 0; li < kLevelsPerOctave; ++li) {
                    const int r = li < kKeypointLevels ? 0 : 1;
                    if (r != region) continue;
                    run[r] = (run[r] + kAlignElems - 1u) / kAlignElems * kAlignElems;
                    offs[static_cast<size_t>(o) * kLevelsPerOctave + li] = run[r];
                    run[r] += n;
                }
            }
        }
        if (keypoint_region_end) *keypoint_region_end = run[0];
        if (total_elems) *total_elems = run[0];                         
        if (hi_elems) *hi_elems = run[1];
        return offs;
    }
    std::vector<uint32_t> offs(
        static_cast<size_t>(last_octave + 1) * kLevelsPerOctave, 0u);
    uint32_t running = 0;
    const bool w256 = w256_enabled();
                                                                                              
                                                                                           
    for (int region = 0; region < (w256 ? 2 : 1); ++region) {
        for (int o = 0; o <= last_octave; ++o) {
            const uint32_t ow = static_cast<uint32_t>(width >> o);
            const uint32_t oh = static_cast<uint32_t>(height >> o);
            const uint32_t n = ow * oh;
            for (int li = 0; li < kLevelsPerOctave; ++li) {
                const bool in_a = !w256 || li < kKeypointLevels;
                if ((region == 0) != in_a) continue;
                running = (running + kAlignElems - 1u) / kAlignElems * kAlignElems;
                offs[static_cast<size_t>(o) * kLevelsPerOctave + li] = running;
                running += n;
            }
        }
        if (region == 0 && keypoint_region_end) *keypoint_region_end = running;
    }
    if (total_elems) *total_elems = running;
    return offs;
}

uint32_t SiftPyramidDawn::level_offset(int octave, int sublevel) const {
    return level_offsets_[static_cast<size_t>(octave) * kLevelsPerOctave +
                          (sublevel - kOctaveFirstSub)];
}

int SiftPyramidDawn::octave_width(int octave) const {
    return octaves_[static_cast<size_t>(octave)].width;
}

int SiftPyramidDawn::octave_height(int octave) const {
    return octaves_[static_cast<size_t>(octave)].height;
}

wgpu::Buffer SiftPyramidDawn::pack_levels(DawnKernelHarness& harness,
                                          std::vector<LevelMeta>* meta) const {
                                                                               
                                                                        
                                                                      
    const int num_octaves = last_octave_ + 1;
    const size_t num_levels =
        static_cast<size_t>(num_octaves) * kLevelsPerOctave;
    meta->assign(num_levels, LevelMeta{});

    uint32_t total = 0, kp_end = 0;
    const std::vector<uint32_t> offs =
        level_layout(width_, height_, last_octave_, &total, &kp_end);           
    for (int o = 0; o < num_octaves; ++o) {
        const uint32_t ow =
            static_cast<uint32_t>(octaves_[static_cast<size_t>(o)].width);
        const uint32_t oh =
            static_cast<uint32_t>(octaves_[static_cast<size_t>(o)].height);
        for (int li = 0; li < kLevelsPerOctave; ++li) {
            LevelMeta& m =
                (*meta)[static_cast<size_t>(o) * kLevelsPerOctave + li];
            m.offset = offs[static_cast<size_t>(o) * kLevelsPerOctave + li];
            m.width = ow;
            m.height = oh;
            m._pad = 0;
        }
    }

                                                                                   
                                                                        
    for (size_t i = 0; i < num_levels; ++i) {
        if ((*meta)[i].offset != level_offsets_[i]) {
            std::cerr << "[SiftPyramidDawn] pack layout drift at level " << i
                      << "\n";
            return wgpu::Buffer();
        }
    }
    (void)harness;
    return packed_buf_;
}

std::vector<float> SiftPyramidDawn::read_level(DawnKernelHarness& harness,
                                               int octave, int sublevel) const {
    const OctaveBuffers& ob = octaves_[static_cast<size_t>(octave)];
    const size_t bytes =
        static_cast<size_t>(ob.width) * ob.height * sizeof(float);
    wgpu::Buffer staging = harness.alloc_staging_for_readback(bytes);
    harness.copy_region(
        packed_buf_,
        static_cast<uint64_t>(level_offset(octave, sublevel)) * sizeof(float),
        staging, 0, bytes);
    std::vector<uint8_t> raw = harness.readback(staging, bytes);
    std::vector<float> out(static_cast<size_t>(ob.width) * ob.height);
    std::memcpy(out.data(), raw.data(), bytes);
    return out;
}

}                    
}                     
