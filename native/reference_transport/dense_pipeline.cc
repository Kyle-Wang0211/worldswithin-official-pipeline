                                              
#include "dense_pipeline.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "bench_noise_input.h"
#include "dense_images.h"
#include "dense_inputs.h"
#include "dense_multires.h"
#include "dense_runner.h"
#include "reconstruction_original_dense_raw.h"

#if !defined(PWDENSE_ORIGINAL_NUMPY_CV2) || PWDENSE_ORIGINAL_NUMPY_CV2 != 1
#error This isolated mainline requires the original NumPy/OpenCV cache provider
#endif

namespace aether::dense {

namespace {
using clk = std::chrono::steady_clock;
double ms_since(clk::time_point t) { return std::chrono::duration<double, std::milli>(clk::now() - t).count(); }

                                                                                                                         
                                                                                                                       
                                                                                                   
thread_local std::string g_write_rows_diag;
int count_open_fds() { int n = 0; const int lim = (int)::getdtablesize(); for (int fd = 0; fd < lim; ++fd) if (::fcntl(fd, F_GETFD) != -1) ++n; return n; }
void record_write_rows_failure(const char* op, const std::string& path, long offset, size_t bytes, size_t written, int err, FILE* f) {
    struct statvfs v{}; const int vrc = f ? ::fstatvfs(::fileno(f), &v) : ::statvfs(path.c_str(), &v); const int verr = vrc == 0 ? 0 : errno;
    char buf[1024];
    std::snprintf(buf, sizeof buf, "write_rows op=%s path=%s offset=%ld bytes=%zu written=%zu errno=%d (%s) ferror=%d statvfs_rc=%d statvfs_errno=%d frsize=%llu bavail=%llu avail_bytes=%llu open_fds=%d fd_limit=%d",
                  op, path.c_str(), offset, bytes, written, err, std::strerror(err), f ? std::ferror(f) : -1, vrc, verr,
                  (unsigned long long)v.f_frsize, (unsigned long long)v.f_bavail, (unsigned long long)v.f_frsize * (unsigned long long)v.f_bavail,
                  count_open_fds(), (int)::getdtablesize());
    g_write_rows_diag = buf;
    std::fprintf(stderr, "[PWDENSE_IO] %s\n", buf);
}
bool write_rows(const std::string& path, const void* data, size_t bytes, long offset) {
    g_write_rows_diag.clear();
    errno = 0; FILE* f = std::fopen(path.c_str(), "r+b");
    if (!f) { record_write_rows_failure("fopen", path, offset, bytes, 0, errno, nullptr); return false; }
    errno = 0; if (std::fseek(f, offset, SEEK_SET) != 0) { record_write_rows_failure("fseek", path, offset, bytes, 0, errno, f); std::fclose(f); return false; }
    errno = 0; const size_t w = std::fwrite(data, 1, bytes, f); const int werr = errno;
    if (w != bytes) { record_write_rows_failure("fwrite", path, offset, bytes, w, werr, f); std::fclose(f); return false; }
    errno = 0; if (std::fclose(f) != 0) { record_write_rows_failure("fclose", path, offset, bytes, w, errno, nullptr); return false; }
    return true;
}
                                                                                                                         
                                                                        
bool pwrite_all(int fd, const void* data, size_t bytes, off_t off) {
    const uint8_t* p = (const uint8_t*)data;
    while (bytes) {
        const ssize_t k = ::pwrite(fd, p, bytes, off);
        if (k <= 0) {
            const int saved_errno = errno;
            std::fprintf(stderr, "[PWDENSE_IO] pwrite_failed fd=%d offset=%lld remaining_bytes=%llu k=%lld errno=%d error=%s\n",
                         fd, (long long)off, (unsigned long long)bytes, (long long)k, saved_errno, std::strerror(saved_errno));
            struct statvfs volume{};
            const int volume_rc = ::fstatvfs(fd, &volume);
            const int volume_errno = errno;
            std::fprintf(stderr, "[PWDENSE_IO] pwrite_volume fd=%d statvfs_rc=%d statvfs_errno=%d frsize=%llu bavail=%llu available_bytes=%llu\n",
                         fd, volume_rc, volume_rc == 0 ? 0 : volume_errno,
                         (unsigned long long)volume.f_frsize, (unsigned long long)volume.f_bavail,
                         (unsigned long long)volume.f_frsize * (unsigned long long)volume.f_bavail);
            errno = saved_errno;
            return false;
        }
        p += k; bytes -= (size_t)k; off += k;
    }
    return true;
}
bool pread_all(int fd, void* data, size_t bytes, off_t off) {
    uint8_t* p = (uint8_t*)data;
    while (bytes) { const ssize_t k = ::pread(fd, p, bytes, off); if (k <= 0) return false; p += k; bytes -= (size_t)k; off += k; }
    return true;
}
struct Fd {                               
    int fd = -1;
    Fd() = default; Fd(const Fd&) = delete; Fd& operator=(const Fd&) = delete;
    ~Fd() { if (fd >= 0) ::close(fd); }
};
                                                                           
struct OriginalReadMap {
    void* data = MAP_FAILED;
    size_t bytes = 0;
    OriginalReadMap() = default;
    OriginalReadMap(const OriginalReadMap&) = delete;
    OriginalReadMap& operator=(const OriginalReadMap&) = delete;
    ~OriginalReadMap() { if (data != MAP_FAILED) ::munmap(data, bytes); }
    bool open(const std::string& path, size_t expected) {
        Fd file; file.fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW);
        struct stat info{};
        if (file.fd < 0 || ::fstat(file.fd, &info) != 0 || !S_ISREG(info.st_mode) ||
            info.st_size < 0 || static_cast<uint64_t>(info.st_size) != expected || !expected) return false;
        data = ::mmap(nullptr, expected, PROT_READ, MAP_PRIVATE, file.fd, 0);
        if (data == MAP_FAILED) return false;
        bytes = expected; return true;
    }
};

                                                                                                             
                                                              
struct RefRecord { bool done = false; uint64_t hs[5] = {0, 0, 0, 0, 0}; uint64_t digest = 0; double ph = 0, ge = 0, fi = 0; size_t n = 0; };

bool create_zero_file(const std::string& path, size_t bytes) {
    FILE* f = std::fopen(path.c_str(), "wb"); if (!f) return false;
    const bool ok = bytes == 0 || (std::fseek(f, (long)bytes - 1, SEEK_SET) == 0 && std::fputc(0, f) != EOF);
    std::fclose(f); return ok;
}

                                                                                                                
                                                                                                                 
                                                         
                                                                                                          
                                                                                                
                                                                                                           
                                                                                                      
                                                                          
                                                                                                             
                                                                                                   
                                                                                                           
                                                                                               
                                                                                                    
  
                                                                                                         
                                                                                                             
                                                                                                             
                                                                                                     
                                                                                                               
constexpr unsigned kDecodeThreadCap = 4;
                                                                                                                
                                                                               
                                                                                                  
                                                                                                                
                                                                                                                
                                                                                      
bool env_flag(const char* k) { const char* v = std::getenv(k); return v && v[0] == '1' && v[1] == 0; }
unsigned decode_threads(size_t n_images) {
    const unsigned hw = std::thread::hardware_concurrency();
    unsigned n = hw ? hw : 1u;
    if (n > kDecodeThreadCap) n = kDecodeThreadCap;
    if (n_images && (size_t)n > n_images) n = (unsigned)n_images;
    return n ? n : 1u;
}

                                                                                                       
                                                                                                       
                                                                                                           
                                                                                               
unsigned fusion_threads(size_t pixels, size_t refs) {
    constexpr size_t budget = size_t{768} << 20;
    constexpr size_t fixed = size_t{4} << 20;
    const size_t slots = pixels > (budget - fixed) / 192 ? 1 : budget / (pixels * 192 + fixed);
    const unsigned hw = std::thread::hardware_concurrency();
    unsigned n = (unsigned)std::min({size_t(hw ? hw : 1u), refs, std::max(size_t{1}, slots)});
                                                                                                        
    const char* v = std::getenv("PWDENSE_PIPE_FUSE_THREADS");
    if (v && *v) {
        char* end = nullptr; const unsigned long requested = std::strtoul(v, &end, 10);
        if (end != v && !*end && requested > 0 && requested < n) n = (unsigned)requested;
    }
    return n ? n : 1u;
}

                                                                                                                 
                                                                                                                
                                                                                          
void bank_to_img3(const std::vector<uint16_t>& g, size_t N, float* dst) {
    for (size_t i = 0; i < N; ++i) { const float fv = fp16_to_fp32(g[i]); for (int c = 0; c < 3; ++c) dst[(size_t)c * N + i] = fv; }
}

                                                                                                        
struct Joiner {
    std::thread& t;
    explicit Joiner(std::thread& th) : t(th) {}
    Joiner(const Joiner&) = delete; Joiner& operator=(const Joiner&) = delete;
    ~Joiner() { if (t.joinable()) t.join(); }
};

                                                                                                         
                                                                                              
struct FuseOut { int frame = -1; FuseFrameResult fr; };
struct FuseQ {
    std::mutex m;
    std::condition_variable cv_task, cv_res;
    std::deque<int> tasks;
    std::deque<FuseOut> res;
    bool closed = false;                                  
    bool done = false;                               
    bool enabled = true;                                                                           
    bool stop = false;                         
    size_t active = 0, delivering = 0, peak_outstanding = 0;
    unsigned workers_left = 0;
    int rc = 0;                                                                                        
    std::string err = "worker error";                                                              
    double busy_ms = 0;                                                          
};
                                                                                                          
                                                                               
struct FuseStopper {
    FuseQ& q; std::vector<std::thread>& threads; DenseStats& stats;
    FuseStopper(FuseQ& qq, std::vector<std::thread>& th, DenseStats& ss) : q(qq), threads(th), stats(ss) {}
    FuseStopper(const FuseStopper&) = delete; FuseStopper& operator=(const FuseStopper&) = delete;
    ~FuseStopper() {
        { std::lock_guard<std::mutex> lk(q.m); q.stop = true; q.closed = true; }
        q.cv_task.notify_all(); q.cv_res.notify_all();
        for (auto& t : threads) if (t.joinable()) t.join();
        stats.fuse_peak_outstanding = q.peak_outstanding;                                                    
    }
};
}              

int dense_run(const DenseJob& job, dense_progress_fn progress, dense_chunk_fn chunk, void* user, DenseStats* st) {
    DenseStats scratch; DenseStats& S = st ? *st : scratch;
    if (job.frames.empty() || job.jpeg_paths.size() != job.frames.size() || job.points.size() % 3) { S.error = "bad job inputs"; return 2; }
    if (chunk || job.full_pipeline_fused_depth_dir.empty() ||
        job.original_fusion_jpeg_paths.size() != job.frames.size() || !job.table_hook) {
        S.error = "original cache producer requires frozen tables, original RGB paths and no progressive PLY";
        return 2;
    }
    const int W = job.session.W, H = job.session.H, NS = job.session.nsrc, NV = NS + 1;
    const size_t N = (size_t)W * H, n2 = (size_t)(H / 4) * (W / 4), n3 = (size_t)(H / 2) * (W / 2);
    const long NP = (long)(job.points.size() / 3);
    if (job.ext_noise_fd < -1 || job.ext_noise_hi_fd < -1 ||
        (job.ext_noise_fd >= 0 && job.ext_noise) ||
        (job.ext_noise_hi_fd >= 0 && job.ext_noise_hi) ||
        (job.ext_noise_hi_fd >= 0 && job.hi_model_path.empty())) {
        S.error = "conflicting frozen noise input sources"; return 2;
    }
                                                                            
    for (int pass = 0; pass < 2; ++pass) {
        const int fd = pass ? job.ext_noise_hi_fd : job.ext_noise_fd;
        if (fd < 0) continue;
        const size_t width = pass ? size_t(W) * 2 : size_t(W);
        const size_t height = pass ? size_t(H) * 2 : size_t(H);
        size_t bytes = 0;
        if (!BenchNoiseInput::expected_bytes(job.frames.size(), width, height, &bytes, &S.error) ||
            !BenchNoiseFileInput::validate_fd(fd, bytes, &S.error)) return 2;
    }

#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
    const bool retain_full_pipeline = !job.full_pipeline_fused_depth_dir.empty();
    if (retain_full_pipeline && (job.has_box || (job.frames.empty() || job.frames.size() > 97) || job.ref_begin != 0 ||
        (job.ref_end != -1 && job.ref_end != static_cast<int>(job.frames.size()) - 1) || W != 768 || H != 576 || NS != 9 ||
        job.hi_model_path.empty() || job.fuse.check_all_views != 1 ||
        job.fuse.geo_mask_thres != 2 || job.fuse.geo_pixel_thres != 0.125f || job.fuse.geo_depth_thres != 0.01f ||
        job.fuse.sat_tol != 1e-5f || job.fuse.min_tri_angle_deg != 3.0f ||
        job.fuse.photo_thres[0] != 0.3f || job.fuse.photo_thres[1] != 0.5f || job.fuse.photo_thres[2] != 0.5f)) {
        S.error = "full_pipeline depth retention requires the complete selected actual-N recipe (capacity97)"; return 2;
    }
#endif

                                                                                                             
                                                                                                              
    auto t = clk::now();
    if (progress && progress("session", 0, 1, user)) return 1;
    std::vector<SessionFrame> frames = job.frames; std::vector<std::string> jpegs = job.jpeg_paths;
    std::vector<int> orig(frames.size()); for (size_t i = 0; i < orig.size(); ++i) orig[i] = (int)i;                               
    if (job.table_hook && (job.has_box || job.table_hook->NF != (int)frames.size())) { S.error = "bad table hook"; return 2; }
    SessionTable tab; if (job.table_hook) tab = *job.table_hook; else build_session_table(frames, job.points, job.session, tab);
    S.NF = (int)job.frames.size(); S.frames_selected = S.NF;
    BoxFilter box{}; const BoxFilter* boxp = nullptr;
    if (job.has_box) {
        for (int k = 0; k < 3; ++k) { box.c[k] = job.box_c[k]; box.s[k] = job.box_s[k]; }
        for (int k = 0; k < 9; ++k) box.rot[k] = job.box_rot[k];
        boxp = &box;
        std::vector<uint8_t> inside(NP, 0);
        for (long n = 0; n < NP; ++n) inside[n] = box.contains(job.points[n * 3], job.points[n * 3 + 1], job.points[n * 3 + 2]);
        std::vector<int> keep;
        for (int i = 0; i < tab.NF; ++i) {
            const uint8_t* v = &tab.vis[(size_t)i * NP];
            for (long n = 0; n < NP; ++n) if (v[n] && inside[n]) { keep.push_back(i); break; }
        }
        if ((int)keep.size() >= NV) {
            std::vector<SessionFrame> f2; std::vector<std::string> j2; std::vector<int> o2;
            for (int i : keep) { f2.push_back(frames[i]); j2.push_back(jpegs[i]); o2.push_back(orig[i]); }
            frames.swap(f2); jpegs.swap(j2); orig.swap(o2);
            SessionTable t2; build_session_table(frames, job.points, job.session, t2); tab = std::move(t2);
        } else {
            S.box_fallback = true;                                                                                             
        }
        S.frames_selected = (int)frames.size();
    }
    const int NF = (int)frames.size();
    S.session_ms = ms_since(t);
    if (progress && progress("session", 1, 1, user)) return 1;

                                                                                                                       
                                              
    const int r0 = job.ref_begin, r1 = job.ref_end < 0 ? NF - 1 : std::min(job.ref_end, NF - 1);
    if (r0 < 0 || r0 > r1) { S.error = "bad ref range"; return 2; }
    std::set<int> infer, imgs;
    for (int f = r0; f <= r1; ++f) { infer.insert(f); for (int j = 0; j < NS; ++j) infer.insert(tab.neighbors[(size_t)f * NS + j]); }
    for (int v : infer) { imgs.insert(v); for (int j = 0; j < NS; ++j) imgs.insert(tab.neighbors[(size_t)v * NS + j]); }

                                                                                                                            
                                                                                                                              
                                                                                                                     
                                                                  
    const bool mr = !job.hi_model_path.empty();
    const int WF = mr ? 2 * W : W, HF = mr ? 2 * H : H;
    const size_t NG = (size_t)WF * HF, n2h = (size_t)(HF / 4) * (WF / 4), n3h = (size_t)(HF / 2) * (WF / 2);
    std::vector<float> cams_hi;
    if (mr) {
        cams_hi = tab.cams;
        for (int i = 0; i < NF; ++i) {
            float* c = &cams_hi[(size_t)i * 36];
            for (int k : {0, 2, 4, 5}) c[k] *= 2.0f;
            c[26] = (float)WF; c[27] = (float)HF;
        }
    }
    const float* cams_pack = mr ? cams_hi.data() : tab.cams.data();
    const int total_infer_views = (mr ? 2 : 1) * (int)infer.size();                                         

                                                                                                                          
                                                                                                                 
    mkdir(job.work_dir.c_str(), 0755);
    std::vector<int> local; for (int f = r0; f <= r1; ++f) local.push_back(f);
    for (int v : infer) if (v < r0 || v > r1) local.push_back(v);
    std::vector<int> lidx(NF, -1); for (size_t i = 0; i < local.size(); ++i) lidx[local[i]] = (int)i;
    const int NL = (int)local.size();
    const std::string pack = job.work_dir;
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
    FusedDepthRetention retained_depth;
    const FusedDepthRetention* retention = nullptr;
    if (retain_full_pipeline) {
        retained_depth.dir = job.full_pipeline_fused_depth_dir;
        for (int frame : local) {
            retained_depth.original_indices.push_back(orig[frame]);
            retained_depth.frame_ids.push_back(frames[frame].frame_id);
        }
                                                                                 
                                                                           
        retention = nullptr;
    }
#endif
    for (const char* nm : {"depth.f32", "conf0.f32", "conf1.f32", "conf2.f32"}) if (!create_zero_file(pack + "/" + nm, (size_t)NL * NG * 4)) { S.error = "cannot create pack"; return 2; }
                                                                                                                               
                                                                                                          
    Fd bank_hi, rgb_fd, lo_fd[4];
    const char* lo_names[4] = {"lo_depth.f32", "lo_conf0.f32", "lo_conf1.f32", "lo_conf2.f32"};
    if (mr) {
        bool ok = create_zero_file(pack + "/bank_hi.f16", (size_t)NF * NG * 2) && create_zero_file(pack + "/rgb.u8", (size_t)NL * NG * 3);
        for (int k = 0; k < 4 && ok; ++k) ok = create_zero_file(pack + "/" + lo_names[k], (size_t)NL * N * 4);
        if (ok) { bank_hi.fd = ::open((pack + "/bank_hi.f16").c_str(), O_RDWR); rgb_fd.fd = ::open((pack + "/rgb.u8").c_str(), O_RDWR); }
        for (int k = 0; k < 4 && ok; ++k) { lo_fd[k].fd = ::open((pack + "/" + lo_names[k]).c_str(), O_RDWR); ok = lo_fd[k].fd >= 0; }
        if (!ok || bank_hi.fd < 0 || rgb_fd.fd < 0) { S.error = "cannot create 2x side files"; return 2; }
    }

                                                                                                             
                                                                                                           
                                                                                               
      
                                                                                                          
                                                                                                           
    const bool split_mode = !job.feat_model_path.empty() && !job.rest_model_path.empty();
    if (mr && split_mode) { S.error = "hi_model_path is not supported in split mode"; return 2; }
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
    if (split_mode && (!job.view_segment_plan.empty() || !job.view_segment_plan_hi.empty())) {
        S.error = "view segments cannot be combined with feature-reuse mode"; return 2;
    }
    if (!mr && !job.view_segment_plan_hi.empty()) { S.error = "hi view plan without hi model"; return 2; }
#endif
    auto run = std::make_unique<DenseRunner>();
    bool ort_ok = false; std::string ort_err;
    auto ort_init = [&] {
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
        if (!job.view_segment_plan.empty()) {
            DenseViewSegmentOptions options;
            options.webgpu = job.webgpu;
            options.intra_op_threads = job.view_segment_intra_op_threads;
            options.native_output_locations = job.view_segment_native_output_locations;
            options.disable_cpu_mem_arena = job.view_segment_disable_cpu_mem_arena;
            options.disable_graph_optimizations = job.view_segment_disable_graph_optimizations;
            ort_ok = run->init_view_segments(job.view_segment_plan, options, W, H, NV,
                                           &ort_err, &S.ort_session_ms);
            return;
        }
#endif
        ort_ok = split_mode ? run->init_split(job.feat_model_path, job.rest_model_path, job.webgpu, W, H, NV, &ort_err, &S.ort_session_ms)
                            : run->init(job.model_path, job.webgpu, W, H, NV, &ort_err, &S.ort_session_ms);
    };
    std::thread ort_thread;
    if (env_flag("PWDENSE_PIPE_SYNC_ORT")) ort_init();                                          
    else ort_thread = std::thread(ort_init);
    Joiner ort_join(ort_thread);
    bool ort_joined = false;
    auto ensure_ort = [&]() -> bool {                                                                  
        if (!ort_joined) { if (ort_thread.joinable()) ort_thread.join(); ort_joined = true; }
        return ort_ok;
    };

    {
        const int saved_errno = errno;
        struct statvfs volume{};
        const int volume_rc = ::statvfs(pack.c_str(), &volume);
        const int volume_errno = errno;
        std::fprintf(stderr, "[PWDENSE_IO] before_decode pack=%s statvfs_rc=%d statvfs_errno=%d frsize=%llu bavail=%llu available_bytes=%llu\n",
                     pack.c_str(), volume_rc, volume_rc == 0 ? 0 : volume_errno,
                     (unsigned long long)volume.f_frsize, (unsigned long long)volume.f_bavail,
                     (unsigned long long)volume.f_frsize * (unsigned long long)volume.f_bavail);
        errno = saved_errno;
    }

                                                                                                
                                                                                                            
                                                                                                      
                                                                                                              
                                                                                    
    t = clk::now();
    std::vector<std::vector<uint16_t>> bank(NF);
    std::vector<std::vector<uint8_t>> rgb(NF);
    const std::vector<int> img_list(imgs.begin(), imgs.end());
    {
        const unsigned nthr = decode_threads(img_list.size());
        S.image_threads = (int)nthr;
        std::mutex mu; std::condition_variable cv;
        size_t next = 0; int ndone = 0; bool failed = false, stop = false; std::string ferr;
        auto worker = [&] {
            for (;;) {
                size_t i;
                { std::lock_guard<std::mutex> lk(mu); if (stop || failed || next >= img_list.size()) return; i = next++; }
                const int v = img_list[i];
                FrameSource fs;
                if (!job.sources.empty()) fs = job.sources[(size_t)orig[v]];
                else { fs.kind = FrameSourceKind::Jpeg; fs.path = jpegs[v]; }
                RgbImage src, rs; std::vector<uint8_t> gray; std::string e;
                if (!load_frame_rgb(fs, src, &e)) {
                    { std::lock_guard<std::mutex> lk(mu); if (!failed) { failed = true; ferr = e; } }
                    cv.notify_all(); return;
                }
                pil_resize_bilinear_rgb(src, W, H, rs);
                pil_rgb_to_l(rs, gray); gray_to_f16(gray, bank[v]);
                if (mr) {                                                                                                            
                    std::vector<uint8_t> lf, lh; std::vector<uint16_t> fh;
                    pil_rgb_to_l(src, lf); pil_resize_bilinear_l(lf, src.w, src.h, WF, HF, lh); gray_to_f16(lh, fh);
                    bool ok = pwrite_all(bank_hi.fd, fh.data(), NG * 2, (off_t)((size_t)v * NG * 2));
                    if (ok && lidx[v] >= 0) {
                        RgbImage rh; pil_resize_bilinear_rgb(src, WF, HF, rh);
                        ok = pwrite_all(rgb_fd.fd, rh.rgb.data(), NG * 3, (off_t)((size_t)lidx[v] * NG * 3));
                    }
                    if (!ok) {
                        { std::lock_guard<std::mutex> lk(mu); if (!failed) { failed = true; ferr = "cannot write the 2x image bank"; } }
                        cv.notify_all(); return;
                    }
                } else if (infer.count(v)) rgb[v] = std::move(rs.rgb);
                { std::lock_guard<std::mutex> lk(mu); ++ndone; }
                cv.notify_all();
            }
        };
        std::vector<std::thread> pool; pool.reserve(nthr);
        for (unsigned k = 0; k < nthr; ++k) pool.emplace_back(worker);
        int rc = 0, reported = -1;
        {   // progress is reported from the CALLING thread as results land; the workers only bump a counter
            std::unique_lock<std::mutex> lk(mu);
            for (;;) {
                if (failed) { rc = 2; break; }
                if (ndone != reported) {
                    reported = ndone; const int d = reported;
                    lk.unlock();
                    const int cancel = progress ? progress("images", d, (int)img_list.size(), user) : 0;
                    lk.lock();
                    if (cancel) { rc = 1; stop = true; break; }
                    continue;
                }
                if (ndone >= (int)img_list.size()) break;
                cv.wait(lk);
            }
        }
        cv.notify_all();
        for (auto& th : pool) th.join();
        if (rc == 1) return 1;
        if (rc == 2) { S.error = ferr.empty() ? std::string("image decode failed") : ferr; return 2; }
    }
    S.images = (int)img_list.size(); S.images_ms = ms_since(t);

                                                                                 
                                                                                                               
                                                                                                                   
                                                                                                                
                                                                                                                 
                                                                                                              
                                                                                   
                                                                                                               
    //     WebGPU Run(); progress is therefore reported from the calling thread like every other phase.
                                                                                                             
                                                                                                              
                                                                                                                
                                                                  
    std::vector<FeatureSet> feat;
    if (split_mode) {
        if (!ensure_ort()) { S.error = "ort init: " + ort_err; return 3; }
        t = clk::now();
        feat.resize(NF);
        std::vector<float> img3((size_t)3 * N);
        std::string ferr;
        for (size_t i = 0; i < img_list.size(); ++i) {
            if (progress && progress("features", (int)i, (int)img_list.size(), user)) return 1;
            const int v = img_list[i];
            bank_to_img3(bank[v], N, img3.data());
            double dt = 0;
            if (!run->run_features(img3.data(), feat[v], &dt, &ferr)) { S.error = "ort features: " + ferr; return 3; }
            (void)dt;
        }
        S.feat_count = (int)img_list.size();
        S.feat_ms_total = ms_since(t);
        if (progress && progress("features", (int)img_list.size(), (int)img_list.size(), user)) return 1;
    }

                                                                                                                  
    {
        std::vector<uint8_t> rgbpack(mr ? 0 : (size_t)NL * N * 3, 0); std::vector<float> campack((size_t)NL * 36); std::vector<int32_t> nbpack((size_t)NL * NS, 0);
        for (int i = 0; i < NL; ++i) {
            const int v = local[i];
            if (!mr && !rgb[v].empty()) std::copy(rgb[v].begin(), rgb[v].end(), rgbpack.begin() + (size_t)i * N * 3);
            std::copy(cams_pack + (size_t)v * 36, cams_pack + (size_t)(v + 1) * 36, campack.begin() + (size_t)i * 36);
            for (int j = 0; j < NS; ++j) { const int s = tab.neighbors[(size_t)v * NS + j]; nbpack[(size_t)i * NS + j] = lidx[s] >= 0 ? lidx[s] : 0; }
        }
        FILE* f;
        if (!mr) { f = std::fopen((pack + "/rgb.u8").c_str(), "wb"); std::fwrite(rgbpack.data(), 1, rgbpack.size(), f); std::fclose(f); }
        f = std::fopen((pack + "/cams.f32").c_str(), "wb"); std::fwrite(campack.data(), 4, campack.size(), f); std::fclose(f);
        f = std::fopen((pack + "/neighbors.i32").c_str(), "wb"); std::fwrite(nbpack.data(), 4, nbpack.size(), f); std::fclose(f);
        f = std::fopen((pack + "/meta.txt").c_str(), "w");
        std::fprintf(f, "%d %d %d %d %d %.9g %.9g %.9g %.9g %.9g", NL, WF, HF, NS, job.fuse.geo_mask_thres, job.fuse.geo_pixel_thres, job.fuse.geo_depth_thres,
                     job.fuse.photo_thres[0], job.fuse.photo_thres[1], job.fuse.photo_thres[2]);
                                                                                                                            
                                                                         
        const FuseParams& fp = job.fuse;
        if (fp.check_all_views || fp.sat_tol != 0.f || fp.min_tri_angle_deg != 0.f || fp.photo_first)
            std::fprintf(f, " %d %.9g %.9g %d", fp.check_all_views, fp.sat_tol, fp.min_tri_angle_deg, fp.photo_first);
        std::fprintf(f, "\n");
        std::fclose(f);
    }

                                                                                                         
                                                                                                             
                                                                                                              
                                                                                                      
                                                                                                             
                                                                                                             
    {
        if (!ensure_ort()) { S.error = "ort init: " + ort_err; return 3; }
        std::string err;
                                                                                                            
                                                                                                            
                                                                                                              
                                                                                                               
                                                                               
          
                                                                                                        
                                                                                                                     
                                                                                     
                                                                                                                          
                                                                                                                         
                                                                                                                     
                                                                                                    
        struct Slot { std::vector<float> imgbuf; FrameInputs in; std::vector<float> nz2, nz3; const float *p2 = nullptr, *p3 = nullptr; std::vector<int32_t> views; };
        const int nfill = split_mode ? 1 : NV;
                                                                                                                  
                                                                                                                  
        const bool neg_swap12 = split_mode && env_flag("PWDENSE_SPLIT_SWAP12") && NS >= 2;
        const std::vector<int> infer_list(infer.begin(), infer.end());
        const bool no_prefetch = env_flag("PWDENSE_PIPE_NO_PREFETCH");
        const int npass = mr ? 2 : 1, total_all = npass * (int)infer_list.size();
                                                                                                                         
        const bool dump_side = mr && env_flag("PWDENSE_KEEP_SIDE");
        if (dump_side) for (int k = 0; k < 4; ++k) if (!create_zero_file(pack + "/hi_" + std::string(lo_names[k] + 3), (size_t)NL * NG * 4)) { S.error = "hi dump"; return 2; }
        int done_all = 0;
        for (int pass = 0; pass < npass; ++pass) {
            const bool lo_pass = mr && pass == 0, hi_pass = mr && pass == 1;
            const size_t PN = hi_pass ? NG : N, p2n = hi_pass ? n2h : n2, p3n = hi_pass ? n3h : n3;
            const float* pcams = hi_pass ? cams_hi.data() : tab.cams.data();
            const float* pext = hi_pass ? job.ext_noise_hi : job.ext_noise;
            const int noise_fd = hi_pass ? job.ext_noise_hi_fd : job.ext_noise_fd;
            if (hi_pass) {                                                                                     
                run.reset();
                try {
                    run = std::make_unique<DenseRunner>();
                } catch (const std::exception& e) { S.error = std::string("ort init (hi): ") + e.what(); return 3; }
                catch (...) { S.error = "ort init (hi): unknown exception"; return 3; }
#if defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
                bool hi_ok;
                if (!job.view_segment_plan_hi.empty()) {
                    DenseViewSegmentOptions options;
                    options.webgpu = job.webgpu;
                    options.intra_op_threads = job.view_segment_intra_op_threads;
                    options.native_output_locations = job.view_segment_native_output_locations;
                    options.disable_cpu_mem_arena = job.view_segment_disable_cpu_mem_arena;
                    options.disable_graph_optimizations = job.view_segment_disable_graph_optimizations;
                    hi_ok = run->init_view_segments(job.view_segment_plan_hi, options, WF, HF, NV,
                                                  &ort_err, &S.ort_hi_session_ms);
                } else hi_ok = run->init(job.hi_model_path, job.webgpu, WF, HF, NV, &ort_err, &S.ort_hi_session_ms);
                if (!hi_ok) { S.error = "ort init (hi): " + ort_err; return 3; }
#else
                if (!run->init(job.hi_model_path, job.webgpu, WF, HF, NV, &ort_err, &S.ort_hi_session_ms)) { S.error = "ort init (hi): " + ort_err; return 3; }
#endif
            }
            Slot slot[2];
            const size_t slot_count = no_prefetch ? 1 : 2;
            for (size_t k = 0; k < slot_count; ++k) slot[k].imgbuf.assign((size_t)nfill * 3 * PN, 0.0f);
            std::string prep_err;                                                       
            auto prep = [&](int v, Slot& s) {
                std::vector<int32_t> views{v}; for (int j = 0; j < NS; ++j) views.push_back(tab.neighbors[(size_t)v * NS + j]);
                if (hi_pass) {
                    std::vector<uint16_t> g(NG);
                    for (int k = 0; k < nfill; ++k) {
                        if (!pread_all(bank_hi.fd, g.data(), NG * 2, (off_t)((size_t)views[k] * NG * 2))) { prep_err = "cannot read the 2x image bank"; return; }
                        bank_to_img3(g, NG, s.imgbuf.data() + (size_t)k * 3 * NG);
                    }
                } else {
                    for (int k = 0; k < nfill; ++k) bank_to_img3(bank[views[k]], N, s.imgbuf.data() + (size_t)k * 3 * N);
                }
                build_frame_inputs(pcams, views, s.in);
                s.views = views;
                if (neg_swap12) std::swap(s.views[1], s.views[2]);

                if (noise_fd >= 0) {
                    if (!BenchNoiseFileInput::read_frame(noise_fd, (size_t)orig[v], job.frames.size(),
                                                        p2n, p3n, s.nz2, s.nz3, &prep_err)) return;
                    s.p2 = s.nz2.data(); s.p3 = s.nz3.data();
                }
                else if (pext) { s.p2 = pext + (size_t)orig[v] * (p2n + p3n); s.p3 = s.p2 + p2n; }                                      
                else { make_noise(job.noise_seed, (int)frames[v].frame_id, p2n, p3n, s.nz2, s.nz3); s.p2 = s.nz2.data(); s.p3 = s.nz3.data(); }
            };
            std::thread prep_thread;
            Joiner prep_join(prep_thread);
            if (!infer_list.empty()) prep(infer_list[0], slot[0]);
            std::vector<float> depth(PN), c0(PN), c1(PN), c2(PN), ms;
                                                                                          
            std::vector<float> lo[4], mg[4];
            if (hi_pass) for (int k = 0; k < 4; ++k) { lo[k].resize(N); mg[k].resize(NG); }
            int done = 0;
            for (size_t idx = 0; idx < infer_list.size(); ++idx) {
                const int v = infer_list[idx];
                if (progress && progress("infer", done_all, total_all, user)) return 1;
                if (prep_thread.joinable()) prep_thread.join();                              
                if (!prep_err.empty()) { S.error = prep_err; return 2; }
                Slot& cur = slot[idx % slot_count];
                if (no_prefetch && idx > 0) { prep(v, cur); if (!prep_err.empty()) { S.error = prep_err; return 2; } }                                   
                if (!no_prefetch && idx + 1 < infer_list.size()) {                                             
                    const int nv2 = infer_list[idx + 1]; Slot* nxt = &slot[(idx + 1) % slot_count];
                    prep_thread = std::thread([&, nv2, nxt] { prep(nv2, *nxt); });
                }
                double dt = 0;
                if (split_mode) {                                                                             
                    std::vector<const FeatureSet*> fv((size_t)NV, nullptr);
                    for (int k = 0; k < NV; ++k) fv[k] = &feat[cur.views[k]];
                    if (!run->run_rest(fv.data(), cur.imgbuf.data(), cur.in, cur.p2, cur.p3, depth.data(), c0.data(), c1.data(), c2.data(), &dt, &err)) { S.error = "ort run: " + err; return 3; }
                } else if (!run->run(cur.imgbuf.data(), cur.in, cur.p2, cur.p3, depth.data(), c0.data(), c1.data(), c2.data(), &dt, &err)) { S.error = "ort run: " + err; return 3; }
                if (done > 0) ms.push_back(dt);                                                 
                if (hi_pass) S.infer_hi_ms_total += dt; else S.infer_ms_total += dt;
                if (lo_pass) {                                                                     
                    const float* rows[4] = {depth.data(), c0.data(), c1.data(), c2.data()};
                    for (int k = 0; k < 4; ++k) if (!pwrite_all(lo_fd[k].fd, rows[k], N * 4, (off_t)((size_t)lidx[v] * N * 4))) { S.error = "lo pack write"; return 2; }
                    ++done; ++done_all;
                    continue;
                }
                const float *pd = depth.data(), *pc0 = c0.data(), *pc1 = c1.data(), *pc2 = c2.data();
                if (hi_pass && dump_side) {                                                                       
                    const float* rows[4] = {depth.data(), c0.data(), c1.data(), c2.data()};
                    for (int k = 0; k < 4; ++k) if (!write_rows(pack + "/hi_" + std::string(lo_names[k] + 3), rows[k], NG * 4, (long)((size_t)lidx[v] * NG * 4))) { S.error = "hi dump | " + g_write_rows_diag; return 2; }
                }
                if (hi_pass) {                                    
                    const auto tm = clk::now();
                    for (int k = 0; k < 4; ++k) if (!pread_all(lo_fd[k].fd, lo[k].data(), N * 4, (off_t)((size_t)lidx[v] * N * 4))) { S.error = "lo pack read"; return 2; }
                    const RECONSTRUCTIONOriginalPlane low[4] = {
                        {lo[0].data(), uint32_t(W), uint32_t(H)}, {lo[1].data(), uint32_t(W), uint32_t(H)},
                        {lo[2].data(), uint32_t(W), uint32_t(H)}, {lo[3].data(), uint32_t(W), uint32_t(H)}};
                    const RECONSTRUCTIONOriginalPlane high[4] = {
                        {depth.data(), uint32_t(WF), uint32_t(HF)}, {c0.data(), uint32_t(WF), uint32_t(HF)},
                        {c1.data(), uint32_t(WF), uint32_t(HF)}, {c2.data(), uint32_t(WF), uint32_t(HF)}};
                    const RECONSTRUCTIONOriginalMutablePlane merged[4] = {
                        {mg[0].data(), uint32_t(WF), uint32_t(HF)}, {mg[1].data(), uint32_t(WF), uint32_t(HF)},
                        {mg[2].data(), uint32_t(WF), uint32_t(HF)}, {mg[3].data(), uint32_t(WF), uint32_t(HF)}};
                    uint64_t kept = 0;
                    if (reconstruction_original_merge_frame(low, high, merged, uint32_t(orig[v]), &kept) != 0) {
                        S.error = "original CER merge: " + std::string(reconstruction_original_dense_raw_last_error()); return 4;
                    }
                    S.mr_hi_kept += kept;
                    S.mr_pixels += NG;
                    pd = mg[0].data(); pc0 = mg[1].data(); pc1 = mg[2].data(); pc2 = mg[3].data();
                    S.merge_ms += ms_since(tm);
                }
                const long off = (long)((size_t)lidx[v] * NG * 4);
                if (!write_rows(pack + "/depth.f32", pd, NG * 4, off) || !write_rows(pack + "/conf0.f32", pc0, NG * 4, off) ||
                    !write_rows(pack + "/conf1.f32", pc1, NG * 4, off) || !write_rows(pack + "/conf2.f32", pc2, NG * 4, off)) { S.error = "pack write | " + g_write_rows_diag; return 2; }
                if (job.ref_depth) {
                    const float* rd = job.ref_depth + (size_t)orig[v] * NG;
                    for (size_t i = 0; i < NG; ++i) {
                        if (!std::isfinite(pd[i])) { ++S.parity_nonfinite; continue; }
                        const double r = std::fabs(pd[i] - rd[i]) / std::max(rd[i], 1e-6f);
                        if (r > 0.01) ++S.parity_bad1;
                        S.parity_worst_rel = std::max(S.parity_worst_rel, r); ++S.parity_pixels;
                    }
                }
                ++done; ++done_all;

            }
            if (prep_thread.joinable()) prep_thread.join();
            S.inferred = done;
            std::sort(ms.begin(), ms.end());
            (hi_pass ? S.infer_hi_ms_median : S.infer_ms_median) = ms.empty() ? 0 : ms[ms.size() / 2];
                                                                                      
            if (lo_pass) std::vector<std::vector<uint16_t>>{}.swap(bank);
        }

        run.reset();                                                                    
        feat.clear(); feat.shrink_to_fit();                                                                         

        if (mr) {                                                
            for (int k = 0; k < 4; ++k) { ::close(lo_fd[k].fd); lo_fd[k].fd = -1; if (!dump_side) std::remove((pack + "/" + lo_names[k]).c_str()); }
            ::close(bank_hi.fd); bank_hi.fd = -1; std::remove((pack + "/bank_hi.f16").c_str());
            ::close(rgb_fd.fd); rgb_fd.fd = -1;
        }
    }
    if (progress && progress("infer", total_infer_views, total_infer_views, user)) return 1;

                                                                                  
                                                                                
                                                                            
    t = clk::now();
    {
        OriginalReadMap maps[5];
        const char* names[4] = {"depth.f32", "conf0.f32", "conf1.f32", "conf2.f32"};
        RECONSTRUCTIONOriginalFusionPack input{};
        for (int k = 0; k < 4; ++k) {
            if (!maps[k].open(pack + "/" + names[k], size_t(NL) * NG * sizeof(float))) {
                S.error = "cannot map complete original fusion arrays"; return 2;
            }
            input.banks[k] = static_cast<const float*>(maps[k].data);
        }
        if (!maps[4].open(pack + "/cams.f32", size_t(NL) * 36 * sizeof(float))) {
            S.error = "cannot map original fusion cameras"; return 2;
        }
        std::vector<double> ids; std::vector<const char*> photos;
        ids.reserve(local.size()); photos.reserve(local.size());
        for (int frame : local) {
            ids.push_back(frames[frame].frame_id);
            photos.push_back(job.original_fusion_jpeg_paths[orig[frame]].c_str());
        }
        input.cams_f32 = static_cast<const float*>(maps[4].data);
        input.frame_ids_f64 = ids.data(); input.original_jpeg_paths = photos.data();
        input.frame_count = uint32_t(NL); input.width = uint32_t(WF); input.height = uint32_t(HF);
        uint64_t kept = 0;
        if (progress && progress("fusion", 0, NL, user)) return 1;
        if (reconstruction_original_fuse_pack(&input, job.full_pipeline_fused_depth_dir.c_str(), nullptr, &kept) != 0) {
            S.error = "original cache fusion: " + std::string(reconstruction_original_dense_raw_last_error()); return 4;
        }
        S.fuse.frames = NL; S.fuse.points = kept; S.fuse_ms = ms_since(t);
        if (progress && progress("fusion", NL, NL, user)) return 1;
        if (progress) progress("done", 1, 1, user);
        return 0;
    }

}

}                            
