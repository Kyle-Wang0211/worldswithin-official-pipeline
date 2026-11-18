                                          
#include "dense_runner.h"

#include <onnxruntime_cxx_api.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <random>
#include <unordered_map>
#if defined(PWDENSE_ENABLE_MODEL_SNAPSHOT)
#include <cerrno>
#include <filesystem>
#include <stdexcept>
#include <sys/stat.h>
#endif

namespace aether::dense {

                                                                                                                     
                                                                                                                 
                                                                                                                    
                                                                                                       
                                                                                                             
                                                                                                           
                                                                                                                         
                                                                                    
                                                                                                                              
                                                                                                                         
                                                                                                              
                                                                                                                     
                                                                                                            
                                                                                                              
                                                                                                      
                                                                                         
namespace {
struct Knobs {
    std::string validation, max_pending, default_cache, storage_cache, profile, log;
    bool verbose = false, int64 = false;
    std::string layout;                                                                                                                   
    static std::string env(const char* k) { const char* v = std::getenv(k); return v ? std::string(v) : std::string(); }
    static Knobs read() {
        Knobs k;
        k.validation = env("PWDENSE_ORT_VALIDATION"); k.max_pending = env("PWDENSE_ORT_MAX_PENDING");
        k.default_cache = env("PWDENSE_ORT_DEFAULT_CACHE"); k.storage_cache = env("PWDENSE_ORT_STORAGE_CACHE");
        k.profile = env("PWDENSE_ORT_PROFILE"); k.log = env("PWDENSE_ORT_LOG");
        k.verbose = env("PWDENSE_ORT_VERBOSE") == "1";
        k.int64 = env("PWDENSE_ORT_INT64") == "1";
        k.layout = env("PWDENSE_ORT_LAYOUT");
        return k;
    }
    bool any() const { return !validation.empty() || !max_pending.empty() || !default_cache.empty() || !storage_cache.empty() || !profile.empty() || !log.empty() || verbose || int64 || !layout.empty(); }
};
std::mutex g_log_mu;
std::string g_log_path;
void file_logger(void*          , OrtLoggingLevel severity, const char* category, const char* logid, const char* code_location, const char* message) {
    std::lock_guard<std::mutex> lk(g_log_mu);
    FILE* f = std::fopen(g_log_path.c_str(), "a"); if (!f) return;
    std::fprintf(f, "[%d] %s %s %s: %s\n", (int)severity, logid ? logid : "", category ? category : "", code_location ? code_location : "", message ? message : "");
    std::fclose(f);
}
void self_report(const Knobs& k) {
    if (k.log.empty()) return;
    std::lock_guard<std::mutex> lk(g_log_mu);
    FILE* f = std::fopen(k.log.c_str(), "a"); if (!f) return;
    std::fprintf(f, "pwdense ort knobs: validationMode=%s maxNumPendingDispatches=%s defaultBufferCacheMode=%s storageBufferCacheMode=%s profile=%s verbose=%d enableInt64=%d\n",
                 k.validation.empty() ? "(default)" : k.validation.c_str(), k.max_pending.empty() ? "(default)" : k.max_pending.c_str(),
                 k.default_cache.empty() ? "(default)" : k.default_cache.c_str(), k.storage_cache.empty() ? "(default)" : k.storage_cache.c_str(),
                 k.profile.empty() ? "(off)" : k.profile.c_str(), k.verbose ? 1 : 0, k.int64 ? 1 : 0);
    std::fclose(f);
}
std::unique_ptr<Ort::Env> make_env(const Knobs& k) {
    const OrtLoggingLevel lvl = k.verbose ? ORT_LOGGING_LEVEL_VERBOSE : ORT_LOGGING_LEVEL_WARNING;
    if (!k.log.empty()) { { std::lock_guard<std::mutex> lk(g_log_mu); g_log_path = k.log; } return std::make_unique<Ort::Env>(lvl, "pwdense", file_logger, nullptr); }
    return std::make_unique<Ort::Env>(lvl, "pwdense");
}
}              

                                                                                                                
                                                                                                            
                                                                 
                                                             
                                                                                                
enum class DenseRIn { Feature, RefImg, Pm1, Pm2, Pm3, Dv, N2, N3 };
struct DenseRBind { DenseRIn what = DenseRIn::RefImg; int slot = 0, level = 0; };
namespace {
                                                                                           
bool parse_feature_name(const std::string& nm, int n_view, int* slot, int* level) {
    static const char kPre[] = "/core/feature/out";
    static const char kSuf[] = "/Conv_output_0";
    const size_t pre = sizeof(kPre) - 1, suf = sizeof(kSuf) - 1;
    if (nm.size() <= pre + suf || nm.compare(0, pre, kPre) != 0 || nm.compare(nm.size() - suf, suf, kSuf) != 0) return false;
    const std::string mid = nm.substr(pre, nm.size() - pre - suf);                  
    if (mid.empty() || mid[0] < '1' || mid[0] > '3') return false;
    *level = mid[0] - '1';
    if (mid.size() == 1) { *slot = 0; return true; }
    if (mid[1] != '_' || mid.size() < 3) return false;
    int k = 0;
    for (size_t i = 2; i < mid.size(); ++i) { if (mid[i] < '0' || mid[i] > '9') return false; k = k * 10 + (mid[i] - '0'); }
    if (k < 1 || k >= n_view) return false;
    *slot = k;
    return true;
}
}              

struct DenseRunner::Impl {
    Knobs knobs = Knobs::read();
    std::unique_ptr<Ort::Env> env = make_env(knobs);
    std::unique_ptr<Ort::Session> sess;
    std::vector<std::string> in_names;
    std::vector<const char*> in_ptrs;
                 
    std::unique_ptr<Ort::Session> feat;                                
    std::vector<std::string> feat_in_names, feat_out_names;
    std::vector<const char*> feat_in_ptrs, feat_out_ptrs;
    std::vector<std::string> rest_in_names;                                                                      
    std::vector<const char*> rest_in_ptrs;
    std::vector<DenseRBind> rest_bind;
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
    std::unique_ptr<DenseViewSegments> view_segments;
#endif

                                                                                                               
                                     
    void configure(Ort::SessionOptions& so, bool webgpu, bool report) const {
        so.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);                                                             
        const Knobs& k = knobs;
        if (k.any()) {                                            
            if (!k.validation.empty())    so.AddConfigEntry("ep.webgpuexecutionprovider.validationMode", k.validation.c_str());
            if (!k.max_pending.empty())   so.AddConfigEntry("ep.webgpuexecutionprovider.maxNumPendingDispatches", k.max_pending.c_str());
            if (!k.default_cache.empty()) so.AddConfigEntry("ep.webgpuexecutionprovider.defaultBufferCacheMode", k.default_cache.c_str());
            if (!k.storage_cache.empty()) so.AddConfigEntry("ep.webgpuexecutionprovider.storageBufferCacheMode", k.storage_cache.c_str());
            if (!k.profile.empty())       so.EnableProfiling(k.profile.c_str());
            if (k.int64)                  so.AddConfigEntry("ep.webgpuexecutionprovider.enableInt64", "1");
            if (!k.layout.empty())        so.AddConfigEntry("ep.webgpuexecutionprovider.preferredLayout", k.layout.c_str());
            if (report) self_report(k);
        }
        if (webgpu) { std::unordered_map<std::string, std::string> opts; so.AppendExecutionProvider("WebGPU", opts); }
    }
};

DenseRunner::DenseRunner() : impl_(new Impl) {}
DenseRunner::~DenseRunner() { release(); }
void DenseRunner::release() {
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
                                                                           
    if (impl_) impl_->view_segments.reset();
#endif
    if (impl_) { impl_->sess.reset(); impl_->feat.reset(); }
    sess_ = nullptr;
}

bool DenseRunner::init(const std::string& model, bool webgpu, int W, int H, int n_view, std::string* err, double* session_ms) {
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
    release();
#endif
    try {
        Ort::SessionOptions so;
        impl_->configure(so, webgpu, true);
        const auto t0 = std::chrono::steady_clock::now();
        impl_->sess = std::make_unique<Ort::Session>(*impl_->env, model.c_str(), so);
        if (session_ms) *session_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        Ort::AllocatorWithDefaultOptions alloc;
        impl_->in_names.clear(); impl_->in_ptrs.clear();
        for (size_t i = 0; i < impl_->sess->GetInputCount(); ++i) impl_->in_names.push_back(impl_->sess->GetInputNameAllocated(i, alloc).get());
        for (auto& s : impl_->in_names) impl_->in_ptrs.push_back(s.c_str());
        W_ = W; H_ = H; nview_ = n_view; split_ = false; sess_ = impl_->sess.get();
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); release(); return false; }
}

#if defined(PWDENSE_ENABLE_MODEL_SNAPSHOT)
bool DenseRunner::snapshot_optimized_model(const std::string& model, const std::string& destination,
                                           bool webgpu, std::string* err, double* session_ms) {
    release();
    if (err) err->clear();
    if (session_ms) *session_ms = 0;
    try {
        if (impl_->knobs.any())
            throw std::runtime_error("model snapshot rejects unfrozen ORT environment overrides");
        if (model.find('\0') != std::string::npos)
            throw std::runtime_error("model snapshot source contains an embedded NUL");
        struct stat info {};
        if (::lstat(model.c_str(), &info) != 0 || !S_ISREG(info.st_mode))
            throw std::runtime_error("model snapshot source must be a regular, non-symlink file");
        const std::filesystem::path dest(destination);
        if (destination.empty() || destination.find('\0') != std::string::npos || dest.extension() != ".onnx")
            throw std::runtime_error("model snapshot destination must have an explicit .onnx suffix");
        if (::lstat(destination.c_str(), &info) == 0)
            throw std::runtime_error("model snapshot destination already exists");
        if (errno != ENOENT)
            throw std::runtime_error("model snapshot cannot inspect destination");
        const auto parent = std::filesystem::absolute(
            dest.has_parent_path() ? dest.parent_path() : std::filesystem::path("."));
        std::filesystem::path prefix;
        for (const auto& component : parent) {
            prefix /= component;
            if (::lstat(prefix.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
                throw std::runtime_error("model snapshot parent must exist as non-symlink directories");
        }
        Ort::SessionOptions so;
        impl_->configure(so, webgpu, true);
        so.SetOptimizedModelFilePath(destination.c_str());
        double initialized_ms = 0;
        {
            const auto t0 = std::chrono::steady_clock::now();
            Ort::Session snapshot(*impl_->env, model.c_str(), so);
            initialized_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
        }
        if (::lstat(destination.c_str(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size <= 0)
            throw std::runtime_error("model snapshot output must be a nonempty regular file");
        if (session_ms) *session_ms = initialized_ms;
        return true;
    } catch (const std::exception& e) {
        if (err) *err = e.what();
                                                                                               
        release();
        return false;
    }
}
#endif

#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
bool DenseRunner::init_view_segments(const std::vector<DenseViewSegmentPlan>& plan,
                                    DenseViewSegmentOptions options, int W, int H, int n_view,
                                    std::string* err, double* session_ms) {
    release();
    if (err) err->clear();
    try {
        if (W <= 0 || H <= 0 || W % 4 || H % 4 || n_view <= 0)
            throw std::runtime_error("invalid view-segment input dimensions");
        if (!options.final_outputs_cpu)
            throw std::runtime_error("DenseRunner requires final CPU outputs");
        if (impl_->knobs.any()) {
            const Knobs& k = impl_->knobs;
                                                                           
                                                                                 
            if (k.storage_cache.empty() || !k.validation.empty() || !k.max_pending.empty() ||
                !k.default_cache.empty() || !k.profile.empty() || k.int64 || !k.layout.empty())
                throw std::runtime_error("view-segment bench rejects unfrozen ORT environment overrides");
            if (!options.webgpu)
                throw std::runtime_error("view-segment storageBufferCacheMode requires WebGPU");
            if (k.storage_cache != "disabled" && k.storage_cache != "lazyRelease" &&
                k.storage_cache != "simple" && k.storage_cache != "bucket")
                throw std::runtime_error("invalid view-segment storageBufferCacheMode");
            if (!options.storage_buffer_cache_mode.empty() && options.storage_buffer_cache_mode != k.storage_cache)
                throw std::runtime_error("view-segment storageBufferCacheMode conflicts with ORT environment override");
            options.storage_buffer_cache_mode = k.storage_cache;
            self_report(k);                                                       
        }
        const auto t0 = std::chrono::steady_clock::now();
        auto next = std::make_unique<DenseViewSegments>();
        if (!next->init(plan, options, err)) return false;
        if (session_ms) *session_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        impl_->in_names = {"imgs", "pm_stage1", "pm_stage2", "pm_stage3",
                           "depth_values", "noise_stage2", "noise_stage3"};
        impl_->in_ptrs.clear();
        for (auto& name : impl_->in_names) impl_->in_ptrs.push_back(name.c_str());
        impl_->view_segments = std::move(next);
        W_ = W; H_ = H; nview_ = n_view; split_ = false;
        sess_ = impl_->view_segments.get();                                                      
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); release(); return false; }
}
#endif

void DenseRunner::feature_sizes(size_t n[3]) const {
    n[0] = (size_t)48 * (H_ / 8) * (W_ / 8);
    n[1] = (size_t)32 * (H_ / 4) * (W_ / 4);
    n[2] = (size_t)16 * (H_ / 2) * (W_ / 2);
}

bool DenseRunner::init_split(const std::string& feat_model, const std::string& rest_model, bool webgpu, int W, int H,
                             int n_view, std::string* err, double* session_ms) {
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
    release();
#endif
    try {
        const auto t0 = std::chrono::steady_clock::now();
        { Ort::SessionOptions so; impl_->configure(so, webgpu, true);
          impl_->feat = std::make_unique<Ort::Session>(*impl_->env, feat_model.c_str(), so); }
        { Ort::SessionOptions so; impl_->configure(so, webgpu, false);
          impl_->sess = std::make_unique<Ort::Session>(*impl_->env, rest_model.c_str(), so); }
        if (session_ms) *session_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        W_ = H_ = 0;                                                 
        Ort::AllocatorWithDefaultOptions alloc;
        impl_->feat_in_names.clear(); impl_->feat_in_ptrs.clear(); impl_->feat_out_names.clear(); impl_->feat_out_ptrs.clear();
        for (size_t i = 0; i < impl_->feat->GetInputCount(); ++i) impl_->feat_in_names.push_back(impl_->feat->GetInputNameAllocated(i, alloc).get());
        for (size_t i = 0; i < impl_->feat->GetOutputCount(); ++i) impl_->feat_out_names.push_back(impl_->feat->GetOutputNameAllocated(i, alloc).get());
        if (impl_->feat_in_names.size() != 1) { if (err) *err = "feature model must take exactly one input"; release(); return false; }
        if (impl_->feat_out_names.size() != 3) { if (err) *err = "feature model must produce three outputs"; release(); return false; }
        for (auto& s : impl_->feat_in_names) impl_->feat_in_ptrs.push_back(s.c_str());
        for (auto& s : impl_->feat_out_names) impl_->feat_out_ptrs.push_back(s.c_str());
                                                                                                                  
                                                                                                                   
        for (size_t i = 0; i < 3; ++i) {
            int slot = -1, level = -1;
            if (!parse_feature_name(impl_->feat_out_names[i], n_view, &slot, &level) || slot != 0 || level != (int)i) {
                if (err) *err = "unexpected feature model output name: " + impl_->feat_out_names[i]; release(); return false;
            }
        }
        impl_->rest_in_names.clear(); impl_->rest_in_ptrs.clear(); impl_->rest_bind.clear();
        for (size_t i = 0; i < impl_->sess->GetInputCount(); ++i) impl_->rest_in_names.push_back(impl_->sess->GetInputNameAllocated(i, alloc).get());
        for (auto& s : impl_->rest_in_names) impl_->rest_in_ptrs.push_back(s.c_str());
        int n_feat_in = 0;
        for (const std::string& nm : impl_->rest_in_names) {
            DenseRBind b;
            if (parse_feature_name(nm, n_view, &b.slot, &b.level)) { b.what = DenseRIn::Feature; ++n_feat_in; }
            else if (nm == "/Gather_output_0") b.what = DenseRIn::RefImg;
            else if (nm == "pm_stage1")        b.what = DenseRIn::Pm1;
            else if (nm == "pm_stage2")        b.what = DenseRIn::Pm2;
            else if (nm == "pm_stage3")        b.what = DenseRIn::Pm3;
            else if (nm == "depth_values")     b.what = DenseRIn::Dv;
            else if (nm == "noise_stage2")     b.what = DenseRIn::N2;
            else if (nm == "noise_stage3")     b.what = DenseRIn::N3;
            else { if (err) *err = "unknown rest-model input: " + nm; release(); return false; }
            impl_->rest_bind.push_back(b);
        }
        if (n_feat_in != n_view * 3) {
            if (err) *err = "rest model wants " + std::to_string(n_feat_in) + " feature inputs, expected " + std::to_string(n_view * 3);
            release(); return false;
        }
        W_ = W; H_ = H; nview_ = n_view; split_ = true; sess_ = impl_->sess.get();
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); release(); return false; }
}

bool DenseRunner::run_features(const float* img, FeatureSet& out, double* ms, std::string* err) {
    if (!sess_ || !split_ || !impl_->feat) { if (err) *err = "split session not initialised"; return false; }
    try {
        auto mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> sh{1, 3, H_, W_};
        Ort::Value in = Ort::Value::CreateTensor<float>(mi, const_cast<float*>(img), (size_t)3 * H_ * W_, sh.data(), sh.size());
        const auto t0 = std::chrono::steady_clock::now();
        auto res = impl_->feat->Run(Ort::RunOptions{nullptr}, impl_->feat_in_ptrs.data(), &in, 1,
                                    impl_->feat_out_ptrs.data(), impl_->feat_out_ptrs.size());
        if (ms) *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        size_t want[3]; feature_sizes(want);
        for (int k = 0; k < 3; ++k) {
            const size_t n = res[k].GetTensorTypeAndShapeInfo().GetElementCount();
            if (n != want[k]) {
                if (err) *err = "feature output " + impl_->feat_out_names[k] + " has " + std::to_string(n) + " elements, expected " + std::to_string(want[k]);
                return false;
            }
            const float* p = res[k].GetTensorData<float>();
            out.f[k].assign(p, p + n);
        }
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); return false; }
}

bool DenseRunner::run_rest(const FeatureSet* const* views, const float* ref_img, const FrameInputs& in,
                           const float* noise2, const float* noise3,
                           float* depth, float* conf0, float* conf1, float* conf2, double* ms, std::string* err) {
    if (!sess_ || !split_) { if (err) *err = "split session not initialised"; return false; }
    try {
        const size_t HW = (size_t)W_ * H_, n2 = (size_t)(H_ / 4) * (W_ / 4), n3 = (size_t)(H_ / 2) * (W_ / 2);
        size_t fsz[3]; feature_sizes(fsz);
        auto mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> sh_ref{1, 3, H_, W_}, sh_pm{1, nview_, 2, 4, 4}, sh_dv{1, DENSE_NUM_DEPTH},
                             sh_n2{1, 1, H_ / 4, W_ / 4}, sh_n3{1, 1, H_ / 2, W_ / 2};
        std::vector<int64_t> sh_f[3] = {{1, 48, H_ / 8, W_ / 8}, {1, 32, H_ / 4, W_ / 4}, {1, 16, H_ / 2, W_ / 2}};
        std::vector<Ort::Value> vals; vals.reserve(impl_->rest_bind.size());
        auto push = [&](const float* p, std::vector<int64_t>& s, size_t n) {
            vals.push_back(Ort::Value::CreateTensor<float>(mi, const_cast<float*>(p), n, s.data(), s.size()));
        };
        for (const DenseRBind& b : impl_->rest_bind) {                                                  
            switch (b.what) {
                case DenseRIn::Feature: {
                    const FeatureSet* fs = views[b.slot];
                    if (!fs || fs->f[b.level].size() != fsz[b.level]) { if (err) *err = "missing/short feature set for view slot " + std::to_string(b.slot); return false; }
                    push(fs->f[b.level].data(), sh_f[b.level], fsz[b.level]);
                    break;
                }
                case DenseRIn::RefImg: push(ref_img, sh_ref, (size_t)3 * HW); break;
                case DenseRIn::Pm1:    push(in.pm[0].data(), sh_pm, (size_t)nview_ * 32); break;
                case DenseRIn::Pm2:    push(in.pm[1].data(), sh_pm, (size_t)nview_ * 32); break;
                case DenseRIn::Pm3:    push(in.pm[2].data(), sh_pm, (size_t)nview_ * 32); break;
                case DenseRIn::Dv:     push(in.dv.data(), sh_dv, DENSE_NUM_DEPTH); break;
                case DenseRIn::N2:     push(noise2, sh_n2, n2); break;
                case DenseRIn::N3:     push(noise3, sh_n3, n3); break;
            }
        }
        const char* out_names[] = {"depth", "conf0", "conf1", "conf2"};
        const auto t0 = std::chrono::steady_clock::now();
        auto out = impl_->sess->Run(Ort::RunOptions{nullptr}, impl_->rest_in_ptrs.data(), vals.data(), vals.size(), out_names, 4);
        if (ms) *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        float* dst[4] = {depth, conf0, conf1, conf2};
        for (int k = 0; k < 4; ++k) {
            const size_t n = out[k].GetTensorTypeAndShapeInfo().GetElementCount();
            if (n != HW) { if (err) *err = "output " + std::string(out_names[k]) + " has " + std::to_string(n) + " elements, expected H*W"; return false; }
            const float* p = out[k].GetTensorData<float>();
            std::copy(p, p + HW, dst[k]);
        }
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); return false; }
}

bool DenseRunner::run(const float* imgs, const FrameInputs& in, const float* noise2, const float* noise3,
                      float* depth, float* conf0, float* conf1, float* conf2, double* ms, std::string* err) {
    if (!sess_) { if (err) *err = "session not initialised"; return false; }
    try {
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
        if (impl_->view_segments &&
            (!imgs || !noise2 || !noise3 || !depth || !conf0 || !conf1 || !conf2 ||
             in.n_view != nview_ || in.dv.size() != DENSE_NUM_DEPTH ||
             in.pm[0].size() != (size_t)nview_ * 32 ||
             in.pm[1].size() != (size_t)nview_ * 32 ||
             in.pm[2].size() != (size_t)nview_ * 32))
            throw std::runtime_error("invalid view-segment input storage");
#endif
        const size_t HW = (size_t)W_ * H_, n2 = (size_t)(H_ / 4) * (W_ / 4), n3 = (size_t)(H_ / 2) * (W_ / 2);
        auto mi = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> sh_img{1, nview_, 3, H_, W_}, sh_pm{1, nview_, 2, 4, 4}, sh_dv{1, DENSE_NUM_DEPTH},
                             sh_n2{1, 1, H_ / 4, W_ / 4}, sh_n3{1, 1, H_ / 2, W_ / 2};
        std::vector<Ort::Value> vals;
        auto push = [&](const float* p, std::vector<int64_t>& s, size_t n) {
            vals.push_back(Ort::Value::CreateTensor<float>(mi, const_cast<float*>(p), n, s.data(), s.size()));
        };
        for (auto& nm : impl_->in_names) {                                                                          
            if      (nm == "imgs")         push(imgs, sh_img, (size_t)nview_ * 3 * HW);
            else if (nm == "pm_stage1")    push(in.pm[0].data(), sh_pm, (size_t)nview_ * 32);
            else if (nm == "pm_stage2")    push(in.pm[1].data(), sh_pm, (size_t)nview_ * 32);
            else if (nm == "pm_stage3")    push(in.pm[2].data(), sh_pm, (size_t)nview_ * 32);
            else if (nm == "depth_values") push(in.dv.data(), sh_dv, DENSE_NUM_DEPTH);
            else if (nm == "noise_stage2") push(noise2, sh_n2, n2);
            else if (nm == "noise_stage3") push(noise3, sh_n3, n3);
            else { if (err) *err = "unknown model input: " + nm; return false; }
        }
        const char* out_names[] = {"depth", "conf0", "conf1", "conf2"};
        const auto t0 = std::chrono::steady_clock::now();
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
        if (impl_->view_segments) {
            std::vector<DenseViewNamedInput> named;
            for (size_t i = 0; i < vals.size(); ++i) named.push_back({impl_->in_names[i], &vals[i]});
            if (!impl_->view_segments->run(named, err)) return false;
            const auto out = impl_->view_segments->outputs();
            if (out.size() != 4) throw std::runtime_error("view-segment final output count differs");
            float* dst[4] = {depth, conf0, conf1, conf2};
                                                                         
            const Ort::Value* ordered[4] = {};
            for (int k = 0; k < 4; ++k) {
                for (const auto& value : out) if (value.name == out_names[k]) ordered[k] = value.value;
                if (!ordered[k] || !ordered[k]->IsTensor())
                    throw std::runtime_error("missing view-segment output: " + std::string(out_names[k]));
                const auto type = ordered[k]->GetTensorTypeAndShapeInfo();
                if (type.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
                    type.GetShape() != std::vector<int64_t>{1, H_, W_} ||
                    ordered[k]->GetTensorMemoryInfo().GetDeviceType() != OrtMemoryInfoDeviceType_CPU)
                    throw std::runtime_error("view-segment output dtype/shape/device differs");
            }
            for (int k = 0; k < 4; ++k)
                std::memcpy(dst[k], ordered[k]->GetTensorData<float>(), HW * sizeof(float));
            if (ms) *ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
            return true;
        }
#endif
        auto out = impl_->sess->Run(Ort::RunOptions{nullptr}, impl_->in_ptrs.data(), vals.data(), vals.size(), out_names, 4);
        if (ms) *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        float* dst[4] = {depth, conf0, conf1, conf2};
        for (int k = 0; k < 4; ++k) {
            const size_t n = out[k].GetTensorTypeAndShapeInfo().GetElementCount();
            if (n != HW) { if (err) *err = "output " + std::string(out_names[k]) + " has " + std::to_string(n) + " elements, expected H*W"; return false; }
            const float* p = out[k].GetTensorData<float>();
            std::copy(p, p + HW, dst[k]);
        }
        return true;
    } catch (const std::exception& e) { if (err) *err = e.what(); return false; }
}

void make_noise(uint64_t seed, int frame_id, size_t n2, size_t n3, std::vector<float>& noise2, std::vector<float>& noise3) {
    std::mt19937_64 gen(seed ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(frame_id + 1)));
    std::normal_distribution<float> N(0.f, 1.f);
    noise2.resize(n2); noise3.resize(n3);
    for (auto& v : noise2) v = N(gen);
    for (auto& v : noise3) v = N(gen);
}

}                            
