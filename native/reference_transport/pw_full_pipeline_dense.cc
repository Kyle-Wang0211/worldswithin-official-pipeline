#include "pw_full_pipeline_dense.h"
#include "dense_pipeline.h"
#include "bench_noise_input.h"
#include "reconstruction_original_dense_raw.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <sys/stat.h>

#if !defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION) || PWDENSE_ENABLE_FUSED_DEPTH_RETENTION != 1
#error All mainline dense translation units must enable the approved retention layout
#endif
#if defined(PWDENSE_BENCH_PAIRED_DIAGNOSTIC) || defined(PWDENSE_ENABLE_VIEW_SEGMENTS)
#error Mainline uses the selected complete models, merge and fusion
#endif

namespace {
using namespace aether::dense;
bool nonempty(const char* p) { return p && *p; }
bool fresh_path(const char* p) {
    struct stat info;
    return nonempty(p) && ::lstat(p, &info) != 0 && errno == ENOENT;
}
int32_t failure(pw_full_pipeline_dense_result* r, const char* message) {
    if (r) { r->status = 2; std::snprintf(r->error, sizeof r->error, "%s", message); }
    return 2;
}
bool path_copy(char* out, size_t capacity, const char* path) {
    if (!nonempty(path) || std::strlen(path) >= capacity) return false;
    std::memcpy(out, path, std::strlen(path) + 1); return true;
}
struct Callbacks {
    pw_full_pipeline_dense_progress progress;
    pw_full_pipeline_dense_chunk chunk;
    void* user;
};
int progress_bridge(const char* phase, int done, int total, void* context) {
    auto* c = static_cast<Callbacks*>(context);
    return c->progress ? c->progress(phase, done, total, c->user) : 0;
}
int chunk_bridge(int index, const float* xyz, const uint8_t* rgb, int count, void* context) {
    auto* c = static_cast<Callbacks*>(context);
    return c->chunk ? c->chunk(index, xyz, rgb, count, c->user) : 0;
}
bool selected(const pw_full_pipeline_dense_options& o) {
    return o.abi_version == PW_FULL_PIPELINE_DENSE_ABI && o.struct_size == sizeof o &&
        o.width == 768 && o.height == 576 && o.nsrc == 9 && o.webgpu == 1 &&
        o.geo_mask_thres == 2 && o.check_all_views == 1 && o.photo_first == 0 &&
        o.geo_pixel_thres == 0.125f && o.geo_depth_thres == 0.01f &&
        o.photo_thres[0] == 0.3f && o.photo_thres[1] == 0.5f && o.photo_thres[2] == 0.5f &&
        o.sat_tol == 1e-5f && o.min_tri_angle_deg == 3.0f;
}
int32_t run(const pw_full_pipeline_dense_frame* frames, int32_t frame_count,
            const float* sparse_xyz, int32_t point_count, const pw_full_pipeline_dense_options* o,
            pw_full_pipeline_dense_progress progress, pw_full_pipeline_dense_chunk chunk, void* user,
            pw_full_pipeline_dense_result* result) {
    if (!result) return 2;
    std::memset(result, 0, sizeof *result);
    result->status = 2;
    if (!frames || (frame_count <= 0 || frame_count > 97) || !o || !selected(*o) || point_count < 0 ||
        (point_count && !sparse_xyz)) return failure(result, "requires complete selected actual-N recipe (capacity97)");
    if (!nonempty(o->low_model_path) || !nonempty(o->high_model_path) ||
        !nonempty(o->low_noise_f32_path) || !nonempty(o->high_noise_f32_path))
        return failure(result, "explicit model pair and exact sorted-frame noise paths required");
    if (chunk || o->out_ply || o->original_fusion_jpeg_count != frame_count ||
        !o->original_fusion_jpeg_paths)
        return failure(result, "original cache producer requires NULL PLY/chunk and actual-N original fusion JPEGs");
    if (!path_copy(result->work_dir, sizeof result->work_dir, o->work_dir) ||
        !path_copy(result->fused_depth_dir, sizeof result->fused_depth_dir, o->fused_depth_dir) ||
        std::strlen(o->fused_depth_dir) + 12 >= sizeof result->order_json)
        return failure(result, "empty or oversized output path");
    std::snprintf(result->order_json, sizeof result->order_json, "%s/order.json", o->fused_depth_dir);
    if (!fresh_path(o->work_dir) || !fresh_path(o->fused_depth_dir))
        return failure(result, "output paths must be fresh; partial runs are never overwritten");
    const bool supplied_table = o->cameras_f32 || o->neighbors_i32;
    if (!supplied_table) return failure(result, "original cache profile requires frozen original camera/neighbor tables");
    if ((supplied_table && (!o->cameras_f32 || !o->neighbors_i32 ||
        o->cameras_count != static_cast<uint64_t>(frame_count) * 36 || o->neighbors_count != static_cast<uint64_t>(frame_count) * 9)) ||
        (!supplied_table && (o->cameras_count || o->neighbors_count || !point_count)))
        return failure(result, "camera/neighbor table counts or real sparse input invalid");
                                                                                
                                                                                  
    if (reconstruction_original_dense_dependencies_validate() != 0)
        return failure(result, reconstruction_original_dense_raw_last_error());
    DenseJob job;
    for (int32_t i = 0; i < frame_count; ++i) {
        const auto& f = frames[i];
        if (!std::isfinite(f.frame_id) || !nonempty(f.jpeg_path) || (i && f.frame_id <= frames[i-1].frame_id))
            return failure(result, "real JPEG paths and strictly increasing unique frame IDs required");
        SessionFrame s{};
        s.frame_id = f.frame_id; s.fx = f.fx; s.fy = f.fy; s.cx = f.cx; s.cy = f.cy;
        s.image_w = f.image_w; s.image_h = f.image_h;
        for (int k = 0; k < 4; ++k) s.q[k] = f.q_wxyz[k];
        for (int k = 0; k < 3; ++k) s.t[k] = f.t[k];
        if (!supplied_table) {
            if (!std::isfinite(s.fx) || !std::isfinite(s.fy) || s.fx <= 0 || s.fy <= 0 ||
                !std::isfinite(s.cx) || !std::isfinite(s.cy) || !std::isfinite(s.image_w) ||
                !std::isfinite(s.image_h) || s.image_w <= 0 || s.image_h <= 0)
                return failure(result, "invalid real camera intrinsics");
            for (double x : s.q) if (!std::isfinite(x)) return failure(result, "nonfinite camera quaternion");
            for (double x : s.t) if (!std::isfinite(x)) return failure(result, "nonfinite camera translation");
        }
        const char* original_rgb = o->original_fusion_jpeg_paths[i];
        if (!nonempty(original_rgb) || original_rgb[0] != '/')
            return failure(result, "absolute original fusion JPEG path required");
        job.frames.push_back(s); job.jpeg_paths.emplace_back(f.jpeg_path);
        job.original_fusion_jpeg_paths.emplace_back(original_rgb);
    }
    for (size_t i = 0; i < static_cast<size_t>(point_count) * 3; ++i)
        if (!std::isfinite(sparse_xyz[i])) return failure(result, "nonfinite sparse xyz");
    if (point_count) job.points.assign(sparse_xyz, sparse_xyz + static_cast<size_t>(point_count) * 3);
    SessionTable table;
    if (supplied_table) {
        for (uint64_t i = 0; i < o->cameras_count; ++i)
            if (!std::isfinite(o->cameras_f32[i])) return failure(result, "nonfinite supplied camera table");
        for (uint64_t i = 0; i < o->neighbors_count; ++i)
            if (o->neighbors_i32[i] < 0 || o->neighbors_i32[i] >= frame_count)
                return failure(result, "supplied neighbor outside complete frame table");
        table.NF = frame_count;
        table.cams.assign(o->cameras_f32, o->cameras_f32 + o->cameras_count);
        table.neighbors.assign(o->neighbors_i32, o->neighbors_i32 + o->neighbors_count);
        job.table_hook = &table;
    }
    BenchNoiseFileInput noise_lo, noise_hi;
    size_t lo_bytes = 0, hi_bytes = 0; std::string error;
    if (!BenchNoiseInput::expected_bytes(frame_count, 768, 576, &lo_bytes, &error) ||
        !BenchNoiseInput::expected_bytes(frame_count, 1536, 1152, &hi_bytes, &error) ||
        !noise_lo.open(o->low_noise_f32_path, lo_bytes, &error) ||
        !noise_hi.open(o->high_noise_f32_path, hi_bytes, &error)) return failure(result, error.c_str());
    job.session.W = o->width; job.session.H = o->height; job.session.nsrc = o->nsrc;
    job.model_path = o->low_model_path; job.hi_model_path = o->high_model_path; job.webgpu = true;
    job.ext_noise_fd = noise_lo.fd(); job.ext_noise_hi_fd = noise_hi.fd();
    job.work_dir = o->work_dir; job.out_ply.clear();
    job.full_pipeline_fused_depth_dir = o->fused_depth_dir;
    job.ref_begin = 0; job.ref_end = -1;
    job.fuse.geo_mask_thres = o->geo_mask_thres;
    job.fuse.geo_pixel_thres = o->geo_pixel_thres; job.fuse.geo_depth_thres = o->geo_depth_thres;
    for (int k = 0; k < 3; ++k) job.fuse.photo_thres[k] = o->photo_thres[k];
    job.fuse.check_all_views = o->check_all_views; job.fuse.photo_first = o->photo_first;
    job.fuse.sat_tol = o->sat_tol; job.fuse.min_tri_angle_deg = o->min_tri_angle_deg;
    Callbacks callbacks{progress, chunk, user}; DenseStats stats;
    const int rc = dense_run(job, progress_bridge, chunk ? chunk_bridge : nullptr, &callbacks, &stats);
    result->status = rc; result->frames = stats.NF; result->inferred = stats.inferred;
    result->fused_frames = stats.fuse.frames; result->points = stats.fuse.points;
    result->low_infer_ms = stats.infer_ms_total; result->high_infer_ms = stats.infer_hi_ms_total;
    result->merge_ms = stats.merge_ms; result->fuse_ms = stats.fuse_ms;
    std::snprintf(result->error, sizeof result->error, "%s", stats.error.c_str());
    return rc;
}
}             

extern "C" const char* pw_full_pipeline_dense_producer_profile(void) { return "original_numpy_opencv_retained_v2"; }
extern "C" int32_t pw_full_pipeline_dense_abi_version(void) { return PW_FULL_PIPELINE_DENSE_ABI; }
extern "C" int32_t pw_full_pipeline_dense_options_init(pw_full_pipeline_dense_options* o) {
    if (!o) return 2;
    std::memset(o, 0, sizeof *o); o->abi_version = PW_FULL_PIPELINE_DENSE_ABI; o->struct_size = sizeof *o;
    o->width = 768; o->height = 576; o->nsrc = 9; o->webgpu = 1;
    o->geo_mask_thres = 2; o->check_all_views = 1; o->photo_first = 0;
    o->geo_pixel_thres = 0.125f; o->geo_depth_thres = 0.01f;
    o->photo_thres[0] = 0.3f; o->photo_thres[1] = 0.5f; o->photo_thres[2] = 0.5f;
    o->sat_tol = 1e-5f; o->min_tri_angle_deg = 3.0f;
    return 0;
}
extern "C" int32_t pw_full_pipeline_dense_run(const pw_full_pipeline_dense_frame* frames, int32_t count,
    const float* xyz, int32_t points, const pw_full_pipeline_dense_options* o,
    pw_full_pipeline_dense_progress progress, pw_full_pipeline_dense_chunk chunk, void* user,
    pw_full_pipeline_dense_result* result) {
    try { return run(frames, count, xyz, points, o, progress, chunk, user, result); }
    catch (const std::exception& e) { return failure(result, e.what()); }
    catch (...) { return failure(result, "native dense exception"); }
}

namespace {
int32_t session_failure(char* error, uint64_t capacity, const char* message) {
    if (error && capacity) std::snprintf(error, static_cast<size_t>(capacity), "%s", message);
    return 2;
}
int32_t session_build(const pw_full_pipeline_dense_frame* frames, int32_t frame_count,
    const float* sparse_xyz, int32_t point_count, uint64_t sparse_count,
    const pw_full_pipeline_dense_options* options, float* cameras, uint64_t cameras_count,
    int32_t* neighbors, uint64_t neighbors_count, char* error, uint64_t error_capacity) {
    if (!error || error_capacity < 512 || error_capacity > SIZE_MAX) return 2;
    error[0] = '\0';
    if (!frames || (frame_count <= 0 || frame_count > 97) || !options || !selected(*options) ||
        !sparse_xyz || point_count <= 0 || sparse_count != static_cast<uint64_t>(point_count) * 3 ||
        !cameras || cameras_count != static_cast<uint64_t>(frame_count) * 36 || !neighbors || neighbors_count != static_cast<uint64_t>(frame_count) * 9)
        return session_failure(error, error_capacity, "requires selected actual-N recipe (capacity97) and exact session buffer counts");
    std::vector<SessionFrame> real_frames;
    real_frames.reserve(frame_count);
    for (int32_t i = 0; i < frame_count; ++i) {
        const auto& f = frames[i];
        if (!std::isfinite(f.frame_id) || !nonempty(f.jpeg_path) ||
            (i && f.frame_id <= frames[i - 1].frame_id))
            return session_failure(error, error_capacity, "real JPEG identity and increasing unique frame IDs required");
        if (!std::isfinite(f.fx) || !std::isfinite(f.fy) || f.fx <= 0 || f.fy <= 0 ||
            !std::isfinite(f.cx) || !std::isfinite(f.cy) || !std::isfinite(f.image_w) ||
            !std::isfinite(f.image_h) || f.image_w <= 0 || f.image_h <= 0)
            return session_failure(error, error_capacity, "invalid real camera intrinsics");
        SessionFrame s{};
        s.frame_id = f.frame_id; s.fx = f.fx; s.fy = f.fy; s.cx = f.cx; s.cy = f.cy;
        s.image_w = f.image_w; s.image_h = f.image_h;
        bool nonzero_q = false;
        for (int k = 0; k < 4; ++k) {
            if (!std::isfinite(f.q_wxyz[k]))
                return session_failure(error, error_capacity, "nonfinite camera quaternion");
            nonzero_q = nonzero_q || f.q_wxyz[k] != 0; s.q[k] = f.q_wxyz[k];
        }
        if (!nonzero_q) return session_failure(error, error_capacity, "zero camera quaternion");
        for (int k = 0; k < 3; ++k) {
            if (!std::isfinite(f.t[k])) return session_failure(error, error_capacity, "nonfinite camera translation");
            s.t[k] = f.t[k];
        }
        real_frames.push_back(s);
    }
    for (uint64_t i = 0; i < sparse_count; ++i)
        if (!std::isfinite(sparse_xyz[i])) return session_failure(error, error_capacity, "nonfinite sparse xyz");
    const std::vector<float> points(sparse_xyz, sparse_xyz + sparse_count);
    SessionParams params;                                                              
    SessionTable table;
    build_session_table(real_frames, points, params, table);
    if (table.NF != frame_count || table.cams.size() != cameras_count || table.neighbors.size() != neighbors_count)
        return session_failure(error, error_capacity, "unchanged session builder returned unexpected dimensions");
    for (float value : table.cams)
        if (!std::isfinite(value)) return session_failure(error, error_capacity, "session camera output nonfinite");
    for (int32_t value : table.neighbors)
        if (value < 0 || value >= frame_count) return session_failure(error, error_capacity, "session neighbor output out of range");
    std::memcpy(cameras, table.cams.data(), table.cams.size() * sizeof(float));
    std::memcpy(neighbors, table.neighbors.data(), table.neighbors.size() * sizeof(int32_t));
    return 0;
}
}             
extern "C" int32_t pw_full_pipeline_session_abi_version(void) { return PW_FULL_PIPELINE_SESSION_ABI; }
extern "C" int32_t pw_full_pipeline_session_build(const pw_full_pipeline_dense_frame* frames, int32_t frame_count,
    const float* sparse_xyz, int32_t point_count, uint64_t sparse_count,
    const pw_full_pipeline_dense_options* options, float* cameras, uint64_t cameras_count,
    int32_t* neighbors, uint64_t neighbors_count, char* error, uint64_t error_capacity) {
    try { return session_build(frames, frame_count, sparse_xyz, point_count, sparse_count,
        options, cameras, cameras_count, neighbors, neighbors_count, error, error_capacity); }
    catch (const std::exception& e) { return session_failure(error, error_capacity, e.what()); }
    catch (...) { return session_failure(error, error_capacity, "native session exception"); }
}
