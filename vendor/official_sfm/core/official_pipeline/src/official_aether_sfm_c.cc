                                                                
                                       
  
                                                                            
                                                                       
                                                                           
                                                                           
  
                               
                                                                           
                                                                            
                                                                          
                                                                   
                                                                            
                            
  
                                                                                
                                                                              
                                                                                
                                                                          
                                                                              
                                                               

#include "aether_sfm_c.h"
#include "aether_ba_solve_policy_v1.h"
#include "arkit_pose_store_v1.h"
#include "mandatory_arkit_gravity_v1.h"
#include "mandatory_gravity_tvg_v1.h"
#include "live_cloud_diagnostics_v1.h"
#include "pair_policy_v2_c.h"
#include "pair_selection_v2.h"
#include "visual_loop_index_v1.h"
#define AETHER_PRECLAMP_INSTR_ENV_OFFICIAL 1
#include "official_isolated_floater.h"
#include "official_mirror_ghost.h"
#include "official_preclamp_instr_v1.h"

#ifndef AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
#define AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1 1
#endif

#define AETHER_GPU_TIMESTAMP_INTERNAL
#define AETHER_GPU_TIMESTAMP_WEAK_IMPORT __attribute__((weak_import))
#include "official_gpu_timestamp_writer_v1.h"
#undef AETHER_GPU_TIMESTAMP_WEAK_IMPORT
#undef AETHER_GPU_TIMESTAMP_INTERNAL

#include "colmap/controllers/incremental_pipeline.h"
#include "colmap/estimators/rotation_averaging.h"
#include "colmap/estimators/two_view_geometry.h"
#include "colmap/geometry/pose_prior.h"
#include "colmap/scene/pose_graph.h"
#include "colmap/feature/types.h"
#include "colmap/feature/utils.h"
#include "colmap/geometry/essential_matrix.h"
#include "colmap/geometry/rigid3.h"
#include "colmap/geometry/triangulation.h"
#include "colmap/scene/camera.h"
#include "colmap/scene/database.h"
#include "colmap/scene/database_cache.h"
#include "colmap/scene/image.h"
#include "colmap/scene/point3d.h"
#include "colmap/scene/reconstruction.h"
#include "colmap/scene/reconstruction_manager.h"
#include "colmap/scene/two_view_geometry.h"
#include "colmap/scene/track.h"
#include "colmap/estimators/bundle_adjustment.h"
#include "colmap/estimators/bundle_adjustment_ceres.h"
#include "colmap/sfm/incremental_mapper.h"
#include "colmap/sfm/observation_manager.h"

#include <glog/logging.h>

#if defined(__APPLE__)
#include <pthread.h>                                                        
#endif

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <utility>
#include <vector>

                                                                                
                                                                     
#include "aether_ghost_mask.h"
#if __has_include("aether/sfm/tail_cache_epoch_v1.h")
#include "aether/sfm/tail_cache_epoch_v1.h"
#else
#include "../../include/aether/sfm/tail_cache_epoch_v1.h"
#endif
                                                                        
                                                                                
                                                                                  
                                                                       
                                                                        
#include "aether_l1_arbitrate.h"
#include "aether_l1_plan.h"

                                                                                    
                                                                                           
                                                                                    
                                                                                       
                                                          
#include "official_gpu_watchdog_v1.h"

                                                                                
extern "C" int aether_dsp_sift_extract(const uint8_t* gray, int width,
                                       int height, int max_features,
                                       float* out_xy, uint8_t* out_desc,
                                       int out_cap, int* out_count);
                                                                         
                                                                           
                                                                             
  
                                                                               
                                                                                 
                                                                                
                                                                        
extern "C" __attribute__((weak)) int aether_dsp_sift_extract_gpu(
    const uint8_t* gray, int width, int height, int max_features,
    int num_threads, float* out_xy, uint8_t* out_desc, int out_cap,
    int* out_count);
                                                                            
                                                                             
                                                                             
                                                                             
                                                                              
                                                                             
                                                                           
extern "C" int aether_dsp_sift_extract_v2(const uint8_t* gray, int width,
                                          int height, int max_features,
                                          float* out_xy, uint8_t* out_desc,
                                          float* out_scales,
                                          float* out_orientations, int out_cap,
                                          int* out_count);
extern "C" __attribute__((weak)) int aether_dsp_sift_extract_gpu_v2(
    const uint8_t* gray, int width, int height, int max_features,
    int num_threads, float* out_xy, uint8_t* out_desc, float* out_scales,
    float* out_orientations, int out_cap, int* out_count);
                                                                           
                                                                   
                                                      
extern "C" __attribute__((weak)) void aether_sed_last_stages(double* out,
                                                             int cap);
                                                                                     
                                                                                                   
extern "C" __attribute__((weak)) const char* aether_sed_last_fail_reason(void);

                                                                                                
                                                                                             
                                                                        
                                                                                       
                                                                                               
                                                                                
#if defined(__APPLE__)
extern "C" __attribute__((weak_import)) int aether_dsp_sift_extract_gpu_warmup(
    double* out_init_ms, double* out_compile_ms,
    double* out_msl_ms, unsigned* out_msl_n,
    double* out_pso_ms, unsigned* out_pso_n);
#else
extern "C" __attribute__((weak)) int aether_dsp_sift_extract_gpu_warmup(
    double* out_init_ms, double* out_compile_ms,
    double* out_msl_ms, unsigned* out_msl_n,
    double* out_pso_ms, unsigned* out_pso_n);
#endif
extern "C" int aether_sift_match(const uint8_t* desc1, int n1,
                                 const uint8_t* desc2, int n2, double max_ratio,
                                 int* out_num_matches);
                                                                            
                                                                        
                                                                            
extern "C" int aether_sift_match_pairs(const uint8_t* desc1, int n1,
                                       const uint8_t* desc2, int n2,
                                       double max_ratio, uint32_t* out_pairs,
                                       int max_pairs, int* out_num_matches);
                                                                              
                                                                                 
                                                                         
                                                                              
                                                               
                                                                             
                                                             
extern "C" __attribute__((weak)) void aether_gpu_match_set_capture_active(int);
extern "C" __attribute__((weak)) void aether_gpu_match_set_preview_fps30(int);
                                                                                    
namespace { void KickExtractWarmupOnce(const char* when); int ExtractWarmupMode(); const char* ExtractWarmupQosName(); }
extern "C" void aether_sfm_match_set_capture_active(int active) {
  if (aether_gpu_match_set_capture_active != nullptr) {
    aether_gpu_match_set_capture_active(active);
  }
                                                                                    
                                                                                         
  if (active != 0 && ExtractWarmupMode() >= 2) KickExtractWarmupOnce("capture_active");
}
extern "C" void aether_sfm_match_set_preview_fps30(int on) {
  if (aether_gpu_match_set_preview_fps30 != nullptr) {
    aether_gpu_match_set_preview_fps30(on);
  }
}
extern "C" __attribute__((weak)) int aether_gpu_match_gemm_pairs(
    const uint8_t* desc1, int n1, const uint8_t* desc2, int n2,
    double max_ratio, uint32_t* out_pairs, int max_pairs,
    int* out_num_matches);
                                                                             
                                                                           
                                                                  
                                                                              
                                                                            
                                                    
extern "C" __attribute__((weak)) int aether_gpu_match_probe_batch(
    const uint8_t* dA, int nA, const uint8_t* const* dBs, const int* nBs,
    int n_cands, double max_ratio, int* out_counts);
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
                                                                             
// never derived from host pointers. Older platform TUs omit this weak symbol
                                                 
extern "C" __attribute__((weak)) int aether_gpu_match_gemm_pairs_resident(
    uint64_t session_nonce, uint32_t frame1, uint32_t generation1,
    const uint8_t* desc1, int n1, uint32_t frame2, uint32_t generation2,
    const uint8_t* desc2, int n2, double max_ratio, uint32_t* out_pairs,
    int max_pairs, int* out_num_matches);
extern "C" __attribute__((weak)) void
aether_gpu_match_descriptor_residency_invalidate(uint64_t session_nonce,
                                                  uint32_t frame_ordinal);
extern "C" __attribute__((weak)) void
aether_gpu_match_descriptor_residency_clear_session(uint64_t session_nonce);
extern "C" __attribute__((weak)) int
aether_gpu_match_descriptor_residency_stats(
    uint64_t session_nonce, uint64_t* hits, uint64_t* misses,
    uint64_t* evictions, uint64_t* stale_replacements,
    uint64_t* upload_bytes, uint64_t* resident_bytes,
    uint64_t* resident_entries, uint64_t* allocation_failures,
    uint64_t* device_resets);
#endif
                                                                            
                                                                           
                                                                            
                                                                              
                                                                            
                                                      
extern "C" __attribute__((weak)) int aether_gpu_match_gemm_pairs_guided(
    const uint8_t* desc1, int n1, const float* xy1,
    const uint8_t* desc2, int n2, const float* xy2, double max_ratio,
    const float* matrix12, const float* matrix21, int guide_mode,
    float max_residual, uint32_t* out_pairs, int max_pairs,
    int* out_num_matches);
                                                                       
                                                                               
                                                                            
                                                                            
                                                
                                                                            
                                                                 
                                                                            
                                                                       
extern "C" double aether_igr_pre_ms;
extern "C" double aether_igr_ba_ms;
extern "C" double aether_igr_merge_ms;
extern "C" int aether_igr_rounds;
                                                                            
                                                                               
                                                                          
                                     
extern "C" double aether_ilr_find_ms;
extern "C" double aether_ilr_setup_ms;
extern "C" double aether_ilr_solve_ms;
extern "C" double aether_ilr_merge_ms;
extern "C" double aether_ilr_filter_ms;
extern "C" double aether_ilr_preproc_ms;
extern "C" double aether_ilr_minim_ms;
extern "C" double aether_ilr_postproc_ms;
                                                                   
                                                                              
                                                                         
                                                                                              
                                                                                                            
                                                                                            
                                                                                
                                                                                                               
extern "C" void aether_match_set_ab_phase(int phase);
int AbPeriod() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_AB_PERIOD")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;
  }();
  return cached;
}

                                                                                 
                                                                                                   
                                                                       
extern "C" double aether_match_gpu_ms;
extern "C" double aether_match_sleep_ms;
extern "C" int aether_match_chunks;
extern "C" double aether_tvg_upright_ms;
extern "C" double aether_tvg_homography_ms;
extern "C" int aether_tvg_upright_calls;
extern "C" int aether_tvg_homography_calls;
extern "C" double aether_ilr_jac_ms;
extern "C" double aether_ilr_lin_ms;
extern "C" double aether_ilr_resid_ms;
extern "C" int64_t aether_ilr_num_resid;
extern "C" int64_t aether_ilr_num_param;
extern "C" int aether_ilr_rounds;
extern "C" int aether_ilr_solves;
extern "C" int aether_ilr_iters;
extern "C" int aether_ilr_term_conv;
extern "C" int aether_ilr_term_nocnv;
extern "C" int aether_ilr_term_other;
extern "C" int aether_ilr_ord_applied;
extern "C" int aether_ilr_ord_skipped;
extern "C" int aether_ilr_bundle_trace_calls;
extern "C" uint32_t aether_ilr_bundle_trace_query[16];
extern "C" int aether_ilr_bundle_trace_neighbor_count[16];
extern "C" int aether_ilr_bundle_trace_neighbor_total[16];
extern "C" uint32_t aether_ilr_bundle_trace_neighbors[16][15];
extern "C" int aether_ba_get_gravity_prior(const char* name,
                                           double* out_gravity_cam_xyz);
extern "C" int aether_ba_set_gravity_prior(const char* name,
                                            const double* gravity_cam_xyz,
                                            double sigma_rad);
extern "C" void aether_ba_clear_gravity_priors();

extern "C" __attribute__((weak)) int aether_gpu_match_last_error(char* buf,
                                                                 int cap);

                                                                             
                                                                          
                                                                              
                                                                              
                                                                           

                                                                           
                                                                            
                                                                           
                                                                             
                                                                      
namespace colmap {
void AetherLastBaSolveInfo(std::string* solver_used,
                           std::string* sparse_backend,
                           int* mixed,
                           int* threads,
                           std::string* dense_backend);
}                     

namespace {

                                                                               
                                                                            
                                                                         
                                                                             
                                      
constexpr bool kProductionOfficialEndpointOnly = true;

double NowMs() {
  using namespace std::chrono;
  return duration<double, std::milli>(steady_clock::now().time_since_epoch())
      .count();
}

#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
uint64_t NewDescriptorResidencySessionNonce() {
  static std::atomic<uint64_t> next{1};
  uint64_t nonce = next.fetch_add(1, std::memory_order_relaxed);
  if (nonce == 0) nonce = next.fetch_add(1, std::memory_order_relaxed);
  return nonce;
}
#endif

                                                                           
                                                                              
                                                                    
struct FrameRecord {
  int frame_id = -1;
  colmap::image_t image_id = 0;
                                                                              
                                                                     
  colmap::image_t persisted_image_id = 0;
  uint64_t frame_identity_digest = 0;
                                                                         
                                                                            
                                                                           
  colmap::camera_t camera_id = colmap::kInvalidCameraId;
  colmap::Camera camera;
  int n_keypoints = 0;
  std::vector<uint8_t> descriptors;                                
                                                                         
                                                                          
                                                                 
  std::vector<Eigen::Vector2d> points;
                                                                               
                                                                                
  colmap::Rigid3d cam_from_world;
                                                                        
  Eigen::Vector3d gravity_cam = Eigen::Vector3d::Zero();
  bool has_pose = false;
                                                                             
                                                                                
                                                                             
                                                                            
                                                                               
                                                    
  bool fed_throttled = false;
};

std::array<double, 3> FrameGravityArray(const FrameRecord& frame) {
  return {frame.gravity_cam.x(), frame.gravity_cam.y(),
          frame.gravity_cam.z()};
}

uint64_t FrameIdentityDigestV1(
    const std::string& name,
    const colmap::Camera& camera,
    const std::vector<Eigen::Vector2d>& points,
    const std::vector<uint8_t>& descriptors) {
  uint64_t hash = UINT64_C(1469598103934665603);
  const auto mix_byte = [&hash](const uint8_t byte) {
    hash ^= byte;
    hash *= UINT64_C(1099511628211);
  };
  const auto mix_u64 = [&mix_byte](const uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
      mix_byte(static_cast<uint8_t>((value >> shift) & 0xff));
    }
  };
  const auto mix_double = [&mix_u64](const double value) {
    uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    mix_u64(bits);
  };
  for (const char ch : name) mix_byte(static_cast<uint8_t>(ch));
  mix_byte(0);
  mix_u64(static_cast<uint64_t>(camera.model_id));
  mix_u64(camera.width);
  mix_u64(camera.height);
  mix_u64(camera.params.size());
  for (const double value : camera.params) mix_double(value);
  mix_u64(points.size());
  for (const Eigen::Vector2d& point : points) {
    mix_double(point.x());
    mix_double(point.y());
  }
  mix_u64(descriptors.size());
  for (const uint8_t value : descriptors) mix_byte(value);
  return hash == 0 ? 1 : hash;
}

aether::sfm::MandatoryGravityTwoViewResultV1
EstimateMandatoryFrameTwoViewGeometry(
    const FrameRecord& frame1,
    const std::vector<Eigen::Vector2d>& points1,
    const FrameRecord& frame2,
    const std::vector<Eigen::Vector2d>& points2,
    const colmap::FeatureMatches& matches,
    const colmap::TwoViewGeometryOptions& options) {
  if (!frame1.has_pose || !frame2.has_pose) {
    aether::sfm::MandatoryGravityTwoViewResultV1 result;
    result.status =
        aether::sfm::MandatoryGravityTwoViewStatusV1::kInvalidInput;
    result.geometry.config = colmap::TwoViewGeometry::DEGENERATE;
    return result;
  }
  return aether::sfm::EstimateMandatoryGravityTwoViewGeometryV1(
      frame1.camera, points1, FrameGravityArray(frame1), frame2.camera,
      points2, FrameGravityArray(frame2), matches, options);
}

bool MandatoryGravityTvgPersistable(
    const aether::sfm::MandatoryGravityTwoViewResultV1& result) {
  return result.status == aether::sfm::MandatoryGravityTwoViewStatusV1::kValid ||
         result.status == aether::sfm::MandatoryGravityTwoViewStatusV1::kPlanar;
}

struct PointIdPair {
  colmap::point3D_t a = 0;
  colmap::point3D_t b = 0;

  bool operator==(const PointIdPair& other) const {
    return a == other.a && b == other.b;
  }
};

struct PointIdPairHash {
  size_t operator()(const PointIdPair& p) const {
    const size_t h1 = std::hash<colmap::point3D_t>{}(p.a);
    const size_t h2 = std::hash<colmap::point3D_t>{}(p.b);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};

PointIdPair CanonicalPointPair(colmap::point3D_t p1, colmap::point3D_t p2) {
  return p1 < p2 ? PointIdPair{p1, p2} : PointIdPair{p2, p1};
}

bool TracksHaveDisjointImages(const colmap::Track& t1,
                              const colmap::Track& t2) {
  std::unordered_set<colmap::image_t> images;
  images.reserve(t1.Length() + t2.Length());
  for (const auto& el : t1.Elements()) {
    if (!images.insert(el.image_id).second) return false;
  }
  for (const auto& el : t2.Elements()) {
    if (!images.insert(el.image_id).second) return false;
  }
  return true;
}

bool ReprojectsCleanlyToTrack(const colmap::Reconstruction& recon,
                              const colmap::Track& track,
                              const Eigen::Vector3d& xyz,
                              double max_reproj_px) {
  for (const auto& el : track.Elements()) {
    if (!recon.ExistsImage(el.image_id)) return false;
    const colmap::Image& image = recon.Image(el.image_id);
    if (!image.HasPose() || el.point2D_idx >= image.NumPoints2D()) return false;
    const colmap::Camera* camera = image.CameraPtr();
    if (!camera) return false;
    const Eigen::Vector3d x_cam = image.CamFromWorld() * xyz;
    if (x_cam.z() <= 0.0) return false;
    const std::optional<Eigen::Vector2d> px = camera->ImgFromCam(x_cam);
    if (!px) return false;
    if ((*px - image.Point2D(el.point2D_idx).xy).norm() > max_reproj_px) {
      return false;
    }
  }
  return true;
}

                                                                                 
                                                                          
                                                              
enum class MergeReject {
  kNone = 0,                              
  kMissing,                                                                
  kSharedImage,                                                             
  kReproj,                                                                    
};

                                                                             
                                                                            
                                                                              
                                                                           
                                                                        
                                                                              
                                                                             
                                                                         
                                                                             
                                                                         
                                                                              
                                                                               
                                                                             
                                                                       
                                                                             
                                                                              
                                                          
  
                                                                               
                                                                            
                                                                              
                                                                    
                                                                           
                                                                            
                                                                               
                                                                              
                                                                             
                                                                               
                                                                                   
                                                                                   
                                                                           
                                                              
struct EnrichTargetHint {
  struct LowTrack {
    Eigen::Vector3d xyz;                                                       
    std::vector<int> frame_idxs;                                              
  };
  std::vector<LowTrack> tracks;
  std::vector<std::vector<int32_t>> by_frame;                           
};

MergeReject CanMergeLivePoints(const colmap::Reconstruction& recon,
                               colmap::point3D_t pid1,
                               colmap::point3D_t pid2,
                               double max_reproj_px,
                               bool require_disjoint_images,
                               Eigen::Vector3d* out_refit_xyz) {
  if (pid1 == pid2) return MergeReject::kMissing;
  if (!recon.ExistsPoint3D(pid1) || !recon.ExistsPoint3D(pid2)) {
    return MergeReject::kMissing;
  }
  const colmap::Point3D& p1 = recon.Point3D(pid1);
  const colmap::Point3D& p2 = recon.Point3D(pid2);
  if (p1.track.Length() == 0 || p2.track.Length() == 0) {
    return MergeReject::kMissing;
  }
  if (require_disjoint_images &&
      !TracksHaveDisjointImages(p1.track, p2.track)) {
    return MergeReject::kSharedImage;
  }
                                              
  std::vector<Eigen::Matrix3x4d> cams_from_world;
  std::vector<Eigen::Vector2d> cam_points;
  cams_from_world.reserve(p1.track.Length() + p2.track.Length());
  cam_points.reserve(p1.track.Length() + p2.track.Length());
  for (const colmap::Track* track : {&p1.track, &p2.track}) {
    for (const auto& el : track->Elements()) {
      if (!recon.ExistsImage(el.image_id)) return MergeReject::kMissing;
      const colmap::Image& image = recon.Image(el.image_id);
      if (!image.HasPose() || el.point2D_idx >= image.NumPoints2D()) {
        return MergeReject::kMissing;
      }
      const colmap::Camera* camera = image.CameraPtr();
      if (!camera) return MergeReject::kMissing;
      const std::optional<Eigen::Vector2d> np =
          camera->CamFromImg(image.Point2D(el.point2D_idx).xy);
      if (!np) return MergeReject::kReproj;
      cams_from_world.push_back(image.CamFromWorld().ToMatrix());
      cam_points.push_back(*np);
    }
  }
  Eigen::Vector3d refit_xyz;
  if (!colmap::TriangulateMultiViewPoint(
          colmap::span<const Eigen::Matrix3x4d>(cams_from_world.data(),
                                                cams_from_world.size()),
          colmap::span<const Eigen::Vector2d>(cam_points.data(),
                                              cam_points.size()),
          &refit_xyz)) {
    return MergeReject::kReproj;
  }
  const bool ok =
      ReprojectsCleanlyToTrack(recon, p1.track, refit_xyz, max_reproj_px) &&
      ReprojectsCleanlyToTrack(recon, p2.track, refit_xyz, max_reproj_px);
  if (!ok) return MergeReject::kReproj;
  if (out_refit_xyz) *out_refit_xyz = refit_xyz;
  return MergeReject::kNone;
}

                                                                            
                                                                            
                                                                          
                                                                            
                                                                            
                                                                           
                                                                           
                                                                  
struct ProbeDebtGrowPair {
  int j = 0;                          
  int f = 0;                        
  colmap::FeatureMatches inliers;                                       
};

}              

struct aether_sfm_session {
  aether_sfm_options_t options{};
  std::string db_path;
  std::string image_path;
  bool owns_db_file = false;                                                    
  uint64_t descriptor_residency_nonce = 0;
                                                                             
                                                                           
                                                       
  aether::official::ba::BaSessionAggregateAccumulatorV1 ba_ptol_aggregate;
  aether::official::ba::BaSessionReceiptRingV1 ba_ptol_receipts;

                                                                               
  std::shared_ptr<colmap::Database> db;
                                                                             
                                                                              
                                                                           
  std::shared_ptr<colmap::DatabaseCache> tail_cache_master;
  aether::sfm::TailCacheEpochV1 tail_cache_epoch;
  std::unordered_set<colmap::image_pair_t> tail_cache_seen_db_pairs;
  std::mutex tail_cache_mutex;
  std::vector<FrameRecord> frames;
                                                                           
                                                                              
                                                           
  aether::sfm::PortableVisualLoopIndexV1 visual_loop_index;
  aether_preclamp_instr_v1::SessionRecords preclamp_instr_records;
                                                                           
                                                                         
  colmap::camera_t camera_id = 0;

                                                                               
                                                                             
                                                                    
  std::vector<Eigen::Vector3d> preview_points;
  std::mutex preview_mutex;                                                   

                                                                                         
                                                                              
                                                                                    
  // cross-checked matches, refined by COLMAP's upstream six-image local
                                                         
                                                                                 
    
                                                                             
                                                                              
                                                                               
                                                                         
                                                                               
                                                                             
                                                                     
  std::shared_ptr<colmap::Reconstruction> live_recon;
  bool live_recon_ready = false;                                                   
                                                                              
                                                                             
                                                                      
  uint64_t live_cloud_diag_snapshot_seq_v2 = 0;
  bool live_cloud_diag_has_previous_points_v2 = false;
  uint64_t live_cloud_diag_previous_snapshot_seq_v2 = 0;
  aether::sfm::LiveCloudDiagPointMapV2 live_cloud_diag_previous_points_v2;
                                                                                             
                                                                                
                                                                                                    
                                                                                                   
  std::vector<aether_sfm_point_t> draft_pair_points;
  std::vector<int32_t> draft_pair_obs_offsets;
  std::vector<aether_sfm_track_obs_t> draft_pair_obs;
  std::vector<colmap::image_t> reg_order;                                                       
  int ba_window = 6;                                                       
  int ba_every_n = 1;                                                                
                                                                                                      
                                                                                         
                                                                                   
                                                                                   
                                                                                      
                                                                                   
                                                                               
                                                                                 
  size_t last_global_refine_reg = 0;                                             
  bool incremental_global_ba_ran = false;                                          
  int64_t stat_incremental_refines = 0;    // # incremental refines executed
                                                                           
                                                                                 
  int64_t stat_tvg_inlier_pairs = 0;                                             
  int64_t stat_raw_pairs = 0;                                                       
  int64_t stat_grow_rejected = 0;                                              
  int64_t stat_grow_accepted = 0;                                               
  int64_t stat_grow_reject_cheirality = 0;
  int64_t stat_grow_reject_reproj = 0;
  int64_t stat_create_reject_cheirality = 0;
  int64_t stat_create_reject_tri_angle = 0;
  int64_t stat_create_reject_reproj = 0;
  int64_t stat_already_assigned = 0;                                            
  int64_t stat_merge_needed = 0;                                              
  int64_t stat_merge_accepted = 0;                                              
  int64_t stat_merge_rejected = 0;                                          
                                                                           
  int64_t stat_merge_reject_shared_image = 0;                                 
  int64_t stat_merge_reject_reproj = 0;                                      
  int64_t stat_merge_reject_missing = 0;                                     
  int64_t stat_spatial_pairs_considered = 0;                                   
  int64_t stat_spatial_pairs_attempted = 0;                                  
  int64_t stat_spatial_pairs_written = 0;                                      
  int64_t stat_spatial_inliers = 0;                                              
  int64_t stat_spatial_anchor_attempted = 0;                                      
  int64_t stat_spatial_anchor_passed = 0;                                          
  int64_t stat_spatial_regions_confirmed = 0;                                  
  int64_t stat_spatial_expanded_attempted = 0;                              
  int64_t stat_spatial_guided_pairs = 0;                                       
  int64_t stat_spatial_guided_inliers = 0;                                       
  int64_t stat_spatial_quadratic_attempted = 0;                                 
  int64_t stat_spatial_quadratic_written = 0;                                  

                                                                        
  int64_t stat_official_tri_added = 0;
  int64_t stat_spatial_budget_skipped = 0;                                         
                                                                               
                                                                                          
                                                                                   
                                                                                  
                                                                                   
                                                                               
                                                                                 
                                                                                 
                                                                               
  int64_t stat_pose_direct_attempted = 0;                                      
  int64_t stat_pose_direct_rescued = 0;                                                  
  int64_t stat_pose_direct_inliers = 0;                                       
  int64_t stat_guided_temporal_attempted = 0;                                       
  int64_t stat_guided_temporal_upgraded = 0;                                         
  int64_t stat_guided_temporal_extra_inliers = 0;                                              
  int64_t stat_temporal_detail_pairs = 0;                                         
  int64_t stat_temporal_detail_matches = 0;                                    
  int64_t stat_temporal_detail_created = 0;                                  
  int64_t stat_temporal_detail_grown = 0;                                          
  int64_t stat_temporal_detail_reject_cheirality = 0;
  int64_t stat_temporal_detail_reject_reproj = 0;
  int64_t stat_temporal_detail_reject_tri_angle = 0;
  int64_t stat_temporal_detail_conflicts = 0;                                       

                                                                            
  int64_t stat_td_grow_refit_attempted = 0;                                   
  int64_t stat_td_grow_refit_accepted = 0;                                
  int64_t stat_td_grow_refit_saved = 0;                                                
                                                                         
                                                      
  int64_t stat_td_grow_refit_min_theta_rej = 0;
                                                                      
  int64_t stat_upgrade_eligible = 0;                                            
  int64_t stat_upgrade_attempted = 0;                                                   
  int64_t stat_upgrade_accepted = 0;                                   
  int64_t stat_upgrade_obs_added = 0;                                     
                                                                            
  int64_t stat_upgrade_pre_npts = 0;
  int64_t stat_upgrade_pre_2view = 0;
  int64_t stat_upgrade_pre_lt3 = 0;                     
  int64_t stat_upgrade_post_npts = 0;
  int64_t stat_upgrade_post_2view = 0;
  int64_t stat_upgrade_post_lt3 = 0;
  double upgrade_theta_p50_pre_deg = 0.0;
  double upgrade_theta_p50_post_deg = 0.0;
  double upgrade_ms = 0.0;
                                                                 
  int stat_mirror_ghost_logged = -1;                                                       
                                                                                         
                                                                                         
                                                                                               
                                                                                                
                                                                                                 
                                                                                              
                                                                                     
  std::mutex deliver_kill_mutex;
  std::unordered_set<colmap::point3D_t> deliver_kill_cache;
  const void* deliver_kill_recon = nullptr;
  size_t deliver_kill_npts = 0;
  double deliver_kill_ms = 0.0;
  int deliver_kill_mg = 0;
  int deliver_kill_if = 0;
  int stat_isolated_floater_logged = -1;                                            
  int64_t stat_enrich_targeted_tracks = 0;                                    
  int64_t stat_enrich_targeted_scored = 0;                                
                                                                                
                                                                         
                                                                       
  std::shared_ptr<const EnrichTargetHint> enrich_hint;
  int64_t stat_reproj_filtered = 0;                                              
  int64_t stat_tri_filtered = 0;                                                    

                                                                           
                                                                             
  int64_t stat_cand_spatial_first_pairs = 0;
  int64_t stat_cand_temporal_fallback_pairs = 0;
                                                                       
                                                                             
                                                                               
                                                                             
                                                                     
                                                                            
                                                                               
                                                                             
                                                                              
                        
  int64_t stat_gpu_match_fail_total = 0;
  int64_t stat_gpu_match_fail_by_rc[8] = {0};
  int64_t stat_gpu_match_fail_max_streak = 0;                                 
  int64_t gpu_match_fail_streak = 0;                                          
                                                                           
                                                                             
                                                                        
                                                                      
                                                                           
  int64_t gpu_pairs_since_rc7 = 0;
                                                                              
                                                                             
                                                                                 
                                                                     
                                                                               
                                              
  std::atomic<int> thermal_state{-1};
  int64_t stat_thermal_throttled_frames = 0;                              
  bool throttle_active_logged = false;                                        
                                                                                
                                                                                       
                                                                                       
                                                                                                
  double stat_phase2_gftol = 0.0;                                            
  int stat_phase2_gref = 0;                                               
  int stat_phase2_giter = 0;                                                 
                                                                             
  int stat_phase2_ftol_ab_arm = -1;
                                                                            
                                                 
  int64_t stat_finalize_rematch_starved_frames = 0;
  int64_t stat_finalize_rematch_candidates = 0;                               
  int64_t stat_finalize_rematch_attempted = 0;                         
  int64_t stat_finalize_rematch_written = 0;                                 
  int64_t stat_finalize_rematch_inliers = 0;                             
  int64_t stat_finalize_rematch_failed = 0;                                
                                                                 
                                                                           
                                                                           
                                         
  int64_t stat_gpu_retry_attempts = 0;
  int64_t stat_gpu_retry_recovered = 0;
                                                                            
                                                                          
                                                                        
                                                                 
                                                                     
                                                                       
                                            
  size_t prepay_scan = 0;
  std::deque<std::pair<int, int>> prepay_due;
  int64_t stat_prepay_attempted = 0;                                  
  int64_t stat_prepay_written = 0;                                
                                                                    
                                                                          
                                                                           
                                                                              
                                                    
                                                                     
                                                                
  int64_t stat_probe_attempted = 0;                              
  int64_t stat_probe_skipped = 0;                                             
  int64_t stat_probe_passed = 0;                                             
  int64_t stat_probe_fail_open = 0;                                            
  double stat_probe_ms = 0.0;                                          
                                                                         
                                                                            
                                                                            
                                                                          
                                                                      
  int64_t stat_probe_batch_calls = 0;
                                                                             
                                                                             
                                                                              
                                                                             
                                                                           
                                                                              
                                                                             
                                                                         
  std::unordered_set<uint64_t> probe_skipped_pairs;
  int64_t stat_probe_debt_registered = 0;                                       
  int64_t stat_probe_debt_repaid_live = 0;                              
  int64_t stat_probe_debt_grow_live_pairs = 0;                                   
  int64_t stat_probe_debt_repaid_finalize = 0;                                 
                                                                             
                                                                        
                                                                        
                                                                            
                                                            
                                                                             
                                                                            
                                                                            
                                                                         
                                                                       
                                                                    
  std::vector<ProbeDebtGrowPair> probe_debt_grow;
  int64_t stat_probe_debt_grow_pairs = 0;                                    
  int64_t stat_probe_debt_grow_added = 0;                            
  int64_t stat_probe_debt_grow_grown = 0;                                   
  int64_t stat_probe_debt_grow_touched = 0;                                
  double stat_probe_debt_grow_ms = 0.0;                                
                                                                      
                                                                            
                                                                         
  int64_t stat_idle_prepay_ticks = 0;
                                                                              
                                                          
                                                             
  double last_add_frame_done_ms = 0.0;

  double t1_stage1_ba_ms = 0.0;
  double t1_stage1_merge_ms = 0.0;
  double t1_enrich_gate_wait_ms = 0.0;
                                                                      
  int64_t stat_epi_attempted = 0;                                        
  int64_t stat_epi_fallback = 0;                                                 
                                                                     
                                                                           
  double last_extract_stages[9] = {0};
                                                                                          
                                                                                                  
                                                                                                   
                                                                                      
                                                                                            
                                                     
  std::thread pf_thread;
  std::mutex pf_mu;
  std::condition_variable pf_cv;
  bool pf_stop = false;
  bool pf_started = false;
  struct PfJob {
    std::vector<uint8_t> gray;                                                             
    const uint8_t* borrow = nullptr;
    int w = 0, h = 0;
    uint64_t digest = 0;
    bool valid = false;
  } pf_job;
  struct PfResult {
    uint64_t digest = 0;
    int w = 0, h = 0;
    std::vector<float> xy;
    std::vector<uint8_t> desc;
    std::vector<float> scales;
    int n = 0;
    int erc = 1;
    bool have_scales = false;
    double extract_ms = 0.0;
    double stages[9] = {0};
    bool valid = false;
  } pf_result;
                                                                                                     
                                                                                                        
                                                                                                         
                                                                                                           
                                                                                                            
                                                   
  PfJob pf_sync_job;
  PfResult pf_sync_result;
  int64_t stat_pf_hits = 0;                                         
  int64_t stat_pf_waits = 0;                                     
  int64_t stat_pf_misses = 0;                                                    
  double last_pf_wait_ms = 0.0;
  int last_pf_state = 0;                                      
                                                                           
                                                                           
                                                                        
  uint64_t gpu_timestamp_run_id = 0;
  uint64_t gpu_timestamp_frame_ordinal = 0;
  uint64_t gpu_timestamp_next_record_id = 1;
  bool gpu_timestamp_probe_written = false;
  uint64_t gpu_timestamp_probe_instance_id = 0;
  bool gpu_timestamp_pending_frame_ready = false;
  uint64_t gpu_timestamp_pending_frame_ordinal = 0;
  AetherGpuTimestampFrameV1 gpu_timestamp_pending_frame{};
                                                                      
                                                                           
                                                                            
                                                                           
                                                                    
  std::thread async_pvba_thread;
  std::atomic<bool> async_pvba_running{false};
  std::atomic<bool> async_pvba_done{false};
  std::shared_ptr<colmap::Reconstruction> async_pvba_result;
  std::mutex async_pvba_mutex;
  int apvba_snapshot_nframes = 0;                                 
                                                                      
                                                                            
                                                                       
                                                           
  std::unordered_map<colmap::frame_t, colmap::Rigid3d> apvba_snapshot_poses;
  double stat_apvba_last_corr_mm = 0.0;                                      
  double stat_apvba_last_corr_deg = 0.0;                       
                                                                              
                                                                             
                                                                      
  int apvba_consecutive_drops = 0;
  int apvba_skip_kicks = 0;
  bool apvba_kick_armed = true;                                             
  std::string apvba_last_error;
  int64_t stat_apvba_kicks = 0;
  int64_t stat_apvba_merges = 0;
  int64_t stat_apvba_dropped = 0;                                   
  double stat_apvba_last_ba_ms = 0.0;
  double stat_apvba_merge_ms_total = 0.0;
  double stat_apvba_copy_ms_total = 0.0;
                                                                         
                                                                            
  int64_t stat_repay_calls = 0;
  int64_t stat_repay_attempted = 0;                                       
  int64_t stat_repay_written = 0;                                      
  int64_t stat_repay_inliers = 0;                                           
  int64_t stat_repay_failed = 0;                                               
  int64_t stat_repay_skipped_thermal = 0;                                    
                                                                             
                                                                           
                                                                             
                                                                              
                                                                             
                                                                           
  std::vector<int> live_win_valid;
  std::unordered_set<uint64_t> live_pairs_done;
                                                                           
                                                                          
                                                                           
                                                                              
  double enrich_start_ms = 0.0;
  std::atomic<bool> enrich_stage1_done{false};
  int64_t stat_enrich_budget_stopped = 0;                                       

                                                                       
                                                                              
                                                                             
  int64_t stat_frag_links = 0;                                                    
  int64_t stat_frag_attempted = 0;                                           
  int64_t stat_frag_accepted = 0;                              
  int64_t stat_frag_obs_merged = 0;                                                
  int64_t stat_frag_reject_reproj = 0;                                           
  int64_t stat_frag_reject_theta = 0;                                      
                                                                                        
  int64_t stat_frag_reject_min_theta = 0;
  int64_t stat_frag_reject_degenerate = 0;                             
  int64_t stat_frag_pre_npts = 0, stat_frag_pre_2view = 0, stat_frag_pre_lt3 = 0;
  int64_t stat_frag_post_npts = 0, stat_frag_post_2view = 0, stat_frag_post_lt3 = 0;
  double frag_theta_p50_pre_deg = 0.0;
  double frag_theta_p50_post_deg = 0.0;
  double frag_ms = 0.0;

                                                            
  std::shared_ptr<colmap::ReconstructionManager> recon_manager;
  std::shared_ptr<const colmap::Reconstruction> recon;                        

                                                                             
                                                                              
                                                                                 
                                                                                  
                                                     
  std::mutex recon_mutex;
  std::thread refine_thread;
  std::atomic<int> finalize_status{0};                                 
  double refine_ms = 0.0;                                            

                                                                               
                                                
                                                                
                                                                              
  double last_extract_ms = 0.0;                                                              
  double last_match_ms = 0.0;                                                    
  int last_n_cand = 0;            // # previous frames in the window matched against
  int last_gpu_matches = 0;                                                   
  int last_cpu_matches = 0;                                                                      

                                                                                      
                                                                                     
                                                                                        
  aether_gpu_watchdog_v1::WatchdogV1 gpu_watchdog;
};

namespace aether_preclamp_instr_v1 {

void ResetOfficialSessionRecords(::aether_sfm_session* session) noexcept {
  if (session == nullptr) return;
  ResetSessionRecords(&session->preclamp_instr_records);
}

bool CopyOfficialSessionRecords(
    const ::aether_sfm_session* session, std::vector<FrameCounts>* out) {
  if (session == nullptr) return false;
  return CopySessionRecords(&session->preclamp_instr_records, out);
}

bool DrainOfficialSessionRecords(
    ::aether_sfm_session* session, std::vector<FrameCounts>* out) noexcept {
  if (session == nullptr) return false;
  return DrainSessionRecords(&session->preclamp_instr_records, out);
}

}                                       

namespace {

enum class TailCacheModeV1 { kOff, kShadow, kOn };

                                                                                
                                                                                
                                                                             
                                                                
                                                                             
                                                                               
                                                                                
                                                                           
                                                                                  
                                                           
                                                                                 
                                                                            
                                                                           
                                                                            
                                                                               
                                                    
TailCacheModeV1 TailCacheMode() {
  static const TailCacheModeV1 cached = [] {
    const char* value = std::getenv("OFFICIAL_AETHER_TAIL_CACHE_V1");
    if (!value) return TailCacheModeV1::kOn;                                
    if (std::strcmp(value, "shadow") == 0) return TailCacheModeV1::kShadow;
    if (std::strcmp(value, "0") == 0 || std::strcmp(value, "off") == 0) {
      return TailCacheModeV1::kOff;
    }
    return TailCacheModeV1::kOn;
  }();
  return cached;
}

bool TailCacheActive() { return TailCacheMode() != TailCacheModeV1::kOff; }

bool DescriptorResidencyEnabledCore() {
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
  static const bool cached = [] {
    const char* value =
        std::getenv("OFFICIAL_AETHER_DESCRIPTOR_RESIDENCY_V1");
    return value && value[0] == '1' && value[1] == '\0';
  }();
  return cached;
#else
  return false;
#endif
}

bool TailCacheTraceEnabled() {
  static const bool cached = [] {
    const char* value = std::getenv("OFFICIAL_AETHER_TAIL_CACHE_V1");
    if (!value) return false;
    return std::strcmp(value, "off") == 0 ||
           std::strcmp(value, "shadow") == 0 ||
           std::strcmp(value, "1") == 0 || std::strcmp(value, "on") == 0;
  }();
  return cached;
}

const char* TailCacheDirtyReasonName(
    aether::sfm::TailCacheDirtyReasonV1 reason) {
  using Reason = aether::sfm::TailCacheDirtyReasonV1;
  switch (reason) {
    case Reason::kNone: return "none";
    case Reason::kPairOverwrite: return "pair_overwrite";
    case Reason::kRemoveFrame: return "remove_frame";
    case Reason::kLatePair: return "late_pair";
    case Reason::kModelReplacement: return "model_replacement";
    case Reason::kFinalizeMove: return "finalize_move";
    case Reason::kReconstructionSwap: return "reconstruction_swap";
    case Reason::kConfigChange: return "config_change";
    case Reason::kCacheInconsistency: return "cache_inconsistency";
    case Reason::kExceptionRetry: return "exception_retry";
    case Reason::kDeviceReset: return "device_reset";
    case Reason::kUnknownMutation: return "unknown_mutation";
  }
  return "unknown_mutation";
}

int64_t EpochMs();
void AppendMatchFailJsonl(aether_sfm_session* s, const std::string& line);

void AppendTailCacheEpochEventLockedV1(aether_sfm_session* s,
                                       const char* phase) {
  if (!s || !phase || !TailCacheTraceEnabled()) return;
  const auto& epoch = s->tail_cache_epoch;
  const std::string line =
      "{\"t\":" + std::to_string(static_cast<long long>(EpochMs())) +
      ",\"type\":\"tail_cache_epoch_event_v1\",\"phase\":\"" + phase +
      "\",\"state\":" + std::to_string(static_cast<int>(epoch.state())) +
      ",\"generation\":" + std::to_string(epoch.generation()) +
      ",\"invalidated_generation\":" +
      std::to_string(epoch.invalidated_generation()) +
      ",\"rebuild_parent_generation\":" +
      std::to_string(epoch.rebuild_parent_generation()) +
      ",\"dirty_reason\":\"" + TailCacheDirtyReasonName(epoch.dirty_reason()) +
      "\"}";
  AppendMatchFailJsonl(s, line);
}

void TailCacheMarkDirtyLockedV1(
    aether_sfm_session* s, aether::sfm::TailCacheDirtyReasonV1 reason,
    const char* phase = "dirty_before_mutation") {
  s->tail_cache_epoch.MarkDirty(reason);
  AppendTailCacheEpochEventLockedV1(s, phase);
}

void TailCacheInitialize(aether_sfm_session* s) {
  if (!s || !s->db || !TailCacheActive()) return;
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  s->tail_cache_master = std::make_shared<colmap::DatabaseCache>();
  s->tail_cache_epoch.StartEmpty();
  s->tail_cache_seen_db_pairs.clear();
  if (s->db->NumImages() != 0) {
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kConfigChange);
  }
}

void TailCacheMarkDirty(aether_sfm_session* s,
                        aether::sfm::TailCacheDirtyReasonV1 reason) {
  if (!s || !TailCacheActive()) return;
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  TailCacheMarkDirtyLockedV1(s, reason);
}

struct TailCacheMutableBuildV1 {
  std::shared_ptr<colmap::DatabaseCache> cache;
  std::vector<uint32_t> image_ids;
  std::vector<uint64_t> qualifying_pair_ids;
  std::unordered_set<colmap::image_pair_t> seen_db_pairs;
};

TailCacheMutableBuildV1 BuildMutableTailCacheV1(
    const colmap::Database& database) {
  TailCacheMutableBuildV1 out;
  out.cache = std::make_shared<colmap::DatabaseCache>();
  const bool has_rigs = database.NumRigs() > 0;
  const bool has_frames = database.NumFrames() > 0;

  if (has_rigs) {
    for (auto rig : database.ReadAllRigs()) out.cache->AddRig(std::move(rig));
  }
  for (auto camera : database.ReadAllCameras()) {
    if (!has_rigs) {
      colmap::Rig rig;
      rig.SetRigId(camera.camera_id);
      rig.AddRefSensor(camera.SensorId());
      out.cache->AddRig(std::move(rig));
    }
    out.cache->AddCamera(std::move(camera));
  }
  if (has_frames) {
    for (auto frame : database.ReadAllFrames()) {
      out.cache->AddFrame(std::move(frame));
    }
  }

  for (auto image : database.ReadAllImages()) {
    if (!has_frames) {
      colmap::Frame frame;
      frame.SetFrameId(image.ImageId());
      frame.SetRigId(image.CameraId());
      frame.AddDataId(image.DataId());
      image.SetFrameId(frame.FrameId());
      out.cache->AddFrame(std::move(frame));
    }
    image.SetPoints2D(colmap::FeatureKeypointsToPointsVector(
        database.ReadKeypoints(image.ImageId())));
    out.image_ids.push_back(static_cast<uint32_t>(image.ImageId()));
    out.cache->AddImage(std::move(image));
  }

  for (auto& [pair_id, geometry] : database.ReadTwoViewGeometries()) {
    out.seen_db_pairs.insert(pair_id);
    if (geometry.inlier_matches.size() < 15) continue;
    const auto [image1, image2] = colmap::PairIdToImagePair(pair_id);
    out.cache->CorrespondenceGraph()->AddTwoViewGeometry(
        image1, image2, std::move(geometry));
    out.qualifying_pair_ids.push_back(pair_id);
  }
  return out;
}

bool TailCacheRebuildLocked(aether_sfm_session* s) {
  if (!s || !s->db ||
      s->tail_cache_epoch.state() !=
          aether::sfm::TailCacheEpochStateV1::kDirty ||
      !s->tail_cache_epoch.BeginRebuild()) {
    return false;
  }
  AppendTailCacheEpochEventLockedV1(s, "rebuild_begin");
  try {
                                                                                
                                                                      
    colmap::DatabaseCache::Options oracle_options;
    oracle_options.min_num_matches = 15;
    oracle_options.load_all_images = true;
    const auto oracle = colmap::DatabaseCache::Create(*s->db, oracle_options);
    (void)oracle;

    TailCacheMutableBuildV1 rebuilt = BuildMutableTailCacheV1(*s->db);
    s->tail_cache_epoch.CompleteRebuild(rebuilt.image_ids,
                                       rebuilt.qualifying_pair_ids);
    if (!s->tail_cache_epoch.readable()) return false;
    s->tail_cache_master = std::move(rebuilt.cache);
    s->tail_cache_seen_db_pairs = std::move(rebuilt.seen_db_pairs);
    AppendTailCacheEpochEventLockedV1(s, "rebuild_complete");
    return true;
  } catch (...) {
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency,
        "rebuild_failed");
    return false;
  }
}

void TailCacheMirrorImage(aether_sfm_session* s,
                          const colmap::Camera& camera,
                          colmap::image_t image_id,
                          const char* name,
                          const std::vector<Eigen::Vector2d>& points) {
  if (!s || !TailCacheActive()) return;
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  if (!s->tail_cache_epoch.readable() || !s->tail_cache_master) return;
  try {
    colmap::Rig rig;
    rig.SetRigId(camera.camera_id);
    rig.AddRefSensor(camera.SensorId());
    s->tail_cache_master->AddRig(std::move(rig));
    s->tail_cache_master->AddCamera(camera);

    colmap::Image image;
    image.SetImageId(image_id);
    image.SetName(name);
    image.SetCameraId(camera.camera_id);
    image.SetPoints2D(points);
    colmap::Frame frame;
    frame.SetFrameId(image_id);
    frame.SetRigId(camera.camera_id);
    frame.AddDataId(image.DataId());
    image.SetFrameId(frame.FrameId());
    s->tail_cache_master->AddFrame(std::move(frame));
    s->tail_cache_master->AddImage(std::move(image));
    if (!s->tail_cache_epoch.AdmitImage(static_cast<uint32_t>(image_id))) {
      TailCacheMarkDirtyLockedV1(
          s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
    }
  } catch (...) {
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
  }
}

bool TailCacheBeginPairWrite(aether_sfm_session* s,
                             colmap::image_t image1,
                             colmap::image_t image2,
                             bool late_write = false) {
  if (!s || !TailCacheActive()) return false;
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  const colmap::image_pair_t pair_id =
      colmap::ImagePairToPairId(image1, image2);
  const bool first_write = s->tail_cache_seen_db_pairs.insert(pair_id).second;
  if (!first_write) {
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kPairOverwrite);
  } else if (late_write) {
                                                                       
                                                                            
                                                                             
                                         
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kLatePair);
  }
  return first_write;
}

void TailCacheCommitPair(aether_sfm_session* s,
                         colmap::image_t image1,
                         colmap::image_t image2,
                         const colmap::TwoViewGeometry& geometry,
                         bool first_write) {
  if (!s || !TailCacheActive() || !first_write ||
      geometry.inlier_matches.size() < 15) {
    return;
  }
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  if (!s->tail_cache_epoch.readable() || !s->tail_cache_master) return;
  const colmap::image_pair_t pair_id =
      colmap::ImagePairToPairId(image1, image2);
  if (!s->tail_cache_epoch.AdmitPair(pair_id)) return;
  try {
    s->tail_cache_master->CorrespondenceGraph()->AddTwoViewGeometry(
        image1, image2, geometry);
  } catch (...) {
    TailCacheMarkDirtyLockedV1(
        s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
  }
}

struct SpatialRevisitCandidate {
  int i = -1;
  int j = -1;
  double distance_m = 0.0;
  double angle_rad = 0.0;
  double score = 0.0;
  int anchor_rank = 0;
  bool quadratic = false;
                                                                              
                                                                              
  int target_score = 0;
};

Eigen::Vector3d CameraForwardWorld(const colmap::Rigid3d& cam_from_world) {
  return cam_from_world.rotation().inverse() * Eigen::Vector3d(0.0, 0.0, 1.0);
}

                                                                            
  
                                                                               
                                                                              
                                                                               
                                                                        
                                                                           
                                                                              
                                                                                             
  
                                                                                
                                                                                
                                           
                                                                           
                                                                              
                                                                            
                                                                            
                                                                            
                                                                               
                                                      
                                                                        
                                                                           
                                                                          
                                                                             
                                                                             
                                    
  
                                                                          
                                                                             
                                                                               
                                                                       
                                                    
constexpr double kStreamCandViewAngleMaxRad = 45.0 * M_PI / 180.0;

std::vector<int> SelectStreamCandidates(const aether_sfm_session& s,
                                        const FrameRecord& rec, int frame_id,
                                        int k, int* out_spatial,
                                        int* out_temporal) {
  std::vector<int> selected;
  if (out_spatial) *out_spatial = 0;
  if (out_temporal) *out_temporal = 0;
  if (k <= 0 || frame_id <= 0) return selected;
  selected.reserve(k);

                                                                        
                                                                                
                                                                             
                                                                          
                                                                            
                              
                                                                       
                                                                          
                                                                            
             
  const auto usable = [&](int j) {
    const FrameRecord& prev = s.frames[j];
    return prev.image_id != 0 && prev.n_keypoints > 0 &&
           !prev.descriptors.empty();
  };

                                                                                        
                                                                          
                                                 
  static const bool temporal_only = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_STREAM_TEMPORAL_ONLY");
    return e && e[0] == '1';
  }();

  int n_spatial = 0;
  if (!temporal_only && rec.has_pose) {
    const Eigen::Vector3d center = rec.cam_from_world.TgtOriginInSrc();
    const Eigen::Vector3d forward = CameraForwardWorld(rec.cam_from_world);
    const double min_dot = std::cos(kStreamCandViewAngleMaxRad);
    std::vector<std::pair<double, int>> compatible;                       
    compatible.reserve(frame_id);
    for (int j = 0; j < frame_id; ++j) {
      const FrameRecord& prev = s.frames[j];
      if (!prev.has_pose || !usable(j)) continue;
      const double dot = std::max(
          -1.0,
          std::min(1.0, forward.dot(CameraForwardWorld(prev.cam_from_world))));
      if (dot < min_dot) continue;
      compatible.emplace_back(
          (center - prev.cam_from_world.TgtOriginInSrc()).squaredNorm(), j);
    }
    n_spatial = std::min(k, static_cast<int>(compatible.size()));
    std::partial_sort(compatible.begin(), compatible.begin() + n_spatial,
                      compatible.end());                                          
    for (int t = 0; t < n_spatial; ++t) {
      selected.push_back(compatible[t].second);
    }
  }
  if (out_spatial) *out_spatial = n_spatial;

                                                                          
                   
  if (static_cast<int>(selected.size()) < k) {
    std::unordered_set<int> chosen(selected.begin(), selected.end());
    for (int j = frame_id - 1;
         j >= 0 && static_cast<int>(selected.size()) < k; --j) {
      if (!usable(j) || !chosen.insert(j).second) continue;
      selected.push_back(j);
      if (out_temporal) ++*out_temporal;
    }
  }
                                                                                
                                                            
  std::sort(selected.begin(), selected.end());
  return selected;
}

                                                        
  
                                                                               
                                                                               
                                                                               
                                                                            
                                                                           
                     
  
                                                                               
                                                                           
                                                               
std::vector<int> SelectSpatialK20TemporalT2Candidates(
    const aether_sfm_session& s, const FrameRecord& rec, const int frame_id,
    const bool thermal_throttled, const int thermal_spatial_k,
    int* out_spatial, int* out_temporal) {
  if (out_spatial) *out_spatial = 0;
  if (out_temporal) *out_temporal = 0;
  if (frame_id <= 0) return {};

  aether_pair_policy_config_v2 abi_config{};
  aether_pair_policy_config_v2_default(&abi_config);
  if (aether_pair_policy_config_v2_validate(&abi_config) !=
      AETHER_PAIR_POLICY_V2_OK) {
    return {};
  }

  static const bool temporal_only = [] {
    const char* value = std::getenv("OFFICIAL_AETHER_STREAM_TEMPORAL_ONLY");
    return value != nullptr && value[0] == '1';
  }();
  if (temporal_only) {
    return SelectStreamCandidates(s, rec, frame_id, abi_config.spatial_k,
                                  out_spatial, out_temporal);
  }

  std::vector<aether::sfm::PairSelectionFrameV2> history;
  history.reserve(static_cast<size_t>(frame_id));
  for (int index = 0; index < frame_id; ++index) {
    const FrameRecord& previous = s.frames[index];
    aether::sfm::PairSelectionFrameV2 frame;
    frame.frame_id = index;
    frame.pose_valid = previous.has_pose;
    frame.matchable = previous.n_keypoints > 0 && !previous.descriptors.empty();
    if (previous.has_pose) {
      const Eigen::Vector3d center =
          previous.cam_from_world.TgtOriginInSrc();
      const Eigen::Vector3d forward =
          CameraForwardWorld(previous.cam_from_world);
      frame.center_xyz = {center.x(), center.y(), center.z()};
      frame.forward_xyz = {forward.x(), forward.y(), forward.z()};
    }
    history.push_back(frame);
  }

  aether::sfm::PairSelectionFrameV2 current;
  current.frame_id = frame_id;
  current.pose_valid = rec.has_pose;
  current.matchable = rec.n_keypoints > 0 && !rec.descriptors.empty();
  if (rec.has_pose) {
    const Eigen::Vector3d center = rec.cam_from_world.TgtOriginInSrc();
    const Eigen::Vector3d forward = CameraForwardWorld(rec.cam_from_world);
    current.center_xyz = {center.x(), center.y(), center.z()};
    current.forward_xyz = {forward.x(), forward.y(), forward.z()};
  }

  aether::sfm::PairSelectionConfigV2 config;
                                                                                           
                                                                                 
                                                                                   
                                                                                            
                                                                                    
                                                                                      
                                                                                       
                                                                                
                                                                                    
                                                                                                   
  config.spatial_k =
      std::max(0, thermal_spatial_k - abi_config.temporal_lookback);
  (void)thermal_throttled;                                                                
  config.temporal_lookback = abi_config.temporal_lookback;
  config.spatial_recent_exclusion = abi_config.spatial_recent_exclusion;
  const aether::sfm::PairSelectionResultV2 result =
      aether::sfm::SelectSpatialTemporalCandidatesV2(history, current, config);

  std::vector<int> selected;
  selected.reserve(result.ordered_pairs.size());
  int spatial = 0;
  int temporal = 0;
  const uint32_t spatial_mask =
      static_cast<uint32_t>(aether::sfm::PairCandidateSourceV2::kSpatial);
  const uint32_t temporal_mask =
      static_cast<uint32_t>(aether::sfm::PairCandidateSourceV2::kTemporal);
  for (const aether::sfm::CanonicalPairCandidateV2& pair :
       result.ordered_pairs) {
    if (pair.second_frame_id != frame_id || pair.first_frame_id < 0 ||
        pair.first_frame_id >= frame_id) {
      continue;
    }
    selected.push_back(pair.first_frame_id);
    if ((pair.source_mask & spatial_mask) != 0U) ++spatial;
    if ((pair.source_mask & temporal_mask) != 0U) ++temporal;
  }
  if (out_spatial) *out_spatial = spatial;
  if (out_temporal) *out_temporal = temporal;
  return selected;
}

                                                                        
                                                                            
                                                                              
                                                                             
                                                                                
                                                                           
                                                                         
                                                
constexpr int kGpuMatchFailStreakWarn = 8;

                                                                         
                                                                               
                                                                            
int64_t EpochMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

                                                              
                                                                         
                                                                        
                                                                          
                                                                               
                                                                         
                                                                            
                                                                         
                                                                            
                                                           
                                                                                                 
                                                                                             
                                                                                        
void AppendMatchFailJsonlAtDbPath(const std::string& db_path,
                                  const std::string& line) {
  try {
    const std::filesystem::path p =
        std::filesystem::path(db_path).parent_path() /
        "sfm_match_fail.jsonl";
    FILE* f = std::fopen(p.string().c_str(), "a");
    if (!f) return;
    std::fputs(line.c_str(), f);
    std::fputc('\n', f);
    std::fclose(f);
  } catch (...) {
                                                   
  }
}

void AppendMatchFailJsonl(aether_sfm_session* s, const std::string& line) {
  if (!s) return;
  AppendMatchFailJsonlAtDbPath(s->db_path, line);
}

                                                                                                           
  
                                                                       
                                                                                               
                                                                                           
                                                                                               
                                                                                                  
                                                                               
                                         
  
                                                                                                 
                                                                                              
  
                                                                                       
                                                                                                 
                                                                                                 
  
                                                                                      
                                                                               
                     
                                                                             
                                                                                     
  
                                                                                                
                                                                                               
                                                                                                
                                                                        
  
                                                                            
                                                                                          
                                                                                         
                                                                                                
                                                                                           
                                                                                                
  
                                                                                                
                                                                                     
                                                                                                       
int ExtractWarmupMode() {
  static const int mode = [] {
    const char* v = std::getenv("OFFICIAL_AETHER_EXTRACT_WARMUP");
    if (v == nullptr || v[0] == '\0') return 0;
    if (v[0] == '0' && v[1] == '\0') return 0;
    if (v[1] != '\0') return 1;                                               
    if (v[0] == '2') return 2;
    if (v[0] == '3') return 3;                                                 
    return 1;                                                                         
  }();
  return mode;
}

                                                                                             
                   
                                                                 
                                                                                 
                                                                 
  
                                                                                              
                                                                                         
                                                                                                      
                                                                                     
                                                            
  
                                                                                                       
                                                                                                       
                                                                                           
                                                                    
  
                                                                                                 
                                                                                                  
                                                                      
const char* ExtractWarmupQosName() {
  return ExtractWarmupMode() == 3 ? "user_initiated" : "utility";
}


                                                                                                   
                                                                                                      
std::mutex g_warmup_mu;
std::string g_warmup_db_path;                              
std::string g_warmup_line;                           
bool g_warmup_line_written = false;

                                    
void FlushWarmupLineLocked() {
  if (g_warmup_line_written) return;
  if (g_warmup_db_path.empty() || g_warmup_line.empty()) return;
  g_warmup_line_written = true;
  const std::string path = g_warmup_db_path;
  const std::string line = g_warmup_line;
  AppendMatchFailJsonlAtDbPath(path, line);
}

                                                                                                          
void KickExtractWarmupOnce(const char* when) {
  if (ExtractWarmupMode() == 0) return;
  if (aether_dsp_sift_extract_gpu_warmup == nullptr) return;                         
  static std::once_flag once;
  std::call_once(once, [when] {
                                                                                              
                                                                                             
    std::thread([when] {
#if defined(__APPLE__)
                                                              
        
                                                                                                      
                                                                                                           
                                                                                             
                                                                                     
                                                         
        
                                                                                                         
                                                                                          
                                                                                           
                                                                                                  
      pthread_set_qos_class_self_np(
          ExtractWarmupMode() == 3 ? QOS_CLASS_USER_INITIATED : QOS_CLASS_UTILITY, 0);
#endif
      const double t0 = NowMs();
      double init_ms = 0.0, compile_ms = 0.0, msl_ms = 0.0, pso_ms = 0.0;
      unsigned msl_n = 0u, pso_n = 0u;
      const int n_new = aether_dsp_sift_extract_gpu_warmup(
          &init_ms, &compile_ms, &msl_ms, &msl_n, &pso_ms, &pso_n);
                                                                                                       
                                                                                                
                                                                                                
      const double tint_ms = compile_ms - msl_ms - pso_ms;
      const double ms = NowMs() - t0;
                                                        
                                                                                                  
                                                           
                                          
                                                                                         
                                                                           
      char line[512];
      std::snprintf(line, sizeof(line),
                    "{\"t\":%lld,\"type\":\"extract_warmup_v1\","
                    "\"ms\":%.1f,\"init_ms\":%.1f,\"compile_ms\":%.1f,"
                    "\"n_new_pipelines\":%d,\"at\":\"%s\",\"mode\":%d,"
                    "\"qos\":\"%s\","
                    "\"tint_ms\":%.1f,\"msl_ms\":%.1f,\"msl_n\":%u,"
                    "\"pso_ms\":%.1f,\"pso_n\":%u}",
                    static_cast<long long>(EpochMs()), ms, init_ms, compile_ms,
                    n_new, when != nullptr ? when : "?", ExtractWarmupMode(),
                    ExtractWarmupQosName(), tint_ms, msl_ms, msl_n, pso_ms, pso_n);
      std::lock_guard<std::mutex> lk(g_warmup_mu);
      g_warmup_line = line;
      FlushWarmupLineLocked();                                                           
    }).detach();
  });
}

                                                                            
                                                                                 
                                                                               
                                                                                
void AppendLiveCloudSnapshotDiagnosticsV2(aether_sfm_session* s,
                                          const char* stage) {
  try {
    if (!s || !stage || !s->live_recon_ready || !s->live_recon) return;
    const double snapshot_start_ms = NowMs();
    const uint64_t snapshot_seq = ++s->live_cloud_diag_snapshot_seq_v2;

    std::vector<aether::sfm::LiveCloudArkitBaCenterPairV2> center_pairs;
    center_pairs.reserve(s->frames.size());
    for (const FrameRecord& frame : s->frames) {
      if (!frame.has_pose || frame.image_id == 0 ||
          !s->live_recon->ExistsImage(frame.image_id)) {
        continue;
      }
      const colmap::Image& image = s->live_recon->Image(frame.image_id);
      if (!image.HasPose()) continue;
      const Eigen::Vector3d arkit_center =
          frame.cam_from_world.TgtOriginInSrc();
      const Eigen::Vector3d ba_center =
          image.CamFromWorld().TgtOriginInSrc();
      center_pairs.push_back({
          {arkit_center.x(), arkit_center.y(), arkit_center.z()},
          {ba_center.x(), ba_center.y(), ba_center.z()},
      });
    }
    const aether::sfm::LiveCloudArkitBaSim3SummaryV2 sim3 =
        aether::sfm::SummarizeLiveCloudArkitBaSim3V2(center_pairs);
    const double sim3_compute_ms = NowMs() - snapshot_start_ms;
    char sim3_line[3072];
    std::snprintf(
        sim3_line, sizeof(sim3_line),
        "{\"t\":%lld,\"type\":\"live_cloud_arkit_ba_sim3_v2\","
        "\"contract\":\"PW_LIVE_CLOUD_SNAPSHOT_DIAG_V2_STREAM\","
        "\"stage\":\"%s\",\"snapshot_seq\":%llu,"
        "\"valid\":%s,\"status\":\"%s\",\"input_pairs\":%zu,"
        "\"pair_count\":%zu,\"inlier_count\":%zu,"
        "\"ba_to_arkit_scale\":%.12g,"
        "\"ba_to_arkit_tx_m\":%.12g,\"ba_to_arkit_ty_m\":%.12g,"
        "\"ba_to_arkit_tz_m\":%.12g,\"ba_to_arkit_qx\":%.12g,"
        "\"ba_to_arkit_qy\":%.12g,\"ba_to_arkit_qz\":%.12g,"
        "\"ba_to_arkit_qw\":%.12g,\"ba_to_arkit_rotation_deg\":%.12g,"
        "\"arkit_to_ba_scale\":%.12g,"
        "\"arkit_to_ba_tx_m\":%.12g,\"arkit_to_ba_ty_m\":%.12g,"
        "\"arkit_to_ba_tz_m\":%.12g,\"arkit_to_ba_qx\":%.12g,"
        "\"arkit_to_ba_qy\":%.12g,\"arkit_to_ba_qz\":%.12g,"
        "\"arkit_to_ba_qw\":%.12g,\"arkit_to_ba_rotation_deg\":%.12g,"
        "\"residual_p50_m\":%.12g,\"residual_p90_m\":%.12g,"
        "\"residual_max_m\":%.12g,\"residual_all_max_m\":%.12g,"
        "\"compute_ms\":%.3f,\"observation_only\":true}",
        static_cast<long long>(EpochMs()), stage,
        static_cast<unsigned long long>(snapshot_seq),
        sim3.valid ? "true" : "false", sim3.status.c_str(),
        sim3.input_pair_count, sim3.pair_count, sim3.inlier_count,
        sim3.ba_to_arkit_scale, sim3.ba_to_arkit_translation_m[0],
        sim3.ba_to_arkit_translation_m[1],
        sim3.ba_to_arkit_translation_m[2],
        sim3.ba_to_arkit_quaternion_xyzw[0],
        sim3.ba_to_arkit_quaternion_xyzw[1],
        sim3.ba_to_arkit_quaternion_xyzw[2],
        sim3.ba_to_arkit_quaternion_xyzw[3],
        sim3.ba_to_arkit_rotation_deg, sim3.arkit_to_ba_scale,
        sim3.arkit_to_ba_translation_m[0],
        sim3.arkit_to_ba_translation_m[1],
        sim3.arkit_to_ba_translation_m[2],
        sim3.arkit_to_ba_quaternion_xyzw[0],
        sim3.arkit_to_ba_quaternion_xyzw[1],
        sim3.arkit_to_ba_quaternion_xyzw[2],
        sim3.arkit_to_ba_quaternion_xyzw[3],
        sim3.arkit_to_ba_rotation_deg, sim3.residual_p50_m,
        sim3.residual_p90_m, sim3.residual_max_m,
        sim3.residual_all_max_m, sim3_compute_ms);
    AppendMatchFailJsonl(s, sim3_line);

    const double point_start_ms = NowMs();
    aether::sfm::LiveCloudDiagPointMapV2 current_points;
    current_points.reserve(s->live_recon->NumPoints3D());
    for (const auto& [point_id, point] : s->live_recon->Points3D()) {
      current_points.emplace(
          static_cast<std::uint64_t>(point_id),
          std::array<double, 3>{point.xyz.x(), point.xyz.y(), point.xyz.z()});
    }

    const bool baseline = !s->live_cloud_diag_has_previous_points_v2;
    aether::sfm::LiveCloudSameIdPointDeltaSummaryV2 point_delta;
    if (baseline) {
      point_delta.current_count = current_points.size();
      point_delta.new_count = current_points.size();
    } else {
      point_delta = aether::sfm::SummarizeLiveCloudSameIdPointDeltasV2(
          s->live_cloud_diag_previous_points_v2, current_points);
    }
    const double point_compute_ms = NowMs() - point_start_ms;
    char point_line[2304];
    std::snprintf(
        point_line, sizeof(point_line),
        "{\"t\":%lld,\"type\":\"live_cloud_same_id_point_delta_v2\","
        "\"contract\":\"PW_LIVE_CLOUD_SNAPSHOT_DIAG_V2_STREAM\","
        "\"stage\":\"%s\",\"snapshot_seq\":%llu,"
        "\"previous_snapshot_seq\":%llu,\"baseline\":%s,"
        "\"valid\":%s,\"previous_count\":%zu,\"current_count\":%zu,"
        "\"common_count\":%zu,\"valid_delta_count\":%zu,"
        "\"new_count\":%zu,\"dropped_count\":%zu,"
        "\"median_dx_m\":%.12g,\"median_dy_m\":%.12g,"
        "\"median_dz_m\":%.12g,\"coherent_median_m\":%.12g,"
        "\"p50_m\":%.12g,\"p90_m\":%.12g,\"p99_m\":%.12g,"
        "\"max_m\":%.12g,\"gte_5cm_count\":%zu,"
        "\"gte_10cm_count\":%zu,\"worst_point_id\":\"%llu\","
        "\"compute_ms\":%.3f,\"observation_only\":true}",
        static_cast<long long>(EpochMs()), stage,
        static_cast<unsigned long long>(snapshot_seq),
        static_cast<unsigned long long>(
            baseline ? 0 : s->live_cloud_diag_previous_snapshot_seq_v2),
        baseline ? "true" : "false", point_delta.valid ? "true" : "false",
        point_delta.previous_count, point_delta.current_count,
        point_delta.common_count, point_delta.valid_delta_count,
        point_delta.new_count, point_delta.dropped_count,
        point_delta.median_delta_m[0], point_delta.median_delta_m[1],
        point_delta.median_delta_m[2], point_delta.coherent_median_norm_m,
        point_delta.p50_norm_m, point_delta.p90_norm_m,
        point_delta.p99_norm_m, point_delta.max_norm_m,
        point_delta.warning_5cm_count, point_delta.severe_10cm_count,
        static_cast<unsigned long long>(point_delta.worst_point_id),
        point_compute_ms);
    AppendMatchFailJsonl(s, point_line);

    s->live_cloud_diag_previous_points_v2 = std::move(current_points);
    s->live_cloud_diag_has_previous_points_v2 = true;
    s->live_cloud_diag_previous_snapshot_seq_v2 = snapshot_seq;
  } catch (...) {
                                                                                 
  }
}

struct LocalBundleTraceSnapshotV1 {
  int raw_calls = 0;
  int calls = 0;
  std::array<uint32_t, 16> query{};
  std::array<int, 16> neighbor_count{};
  std::array<int, 16> neighbor_total{};
  std::array<std::array<uint32_t, 15>, 16> neighbors{};
};

void ResetLocalBundleTraceV1() {
  aether_ilr_bundle_trace_calls = 0;
  std::memset(aether_ilr_bundle_trace_query, 0,
              sizeof(aether_ilr_bundle_trace_query));
  std::memset(aether_ilr_bundle_trace_neighbor_count, 0,
              sizeof(aether_ilr_bundle_trace_neighbor_count));
  std::memset(aether_ilr_bundle_trace_neighbor_total, 0,
              sizeof(aether_ilr_bundle_trace_neighbor_total));
  std::memset(aether_ilr_bundle_trace_neighbors, 0,
              sizeof(aether_ilr_bundle_trace_neighbors));
}

LocalBundleTraceSnapshotV1 CaptureLocalBundleTraceV1() {
  LocalBundleTraceSnapshotV1 snapshot;
  snapshot.raw_calls = std::max(aether_ilr_bundle_trace_calls, 0);
  snapshot.calls = std::min(snapshot.raw_calls, 16);
  for (int call = 0; call < snapshot.calls; ++call) {
    snapshot.query[call] = aether_ilr_bundle_trace_query[call];
    snapshot.neighbor_count[call] = std::min(
        std::max(aether_ilr_bundle_trace_neighbor_count[call], 0), 15);
    snapshot.neighbor_total[call] =
        std::max(aether_ilr_bundle_trace_neighbor_total[call], 0);
    for (int i = 0; i < snapshot.neighbor_count[call]; ++i) {
      snapshot.neighbors[call][i] =
          aether_ilr_bundle_trace_neighbors[call][i];
    }
  }
  return snapshot;
}

bool LocalBundleTraceExactV1(const LocalBundleTraceSnapshotV1& fresh,
                             const LocalBundleTraceSnapshotV1& mutable_cache) {
  if (fresh.raw_calls != mutable_cache.raw_calls ||
      fresh.calls != mutable_cache.calls) {
    return false;
  }
  for (int call = 0; call < fresh.calls; ++call) {
    if (fresh.query[call] != mutable_cache.query[call] ||
        fresh.neighbor_count[call] != mutable_cache.neighbor_count[call] ||
        fresh.neighbor_total[call] != mutable_cache.neighbor_total[call]) {
      return false;
    }
    for (int i = 0; i < fresh.neighbor_count[call]; ++i) {
      if (fresh.neighbors[call][i] != mutable_cache.neighbors[call][i]) {
        return false;
      }
    }
  }
  return true;
}

void AppendTailLocalBundleTraceV1(
    aether_sfm_session* s,
    int frame_id,
    int cache_reused,
    uint64_t cache_generation,
    const char* source,
    const LocalBundleTraceSnapshotV1& trace) {
  if (!s || !TailCacheTraceEnabled()) return;
  const char* mode =
      TailCacheMode() == TailCacheModeV1::kOn
          ? "on"
          : (TailCacheMode() == TailCacheModeV1::kShadow ? "shadow" : "off");
  for (int call = 0; call < trace.calls; ++call) {
    const int count = trace.neighbor_count[call];
    const int total = trace.neighbor_total[call];
    std::string line =
        "{\"t\":" + std::to_string(static_cast<long long>(EpochMs())) +
        ",\"type\":\"tail_local_bundle_v1\",\"fid\":" +
        std::to_string(frame_id) + ",\"cache\":\"" + mode +
        "\",\"source\":\"" + source +
        "\",\"cache_reused\":" + std::to_string(cache_reused) +
        ",\"tail_gen\":" + std::to_string(cache_generation) +
        ",\"call\":" + std::to_string(call) + ",\"query\":" +
        std::to_string(trace.query[call]) +
        ",\"neighbors\":[";
    for (int i = 0; i < count; ++i) {
      if (i != 0) line += ',';
      line += std::to_string(trace.neighbors[call][i]);
    }
    line += "],\"ba_images\":[" +
            std::to_string(trace.query[call]);
    for (int i = 0; i < count; ++i) {
      line += ',';
      line += std::to_string(trace.neighbors[call][i]);
    }
    line += "],\"neighbor_total\":" + std::to_string(total) +
            ",\"truncated\":" +
            (trace.raw_calls > trace.calls || total > count ? "true" : "false") +
            "}";
    AppendMatchFailJsonl(s, line);
  }
}

void AppendTailMatchCandidatesV1(aether_sfm_session* s,
                                 int frame_id,
                                 int configured_k,
                                 const std::vector<int>& candidates) {
  if (!s || !TailCacheTraceEnabled()) return;
  std::string line =
      "{\"t\":" + std::to_string(static_cast<long long>(EpochMs())) +
      ",\"type\":\"tail_match_candidates_v1\",\"fid\":" +
      std::to_string(frame_id) + ",\"configured_k\":" +
      std::to_string(configured_k) + ",\"match_candidate_ids\":[";
  bool first = true;
  for (const int index : candidates) {
    if (index < 0 || index >= static_cast<int>(s->frames.size())) continue;
    if (!first) line += ',';
    first = false;
    line += std::to_string(s->frames[static_cast<size_t>(index)].frame_id);
  }
  line += "],\"image_ids\":[";
  first = true;
  for (const int index : candidates) {
    if (index < 0 || index >= static_cast<int>(s->frames.size())) continue;
    if (!first) line += ',';
    first = false;
    line += std::to_string(s->frames[static_cast<size_t>(index)].image_id);
  }
  line += "]}";
  AppendMatchFailJsonl(s, line);
}

uint64_t NewGpuTimestampRunId() {
  static std::atomic<uint64_t> sequence{1};
  constexpr uint64_t kSequenceBits = 20;
  constexpr uint64_t kSequenceMask = (uint64_t{1} << kSequenceBits) - 1;
  const uint64_t epoch_ms =
      static_cast<uint64_t>(std::max<int64_t>(EpochMs(), 1));
  const uint64_t suffix =
      sequence.fetch_add(1, std::memory_order_relaxed) & kSequenceMask;
  return (epoch_ms << kSequenceBits) | suffix;
}

void AppendGpuTimestampProbeRecord(aether_sfm_session* s) {
  if (s == nullptr || s->gpu_timestamp_probe_written ||
      aether_sed_gpu_timestamp_probe_v1 == nullptr) {
    return;
  }
  AetherGpuTimestampProbeV1 probe{};
  probe.struct_size = sizeof(probe);
  if (aether_sed_gpu_timestamp_probe_v1(&probe) !=
      AETHER_GPU_TIMESTAMP_PULL_OK_V1) {
    return;
  }

  const aether::official::gpu_timestamp_writer_v1::RecordIdentityV1 identity{
      .run_id = s->gpu_timestamp_run_id,
      .frame_id = -1,
      .frame_ordinal = 0,
      .probe_record_id = s->gpu_timestamp_next_record_id,
  };
  std::string json;
  if (!aether::official::gpu_timestamp_writer_v1::SerializeProbeRecordV1(
          probe, identity, EpochMs(), &json)) {
    return;
  }
  AppendMatchFailJsonl(s, json);
  ++s->gpu_timestamp_next_record_id;
  s->gpu_timestamp_probe_instance_id = probe.probe_instance_id;
  s->gpu_timestamp_probe_written = true;
}

void PullGpuTimestampAfterExtraction(aether_sfm_session* s) {
  if (s == nullptr) return;
  s->gpu_timestamp_pending_frame_ready = false;
  ++s->gpu_timestamp_frame_ordinal;
  AppendGpuTimestampProbeRecord(s);
  if (!s->gpu_timestamp_probe_written ||
      aether_dsp_sift_take_last_gpu_timestamp_frame_v1 == nullptr) {
    return;
  }

  AetherGpuTimestampFrameV1 frame{};
  frame.struct_size = sizeof(frame);
  if (aether_dsp_sift_take_last_gpu_timestamp_frame_v1(&frame) !=
          AETHER_GPU_TIMESTAMP_PULL_OK_V1 ||
      frame.probe_instance_id != s->gpu_timestamp_probe_instance_id) {
    return;
  }
  s->gpu_timestamp_pending_frame = frame;
  s->gpu_timestamp_pending_frame_ordinal =
      s->gpu_timestamp_frame_ordinal;
  s->gpu_timestamp_pending_frame_ready = true;
}

void AppendGpuTimestampFrameRecord(aether_sfm_session* s, int frame_id) {
  if (s == nullptr || !s->gpu_timestamp_pending_frame_ready ||
      !s->gpu_timestamp_probe_written) {
    return;
  }
  const AetherGpuTimestampFrameV1 frame =
      s->gpu_timestamp_pending_frame;
  const uint64_t frame_ordinal =
      s->gpu_timestamp_pending_frame_ordinal;
  s->gpu_timestamp_pending_frame_ready = false;

  const aether::official::gpu_timestamp_writer_v1::RecordIdentityV1 identity{
      .run_id = s->gpu_timestamp_run_id,
      .frame_id = frame_id,
      .frame_ordinal = frame_ordinal,
      .probe_record_id = s->gpu_timestamp_next_record_id,
  };
  std::string json;
  if (!aether::official::gpu_timestamp_writer_v1::SerializeFrameRecordV1(
          frame, identity, EpochMs(), &json)) {
    return;
  }
  AppendMatchFailJsonl(s, json);
  ++s->gpu_timestamp_next_record_id;
}

std::string JsonSafe(const char* text, size_t max_len);

                                                                            
                                                                
                                                                          
                                                   
void AppendBaRingJsonl(aether_sfm_session* s, int stage) {
  try {
                                                                          
                                                                            
                                                                         
    const aether::official::ba::BaReceiptRingSnapshotV1 snapshot =
        s->ba_ptol_receipts.Drain();
    if (snapshot.count == 0 && snapshot.overwrite_count == 0) return;
    std::string line = "{\"t\":" + std::to_string(EpochMs()) +
                       ",\"type\":\"ba_rounds\",\"stage\":" +
                       std::to_string(stage) +
                       ",\"receipt_overwrite_count\":" +
                       std::to_string(snapshot.overwrite_count) +
                       ",\"solves\":[";
    char buf[768];
    for (std::size_t i = 0; i < snapshot.count; ++i) {
      const aether::official::ba::BaSolveReceiptV1& receipt =
          snapshot.receipts[i];
      const auto& ptol = receipt.ptol;
      const std::string safe_raw =
          JsonSafe(ptol.raw.data(), ptol.raw.size() - 1);
      std::snprintf(buf, sizeof(buf),
                    "%s{\"total_s\":%.2f,\"jac_s\":%.2f,\"lin_s\":%.2f,"
                    "\"res_s\":%.2f,\"pre_s\":%.2f,\"min_s\":%.2f,"
                    "\"post_s\":%.2f,\"solve_seq\":%llu,"
                    "\"scope\":\"%s\",\"ptol_raw\":%s%s%s,"
                    "\"ptol_raw_truncated\":%d,\"ptol_requested\":%.17g,"
                    "\"ptol_parse_status\":\"%s\",\"ptol_base\":%.17g,"
                    "\"ptol_effective\":%.17g,\"ptol_source\":\"%s\","
                    "\"ptol_fallback\":\"%s\",\"gpu_fallback\":%d,"
                    "\"iters\":%d,\"term\":%d,\"thr\":%d}",
                    i ? "," : "", receipt.total_s, receipt.jac_s,
                    receipt.lin_s, receipt.res_s, receipt.pre_s,
                    receipt.min_s, receipt.post_s,
                    static_cast<unsigned long long>(receipt.solve_seq),
                    aether::official::ba::SolveScopeNameV1(receipt.scope),
                    ptol.raw_present ? "\"" : "",
                    ptol.raw_present ? safe_raw.c_str() : "null",
                    ptol.raw_present ? "\"" : "",
                    ptol.raw_truncated ? 1 : 0, ptol.requested,
                    aether::official::ba::PtolParseStatusNameV1(
                        ptol.parse_status),
                    ptol.base, ptol.effective,
                    aether::official::ba::PtolSourceNameV1(ptol.source),
                    aether::official::ba::PtolFallbackNameV1(ptol),
                    receipt.gpu_fallback ? 1 : 0, receipt.iters,
                    receipt.term, receipt.threads);
      line += buf;
    }
    line += "]}";
    AppendMatchFailJsonl(s, line);
  } catch (...) {
                     
  }
}

void AppendBaSessionAggregateJsonl(aether_sfm_session* s,
                                   const char* reason) {
  try {
    const aether::official::ba::BaSessionAggregateSnapshotV1 snapshot =
        aether::official::ba::GetBaSessionAggregateSnapshotV1(
            &s->ba_ptol_aggregate);
    const uint64_t accounted = snapshot.scopes[0].solve_count +
                               snapshot.scopes[1].solve_count;
    std::string line =
        "{\"t\":" + std::to_string(EpochMs()) +
        ",\"type\":\"ba_ptol_session_v1\",\"reason\":\"" +
        JsonSafe(reason, 32) + "\",\"total_solve_count\":" +
        std::to_string(snapshot.total_solve_count) +
        ",\"accounted_solve_count\":" + std::to_string(accounted) +
        ",\"count_mismatch\":" +
        std::to_string(accounted == snapshot.total_solve_count ? 0 : 1) +
        ",\"overflow_semantics\":"
        "\"ring_overwrite_reset_discard_or_raw_truncate\"" +
        ",\"scopes\":[";
    char buf[768];
    for (std::size_t i = 0; i < snapshot.scopes.size(); ++i) {
      const auto& aggregate = snapshot.scopes[i];
      const auto& ptol = aggregate.last;
      const std::string raw = JsonSafe(ptol.raw.data(), ptol.raw.size() - 1);
      std::snprintf(
          buf, sizeof(buf),
          "%s{\"scope\":\"%s\",\"solve_count\":%llu,"
          "\"gpu_fallback_count\":%llu,\"raw\":%s%s%s,"
          "\"raw_truncated\":%d,\"parse\":\"%s\","
          "\"requested\":%.17g,\"base\":%.17g,\"effective\":%.17g,"
          "\"source\":\"%s\",\"fallback\":\"%s\","
          "\"mismatch\":%llu,\"overflow\":%llu}",
          i == 0 ? "" : ",",
          aether::official::ba::SolveScopeNameV1(aggregate.scope),
          static_cast<unsigned long long>(aggregate.solve_count),
          static_cast<unsigned long long>(aggregate.gpu_fallback_count),
          aggregate.has_last && ptol.raw_present ? "\"" : "",
          aggregate.has_last && ptol.raw_present ? raw.c_str() : "null",
          aggregate.has_last && ptol.raw_present ? "\"" : "",
          aggregate.has_last && ptol.raw_truncated ? 1 : 0,
          aether::official::ba::PtolParseStatusNameV1(ptol.parse_status),
          ptol.requested, ptol.base, ptol.effective,
          aether::official::ba::PtolSourceNameV1(ptol.source),
          aether::official::ba::PtolFallbackNameV1(ptol),
          static_cast<unsigned long long>(aggregate.mismatch),
          static_cast<unsigned long long>(aggregate.overflow));
      line += buf;
    }
    line += "]}";
    AppendMatchFailJsonl(s, line);
  } catch (...) {
                     
  }
}

                                                                             
                                                                             
std::string JsonSafe(const char* text, size_t max_len) {
  std::string out;
  if (!text) return out;
  out.reserve(max_len);
  for (size_t i = 0; text[i] != '\0' && i < max_len; ++i) {
    const char c = text[i];
    out.push_back((c == '"' || c == '\\' || (c >= 0 && c < 0x20)) ? ' ' : c);
  }
  return out;
}

void NoteGpuMatchFailure(aether_sfm_session* s, int rc, int prev_frame_id,
                         int new_frame_id) {
  ++s->stat_gpu_match_fail_total;
  const int bucket = (rc >= 1 && rc <= 7) ? rc : 0;
  ++s->stat_gpu_match_fail_by_rc[bucket];
                                                                           
                                                 
  if (rc == 7) s->gpu_pairs_since_rc7 = 0;
  ++s->gpu_match_fail_streak;
  if (s->gpu_match_fail_streak > s->stat_gpu_match_fail_max_streak) {
    s->stat_gpu_match_fail_max_streak = s->gpu_match_fail_streak;
  }
                                                                          
                                                                              
                                                                       
                                                                        
                                                                                   
  {
    char head[192];
    std::snprintf(head, sizeof(head),
                  "{\"t\":%lld,\"type\":\"gpu_match_fail\",\"rc\":%d,"
                  "\"pair\":[%d,%d],\"streak\":%lld,\"total\":%lld,"
                  "\"thermal\":%d",
                  static_cast<long long>(EpochMs()), rc, prev_frame_id,
                  new_frame_id, static_cast<long long>(s->gpu_match_fail_streak),
                  static_cast<long long>(s->stat_gpu_match_fail_total),
                  s->thermal_state.load(std::memory_order_relaxed));
    std::string line(head);
    if (rc == 7 && aether_gpu_match_last_error != nullptr) {
      char err[192] = {0};
      if (aether_gpu_match_last_error(err, sizeof(err)) > 0) {
        line += ",\"metal_err\":\"";
        line += JsonSafe(err, sizeof(err));
        line += "\"";
      }
    }
    line += "}";
    AppendMatchFailJsonl(s, line);
  }
  if (s->gpu_match_fail_streak == kGpuMatchFailStreakWarn ||
      (s->gpu_match_fail_streak > kGpuMatchFailStreakWarn &&
       s->gpu_match_fail_streak % 64 == 0)) {
    LOG(WARNING) << "[aether_sfm] GPU matcher failing in a segment: "
                 << s->gpu_match_fail_streak
                 << " consecutive pair failures (last rc=" << rc
                 << ", total=" << s->stat_gpu_match_fail_total
                 << "). Pairs are skipped fail-closed; finalize re-matches "
                    "starved frames.";
  }
}

                                                                            
                                                                           
                                                                        
                                                                          
                                                                         
                                                                              
                                                                             
                                                                           
                                                                       
                                                                         
                                                                
                                                                                      
                
int GpuMatchRetryLimit() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_GPU_MATCH_RETRY")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
    return 2;
  }();
  return cached;
}

                                                       
                                                                    
                                                                             
                                                                            
                                                                            
                                                                           
                                                                            
                                                                         
                                                                          
                                                                      
                                                                          
                                                                      
                                                                            
                                                                           
                                                             
                                                                         
                                                                          
                                                                              
                                                                         
                                                                              
                                                    
                                                                             
                                                                           
                                                                           
                                                                            
                                                                          
                                                                     
int ProbeGateMin() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_PROBE_GATE_MIN")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
                                                                                
                                              
      
                                                                         
                                                                              
                                                                        
                                                                         
                                                                                  
      
                                                                         
    // lossless too, because watching coverage grow shot by shot IS the product.
                                                                             
                                                                             
                                                                                   
                                                
      
                                                                               
                                                                               
                                                                            
                                                                                 
                                                                                  
                                                                       
                                                                                    
    return 0;
  }();
  return cached;
}
constexpr int kProbeGateRowsDefault = 512;                             
                                                                            
                                                                               
                                                                           
                                                                              
                                                                               
                                                                              
                                                                                
                                                                                 
                                                                                  
                                                             
int ProbeGateRows() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_PROBE_GATE_ROWS")) {
      const int v = std::atoi(e);
      if (v >= 32 && v <= 8192) return v;
    }
    return kProbeGateRowsDefault;
  }();
  return cached;
}
                                                                              
                                                                              
                                         
bool ProbeGateBatchEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_GATE_BATCH");
    return !(e && e[0] == '0');
  }();
  return cached;
}
                                                                           
                                                                            
                                                                            
                                                                            
                                         
bool ProbeGateTopRows() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_GATE_TOP");
    return !(e && e[0] == '0');
  }();
  return cached;
}

int GpuMatchGemmPairsRetry(aether_sfm_session* s, int frame1,
                           const uint8_t* d1, int n1, int frame2,
                           const uint8_t* d2, int n2, double max_ratio,
                           uint32_t* out_pairs, int max_pairs,
                           int* out_num_matches) {
  const auto match_once = [&] {
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
    if (s && s->descriptor_residency_nonce != 0 && frame1 >= 0 && frame2 >= 0 &&
        aether_gpu_match_gemm_pairs_resident != nullptr) {
      constexpr uint32_t kImmutableDescriptorGenerationV1 = 1;
      return aether_gpu_match_gemm_pairs_resident(
          s->descriptor_residency_nonce, static_cast<uint32_t>(frame1),
          kImmutableDescriptorGenerationV1, d1, n1,
          static_cast<uint32_t>(frame2), kImmutableDescriptorGenerationV1, d2,
          n2, max_ratio, out_pairs, max_pairs, out_num_matches);
    }
#endif
    return aether_gpu_match_gemm_pairs(d1, n1, d2, n2, max_ratio, out_pairs,
                                       max_pairs, out_num_matches);
  };
  int rc = match_once();
  for (int attempt = 0; rc == 7 && attempt < GpuMatchRetryLimit(); ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
    if (s) ++s->stat_gpu_retry_attempts;
    rc = match_once();
    if (rc == 0 && s) ++s->stat_gpu_retry_recovered;
  }
  return rc;
}

void AppendDescriptorResidencyStats(aether_sfm_session* s) {
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
  if (!s || s->descriptor_residency_nonce == 0 ||
      aether_gpu_match_descriptor_residency_stats == nullptr) {
    return;
  }
  uint64_t hits = 0;
  uint64_t misses = 0;
  uint64_t evictions = 0;
  uint64_t stale_replacements = 0;
  uint64_t upload_bytes = 0;
  uint64_t resident_bytes = 0;
  uint64_t resident_entries = 0;
  uint64_t allocation_failures = 0;
  uint64_t device_resets = 0;
  const int present = aether_gpu_match_descriptor_residency_stats(
      s->descriptor_residency_nonce, &hits, &misses, &evictions,
      &stale_replacements, &upload_bytes, &resident_bytes, &resident_entries,
      &allocation_failures, &device_resets);
  const int enabled = DescriptorResidencyEnabledCore() ? 1 : 0;
  char line[640];
  std::snprintf(
      line, sizeof(line),
      "{\"t\":%lld,\"type\":\"descriptor_residency_v1\",\"schema\":1,"
      "\"enabled\":%d,\"state_present\":%d,\"hits\":%llu,"
      "\"misses\":%llu,\"evictions\":%llu,\"stale_replacements\":%llu,"
      "\"upload_bytes\":%llu,\"resident_bytes\":%llu,"
      "\"resident_entries\":%llu,\"allocation_failures\":%llu,"
      "\"device_resets\":%llu}",
      static_cast<long long>(EpochMs()), enabled, present,
      static_cast<unsigned long long>(hits),
      static_cast<unsigned long long>(misses),
      static_cast<unsigned long long>(evictions),
      static_cast<unsigned long long>(stale_replacements),
      static_cast<unsigned long long>(upload_bytes),
      static_cast<unsigned long long>(resident_bytes),
      static_cast<unsigned long long>(resident_entries),
      static_cast<unsigned long long>(allocation_failures),
      static_cast<unsigned long long>(device_resets));
  AppendMatchFailJsonl(s, line);
#else
  (void)s;
#endif
}

void AppendTailCacheStats(aether_sfm_session* s) {
  if (!s || !TailCacheTraceEnabled()) return;
  const TailCacheModeV1 mode = TailCacheMode();
  std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
  char line[512];
  std::snprintf(
      line, sizeof(line),
      "{\"t\":%lld,\"type\":\"tail_cache_v1\",\"schema\":1,"
      "\"mode\":\"%s\",\"state\":%d,\"generation\":%llu,"
      "\"invalidated_generation\":%llu,\"rebuild_parent_generation\":%llu,"
      "\"dirty_reason\":\"%s\",\"images\":%zu,\"pairs\":%zu}",
      static_cast<long long>(EpochMs()),
      mode == TailCacheModeV1::kOn
          ? "on"
          : (mode == TailCacheModeV1::kShadow ? "shadow" : "off"),
      static_cast<int>(s->tail_cache_epoch.state()),
      static_cast<unsigned long long>(s->tail_cache_epoch.generation()),
      static_cast<unsigned long long>(
          s->tail_cache_epoch.invalidated_generation()),
      static_cast<unsigned long long>(
          s->tail_cache_epoch.rebuild_parent_generation()),
      TailCacheDirtyReasonName(s->tail_cache_epoch.dirty_reason()),
      s->tail_cache_epoch.image_count(), s->tail_cache_epoch.pair_count());
  AppendMatchFailJsonl(s, line);
}

                                                                           
                                                                           
int GpuMatchGuidedRetry(aether_sfm_session* s, const uint8_t* d1, int n1,
                        const float* xy1, const uint8_t* d2, int n2,
                        const float* xy2, double max_ratio,
                        const float* matrix12, const float* matrix21,
                        int guide_mode, float max_residual, uint32_t* out_pairs,
                        int max_pairs, int* out_num_matches) {
  int rc = aether_gpu_match_gemm_pairs_guided(
      d1, n1, xy1, d2, n2, xy2, max_ratio, matrix12, matrix21, guide_mode,
      max_residual, out_pairs, max_pairs, out_num_matches);
  for (int attempt = 0; rc == 7 && attempt < GpuMatchRetryLimit(); ++attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
    if (s) ++s->stat_gpu_retry_attempts;
    rc = aether_gpu_match_gemm_pairs_guided(
        d1, n1, xy1, d2, n2, xy2, max_ratio, matrix12, matrix21, guide_mode,
        max_residual, out_pairs, max_pairs, out_num_matches);
    if (rc == 0 && s) ++s->stat_gpu_retry_recovered;
  }
  return rc;
}

constexpr int kSpatialCandidatePool = 12;
constexpr int kSpatialAnchorsPerFrame = 3;
constexpr int kSpatialPreliminaryInliers = 15;
constexpr int kSpatialFinalInliers = 30;
constexpr double kSpatialPreliminaryMinInlierRatio = 0.10;
constexpr double kSpatialFinalMinInlierRatio = 0.25;
constexpr double kSpatialMatchRatio = 0.8;
constexpr double kGuidedMaxErrorPixels = 4.0;
                                                                                              
                                                                                             
                                                                                                 
                                                                                                         
                                                                                                              
                                                                                    
constexpr double kPoseDirectBootstrapErrorPixels = 128.0;
                                                                                                
inline double PoseDirectBandPixels() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_POSE_DIRECT_BAND_PX");
    if (!e || !*e) return kPoseDirectBootstrapErrorPixels;
    const double v = std::atof(e);
    return v > 0.0 ? v : kPoseDirectBootstrapErrorPixels;
  }();
  return cached;
}
                                                                    
constexpr double kPoseDirectMinBaselineMeters = 0.02;
constexpr double kSpatialFinalMaxErrorPixels = 1.0;
constexpr int kSpatialMaxTotalPairs = 1200;
                                                                              
                                                                             
                                                       
constexpr int kSpatialInitialAnchorBudget = 720;

                                                                              
                                                                          
                                                                      
                                                                               
                                                                                 
                                                                            
                                                                              
                                                                            
                                               
double EnvGateDouble(const char* name, double fallback) {
  const char* e = std::getenv(name);
  if (e && e[0]) {
    const double v = std::atof(e);
    if (v > 0.0) return v;
  }
  return fallback;
}

                                                                                           
                                                               
                                                                               
                                                                              
                                                                            
                                                                         
                                                                            
double LiveCreateTriMinAngleRad() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_LIVE_TRI_MIN_ANGLE");
    if (e && e[0]) {
      const double deg = std::atof(e);
      if (deg > 0.0) return deg * M_PI / 180.0;
    }
    return 0.03490658503988659;                                
  }();
  return cached;
}

                                                                                             
double LiveGrowMaxReprojPx() {
  static const double cached = EnvGateDouble("OFFICIAL_AETHER_GROW_REPROJ_PX", 14.0);
  return cached;
}

                                                                              
                                                                           
                                                                            
                                                                              
                                                                             
                                                                                 
                                                                             
                                                                                
                                                                    
double LiveCreateMaxReprojPx() {
  static const double cached =
      EnvGateDouble("OFFICIAL_AETHER_CREATE_REPROJ_PX", 10.0);
  return cached;
}

// ②c OFFICIAL_AETHER_OFFICIAL_TRIANGULATE — call upstream
                                                                      
                                                                           
                                                                              
                                                                              
                                                        
bool OfficialTriangulateImageEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_OFFICIAL_TRIANGULATE");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                                
// create/grow/merge and leave point authoring entirely to upstream
                                                                          
                                                                         
                                                                            
                                                                 
                                                                      
// upstream-triangulation arm.
bool SelfDevLiveTriangulationEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_SELFDEV_TRIANGULATE");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                          
                                                                                           
                                                                                          
                                                                                                 
                                                                                       
                                                                                                
                                                                                   
bool PairDraftEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PAIR_DRAFT");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                                 
// two-view feature tracks. Upstream default is true ("By default, COLMAP
                                                                          
                                                                      
                                                                              
// 2-view geometry that makes up most of the delta. Unset keeps upstream's
                                                   
                                                                                                      
                                                                                           
                                                                                                       
                                                                 
                                                                                                      
                                                                                                      
                                                                                                         
                                                                                                                      
                                                                           
                                                                  
bool TriIgnoreTwoViewTracks() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_TRI_IGNORE_2VIEW");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                              
                                                            
                                                                             
                                                                              
                                                                                  
                                                                   
                                                                                
                                                                                
                                                                                
                                                                                              
                                                                                    
                                                                                    
double BirthMinTriAngleDeg() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_BIRTH_MIN_TRI_ANGLE");
    if (e && e[0]) {
      const double d = std::atof(e);
      if (d > 0.0) return d;
    }
    return 0.0;                        
  }();
  return cached;
}
                                                             
                                                                                             
                                   
void ApplyBirthGateAB(colmap::IncrementalPipelineOptions* o) {
  const double b = BirthMinTriAngleDeg();
  if (b > 0.0) {
    o->triangulation.min_angle = b;                        
    o->mapper.filter_min_tri_angle = b;                            
  }
}

                                                                    
                                                                             
                                                                           
                                                                             
                                      
                                                                               
                                                                          
                                                                     
                                                                       
                                                                         
                                                                             
                                                                              
                                                                            
                                                                            
                                                                          
                                                                            
                                      
bool AsyncPreviewBaPropagate() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ASYNC_PREVIEW_BA_PROPAGATE");
    return !(e && e[0] == '0');
  }();
  return cached;
}
bool AsyncPreviewBaMatchCadence() {
  static const bool cached = [] {
    const char* e =
        std::getenv("OFFICIAL_AETHER_ASYNC_PREVIEW_BA_MATCH_CADENCE");
    return e && e[0] == '1';
  }();
  return cached;
}
                                                                              
                                                                         
                                                                            
                                                                        
                                                                            
                                                                          
                                                                        
                                                                            
                                                                         
                                                                          
                                                                        
                                                  
bool AsyncPreviewBaParamOnly() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ASYNC_PREVIEW_BA_PARAM_ONLY");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                                                  
                                                                             
                                                                             
                                                     
double TemporalDetailTriMinAngleRad() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_TD_TRI_ANGLE");
    if (e && e[0]) {
      const double deg = std::atof(e);
      if (deg > 0.0) return deg * M_PI / 180.0;
    }
    return 0.03490658503988659;                                
  }();
  return cached;
}
double TemporalDetailMaxReprojPx() {
  static const double cached = EnvGateDouble("OFFICIAL_AETHER_TD_REPROJ_PX", 3.0);
  return cached;
}

                                                                                      
                                                                              
                                                                             
                                                                             
                                                                               
                                                                       
                                                                               
                                                                          
                                                                                     
                                                                      
int EnrichPairCapOverride() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_ENRICH_PAIR_CAP")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;
  }();
  return cached;
}
int SpatialTotalPairBudget() {
  const int cap = EnrichPairCapOverride();
                                                                                          
                                                                          
                                                                              
                                                                            
                                                                         
                                                     
  return cap > 0 ? std::max(cap, kSpatialMaxTotalPairs) : kSpatialMaxTotalPairs;
}

                                                                             
                                                                            
                                                                               
                                                                             
                                                                             
bool TdGrowRefitEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_TD_GROW_REFIT");
    return e && e[0] == '1';
  }();
  return cached;
}
                                                                          
                                                         
bool TrackUpgradeEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_TRACK_UPGRADE");
    return e && e[0] == '1';
  }();
  return cached;
}
                                                                              
                                                                              
bool EnrichTargetedEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ENRICH_TARGETED");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                                        
                                                                                
                                                                      
                                                                           
                                                                              
                                                                           
                                                                      
                                                                              
                                                                 
bool GuidedTemporalEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_GUIDED_TEMPORAL");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                  
                                                                                               
                                                                                                 
                                                                                                         
                                                                                                
                                                                                                 
                                                                                                        
                                                                                            
                                                               
                                                                                              
                                                                                                      
                                                                                                           
                                                                                                        
                                                                                                            
                                                                                                             
                                                                                                                 
bool PoseDirectEEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_POSE_DIRECT_E");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                         
                                                                  
                                       
bool FragMergeEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_FRAG_MERGE");
    return e && e[0] == '1';
  }();
  return cached;
}
                                                                           
                                                                                     
                                                                             
                                                                        
                            
double FragMergeMaxReprojPx() {
  static const double cached =
      EnvGateDouble("OFFICIAL_AETHER_FRAG_MERGE_REPROJ_PX", 0.0);
  return cached;
}
                                                                                   
                                                                              
                                                                            
                                                                           
                                                                         
                                                                           
                                                                             
                                                                        
                                                                  
double FragMergeMinThetaDeg() {
  static const double cached =
      EnvGateDouble("OFFICIAL_AETHER_FRAG_MERGE_MIN_THETA_DEG", 0.0);
  return cached;
}
                                                                              
                                                                                       
                                                                             
                                                                            
                                                                   
double TdGrowRefitMinThetaDeg() {
  static const double cached =
      EnvGateDouble("OFFICIAL_AETHER_TD_GROW_REFIT_MIN_THETA_DEG", 0.0);
  return cached;
}

                                                                            
                                                                          
                                                                                
                                                                              
                                                                           
                                                                         
                                     
                                                                         
                                                                     
                                                                 
                                                                      
                                                                   
                                                                                        
                                         
int Stage1RoundsCap() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_STAGE1_ROUNDS_CAP")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;
  }();
  return cached;
}
double Stage1FtolOverride() {
  static const double cached = EnvGateDouble("OFFICIAL_AETHER_STAGE1_FTOL", 0.0);
  return cached;
}

                                                                        
                                                                      
                                                                          
                                                                    
                                                                         
                                                                           
                                                                           
                                              
bool Stage1HalfThreadsLegacy() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_STAGE1_HALF_THREADS");
    return e && e[0] == '1';
  }();
  return cached;
}
bool Stage1UtilityQosLegacy() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_STAGE1_UTILITY_QOS");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                           
// the quadratic enrichment pass. Provenance: upstream COLMAP's own matching
                                                          
                                                                           
                                                                   
                                                                            
                                                                        
                                                                
                                                                            
                                                                           
                                                                 
                                                                     
                                                                           
                                                                        
                                                                     
                                                                           
                           
                                                                             
                                                                              
                                                                           
  
                                                                             
                                                                                
                                                                         
                                                                           
  
                                                                             
                                                                             
                                                                               
                                                                          
                                                                        
                                                                           
                                                                        
                                                                            
  
                                                                              
                                                                                
                                                                               
                                                                
                                                                     
                                                                             
                                                              
                          
                                                                                  
                                                                       
                                                                                 
                                                                                
                                                                            
                                                                             
                                                                                
                                                                              
                                                                  
                                                                         
                                                                            
bool MatchTvgOverlapEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_MATCH_TVG_OVERLAP");
    return !(e && e[0] == '0');
  }();
  return cached;
}

bool QuadPipelineEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_QUAD_PIPELINE");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                                    
                                                                         
                                                                  
                                                                            
                                                                    
                                                                             
                                                                       
                                                                             
                                                                              
                                                                       
                                                                              
                                                                           
                                                                            
                                                                         
                                                                          
                                    
                                                                      
                                                                     
                                                                                
                                                                     
                                                                             
                                                                            
bool EpiPriorMatchEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_EPI_PRIOR_MATCH");
    return e && e[0] == '1';
  }();
  return cached;
}
double EpiBandBasePx() {
  static const double cached = EnvGateDouble("OFFICIAL_AETHER_EPI_BAND_BASE_PX", 30.0);
  return cached;
}
double EpiBandPerGapPx() {
  static const double cached =
      EnvGateDouble("OFFICIAL_AETHER_EPI_BAND_PER_GAP_PX", 5.0);
  return cached;
}
double EpiBandMaxPx() {
  static const double cached = EnvGateDouble("OFFICIAL_AETHER_EPI_BAND_MAX_PX", 150.0);
  return cached;
}
int EpiFallbackMinMatches() {
  static const int cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_EPI_FALLBACK_MIN");
    const int v = e ? std::atoi(e) : 150;
    return v > 0 ? v : 150;
  }();
  return cached;
}

                                                                         
                                                                                  
                                                                                 
                                                                             
                                                                                             
                                                                                          
                                                                                      
                                                                                         
                                                                                       
                                                                             
                                                                               
                                                              
                                                                               
                                                                                
                                                                                  
                                                                              
                                           
                                                                            
                                                            
bool StreamingLocalBaEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_LOCAL_BA");
    return !(e && e[0] == '0');                                              
  }();
  return cached;
}

int StreamingLocalBaPeriod(int session_value) {
  static const int env_period = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_BA_EVERY_N")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;                                                  
  }();
  if (env_period > 0) return env_period;
  return session_value > 0 ? session_value : 1;
}

bool IncrementalGlobalBaEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_INCREMENTAL_GLOBAL_BA");
    return e && e[0] == '1';
  }();
  return cached;
}
int IncrementalGlobalBaEveryN() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_INCREMENTAL_GLOBAL_BA_EVERY_N")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 25;                                                 
  }();
  return cached;
}
int IncrementalGlobalBaWindow() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_INCREMENTAL_GLOBAL_BA_WINDOW")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 40;                                                                
  }();
  return cached;
}
int IncrementalFinalizeRounds() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_INCREMENTAL_FINALIZE_ROUNDS")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 2;                                                                    
  }();
  return cached;
}

                                                                           
                                                                       
                                                                       
                                                                             
                                                                            
                                                                             
  
                                         
                                                                                
                                                                         
                                                                  
                                                                             
                                                                            
                                                                             
                                                                               
                                                                             
                                                              
                                                                            
                                                            
                                                               
  
                                                                             
                                                                              
                                                                             
                                                                           
                                             
enum class EnrichBudgetMode { kOff = 0, kAuto = 1, kFixed = 2 };
constexpr double kEnrichAutoFloorMs = 30000.0;

EnrichBudgetMode EnrichBudgetModeOf() {
  static const EnrichBudgetMode cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ENRICH_TIME_BUDGET_MS");
    if (!e || !e[0]) return EnrichBudgetMode::kAuto;
    const long v = std::atol(e);
    if (v > 0) return EnrichBudgetMode::kFixed;
    return EnrichBudgetMode::kOff;
  }();
  return cached;
}

double EnrichBudgetFixedMs() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ENRICH_TIME_BUDGET_MS");
    const long v = e && e[0] ? std::atol(e) : 0;
    return v > 0 ? static_cast<double>(v) : 0.0;
  }();
  return cached;
}

                                                                           
                                                                           
                                                                           
bool EnrichBudgetExhausted(aether_sfm_session* s) {
  if (!s || s->enrich_start_ms <= 0.0) return false;
  switch (EnrichBudgetModeOf()) {
    case EnrichBudgetMode::kOff:
      return false;
    case EnrichBudgetMode::kFixed:
      return NowMs() - s->enrich_start_ms > EnrichBudgetFixedMs();
    case EnrichBudgetMode::kAuto:
      return s->enrich_stage1_done.load(std::memory_order_relaxed) &&
             NowMs() - s->enrich_start_ms > kEnrichAutoFloorMs;
  }
  return false;
}

                                                                             
                                                                        
bool EnrichBudgetArmed(bool stage1_planned) {
  switch (EnrichBudgetModeOf()) {
    case EnrichBudgetMode::kOff:
      return false;
    case EnrichBudgetMode::kFixed:
      return true;
    case EnrichBudgetMode::kAuto:
      return stage1_planned;
  }
  return false;
}

enum class SpatialPairPurpose {
  kAnchor,
  kConfirm,
  kExpansion,
  kQuadraticAnchor,
  kQuadraticConfirm,
  kQuadraticExpansion,
};

struct VerifiedSpatialPair {
  bool valid = false;
  bool existing = false;
  bool written = false;
  int raw_matches = 0;
  int initial_inliers = 0;
  int final_inliers = 0;
  colmap::FeatureMatches matches;
  colmap::TwoViewGeometry geometry;
};

using SpatialPairCache = std::unordered_map<uint64_t, VerifiedSpatialPair>;

uint64_t FramePairKey(int frame_idx1, int frame_idx2) {
  const uint32_t hi = static_cast<uint32_t>(std::max(frame_idx1, frame_idx2));
  const uint32_t lo = static_cast<uint32_t>(std::min(frame_idx1, frame_idx2));
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

                                                                 
                                                                             
                                                               
                                                                               
                                                            
struct LiveGrowView {
  colmap::image_t image_id = 0;
  const std::vector<Eigen::Vector2d>* points = nullptr;                       
  const colmap::Camera* camera = nullptr;
  colmap::Rigid3d cam_from_world;
};

                                                                    
                                                                          
                                                                      
                                                                               
                                                                
                                                                  
                                                                     
                
  
                                                                              
                                                                           
                                                                            
                                                                       
                                                                         
                                                    
  
                                                                       
                                                                              
                                                                           
                                                                              
                                             
void GrowLiveTracksFromTvgInliers(
    aether_sfm_session* s, colmap::Reconstruction* recon,
    const LiveGrowView& v1, const LiveGrowView& v2,
    const colmap::FeatureMatches& inliers, double min_tri_angle_rad,
    double max_create_reproj_px, double max_grow_reproj_px,
    double max_merge_reproj_px, bool merge_require_disjoint_images,
    bool allow_merge, bool allow_grow,
    std::unordered_set<PointIdPair, PointIdPairHash>* merge_trials,
    std::unordered_set<colmap::point3D_t>* touched) {
  const Eigen::Matrix3x4d P_prev = v1.cam_from_world.ToMatrix();
  const Eigen::Matrix3x4d P_cur = v2.cam_from_world.ToMatrix();
  const Eigen::Vector3d c_prev = v1.cam_from_world.TgtOriginInSrc();
  const Eigen::Vector3d c_cur = v2.cam_from_world.TgtOriginInSrc();
  const int grow_n = static_cast<int>(inliers.size());
  for (int mm = 0; mm < grow_n; ++mm) {
    const uint32_t i1 = inliers[mm].point2D_idx1;
    const uint32_t i2 = inliers[mm].point2D_idx2;
    if (i1 >= v1.points->size() || i2 >= v2.points->size()) continue;
    const colmap::Point2D& o1 = recon->Image(v1.image_id).Point2D(i1);
    const colmap::Point2D& o2 = recon->Image(v2.image_id).Point2D(i2);
    const bool has1 = o1.HasPoint3D();
    const bool has2 = o2.HasPoint3D();

                                                                    
                                                                       
                                                                    
                                                                         
                                                                        
    if (has1 && has2) {
      if (o1.point3D_id == o2.point3D_id) ++s->stat_already_assigned;
      else {
        ++s->stat_merge_needed;
                                                                           
                                                                        
                                                                             
                                                                               
                                                                             
                                                                           
                                                                             
                                                                           
                                                                              
                                                                               
                                                            
        if (!allow_merge) continue;
        const PointIdPair key =
            CanonicalPointPair(o1.point3D_id, o2.point3D_id);
        if (!merge_trials->insert(key).second) continue;
        Eigen::Vector3d refit_xyz;
        const MergeReject verdict =
            CanMergeLivePoints(*recon, key.a, key.b, max_merge_reproj_px,
                               merge_require_disjoint_images, &refit_xyz);
        if (verdict == MergeReject::kNone) {
          const colmap::point3D_t merged = recon->MergePoints3D(key.a, key.b);
                                                                       
                                                                     
                                                                     
                                                                      
                                                              
          recon->Point3D(merged).xyz = refit_xyz;
          touched->erase(key.a);
          touched->erase(key.b);
          touched->insert(merged);
          ++s->stat_merge_accepted;
        } else {
          ++s->stat_merge_rejected;
          switch (verdict) {
            case MergeReject::kSharedImage:
              ++s->stat_merge_reject_shared_image;
              break;
            case MergeReject::kReproj:
              ++s->stat_merge_reject_reproj;
              break;
            default:
              ++s->stat_merge_reject_missing;
              break;
          }
        }
      }
      continue;
    }

                                                                          
                                                                          
                                                                    
                                                                           
                                                                       
                                                                         
                                                                      
                                                                            
                                                                        
                                                                           
                           
    if (has1 || has2) {
                                                                             
                                                                         
                                                                           
                                                                              
                                                                              
                                                                           
                                                                              
                                                          
      if (!allow_grow) continue;
      const colmap::point3D_t pid = has1 ? o1.point3D_id : o2.point3D_id;
      const colmap::image_t gimg = has1 ? v2.image_id : v1.image_id;
      const colmap::point2D_t gidx = has1 ? i2 : i1;
      const colmap::Rigid3d& gcfw =
          has1 ? v2.cam_from_world : v1.cam_from_world;
      const colmap::Camera& gcamera = has1 ? *v2.camera : *v1.camera;
      const Eigen::Vector2d& gkp = has1 ? (*v2.points)[i2] : (*v1.points)[i1];
      const Eigen::Vector3d Xg = gcfw * recon->Point3D(pid).xyz;
      if (Xg.z() <= 0.0) {
        ++s->stat_grow_rejected;
        ++s->stat_grow_reject_cheirality;
        continue;
      }
      const std::optional<Eigen::Vector2d> pg = gcamera.ImgFromCam(Xg);
      const double grow_gate = max_grow_reproj_px;
      if (!pg || (*pg - gkp).norm() > grow_gate) {
        ++s->stat_grow_rejected;
        ++s->stat_grow_reject_reproj;
        continue;
      }
      recon->AddObservation(pid, colmap::TrackElement(gimg, gidx));
      touched->insert(pid);
      ++s->stat_grow_accepted;
      continue;
    }

                                                                    
    const std::optional<Eigen::Vector2d> nn1 =
        v1.camera->CamFromImg((*v1.points)[i1]);
    const std::optional<Eigen::Vector2d> nn2 =
        v2.camera->CamFromImg((*v2.points)[i2]);
    if (!nn1 || !nn2) {
      ++s->stat_create_reject_reproj;
      continue;
    }
    Eigen::Vector3d X_world;
    if (!colmap::TriangulatePoint(P_prev, P_cur, *nn1, *nn2, &X_world)) {
      ++s->stat_create_reject_tri_angle;
      continue;
    }
    const Eigen::Vector3d X_prev = v1.cam_from_world * X_world;
    const Eigen::Vector3d X_cur = v2.cam_from_world * X_world;
    if (X_prev.z() <= 0.0 || X_cur.z() <= 0.0) {
      ++s->stat_create_reject_cheirality;
      continue;
    }
    if (colmap::CalculateTriangulationAngle(c_prev, c_cur, X_world) <
        min_tri_angle_rad) {
      ++s->stat_create_reject_tri_angle;
      continue;
    }
    const std::optional<Eigen::Vector2d> rr1 = v1.camera->ImgFromCam(X_prev);
    const std::optional<Eigen::Vector2d> rr2 = v2.camera->ImgFromCam(X_cur);
    if (!rr1 || !rr2) {
      ++s->stat_create_reject_reproj;
      continue;
    }
    if ((*rr1 - (*v1.points)[i1]).norm() > max_create_reproj_px ||
        (*rr2 - (*v2.points)[i2]).norm() > max_create_reproj_px) {
      ++s->stat_create_reject_reproj;
      continue;
    }

    colmap::Track track;
    track.AddElement(v1.image_id, i1);
    track.AddElement(v2.image_id, i2);
    const colmap::point3D_t pid =
        recon->AddPoint3D(X_world, std::move(track), Eigen::Vector3ub::Zero());
    touched->insert(pid);
  }
}

                                                                             
                                                                              
                                                                              
                                                                            
                                                                           
                                                                                  
                                                                                
                                                                 
int CountActiveComponentsV1(const aether_sfm_session& s) {
  const int n = static_cast<int>(s.frames.size());
  if (n <= 0) return 0;
  std::vector<int> parent(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) parent[static_cast<size_t>(i)] = i;
  const auto find = [&parent](int x) {
    while (parent[static_cast<size_t>(x)] != x) {
      parent[static_cast<size_t>(x)] =
          parent[static_cast<size_t>(parent[static_cast<size_t>(x)])];
      x = parent[static_cast<size_t>(x)];
    }
    return x;
  };
  for (const uint64_t key : s.live_pairs_done) {
    const int a = static_cast<int>(key >> 32);
    const int b = static_cast<int>(key & 0xFFFFFFFFULL);
    if (a < 0 || a >= n || b < 0 || b >= n) continue;
    const int ra = find(a);
    const int rb = find(b);
    if (ra != rb) parent[static_cast<size_t>(ra)] = rb;
  }
                                                                           
                                                                                
                                                                           
                                                                  
  std::unordered_set<int> roots;
  for (int i = 0; i < n; ++i) {
    const FrameRecord& frame = s.frames[static_cast<size_t>(i)];
    if (frame.image_id == 0 || frame.n_keypoints <= 0) continue;
    roots.insert(find(i));
  }
  return static_cast<int>(roots.size());
}

bool IsQuadraticPurpose(SpatialPairPurpose purpose) {
  return purpose == SpatialPairPurpose::kQuadraticAnchor ||
         purpose == SpatialPairPurpose::kQuadraticConfirm ||
         purpose == SpatialPairPurpose::kQuadraticExpansion;
}

bool IsExpansionPurpose(SpatialPairPurpose purpose) {
  return purpose == SpatialPairPurpose::kExpansion ||
         purpose == SpatialPairPurpose::kQuadraticExpansion;
}

void CopyMatrixRowMajor(const Eigen::Matrix3d& matrix,
                        std::array<float, 9>* out) {
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      (*out)[row * 3 + col] = static_cast<float>(matrix(row, col));
    }
  }
}

void CopyPixelPoints(const std::vector<Eigen::Vector2d>& points,
                     std::vector<float>* out) {
  out->resize(points.size() * 2);
  for (size_t i = 0; i < points.size(); ++i) {
    (*out)[2 * i] = static_cast<float>(points[i].x());
    (*out)[2 * i + 1] = static_cast<float>(points[i].y());
  }
}

void CopyNormalizedPoints(const colmap::Camera& camera,
                          const std::vector<Eigen::Vector2d>& points,
                          std::vector<float>* out) {
  out->resize(points.size() * 2);
  for (size_t i = 0; i < points.size(); ++i) {
    const std::optional<Eigen::Vector2d> normalized = camera.CamFromImg(points[i]);
    (*out)[2 * i] = normalized ? static_cast<float>(normalized->x()) : 1e6f;
    (*out)[2 * i + 1] =
        normalized ? static_cast<float>(normalized->y()) : 1e6f;
  }
}

void SetAllCameraIntrinsicsConstant(
    const colmap::Reconstruction& reconstruction,
    const std::vector<colmap::image_t>& image_ids,
    colmap::BundleAdjustmentConfig& config) {
  std::unordered_set<colmap::camera_t> camera_ids;
  camera_ids.reserve(image_ids.size());
  for (const colmap::image_t image_id : image_ids) {
    if (!reconstruction.ExistsImage(image_id)) continue;
    const colmap::Image& image = reconstruction.Image(image_id);
    if (camera_ids.insert(image.CameraId()).second) {
      config.SetConstantCamIntrinsics(image.CameraId());
    }
  }
}

bool PrepareGuidedGeometry(const aether_sfm_session* s,
                           const FrameRecord& a,
                           const FrameRecord& b,
                           const colmap::TwoViewGeometry& geometry,
                           std::vector<float>* xy_a,
                           std::vector<float>* xy_b,
                           std::array<float, 9>* matrix_ab,
                           std::array<float, 9>* matrix_ba,
                           int* guide_mode,
                           float* max_residual,
                                                                                           
                                                                                        
                           double max_error_pixels = kGuidedMaxErrorPixels) {
  if (!s || !xy_a || !xy_b || !matrix_ab || !matrix_ba || !guide_mode ||
      !max_residual) {
    return false;
  }

  const bool calibrated =
      (geometry.config == colmap::TwoViewGeometry::CALIBRATED ||
       geometry.config == colmap::TwoViewGeometry::CALIBRATED_RIG) &&
      geometry.E.has_value();
  const bool uncalibrated =
      geometry.config == colmap::TwoViewGeometry::UNCALIBRATED &&
      geometry.F.has_value();
  const bool homography =
      (geometry.config == colmap::TwoViewGeometry::PLANAR ||
       geometry.config == colmap::TwoViewGeometry::PANORAMIC ||
       geometry.config == colmap::TwoViewGeometry::PLANAR_OR_PANORAMIC) &&
      geometry.H.has_value();

  if (calibrated) {
    CopyNormalizedPoints(a.camera, a.points, xy_a);
    CopyNormalizedPoints(b.camera, b.points, xy_b);
    CopyMatrixRowMajor(*geometry.E, matrix_ab);
    CopyMatrixRowMajor(geometry.E->transpose(), matrix_ba);
                                                                             
                                                   
    const double normalized_error =
        0.5 * (a.camera.CamFromImgThreshold(max_error_pixels) +
               b.camera.CamFromImgThreshold(max_error_pixels));
    *max_residual = static_cast<float>(normalized_error * normalized_error);
    *guide_mode = 1;
    return true;
  }
  if (uncalibrated) {
    CopyPixelPoints(a.points, xy_a);
    CopyPixelPoints(b.points, xy_b);
    CopyMatrixRowMajor(*geometry.F, matrix_ab);
    CopyMatrixRowMajor(geometry.F->transpose(), matrix_ba);
    *max_residual = static_cast<float>(max_error_pixels * max_error_pixels);
    *guide_mode = 1;
    return true;
  }
  if (homography) {
    if (!geometry.H->allFinite() || std::abs(geometry.H->determinant()) < 1e-12) {
      return false;
    }
    const Eigen::Matrix3d inverse = geometry.H->inverse();
    if (!inverse.allFinite()) return false;
    CopyPixelPoints(a.points, xy_a);
    CopyPixelPoints(b.points, xy_b);
    CopyMatrixRowMajor(*geometry.H, matrix_ab);
    CopyMatrixRowMajor(inverse, matrix_ba);
    *max_residual = static_cast<float>(max_error_pixels * max_error_pixels);
    *guide_mode = 2;
    return true;
  }
  return false;
}

                                                                                      
                                                                                         
                                                                                                           
bool BuildPoseDirectGuideGeometry(const FrameRecord& a,
                                  const FrameRecord& b,
                                  colmap::TwoViewGeometry* out) {
  if (!out || !a.has_pose || !b.has_pose) return false;
  const colmap::Rigid3d b_from_a =
      b.cam_from_world * colmap::Inverse(a.cam_from_world);
                                                                                               
  if (!b_from_a.translation().allFinite()) return false;
                                                                                    
  if (b_from_a.translation().norm() < kPoseDirectMinBaselineMeters) return false;
  const Eigen::Matrix3d essential = colmap::EssentialMatrixFromPose(b_from_a);
  if (!essential.allFinite()) return false;
  out->config = colmap::TwoViewGeometry::CALIBRATED;
  out->E = essential;
  return true;
}

                                                                               
                                                           
                                                                                       
                                                                        
                                                                                
                         
bool RunGuidedMatch(aether_sfm_session* s,
                    const FrameRecord& a,
                    const FrameRecord& b,
                    const colmap::TwoViewGeometry& geometry,
                    colmap::FeatureMatches* guided_matches,
                    double max_error_pixels = kGuidedMaxErrorPixels) {
  if (!s || !guided_matches || !s->options.use_gpu_match ||
      aether_gpu_match_gemm_pairs_guided == nullptr) {
    return false;
  }

  std::vector<float> xy_a;
  std::vector<float> xy_b;
  std::array<float, 9> matrix_ab{};
  std::array<float, 9> matrix_ba{};
  int guide_mode = 0;
  float max_residual = 0.0f;
  if (!PrepareGuidedGeometry(s, a, b, geometry, &xy_a, &xy_b, &matrix_ab,
                             &matrix_ba, &guide_mode, &max_residual,
                             max_error_pixels)) {
    return false;
  }

  const int cap = std::min(a.n_keypoints, b.n_keypoints);
  if (cap < kSpatialFinalInliers) return false;
  std::vector<uint32_t> pair_buf(static_cast<size_t>(cap) * 2);
  int num_matches = 0;
                                                                           
                                            
  const int rc = GpuMatchGuidedRetry(
      s, a.descriptors.data(), a.n_keypoints, xy_a.data(),
      b.descriptors.data(), b.n_keypoints, xy_b.data(), kSpatialMatchRatio,
      matrix_ab.data(), matrix_ba.data(), guide_mode, max_residual,
      pair_buf.data(), cap, &num_matches);
  if (rc != 0) return false;

  guided_matches->resize(num_matches);
  for (int m = 0; m < num_matches; ++m) {
    (*guided_matches)[m].point2D_idx1 = pair_buf[2 * m];
    (*guided_matches)[m].point2D_idx2 = pair_buf[2 * m + 1];
  }
  return true;
}

                                                                            
                                                                        
                                                                    
                                                                        
                                                                              
                                   
bool ArkitGuidedMatchPair(aether_sfm_session* s, const FrameRecord& a,
                          const FrameRecord& b, int gap, double ratio,
                          uint32_t* pair_buf, int cap, int* num_matches) {
  if (!s || !pair_buf || !num_matches || cap <= 0) return false;
  if (!a.has_pose || !b.has_pose) return false;
  if (a.descriptors.empty() || b.descriptors.empty()) return false;
  if (!s->options.use_gpu_match ||
      aether_gpu_match_gemm_pairs_guided == nullptr) {
    return false;
  }
  colmap::TwoViewGeometry geo;
  geo.config = colmap::TwoViewGeometry::CALIBRATED;
  geo.E = colmap::EssentialMatrixFromPose(b.cam_from_world *
                                          colmap::Inverse(a.cam_from_world));
  std::vector<float> xy_a;
  std::vector<float> xy_b;
  std::array<float, 9> matrix_ab{};
  std::array<float, 9> matrix_ba{};
  int guide_mode = 0;
  float max_residual = 0.0f;
  if (!PrepareGuidedGeometry(s, a, b, geo, &xy_a, &xy_b, &matrix_ab,
                             &matrix_ba, &guide_mode, &max_residual)) {
    return false;
  }
                                                                       
                                                                         
  const double band_px =
      std::min(EpiBandMaxPx(), EpiBandBasePx() + EpiBandPerGapPx() * gap);
  const double normalized =
      0.5 * (a.camera.CamFromImgThreshold(band_px) +
             b.camera.CamFromImgThreshold(band_px));
  max_residual = static_cast<float>(normalized * normalized);
  ++s->stat_epi_attempted;
  const int rc = GpuMatchGuidedRetry(
      s, a.descriptors.data(), a.n_keypoints, xy_a.data(),
      b.descriptors.data(), b.n_keypoints, xy_b.data(), ratio,
      matrix_ab.data(), matrix_ba.data(), guide_mode, max_residual, pair_buf,
      cap, num_matches);
  if (rc != 0) {
    ++s->stat_epi_fallback;
    return false;
  }
  if (*num_matches < EpiFallbackMinMatches()) {
    ++s->stat_epi_fallback;
    *num_matches = 0;
    return false;
  }
  return true;
}

bool VerifySpatialPair(aether_sfm_session* s,
                       int frame_idx1,
                       int frame_idx2,
                       SpatialPairPurpose purpose,
                       SpatialPairCache* cache,
                       int* attempted_total) {
  if (!s || !s->db || !cache || !attempted_total || frame_idx1 == frame_idx2) {
    return false;
  }
  const int later = std::max(frame_idx1, frame_idx2);
  const int earlier = std::min(frame_idx1, frame_idx2);
  if (earlier < 0 || later >= static_cast<int>(s->frames.size())) return false;

  const uint64_t key = FramePairKey(later, earlier);
  const auto cached = cache->find(key);
  if (cached != cache->end()) return cached->second.valid;
  auto [it, inserted] = cache->try_emplace(key);
  (void)inserted;
  VerifiedSpatialPair& result = it->second;
  const FrameRecord& a = s->frames[earlier];
  const FrameRecord& b = s->frames[later];
  if (a.n_keypoints <= 0 || b.n_keypoints <= 0) return false;

  if (s->db->ExistsMatches(a.image_id, b.image_id)) {
    result.existing = true;
    if (!s->db->ExistsTwoViewGeometry(a.image_id, b.image_id)) return false;
    result.matches = s->db->ReadMatches(a.image_id, b.image_id);
    result.geometry = s->db->ReadTwoViewGeometry(a.image_id, b.image_id);
    result.raw_matches = static_cast<int>(result.matches.size());
    result.initial_inliers =
        static_cast<int>(result.geometry.inlier_matches.size());
    result.final_inliers = result.initial_inliers;
    result.valid = result.raw_matches >= kSpatialPreliminaryInliers &&
                   result.final_inliers >= kSpatialFinalInliers &&
                   static_cast<double>(result.initial_inliers) >=
                       kSpatialFinalMinInlierRatio * result.raw_matches;
    return result.valid;
  }

                                                                      
                                                                               
                                                                       
                                                                             
                                                                             
                                                                               
                                                                              
                                                                               
                                                                          
                                                                              
                                                                        
                                                               
  if (a.descriptors.empty() || b.descriptors.empty()) return false;

  if (*attempted_total >= SpatialTotalPairBudget()) {
    ++s->stat_spatial_budget_skipped;
    return false;
  }
                                                                        
                                                                             
                                                                               
  if (EnrichBudgetExhausted(s)) {
    ++s->stat_enrich_budget_stopped;
    return false;
  }
  ++*attempted_total;
  ++s->stat_spatial_pairs_attempted;
  if (purpose == SpatialPairPurpose::kAnchor ||
      purpose == SpatialPairPurpose::kQuadraticAnchor) {
    ++s->stat_spatial_anchor_attempted;
  }
  if (IsExpansionPurpose(purpose)) ++s->stat_spatial_expanded_attempted;
  if (IsQuadraticPurpose(purpose)) ++s->stat_spatial_quadratic_attempted;

  const int cap = std::min(a.n_keypoints, b.n_keypoints);
  if (cap < kSpatialPreliminaryInliers) return false;
  std::vector<uint32_t> pair_buf(static_cast<size_t>(cap) * 2);
  int num_matches = 0;
  int match_rc = 1;
  if (s->options.use_gpu_match) {
                                                                             
                                                                              
                                                                    
    if (aether_gpu_match_gemm_pairs == nullptr) return false;
    match_rc = GpuMatchGemmPairsRetry(
        s, a.frame_id, a.descriptors.data(), a.n_keypoints, b.frame_id,
        b.descriptors.data(), b.n_keypoints, kSpatialMatchRatio,
        pair_buf.data(), cap, &num_matches);
    if (match_rc != 0) return false;
  } else {
    match_rc = aether_sift_match_pairs(
        a.descriptors.data(), a.n_keypoints, b.descriptors.data(),
        b.n_keypoints, kSpatialMatchRatio, pair_buf.data(), cap, &num_matches);
  }
  if (match_rc != 0 || num_matches < kSpatialPreliminaryInliers) return false;

  colmap::FeatureMatches matches(num_matches);
  for (int m = 0; m < num_matches; ++m) {
    matches[m].point2D_idx1 = pair_buf[2 * m];
    matches[m].point2D_idx2 = pair_buf[2 * m + 1];
  }
  colmap::FeatureMatches matches_for_tvg = matches;
  const colmap::TwoViewGeometryOptions tvg_options;
  const auto initial_tvg = EstimateMandatoryFrameTwoViewGeometry(
      a, a.points, b, b.points, matches_for_tvg, tvg_options);
  if (!MandatoryGravityTvgPersistable(initial_tvg)) return false;
  colmap::TwoViewGeometry geometry = initial_tvg.geometry;
  const int initial_inliers =
      static_cast<int>(geometry.inlier_matches.size());
  if (initial_inliers < kSpatialPreliminaryInliers ||
      static_cast<double>(initial_inliers) <
          kSpatialPreliminaryMinInlierRatio * num_matches) {
    return false;
  }

                                                                        
                                                                              
                                                                             
                                                                             
                                                                              
                                       
  colmap::FeatureMatches guided_matches;
  if (!RunGuidedMatch(s, a, b, geometry, &guided_matches)) return false;
                                                                               
                                                                                 
  ++s->stat_spatial_guided_pairs;
  s->stat_spatial_guided_inliers += static_cast<int64_t>(guided_matches.size());
  colmap::TwoViewGeometryOptions final_tvg_options;
  final_tvg_options.ransac_options.max_error = kSpatialFinalMaxErrorPixels;
  final_tvg_options.min_num_inliers = kSpatialFinalInliers;
  final_tvg_options.min_inlier_ratio = kSpatialFinalMinInlierRatio;
  colmap::FeatureMatches guided_for_tvg = guided_matches;
  const auto final_tvg = EstimateMandatoryFrameTwoViewGeometry(
      a, a.points, b, b.points, guided_for_tvg, final_tvg_options);
  if (!MandatoryGravityTvgPersistable(final_tvg)) return false;
  colmap::TwoViewGeometry final_geometry = final_tvg.geometry;
  const int final_inliers =
      static_cast<int>(final_geometry.inlier_matches.size());
  if (final_inliers < kSpatialFinalInliers ||
      static_cast<double>(final_inliers) <
          kSpatialFinalMinInlierRatio * guided_matches.size()) {
    return false;
  }

  result.raw_matches = static_cast<int>(guided_matches.size());
  result.initial_inliers = initial_inliers;
  result.final_inliers = final_inliers;
  result.matches = std::move(guided_matches);
  result.geometry = std::move(final_geometry);
  result.valid = true;
  return true;
}

bool WriteVerifiedSpatialPair(aether_sfm_session* s,
                              int frame_idx1,
                              int frame_idx2,
                              bool quadratic,
                              SpatialPairCache* cache) {
  if (!s || !s->db || !cache || frame_idx1 == frame_idx2) return false;
  const int later = std::max(frame_idx1, frame_idx2);
  const int earlier = std::min(frame_idx1, frame_idx2);
  const auto it = cache->find(FramePairKey(later, earlier));
  if (it == cache->end() || !it->second.valid) return false;
  VerifiedSpatialPair& result = it->second;
  if (result.existing || result.written) return true;

  const FrameRecord& a = s->frames[earlier];
  const FrameRecord& b = s->frames[later];
  if (s->db->ExistsMatches(a.image_id, b.image_id) ||
      s->db->ExistsTwoViewGeometry(a.image_id, b.image_id)) {
    return false;
  }
  s->db->WriteMatches(a.image_id, b.image_id, result.matches);
  s->db->WriteTwoViewGeometry(a.image_id, b.image_id, result.geometry);
  result.written = true;
  ++s->stat_spatial_pairs_written;
  s->stat_spatial_inliers += result.final_inliers;
  if (quadratic) ++s->stat_spatial_quadratic_written;
  return true;
}

std::vector<SpatialRevisitCandidate> SelectDiverseAnchors(
    std::vector<SpatialRevisitCandidate> pool, int temporal_k) {
  std::vector<SpatialRevisitCandidate> selected;
  selected.reserve(kSpatialAnchorsPerFrame);
  const int min_separation = std::max(3, temporal_k / 3);
  for (const SpatialRevisitCandidate& candidate : pool) {
    bool separated = true;
    for (const SpatialRevisitCandidate& prior : selected) {
      if (std::abs(candidate.j - prior.j) < min_separation) {
        separated = false;
        break;
      }
    }
    if (separated) selected.push_back(candidate);
    if (static_cast<int>(selected.size()) == kSpatialAnchorsPerFrame) break;
  }
  for (const SpatialRevisitCandidate& candidate : pool) {
    if (static_cast<int>(selected.size()) == kSpatialAnchorsPerFrame) break;
    const bool duplicate = std::any_of(
        selected.begin(), selected.end(), [&](const SpatialRevisitCandidate& x) {
          return x.i == candidate.i && x.j == candidate.j;
        });
    if (!duplicate) selected.push_back(candidate);
  }
  for (int rank = 0; rank < static_cast<int>(selected.size()); ++rank) {
    selected[rank].anchor_rank = rank;
  }
  return selected;
}

std::vector<SpatialRevisitCandidate> BuildSpatialAnchors(
    aether_sfm_session* s, int temporal_k) {
  constexpr double kPrimaryDistanceMeters = 1.0;
  constexpr double kFallbackDistanceMeters = 1.5;
  constexpr double kPrimaryAngleRadians = 45.0 * M_PI / 180.0;
  constexpr double kFallbackAngleRadians = 60.0 * M_PI / 180.0;
  const double primary_min_dot = std::cos(kPrimaryAngleRadians);
  const double fallback_min_dot = std::cos(kFallbackAngleRadians);
  std::vector<SpatialRevisitCandidate> anchors;

  for (int i = 0; i < static_cast<int>(s->frames.size()); ++i) {
    const FrameRecord& current = s->frames[i];
    if (!current.has_pose) continue;
    const Eigen::Vector3d center = current.cam_from_world.TgtOriginInSrc();
    const Eigen::Vector3d forward = CameraForwardWorld(current.cam_from_world);
    std::vector<SpatialRevisitCandidate> primary;
    std::vector<SpatialRevisitCandidate> fallback;
    for (int j = 0; j + temporal_k < i; ++j) {
      const FrameRecord& previous = s->frames[j];
      if (!previous.has_pose) continue;
      const double distance =
          (center - previous.cam_from_world.TgtOriginInSrc()).norm();
      if (distance > kFallbackDistanceMeters) continue;
      const double dot = std::max(
          -1.0, std::min(1.0, forward.dot(CameraForwardWorld(previous.cam_from_world))));
      if (dot < fallback_min_dot) continue;
      const double angle = std::acos(dot);
      SpatialRevisitCandidate candidate;
      candidate.i = i;
      candidate.j = j;
      candidate.distance_m = distance;
      candidate.angle_rad = angle;
      if (distance <= kPrimaryDistanceMeters && dot >= primary_min_dot) {
        candidate.score = distance / kPrimaryDistanceMeters +
                          angle / kPrimaryAngleRadians;
        primary.push_back(candidate);
      } else {
        candidate.score = 2.0 + distance / kFallbackDistanceMeters +
                          angle / kFallbackAngleRadians;
        fallback.push_back(candidate);
      }
    }
    const auto quality_order = [](const SpatialRevisitCandidate& a,
                                  const SpatialRevisitCandidate& b) {
      if (a.score != b.score) return a.score < b.score;
      return std::abs(a.i - a.j) > std::abs(b.i - b.j);
    };
    std::sort(primary.begin(), primary.end(), quality_order);
    std::sort(fallback.begin(), fallback.end(), quality_order);
    std::vector<SpatialRevisitCandidate> pool;
    pool.reserve(kSpatialCandidatePool);
    for (const SpatialRevisitCandidate& candidate : primary) {
      if (static_cast<int>(pool.size()) == kSpatialCandidatePool) break;
      pool.push_back(candidate);
    }
    for (const SpatialRevisitCandidate& candidate : fallback) {
      if (static_cast<int>(pool.size()) == kSpatialCandidatePool) break;
      pool.push_back(candidate);
    }
    s->stat_spatial_pairs_considered += pool.size();
    std::vector<SpatialRevisitCandidate> selected =
        SelectDiverseAnchors(std::move(pool), temporal_k);
    anchors.insert(anchors.end(), selected.begin(), selected.end());
  }
  return anchors;
}

std::vector<SpatialRevisitCandidate> BuildQuadraticAnchors(
    const aether_sfm_session* s, int temporal_k) {
  std::vector<SpatialRevisitCandidate> anchors;
  for (int i = 0; i < static_cast<int>(s->frames.size()); ++i) {
    int rank = 0;
    for (int offset = 1; offset <= i; offset *= 2) {
      if (offset > temporal_k) {
        SpatialRevisitCandidate candidate;
        candidate.i = i;
        candidate.j = i - offset;
        candidate.score = static_cast<double>(rank);
        candidate.anchor_rank = rank;
        candidate.quadratic = true;
        anchors.push_back(candidate);
        if (++rank == kSpatialAnchorsPerFrame) break;
      }
      if (offset > i / 2) break;
    }
  }
  return anchors;
}

int ProcessRevisitAnchors(aether_sfm_session* s,
                          std::vector<SpatialRevisitCandidate> anchors,
                          bool quadratic,
                          int anchor_attempt_limit,
                          SpatialPairCache* cache,
                          int* attempted_total) {
  std::sort(anchors.begin(), anchors.end(),
            [](const SpatialRevisitCandidate& a,
               const SpatialRevisitCandidate& b) {
                                                                           
                                                                               
                                                                                
                                            
              if (a.anchor_rank != b.anchor_rank) {
                return a.anchor_rank < b.anchor_rank;
              }
              if (a.score != b.score) return a.score < b.score;
              if (a.i != b.i) return a.i < b.i;
              return a.j < b.j;
            });

  std::vector<SpatialRevisitCandidate> successful;
  size_t anchor_index = 0;
  for (; anchor_index < anchors.size(); ++anchor_index) {
    if (*attempted_total >= anchor_attempt_limit) break;
    const SpatialPairPurpose purpose =
        quadratic ? SpatialPairPurpose::kQuadraticAnchor
                  : SpatialPairPurpose::kAnchor;
    if (VerifySpatialPair(s, anchors[anchor_index].i, anchors[anchor_index].j,
                          purpose, cache, attempted_total)) {
      ++s->stat_spatial_anchor_passed;
      successful.push_back(anchors[anchor_index]);
    }
  }
  if (anchor_index < anchors.size()) {
    s->stat_spatial_budget_skipped += anchors.size() - anchor_index;
  }

  std::sort(successful.begin(), successful.end(),
            [&](const SpatialRevisitCandidate& a,
                const SpatialRevisitCandidate& b) {
              const int a_inliers = cache->at(FramePairKey(a.i, a.j)).final_inliers;
              const int b_inliers = cache->at(FramePairKey(b.i, b.j)).final_inliers;
              if (a_inliers != b_inliers) return a_inliers > b_inliers;
              return a.score < b.score;
            });

                                                                               
                                                                             
  std::vector<SpatialRevisitCandidate> seeds;
  for (const SpatialRevisitCandidate& candidate : successful) {
    const bool overlaps = std::any_of(
        seeds.begin(), seeds.end(), [&](const SpatialRevisitCandidate& seed) {
          return std::abs(candidate.i - seed.i) <= 4 &&
                 std::abs(candidate.j - seed.j) <= 4;
        });
    if (!overlaps) seeds.push_back(candidate);
  }

  int confirmed_regions = 0;
  for (const SpatialRevisitCandidate& seed : seeds) {
    int available = 0;
    int passed = 0;
    std::vector<std::pair<int, int>> support_pairs;
    for (int delta = -1; delta <= 1; ++delta) {
      const int i = seed.i + delta;
      const int j = seed.j + delta;
      if (i < 0 || j < 0 || i >= static_cast<int>(s->frames.size()) ||
          j >= static_cast<int>(s->frames.size()) || i == j) {
        continue;
      }
      ++available;
      const SpatialPairPurpose purpose =
          quadratic ? SpatialPairPurpose::kQuadraticConfirm
                    : SpatialPairPurpose::kConfirm;
      if (VerifySpatialPair(s, i, j, purpose, cache, attempted_total)) {
        ++passed;
        support_pairs.emplace_back(i, j);
      }
    }
    if (available < 2 || passed < 2) continue;

    ++confirmed_regions;
    ++s->stat_spatial_regions_confirmed;
    for (const auto& pair : support_pairs) {
      WriteVerifiedSpatialPair(s, pair.first, pair.second, quadratic, cache);
    }

                                                                               
                                                                              
                                                                             
    for (int di = -2; di <= 2; ++di) {
      for (int dj = -2; dj <= 2; ++dj) {
        const int i = seed.i + di;
        const int j = seed.j + dj;
        if (i < 0 || j < 0 || i >= static_cast<int>(s->frames.size()) ||
            j >= static_cast<int>(s->frames.size()) || i == j) {
          continue;
        }
        const SpatialPairPurpose purpose =
            quadratic ? SpatialPairPurpose::kQuadraticExpansion
                      : SpatialPairPurpose::kExpansion;
        if (VerifySpatialPair(s, i, j, purpose, cache, attempted_total)) {
          WriteVerifiedSpatialPair(s, i, j, quadratic, cache);
        }
      }
    }
  }
  return confirmed_regions;
}

                                                                                
                                                                              
                                                                               
                                                                          
                                                                                 
                                                                           
         
constexpr double kEnrichTargetLowParallaxRad = 3.0 * M_PI / 180.0;
constexpr double kEnrichTargetGainRad = 5.0 * M_PI / 180.0;

std::shared_ptr<const EnrichTargetHint> BuildEnrichTargetHint(
    const aether_sfm_session& s, const colmap::Reconstruction& recon) {
  auto hint = std::make_shared<EnrichTargetHint>();
  std::unordered_map<colmap::image_t, int> idx_of;
  idx_of.reserve(s.frames.size());
  for (int i = 0; i < static_cast<int>(s.frames.size()); ++i) {
    idx_of[s.frames[i].image_id] = i;
  }
  hint->by_frame.assign(s.frames.size(), {});
  std::vector<int> fidx;
  for (const auto& [pid, pt] : recon.Points3D()) {
    const auto& els = pt.track.Elements();
    if (els.size() < 2) continue;
    fidx.clear();
    for (const auto& el : els) {
      const auto it = idx_of.find(el.image_id);
      if (it == idx_of.end() || !s.frames[it->second].has_pose) continue;
      if (std::find(fidx.begin(), fidx.end(), it->second) == fidx.end()) {
        fidx.push_back(it->second);
      }
    }
    if (fidx.size() < 2) continue;
    double max_ang = 0.0;
    for (size_t a = 0; a < fidx.size(); ++a) {
      const Eigen::Vector3d ca =
          s.frames[fidx[a]].cam_from_world.TgtOriginInSrc();
      for (size_t b = a + 1; b < fidx.size(); ++b) {
        max_ang = std::max(
            max_ang,
            colmap::CalculateTriangulationAngle(
                ca, s.frames[fidx[b]].cam_from_world.TgtOriginInSrc(),
                pt.xyz));
      }
    }
    if (els.size() != 2 && max_ang >= kEnrichTargetLowParallaxRad) continue;
    const int32_t tid = static_cast<int32_t>(hint->tracks.size());
    hint->tracks.push_back({pt.xyz, fidx});
    for (const int f : fidx) hint->by_frame[f].push_back(tid);
  }
  return hint;
}

                                                                               
                                                                             
                                                                                  
                                                                                
                          
int ScoreEnrichPair(const aether_sfm_session& s, const EnrichTargetHint& hint,
                    int i, int j) {
  const auto sees = [&](const FrameRecord& fr, const Eigen::Vector3d& X) {
    const Eigen::Vector3d xc = fr.cam_from_world * X;
    if (xc.z() <= 0.0) return false;
    const std::optional<Eigen::Vector2d> px = fr.camera.ImgFromCam(xc);
    return px && px->x() >= 0.0 && px->y() >= 0.0 &&
           px->x() < static_cast<double>(fr.camera.width) &&
           px->y() < static_cast<double>(fr.camera.height);
  };
  const auto count_dir = [&](int obs_f, int new_f) {
    if (obs_f < 0 || obs_f >= static_cast<int>(hint.by_frame.size())) return 0;
    const FrameRecord& nf = s.frames[new_f];
    const Eigen::Vector3d cn = nf.cam_from_world.TgtOriginInSrc();
    int n = 0;
    for (const int32_t tid : hint.by_frame[obs_f]) {
      const EnrichTargetHint::LowTrack& t = hint.tracks[tid];
      if (std::find(t.frame_idxs.begin(), t.frame_idxs.end(), new_f) !=
          t.frame_idxs.end()) {
        continue;                                                            
      }
      if (!sees(nf, t.xyz)) continue;
      for (const int f : t.frame_idxs) {
        if (colmap::CalculateTriangulationAngle(
                cn, s.frames[f].cam_from_world.TgtOriginInSrc(), t.xyz) >=
            kEnrichTargetGainRad) {
          ++n;
          break;
        }
      }
    }
    return n;
  };
  return count_dir(i, j) + count_dir(j, i);
}

void AddSpatialRevisitMatches(aether_sfm_session* s) {
  if (kProductionOfficialEndpointOnly) return;
  if (!s || !s->db || s->frames.size() < 3 || s->camera_id == 0) return;
  const int temporal_k = s->options.k_neighbors > 0 ? s->options.k_neighbors : 12;
  SpatialPairCache cache;
  cache.reserve(kSpatialMaxTotalPairs * 2);
  int attempted_total = 0;

  const std::vector<SpatialRevisitCandidate> spatial_anchors =
      BuildSpatialAnchors(s, temporal_k);
  const int spatial_regions = ProcessRevisitAnchors(
      s, spatial_anchors, false,
      std::min(kSpatialInitialAnchorBudget, SpatialTotalPairBudget()), &cache,
      &attempted_total);

                                                                              
                                                                                
                                       
  if (spatial_regions == 0 && attempted_total < SpatialTotalPairBudget()) {
    const std::vector<SpatialRevisitCandidate> quadratic_anchors =
        BuildQuadraticAnchors(s, temporal_k);
    ProcessRevisitAnchors(s, quadratic_anchors, true, SpatialTotalPairBudget(),
                          &cache, &attempted_total);
  }

                                                             
                                                                                       
                                                                          
                                                                               
                                                                         
                                                                           
                                                                             
                                                                          
                                                                            
                                                                          
                                          
  const int enrich_cap = EnrichPairCapOverride();
  if (enrich_cap > 0) {
                                                                                         
                                                                           
                                                                        
                                                                              
                                                                             
                                                                             
                                                                           
    const std::shared_ptr<const EnrichTargetHint> hint =
        EnrichTargetedEnabled() ? s->enrich_hint : nullptr;
    if (hint) {
      s->stat_enrich_targeted_tracks =
          static_cast<int64_t>(hint->tracks.size());
    }
    std::vector<SpatialRevisitCandidate> topup;
    const int num_frames = static_cast<int>(s->frames.size());
    for (int i = 0; i < num_frames; ++i) {
      const FrameRecord& frame_i = s->frames[i];
      for (int j = 0; j + temporal_k < i; ++j) {
        const FrameRecord& frame_j = s->frames[j];
        SpatialRevisitCandidate candidate;
        candidate.i = i;
        candidate.j = j;
        if (frame_i.has_pose && frame_j.has_pose) {
          candidate.distance_m =
              (frame_i.cam_from_world.TgtOriginInSrc() -
               frame_j.cam_from_world.TgtOriginInSrc())
                  .norm();
          candidate.score = candidate.distance_m;
          if (hint) {
            candidate.target_score = ScoreEnrichPair(*s, *hint, i, j);
            if (candidate.target_score > 0) ++s->stat_enrich_targeted_scored;
          }
        } else {
                                                                     
          candidate.score = 1.0e9 + static_cast<double>(i - j);
        }
        topup.push_back(candidate);
      }
    }
    std::sort(topup.begin(), topup.end(),
              [](const SpatialRevisitCandidate& a,
                 const SpatialRevisitCandidate& b) {
                if (a.target_score != b.target_score) {
                  return a.target_score > b.target_score;                   
                }
                if (a.score != b.score) return a.score < b.score;
                if (a.i != b.i) return a.i < b.i;
                return a.j < b.j;
              });
    if (hint) {
      LOG(WARNING) << "[aether_sfm] enrich top-up targeted: low-parallax "
                      "tracks="
                   << s->stat_enrich_targeted_tracks
                   << " scored_pairs=" << s->stat_enrich_targeted_scored
                   << " of " << topup.size() << " beyond-K pairs";
    }
    for (const SpatialRevisitCandidate& candidate : topup) {
      if (attempted_total >= enrich_cap) break;
      if (VerifySpatialPair(s, candidate.i, candidate.j,
                            SpatialPairPurpose::kExpansion, &cache,
                            &attempted_total)) {
        WriteVerifiedSpatialPair(s, candidate.i, candidate.j, false, &cache);
      }
    }
  }
}

                                                                                
                                                                             
                                                                                 
void PolishPointGN(const std::vector<const colmap::Image*>& images,
                   const std::vector<const colmap::Camera*>& cameras,
                   const std::vector<Eigen::Vector2d>& obs_px,
                   Eigen::Vector3d* xyz);

                                                                                
                                                                                 
                                                                               
                                                                             
                                                                           
                                                                           
                                                                            
void RestoreTemporalDetail(aether_sfm_session* s,
                           colmap::Reconstruction* reconstruction) {
  if (kProductionOfficialEndpointOnly) return;
  if (!s || !reconstruction || s->frames.size() < 2) return;

                                                                                                
                                                                    
  const double kMinTriAngleRad = TemporalDetailTriMinAngleRad();
                                                                        
                                                                      
  const double kMaxReprojPx = TemporalDetailMaxReprojPx();
                                                                                        
                                                                                
                                                                           
                                                                          
                                                                               
                                                                             
                                                                           
                                                                          
                                                                          
                                                                              
                                                                         
                                                                  
                                           
  const bool kGrowRefit = TdGrowRefitEnabled();
  const int temporal_k =
      std::max(1, s->options.k_neighbors > 0 ? s->options.k_neighbors : 12);
  const int num_frames = static_cast<int>(s->frames.size());
  auto db = colmap::Database::Open(s->db_path);

  const auto reprojects_cleanly = [&](const colmap::Image& image,
                                      const colmap::Camera& camera,
                                      colmap::point2D_t point2D_idx,
                                      const Eigen::Vector3d& xyz) {
    const Eigen::Vector3d x_cam = image.CamFromWorld() * xyz;
    if (x_cam.z() <= 0.0) return false;
    const std::optional<Eigen::Vector2d> projected = camera.ImgFromCam(x_cam);
    return projected &&
           (*projected - image.Point2D(point2D_idx).xy).norm() <=
               kMaxReprojPx;
  };

  const auto has_image_in_track = [](const colmap::Track& track,
                                     colmap::image_t image_id) {
    return std::any_of(track.Elements().begin(), track.Elements().end(),
                       [image_id](const colmap::TrackElement& element) {
                         return element.image_id == image_id;
                       });
  };

  const auto has_stable_baseline = [&](const colmap::Track& track,
                                       const colmap::Image& candidate,
                                       const Eigen::Vector3d& xyz) {
    const Eigen::Vector3d candidate_center =
        candidate.CamFromWorld().TgtOriginInSrc();
    for (const colmap::TrackElement& element : track.Elements()) {
      if (!reconstruction->ExistsImage(element.image_id)) continue;
      const colmap::Image& observed = reconstruction->Image(element.image_id);
      if (!observed.HasPose()) continue;
      if (colmap::CalculateTriangulationAngle(
              candidate_center, observed.CamFromWorld().TgtOriginInSrc(), xyz) >=
          kMinTriAngleRad) {
        return true;
      }
    }
    return false;
  };

  for (int gap = std::min(temporal_k, num_frames - 1); gap >= 1; --gap) {
    for (int right = gap; right < num_frames; ++right) {
      const FrameRecord& frame1 = s->frames[right - gap];
      const FrameRecord& frame2 = s->frames[right];
      if (!reconstruction->ExistsImage(frame1.image_id) ||
          !reconstruction->ExistsImage(frame2.image_id)) {
        continue;
      }
      colmap::Image& image1 = reconstruction->Image(frame1.image_id);
      colmap::Image& image2 = reconstruction->Image(frame2.image_id);
      if (!image1.HasPose() || !image2.HasPose() ||
          !db->ExistsTwoViewGeometry(frame1.image_id, frame2.image_id)) {
        continue;
      }

      const colmap::TwoViewGeometry geometry =
          db->ReadTwoViewGeometry(frame1.image_id, frame2.image_id);
      if (geometry.inlier_matches.empty()) continue;
      ++s->stat_temporal_detail_pairs;
      s->stat_temporal_detail_matches += geometry.inlier_matches.size();

      const colmap::Camera& camera1 =
          reconstruction->Camera(image1.CameraId());
      const colmap::Camera& camera2 =
          reconstruction->Camera(image2.CameraId());
      for (const colmap::FeatureMatch& match : geometry.inlier_matches) {
        if (match.point2D_idx1 >= image1.NumPoints2D() ||
            match.point2D_idx2 >= image2.NumPoints2D()) {
          ++s->stat_temporal_detail_conflicts;
          continue;
        }

        const colmap::Point2D& point1 =
            image1.Point2D(match.point2D_idx1);
        const colmap::Point2D& point2 =
            image2.Point2D(match.point2D_idx2);
        const bool has1 = point1.HasPoint3D();
        const bool has2 = point2.HasPoint3D();

        if (has1 && has2) {
          if (point1.point3D_id != point2.point3D_id) {
            ++s->stat_temporal_detail_conflicts;
          }
          continue;
        }

        if (has1 || has2) {
          const colmap::point3D_t point3D_id =
              has1 ? point1.point3D_id : point2.point3D_id;
          const colmap::image_t grow_image_id =
              has1 ? frame2.image_id : frame1.image_id;
          const colmap::point2D_t grow_point2D_idx =
              has1 ? match.point2D_idx2 : match.point2D_idx1;
          colmap::Image& grow_image = has1 ? image2 : image1;
          const colmap::Camera& grow_camera = has1 ? camera2 : camera1;
          const colmap::Point3D& point3D =
              reconstruction->Point3D(point3D_id);
          if (has_image_in_track(point3D.track, grow_image_id)) {
            ++s->stat_temporal_detail_conflicts;
            continue;
          }
                                                                              
          if (kGrowRefit) {
            ++s->stat_td_grow_refit_attempted;
            // Legacy-gate verdict, attribution only: how many accepts the
                                                            
            const Eigen::Vector3d x_cam_cur =
                grow_image.CamFromWorld() * point3D.xyz;
            const bool legacy_ok =
                x_cam_cur.z() > 0.0 &&
                reprojects_cleanly(grow_image, grow_camera, grow_point2D_idx,
                                   point3D.xyz) &&
                has_stable_baseline(point3D.track, grow_image, point3D.xyz);
            bool usable = true;
            std::vector<Eigen::Matrix3x4d> cams_from_world;
            std::vector<Eigen::Vector2d> cam_points;
            std::vector<const colmap::Image*> union_imgs;
            std::vector<const colmap::Camera*> union_cams;
            std::vector<Eigen::Vector2d> union_px;
            cams_from_world.reserve(point3D.track.Length() + 1);
            cam_points.reserve(point3D.track.Length() + 1);
            for (const colmap::TrackElement& el : point3D.track.Elements()) {
              if (!reconstruction->ExistsImage(el.image_id)) {
                usable = false;
                break;
              }
              const colmap::Image& im = reconstruction->Image(el.image_id);
              if (!im.HasPose() || el.point2D_idx >= im.NumPoints2D()) {
                usable = false;
                break;
              }
              const colmap::Camera& cm =
                  reconstruction->Camera(im.CameraId());
              const std::optional<Eigen::Vector2d> nc =
                  cm.CamFromImg(im.Point2D(el.point2D_idx).xy);
              if (!nc) {
                usable = false;
                break;
              }
              cams_from_world.push_back(im.CamFromWorld().ToMatrix());
              cam_points.push_back(*nc);
              union_imgs.push_back(&im);
              union_cams.push_back(&cm);
              union_px.push_back(im.Point2D(el.point2D_idx).xy);
            }
            const std::optional<Eigen::Vector2d> nc_cand =
                grow_camera.CamFromImg(
                    grow_image.Point2D(grow_point2D_idx).xy);
            if (usable && nc_cand) {
              cams_from_world.push_back(grow_image.CamFromWorld().ToMatrix());
              cam_points.push_back(*nc_cand);
              union_imgs.push_back(&grow_image);
              union_cams.push_back(&grow_camera);
              union_px.push_back(grow_image.Point2D(grow_point2D_idx).xy);
              Eigen::Vector3d refit_xyz;
              if (colmap::TriangulateMultiViewPoint(
                      colmap::span<const Eigen::Matrix3x4d>(
                          cams_from_world.data(), cams_from_world.size()),
                      colmap::span<const Eigen::Vector2d>(cam_points.data(),
                                                          cam_points.size()),
                      &refit_xyz)) {
                                                                       
                                                                     
                                                                         
                                                                             
                                                           
                PolishPointGN(union_imgs, union_cams, union_px, &refit_xyz);
                bool refit_ok = reprojects_cleanly(
                    grow_image, grow_camera, grow_point2D_idx, refit_xyz);
                if (refit_ok) {
                  for (const colmap::TrackElement& el :
                       point3D.track.Elements()) {
                    const colmap::Image& im =
                        reconstruction->Image(el.image_id);
                    if (!reprojects_cleanly(
                            im, reconstruction->Camera(im.CameraId()),
                            el.point2D_idx, refit_xyz)) {
                      refit_ok = false;
                      break;
                    }
                  }
                }
                                                                           
                                                                            
                                                                                   
                                                                         
                                                                          
                                                                        
                                                                            
                static const double kGrowMinThetaRad =
                    TdGrowRefitMinThetaDeg() * M_PI / 180.0;
                if (refit_ok && kGrowMinThetaRad > 0.0) {
                  double th_union = 0.0;
                  for (size_t i2 = 0; i2 < union_imgs.size(); ++i2) {
                    const Eigen::Vector3d ci =
                        union_imgs[i2]->CamFromWorld().TgtOriginInSrc();
                    for (size_t j2 = i2 + 1; j2 < union_imgs.size(); ++j2) {
                      th_union = std::max(
                          th_union,
                          colmap::CalculateTriangulationAngle(
                              ci,
                              union_imgs[j2]->CamFromWorld().TgtOriginInSrc(),
                              refit_xyz));
                    }
                  }
                  if (th_union < kGrowMinThetaRad) {
                    ++s->stat_td_grow_refit_min_theta_rej;
                    refit_ok = false;
                  }
                }
                if (refit_ok &&
                    has_stable_baseline(point3D.track, grow_image,
                                        refit_xyz)) {
                  reconstruction->Point3D(point3D_id).xyz = refit_xyz;
                  reconstruction->AddObservation(
                      point3D_id,
                      colmap::TrackElement(grow_image_id, grow_point2D_idx));
                  ++s->stat_temporal_detail_grown;
                  ++s->stat_td_grow_refit_accepted;
                  if (!legacy_ok) ++s->stat_td_grow_refit_saved;
                  continue;
                }
              }
            }
                                                                           
                                                                     
          }
          const Eigen::Vector3d x_cam =
              grow_image.CamFromWorld() * point3D.xyz;
          if (x_cam.z() <= 0.0) {
            ++s->stat_temporal_detail_reject_cheirality;
            continue;
          }
          if (!reprojects_cleanly(grow_image, grow_camera,
                                  grow_point2D_idx, point3D.xyz)) {
            ++s->stat_temporal_detail_reject_reproj;
            continue;
          }
          if (!has_stable_baseline(point3D.track, grow_image, point3D.xyz)) {
            ++s->stat_temporal_detail_reject_tri_angle;
            continue;
          }
          reconstruction->AddObservation(
              point3D_id,
              colmap::TrackElement(grow_image_id, grow_point2D_idx));
          ++s->stat_temporal_detail_grown;
          continue;
        }

        const std::optional<Eigen::Vector2d> cam_point1 =
            camera1.CamFromImg(point1.xy);
        const std::optional<Eigen::Vector2d> cam_point2 =
            camera2.CamFromImg(point2.xy);
        if (!cam_point1 || !cam_point2) {
          ++s->stat_temporal_detail_reject_reproj;
          continue;
        }
        Eigen::Vector3d xyz;
        if (!colmap::TriangulatePoint(image1.CamFromWorld().ToMatrix(),
                                     image2.CamFromWorld().ToMatrix(),
                                     *cam_point1, *cam_point2, &xyz)) {
          ++s->stat_temporal_detail_reject_tri_angle;
          continue;
        }
        const Eigen::Vector3d x_cam1 = image1.CamFromWorld() * xyz;
        const Eigen::Vector3d x_cam2 = image2.CamFromWorld() * xyz;
        if (x_cam1.z() <= 0.0 || x_cam2.z() <= 0.0) {
          ++s->stat_temporal_detail_reject_cheirality;
          continue;
        }
        if (colmap::CalculateTriangulationAngle(
                image1.CamFromWorld().TgtOriginInSrc(),
                image2.CamFromWorld().TgtOriginInSrc(), xyz) <
            kMinTriAngleRad) {
          ++s->stat_temporal_detail_reject_tri_angle;
          continue;
        }
        if (!reprojects_cleanly(image1, camera1, match.point2D_idx1, xyz) ||
            !reprojects_cleanly(image2, camera2, match.point2D_idx2, xyz)) {
          ++s->stat_temporal_detail_reject_reproj;
          continue;
        }

        colmap::Track track;
        track.AddElement(frame1.image_id, match.point2D_idx1);
        track.AddElement(frame2.image_id, match.point2D_idx2);
        reconstruction->AddPoint3D(xyz, std::move(track),
                                   Eigen::Vector3ub::Zero());
        ++s->stat_temporal_detail_created;
      }
    }
  }
  db->Close();
}

                                                                               
                                                                               
                                                                               
                                                                            
                                                                                   
                                                                             
                                                                             
                                
void PolishPointGN(const std::vector<const colmap::Image*>& images,
                   const std::vector<const colmap::Camera*>& cameras,
                   const std::vector<Eigen::Vector2d>& obs_px,
                   Eigen::Vector3d* xyz) {
  for (int iter = 0; iter < 4; ++iter) {
    Eigen::Matrix3d H = Eigen::Matrix3d::Zero();
    Eigen::Vector3d b = Eigen::Vector3d::Zero();
    int n_used = 0;
    for (size_t i = 0; i < images.size(); ++i) {
      const colmap::Camera& cam = *cameras[i];
      if (cam.model_id != colmap::SimplePinholeCameraModel::model_id &&
          cam.model_id != colmap::PinholeCameraModel::model_id) {
        return;
      }
      const colmap::Rigid3d& cfw = images[i]->CamFromWorld();
      const Eigen::Vector3d xc = cfw * (*xyz);
      if (xc.z() <= 1e-9) return;
      const std::optional<Eigen::Vector2d> px = cam.ImgFromCam(xc);
      if (!px) return;
      const double fx = cam.FocalLengthX();
      const double fy = cam.FocalLengthY();
      const double iz = 1.0 / xc.z();
      const Eigen::Vector2d r = *px - obs_px[i];
      Eigen::Matrix<double, 2, 3> jc;
      jc << fx * iz, 0.0, -fx * xc.x() * iz * iz,    
          0.0, fy * iz, -fy * xc.y() * iz * iz;
      const Eigen::Matrix<double, 2, 3> J =
          jc * cfw.rotation().toRotationMatrix();
      H += J.transpose() * J;
      b += J.transpose() * r;
      ++n_used;
    }
    if (n_used < 2) return;
    H.diagonal().array() += 1e-8 * H.trace() + 1e-12;
    const Eigen::Vector3d dx = H.ldlt().solve(b);
    if (!dx.allFinite()) return;
    *xyz -= dx;
  }
}

                                                                              
                                                                                      
                                                                         
  
                                                                          
                                                                          
                                                                              
                                                                            
                                                                                
                                                                         
                                                                          
                                                                                
                                                                             
                                                                         
                                            
                                                                             
                                                                            
                                                   
                                                                             
                                                                            
                                                                           
                                                                              
                    
                                                                             
                                                                                   
                                                                     
void UpgradeLowParallaxTracks(aether_sfm_session* s,
                              colmap::Reconstruction* reconstruction) {
  if (kProductionOfficialEndpointOnly) return;
  if (!TrackUpgradeEnabled() || !s || !reconstruction) return;
  const double t0 = NowMs();
  try {
    const double kMaxReprojPx = TemporalDetailMaxReprojPx();
    constexpr double kLowParallaxRad = 3.0 * M_PI / 180.0;

                                                                          
    const auto theta_max = [&](const colmap::Track& track,
                               const Eigen::Vector3d& xyz) {
      double best = 0.0;
      const auto& els = track.Elements();
      for (size_t a = 0; a < els.size(); ++a) {
        if (!reconstruction->ExistsImage(els[a].image_id)) continue;
        const colmap::Image& ia = reconstruction->Image(els[a].image_id);
        if (!ia.HasPose()) continue;
        const Eigen::Vector3d ca = ia.CamFromWorld().TgtOriginInSrc();
        for (size_t b = a + 1; b < els.size(); ++b) {
          if (!reconstruction->ExistsImage(els[b].image_id)) continue;
          const colmap::Image& ib = reconstruction->Image(els[b].image_id);
          if (!ib.HasPose()) continue;
          best = std::max(best,
                          colmap::CalculateTriangulationAngle(
                              ca, ib.CamFromWorld().TgtOriginInSrc(), xyz));
        }
      }
      return best;
    };
                                                                                   
                                                       
    const auto summarize = [&](int64_t* n_out, int64_t* n2_out,
                               int64_t* lt3_out, double* p50_out) {
      std::vector<double> thetas;
      thetas.reserve(reconstruction->NumPoints3D());
      int64_t n2 = 0, lt3 = 0;
      for (const auto& [pid, pt] : reconstruction->Points3D()) {
        const double th = theta_max(pt.track, pt.xyz);
        thetas.push_back(th);
        if (pt.track.Length() == 2) ++n2;
        if (th < kLowParallaxRad) ++lt3;
      }
      *n_out = static_cast<int64_t>(thetas.size());
      *n2_out = n2;
      *lt3_out = lt3;
      if (!thetas.empty()) {
        std::nth_element(thetas.begin(), thetas.begin() + thetas.size() / 2,
                         thetas.end());
        *p50_out = thetas[thetas.size() / 2] * 180.0 / M_PI;
      }
    };

                                                                            
                                    
    std::vector<colmap::point3D_t> eligible;
    {
      std::vector<double> thetas;
      thetas.reserve(reconstruction->NumPoints3D());
      int64_t n2 = 0, lt3 = 0;
      for (const auto& [pid, pt] : reconstruction->Points3D()) {
        const double th = theta_max(pt.track, pt.xyz);
        thetas.push_back(th);
        const bool two_view = pt.track.Length() == 2;
        if (two_view) ++n2;
        if (th < kLowParallaxRad) ++lt3;
        if (two_view || th < kLowParallaxRad) eligible.push_back(pid);
      }
      s->stat_upgrade_pre_npts = static_cast<int64_t>(thetas.size());
      s->stat_upgrade_pre_2view = n2;
      s->stat_upgrade_pre_lt3 = lt3;
      if (!thetas.empty()) {
        std::nth_element(thetas.begin(), thetas.begin() + thetas.size() / 2,
                         thetas.end());
        s->upgrade_theta_p50_pre_deg =
            thetas[thetas.size() / 2] * 180.0 / M_PI;
      }
    }
    s->stat_upgrade_eligible = static_cast<int64_t>(eligible.size());
    if (eligible.empty()) {
      s->upgrade_ms = NowMs() - t0;
      return;
    }

                                                                            
                                                                                
                                                    
    const auto key_of = [](colmap::image_t img, uint32_t idx) {
      return (static_cast<uint64_t>(img) << 32) | idx;
    };
    std::unordered_set<uint64_t> needed;
    needed.reserve(eligible.size() * 3);
    for (const colmap::point3D_t pid : eligible) {
      for (const colmap::TrackElement& el :
           reconstruction->Point3D(pid).track.Elements()) {
        needed.insert(key_of(el.image_id, el.point2D_idx));
      }
    }
    std::unordered_map<uint64_t,
                       std::vector<std::pair<colmap::image_t, uint32_t>>>
        corr;
    corr.reserve(needed.size());
    {
      auto db = colmap::Database::Open(s->db_path);
      for (const auto& [pair_id, n_inliers] :
           db->ReadTwoViewGeometryNumInliers()) {
        if (n_inliers <= 0) continue;
        const auto [id1, id2] = colmap::PairIdToImagePair(pair_id);
        const colmap::TwoViewGeometry g = db->ReadTwoViewGeometry(id1, id2);
        for (const colmap::FeatureMatch& m : g.inlier_matches) {
          const uint64_t k1 = key_of(id1, m.point2D_idx1);
          const uint64_t k2 = key_of(id2, m.point2D_idx2);
          if (needed.count(k1)) corr[k1].emplace_back(id2, m.point2D_idx2);
          if (needed.count(k2)) corr[k2].emplace_back(id1, m.point2D_idx1);
        }
      }
      db->Close();
    }

                                                                            
                                                                          
    for (const colmap::point3D_t pid : eligible) {
      if (!reconstruction->ExistsPoint3D(pid)) continue;              
      const colmap::Point3D& point = reconstruction->Point3D(pid);

      std::unordered_set<colmap::image_t> excluded;                            
      for (const colmap::TrackElement& el : point.track.Elements()) {
        excluded.insert(el.image_id);
      }
      std::vector<std::pair<colmap::image_t, uint32_t>> kept;
      for (const colmap::TrackElement& el : point.track.Elements()) {
        const auto it = corr.find(key_of(el.image_id, el.point2D_idx));
        if (it == corr.end()) continue;
        for (const auto& [img2, idx2] : it->second) {
          if (excluded.count(img2)) continue;
          if (!reconstruction->ExistsImage(img2)) continue;
          const colmap::Image& im2 = reconstruction->Image(img2);
          if (!im2.HasPose() || idx2 >= im2.NumPoints2D()) continue;
          if (im2.Point2D(idx2).HasPoint3D()) continue;                            
          excluded.insert(img2);                            
          kept.emplace_back(img2, idx2);
        }
      }
      if (kept.empty()) continue;
      ++s->stat_upgrade_attempted;

      Eigen::Vector3d accepted_xyz = Eigen::Vector3d::Zero();
      bool accepted = false;
      for (int round = 0; round < 3 && !kept.empty(); ++round) {
        std::vector<Eigen::Matrix3x4d> cams_from_world;
        std::vector<Eigen::Vector2d> cam_points;
        std::vector<const colmap::Image*> imgs;
        std::vector<const colmap::Camera*> cams;
        std::vector<Eigen::Vector2d> px;
        bool bad = false;
        const auto push_obs = [&](colmap::image_t img_id, uint32_t idx) {
          if (bad) return;
          if (!reconstruction->ExistsImage(img_id)) {
            bad = true;
            return;
          }
          const colmap::Image& im = reconstruction->Image(img_id);
          if (!im.HasPose() || idx >= im.NumPoints2D()) {
            bad = true;
            return;
          }
          const colmap::Camera& cm = reconstruction->Camera(im.CameraId());
          const Eigen::Vector2d p = im.Point2D(idx).xy;
          const std::optional<Eigen::Vector2d> nc = cm.CamFromImg(p);
          if (!nc) {
            bad = true;
            return;
          }
          cams_from_world.push_back(im.CamFromWorld().ToMatrix());
          cam_points.push_back(*nc);
          imgs.push_back(&im);
          cams.push_back(&cm);
          px.push_back(p);
        };
        for (const colmap::TrackElement& el : point.track.Elements()) {
          push_obs(el.image_id, el.point2D_idx);
        }
        const size_t n_track = imgs.size();
        for (const auto& [img2, idx2] : kept) push_obs(img2, idx2);
        if (bad) break;

        Eigen::Vector3d X;
        if (!colmap::TriangulateMultiViewPoint(
                colmap::span<const Eigen::Matrix3x4d>(cams_from_world.data(),
                                                      cams_from_world.size()),
                colmap::span<const Eigen::Vector2d>(cam_points.data(),
                                                    cam_points.size()),
                &X)) {
          break;
        }
        PolishPointGN(imgs, cams, px, &X);

        bool track_bad = false;
        std::vector<size_t> drop;                                 
        double worst_res = -1.0;
        size_t worst_cand = static_cast<size_t>(-1);
        for (size_t o = 0; o < imgs.size(); ++o) {
          double res = std::numeric_limits<double>::infinity();
          const Eigen::Vector3d xc = imgs[o]->CamFromWorld() * X;
          if (xc.z() > 0.0) {
            const std::optional<Eigen::Vector2d> pp = cams[o]->ImgFromCam(xc);
            if (pp) res = (*pp - px[o]).norm();
          }
          const bool ok = res <= kMaxReprojPx;
          if (o < n_track) {
            if (!ok) track_bad = true;
          } else {
            const size_t ci = o - n_track;
            if (!ok) drop.push_back(ci);
            if (res > worst_res) {
              worst_res = res;
              worst_cand = ci;
            }
          }
        }
        if (!track_bad && drop.empty()) {
          accepted = true;
          accepted_xyz = X;
          break;
        }
        if (!drop.empty()) {
          for (auto it = drop.rbegin(); it != drop.rend(); ++it) {
            kept.erase(kept.begin() + *it);
          }
        } else {
                                                                            
                                                                               
                                                
          if (worst_cand == static_cast<size_t>(-1)) break;
          kept.erase(kept.begin() + worst_cand);
        }
      }

      if (accepted && !kept.empty()) {
        reconstruction->Point3D(pid).xyz = accepted_xyz;
        for (const auto& [img2, idx2] : kept) {
          reconstruction->AddObservation(
              pid, colmap::TrackElement(img2, idx2));
        }
        ++s->stat_upgrade_accepted;
        s->stat_upgrade_obs_added += static_cast<int64_t>(kept.size());
      }
    }

                                 
    summarize(&s->stat_upgrade_post_npts, &s->stat_upgrade_post_2view,
              &s->stat_upgrade_post_lt3, &s->upgrade_theta_p50_post_deg);
    s->upgrade_ms = NowMs() - t0;
    LOG(WARNING) << "[aether_sfm] track-upgrade: eligible="
                 << s->stat_upgrade_eligible
                 << " attempted=" << s->stat_upgrade_attempted
                 << " accepted=" << s->stat_upgrade_accepted
                 << " obs_added=" << s->stat_upgrade_obs_added << " pre{n="
                 << s->stat_upgrade_pre_npts << " 2view="
                 << s->stat_upgrade_pre_2view << " lt3="
                 << s->stat_upgrade_pre_lt3 << " theta_p50="
                 << s->upgrade_theta_p50_pre_deg << "deg} post{n="
                 << s->stat_upgrade_post_npts << " 2view="
                 << s->stat_upgrade_post_2view << " lt3="
                 << s->stat_upgrade_post_lt3 << " theta_p50="
                 << s->upgrade_theta_p50_post_deg << "deg} in "
                 << static_cast<int64_t>(s->upgrade_ms) << "ms";
  } catch (const std::exception& e) {
                                                              
    s->upgrade_ms = NowMs() - t0;
    LOG(WARNING) << "[aether_sfm] track-upgrade aborted: " << e.what();
  }
}

                                                                             
                                                                        
                                                                          
              
  
                                                                            
                                                                             
                                                                          
                                                                          
                                                                      
                                                                         
                                                                            
                                                                            
                                                                             
                                                                              
                                                    
                                                                            
                                                                        
                                                                         
                                                                                
                                                                           
                                                                     
                                                                  
                                                                           
                                                                            
                                                                              
                                                                                  
                                                                    
                                                                          
                                                                 
void MergeFragmentTracks(aether_sfm_session* s,
                         colmap::Reconstruction* reconstruction) {
  if (kProductionOfficialEndpointOnly) return;
  if (!FragMergeEnabled() || !s || !reconstruction) return;
  const double t0 = NowMs();
  try {
    const double kMaxReprojPx = FragMergeMaxReprojPx() > 0.0
                                    ? FragMergeMaxReprojPx()
                                    : TemporalDetailMaxReprojPx();
    constexpr double kLowParallaxRad = 3.0 * M_PI / 180.0;
                                                                            
    const double kMinThetaRad = FragMergeMinThetaDeg() * M_PI / 180.0;

    const auto theta_max = [&](const colmap::Track& track,
                               const Eigen::Vector3d& xyz) {
      double best = 0.0;
      const auto& els = track.Elements();
      for (size_t a = 0; a < els.size(); ++a) {
        if (!reconstruction->ExistsImage(els[a].image_id)) continue;
        const colmap::Image& ia = reconstruction->Image(els[a].image_id);
        if (!ia.HasPose()) continue;
        const Eigen::Vector3d ca = ia.CamFromWorld().TgtOriginInSrc();
        for (size_t b = a + 1; b < els.size(); ++b) {
          if (!reconstruction->ExistsImage(els[b].image_id)) continue;
          const colmap::Image& ib = reconstruction->Image(els[b].image_id);
          if (!ib.HasPose()) continue;
          best = std::max(best,
                          colmap::CalculateTriangulationAngle(
                              ca, ib.CamFromWorld().TgtOriginInSrc(), xyz));
        }
      }
      return best;
    };
    const auto summarize = [&](int64_t* n_out, int64_t* n2_out,
                               int64_t* lt3_out, double* p50_out) {
      std::vector<double> thetas;
      thetas.reserve(reconstruction->NumPoints3D());
      int64_t n2 = 0, lt3 = 0;
      for (const auto& [pid, pt] : reconstruction->Points3D()) {
        const double th = theta_max(pt.track, pt.xyz);
        thetas.push_back(th);
        if (pt.track.Length() == 2) ++n2;
        if (th < kLowParallaxRad) ++lt3;
      }
      *n_out = static_cast<int64_t>(thetas.size());
      *n2_out = n2;
      *lt3_out = lt3;
      if (!thetas.empty()) {
        std::nth_element(thetas.begin(), thetas.begin() + thetas.size() / 2,
                         thetas.end());
        *p50_out = thetas[thetas.size() / 2] * 180.0 / M_PI;
      }
    };

                                                                         
                                                                        
                                                                             
                                          
    std::vector<colmap::point3D_t> eligible;
    {
      std::vector<double> thetas;
      thetas.reserve(reconstruction->NumPoints3D());
      int64_t n2 = 0, lt3 = 0;
      for (const auto& [pid, pt] : reconstruction->Points3D()) {
        const double th = theta_max(pt.track, pt.xyz);
        thetas.push_back(th);
        const bool two_view = pt.track.Length() == 2;
        if (two_view) ++n2;
        if (th < kLowParallaxRad) ++lt3;
        if (two_view || th < kLowParallaxRad) eligible.push_back(pid);
      }
      s->stat_frag_pre_npts = static_cast<int64_t>(thetas.size());
      s->stat_frag_pre_2view = n2;
      s->stat_frag_pre_lt3 = lt3;
      if (!thetas.empty()) {
        std::nth_element(thetas.begin(), thetas.begin() + thetas.size() / 2,
                         thetas.end());
        s->frag_theta_p50_pre_deg = thetas[thetas.size() / 2] * 180.0 / M_PI;
      }
    }
    if (eligible.empty()) {
      s->frag_ms = NowMs() - t0;
      return;
    }

                                                                            
                                                                      
    const auto key_of = [](colmap::image_t img, uint32_t idx) {
      return (static_cast<uint64_t>(img) << 32) | idx;
    };
    std::unordered_set<uint64_t> needed;
    needed.reserve(eligible.size() * 3);
    for (const colmap::point3D_t pid : eligible) {
      for (const colmap::TrackElement& el :
           reconstruction->Point3D(pid).track.Elements()) {
        needed.insert(key_of(el.image_id, el.point2D_idx));
      }
    }
    std::unordered_map<uint64_t,
                       std::vector<std::pair<colmap::image_t, uint32_t>>>
        corr;
    corr.reserve(needed.size());
    {
      auto db = colmap::Database::Open(s->db_path);
      for (const auto& [pair_id, n_inliers] :
           db->ReadTwoViewGeometryNumInliers()) {
        if (n_inliers <= 0) continue;
        const auto [id1, id2] = colmap::PairIdToImagePair(pair_id);
        const colmap::TwoViewGeometry g = db->ReadTwoViewGeometry(id1, id2);
        for (const colmap::FeatureMatch& m : g.inlier_matches) {
          const uint64_t k1 = key_of(id1, m.point2D_idx1);
          const uint64_t k2 = key_of(id2, m.point2D_idx2);
          if (needed.count(k1)) corr[k1].emplace_back(id2, m.point2D_idx2);
          if (needed.count(k2)) corr[k2].emplace_back(id1, m.point2D_idx1);
        }
      }
      db->Close();
    }

                                                                          
                                                                        
                                                                
    std::unordered_map<PointIdPair, int, PointIdPairHash> links;
    for (const colmap::point3D_t pid : eligible) {
      if (!reconstruction->ExistsPoint3D(pid)) continue;              
      for (const colmap::TrackElement& el :
           reconstruction->Point3D(pid).track.Elements()) {
        const auto it = corr.find(key_of(el.image_id, el.point2D_idx));
        if (it == corr.end()) continue;
        for (const auto& [img2, idx2] : it->second) {
          if (!reconstruction->ExistsImage(img2)) continue;
          const colmap::Image& im2 = reconstruction->Image(img2);
          if (!im2.HasPose() || idx2 >= im2.NumPoints2D()) continue;
          const colmap::Point2D& p2 = im2.Point2D(idx2);
          if (!p2.HasPoint3D() || p2.point3D_id == pid) continue;
          ++links[CanonicalPointPair(pid, p2.point3D_id)];
        }
      }
    }
    s->stat_frag_links = static_cast<int64_t>(links.size());

                                                       
    std::vector<std::pair<PointIdPair, int>> ranked(links.begin(),
                                                    links.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const std::pair<PointIdPair, int>& a,
                 const std::pair<PointIdPair, int>& b) {
                if (a.second != b.second) return a.second > b.second;
                if (a.first.a != b.first.a) return a.first.a < b.first.a;
                return a.first.b < b.first.b;
              });

                                                                               
                                      
    std::unordered_map<colmap::point3D_t, colmap::point3D_t> redirect;
    const auto resolve = [&](colmap::point3D_t pid) {
      auto it = redirect.find(pid);
      while (it != redirect.end()) {
        pid = it->second;
        it = redirect.find(pid);
      }
      return pid;
    };
    for (const auto& [pair, n_links] : ranked) {
      const colmap::point3D_t a = resolve(pair.a);
      const colmap::point3D_t b = resolve(pair.b);
      if (a == b) continue;                                       
      if (!reconstruction->ExistsPoint3D(a) ||
          !reconstruction->ExistsPoint3D(b)) {
        continue;
      }
      ++s->stat_frag_attempted;
      const colmap::Point3D& pa = reconstruction->Point3D(a);
      const colmap::Point3D& pb = reconstruction->Point3D(b);

                                                                            
                                        
      std::vector<Eigen::Matrix3x4d> cams_from_world;
      std::vector<Eigen::Vector2d> cam_points;
      std::vector<const colmap::Image*> imgs;
      std::vector<const colmap::Camera*> cams;
      std::vector<Eigen::Vector2d> px;
      bool bad = false;
      const auto push_track = [&](const colmap::Track& track) {
        for (const colmap::TrackElement& el : track.Elements()) {
          if (bad) return;
          if (!reconstruction->ExistsImage(el.image_id)) {
            bad = true;
            return;
          }
          const colmap::Image& im = reconstruction->Image(el.image_id);
          if (!im.HasPose() || el.point2D_idx >= im.NumPoints2D()) {
            bad = true;
            return;
          }
          const colmap::Camera& cm = reconstruction->Camera(im.CameraId());
          const Eigen::Vector2d p = im.Point2D(el.point2D_idx).xy;
          const std::optional<Eigen::Vector2d> nc = cm.CamFromImg(p);
          if (!nc) {
            bad = true;
            return;
          }
          cams_from_world.push_back(im.CamFromWorld().ToMatrix());
          cam_points.push_back(*nc);
          imgs.push_back(&im);
          cams.push_back(&cm);
          px.push_back(p);
        }
      };
      push_track(pa.track);
      push_track(pb.track);
      if (bad || cam_points.size() < 3) {
        ++s->stat_frag_reject_degenerate;
        continue;
      }
      Eigen::Vector3d X;
      if (!colmap::TriangulateMultiViewPoint(
              colmap::span<const Eigen::Matrix3x4d>(cams_from_world.data(),
                                                    cams_from_world.size()),
              colmap::span<const Eigen::Vector2d>(cam_points.data(),
                                                  cam_points.size()),
              &X)) {
        ++s->stat_frag_reject_degenerate;
        continue;
      }
      PolishPointGN(imgs, cams, px, &X);

                                                                         
                                                      
      bool ok = true;
      for (size_t o = 0; o < imgs.size(); ++o) {
        const Eigen::Vector3d xc = imgs[o]->CamFromWorld() * X;
        if (xc.z() <= 0.0) {
          ok = false;
          break;
        }
        const std::optional<Eigen::Vector2d> pp = cams[o]->ImgFromCam(xc);
        if (!pp || (*pp - px[o]).norm() > kMaxReprojPx) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        ++s->stat_frag_reject_reproj;
        continue;
      }
                                                                              
                                   
      const double th_a = theta_max(pa.track, pa.xyz);
      const double th_b = theta_max(pb.track, pb.xyz);
      double th_union = 0.0;
      for (size_t i2 = 0; i2 < imgs.size(); ++i2) {
        const Eigen::Vector3d ci =
            imgs[i2]->CamFromWorld().TgtOriginInSrc();
        for (size_t j2 = i2 + 1; j2 < imgs.size(); ++j2) {
          th_union = std::max(
              th_union,
              colmap::CalculateTriangulationAngle(
                  ci, imgs[j2]->CamFromWorld().TgtOriginInSrc(), X));
        }
      }
      if (th_union <= std::max(th_a, th_b)) {
        ++s->stat_frag_reject_theta;
        continue;
      }
                                                                       
                                                                            
                                                                        
                                                                         
      if (kMinThetaRad > 0.0 && th_union < kMinThetaRad) {
        ++s->stat_frag_reject_min_theta;
        continue;
      }

      const size_t union_len = imgs.size();
      const colmap::point3D_t merged = reconstruction->MergePoints3D(a, b);
                                                                      
                                                                        
      reconstruction->Point3D(merged).xyz = X;
      redirect[a] = merged;
      redirect[b] = merged;
      ++s->stat_frag_accepted;
      s->stat_frag_obs_merged += static_cast<int64_t>(union_len);
    }

                                                              
    summarize(&s->stat_frag_post_npts, &s->stat_frag_post_2view,
              &s->stat_frag_post_lt3, &s->frag_theta_p50_post_deg);
    s->frag_ms = NowMs() - t0;
    const bool conserved =
        s->stat_frag_post_npts + s->stat_frag_accepted ==
        s->stat_frag_pre_npts;
    LOG(WARNING) << "[aether_sfm] frag-merge: links=" << s->stat_frag_links
                 << " attempted=" << s->stat_frag_attempted
                 << " accepted=" << s->stat_frag_accepted
                 << " obs_merged=" << s->stat_frag_obs_merged
                 << " rej{reproj=" << s->stat_frag_reject_reproj
                 << " theta=" << s->stat_frag_reject_theta
                 << " min_theta=" << s->stat_frag_reject_min_theta
                 << " degen=" << s->stat_frag_reject_degenerate << "} pre{n="
                 << s->stat_frag_pre_npts << " 2view="
                 << s->stat_frag_pre_2view << " lt3=" << s->stat_frag_pre_lt3
                 << " theta_p50=" << s->frag_theta_p50_pre_deg
                 << "deg} post{n=" << s->stat_frag_post_npts << " 2view="
                 << s->stat_frag_post_2view << " lt3="
                 << s->stat_frag_post_lt3 << " theta_p50="
                 << s->frag_theta_p50_post_deg << "deg} conservation="
                 << (conserved ? "OK" : "VIOLATED") << " in "
                 << static_cast<int64_t>(s->frag_ms) << "ms";
  } catch (const std::exception& e) {
                                                              
    s->frag_ms = NowMs() - t0;
    LOG(WARNING) << "[aether_sfm] frag-merge aborted: " << e.what();
  }
}

std::string ArkitPoseStorePath(const aether_sfm_session& session) {
  return session.db_path + ".arkit_pose_v1";
}

aether::sfm::ArkitPoseRecordV1 ArkitPoseRecordFromFrame(
    const FrameRecord& frame) {
  aether::sfm::ArkitPoseRecordV1 record;
  record.frame_id = frame.frame_id;
  record.image_id = static_cast<uint32_t>(frame.persisted_image_id);
  record.active = frame.image_id != 0;
  record.frame_identity_digest = frame.frame_identity_digest;
  record.cam_from_world_q_xyzw = {
      frame.cam_from_world.rotation().x(), frame.cam_from_world.rotation().y(),
      frame.cam_from_world.rotation().z(), frame.cam_from_world.rotation().w()};
  record.cam_from_world_t_xyz = {frame.cam_from_world.translation().x(),
                                 frame.cam_from_world.translation().y(),
                                 frame.cam_from_world.translation().z()};
  record.gravity_cam_xyz = {frame.gravity_cam.x(), frame.gravity_cam.y(),
                            frame.gravity_cam.z()};
  return record;
}

bool PersistArkitPoseSnapshot(const aether_sfm_session& session,
                              const FrameRecord& pending) {
  std::vector<aether::sfm::ArkitPoseRecordV1> records;
  records.reserve(session.frames.size() + 1);
  for (const FrameRecord& frame : session.frames) {
    records.push_back(ArkitPoseRecordFromFrame(frame));
  }
  records.push_back(ArkitPoseRecordFromFrame(pending));
  return aether::sfm::WriteArkitPoseStoreV1(ArkitPoseStorePath(session),
                                            records) ==
         aether::sfm::ArkitPoseStoreStatusV1::kOk;
}

                                                                               
                                                                               
                                                                                
                                                                             
                                                                           
                                                  
bool PersistArkitPoseSnapshotWithWithdrawal(const aether_sfm_session& session,
                                            int withdrawn_frame_id) {
  if (withdrawn_frame_id < 0 ||
      withdrawn_frame_id >= static_cast<int>(session.frames.size())) {
    return false;
  }
  std::vector<aether::sfm::ArkitPoseRecordV1> records;
  records.reserve(session.frames.size());
  for (const FrameRecord& frame : session.frames) {
    records.push_back(ArkitPoseRecordFromFrame(frame));
  }
  records[static_cast<size_t>(withdrawn_frame_id)].active = false;
  return aether::sfm::WriteArkitPoseStoreV1(ArkitPoseStorePath(session),
                                            records) ==
         aether::sfm::ArkitPoseStoreStatusV1::kOk;
}

bool PersistCurrentArkitPoseSnapshot(const aether_sfm_session& session) {
  std::vector<aether::sfm::ArkitPoseRecordV1> records;
  records.reserve(session.frames.size());
  for (const FrameRecord& frame : session.frames) {
    records.push_back(ArkitPoseRecordFromFrame(frame));
  }
  return aether::sfm::WriteArkitPoseStoreV1(ArkitPoseStorePath(session),
                                            records) ==
         aether::sfm::ArkitPoseStoreStatusV1::kOk;
}

                                                                                                                                                                                   
                                                                              
                                                                               
                                                                              
                                                                                
                                                                               
                                                                              
bool RebuildFrameRecordsForResume(aether_sfm_session* s) {
  if (!s || !s->db) return false;
  if (!s->frames.empty()) return true;
  try {
    std::vector<aether::sfm::ArkitPoseRecordV1> pose_records;
    if (aether::sfm::ReadArkitPoseStoreV1(ArkitPoseStorePath(*s),
                                         &pose_records) !=
        aether::sfm::ArkitPoseStoreStatusV1::kOk) {
      return false;
    }
    std::vector<colmap::Image> images = s->db->ReadAllImages();
    if (images.empty() || images.size() != pose_records.size()) return false;
    std::sort(images.begin(), images.end(),
              [](const colmap::Image& a, const colmap::Image& b) {
                return a.ImageId() < b.ImageId();
              });
    auto restored = std::make_shared<colmap::Reconstruction>();
    std::vector<FrameRecord> frames;
    frames.reserve(images.size());
    std::vector<colmap::image_t> image_ids;
    image_ids.reserve(images.size());
    aether_ba_clear_gravity_priors();
    for (size_t i = 0; i < images.size(); ++i) {
      const colmap::Image& image = images[i];
      const aether::sfm::ArkitPoseRecordV1& pose = pose_records[i];
      if (pose.frame_id != static_cast<int32_t>(i) ||
          pose.image_id != static_cast<uint32_t>(image.ImageId())) {
        return false;
      }
      char expected_name[64];
      std::snprintf(expected_name, sizeof(expected_name), "frame_%06zu.jpg", i);
      if (image.Name() != expected_name) return false;

      FrameRecord rec;
      rec.image_id = image.ImageId();
      rec.persisted_image_id = image.ImageId();
      rec.camera_id = image.CameraId();
      rec.camera = s->db->ReadCamera(image.CameraId());
      rec.frame_id = pose.frame_id;
      const colmap::FeatureKeypoints keypoints =
          s->db->ReadKeypoints(image.ImageId());
      const colmap::FeatureDescriptors descriptors =
          s->db->ReadDescriptors(image.ImageId());
      rec.n_keypoints = static_cast<int>(keypoints.size());
      rec.points = colmap::FeatureKeypointsToPointsVector(keypoints);
      rec.descriptors.assign(descriptors.data.data(),
                             descriptors.data.data() + descriptors.data.size());
      rec.frame_identity_digest = FrameIdentityDigestV1(
          image.Name(), rec.camera, rec.points, rec.descriptors);
      if (rec.frame_identity_digest != pose.frame_identity_digest) return false;
      rec.cam_from_world = colmap::Rigid3d(
          Eigen::Quaterniond(pose.cam_from_world_q_xyzw[3],
                             pose.cam_from_world_q_xyzw[0],
                             pose.cam_from_world_q_xyzw[1],
                             pose.cam_from_world_q_xyzw[2]),
          Eigen::Vector3d(pose.cam_from_world_t_xyz[0],
                          pose.cam_from_world_t_xyz[1],
                          pose.cam_from_world_t_xyz[2]));
      rec.gravity_cam = Eigen::Vector3d(pose.gravity_cam_xyz[0],
                                        pose.gravity_cam_xyz[1],
                                        pose.gravity_cam_xyz[2]);
      rec.has_pose = true;

      if (!pose.active) {
                                                                           
                                                                         
                                                                          
                                                                               
                                                                         
                                                                             
                                                                       
                                                                         
                                                                                
                                                                                 
                                                                              
                                                                                 
                                                             
        rec.image_id = 0;
        rec.descriptors.clear();
        rec.descriptors.shrink_to_fit();
        rec.points.clear();
        rec.points.shrink_to_fit();
        rec.n_keypoints = 0;
        frames.push_back(std::move(rec));
        continue;
      }
      if (!restored->ExistsCamera(rec.camera_id)) {
        restored->AddCameraWithTrivialRig(rec.camera);
      }
      colmap::Image restored_image;
      restored_image.SetImageId(rec.image_id);
      restored_image.SetName(image.Name());
      restored_image.SetCameraId(rec.camera_id);
      restored_image.SetPoints2D(rec.points);
      restored->AddImageWithTrivialFrame(std::move(restored_image),
                                         rec.cam_from_world);
      const double gravity[3] = {rec.gravity_cam.x(), rec.gravity_cam.y(),
                                 rec.gravity_cam.z()};
                                                                            
                                                                              
                                                                               
                                                              
      constexpr double kGravityBaSigmaRad =
          0.5 * 3.14159265358979323846 / 180.0;
      if (!aether_ba_set_gravity_prior(image.Name().c_str(), gravity,
                                       kGravityBaSigmaRad)) {
        return false;
      }
      image_ids.push_back(rec.image_id);
      frames.push_back(std::move(rec));
    }

    if (image_ids.empty()) return false;
    colmap::DatabaseCache::Options cache_options;
    cache_options.min_num_matches = 15;
    cache_options.load_all_images = true;
    auto cache = colmap::DatabaseCache::Create(*s->db, cache_options);
    colmap::IncrementalPipelineOptions options;
    options.min_num_matches = 15;
    options.load_all_images = true;
    options.triangulation.ignore_two_view_tracks = TriIgnoreTwoViewTracks();
    ApplyBirthGateAB(&options);                               
    colmap::IncrementalMapper mapper(cache);
    mapper.BeginReconstruction(restored);
    for (const colmap::image_t image_id : image_ids) {
      mapper.TriangulateImage(options.Triangulation(), image_id);
    }
    mapper.CompleteAndMergeTracks(options.Triangulation());
    mapper.EndReconstruction(            false);

    s->camera_id = frames.front().camera_id;
    s->frames = std::move(frames);
    s->reg_order = std::move(image_ids);
    s->live_recon = std::move(restored);
    s->live_recon_ready = true;
    return true;
  } catch (const std::exception& e) {
    LOG(ERROR) << "[aether_sfm] mandatory ARKit recovery blocked: "
               << e.what();
    s->frames.clear();
    s->reg_order.clear();
    s->live_recon.reset();
    s->live_recon_ready = false;
    return false;
  }
}

                                                                                                                                                           
                                                                        
                                                                          
                                                                              
                                                                        
                                                                          
                                                                           
                                                                              
                                                                             
                                                                                
                                               
  
                                                                             
                                                                 
                                                                       
                                                                            
                              
  
                                                                           
                                                                            
                                                                            
                                                                           
                                                                              
                                                                          
                                                                            
                                                
  
                                                                               
                                                                          
                                                                       
                                                                              
                                                                             
                                                                           
                                                                              
                                                                   
  
                                                                          
                                                                         
                                                                             
                                                                         
                                                                             
         
  
                                                                              
                                                                        
                                                                         
                                                                             
                                                                       
                                                                             
                                                        
constexpr int kRematchMinValidWindowPairs = 4;                                 
constexpr int kRematchValidInlierGate = 15;                                   
constexpr int kRematchNearGap = 2;                                              
constexpr int kFinalizeRematchMaxPairs = 800;                              

                                                                            
                                                                             
                                                                                  
                                                                         
                                                                           
                                                                           
                                                                          
                                                                                
                                                 
  
                                                                         
// derived from the image-id pair, so the RANSAC stream is a function of the
                                                                            
                                                                              
                                                                       
  
                                                                         
int EnrichTvgThreads() {
  static const int cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ENRICH_TVG_THREADS");
    const int v = e && e[0] ? std::atoi(e) : 0;
    if (v <= 0) return 0;
    return std::min(v, 8);
  }();
  return cached;
}

uint32_t EnrichTvgPairSeed(colmap::image_t id1, colmap::image_t id2) {
  uint64_t h = (static_cast<uint64_t>(id1) << 32) | id2;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ULL;
  h ^= h >> 33;
  // Never 0/-1: SetPRNGSeed(-1) means "seed from time" upstream.
  const uint32_t seed = static_cast<uint32_t>(h) | 1u;
  return seed == static_cast<uint32_t>(-1) ? 1u : seed;
}

struct EnrichTvgJob {
                                                                             
                                                                
  const FrameRecord* frame_a = nullptr;
  const FrameRecord* frame_b = nullptr;
  const std::vector<Eigen::Vector2d>* pts_a = nullptr;
  const std::vector<Eigen::Vector2d>* pts_b = nullptr;
  int a_idx = -1, b_idx = -1;                                                  
  colmap::FeatureMatches matches;
  const colmap::TwoViewGeometryOptions* tvg_options = nullptr;
  bool was_probe_debt = false;                                     
           
  aether::sfm::MandatoryGravityTwoViewResultV1 tvg;
  std::atomic<bool> ready{false};
};

class EnrichTvgFarm {
 public:
  explicit EnrichTvgFarm(int n_workers) {
    workers_.reserve(static_cast<size_t>(n_workers));
    for (int i = 0; i < n_workers; ++i) {
      workers_.emplace_back([this] {
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
        for (;;) {
          EnrichTvgJob* job = nullptr;
          {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [this] { return stop_ || !queue_.empty(); });
            if (stop_ && queue_.empty()) return;
            job = queue_.front();
            queue_.pop_front();
          }
          colmap::SetPRNGSeed(EnrichTvgPairSeed(job->frame_a->image_id,
                                                job->frame_b->image_id));
          job->tvg = EstimateMandatoryFrameTwoViewGeometry(
              *job->frame_a, *job->pts_a, *job->frame_b, *job->pts_b,
              job->matches, *job->tvg_options);
          job->ready.store(true, std::memory_order_release);
          done_cv_.notify_all();
        }
      });
    }
  }

  ~EnrichTvgFarm() {
    {
      std::lock_guard<std::mutex> lk(mu_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) w.join();
  }

  void Submit(EnrichTvgJob* job) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      queue_.push_back(job);
    }
    cv_.notify_one();
  }

                                                                           
  void Wait(EnrichTvgJob* job) {
    if (job->ready.load(std::memory_order_acquire)) return;
    std::unique_lock<std::mutex> lk(mu_);
    done_cv_.wait(
        lk, [&] { return job->ready.load(std::memory_order_acquire); });
  }

 private:
  std::vector<std::thread> workers_;
  std::mutex mu_;
  std::condition_variable cv_;                               
  std::condition_variable done_cv_;                                 
  std::deque<EnrichTvgJob*> queue_;
  bool stop_ = false;
};

void FinalizeRematchStarvedFrames(aether_sfm_session* s) {
                                                                            
                                                            
                                                                                
                                                                               
                                                                           
                                                                                     
                                                                      
                                                                             
                                                                           
                                                                             
                                                                          
                                                                          
  if (!s || !s->db || s->frames.size() < 3 || s->camera_id == 0) return;
  const bool gpu_avail =
      s->options.use_gpu_match && (aether_gpu_match_gemm_pairs != nullptr);
                                                                            
                                                                      
  if (s->options.use_gpu_match && !gpu_avail) return;
  try {
    const int num_frames = static_cast<int>(s->frames.size());
    const int K = s->options.k_neighbors > 0 ? s->options.k_neighbors : 12;

    std::unordered_map<colmap::image_t, int> idx_of;
    idx_of.reserve(s->frames.size());
                                                                            
                                                                                
    for (int i = 0; i < num_frames; ++i) {
      if (s->frames[i].image_id == 0) continue;
      idx_of[s->frames[i].image_id] = i;
    }

                                                                           
                                                                       
                                   
    std::vector<int> win_valid(num_frames, 0);
    for (const auto& [pair_id, n_inliers] :
         s->db->ReadTwoViewGeometryNumInliers()) {
      if (n_inliers < kRematchValidInlierGate) continue;
      const auto [id1, id2] = colmap::PairIdToImagePair(pair_id);
      const auto a = idx_of.find(id1);
      const auto b = idx_of.find(id2);
      if (a == idx_of.end() || b == idx_of.end()) continue;
      if (std::abs(a->second - b->second) > K) continue;
      ++win_valid[a->second];
      ++win_valid[b->second];
    }
    std::vector<char> starved(num_frames, 0);
    int64_t n_starved = 0;
    for (int i = 0; i < num_frames; ++i) {
                                                                       
                                                                                 
                                                                            
                                                                           
                                                                         
      if (s->frames[i].n_keypoints > 0 &&
          (win_valid[i] < kRematchMinValidWindowPairs ||
           s->frames[i].fed_throttled)) {
        starved[i] = 1;
        ++n_starved;
      }
    }
    s->stat_finalize_rematch_starved_frames += n_starved;

                                                                          
                                                                 
                                                                    
                                                                    
    std::vector<std::pair<int, int>> todo;                                 
    todo.reserve(kFinalizeRematchMaxPairs);
    for (int gap = 1; gap <= K; ++gap) {
      for (int f = gap; f < num_frames; ++f) {
        const int j = f - gap;
        if (gap > kRematchNearGap && !starved[f] && !starved[j]) continue;
                                                                                    
                                                                                   
                                          
        if (s->frames[j].image_id == 0 || s->frames[f].image_id == 0) continue;
        if (s->frames[j].n_keypoints <= 0 || s->frames[f].n_keypoints <= 0) {
          continue;
        }
        if (s->db->ExistsMatches(s->frames[j].image_id,
                                 s->frames[f].image_id)) {
          continue;
        }
        ++s->stat_finalize_rematch_candidates;
        if (static_cast<int>(todo.size()) < kFinalizeRematchMaxPairs) {
          todo.emplace_back(j, f);
        }
      }
    }
                                                                            
                                                                            
                                                                              
                                                                        
                                                                             
                                                                           
                                                                           
                                                                            
                                                                              
                                                                            
                                                                          
    if (!s->probe_skipped_pairs.empty()) {
      std::unordered_set<uint64_t> in_todo;
      in_todo.reserve(todo.size());
      for (const auto& [tj, tf] : todo) in_todo.insert(FramePairKey(tj, tf));
      std::vector<uint64_t> debt(s->probe_skipped_pairs.begin(),
                                 s->probe_skipped_pairs.end());
      std::sort(debt.begin(), debt.end());                                
      for (const uint64_t key : debt) {
        const int f = static_cast<int>(key >> 32);
        const int j = static_cast<int>(key & 0xFFFFFFFFULL);
        if (j < 0 || f <= j || f >= num_frames) {
          s->probe_skipped_pairs.erase(key);                                 
          continue;
        }
        if (s->frames[j].image_id == 0 || s->frames[f].image_id == 0) {
          s->probe_skipped_pairs.erase(key);                                  
          continue;
        }
        if (s->frames[j].n_keypoints <= 0 || s->frames[f].n_keypoints <= 0) {
          continue;
        }
        if (in_todo.count(key)) continue;                                   
        if (s->db->ExistsMatches(s->frames[j].image_id,
                                 s->frames[f].image_id)) {
          s->probe_skipped_pairs.erase(key);                                
          continue;
        }
        todo.emplace_back(j, f);
      }
    }
    if (todo.empty()) return;
                                                                          
                                                                            
    std::sort(todo.begin(), todo.end(),
              [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                if (a.second != b.second) return a.second < b.second;
                return a.first < b.first;
              });

                                                                          
                                                                           
                                          
    struct RematchFeat {
      const uint8_t* desc = nullptr;
      const std::vector<Eigen::Vector2d>* pts = nullptr;
      int n = 0;
      std::vector<uint8_t> desc_store;
      std::vector<Eigen::Vector2d> pts_store;
    };
    std::unordered_map<int, RematchFeat> cache;
    const auto get_feat = [&](int i) -> const RematchFeat* {
      const FrameRecord& fr = s->frames[i];
      auto it = cache.find(i);
      if (it != cache.end()) return it->second.n > 0 ? &it->second : nullptr;
      RematchFeat& feat = cache[i];
      if (!fr.descriptors.empty() && !fr.points.empty()) {
        feat.desc = fr.descriptors.data();
        feat.pts = &fr.points;
        feat.n = fr.n_keypoints;
        return &feat;
      }
      const colmap::FeatureKeypoints kps = s->db->ReadKeypoints(fr.image_id);
      const colmap::FeatureDescriptors d = s->db->ReadDescriptors(fr.image_id);
      const int n = static_cast<int>(kps.size());
      if (n <= 0 || d.data.rows() != n || d.data.cols() != 128) {
        feat.n = 0;                                       
        return nullptr;
      }
      feat.desc_store.assign(d.data.data(),
                             d.data.data() + static_cast<size_t>(n) * 128);
      feat.pts_store = colmap::FeatureKeypointsToPointsVector(kps);
      feat.desc = feat.desc_store.data();
      feat.pts = &feat.pts_store;
      feat.n = n;
      return &feat;
    };

                                                                            
    const double ratio =
        s->options.match_max_ratio > 0 ? s->options.match_max_ratio : 0.8;
    const colmap::TwoViewGeometryOptions tvg_options;                    
    std::vector<uint32_t> pair_buf;
    size_t done = 0;
                                                                           
                                                                             
                                                                        
                                                              
    const int tvg_threads = EnrichTvgThreads();
    std::unique_ptr<EnrichTvgFarm> farm;
    if (tvg_threads > 0) farm = std::make_unique<EnrichTvgFarm>(tvg_threads);
    const size_t max_inflight = static_cast<size_t>(tvg_threads) * 2;
    std::deque<std::unique_ptr<EnrichTvgJob>> inflight;
    const auto commit_front = [&] {
      EnrichTvgJob* job = inflight.front().get();
      farm->Wait(job);
      if (MandatoryGravityTvgPersistable(job->tvg)) {
                                                                             
        s->db->WriteMatches(job->frame_a->image_id, job->frame_b->image_id,
                            job->matches);
        const colmap::TwoViewGeometry& geometry = job->tvg.geometry;
        s->db->WriteTwoViewGeometry(job->frame_a->image_id,
                                    job->frame_b->image_id, geometry);
        ++s->stat_finalize_rematch_written;
        s->stat_finalize_rematch_inliers +=
            static_cast<int64_t>(geometry.inlier_matches.size());
        if (job->was_probe_debt && !geometry.inlier_matches.empty()) {
          s->probe_debt_grow.push_back(ProbeDebtGrowPair{
              job->a_idx, job->b_idx, geometry.inlier_matches});
        }
      }
      inflight.pop_front();
    };
    const auto drain_all = [&] {
      while (!inflight.empty()) commit_front();
    };
    for (const auto& [j, f] : todo) {
                                                                              
                                                                          
                                                                              
                                                                     
      if (EnrichBudgetExhausted(s)) {
        const int64_t remaining = static_cast<int64_t>(todo.size() - done);
        s->stat_enrich_budget_stopped += remaining;
        LOG(WARNING) << "[aether_sfm] finalize re-match stopped by the "
                        "enrichment time budget: "
                     << remaining << " of " << todo.size()
                     << " candidate pairs left unattempted";
        break;
      }
      ++done;
                                                                        
                                                                       
      for (auto it = cache.begin(); it != cache.end();) {
        if (it->first < f - K) it = cache.erase(it);
        else ++it;
      }
      const RematchFeat* fa = get_feat(j);
      const RematchFeat* fb = get_feat(f);
      if (!fa || !fb) continue;
      const int cap = fa->n < fb->n ? fa->n : fb->n;
      pair_buf.resize(static_cast<size_t>(cap) * 2);
      int num_matches = 0;
      ++s->stat_finalize_rematch_attempted;
                                                                             
                                                                        
      const int mrc =
          gpu_avail
              ? GpuMatchGemmPairsRetry(s, s->frames[j].frame_id, fa->desc,
                                       fa->n, s->frames[f].frame_id, fb->desc,
                                       fb->n, ratio, pair_buf.data(), cap,
                                       &num_matches)
              : aether_sift_match_pairs(fa->desc, fa->n, fb->desc, fb->n,
                                        ratio, pair_buf.data(), cap,
                                        &num_matches);
      if (mrc != 0) {
                                                                         
                                                                           
                                                                            
                                 
        ++s->stat_finalize_rematch_failed;
        continue;
      }
                                                                         
                                                                               
                                             
      bool was_probe_debt = false;
      if (!s->probe_skipped_pairs.empty() &&
          s->probe_skipped_pairs.erase(FramePairKey(j, f)) > 0) {
        ++s->stat_probe_debt_repaid_finalize;
        was_probe_debt = true;
      }
      if (num_matches <= 0) continue;                               

      colmap::FeatureMatches matches(num_matches);
      for (int m = 0; m < num_matches; ++m) {
        matches[m].point2D_idx1 = pair_buf[2 * m];
        matches[m].point2D_idx2 = pair_buf[2 * m + 1];
      }
                                                                              
                                                                        
                                                                            
                                                         
      const bool farm_eligible =
          farm && !s->frames[j].descriptors.empty() &&
          !s->frames[f].descriptors.empty();
      if (farm_eligible) {
        auto job = std::make_unique<EnrichTvgJob>();
        job->frame_a = &s->frames[j];
        job->frame_b = &s->frames[f];
        job->pts_a = fa->pts;
        job->pts_b = fb->pts;
        job->a_idx = j;
        job->b_idx = f;
        job->matches = std::move(matches);
        job->tvg_options = &tvg_options;
        job->was_probe_debt = was_probe_debt;
        farm->Submit(job.get());
        inflight.push_back(std::move(job));
        while (inflight.size() >= max_inflight) commit_front();
        continue;
      }
      if (farm) drain_all();                                               
      const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
          s->frames[j], *fa->pts, s->frames[f], *fb->pts, matches,
          tvg_options);
      if (!MandatoryGravityTvgPersistable(tvg)) {
                                                                              
                                                                      
        continue;
      }
      s->db->WriteMatches(s->frames[j].image_id, s->frames[f].image_id,
                          matches);
      const colmap::TwoViewGeometry& geometry = tvg.geometry;
      s->db->WriteTwoViewGeometry(s->frames[j].image_id,
                                  s->frames[f].image_id, geometry);
      ++s->stat_finalize_rematch_written;
      s->stat_finalize_rematch_inliers +=
          static_cast<int64_t>(geometry.inlier_matches.size());
                                                                           
                                                                            
                                                                           
                                                                            
                                                                          
                                                                      
                                                                             
                                              
      if (was_probe_debt && !geometry.inlier_matches.empty()) {
        s->probe_debt_grow.push_back(
            ProbeDebtGrowPair{j, f, geometry.inlier_matches});
      }
    }
                                                                               
                                                                 
    if (farm) drain_all();
    LOG(WARNING) << "[aether_sfm] finalize re-match: starved_frames="
                 << n_starved
                 << " candidates=" << s->stat_finalize_rematch_candidates
                 << " attempted=" << s->stat_finalize_rematch_attempted
                 << " written=" << s->stat_finalize_rematch_written
                 << " inliers=" << s->stat_finalize_rematch_inliers
                 << " failed=" << s->stat_finalize_rematch_failed
                 << " gpu_fail_capture=" << s->stat_gpu_match_fail_total
                 << " probe_debt_repaid="
                 << s->stat_probe_debt_repaid_finalize
                 << " probe_debt_left=" << s->probe_skipped_pairs.size();
                                                                               
                                                                             
                                                                             
                                                                         
                                                                           
                                                     
    {
      char rline[384];
      std::snprintf(rline, sizeof(rline),
                    "{\"t\":%lld,\"type\":\"finalize_rematch_summary\","
                    "\"starved_frames\":%lld,\"candidates\":%lld,"
                    "\"attempted\":%lld,\"written\":%lld,\"inliers\":%lld,"
                    "\"failed\":%lld,\"unattempted\":%lld,"
                    "\"probe_debt_registered\":%lld,"
                    "\"probe_debt_repaid_live\":%lld,"
                    "\"probe_debt_repaid_finalize\":%lld,"
                    "\"probe_debt_left\":%lld}",
                    static_cast<long long>(EpochMs()),
                    (long long)n_starved,
                    (long long)s->stat_finalize_rematch_candidates,
                    (long long)s->stat_finalize_rematch_attempted,
                    (long long)s->stat_finalize_rematch_written,
                    (long long)s->stat_finalize_rematch_inliers,
                    (long long)s->stat_finalize_rematch_failed,
                    (long long)(todo.size() - done),
                    (long long)s->stat_probe_debt_registered,
                    (long long)s->stat_probe_debt_repaid_live,
                    (long long)s->stat_probe_debt_repaid_finalize,
                    (long long)s->probe_skipped_pairs.size());
      AppendMatchFailJsonl(s, rline);
    }
  } catch (const std::exception& e) {
                                                                              
    LOG(WARNING) << "[aether_sfm] finalize re-match aborted: " << e.what();
  }
}

                                                                       
                                     
  
// Upstream: SequentialPairingOptions{overlap = 10, quadratic_overlap = true}
                                                                         
                                                                  
                                                                   
  
                                                                            
                                                                            
                                                                             
                                                                               
// sequence from drifting, which upstream enables by default.
  
// This is pair SELECTION only — as upstream, candidates are matched with the
                                                                            
                                                                
                                                                        
                                                                          
                                                                              
  
                                                                           
                                                                           
// runs AFTER the final global BA. This pass is upstream COLMAP behaviour and
                                                                             
int OfficialQuadraticOverlap() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_QUADRATIC_OVERLAP")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
    return 10;                                                     
  }();
  return cached;
}

void AddOfficialQuadraticPairs(aether_sfm_session* s) {
  const int overlap = OfficialQuadraticOverlap();
  if (overlap <= 0) return;
  if (!s || !s->db || s->frames.size() < 2 || s->camera_id == 0) return;
  const bool gpu_avail =
      s->options.use_gpu_match && (aether_gpu_match_gemm_pairs != nullptr);
                                                                             
                                                           
  if (s->options.use_gpu_match && !gpu_avail) return;
  const double t0 = NowMs();
  try {
    const int num_frames = static_cast<int>(s->frames.size());

    // 1) Upstream pair set: (i, i + 2^k), k = 0..overlap-1. Pairs already in
                                                                              
                                                                         
    std::vector<std::pair<int, int>> todo;
    for (int i = 0; i < num_frames; ++i) {
      for (int k = 0; k < overlap; ++k) {
        const int64_t j64 = static_cast<int64_t>(i) + (1ll << k);
        if (j64 >= num_frames) break;
        const int j = static_cast<int>(j64);
        const colmap::image_t img1 = s->frames[i].image_id;
        const colmap::image_t img2 = s->frames[j].image_id;
        if (img1 == 0 || img2 == 0 || img1 == img2) continue;
        if (s->db->ExistsMatches(img1, img2) ||
            s->db->ExistsTwoViewGeometry(img1, img2)) {
          continue;
        }
        todo.emplace_back(i, j);
      }
    }
                                                                     
                                                                         
                                                                          
                              
                                                                         
                                                                          
                                                                         
                                                                        
                                                               
    std::stable_sort(todo.begin(), todo.end(),
                     [](const std::pair<int, int>& a,
                        const std::pair<int, int>& b) {
                       return (a.second - a.first) < (b.second - b.first);
                     });

                                                                             
                                                                              
                                                                              
                                                                             
                                                                              
                                           
    struct QuadFeat {
      const uint8_t* desc = nullptr;
      const std::vector<Eigen::Vector2d>* pts = nullptr;
      int n = 0;
      bool loaded = false;                              
      uint64_t last_use = 0;
      std::vector<uint8_t> desc_store;
      std::vector<Eigen::Vector2d> pts_store;
    };
    constexpr int kQuadMaxLoadedFrames = 64;                                 
    std::unordered_map<int, QuadFeat> cache;
    uint64_t use_clock = 0;
    int loaded_count = 0;
    const auto get_feat = [&](int i, int pin) -> const QuadFeat* {
      auto it = cache.find(i);
      if (it != cache.end()) {
        it->second.last_use = ++use_clock;
        return it->second.n > 0 ? &it->second : nullptr;
      }
      const FrameRecord& fr = s->frames[i];
      if (!fr.descriptors.empty() && !fr.points.empty()) {
        QuadFeat& feat = cache[i];
        feat.desc = fr.descriptors.data();
        feat.pts = &fr.points;
        feat.n = fr.n_keypoints;
        feat.last_use = ++use_clock;
        return feat.n > 0 ? &feat : nullptr;
      }
      if (loaded_count >= kQuadMaxLoadedFrames) {
        int victim = -1;
        uint64_t oldest = UINT64_MAX;
        for (const auto& [idx, f] : cache) {
          if (f.loaded && idx != pin && f.last_use < oldest) {
            oldest = f.last_use;
            victim = idx;
          }
        }
                                                                        
                                                                             
                       
        if (victim >= 0) {
          cache.erase(victim);
          --loaded_count;
        }
      }
      QuadFeat& feat = cache[i];
      feat.last_use = ++use_clock;
      const colmap::FeatureKeypoints kps = s->db->ReadKeypoints(fr.image_id);
      const colmap::FeatureDescriptors d = s->db->ReadDescriptors(fr.image_id);
      const int n = static_cast<int>(kps.size());
      if (n <= 0 || d.data.rows() != n || d.data.cols() != 128) {
        feat.n = 0;                                       
        return nullptr;
      }
      feat.desc_store.assign(d.data.data(),
                             d.data.data() + static_cast<size_t>(n) * 128);
      feat.pts_store = colmap::FeatureKeypointsToPointsVector(kps);
      feat.desc = feat.desc_store.data();
      feat.pts = &feat.pts_store;
      feat.n = n;
      feat.loaded = true;
      ++loaded_count;
      return &feat;
    };

                                                                              
                                                                          
    //    tightened inlier gates) so what lands in sqlite is what upstream
                                               
    const double ratio =
        s->options.match_max_ratio > 0 ? s->options.match_max_ratio : 0.8;
    const colmap::TwoViewGeometryOptions tvg_options;                    
    std::vector<uint32_t> pair_buf;
    int64_t written = 0;
    int64_t budget_stopped = 0;
    size_t done = 0;
                                                                        
                                                                            
                                                                       
                                                                     
                                                          
    bool quad_pipeline = QuadPipelineEnabled() && gpu_avail;
    if (quad_pipeline) {
      for (const auto& [pi, pj] : todo) {
        if (s->frames[pi].descriptors.empty() ||
            s->frames[pj].descriptors.empty()) {
          quad_pipeline = false;
          break;
        }
      }
    }
    if (quad_pipeline) {
      struct QuadMatchResult {
        int i = 0, j = 0;
        bool ran_matcher = false;
        int mrc = 1;
        int num_matches = 0;
        std::vector<uint32_t> pairs;
      };
      std::mutex q_mu;
      std::condition_variable q_cv;
      std::deque<QuadMatchResult> q;
      bool producer_done = false;
      bool consumer_abort = false;
      constexpr size_t kQuadPipelineDepth = 3;                                
      std::thread producer([&] {
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
        for (const auto& [pi, pj] : todo) {
          {
            std::unique_lock<std::mutex> lk(q_mu);
            q_cv.wait(lk, [&] {
              return q.size() < kQuadPipelineDepth || consumer_abort;
            });
            if (consumer_abort) break;
          }
          QuadMatchResult r;
          r.i = pi;
          r.j = pj;
          const FrameRecord& fa = s->frames[pi];
          const FrameRecord& fb = s->frames[pj];
          const int cap =
              fa.n_keypoints < fb.n_keypoints ? fa.n_keypoints : fb.n_keypoints;
          if (cap > 0) {
            r.pairs.resize(static_cast<size_t>(cap) * 2);
            r.ran_matcher = true;
                                                                              
                                                                    
                                                                         
            if (EpiPriorMatchEnabled() &&
                ArkitGuidedMatchPair(s, fa, fb, pj - pi, ratio,
                                     r.pairs.data(), cap, &r.num_matches)) {
              r.mrc = 0;
            } else {
              r.mrc = GpuMatchGemmPairsRetry(
                  s, fa.frame_id, fa.descriptors.data(), fa.n_keypoints,
                  fb.frame_id, fb.descriptors.data(), fb.n_keypoints, ratio,
                  r.pairs.data(), cap, &r.num_matches);
            }
          }
          std::lock_guard<std::mutex> lk(q_mu);
          q.push_back(std::move(r));
          q_cv.notify_all();
        }
        std::lock_guard<std::mutex> lk(q_mu);
        producer_done = true;
        q_cv.notify_all();
      });
                                                                          
                                                                         
                                                                             
                                                                              
                                                                               
      const int q_tvg_threads = EnrichTvgThreads();
      std::unique_ptr<EnrichTvgFarm> q_farm;
      if (q_tvg_threads > 0)
        q_farm = std::make_unique<EnrichTvgFarm>(q_tvg_threads);
      const size_t q_max_inflight = static_cast<size_t>(q_tvg_threads) * 2;
      std::deque<std::unique_ptr<EnrichTvgJob>> q_inflight;
      const auto q_commit_front = [&] {
        EnrichTvgJob* job = q_inflight.front().get();
        q_farm->Wait(job);
        if (MandatoryGravityTvgPersistable(job->tvg)) {
          s->db->WriteMatches(job->frame_a->image_id, job->frame_b->image_id,
                              job->matches);
          s->db->WriteTwoViewGeometry(job->frame_a->image_id,
                                      job->frame_b->image_id,
                                      job->tvg.geometry);
          ++s->stat_spatial_quadratic_written;
          ++written;
        }
        q_inflight.pop_front();
      };
      const auto q_drain_all = [&] {
        while (!q_inflight.empty()) q_commit_front();
      };
      for (size_t idx = 0; idx < todo.size(); ++idx) {
        QuadMatchResult r;
        {
          std::unique_lock<std::mutex> lk(q_mu);
          q_cv.wait(lk, [&] { return !q.empty() || producer_done; });
          if (q.empty()) break;                         
          r = std::move(q.front());
          q.pop_front();
          q_cv.notify_all();
        }
        if (EnrichBudgetExhausted(s)) {
          budget_stopped = static_cast<int64_t>(todo.size() - done);
          s->stat_enrich_budget_stopped += budget_stopped;
          LOG(WARNING) << "[aether_sfm] official quadratic overlap stopped by "
                          "the enrichment time budget: "
                       << budget_stopped << " of " << todo.size()
                       << " candidate pairs left unattempted";
          break;
        }
        ++done;
        if (!r.ran_matcher) continue;                                         
        ++s->stat_spatial_quadratic_attempted;
        if (r.mrc != 0) continue;
        if (r.num_matches <= 0) continue;
        colmap::FeatureMatches matches(r.num_matches);
        for (int m = 0; m < r.num_matches; ++m) {
          matches[m].point2D_idx1 = r.pairs[2 * m];
          matches[m].point2D_idx2 = r.pairs[2 * m + 1];
        }
        if (q_farm) {
          auto job = std::make_unique<EnrichTvgJob>();
          job->frame_a = &s->frames[r.i];
          job->frame_b = &s->frames[r.j];
          job->pts_a = &s->frames[r.i].points;
          job->pts_b = &s->frames[r.j].points;
          job->a_idx = r.i;
          job->b_idx = r.j;
          job->matches = std::move(matches);
          job->tvg_options = &tvg_options;
          q_farm->Submit(job.get());
          q_inflight.push_back(std::move(job));
          while (q_inflight.size() >= q_max_inflight) q_commit_front();
          continue;
        }
        const colmap::image_t img1 = s->frames[r.i].image_id;
        const colmap::image_t img2 = s->frames[r.j].image_id;
        const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
            s->frames[r.i], s->frames[r.i].points, s->frames[r.j],
            s->frames[r.j].points, matches, tvg_options);
        if (!MandatoryGravityTvgPersistable(tvg)) continue;
        s->db->WriteMatches(img1, img2, matches);
        s->db->WriteTwoViewGeometry(img1, img2, tvg.geometry);
        ++s->stat_spatial_quadratic_written;
        ++written;
      }
                                                                      
                                                        
      if (q_farm) q_drain_all();
      {
        std::lock_guard<std::mutex> lk(q_mu);
        consumer_abort = true;
        q_cv.notify_all();
      }
      producer.join();
    } else
    for (const auto& [i, j] : todo) {
                                                                            
                                                                              
                                                                            
                                                                  
      if (EnrichBudgetExhausted(s)) {
        budget_stopped = static_cast<int64_t>(todo.size() - done);
        s->stat_enrich_budget_stopped += budget_stopped;
        LOG(WARNING) << "[aether_sfm] official quadratic overlap stopped by "
                        "the enrichment time budget: "
                     << budget_stopped << " of " << todo.size()
                     << " candidate pairs left unattempted";
        break;
      }
      ++done;
      const QuadFeat* fa = get_feat(i,         -1);
      if (!fa) continue;
      const QuadFeat* fb = get_feat(j,         i);
      if (!fb) continue;
      const int cap = fa->n < fb->n ? fa->n : fb->n;
      if (cap <= 0) continue;
      pair_buf.resize(static_cast<size_t>(cap) * 2);
      int num_matches = 0;
      ++s->stat_spatial_quadratic_attempted;
                                                                       
                                                               
      int mrc = 1;
      if (EpiPriorMatchEnabled() && gpu_avail &&
          ArkitGuidedMatchPair(s, s->frames[i], s->frames[j], j - i, ratio,
                               pair_buf.data(), cap, &num_matches)) {
        mrc = 0;
      }
                                                                             
      if (mrc != 0) {
        mrc =
            gpu_avail ? GpuMatchGemmPairsRetry(
                            s, s->frames[i].frame_id, fa->desc, fa->n,
                            s->frames[j].frame_id, fb->desc, fb->n, ratio,
                            pair_buf.data(), cap, &num_matches)
                      : aether_sift_match_pairs(fa->desc, fa->n, fb->desc,
                                                fb->n, ratio, pair_buf.data(),
                                                cap, &num_matches);
      }
      if (mrc != 0) continue;                                                   
      if (num_matches <= 0) continue;                               

      colmap::FeatureMatches matches(num_matches);
      for (int m = 0; m < num_matches; ++m) {
        matches[m].point2D_idx1 = pair_buf[2 * m];
        matches[m].point2D_idx2 = pair_buf[2 * m + 1];
      }
      const colmap::image_t img1 = s->frames[i].image_id;
      const colmap::image_t img2 = s->frames[j].image_id;
      const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
          s->frames[i], *fa->pts, s->frames[j], *fb->pts, matches,
          tvg_options);
      if (!MandatoryGravityTvgPersistable(tvg)) continue;
      s->db->WriteMatches(img1, img2, matches);
      s->db->WriteTwoViewGeometry(img1, img2, tvg.geometry);
      ++s->stat_spatial_quadratic_written;
      ++written;
    }
    LOG(WARNING) << "[aether_sfm] official quadratic overlap: overlap="
                 << overlap << " candidates=" << todo.size()
                 << " written=" << written << " in " << (NowMs() - t0)
                 << " ms";
                                                                            
                                                                           
                                                                               
                                    
    {
      char qline[288];
      std::snprintf(qline, sizeof(qline),
                    "{\"t\":%lld,\"type\":\"quadratic_summary\","
                    "\"candidates\":%zu,\"written\":%lld,"
                    "\"budget_stopped\":%lld,\"ms\":%.0f,"
                    "\"prepaid_attempted\":%lld,\"prepaid_written\":%lld}",
                    static_cast<long long>(EpochMs()), todo.size(),
                    (long long)written, (long long)budget_stopped,
                    NowMs() - t0, (long long)s->stat_prepay_attempted,
                    (long long)s->stat_prepay_written);
      AppendMatchFailJsonl(s, qline);
    }
                                                                          
    if (EpiPriorMatchEnabled()) {
      char eline[224];
      std::snprintf(eline, sizeof(eline),
                    "{\"t\":%lld,\"type\":\"epi_summary\","
                    "\"attempted\":%lld,\"fallback\":%lld,"
                    "\"band_base\":%.0f,\"band_per_gap\":%.0f,"
                    "\"band_max\":%.0f,\"fallback_min\":%d}",
                    static_cast<long long>(EpochMs()),
                    (long long)s->stat_epi_attempted,
                    (long long)s->stat_epi_fallback, EpiBandBasePx(),
                    EpiBandPerGapPx(), EpiBandMaxPx(), EpiFallbackMinMatches());
      AppendMatchFailJsonl(s, eline);
    }
  } catch (const std::exception& e) {
    LOG(WARNING) << "[aether_sfm] official quadratic overlap aborted: "
                 << e.what();
  } catch (...) {
    LOG(WARNING) << "[aether_sfm] official quadratic overlap aborted";
  }
}

                                                                             
std::shared_ptr<const colmap::Reconstruction> PickBestAndReport(
    const std::shared_ptr<colmap::ReconstructionManager>& manager,
    double solve_ms, char* out_json, int out_cap) {
  std::shared_ptr<const colmap::Reconstruction> best;
  size_t best_reg = 0, best_pts = 0;
  double best_reproj = 0.0, best_track = 0.0;
  for (size_t i = 0; i < manager->Size(); ++i) {
    const auto& recon = manager->Get(i);
    if (recon->NumRegImages() >= best_reg) {
      best_reg = recon->NumRegImages();
      best_pts = recon->NumPoints3D();
      best_reproj = recon->ComputeMeanReprojectionError();
      best_track = recon->ComputeMeanTrackLength();
      best = recon;
    }
  }
  if (out_json && out_cap > 0) {
    std::snprintf(out_json, out_cap,
                  "{\"solve_ms\":%.1f,\"n_models\":%zu,\"n_registered\":%zu,"
                  "\"n_points3d\":%zu,\"reproj_px\":%.4f,\"track_len\":%.3f}",
                  solve_ms, manager->Size(), best_reg, best_pts, best_reproj,
                  best_track);
  }
  return best;
}

                                                                               
                                                                            
                                                                          
                                                                           
                                                                          
                                                                              
                                                       
                             
double TriMinAngleDeg(const char* env_name) {
  const char* e = std::getenv(env_name);
  if (e && e[0]) {
    const double d = std::atof(e);
    if (d > 0.0) return d;
  }
  return 2.0;
}

                                                                   
                                                                                                       
                                                                                                    
                                              
  
                                                                                           
                                                                                        
                                                                                           
                                                                                                
                                                                                                     
                                                                 
  
                                                                                              
                                                                                               
                                           
                                                                                                                   
                                                                                
                                                                      
int TriMaxTransitivity() {
  const char* e = std::getenv("OFFICIAL_AETHER_TRI_TRANSITIVITY");
  if (e && e[0]) {
    const int v = std::atoi(e);
    if (v >= 1 && v <= 5) return v;
  }
  return 1;                 
}

                                                                         
                                                                       
                                                                                  
                                                                            
                                                                             
                                                                              
                                                                          
// +15.7%). Attribution was exact: the threads-only arm was equal to baseline
                                                                        
                                                                            
                                                                          
                                                                       
                                                                        
                                                                      
                                                                                           
                                                     
bool BaMixedEnabled() {
  const char* e = std::getenv("OFFICIAL_AETHER_BA_MIXED");
  return e && e[0] == '1';
}

                                                                           
                                                                              
                                                                             
                                                                             
                                                                             
                                                                                   
                                                                            
                                                                             
                                       
int FinalizeBaThreads() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_BA_THREADS")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
                                                                             
                                                                             
                                                                               
                                                                          
                                                                       
                                                                         
                                                                          
                                                                                
                                                                      
    const int hw = static_cast<int>(std::thread::hardware_concurrency());
    return std::max(1, std::min(6, hw));
  }();
  return cached;
}

                                                                             
                                                                            
int LiveBaThreads() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_BA_THREADS")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    const int hw = static_cast<int>(std::thread::hardware_concurrency());
    return std::max(1, std::min(6, hw - 2));
  }();
  return cached;
}

                                                                    
  
                                                                        
                                                                              
// the two upstream defaults:
                                                                      
                                                         
                                                                              
                                                                                 
                                                                               
                                                                                
                                                                              
                                                                         
                          
  
                                                                                 
                                                                              
  
// Both knobs default to "leave the upstream value alone" so an unset
                                                                                
                   
int LiveLocalBaThreadsOverride() {                        
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_LBA_THREADS")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;
  }();
  return cached;
}

                                                                             
                                                                                
                                                                              
                                                             
double LiveLocalBaFtol() {
  static const double cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_LBA_FTOL")) {
      const double v = std::atof(e);
      if (v >= 0.0) return v;
    }
    return 0.0;
  }();
  return cached;
}

                                                                                       
                                                                                
                                                                                    
                                                                              
                                                                                    
int LiveLocalBaLossType() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_LBA_LOSS")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
    return -1;
  }();
  return cached;
}

                                                                                         
                                                                                       
                                                                                   
                                                                              
                                                                      
int LiveLocalBaMaxIter() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_LBA_MAX_ITER")) {
      const int v = std::atoi(e);
      if (v > 0) return v;
    }
    return 0;
  }();
  return cached;
}

                                                                             
                                                                                       
                                                                                               
                                                                                                   
                                                                                               
double LiveTriMinAngleDegOverride() {
  static const double cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_TRI_MIN_ANGLE_DEG")) {
      const double v = std::atof(e);
      if (v > 0.0) return v;
    }
    return 0.0;
  }();
  return cached;
}

                                                                                              
                                                                                 
                                                                                               
                                                                                                   
                                                                               
bool FinalizeGravityRaEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_FINALIZE_GRAVITY_RA");
    return e != nullptr && e[0] == '1';
  }();
  return cached;
}

                                                                                     
std::unordered_map<colmap::frame_t, colmap::Rigid3d> MaybeRunFinalizeGravityRa(
    const std::string& db_path, colmap::Reconstruction* refined) {
  if (!FinalizeGravityRaEnabled() || refined == nullptr) return {};
  const double t0 = NowMs();
  try {
                                                                                           
                                          
    std::unordered_map<colmap::frame_t, colmap::Rigid3d> old_poses;
    for (const auto& [frame_id, frame] : refined->Frames()) {
      if (frame.HasPose()) old_poses.emplace(frame_id, frame.RigFromWorld());
    }
    if (old_poses.size() < 3) return {};

                                                                         
    auto database = colmap::Database::Open(db_path);
    colmap::DatabaseCache::Options cache_options;
    cache_options.min_num_matches = 15;
    auto cache = colmap::DatabaseCache::Create(*database, cache_options);
    colmap::MaybeDecomposeRelativePoses(cache.get());
    colmap::PoseGraph pose_graph;
    pose_graph.Load(*cache->CorrespondenceGraph());
    if (pose_graph.Empty()) {
      LOG(WARNING) << "[gravity-ra] empty pose graph — skipped";
      return {};
    }

                                                                                 
                                                                                   
                                                                                             
                                                                                                  
                                                         
    std::vector<colmap::PosePrior> pose_priors;
    pose_priors.reserve(refined->NumImages());
    for (const auto& [image_id, image] : refined->Images()) {
      double g[3];
      if (!aether_ba_get_gravity_prior(image.Name().c_str(), g)) continue;
      Eigen::Vector3d gravity(g[0], g[1], g[2]);
      const double n = gravity.norm();
      if (!std::isfinite(n) || n < 1e-9) continue;
      colmap::PosePrior prior;
      prior.pose_prior_id = image_id;
      prior.gravity = gravity / n;
      pose_priors.push_back(prior);
    }

    colmap::RotationEstimatorOptions ra_options;
    ra_options.use_gravity = !pose_priors.empty();
    ra_options.use_stratified = true;
                                                                                       
                                                                     
    ra_options.skip_initialization = false;
                                                                                  
    ra_options.filter_unregistered = true;

    if (!colmap::RunRotationAveraging(
            ra_options, pose_graph, *refined, pose_priors)) {
      LOG(WARNING) << "[gravity-ra] solve failed — old poses restored";
      for (const auto& [frame_id, pose] : old_poses) {
        refined->Frame(frame_id).SetRigFromWorld(pose);
      }
      return {};
    }

                                                                                            
                                                                                                
                                                                                              
                                                                                                     
    Eigen::Matrix3d gauge_accum = Eigen::Matrix3d::Zero();
    int gauge_n = 0;
    for (const auto& [frame_id, old_pose] : old_poses) {
      const auto& frame = refined->Frame(frame_id);
      if (!frame.HasPose()) continue;
      const auto& ra_q = frame.RigFromWorld().rotation();
      if (ra_q.coeffs().hasNaN()) continue;
      gauge_accum += ra_q.toRotationMatrix().transpose() *
                     old_pose.rotation().toRotationMatrix();
      ++gauge_n;
    }
    Eigen::Matrix3d gauge = Eigen::Matrix3d::Identity();
    if (gauge_n >= 3) {
      Eigen::JacobiSVD<Eigen::Matrix3d> svd(
          gauge_accum, Eigen::ComputeFullU | Eigen::ComputeFullV);
      gauge = svd.matrixU() * svd.matrixV().transpose();
      if (gauge.determinant() < 0) {
        Eigen::Matrix3d flip = Eigen::Matrix3d::Identity();
        flip(2, 2) = -1;
        gauge = svd.matrixU() * flip * svd.matrixV().transpose();
      }
    }
    const Eigen::AngleAxisd gauge_aa(gauge);
    const Eigen::Quaterniond gauge_q(gauge);

                                                                                                          
    int applied = 0;
    double max_delta_deg = 0.0, sum_delta_deg = 0.0;
    for (const auto& [frame_id, old_pose] : old_poses) {
      auto& frame = refined->Frame(frame_id);
      const colmap::Rigid3d ra_pose =
          frame.HasPose() ? frame.RigFromWorld() : old_pose;
      const bool ra_valid = frame.HasPose() &&
                            !ra_pose.rotation().coeffs().hasNaN();
      if (!ra_valid) {
        frame.SetRigFromWorld(old_pose);
        continue;
      }
      const Eigen::Quaterniond aligned_q = ra_pose.rotation() * gauge_q;
      const Eigen::Vector3d center_old =
          -(old_pose.rotation().inverse() * old_pose.translation());
      frame.SetRigFromWorld(
          colmap::Rigid3d(aligned_q, -(aligned_q * center_old)));
      const double delta_deg =
          old_pose.rotation().angularDistance(aligned_q) * 180.0 / M_PI;
      max_delta_deg = std::max(max_delta_deg, delta_deg);
      sum_delta_deg += delta_deg;
      ++applied;
    }
    LOG(INFO) << "[gravity-ra] applied=" << applied << "/" << old_poses.size()
              << " priors=" << pose_priors.size()
              << " gauge_deg=" << gauge_aa.angle() * 180.0 / M_PI
              << " gauge_axis=[" << gauge_aa.axis().transpose() << "]"
              << " mean_delta=" << (applied ? sum_delta_deg / applied : 0.0)
              << "deg max_delta=" << max_delta_deg
              << "deg ms=" << (NowMs() - t0);
    return old_poses;
  } catch (const std::exception& e) {
    LOG(WARNING) << "[gravity-ra] exception: " << e.what() << " — skipped";
  } catch (...) {
    LOG(WARNING) << "[gravity-ra] unknown exception — skipped";
  }
  return {};
}

                                                                                       
                                                                                                
                                                                                               
                                                                                             
                                                                                      
                                                                                    
void MaybeReanchorAfterGravityRa(
    const std::unordered_map<colmap::frame_t, colmap::Rigid3d>& snapshot,
    colmap::Reconstruction* refined) {
  if (snapshot.empty() || refined == nullptr) return;
  try {
    std::vector<Eigen::Vector3d> src, dst;
    src.reserve(snapshot.size());
    dst.reserve(snapshot.size());
    for (const auto& [frame_id, old_pose] : snapshot) {
      if (!refined->ExistsFrame(frame_id)) continue;
      const auto& frame = refined->Frame(frame_id);
      if (!frame.HasPose()) continue;
      const auto& cur = frame.RigFromWorld();
      if (cur.rotation().coeffs().hasNaN() || cur.translation().hasNaN()) {
        continue;
      }
      src.push_back(-(cur.rotation().inverse() * cur.translation()));
      dst.push_back(
          -(old_pose.rotation().inverse() * old_pose.translation()));
    }
    if (src.size() < 3) return;
    Eigen::Matrix<double, 3, Eigen::Dynamic> S(3, src.size()), D(3, src.size());
    for (size_t i = 0; i < src.size(); ++i) {
      S.col(i) = src[i];
      D.col(i) = dst[i];
    }
    const Eigen::Matrix4d T = Eigen::umeyama(S, D,                  true);
    const Eigen::Matrix3d sR = T.topLeftCorner<3, 3>();
    const double scale = std::cbrt(sR.determinant());
    if (!std::isfinite(scale) || scale < 0.5 || scale > 2.0) {
      LOG(WARNING) << "[gravity-ra] reanchor scale " << scale
                   << " out of sane band — skipped";
      return;
    }
    const Eigen::Matrix3d R = sR / scale;
    const colmap::Sim3d new_from_old(
        scale, Eigen::Quaterniond(R), T.topRightCorner<3, 1>());
    refined->Transform(new_from_old);
    const Eigen::AngleAxisd aa(R);
    LOG(INFO) << "[gravity-ra] reanchored: rot="
              << aa.angle() * 180.0 / M_PI
              << "deg scale=" << scale
              << " t=" << T.topRightCorner<3, 1>().norm();
  } catch (const std::exception& e) {
    LOG(WARNING) << "[gravity-ra] reanchor exception: " << e.what();
  } catch (...) {
    LOG(WARNING) << "[gravity-ra] reanchor unknown exception";
  }
}

int LiveLocalBaMtFloorOverride() {                       
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_LBA_MT_FLOOR")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
    return -1;
  }();
  return cached;
}

                                                                                
                                                                            
aether_sfm_result_t RunIncremental(
    const std::string& db_path, const std::string& image_path,
    const aether_sfm_options_t& opts,
    aether::official::ba::BaSessionAggregateAccumulatorV1* ba_ptol_aggregate,
    aether::official::ba::BaSessionReceiptRingV1* ba_ptol_receipts,
    std::shared_ptr<colmap::ReconstructionManager>* out_manager,
    std::shared_ptr<const colmap::Reconstruction>* out_recon, char* out_json,
    int out_cap, bool local_only = false,
                                                                                               
                                                                               
                                                                                         
                                           
    const std::string& seed_model_path = std::string()) {
  aether::official::ba::ScopedBaSessionAggregateBindingV1 ba_session_binding(
      ba_ptol_aggregate, ba_ptol_receipts);
  try {
    auto pipeline_opts = std::make_shared<colmap::IncrementalPipelineOptions>();
    pipeline_opts->triangulation.ignore_two_view_tracks = TriIgnoreTwoViewTracks();
    pipeline_opts->min_num_matches = 15;
                                                                            
                                                                         
    if (image_path.empty()) pipeline_opts->extract_colors = false;
                                                                          
                                                                               
                                                                             
                                                                               
                                                                                
                                                                                
                                                                               
                                                                               
                                                                        
                                                                                
                                                                                   
    pipeline_opts->defer_global_ba = true;
                                                                                 
                                                                               
                                                                                 
                                                          
    if (local_only) pipeline_opts->skip_finalize_global_ba = true;
                                                                                 
                                                                                   
    pipeline_opts->ba_local_max_num_iterations = 15;
    pipeline_opts->ba_min_num_residuals_for_cpu_multi_threading = 6000;
                                                                                    
                                                                                
                                                   
                                                                                  
                                                                              
                                                                                    
                                                                                   
                                                                     
                                                                                  
                                                                                  
                                                                                   
                                                                                  
                                                                                 
                                                                                         
    pipeline_opts->ba_local_loss_type = 2;              
    pipeline_opts->ba_local_loss_scale = 1.0;
    pipeline_opts->ba_global_loss_type = 2;                                           
    pipeline_opts->ba_global_loss_scale = 1.0;
    pipeline_opts->ba_global_function_tolerance = 1e-6;                  
                                                                         
                                                                      
                                                                                
                                                                             
                                                                             
                                                                           
                                                                     
                                                                             
                                                  
    pipeline_opts->ba_global_mixed_precision = BaMixedEnabled();
    pipeline_opts->num_threads = FinalizeBaThreads();
    pipeline_opts->mapper.ba_local_num_images = 6;
                                                                                 
                                                                              
    pipeline_opts->ba_refine_focal_length = false;
    pipeline_opts->ba_refine_principal_point = false;
    pipeline_opts->ba_refine_extra_params = false;
                                                                                  
                                                                                   
                                                                                
                                                                                
                                                                              
    // down to 1.5° pairwise parallax (cap47 attribution: 25.9% of native
                                                                                 
                                                                             
                                                                               
                                                                            
                                                                           
                                         
    pipeline_opts->triangulation.min_angle =
        TriMinAngleDeg("OFFICIAL_AETHER_TRI_MIN_ANGLE");
                                                                                       
    ApplyBirthGateAB(pipeline_opts.get());
                                                                                   
                                                                                       
                                                                                  
                                                                                      
                                                                                         
                                                        
                                                                            
                                                                              
                                                                                  
                                                                                  
                                                                               
      
                                                                          
                                                                                   
                                                                            
                                                                                   
    auto manager = std::make_shared<colmap::ReconstructionManager>();
                                                                                             
                                                                                                 
                                                                                  
                                                                   
    if (!seed_model_path.empty()) {
      manager->Read(seed_model_path);
    }

    const double t0 = NowMs();
    pipeline_opts->image_path = image_path;                                          
    colmap::IncrementalPipeline pipeline(
        pipeline_opts, colmap::Database::Open(db_path), manager);
    pipeline.Run();
    const double solve_ms = NowMs() - t0;

    *out_manager = manager;
    *out_recon = PickBestAndReport(manager, solve_ms, out_json, out_cap);
    return AETHER_SFM_OK;
  } catch (const std::exception& e) {
    if (out_json && out_cap > 0) {
      std::snprintf(out_json, out_cap, "{\"error\":\"%s\"}", e.what());
    }
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                         
                                                                              
                                                                             
                                                                               
                                           
                                                                     
void MaybeWriteGhostMask(aether_sfm_session* s,
                         const colmap::Reconstruction& recon);

void RefineFailClosed(aether_sfm_session* s, const char* why) {
  LOG(ERROR) << "[aether_sfm] mandatory-ARKit refine blocked (" << why
             << "); no P3P/full-rerun fallback is permitted";
  s->finalize_status.store(3);                              
}

                                                                        
                                                                                  
                                                                                   
                                                                                     
                                                                              
                                                                          
                                                                     
                                                                      
                                                                                     
                                                                    
std::shared_ptr<colmap::IncrementalPipelineOptions> MakePhase2Options(
    aether_sfm_session* s) {
  auto popts = std::make_shared<colmap::IncrementalPipelineOptions>();
  popts->triangulation.ignore_two_view_tracks = TriIgnoreTwoViewTracks();
  popts->min_num_matches = 15;
  popts->ba_min_num_residuals_for_cpu_multi_threading = 6000;
  popts->ba_global_loss_type = 2;                                                     
  popts->ba_global_loss_scale = 1.0;
  popts->ba_global_function_tolerance = 1e-6;                  
                                                                                               
                                                                                               
                                                                                            
                                                                                                  
  if (const char* e = std::getenv("OFFICIAL_AETHER_BA_GLOBAL_FTOL")) {
    const double v = std::atof(e);
    if (v > 0.0) popts->ba_global_function_tolerance = v;
  }
                                                                                              
                                                                                                        
                                                                                                             
                                                                                                       
                                                                                               
                                                                                            
  if (const char* e = std::getenv("OFFICIAL_AETHER_BA_GLOBAL_FTOL_AB")) {
    const double v = std::atof(e);
    if (v > 0.0 && !std::getenv("OFFICIAL_AETHER_BA_GLOBAL_FTOL")) {
      uint64_t h = 1469598103934665603ULL;
      for (const char c : s->db_path) {
        h ^= static_cast<uint8_t>(c);
        h *= 1099511628211ULL;
      }
      const int arm = static_cast<int>(h & 1ULL);
      if (arm == 1) popts->ba_global_function_tolerance = v;
      s->stat_phase2_ftol_ab_arm = arm;
    }
  }
                                                                       
                                                                                         
                                                                                             
                                                                        
                                                                                    
                                                                                   
                                                                                           
                                                                                          
    
                                                                                               
                                                                                          
                                                                                         
                                                                            
                                                                                                        
                                                                                              
                                                                              
                                                                                            
                                                                                                
  popts->ba_global_max_refinements = 5;
  if (const char* e = std::getenv("OFFICIAL_AETHER_FINALIZE_TOTAL_ROUNDS")) {
    const int v = std::atoi(e);
    if (v > 0) popts->ba_global_max_refinements = v;
  }
  popts->ba_global_max_num_iterations = 50;
                                                                                        
                                                                                    
                                                                               
                                                                                            
                                                                                         
  if (const char* e = std::getenv("OFFICIAL_AETHER_BA_GLOBAL_MAX_ITERS")) {
    const int v = std::atoi(e);
    if (v > 0) popts->ba_global_max_num_iterations = v;
  }
  popts->ba_refine_focal_length = false;
  popts->ba_refine_principal_point = false;
  popts->ba_refine_extra_params = false;
                                                                              
                                                                 
                                                                                    
                                                                     
                                                                                 
                                                                             
                     
  popts->ba_global_mixed_precision = BaMixedEnabled();
  popts->num_threads = FinalizeBaThreads();
                                                                             
                                                                             
                                                                              
                                   
  popts->triangulation.min_angle = TriMinAngleDeg("OFFICIAL_AETHER_TRI_MIN_ANGLE_P2");
                                                                                     
  ApplyBirthGateAB(popts.get());
                                                                                             
                                                                                            
                                                                                
                                               
                                                                                               
                                                       
                                                                               
                                                                                       
                                                                                  
                                                                                      
                                                            
                                                                              
                                                                              
                                                                               
                                                                              
                                                                             
                                                                              
                                                             
  popts->load_all_images = true;
  popts->image_path = s->image_path;                                          
                                                                                            
                                                                                           
                       
  s->stat_phase2_gftol = popts->ba_global_function_tolerance;
  s->stat_phase2_gref = popts->ba_global_max_refinements;
  s->stat_phase2_giter = popts->ba_global_max_num_iterations;
  return popts;
}

                                                                              
                                                                           
                                                                                        
                                                                             
                                                                               
                                                                     
                                                                               
                                                                          
                                                                     
void WriteFinalizeSegments(aether_sfm_session* s, bool live_reuse,
                           double cache_pre_ms, double enrich_ms,
                           double stage1_ms, int stage1_rounds,
                           const char* stage1_state, double stage2_ms,
                           int stage2_rounds_budget, double temporal_ms,
                           double total_ms) {
  try {
    const std::filesystem::path dir =
        std::filesystem::path(s->db_path).parent_path();
    const std::filesystem::path tmp = dir / "official_finalize_segments.json.tmp";
    const std::filesystem::path dst = dir / "official_finalize_segments.json";
    std::string solver_used, sparse_backend, dense_backend;
    int mixed = 0, threads = 0;
    colmap::AetherLastBaSolveInfo(&solver_used, &sparse_backend, &mixed,
                                  &threads, &dense_backend);
    const int64_t epoch_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    char buf[4096];                                                    
    const int n = std::snprintf(
        buf, sizeof(buf),
        "{\"t\":%lld,\"live_reuse\":%d,"
        "\"cache_pre_ms\":%lld,\"enrich_ms\":%lld,"
        "\"stage1_ms\":%lld,\"stage1_rounds\":%d,\"stage1_state\":\"%s\","
        "\"stage2_ms\":%lld,\"stage2_rounds_budget\":%d,"
        "\"temporal_ms\":%lld,\"total_ms\":%lld,"
        "\"rematch_starved_frames\":%lld,\"rematch_candidates\":%lld,"
        "\"rematch_attempted\":%lld,\"rematch_written\":%lld,"
        "\"rematch_inliers\":%lld,\"rematch_failed\":%lld,"
        "\"rematch_budget\":%d,"

                                                          
        "\"repay_calls\":%lld,\"repay_attempted\":%lld,"
        "\"repay_written\":%lld,\"repay_inliers\":%lld,"
        "\"repay_failed\":%lld,\"repay_skipped_thermal\":%lld,"
                                                                            
        "\"idle_prepay_ticks\":%lld,"

                                                           
                                                                      
                                                                     
                                                           
        "\"probe_attempted\":%lld,\"probe_skipped\":%lld,"
        "\"probe_passed\":%lld,\"probe_fail_open\":%lld,\"probe_ms\":%lld,"
        "\"probe_batch_calls\":%lld,\"probe_debt_registered\":%lld,"
        "\"probe_debt_repaid_live\":%lld,"
        "\"probe_debt_repaid_finalize\":%lld,\"probe_debt_left\":%lld,"
                                                                           
                                                                             
        "\"probe_debt_grow_pairs\":%lld,\"probe_debt_grow_added\":%lld,"
        "\"probe_debt_grow_grown\":%lld,"
        "\"probe_debt_grow_touched\":%lld,\"probe_debt_grow_ms\":%lld,"
        "\"gpu_retry_attempts\":%lld,\"gpu_retry_recovered\":%lld,"
        "\"enrich_budget_mode\":%d,\"enrich_budget_stopped\":%lld,"

                                        
        "\"grow_refit_attempted\":%lld,\"grow_refit_accepted\":%lld,"
        "\"grow_refit_saved\":%lld,\"grow_refit_min_theta_rej\":%lld,"
        "\"upgrade_eligible\":%lld,\"upgrade_attempted\":%lld,"
        "\"upgrade_accepted\":%lld,\"upgrade_obs_added\":%lld,"
        "\"upgrade_ms\":%lld,"
        "\"theta_pre_n\":%lld,\"theta_pre_2view\":%lld,"
        "\"theta_pre_lt3\":%lld,\"theta_pre_p50_deg\":%.3f,"
        "\"theta_post_n\":%lld,\"theta_post_2view\":%lld,"
        "\"theta_post_lt3\":%lld,\"theta_post_p50_deg\":%.3f,"
        "\"enrich_targeted_tracks\":%lld,\"enrich_targeted_scored\":%lld,"

                                                
        "\"frag_links\":%lld,\"frag_attempted\":%lld,"
        "\"frag_accepted\":%lld,\"frag_obs_merged\":%lld,"
        "\"frag_reject_reproj\":%lld,\"frag_reject_theta\":%lld,"
        "\"frag_reject_min_theta\":%lld,"
        "\"frag_reject_degenerate\":%lld,\"frag_ms\":%lld,"
        "\"frag_pre_n\":%lld,\"frag_pre_2view\":%lld,\"frag_pre_lt3\":%lld,"
        "\"frag_pre_p50_deg\":%.3f,"
        "\"frag_post_n\":%lld,\"frag_post_2view\":%lld,"
        "\"frag_post_lt3\":%lld,\"frag_post_p50_deg\":%.3f,"

                                                               
        "\"guided_temporal_attempted\":%lld,"
        "\"guided_temporal_upgraded\":%lld,"
        "\"guided_temporal_extra_inliers\":%lld,"
        "\"solver_used\":\"%s\",\"sparse_backend\":\"%s\","
        "\"dense_backend\":\"%s\","
                                                                                   
                                                                                           
        "\"gftol_used\":%.3e,\"gref_used\":%d,\"giter_used\":%d,"
        "\"ftol_ab_arm\":%d,"
        "\"mixed\":%d,\"threads\":%d}\n",
        static_cast<long long>(epoch_ms), live_reuse ? 1 : 0,
        static_cast<long long>(cache_pre_ms),
        static_cast<long long>(enrich_ms), static_cast<long long>(stage1_ms),
        stage1_rounds, stage1_state, static_cast<long long>(stage2_ms),
        stage2_rounds_budget, static_cast<long long>(temporal_ms),
        static_cast<long long>(total_ms),
        static_cast<long long>(s->stat_finalize_rematch_starved_frames),
        static_cast<long long>(s->stat_finalize_rematch_candidates),
        static_cast<long long>(s->stat_finalize_rematch_attempted),
        static_cast<long long>(s->stat_finalize_rematch_written),
        static_cast<long long>(s->stat_finalize_rematch_inliers),
        static_cast<long long>(s->stat_finalize_rematch_failed),
        kFinalizeRematchMaxPairs,
        static_cast<long long>(s->stat_repay_calls),
        static_cast<long long>(s->stat_repay_attempted),
        static_cast<long long>(s->stat_repay_written),
        static_cast<long long>(s->stat_repay_inliers),
        static_cast<long long>(s->stat_repay_failed),
        static_cast<long long>(s->stat_repay_skipped_thermal),
        static_cast<long long>(s->stat_idle_prepay_ticks),
        static_cast<long long>(s->stat_probe_attempted),
        static_cast<long long>(s->stat_probe_skipped),
        static_cast<long long>(s->stat_probe_passed),
        static_cast<long long>(s->stat_probe_fail_open),
        static_cast<long long>(s->stat_probe_ms),
        static_cast<long long>(s->stat_probe_batch_calls),
        static_cast<long long>(s->stat_probe_debt_registered),
        static_cast<long long>(s->stat_probe_debt_repaid_live),
        static_cast<long long>(s->stat_probe_debt_repaid_finalize),
        static_cast<long long>(s->probe_skipped_pairs.size()),
        static_cast<long long>(s->stat_probe_debt_grow_pairs),
        static_cast<long long>(s->stat_probe_debt_grow_added),
        static_cast<long long>(s->stat_probe_debt_grow_grown),
        static_cast<long long>(s->stat_probe_debt_grow_touched),
        static_cast<long long>(s->stat_probe_debt_grow_ms),
        static_cast<long long>(s->stat_gpu_retry_attempts),
        static_cast<long long>(s->stat_gpu_retry_recovered),
        static_cast<int>(EnrichBudgetModeOf()),
        static_cast<long long>(s->stat_enrich_budget_stopped),
        static_cast<long long>(s->stat_td_grow_refit_attempted),
        static_cast<long long>(s->stat_td_grow_refit_accepted),
        static_cast<long long>(s->stat_td_grow_refit_saved),
        static_cast<long long>(s->stat_td_grow_refit_min_theta_rej),
        static_cast<long long>(s->stat_upgrade_eligible),
        static_cast<long long>(s->stat_upgrade_attempted),
        static_cast<long long>(s->stat_upgrade_accepted),
        static_cast<long long>(s->stat_upgrade_obs_added),
        static_cast<long long>(s->upgrade_ms),
        static_cast<long long>(s->stat_upgrade_pre_npts),
        static_cast<long long>(s->stat_upgrade_pre_2view),
        static_cast<long long>(s->stat_upgrade_pre_lt3),
        s->upgrade_theta_p50_pre_deg,
        static_cast<long long>(s->stat_upgrade_post_npts),
        static_cast<long long>(s->stat_upgrade_post_2view),
        static_cast<long long>(s->stat_upgrade_post_lt3),
        s->upgrade_theta_p50_post_deg,
        static_cast<long long>(s->stat_enrich_targeted_tracks),
        static_cast<long long>(s->stat_enrich_targeted_scored),
        static_cast<long long>(s->stat_frag_links),
        static_cast<long long>(s->stat_frag_attempted),
        static_cast<long long>(s->stat_frag_accepted),
        static_cast<long long>(s->stat_frag_obs_merged),
        static_cast<long long>(s->stat_frag_reject_reproj),
        static_cast<long long>(s->stat_frag_reject_theta),
        static_cast<long long>(s->stat_frag_reject_min_theta),
        static_cast<long long>(s->stat_frag_reject_degenerate),
        static_cast<long long>(s->frag_ms),
        static_cast<long long>(s->stat_frag_pre_npts),
        static_cast<long long>(s->stat_frag_pre_2view),
        static_cast<long long>(s->stat_frag_pre_lt3),
        s->frag_theta_p50_pre_deg,
        static_cast<long long>(s->stat_frag_post_npts),
        static_cast<long long>(s->stat_frag_post_2view),
        static_cast<long long>(s->stat_frag_post_lt3),
        s->frag_theta_p50_post_deg,
        static_cast<long long>(s->stat_guided_temporal_attempted),
        static_cast<long long>(s->stat_guided_temporal_upgraded),
        static_cast<long long>(s->stat_guided_temporal_extra_inliers),
        solver_used.c_str(), sparse_backend.c_str(), dense_backend.c_str(),
        s->stat_phase2_gftol, s->stat_phase2_gref, s->stat_phase2_giter,
        s->stat_phase2_ftol_ab_arm, mixed, threads);
    if (n <= 0 || n >= static_cast<int>(sizeof(buf))) return;
    FILE* f = std::fopen(tmp.string().c_str(), "w");
    if (!f) return;
    const size_t written = std::fwrite(buf, 1, static_cast<size_t>(n), f);
    std::fclose(f);
    if (written != static_cast<size_t>(n)) {
      std::remove(tmp.string().c_str());
      return;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, dst, ec);                              
    if (ec) std::remove(tmp.string().c_str());
  } catch (...) {
                                                       
  }
}

                                                                                      
                                                                        
                                                                               
                                                                              
                                                                              
                                                                         
                                                                             
                                                                        
                                                                         
                                                                              
                                                                            
                                                                            
                                                                    
                                                                            
                                                                           
                                                                             
                                                                
                                                                                
                                                                          
                                                                                       
                                                                              
                                                      
void WriteL1PlanSidecars(aether_sfm_session* s,
                         const colmap::Reconstruction& recon,
                         const std::vector<double>& cloud_xyz,
                         const std::vector<uint8_t>& flags,
                         const aether_ghost::GhostMaskStats& st,
                         const aether_ghost::GhostMaskIntermediates& inter) {
  const double t0 = NowMs();
  const std::filesystem::path dir =
      std::filesystem::path(s->db_path).parent_path();
                                                                            
                                                                       
                                                                       
  std::unordered_map<int, std::string> jpeg_of;
  {
    std::ifstream jf(dir / "official_sfm_fed_frames.jsonl");
    std::string line;
    while (std::getline(jf, line)) {
      const size_t fpos = line.find("\"frameId\":");
      const size_t jpos = line.find("\"jpegPath\":\"");
      if (fpos == std::string::npos || jpos == std::string::npos) continue;
      const int fid = std::atoi(line.c_str() + fpos + 10);
      std::string path;
      for (size_t i = jpos + 12; i < line.size() && line[i] != '"'; ++i) {
        if (line[i] == '\\' && i + 1 < line.size()) ++i;                     
        path.push_back(line[i]);
      }
      if (fid >= 0 && !path.empty()) jpeg_of[fid] = std::move(path);
    }
  }
  if (jpeg_of.empty()) {
    LOG(WARNING) << "[aether_sfm] l1_plan: official_sfm_fed_frames.jsonl missing/empty "
                    "— no JPEG mapping, plan skipped";
    return;
  }
                                                                       
                              
  std::vector<aether_l1::L1Frame> frames;
  std::unordered_map<colmap::image_t, int> frame_of_image;
  {
    std::vector<const colmap::Image*> imgs;
    for (const auto& [iid, image] : recon.Images()) {
      if (image.HasPose()) imgs.push_back(&image);
    }
    std::sort(imgs.begin(), imgs.end(),
              [](const colmap::Image* a, const colmap::Image* b) {
                return a->ImageId() < b->ImageId();
              });
    for (const colmap::Image* image : imgs) {
      int fid = -1;
      if (std::sscanf(image->Name().c_str(), "frame_%d.jpg", &fid) != 1) {
        continue;
      }
      aether_l1::L1Frame fr;
      fr.frame_id = fid;
      const auto jit = jpeg_of.find(fid);
      if (jit != jpeg_of.end()) fr.jpeg = jit->second;
      const Eigen::Matrix3x4d m = image->CamFromWorld().ToMatrix();
      for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c) fr.w2c[r * 4 + c] = m(r, c);
      fr.w2c[15] = 1.0;
      const colmap::Camera& cam = *image->CameraPtr();
      const double sx = static_cast<double>(aether_l1::kProcW) /
                        static_cast<double>(cam.width);
      const double sy = static_cast<double>(aether_l1::kProcH) /
                        static_cast<double>(cam.height);
      fr.k[0] = cam.FocalLengthX() * sx;
      fr.k[1] = cam.FocalLengthY() * sy;
      fr.k[2] = cam.PrincipalPointX() * sx;
      fr.k[3] = cam.PrincipalPointY() * sy;
      frame_of_image[image->ImageId()] = static_cast<int>(frames.size());
      frames.push_back(std::move(fr));
    }
  }
                                                                            
                                                                
  std::vector<double> pts;
  pts.reserve(cloud_xyz.size());
  {
    int32_t row = 0;
    for (const auto& [pid, pt] : recon.Points3D()) {
      pts.push_back(pt.xyz.x());
      pts.push_back(pt.xyz.y());
      pts.push_back(pt.xyz.z());
      for (const auto& el : pt.track.Elements()) {
        const auto it = frame_of_image.find(el.image_id);
        if (it != frame_of_image.end())
          frames[static_cast<size_t>(it->second)].obs.push_back(row);
      }
      ++row;
    }
  }
  int budget_ms = aether_l1::kDefaultBudgetMs;
  if (const char* b = std::getenv("OFFICIAL_AETHER_L1_BUDGET_MS")) {
    const int v = std::atoi(b);
    if (v > 0) budget_ms = v;
  }
  aether_l1::L1Plan plan;
  if (!aether_l1::BuildL1Plan(cloud_xyz.data(), flags.data(),
                              cloud_xyz.size() / 3, st.plane_n, st.plane_d,
                              pts.data(), pts.size() / 3, frames, budget_ms,
                              &plan)) {
    LOG(WARNING) << "[aether_sfm] l1_plan: BuildL1Plan failed";
    return;
  }
  if (plan.refs.empty()) {
                                                                      
    std::error_code ec;
    std::filesystem::remove(dir / "arbitration_plan.json", ec);
    std::filesystem::remove(dir / "arbitration_plan.bin", ec);
    std::filesystem::remove(dir / "arbitration_points.bin", ec);
    LOG(WARNING) << "[aether_sfm] l1_plan: no refs (marked_cells="
                 << plan.n_marked_cells << ") — plan not written";
    return;
  }
  const bool ok =
      aether_l1::WriteL1PlanJson((dir / "arbitration_plan.json").string(),
                                 plan, frames) &&
      aether_l1::WriteL1PlanBin((dir / "arbitration_plan.bin").string(), plan,
                                frames) &&
      aether_l1::WriteL1PointsSidecar(
          (dir / "arbitration_points.bin").string(), cloud_xyz.data(),
          flags.data(), cloud_xyz.size() / 3, inter.sd, inter.local_off);
  LOG(WARNING) << "[aether_sfm] l1_plan: refs=" << plan.refs.size() << "/"
               << plan.budget_refs << " marked_cells=" << plan.n_marked_cells
               << " cov2=" << plan.n_cells_cov2
               << " cov1=" << plan.n_cells_cov1
               << " cov0=" << plan.n_cells_cov0
               << " dropped_srcs=" << plan.n_refs_dropped_srcs
               << " missing_jpeg=" << plan.n_frames_missing_jpeg
               << " write=" << (ok ? "ok" : "FAILED") << " ("
               << static_cast<int>(NowMs() - t0) << "ms)";
}

void MaybeWriteGhostMask(aether_sfm_session* s,
                         const colmap::Reconstruction& recon) {
  const char* e = std::getenv("OFFICIAL_AETHER_GHOST_MASK");
  if (!e || e[0] != '1') return;
  try {
    const double t0 = NowMs();
    const auto& pts = recon.Points3D();
    std::vector<double> xyz;
    xyz.reserve(pts.size() * 3);
    for (const auto& [pid, pt] : pts) {
      xyz.push_back(static_cast<double>(static_cast<float>(pt.xyz.x())));
      xyz.push_back(static_cast<double>(static_cast<float>(pt.xyz.y())));
      xyz.push_back(static_cast<double>(static_cast<float>(pt.xyz.z())));
    }
    std::vector<uint8_t> flags;
    aether_ghost::GhostMaskStats st;
    aether_ghost::GhostMaskIntermediates inter;
    if (!aether_ghost::ComputeGhostMask(xyz.data(), xyz.size() / 3, &flags,
                                        &st, &inter)) {
      LOG(WARNING) << "[aether_sfm] ghost_mask: degenerate cloud ("
                   << xyz.size() / 3 << " pts) — no sidecar";
      return;
    }
    const std::filesystem::path dir =
        std::filesystem::path(s->db_path).parent_path();
    {
      const std::filesystem::path tmp = dir / "ghost_mask.bin.tmp";
      FILE* f = std::fopen(tmp.string().c_str(), "wb");
      if (!f) return;
      const size_t w = std::fwrite(flags.data(), 1, flags.size(), f);
      std::fclose(f);
      if (w != flags.size()) {
        std::remove(tmp.string().c_str());
        return;
      }
      std::error_code ec;
      std::filesystem::rename(tmp, dir / "ghost_mask.bin", ec);
      if (ec) {
        std::remove(tmp.string().c_str());
        return;
      }
    }
    char buf[768];
    const int n = std::snprintf(
        buf, sizeof(buf),
        "{\"version\":1,\"n_points\":%lld,\"n_region\":%lld,"
        "\"n_band15\":%lld,\"n_band10\":%lld,\"n_cell_ghost\":%lld,"
        "\"n_clean\":%lld,\"n_bim_cells\":%lld,\"n_floor_cells\":%lld,"
        "\"plane_n\":[%.17g,%.17g,%.17g],\"plane_d\":%.17g,"
        "\"pass_ms\":%lld,"
        "\"bits\":\"0:in_region,1:band15,2:cell_ghost,3:in_clean,"
        "4:band10\",\"order\":\"points3d_iteration==get_points\"}\n",
        static_cast<long long>(st.n_points),
        static_cast<long long>(st.n_region),
        static_cast<long long>(st.n_band15),
        static_cast<long long>(st.n_band10),
        static_cast<long long>(st.n_cell_ghost),
        static_cast<long long>(st.n_clean),
        static_cast<long long>(st.n_bim_cells),
        static_cast<long long>(st.n_floor_cells), st.plane_n[0], st.plane_n[1],
        st.plane_n[2], st.plane_d, static_cast<long long>(NowMs() - t0));
    if (n > 0 && n < static_cast<int>(sizeof(buf))) {
      const std::filesystem::path tmp = dir / "ghost_mask.json.tmp";
      FILE* f = std::fopen(tmp.string().c_str(), "w");
      if (f) {
        const size_t w = std::fwrite(buf, 1, static_cast<size_t>(n), f);
        std::fclose(f);
        std::error_code ec;
        if (w == static_cast<size_t>(n)) {
          std::filesystem::rename(tmp, dir / "ghost_mask.json", ec);
        }
        if (w != static_cast<size_t>(n) || ec)
          std::remove(tmp.string().c_str());
      }
    }
    LOG(WARNING) << "[aether_sfm] ghost_mask: n=" << st.n_points
                 << " region=" << st.n_region << " band15=" << st.n_band15
                 << " cell_ghost=" << st.n_cell_ghost
                 << " bim_cells=" << st.n_bim_cells << "/"
                 << st.n_floor_cells << " (" << static_cast<int>(NowMs() - t0)
                 << "ms)";
                                                                             
                                                                        
    WriteL1PlanSidecars(s, recon, xyz, flags, st, inter);
  } catch (const std::exception& ex) {
    LOG(WARNING) << "[aether_sfm] ghost_mask failed (swallowed): "
                 << ex.what();
  } catch (...) {
    LOG(WARNING) << "[aether_sfm] ghost_mask failed (swallowed)";
  }
}

                                                                              
                                                                          
                                                                          
                                                                    
bool ProbeDebtGrowEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_DEBT_GROW");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                         
                                                                             
                                                                           
                                                                          
                                                                           
                                                                     
bool ProbeDebtGrowArkitPose() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_DEBT_GROW_ARKIT_POSE");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                             
                                                                           
                                                                            
                                                                       
                                                                             
                                              
                                                                                          
                                                                                 
                                    
  
                                                                             
                                                                                
                                                      
                                                                                  
                                                                                 
                                                                          
                                                                              
                                        
                                                                             
                                                                             
                                                             
                                                                                
                                                                               
                           
  
                                                                                
                                                                             
                                  
                                                                                 
bool ProbeDebtGrowLiveEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_DEBT_GROW_LIVE");
    return !(e && e[0] == '0');
  }();
  return cached;
}

bool ProbeDebtGrowObs() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_DEBT_GROW_OBS");
    return e && e[0] == '1';
  }();
  return cached;
}

bool ProbeDebtGrowMerge() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_PROBE_DEBT_GROW_MERGE");
    return e && e[0] == '1';
  }();
  return cached;
}

                                                                           
                                                                           
          
  
                                                                                
                                                                       
                                                                           
                                                                          
                                                                             
                                                               
                                                                         
                                                                             
                                                                            
                                                                  
  
                                                                            
                                                                         
                                                                           
                                                                           
                                                                        
  
                                                                           
                                                    
                                                                          
                                                    
                                                                  
                                                                             
                                                                         
                                                                         
                                                                     
                                                                           
                                                                   
                                                            
void ApplyProbeDebtGrowth(aether_sfm_session* s,
                          colmap::Reconstruction* recon) {
  if (!s) return;
  if (s->probe_debt_grow.empty()) return;
  if (!recon || !ProbeDebtGrowEnabled() ||
      !SelfDevLiveTriangulationEnabled()) {
    s->probe_debt_grow.clear();
    return;
  }
  const double t0 = NowMs();
  try {
                                                                            
                                                                            
                           
    const double min_tri_angle_rad = LiveCreateTriMinAngleRad();
    const double max_create_reproj_px = LiveCreateMaxReprojPx();
    const double max_grow_reproj_px = LiveGrowMaxReprojPx();
    constexpr double kMaxMergeReprojPx = 8.0;
    constexpr bool kMergeRequireDisjointImages = false;
    const bool arkit_pose = ProbeDebtGrowArkitPose();
    const bool allow_merge = ProbeDebtGrowMerge();
    const bool allow_grow = ProbeDebtGrowObs();

                                                                            
                                                                            
                                                                           
                                                 
    std::sort(s->probe_debt_grow.begin(), s->probe_debt_grow.end(),
              [](const ProbeDebtGrowPair& a, const ProbeDebtGrowPair& b) {
                if (a.f != b.f) return a.f < b.f;
                return a.j < b.j;
              });

    std::unordered_set<PointIdPair, PointIdPairHash> merge_trials;
    std::unordered_set<colmap::point3D_t> touched;
    const int num_frames = static_cast<int>(s->frames.size());
    const int64_t points_before = static_cast<int64_t>(recon->NumPoints3D());
    const int64_t grown_before = s->stat_grow_accepted;
    for (const ProbeDebtGrowPair& e : s->probe_debt_grow) {
      if (e.j < 0 || e.f < 0 || e.j >= num_frames || e.f >= num_frames) {
        continue;
      }
      const FrameRecord& f1 = s->frames[e.j];
      const FrameRecord& f2 = s->frames[e.f];
                                                                         
      if (f1.image_id == 0 || f2.image_id == 0) continue;
      if (!f1.has_pose || !f2.has_pose) continue;
      if (f1.points.empty() || f2.points.empty()) continue;
      if (!recon->ExistsImage(f1.image_id) ||
          !recon->ExistsImage(f2.image_id)) {
        continue;
      }
      const colmap::Image& im1 = recon->Image(f1.image_id);
      const colmap::Image& im2 = recon->Image(f2.image_id);
      if (!im1.HasPose() || !im2.HasPose()) continue;
                                                                        
                                                                               
      if (im1.NumPoints2D() != f1.points.size() ||
          im2.NumPoints2D() != f2.points.size()) {
        continue;
      }
      const colmap::Rigid3d pose1 =
          arkit_pose ? f1.cam_from_world : im1.CamFromWorld();
      const colmap::Rigid3d pose2 =
          arkit_pose ? f2.cam_from_world : im2.CamFromWorld();
      const LiveGrowView v1{f1.image_id, &f1.points, &f1.camera, pose1};
      const LiveGrowView v2{f2.image_id, &f2.points, &f2.camera, pose2};
      GrowLiveTracksFromTvgInliers(
          s, recon, v1, v2, e.inliers, min_tri_angle_rad,
          max_create_reproj_px, max_grow_reproj_px, kMaxMergeReprojPx,
          kMergeRequireDisjointImages, allow_merge, allow_grow, &merge_trials,
          &touched);
      ++s->stat_probe_debt_grow_pairs;
    }
    s->stat_probe_debt_grow_added =
        static_cast<int64_t>(recon->NumPoints3D()) - points_before;
    s->stat_probe_debt_grow_grown = s->stat_grow_accepted - grown_before;
    s->stat_probe_debt_grow_touched = static_cast<int64_t>(touched.size());
  } catch (const std::exception& ex) {
                                                                              
    LOG(WARNING) << "[aether_sfm] probe-debt growth aborted: " << ex.what();
  } catch (...) {
    LOG(WARNING) << "[aether_sfm] probe-debt growth aborted";
  }
  s->stat_probe_debt_grow_ms = NowMs() - t0;
  s->probe_debt_grow.clear();
  LOG(WARNING) << "[aether_sfm] probe-debt growth: pairs="
               << s->stat_probe_debt_grow_pairs
               << " points_added=" << s->stat_probe_debt_grow_added
               << " obs_grown=" << s->stat_probe_debt_grow_grown
               << " touched=" << s->stat_probe_debt_grow_touched
               << " ms=" << s->stat_probe_debt_grow_ms;
}

                         
  
                                                                            
                                                                                
                                                                             
                                                                            
                                                                          
                                                        
                                                                                
                                                                     
                                                                             
                                                                           
                                                                           
                                                                             
                                                                               
                                                                            
                                                                          
                                                                              
                                                                               
                                                                                        
                                                                           
                                                                            
                                                                            
                                                         
  
                                                                               
                                                                             
                                                                            
                                                                           
void RefineGlobalBA(aether_sfm_session* s,
                    std::shared_ptr<colmap::Reconstruction> model,
                    bool live_reuse) {
  aether::official::ba::ScopedBaSessionAggregateBindingV1 ba_session_binding(
      &s->ba_ptol_aggregate, &s->ba_ptol_receipts);
                                                                                
                                                                            
                                                       
  {
    auto& ba_progress = aether::official::ba::GlobalBaProgressV1();
    ba_progress.stage.store(0, std::memory_order_relaxed);
    ba_progress.round.store(0, std::memory_order_relaxed);
    ba_progress.iteration.store(0, std::memory_order_relaxed);
    ba_progress.max_iterations.store(0, std::memory_order_relaxed);
  }
  try {
#if defined(__APPLE__)
                                                                             
                                                                               
                                                                               
                                                             
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
    const double t0 = NowMs();
    s->gpu_watchdog.InProgress();                                               
                                                                          
                                                                            
                                                                             
    try {
      std::filesystem::remove(std::filesystem::path(s->db_path).parent_path() /
                              "official_finalize_segments.json");
    } catch (...) {
    }
    auto popts = MakePhase2Options(s);
                                                                               
                                                                                   
                                                                                   
                                                                                    
                                                                                    
                                                                                   
                                                                              
                                                                              
                                                                                 
                                                                                     
                                                                                  
                                                                       
    if (s->incremental_global_ba_ran) {
      const int cap = std::max(1, IncrementalFinalizeRounds());
      const int before = popts->ba_global_max_refinements;
      popts->ba_global_max_refinements = std::min(before, cap);
      LOG(WARNING) << "[aether_sfm] finalize collapse: incremental global BA ran ("
                   << s->stat_incremental_refines << " refines) → global-BA rounds "
                   << before << " → " << popts->ba_global_max_refinements;
    }
    std::shared_ptr<colmap::Reconstruction> refined;
    double enrich_ms = 0.0, cache_pre_ms = 0.0, stage1_ms = 0.0;
    int stage1_rounds = 0;
    const char* stage1_state = "off";
                                                                          
                                                                              
                                                                            
                                                                             
                                                                              
                                                                       
                                                                       
                                                                              
                                                                           
                                                                            
    std::unordered_set<colmap::point3D_t> live_origin_ids;
    if (live_reuse && model) {
      live_origin_ids.reserve(model->NumPoints3D() * 2);
      for (const auto& [pid, _] : model->Points3D()) live_origin_ids.insert(pid);
    }
    if (live_reuse) {
      refined = std::move(model);                                               
                                                                             
                                                                             
                                                                               
                                                                      
                                         
      if (EnrichTargetedEnabled() && s->frames.size() >= 3) {
        try {
          s->enrich_hint = BuildEnrichTargetHint(*s, *refined);
        } catch (...) {
          s->enrich_hint.reset();
        }
      }
                                                                                     
                                                                           
      static const bool no_overlap = [] {
        const char* e = std::getenv("OFFICIAL_AETHER_FINALIZE_NO_OVERLAP");
        return e && e[0] == '1';
      }();
                                                                           
      std::shared_ptr<colmap::DatabaseCache> cache_pre;
      if (!no_overlap && s->db && s->frames.size() >= 2) {
        try {
          const double t_cache = NowMs();
          colmap::DatabaseCache::Options copts;
          copts.min_num_matches =
              static_cast<size_t>(popts->min_num_matches);
          copts.load_all_images = true;                          
          cache_pre = colmap::DatabaseCache::Create(*s->db, copts);
          cache_pre_ms = NowMs() - t_cache;
        } catch (const std::exception& e) {
          cache_pre.reset();
          LOG(WARNING) << "[aether_sfm] finalize stage-1 cache snapshot failed"
                          " ("
                       << e.what() << ") — no overlap, serial refinement";
        }
      }
                                                                             
                                                                            
                                                                               
                                                                              
                                                                            
                                                                               
                                                                              
                                                                           
      std::atomic<bool> enrich_done{false};
                                                                          
                                                                           
                                                                            
                                                                           
      const bool budget_armed = EnrichBudgetArmed(cache_pre != nullptr);
      s->enrich_stage1_done.store(false, std::memory_order_relaxed);
      s->enrich_start_ms = 0.0;
      const double t_enrich0 = NowMs();
      std::thread enrich([s, &enrich_ms, &enrich_done, t_enrich0,
                          budget_armed] {
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
        s->enrich_start_ms = NowMs();
        try {
                                                                            
                                                                            
                                                                          
                                                                             
                                                                            
                                                                                
                                                                         
                                                                                
                                                                               
                                                                              
                                                                           
                             
                                                                          
                                                                                
                                                                             
                                                                             
          double rematch_ms = 0, quad_ms = 0, spatial_ms = 0;
          if (budget_armed) {
            s->gpu_watchdog.InProgress();
            const double t_r = NowMs();
            FinalizeRematchStarvedFrames(s);
            rematch_ms = NowMs() - t_r;
            s->gpu_watchdog.InProgress();
            const double t_q = NowMs();
            AddOfficialQuadraticPairs(s);
            quad_ms = NowMs() - t_q;
            s->gpu_watchdog.InProgress();
            const double t_sp = NowMs();
            AddSpatialRevisitMatches(s);
            spatial_ms = NowMs() - t_sp;
            s->gpu_watchdog.InProgress();
          } else {
            s->gpu_watchdog.InProgress();
            const double t_q = NowMs();
            AddOfficialQuadraticPairs(s);
            quad_ms = NowMs() - t_q;
            s->gpu_watchdog.InProgress();
            const double t_sp = NowMs();
            AddSpatialRevisitMatches(s);
            spatial_ms = NowMs() - t_sp;
            s->gpu_watchdog.InProgress();
            const double t_r = NowMs();
            FinalizeRematchStarvedFrames(s);
            rematch_ms = NowMs() - t_r;
            s->gpu_watchdog.InProgress();
          }
          {
            char eline[256];
            std::snprintf(eline, sizeof(eline),
                          "{\"t\":%lld,\"type\":\"enrich_split\","
                          "\"armed\":%d,\"rematch_ms\":%.0f,"
                          "\"quadratic_ms\":%.0f,\"spatial_ms\":%.0f}",
                          static_cast<long long>(EpochMs()),
                          budget_armed ? 1 : 0, rematch_ms, quad_ms,
                          spatial_ms);
            AppendMatchFailJsonl(s, eline);
          }
        } catch (const std::exception& e) {
          LOG(WARNING) << "[aether_sfm] finalize db enrichment aborted: "
                       << e.what();
        } catch (...) {
          LOG(WARNING) << "[aether_sfm] finalize db enrichment aborted";
        }
        try {
          if (s->db) s->db->Close();                                    
        } catch (...) {
        }
        enrich_ms = NowMs() - t_enrich0;
        enrich_done.store(true);
      });
                                                                          
                                                                     
                                                                          
                                                                           
                                                                             
                                                                            
                                                                           
                                                                              
                          
      if (cache_pre) {
        const double t_s1 = NowMs();
#if defined(__APPLE__)
                                                                      
                                                                              
                                                                        
                                                                            
                                                                         
                                                                              
                                                     
        if (Stage1UtilityQosLegacy()) {
          pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
        }
#endif
        aether::official::ba::ResetActiveBaSessionReceiptRingV1();
        try {
          colmap::IncrementalMapper mapper(cache_pre);
          mapper.BeginReconstruction(refined);
          const auto mapper_opts = popts->Mapper();
          auto ba_opts = popts->GlobalBundleAdjustment();
          const auto tri_opts = popts->Triangulation();
                                                                              
                                                                          
                                                                             
                                                                                 
                                                                             
                                                                     
                                                                        
                                                                              
                                                                       
                                   
          if (ba_opts.ceres) {
            const int full =
                ba_opts.ceres->solver_options.num_threads > 0
                    ? ba_opts.ceres->solver_options.num_threads
                    : static_cast<int>(std::thread::hardware_concurrency());
                                                                              
                                                                              
                                                                            
                                                                           
                                                                
            ba_opts.ceres->solver_options.num_threads =
                Stage1HalfThreadsLegacy() ? std::max(1, full / 2) : full;
                                                                              
                                                                         
            if (Stage1FtolOverride() > 0.0) {
              ba_opts.ceres->solver_options.function_tolerance =
                  Stage1FtolOverride();
            }
                                                                           
                                                                           
                                                                             
                                                                             
                                                      
            if (const char* e =
                    std::getenv("OFFICIAL_AETHER_BA_S1_MAX_ITERS")) {
              const int v = std::atoi(e);
              if (v > 0) ba_opts.ceres->solver_options.max_num_iterations = v;
            }
          }
                                                                             
                                                                           
                                                                             
                                 
          if (!enrich_done.load()) mapper.CompleteAndMergeTracks(tri_opts);
          if (!enrich_done.load()) mapper.Retriangulate(tri_opts);
                                                                             
                                                                             
                                                                           
                                                               
          const int s1_rounds_max =
              Stage1RoundsCap() > 0
                  ? std::min(popts->ba_global_max_refinements,
                             Stage1RoundsCap())
                  : popts->ba_global_max_refinements;
          for (int i = 0; i < s1_rounds_max; ++i) {
            if (enrich_done.load()) break;                  
            s->gpu_watchdog.InProgress();                                       
            {                                                  
              auto& ba_progress = aether::official::ba::GlobalBaProgressV1();
              ba_progress.stage.store(1, std::memory_order_relaxed);
              ba_progress.round.store(i + 1, std::memory_order_relaxed);
            }
            const size_t num_obs = refined->ComputeNumObservations();
            // [AETHER-T1] stage-1 per-part attribution (our own loop).
            const double t_s1ba = NowMs();
            {
              aether::official::ba::ScopedGlobalBaSolveV1 global_ba_scope;
              mapper.AdjustGlobalBundle(mapper_opts, ba_opts);
            }
            s->t1_stage1_ba_ms += NowMs() - t_s1ba;
            const double t_s1mg = NowMs();
            size_t num_changed = mapper.CompleteAndMergeTracks(tri_opts);
            num_changed += mapper.FilterPoints(mapper_opts);
            s->t1_stage1_merge_ms += NowMs() - t_s1mg;
            ++stage1_rounds;
            const double changed =
                num_obs == 0 ? 0
                             : static_cast<double>(num_changed) / num_obs;
            if (changed < popts->ba_global_max_refinement_change) break;
          }
          mapper.EndReconstruction(            false);
          stage1_state = "ok";
        } catch (const std::exception& e) {
          stage1_state = "failed";
          stage1_rounds = 0;
          LOG(WARNING) << "[aether_sfm] finalize stage-1 refine failed ("
                       << e.what()
                       << ") — stage 2 runs the full refinement alone";
        } catch (...) {
          stage1_state = "failed";
          stage1_rounds = 0;
          LOG(WARNING) << "[aether_sfm] finalize stage-1 refine failed — "
                          "stage 2 runs the full refinement alone";
        }
        stage1_ms = NowMs() - t_s1;
        s->gpu_watchdog.InProgress();                                       
        AppendBaRingJsonl(s, 1);                                             
                                                                              
                                                                             
                                                                             
                                                                          
        s->enrich_stage1_done.store(true, std::memory_order_relaxed);
        cache_pre.reset();                                                   
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
      }
                                                                             
                                                                    
      const double t_join = NowMs();
      enrich.join();
      s->t1_enrich_gate_wait_ms = NowMs() - t_join;
      s->enrich_hint.reset();                                            
                                                                         
                                                                              
                                                                        
                                                                      
                                                                           
                                                                         
      ApplyProbeDebtGrowth(s, refined.get());
                                                                          
                                                                                
                                                                             
                                                                           
                                                                              
                                                                                 
      if (stage1_rounds > 0) {
        popts->ba_global_max_refinements =
            std::max(1, popts->ba_global_max_refinements - stage1_rounds);
      }
    } else {
                                                                                
      refined = std::make_shared<colmap::Reconstruction>(*model);
                                                                            
                                                                             
                                                                          
      s->probe_debt_grow.clear();
    }
                                                                                
                                                                      
    const double t_s2 = NowMs();
                                                                              
    aether_igr_pre_ms = 0.0;
    aether_igr_ba_ms = 0.0;
    aether_igr_merge_ms = 0.0;
    aether_igr_rounds = 0;
    aether::official::ba::ResetActiveBaSessionReceiptRingV1();
    {                                                                             
                                                                                   
                                   
      auto& ba_progress = aether::official::ba::GlobalBaProgressV1();
      ba_progress.stage.store(2, std::memory_order_relaxed);
      ba_progress.round.store(0, std::memory_order_relaxed);
    }
                                                                                        
                                                                           
    const auto gravity_ra_snapshot =
        MaybeRunFinalizeGravityRa(s->db_path, refined.get());
    auto manager = std::make_shared<colmap::ReconstructionManager>();
    colmap::IncrementalPipeline pipeline(
        popts, colmap::Database::Open(s->db_path), manager);
    s->gpu_watchdog.InProgress();                                       
                                                                                            
                                                                                         
                                                                                             
                                                                                                   
                                                                                                      
                                                                                                        
                                                                                            
                                         
    if (s->stat_pose_direct_attempted > 0 ||
        s->stat_guided_temporal_attempted > 0) {
      LOG(WARNING) << "[guided-chain] rung1_attempted="
                   << s->stat_guided_temporal_attempted
                   << " rung1_upgraded=" << s->stat_guided_temporal_upgraded
                   << " rung1_extra_inliers="
                   << s->stat_guided_temporal_extra_inliers
                   << " | rung2_attempted=" << s->stat_pose_direct_attempted
                   << " rung2_rescued=" << s->stat_pose_direct_rescued
                   << " rung2_inliers=" << s->stat_pose_direct_inliers;
    }
    std::unordered_map<colmap::frame_t, std::pair<size_t, std::string>>
        diag_before;
    for (const auto& [frame_id, frame] : refined->Frames()) {
      if (!frame.HasPose()) continue;
      size_t obs = 0;
      std::string img_name;
      for (const auto& data_id : frame.ImageIds()) {
        if (!refined->ExistsImage(data_id.id)) continue;
        const auto& im = refined->Image(data_id.id);
        obs += im.NumPoints3D();
        if (img_name.empty()) img_name = im.Name();
      }
      diag_before.emplace(frame_id, std::make_pair(obs, img_name));
    }
    pipeline.RefineReconstruction(refined);
    {
      int lost_zero_obs = 0, lost_with_obs = 0;
      std::string lost_detail;
      for (const auto& [frame_id, before] : diag_before) {
        const bool still =
            refined->ExistsFrame(frame_id) && refined->Frame(frame_id).HasPose();
        if (still) continue;
        (before.first == 0 ? lost_zero_obs : lost_with_obs)++;
        if (lost_detail.size() < 400) {
          lost_detail += " " + before.second + "(obs=" +
                         std::to_string(before.first) + ")";
        }
      }
      LOG(WARNING) << "[frame-loss] before=" << diag_before.size()
                   << " after=" << refined->NumRegFrames()
                   << " lost_zero_obs=" << lost_zero_obs
                   << " lost_with_obs=" << lost_with_obs << " |" << lost_detail;
    }
                                                                                     
                                                        
    MaybeReanchorAfterGravityRa(gravity_ra_snapshot, refined.get());
    s->gpu_watchdog.InProgress();                                       
    const double stage2_ms = NowMs() - t_s2;
                                                                                  
                                                           
    {
      char sline[320];
      std::snprintf(
          sline, sizeof(sline),
          "{\"t\":%lld,\"type\":\"finalize_split\",\"stage1_ba_ms\":%.0f,"
          "\"stage1_merge_ms\":%.0f,\"enrich_gate_wait_ms\":%.0f,"
          "\"s2_pre_ms\":%.0f,\"s2_ba_ms\":%.0f,\"s2_merge_ms\":%.0f,"
          "\"s2_rounds\":%d,\"s2_other_ms\":%.0f}",
          static_cast<long long>(EpochMs()), s->t1_stage1_ba_ms,
          s->t1_stage1_merge_ms, s->t1_enrich_gate_wait_ms, aether_igr_pre_ms,
          aether_igr_ba_ms, aether_igr_merge_ms, aether_igr_rounds,
          stage2_ms - aether_igr_pre_ms - aether_igr_ba_ms -
              aether_igr_merge_ms);
      AppendMatchFailJsonl(s, sline);
    }
    AppendBaRingJsonl(s, 2);                                             
    AppendBaSessionAggregateJsonl(s, "post_stage2");
                                                                         
    if (s->stat_apvba_kicks > 0) {
      char aline[512];
      std::snprintf(aline, sizeof(aline),
                    "{\"t\":%lld,\"type\":\"apvba_summary\","
                    "\"kicks\":%lld,\"merges\":%lld,\"dropped\":%lld,"
                    "\"last_ba_ms\":%.0f,\"merge_ms_total\":%.0f,"
                    "\"copy_ms_total\":%.0f,\"corr_mm\":%.2f,"
                    "\"corr_deg\":%.3f,\"propagate\":%d,\"match_cadence\":%d,"
                    "\"param_only\":%d,\"last_error\":\"%s\"}",
                    static_cast<long long>(EpochMs()),
                    (long long)s->stat_apvba_kicks,
                    (long long)s->stat_apvba_merges,
                    (long long)s->stat_apvba_dropped,
                    s->stat_apvba_last_ba_ms, s->stat_apvba_merge_ms_total,
                    s->stat_apvba_copy_ms_total, s->stat_apvba_last_corr_mm,
                    s->stat_apvba_last_corr_deg,
                    AsyncPreviewBaPropagate() ? 1 : 0,
                    AsyncPreviewBaMatchCadence() ? 1 : 0,
                    AsyncPreviewBaParamOnly() ? 1 : 0,
                    JsonSafe(s->apvba_last_error.c_str(), 96).c_str());
      AppendMatchFailJsonl(s, aline);
    }
    const double t_td = NowMs();
    RestoreTemporalDetail(s, refined.get());
    s->gpu_watchdog.InProgress();                                               
    const double temporal_ms = NowMs() - t_td;
                                                                               
                                                                               
    UpgradeLowParallaxTracks(s, refined.get());
                                                                        
                                                                   
    MergeFragmentTracks(s, refined.get());
    s->gpu_watchdog.InProgress();                                                    
                                                                            
                                                                             
                                                                         
    MaybeWriteGhostMask(s, *refined);
    LOG(WARNING) << "[aether_sfm] finalize worker: cache_pre="
                 << static_cast<int64_t>(cache_pre_ms)
                 << "ms enrich=" << static_cast<int64_t>(enrich_ms)
                 << "ms stage1=" << static_cast<int64_t>(stage1_ms) << "ms ("
                 << stage1_state << ", rounds=" << stage1_rounds
                 << ") stage2=" << static_cast<int64_t>(stage2_ms)
                 << "ms (rounds<=" << popts->ba_global_max_refinements
                 << ") temporal=" << static_cast<int64_t>(temporal_ms)
                 << "ms upgrade=" << static_cast<int64_t>(s->upgrade_ms)
                 << "ms total=" << static_cast<int64_t>(NowMs() - t0) << "ms";
                                                                             
                                                                                 
                                                                    
    WriteFinalizeSegments(s, live_reuse, cache_pre_ms, enrich_ms, stage1_ms,
                          stage1_rounds, stage1_state, stage2_ms,
                          popts->ba_global_max_refinements, temporal_ms,
                          NowMs() - t0);
                                                                             
                                                                            
                                                                 
                                                     
    if (live_reuse && refined) {
      int64_t from_live = 0, from_official = 0;
      for (const auto& [pid, _] : refined->Points3D()) {
        if (live_origin_ids.count(pid)) ++from_live;
        else ++from_official;
      }
      const int64_t live_dropped =
          static_cast<int64_t>(live_origin_ids.size()) - from_live;
      LOG(WARNING) << "[aether_sfm] delivered-point provenance: live-created="
                   << from_live << " official-retriangulated=" << from_official
                   << " live-dropped-by-official-filtering=" << live_dropped
                   << " (live model had " << live_origin_ids.size()
                   << "; capture-time official TriangulateImage added "
                   << s->stat_official_tri_added << " obs)";
      char pline[256];
      std::snprintf(pline, sizeof(pline),
                    "{\"t\":%lld,\"type\":\"point_provenance\",\"live\":%lld,"
                    "\"official\":%lld,\"live_dropped\":%lld,\"live_total\":%zu}",
                    static_cast<long long>(EpochMs()), (long long)from_live,
                    (long long)from_official, (long long)live_dropped,
                    live_origin_ids.size());
      AppendMatchFailJsonl(s, pline);
    }
    {
      std::lock_guard<std::mutex> lk(s->recon_mutex);
      s->recon = refined;
      s->refine_ms = NowMs() - t0;
    }
                                                                                
    aether::official::ba::GlobalBaProgressV1().stage.store(
        0, std::memory_order_relaxed);
    s->finalize_status.store(2);                                
  } catch (const std::exception& e) {
    RefineFailClosed(s, e.what());
  } catch (...) {
    RefineFailClosed(s, "unknown exception");
  }
}

}              

extern "C" {

void aether_sfm_options_default(aether_sfm_options_t* out) {
  if (!out) return;
  out->max_features = 2048;
  out->image_width = 0;
  out->image_height = 0;
  out->match_max_ratio = 0.8f;
  out->use_gpu_match = 0;                                                       
  out->k_neighbors = 6;
  out->use_gpu_extract = 0;                                                 
}

const char* aether_sfm_result_str(aether_sfm_result_t code) {
  switch (code) {
    case AETHER_SFM_OK: return "AETHER_SFM_OK";
    case AETHER_SFM_ERR_INVALID_ARG: return "AETHER_SFM_ERR_INVALID_ARG";
    case AETHER_SFM_ERR_DB: return "AETHER_SFM_ERR_DB";
    case AETHER_SFM_ERR_EXTRACT: return "AETHER_SFM_ERR_EXTRACT";
    case AETHER_SFM_ERR_NO_INITIAL_PAIR: return "AETHER_SFM_ERR_NO_INITIAL_PAIR";
    case AETHER_SFM_ERR_NOT_REGISTERED: return "AETHER_SFM_ERR_NOT_REGISTERED";
    case AETHER_SFM_ERR_INTERNAL: return "AETHER_SFM_ERR_INTERNAL";
    case AETHER_SFM_ERR_UNSUPPORTED: return "AETHER_SFM_ERR_UNSUPPORTED";
  }
  return "AETHER_SFM_ERR_UNKNOWN";
}

                                                                                                                                                                   
aether_sfm_result_t aether_sfm_run(const char* db_path, const char* image_path,
                                   const aether_sfm_options_t* options,
                                   aether_sfm_session_t** out_session,
                                   char* out_json, int out_cap) {
  if (!db_path || !image_path) return AETHER_SFM_ERR_INVALID_ARG;

  aether_sfm_options_t opts;
  if (options) {
    opts = *options;
  } else {
    aether_sfm_options_default(&opts);
  }

  std::shared_ptr<colmap::ReconstructionManager> manager;
  std::shared_ptr<const colmap::Reconstruction> recon;
                                                                            
                                                                              
                                                                          
  aether::official::ba::BaSessionAggregateAccumulatorV1 batch_ptol_aggregate;
  aether::official::ba::BaSessionReceiptRingV1 batch_ptol_receipts;
                                                                              
                                                                            
                                     
  aether_ba_clear_gravity_priors();
  const aether_sfm_result_t rc = RunIncremental(
      db_path, image_path, opts, &batch_ptol_aggregate,
      &batch_ptol_receipts, &manager, &recon, out_json, out_cap);
  const auto persist_temporary_batch_receipts = [&](const char* reason) {
    auto receipt_sink = std::make_unique<aether_sfm_session>();
    receipt_sink->db_path = db_path;
    receipt_sink->ba_ptol_aggregate.Replace(batch_ptol_aggregate.Snapshot());
    receipt_sink->ba_ptol_receipts.Replace(batch_ptol_receipts.Snapshot());
    AppendBaRingJsonl(receipt_sink.get(), -1);
    AppendBaSessionAggregateJsonl(receipt_sink.get(), reason);
  };
  if (rc != AETHER_SFM_OK) {
    persist_temporary_batch_receipts("batch_error");
    return rc;
  }

  if (out_session) {
    auto* s = new aether_sfm_session();
    aether_preclamp_instr_v1::ResetOfficialSessionRecords(s);
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
    if (DescriptorResidencyEnabledCore()) {
      s->descriptor_residency_nonce = NewDescriptorResidencySessionNonce();
    }
#endif
    s->options = opts;
    s->ba_ptol_aggregate.Replace(batch_ptol_aggregate.Snapshot());
    s->ba_ptol_receipts.Replace(batch_ptol_receipts.Snapshot());
    s->db_path = db_path;
    s->image_path = image_path;
    s->owns_db_file = false;                                          
    s->recon_manager = manager;
    s->recon = recon;
    *out_session = s;
  } else {
                                                                             
                                                                              
                                                                           
    persist_temporary_batch_receipts("batch_no_session");
  }
  return AETHER_SFM_OK;
}

                                                                                     
                                                                                       
                                                                                 
                                                                                              
aether_sfm_result_t aether_sfm_write_model(aether_sfm_session_t* s,
                                           const char* out_dir) {
  if (!s || !out_dir) return AETHER_SFM_ERR_INVALID_ARG;
  try {
    if (!s->recon) return AETHER_SFM_ERR_NOT_REGISTERED;
    std::filesystem::create_directories(out_dir);
    s->recon->Write(out_dir);
    return AETHER_SFM_OK;
  } catch (const std::exception& e) {
    LOG(ERROR) << "[aether_sfm] write_model failed: " << e.what();
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                                               
                                                                          
  
                                                                                          
                                                                                             
                                                                                                
  
                                                      
                                                                                  
                                                                                                      
aether_sfm_result_t aether_sfm_continue_from_model(
    const char* db_path, const char* image_path, const char* model_in_path,
    const char* model_out_path, const aether_sfm_options_t* options,
    aether_sfm_session_t** out_session, char* out_json, int out_cap) {
  if (!db_path || !image_path || !model_in_path) {
    return AETHER_SFM_ERR_INVALID_ARG;
  }

  aether_sfm_options_t opts;
  if (options) {
    opts = *options;
  } else {
    aether_sfm_options_default(&opts);
  }

  std::shared_ptr<colmap::ReconstructionManager> manager;
  std::shared_ptr<const colmap::Reconstruction> recon;
  aether::official::ba::BaSessionAggregateAccumulatorV1 batch_ptol_aggregate;
  aether::official::ba::BaSessionReceiptRingV1 batch_ptol_receipts;
                                                                                          
                                                        
  aether_ba_clear_gravity_priors();
  const aether_sfm_result_t rc = RunIncremental(
      db_path, image_path, opts, &batch_ptol_aggregate, &batch_ptol_receipts,
      &manager, &recon, out_json, out_cap,                false,
                          std::string(model_in_path));
  if (rc != AETHER_SFM_OK) return rc;

  if (model_out_path && *model_out_path && recon) {
    try {
      std::filesystem::create_directories(model_out_path);
      recon->Write(model_out_path);
    } catch (const std::exception& e) {
                                                                                   
      LOG(ERROR) << "[aether_sfm] continue: model write failed: " << e.what();
      return AETHER_SFM_ERR_INTERNAL;
    }
  }

  if (out_session) {
    auto* s = new aether_sfm_session();
    aether_preclamp_instr_v1::ResetOfficialSessionRecords(s);
    s->options = opts;
    s->ba_ptol_aggregate.Replace(batch_ptol_aggregate.Snapshot());
    s->ba_ptol_receipts.Replace(batch_ptol_receipts.Snapshot());
    s->db_path = db_path;
    s->image_path = image_path;
    s->owns_db_file = false;                                            
    s->recon_manager = manager;
    s->recon = recon;
    *out_session = s;
  }
  return AETHER_SFM_OK;
}

aether_sfm_result_t aether_sfm_run_dir(const char* capture_dir,
                                       const aether_sfm_options_t* options,
                                       aether_sfm_session_t** out_session,
                                       char* out_json, int out_cap) {
                                                                          
                                                                                    
                                                                              
                                                                              
                                                                                
                                                                               
                                                          
  (void)capture_dir;
  (void)options;
  (void)out_session;
  if (out_json && out_cap > 0) {
    std::snprintf(out_json, out_cap,
                  "{\"error\":\"run_dir not implemented: drive create/"
                  "add_frame/finalize with platform-decoded frames\"}");
  }
  return AETHER_SFM_ERR_UNSUPPORTED;
}

                                                                                                                                                                                         
aether_sfm_result_t aether_sfm_create(const char* db_path,
                                      const aether_sfm_options_t* options,
                                      aether_sfm_session_t** out_session) {
  if (!db_path || !out_session) return AETHER_SFM_ERR_INVALID_ARG;
  try {
    aether_ba_clear_gravity_priors();
    auto* s = new aether_sfm_session();
    aether_preclamp_instr_v1::ResetOfficialSessionRecords(s);
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
    if (DescriptorResidencyEnabledCore()) {
      s->descriptor_residency_nonce = NewDescriptorResidencySessionNonce();
    }
#endif
    s->gpu_timestamp_run_id = NewGpuTimestampRunId();
    if (options) {
      s->options = *options;
    } else {
      aether_sfm_options_default(&s->options);
    }
                                                                              
                                                                            
                                                                              
    s->live_recon = std::make_shared<colmap::Reconstruction>();
    s->db_path = db_path;
                                                                              
                                                                             
                                                      
    s->owns_db_file = false;
    s->db = colmap::Database::Open(db_path);
    if (!s->db) {
      delete s;
      return AETHER_SFM_ERR_DB;
    }
    TailCacheInitialize(s);
                                                                             
                                                                            
                                                                                        
                                                                           
    {
      char bline[384];
      std::snprintf(bline, sizeof(bline),
                    "{\"t\":%lld,\"type\":\"build_stamp\","
                    "\"built\":\"%s %s\","
                    "\"diag_contract\":\"PW_LIVE_CLOUD_DIAG_V1_STREAM\","
                    "\"snapshot_diag_contract\":"
                    "\"PW_LIVE_CLOUD_SNAPSHOT_DIAG_V2_STREAM\","
                    "\"native_diag_build_id\":"
                    "\"PW_LIVE_CLOUD_DIAG_NATIVE_V2_RECONSTRUCTION_SNAPSHOT\","
                    "\"observation_only\":true}",
                    static_cast<long long>(EpochMs()), __DATE__, __TIME__);
      AppendMatchFailJsonl(s, bline);
    }
                                                                                               
                                                                                           
    {
      std::lock_guard<std::mutex> lk(g_warmup_mu);
      if (g_warmup_db_path.empty()) g_warmup_db_path = s->db_path;
      FlushWarmupLineLocked();
    }
    KickExtractWarmupOnce("create");
                                                                                    
                                                                                         
                                                                        
    s->gpu_watchdog.Start([s](const std::string& line) {
      AppendMatchFailJsonl(s, line);
    });
    *out_session = s;
    return AETHER_SFM_OK;
  } catch (const std::exception&) {
    return AETHER_SFM_ERR_DB;
  }
}

                                                                         
                                                                                          
                                                                                 
                                                                              
                                                                               
                                                                                
                                                                                  
  
                                                                                  
                                                                                      
                                                                                  
                                                                                  
                                                          
  
                                                                              
                                                                               
                                                                                 
                                                                                    
                                                                                   
                                                                                
                                                                                    
                                                                    
  
                                                                                    
                                                                                
                                                                                   
                                                                                 
                                                                                    
                                                                     
                                                                                  
                                                                                    
                                                                               
                                                                                 
                                                                              
                                                                               
                                                                                   
                                                                                   
static void MaybeIncrementalGlobalRefine(aether_sfm_session* s) {
  if (!IncrementalGlobalBaEnabled()) return;
  if (!s || !s->live_recon_ready || !s->live_recon) return;
  aether::official::ba::ScopedBaSessionAggregateBindingV1 ba_session_binding(
      &s->ba_ptol_aggregate, &s->ba_ptol_receipts);
  const int thermal_now = s->thermal_state.load(std::memory_order_relaxed);
  if (thermal_now >= 3) return;                                            
  const size_t n_reg = s->reg_order.size();
  if (n_reg < 3) return;
  const size_t every = static_cast<size_t>(IncrementalGlobalBaEveryN()) *
                       (thermal_now >= 2 ? 2 : 1);                                 
  if (n_reg < s->last_global_refine_reg + every) return;

  const double t0 = NowMs();
  try {
                                                                                  
    const size_t window = static_cast<size_t>(IncrementalGlobalBaWindow());
    const size_t free_from = n_reg > window ? n_reg - window : 0;
    colmap::BundleAdjustmentConfig cfg;
    size_t n_frozen = 0;
    for (size_t i = 0; i < n_reg; ++i) {
      const colmap::image_t img = s->reg_order[i];
      cfg.AddImage(img);
      if (i < free_from) {
                                                                                 
                                                                
        cfg.SetConstantRigFromWorldPose(s->live_recon->Image(img).FrameId());
        ++n_frozen;
      }
    }
    if (free_from == 0)                                                      
      cfg.FixGauge(colmap::BundleAdjustmentGauge::THREE_POINTS);
    SetAllCameraIntrinsicsConstant(*s->live_recon, s->reg_order, cfg);

                                                                                  
                                                                       
                                                                             
    const size_t kMaxObs =
        std::min<size_t>(400000, std::max<size_t>(80000, n_reg * 120));
    std::vector<std::pair<int, colmap::point3D_t>> ranked;
    ranked.reserve(s->live_recon->NumPoints3D());
    for (const auto& [pid, pt] : s->live_recon->Points3D())
      ranked.emplace_back(static_cast<int>(pt.track.Length()), pid);
    std::sort(ranked.begin(), ranked.end(),
              std::greater<std::pair<int, colmap::point3D_t>>());
    size_t obs_budget = 0;
    for (const auto& [len, pid] : ranked) {
      if (obs_budget >= kMaxObs) break;
      cfg.AddVariablePoint(pid);
      obs_budget += static_cast<size_t>(len);
    }

    colmap::BundleAdjustmentOptions opt;
    opt.refine_rig_from_world = true;                                          
    opt.refine_points3D = true;
    opt.refine_focal_length = false;                                            
    opt.refine_principal_point = false;
    opt.print_summary = false;
    opt.ceres->loss_function_type =
        colmap::CeresBundleAdjustmentOptions::LossFunctionType::CAUCHY;
    opt.ceres->loss_function_scale = 1.0;
    opt.ceres->solver_options.max_num_iterations = 10;                           
                                                                                   
                                                                               
                                                                            
                                                                                
    opt.ceres->solver_options.num_threads = LiveBaThreads();
    {
      aether::official::ba::ScopedGlobalBaSolveV1 global_ba_scope;
      colmap::CreateDefaultBundleAdjuster(opt, cfg, *s->live_recon)->Solve();
    }

    s->last_global_refine_reg = n_reg;
    s->incremental_global_ba_ran = true;
    ++s->stat_incremental_refines;
    LOG(WARNING) << "[aether_sfm] incremental global BA #"
                 << s->stat_incremental_refines << " reg=" << n_reg
                 << " free=" << (n_reg - n_frozen) << " frozen=" << n_frozen
                 << " varpts_obs<=" << kMaxObs << " thermal=" << thermal_now
                 << " took=" << static_cast<int64_t>(NowMs() - t0) << "ms";
                                                                                    
                                                                                  
  } catch (const std::exception& e) {
                                                                              
                                                                 
    s->last_global_refine_reg = n_reg;
    LOG(WARNING) << "[aether_sfm] incremental global BA skipped (" << e.what()
                 << ") — windowed cloud kept";
  }
}

// Shared core of aether_sfm_add_frame (extraction upstream) and
                                                                                
                                                                               
                                                                              
                                                             
  
                                                                              
                                                                           
                                                     
                                                                             
                                                                                
                                                                       
                                                                              
                                                                         
static aether_sfm_result_t AddFrameFeaturesImpl(
    aether_sfm_session_t* s, const float* xy, const uint8_t* desc,
    const float* scales, int n, int width, int height, float fx, float fy,
    float cx, float cy, const double pose_qwxyz[4], const double pose_t[3],
    double extract_ms, int* out_frame_id) {
  aether::official::ba::ScopedBaSessionAggregateBindingV1 ba_session_binding(
      &s->ba_ptol_aggregate, &s->ba_ptol_receipts);
  try {
                                                                            
                                                                              
                                                                              
                                        
    aether::sfm::MandatoryArkitGravityPoseV1 gravity_pose;
    const aether::sfm::MandatoryArkitGravityPoseStatusV1 gravity_pose_status =
        aether::sfm::BuildMandatoryArkitGravityPoseV1(pose_qwxyz, pose_t,
                                                      &gravity_pose);
    if (gravity_pose_status !=
        aether::sfm::MandatoryArkitGravityPoseStatusV1::kOk) {
                                                                             
                                                                           
                                                                               
                                                                                
                                                                                
                                                                              
      LOG(WARNING) << "[aether_sfm][ARKIT-POSE-REJECT] frame rejected before "
                      "any db write, status="
                   << static_cast<int>(gravity_pose_status);
      return AETHER_SFM_ERR_INVALID_ARG;
    }

                                                                            
                                                                             
                                                                              
    if (width <= 0 || height <= 0 || !std::isfinite(fx) ||
        !std::isfinite(fy) || !std::isfinite(cx) || !std::isfinite(cy) ||
        fx <= 0.0f || fy <= 0.0f || cx < 0.0f || cy < 0.0f ||
        cx >= static_cast<float>(width) || cy >= static_cast<float>(height)) {
      return AETHER_SFM_ERR_INVALID_ARG;
    }
    colmap::Camera camera = colmap::Camera::CreateFromModelId(
        colmap::kInvalidCameraId, colmap::PinholeCameraModel::model_id,
                         fx, width, height);
    camera.SetFocalLengthX(fx);
    camera.SetFocalLengthY(fy);
    camera.SetPrincipalPointX(cx);
    camera.SetPrincipalPointY(cy);
    const colmap::camera_t camera_id = s->db->WriteCamera(camera);
    camera.camera_id = camera_id;
    if (s->camera_id == 0) s->camera_id = camera_id;
                                                                             
                      
    if (s->live_recon) {
      s->live_recon->AddCameraWithTrivialRig(camera);
      s->live_recon_ready = true;
    }

                                                
    const int frame_id = static_cast<int>(s->frames.size());
    colmap::Image image;
    char name[64];
    std::snprintf(name, sizeof(name), "frame_%06d.jpg", frame_id);
    image.SetName(name);
    image.SetCameraId(camera_id);
    const colmap::image_t image_id = s->db->WriteImage(image);

                                                                            
    // COLMAP exposes no DeleteImage to take it back. frame_id is derived from
                                                                           
                                                              
                                                                                
                                                                                
                                                 
                                                                              
                                                                          
                                                                            
                                                              
    struct OrdinalGuardV1 {
      aether_sfm_session* session = nullptr;
      int frame_id = -1;
      colmap::image_t image_id = 0;
      colmap::camera_t camera_id = 0;
      bool committed = false;
      ~OrdinalGuardV1() {
        if (committed || session == nullptr) return;
                                                                   
        try {
          FrameRecord withdrawn;
          withdrawn.frame_id = frame_id;
          withdrawn.image_id = 0;                                             
          withdrawn.persisted_image_id = image_id;
          withdrawn.camera_id = camera_id;
          withdrawn.n_keypoints = 0;
          session->frames.push_back(std::move(withdrawn));
          LOG(ERROR) << "[aether_sfm][ORDINAL-GUARD] frame " << frame_id
                     << " aborted after its db row was created; the ordinal is "
                        "consumed so the capture continues on the next frame";
                                                                               
                                                                            
                                                             
          (void)PersistCurrentArkitPoseSnapshot(*session);
        } catch (...) {
        }
      }
    } ordinal_guard{s, frame_id, image_id, camera_id, false};

    colmap::FeatureKeypoints kps(n);
    for (int i = 0; i < n; ++i) {
                                                                
                                                                                 
                                                                        
                                                                             
                                               
      if (scales != nullptr && std::isfinite(scales[i]) && scales[i] > 0.0f) {
        kps[i] =
            colmap::FeatureKeypoint(xy[2 * i], xy[2 * i + 1], scales[i], 0);
      } else {
        kps[i] = colmap::FeatureKeypoint(xy[2 * i], xy[2 * i + 1]);
      }
    }
                                                                    
                                                                         
                                                                           
                                                       
    colmap::FeatureDescriptors descriptors;
    descriptors.type = colmap::FeatureExtractorType::SIFT;
    descriptors.data.resize(n, 128);
    std::memcpy(descriptors.data.data(), desc,
                static_cast<size_t>(n) * 128);
    s->db->WriteKeypoints(image_id, kps);
    s->db->WriteDescriptors(image_id, descriptors);

    FrameRecord rec;
    rec.frame_id = frame_id;
    rec.image_id = image_id;
    rec.persisted_image_id = image_id;
    rec.camera_id = camera_id;
    rec.camera = camera;
    rec.n_keypoints = n;
    rec.descriptors.assign(desc, desc + static_cast<size_t>(n) * 128);
    rec.points = colmap::FeatureKeypointsToPointsVector(kps);
    rec.frame_identity_digest =
        FrameIdentityDigestV1(name, camera, rec.points, rec.descriptors);
    TailCacheMirrorImage(s, camera, image_id, name, rec.points);

    rec.cam_from_world = colmap::Rigid3d(
        Eigen::Quaterniond(gravity_pose.cam_from_world_qwxyz[0],
                           gravity_pose.cam_from_world_qwxyz[1],
                           gravity_pose.cam_from_world_qwxyz[2],
                           gravity_pose.cam_from_world_qwxyz[3]),
        Eigen::Vector3d(gravity_pose.cam_from_world_t_xyz[0],
                        gravity_pose.cam_from_world_t_xyz[1],
                        gravity_pose.cam_from_world_t_xyz[2]));
    rec.gravity_cam = Eigen::Vector3d(gravity_pose.gravity_cam_xyz[0],
                                      gravity_pose.gravity_cam_xyz[1],
                                      gravity_pose.gravity_cam_xyz[2]);
    rec.has_pose = true;

    const double gravity_cam_xyz[3] = {rec.gravity_cam.x(),
                                       rec.gravity_cam.y(),
                                       rec.gravity_cam.z()};
                                                                                 
                                                                      
                                                                                 
                                                                                
                                                                                
                                                                        
                                                                              
                                                                                
                                                           
    constexpr double kGravityBaSigmaRad =
        0.5 * 3.14159265358979323846 / 180.0;
    if (!aether_ba_set_gravity_prior(name, gravity_cam_xyz,
                                     kGravityBaSigmaRad)) {
                                                                             
                                                                            
                                                                          
                             
      return AETHER_SFM_ERR_INVALID_ARG;
    }
    if (!PersistArkitPoseSnapshot(*s, rec)) {
                                                                       
                                                                         
                                                                   
        
                                                                                 
                                                                                
                            
      LOG(ERROR) << "[aether_sfm][ARKIT-PERSIST-FAIL] frame " << rec.frame_id
                 << " dropped from the reconstruction: pose snapshot write "
                    "failed. Live capture continues on the next ordinal.";
      return AETHER_SFM_ERR_INTERNAL;
    }

                                                                            
                                                                            
                                                
    if (s->live_recon_ready) {
      colmap::Image rimg;
      rimg.SetImageId(image_id);
      rimg.SetName(name);
      rimg.SetCameraId(rec.camera_id);
      rimg.SetPoints2D(rec.points);                            
      s->live_recon->AddImageWithTrivialFrame(std::move(rimg), rec.cam_from_world);
      s->reg_order.push_back(image_id);
    }

                                                                      
                                                                            
                                                                          
                                                                        
                                                                       
                                                                             
                                                                         
                                                                          
                                                                         
                                                                          
                                                            
                                                                            
                                                                                 
                                                                          
                                                                            
                                                                                
                                                                                     
                              
                                                                                 
                                                                      
                                                                                          
                                                                               
                                                                                
                            
                                                       
                                                                               
                               
                                                                                
                                                                                
                                                                              
                                                                             
                                                                             
                                                                              
                                                                          
                                                                               
                                                                            
                                                                                        
                                                                     
    static const int env_base_k = [] {
      if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_CAND_K")) {
        const int v = std::atoi(e);
        if (v > 0) return v;
      }
      return 0;                                
    }();
    static const int hot_k = [] {
      if (const char* e = std::getenv("OFFICIAL_AETHER_LIVE_CAND_K_HOT")) {
        return std::atoi(e);                                              
      }
      return 0;                                    
    }();
    const int base_k = env_base_k > 0
                           ? env_base_k
                           : (s->options.k_neighbors > 0 ? s->options.k_neighbors
                                                         : 6);
    const int thermal_now = s->thermal_state.load(std::memory_order_relaxed);
    const bool throttled = thermal_now >= 2 && hot_k > 0 && hot_k < base_k;
    const int k = throttled ? hot_k : base_k;
    if (throttled) {
      rec.fed_throttled = true;
      ++s->stat_thermal_throttled_frames;
    }
    if (throttled != s->throttle_active_logged) {
      s->throttle_active_logged = throttled;
      LOG(WARNING) << "[aether_sfm] thermal throttle "
                   << (throttled ? "ON" : "OFF") << " at frame " << frame_id
                   << " (thermal=" << thermal_now << ", live K "
                   << (throttled ? base_k : hot_k) << "→"
                   << (throttled ? hot_k : base_k)
                   << "); finalize re-match restores the pair topology.";
      char tline[160];
      std::snprintf(tline, sizeof(tline),
                    "{\"t\":%lld,\"type\":\"thermal_throttle\",\"on\":%d,"
                    "\"frame\":%d,\"thermal\":%d,\"k\":%d}",
                    static_cast<long long>(EpochMs()), throttled ? 1 : 0,
                    frame_id, thermal_now, k);
      AppendMatchFailJsonl(s, tline);
    }
    int cand_spatial = 0, cand_temporal = 0;
    std::vector<int> candidates =
        SelectSpatialK20TemporalT2Candidates(
            *s, rec, frame_id, throttled, k, &cand_spatial, &cand_temporal);
                                                                             
                                                                            
                                                                              
                                         
    try {
      const std::vector<aether::sfm::VisualLoopCandidateV1> loop_candidates =
          s->visual_loop_index.QueryAndAdd(
              frame_id, rec.descriptors.data(), rec.n_keypoints, candidates);
      for (const aether::sfm::VisualLoopCandidateV1& loop : loop_candidates) {
        if (loop.frame_id < 0 || loop.frame_id >= frame_id) continue;
        const FrameRecord& previous = s->frames[loop.frame_id];
        if (previous.image_id == 0 ||
            s->db->ExistsMatches(previous.image_id, image_id) ||
            s->db->ExistsTwoViewGeometry(previous.image_id, image_id)) {
          continue;
        }
        candidates.push_back(loop.frame_id);
      }
      std::sort(candidates.begin(), candidates.end());
      candidates.erase(std::unique(candidates.begin(), candidates.end()),
                       candidates.end());
    } catch (...) {
                                                                   
    }
    AppendTailMatchCandidatesV1(
        s, frame_id, static_cast<int>(candidates.size()), candidates);
    s->stat_cand_spatial_first_pairs += cand_spatial;
    s->stat_cand_temporal_fallback_pairs += cand_temporal;
    const double ratio = s->options.match_max_ratio > 0
                             ? s->options.match_max_ratio
                             : 0.7;
    const colmap::TwoViewGeometryOptions tvg_options;                    
    const double t_match0 = NowMs();
                                                                             
                                                                          
                                                                            
                                                                         
                                               
    double t2_gpu_ms = 0.0, t2_tvg_ms = 0.0, t2_tri_ms = 0.0, t2_lba_ms = 0.0;
                                                                                     
    const int ab_phase =
        AbPeriod() > 0 ? ((frame_id / AbPeriod()) & 1) : -1;
    aether_match_set_ab_phase(ab_phase);
                                                                                 
                                                                              
                                                                     
    aether_match_gpu_ms = aether_match_sleep_ms = 0.0;
    aether_match_chunks = 0;
    aether_tvg_upright_ms = aether_tvg_homography_ms = 0.0;
    aether_tvg_upright_calls = aether_tvg_homography_calls = 0;
    const bool tail_cache_observed = TailCacheTraceEnabled();
    double t2_cache_ms = 0.0;
    int tail_cache_reused = 0;
    uint64_t tail_cache_generation = 0;
    int n_cand = 0, gpu_matches = 0, cpu_matches = 0;
    std::vector<uint32_t> pair_buf;
                                                                                 
                                                                              
                                                                          
                                                                            
                                                    
    std::unordered_set<colmap::point3D_t> touched;
                                                                                  
                                                                                        
                                              
    const double kMinTriAngleRad = LiveCreateTriMinAngleRad();
    const double kMaxCreateReprojPx = LiveCreateMaxReprojPx();                   
    const bool kSelfDevTri = SelfDevLiveTriangulationEnabled();
    const double kMaxGrowReprojPx = LiveGrowMaxReprojPx();                                        
    constexpr double kMaxMergeReprojPx = 8.0;                                         
                                                                               
                                                                         
    // CanMergeLivePoints subsumes the safety concern). Host attribution on the
                                                                             
                                                                             
                                                                             
    constexpr bool kMergeRequireDisjointImages = false;
    const bool gpu_match_avail =
        s->options.use_gpu_match && (aether_gpu_match_gemm_pairs != nullptr);
                                                                              
                                                                           
                                                                            
                                                                            
                                                            
                                                                            
                                                                                   
                                                                             
    const int probe_gate_min = ProbeGateMin();
    std::vector<uint8_t> probe_desc_buf;
    std::vector<uint32_t> probe_pair_buf;
    int probe_rows = 0;
    if (probe_gate_min > 0 && rec.n_keypoints > ProbeGateRows()) {
      probe_rows = ProbeGateRows();
      probe_desc_buf.resize(static_cast<size_t>(probe_rows) * 128);
      if (ProbeGateTopRows()) {
        std::memcpy(probe_desc_buf.data(), rec.descriptors.data(),
                    static_cast<size_t>(probe_rows) * 128);
      } else {
        const int probe_step = rec.n_keypoints / probe_rows;         
        for (int r = 0; r < probe_rows; ++r) {
          std::memcpy(probe_desc_buf.data() + static_cast<size_t>(r) * 128,
                      rec.descriptors.data() +
                          static_cast<size_t>(r) * probe_step * 128,
                      128);
        }
      }
      probe_pair_buf.resize(static_cast<size_t>(probe_rows) * 2);
    }
    int frame_probe_attempted = 0, frame_probe_skipped = 0;
    double frame_probe_ms = 0.0;
                                                                          
                                                                          
                                                                                 
                                                                               
                                                                          
                                                                             
                                                                         
                                                                        
                                                                        
                                                            
    std::vector<int> probe_batch_scores;                                 
    bool probe_batch_ready = false;
    const bool probe_batch_route =
        probe_rows > 0 && gpu_match_avail && ProbeGateBatchEnabled() &&
        aether_gpu_match_probe_batch != nullptr;
    if (probe_batch_route && !candidates.empty()) {
      std::vector<const uint8_t*> pb_descs;
      std::vector<int> pb_ns;
      std::vector<size_t> pb_pos;
      pb_descs.reserve(candidates.size());
      pb_ns.reserve(candidates.size());
      pb_pos.reserve(candidates.size());
      for (size_t ci = 0; ci < candidates.size(); ++ci) {
        const FrameRecord& prev = s->frames[candidates[ci]];
        if (prev.n_keypoints > 0 && !prev.descriptors.empty()) {
          pb_descs.push_back(prev.descriptors.data());
          pb_ns.push_back(prev.n_keypoints);
          pb_pos.push_back(ci);
        }
      }
      if (!pb_descs.empty()) {
        std::vector<int> pb_counts(pb_descs.size(), 0);
        const double t_p0 = NowMs();
        const int prc = aether_gpu_match_probe_batch(
            probe_desc_buf.data(), probe_rows, pb_descs.data(), pb_ns.data(),
            static_cast<int>(pb_descs.size()), ratio, pb_counts.data());
        const double probe_ms = NowMs() - t_p0;
        frame_probe_ms += probe_ms;
        s->stat_probe_ms += probe_ms;
        ++s->stat_probe_batch_calls;
        s->stat_probe_attempted += static_cast<int64_t>(pb_descs.size());
        frame_probe_attempted += static_cast<int>(pb_descs.size());
        if (prc == 0) {
          probe_batch_scores.assign(candidates.size(), -1);
          for (size_t k = 0; k < pb_pos.size(); ++k) {
            probe_batch_scores[pb_pos[k]] = pb_counts[k];
          }
          probe_batch_ready = true;
        } else {
          s->stat_probe_fail_open += static_cast<int64_t>(pb_descs.size());
        }
      }
    }
    std::unordered_set<PointIdPair, PointIdPairHash> merge_trials;
                                                                               
                                                                                
                                                                                 
                                                                             
                                                                            
                                                            
    struct OvMatchResult {
      int mrc = 1;
      int num_matches = 0;
      bool used_gpu = false;
      std::vector<uint32_t> pairs;
    };
    const bool ov_active = MatchTvgOverlapEnabled() && gpu_match_avail &&
                           probe_gate_min <= 0 && !EpiPriorMatchEnabled() &&
                           candidates.size() > 1;
    std::mutex ov_mu;
    std::condition_variable ov_cv;
    std::deque<OvMatchResult> ov_q;
    bool ov_abort = false;
    constexpr size_t kOvDepth = 3;
    std::thread ov_producer;
    if (ov_active) {
      ov_producer = std::thread([&] {
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
        for (size_t k = 0; k < candidates.size(); ++k) {
          {
            std::unique_lock<std::mutex> lk(ov_mu);
            ov_cv.wait(lk, [&] { return ov_q.size() < kOvDepth || ov_abort; });
            if (ov_abort) break;
          }
          const FrameRecord& pf = s->frames[candidates[k]];
          OvMatchResult r;
          const int pcap = pf.n_keypoints < rec.n_keypoints ? pf.n_keypoints
                                                            : rec.n_keypoints;
          if (pcap > 0 && !pf.descriptors.empty()) {
            r.pairs.resize(static_cast<size_t>(pcap) * 2);
            r.mrc = GpuMatchGemmPairsRetry(
                s, pf.frame_id, pf.descriptors.data(), pf.n_keypoints,
                rec.frame_id, rec.descriptors.data(), rec.n_keypoints, ratio,
                r.pairs.data(), pcap, &r.num_matches);
            r.used_gpu = (r.mrc == 0);
          }
          {
            std::lock_guard<std::mutex> lk(ov_mu);
            ov_q.push_back(std::move(r));
          }
          ov_cv.notify_all();
        }
      });
    }
    struct OvJoin {
      std::thread* th;
      std::mutex* mu;
      std::condition_variable* cv;
      bool* abort_flag;
      ~OvJoin() {
        if (th && th->joinable()) {
          {
            std::lock_guard<std::mutex> lk(*mu);
            *abort_flag = true;
          }
          cv->notify_all();
          th->join();
        }
      }
    } ov_join{ov_active ? &ov_producer : nullptr, &ov_mu, &ov_cv, &ov_abort};
    for (size_t cand_i = 0; cand_i < candidates.size(); ++cand_i) {
      const int j = candidates[cand_i];
      const FrameRecord& prev = s->frames[j];
      ++n_cand;
      s->gpu_watchdog.InProgress();                                        
                                                                         
                                                                           
                                                                              
                                                                                
                                                                    
                                                                         
                                                                             
                                                                      
                                                                            
                                                                       
                                                                     
      if (probe_rows > 0 && prev.n_keypoints > 0 &&
          !prev.descriptors.empty()) {
        if (probe_batch_ready) {
          const int probe_score = probe_batch_scores[cand_i];
          if (probe_score >= 0) {
            if (probe_score < probe_gate_min) {
              ++s->stat_probe_skipped;
              ++frame_probe_skipped;
              if (s->probe_skipped_pairs
                      .insert(FramePairKey(frame_id, j))
                      .second) {
                ++s->stat_probe_debt_registered;
              }
              continue;                                            
            }
            ++s->stat_probe_passed;
          }
        } else if (!probe_batch_route) {
                                                                       
                                                                
                                                                        
                                                                 
          const double t_p0 = NowMs();
          int probe_matches = 0;
                                                                            
                                                                      
                                                                        
                                                                    
                                    
          const int prc =
              gpu_match_avail
                  ? aether_gpu_match_gemm_pairs(
                        probe_desc_buf.data(), probe_rows,
                        prev.descriptors.data(), prev.n_keypoints, ratio,
                        probe_pair_buf.data(), probe_rows, &probe_matches)
                  : aether_sift_match_pairs(
                        probe_desc_buf.data(), probe_rows,
                        prev.descriptors.data(), prev.n_keypoints, ratio,
                        probe_pair_buf.data(), probe_rows, &probe_matches);
          const double probe_ms = NowMs() - t_p0;
          frame_probe_ms += probe_ms;
          s->stat_probe_ms += probe_ms;
          ++s->stat_probe_attempted;
          ++frame_probe_attempted;
          if (prc == 0) {
            if (probe_matches < probe_gate_min) {
              ++s->stat_probe_skipped;
              ++frame_probe_skipped;
              if (s->probe_skipped_pairs
                      .insert(FramePairKey(frame_id, j))
                      .second) {
                ++s->stat_probe_debt_registered;
              }
              continue;                                            
            }
            ++s->stat_probe_passed;
          } else {
            ++s->stat_probe_fail_open;                                     
          }
        }
                                                                         
                                                                   
      }
                                                                               
      const int cap = prev.n_keypoints < rec.n_keypoints ? prev.n_keypoints
                                                         : rec.n_keypoints;
      pair_buf.resize(static_cast<size_t>(cap) * 2);
      int num_matches = 0;
                                                                              
                                                                            
                                                                            
                                                                              
                                                                   
      int mrc = 1;
      bool used_gpu = false;
                                                                                 
                                                                               
                                                                             
                                                                    
      bool ov_consumed = false;
      if (ov_active) {
        OvMatchResult r;
        {
          std::unique_lock<std::mutex> lk(ov_mu);
          ov_cv.wait(lk, [&] { return !ov_q.empty(); });
          r = std::move(ov_q.front());
          ov_q.pop_front();
        }
        ov_cv.notify_all();
        ov_consumed = true;
        mrc = r.mrc;
        num_matches = r.num_matches;
        used_gpu = r.used_gpu;
        if (mrc != 0) {
          NoteGpuMatchFailure(s, mrc, j, frame_id);
          continue;
        }
        s->gpu_match_fail_streak = 0;
        ++s->gpu_pairs_since_rc7;
        if (num_matches > 0) {
          pair_buf.assign(r.pairs.begin(),
                          r.pairs.begin() + static_cast<size_t>(num_matches) * 2);
        }
      }
                                                                            
                                                                        
                              
      if (!ov_consumed && EpiPriorMatchEnabled() && gpu_match_avail) {
        const double t2_m0 = NowMs();
        const int gap = static_cast<int>(s->frames.size()) - j;
        const bool ok = ArkitGuidedMatchPair(s, prev, rec, gap, ratio,
                                             pair_buf.data(), cap,
                                             &num_matches);
        t2_gpu_ms += NowMs() - t2_m0;
        if (ok) {
          mrc = 0;
          used_gpu = true;
          s->gpu_match_fail_streak = 0;
          ++s->gpu_pairs_since_rc7;
        }
      }
      if (!ov_consumed && mrc != 0 && gpu_match_avail) {
                                                                         
                                                                        
                                                     
        const double t2_m0 = NowMs();
        mrc = GpuMatchGemmPairsRetry(
            s, prev.frame_id, prev.descriptors.data(), prev.n_keypoints,
            rec.frame_id, rec.descriptors.data(), rec.n_keypoints, ratio,
            pair_buf.data(), cap, &num_matches);
        t2_gpu_ms += NowMs() - t2_m0;
        if (mrc != 0) {
                                                                              
                                                                             
                                                                             
                                                                             
                                                                            
          NoteGpuMatchFailure(s, mrc, j, frame_id);
          continue;
        }
        s->gpu_match_fail_streak = 0;                                        
        ++s->gpu_pairs_since_rc7;                                          
        used_gpu = true;
      } else if (!ov_consumed && !gpu_match_avail) {
        const double t2_m0 = NowMs();
        mrc = aether_sift_match_pairs(
            prev.descriptors.data(), prev.n_keypoints, rec.descriptors.data(),
            rec.n_keypoints, ratio, pair_buf.data(), cap, &num_matches);
        t2_gpu_ms += NowMs() - t2_m0;
      }
      s->gpu_watchdog.InProgress();                                        
      if (mrc != 0) continue;
                                                                                              
                                                                                                     
                                                                                           
                                                                                                
                                                                                 
      const bool zero_raw_matches = num_matches <= 0;
      if (zero_raw_matches &&
          !(PoseDirectEEnabled() && prev.has_pose && rec.has_pose)) {
        continue;
      }
      if (num_matches > 0) {
        if (used_gpu) gpu_matches += num_matches; else cpu_matches += num_matches;
      }

      const double t2_v0 = NowMs();                                       
      colmap::FeatureMatches matches(num_matches);
      for (int m = 0; m < num_matches; ++m) {
        matches[m].point2D_idx1 = pair_buf[2 * m];
        matches[m].point2D_idx2 = pair_buf[2 * m + 1];
      }

      colmap::TwoViewGeometry two_view_geometry;
                                                                             
                                                                                 
                                                                             
                                                                         
                                                                               
                                                                               
                                                                  
                                                                              
                                                              
                                                                           
                                                                                 
                                                                                
                                                                                
                                                                               
                                                                 
                                                                                                 
                                                                                                        
                                                               
      if ((GuidedTemporalEnabled() || PoseDirectEEnabled()) && gpu_match_avail &&
          aether_gpu_match_gemm_pairs_guided != nullptr) {
        bool raw_seed_ok = false;
        if (!zero_raw_matches) {
          const auto initial_tvg = EstimateMandatoryFrameTwoViewGeometry(
              prev, prev.points, rec, rec.points, matches, tvg_options);
          raw_seed_ok = MandatoryGravityTvgPersistable(initial_tvg);
          if (raw_seed_ok) two_view_geometry = initial_tvg.geometry;
        }
        bool pair_ok = raw_seed_ok;

                                                                                                               
        if (raw_seed_ok && GuidedTemporalEnabled()) {
          colmap::FeatureMatches guided;
          if (RunGuidedMatch(s, prev, rec, two_view_geometry, &guided) &&
              !guided.empty()) {
            ++s->stat_guided_temporal_attempted;
            const auto guided_tvg = EstimateMandatoryFrameTwoViewGeometry(
                prev, prev.points, rec, rec.points, guided, tvg_options);
            if (MandatoryGravityTvgPersistable(guided_tvg) &&
                guided_tvg.geometry.inlier_matches.size() >
                two_view_geometry.inlier_matches.size()) {
              ++s->stat_guided_temporal_upgraded;
              s->stat_guided_temporal_extra_inliers +=
                  static_cast<int64_t>(
                      guided_tvg.geometry.inlier_matches.size()) -
                  static_cast<int64_t>(two_view_geometry.inlier_matches.size());
              matches = std::move(guided);
              num_matches = static_cast<int>(matches.size());
              two_view_geometry = guided_tvg.geometry;
            }
          }
        }

                                                                                               
                                                                                              
                                                                                                  
                                                                                                    
        if (!raw_seed_ok && PoseDirectEEnabled()) {
          colmap::TwoViewGeometry seed;
          if (BuildPoseDirectGuideGeometry(prev, rec, &seed)) {
            ++s->stat_pose_direct_attempted;
            colmap::FeatureMatches boot;
            if (RunGuidedMatch(s, prev, rec, seed, &boot,
                               PoseDirectBandPixels()) &&
                !boot.empty()) {
              const auto boot_tvg = EstimateMandatoryFrameTwoViewGeometry(
                  prev, prev.points, rec, rec.points, boot, tvg_options);
              if (MandatoryGravityTvgPersistable(boot_tvg)) {
                colmap::FeatureMatches refined;
                bool took_refined = false;
                if (RunGuidedMatch(s, prev, rec, boot_tvg.geometry, &refined) &&
                    !refined.empty()) {
                  const auto refined_tvg = EstimateMandatoryFrameTwoViewGeometry(
                      prev, prev.points, rec, rec.points, refined, tvg_options);
                  if (MandatoryGravityTvgPersistable(refined_tvg) &&
                      refined_tvg.geometry.inlier_matches.size() >
                          boot_tvg.geometry.inlier_matches.size()) {
                    matches = std::move(refined);
                    two_view_geometry = refined_tvg.geometry;
                    took_refined = true;
                  }
                }
                if (!took_refined) {
                  matches = std::move(boot);
                  two_view_geometry = boot_tvg.geometry;
                }
                num_matches = static_cast<int>(matches.size());
                pair_ok = true;
                ++s->stat_pose_direct_rescued;
                s->stat_pose_direct_inliers += static_cast<int64_t>(
                    two_view_geometry.inlier_matches.size());
              }
            }
          }
        }

                                                                                                 
        if (!pair_ok) continue;
      } else {
        if (zero_raw_matches) continue;                                       
        const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
            prev, prev.points, rec, rec.points, matches, tvg_options);
        if (!MandatoryGravityTvgPersistable(tvg)) continue;
        two_view_geometry = tvg.geometry;
      }
      const bool tail_pair_first_write =
          TailCacheBeginPairWrite(s, prev.image_id, image_id);
      s->db->WriteMatches(prev.image_id, image_id, matches);
      s->db->WriteTwoViewGeometry(prev.image_id, image_id, two_view_geometry);
      TailCacheCommitPair(s, prev.image_id, image_id, two_view_geometry,
                          tail_pair_first_write);
                                                                          
                                                                            
      s->live_pairs_done.insert(FramePairKey(frame_id, j));
      t2_tvg_ms += NowMs() - t2_v0;
                                                                            
                                                                          
                                                                          
                           
      {
        if (static_cast<int>(s->live_win_valid.size()) <= frame_id) {
          s->live_win_valid.resize(frame_id + 1, 0);
        }
        const int window_k =
            s->options.k_neighbors > 0 ? s->options.k_neighbors : 12;
        if (frame_id - j <= window_k &&
            static_cast<int>(two_view_geometry.inlier_matches.size()) >=
                kRematchValidInlierGate) {
          ++s->live_win_valid[j];
          ++s->live_win_valid[frame_id];
        }
      }

                                                                            
                                                              
                                                                            
                                                                          
                                                                               
                                                                            
      if (rec.has_pose && prev.has_pose && s->live_recon_ready) {
        const colmap::FeatureMatches& inliers =
            two_view_geometry.inlier_matches;
        s->stat_tvg_inlier_pairs += static_cast<int>(inliers.size());
        if (kSelfDevTri) {
          const LiveGrowView v_prev{prev.image_id, &prev.points, &prev.camera,
                                    prev.cam_from_world};
          const LiveGrowView v_cur{image_id, &rec.points, &rec.camera,
                                   rec.cam_from_world};
          GrowLiveTracksFromTvgInliers(
              s, s->live_recon.get(), v_prev, v_cur, inliers, kMinTriAngleRad,
              kMaxCreateReprojPx, kMaxGrowReprojPx, kMaxMergeReprojPx,
              kMergeRequireDisjointImages,                 true,
                             true, &merge_trials, &touched);
        }
      }
    }

                                                                                                                                                        
    // Run the upstream mapper's two-round local refinement. It selects the six
                                                                               
                                                                            
                                                            
                                                                                                  
                                                                                                    
                                                                                                     
                                                                                                         
                                                                                              
                                                                                   
                                                                                                     
    const bool pair_cloud_enabled = [] {
      static const bool cached = [] {
        const char* e = std::getenv("OFFICIAL_AETHER_PAIR_CLOUD");
                                                                                        
                                                                                           
                                                                                                  
                                                                                                    
                                                                                            
        return e && e[0] == '1' && e[1] == '\0';
      }();
      return cached;
    }();
    const size_t live_reg_n =
        (s->live_recon_ready && s->live_recon) ? s->live_recon->NumRegImages()
                                               : 0;
                                                                                                      
                                                                                               
    s->draft_pair_points.clear();
    s->draft_pair_obs_offsets.clear();
    s->draft_pair_obs.clear();
    bool live_local_ba_attempted = false;
    bool live_local_ba_succeeded = false;
    if (rec.has_pose && s->live_recon_ready &&
        live_reg_n >= (pair_cloud_enabled ? 2u : 3u) &&
        StreamingLocalBaEnabled() &&
        (frame_id % StreamingLocalBaPeriod(s->ba_every_n)) == 0) {
      try {
        colmap::DatabaseCache::Options cache_options;
        cache_options.min_num_matches = 15;
        cache_options.load_all_images = true;
        const double t2_cache0 = tail_cache_observed ? NowMs() : 0.0;
        std::shared_ptr<colmap::DatabaseCache> cache;
        std::shared_ptr<colmap::DatabaseCache> tail_shadow_cache;
        std::unique_lock<std::mutex> tail_cache_use_lock;
        if (TailCacheMode() == TailCacheModeV1::kOn) {
          tail_cache_use_lock =
              std::unique_lock<std::mutex>(s->tail_cache_mutex);
          if (!s->tail_cache_master || !s->tail_cache_epoch.readable()) {
            if (s->tail_cache_epoch.state() !=
                aether::sfm::TailCacheEpochStateV1::kDirty) {
              TailCacheMarkDirtyLockedV1(
                  s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
            }
            TailCacheRebuildLocked(s);
          }
          if (s->tail_cache_master && s->tail_cache_epoch.readable()) {
            cache = s->tail_cache_master;
            tail_cache_reused = 1;
            tail_cache_generation = s->tail_cache_epoch.generation();
          }
        }
        if (!cache) {
          cache = colmap::DatabaseCache::Create(*s->db, cache_options);
          if (TailCacheMode() == TailCacheModeV1::kShadow) {
            std::lock_guard<std::mutex> shadow_lock(s->tail_cache_mutex);
            if (!s->tail_cache_master || !s->tail_cache_epoch.readable()) {
              if (s->tail_cache_epoch.state() !=
                  aether::sfm::TailCacheEpochStateV1::kDirty) {
                TailCacheMarkDirtyLockedV1(
                    s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
              }
              TailCacheRebuildLocked(s);
            }
            tail_cache_generation = s->tail_cache_epoch.generation();
            if (s->tail_cache_master && s->tail_cache_epoch.readable()) {
              tail_shadow_cache = s->tail_cache_master;
            }
          }
        }
        if (tail_cache_observed) t2_cache_ms += NowMs() - t2_cache0;

        colmap::IncrementalPipelineOptions official_options;
        official_options.min_num_matches = 15;
        official_options.triangulation.ignore_two_view_tracks =
            TriIgnoreTwoViewTracks();
        official_options.load_all_images = true;
        official_options.ba_refine_focal_length = false;
        official_options.ba_refine_principal_point = false;
        official_options.ba_refine_extra_params = false;
        official_options.mapper.ba_local_num_images =
            s->ba_window > 0 ? s->ba_window : 6;
                                                                                 
                                                                            
                                                                                  
                                                                            
                                                                            
                                                              
                                                                          
                                                               
                                                                            
                                                                     
                                                                           
                                                                             
                                                                                
                                                                           
                                                                    
        official_options.num_threads = LiveBaThreads();
        official_options.ba_min_num_residuals_for_cpu_multi_threading = 6000;
        if (const int t = LiveLocalBaThreadsOverride(); t > 0) {
          official_options.num_threads = t;
        }
        if (const int f = LiveLocalBaMtFloorOverride(); f >= 0) {
          official_options.ba_min_num_residuals_for_cpu_multi_threading = f;
        }
        ApplyBirthGateAB(&official_options);                               
                                                                                
                                                                                   
                                                                        
                                                                                   
                                                                           
                                                                                  
                                                                                   
                                                                                
                                                                                
                                                                                 
                                                 
        official_options.ba_local_function_tolerance = LiveLocalBaFtol();
                                                                                            
                                             
        if (const int lt = LiveLocalBaLossType(); lt >= 0) {
          official_options.ba_local_loss_type = lt;
          official_options.ba_local_loss_scale = 1.0;
        }
        if (const int mi = LiveLocalBaMaxIter(); mi > 0) {
          official_options.ba_local_max_num_iterations = mi;
        }
        if (const double ma = LiveTriMinAngleDegOverride(); ma > 0.0) {
          official_options.triangulation.min_angle = ma;
        }

        colmap::IncrementalMapper mapper(cache);
        mapper.BeginReconstruction(s->live_recon);

                                                                 
                                                                               
                                                                               
                                                                                
                                                                                 
                                                                           
                                                                            
                                                                                 
                                                                                
        if (OfficialTriangulateImageEnabled()) {
          const double t2_t0 = NowMs();
          const size_t n_off =
              mapper.TriangulateImage(official_options.Triangulation(), image_id);
          s->stat_official_tri_added += static_cast<int64_t>(n_off);
          t2_tri_ms += NowMs() - t2_t0;
        }
                                                                            
                                                                         
                                                                         
                                                                              
                                                                              
                                                  
        LocalBundleTraceSnapshotV1 tail_shadow_trace;
        bool tail_shadow_executed = false;
        bool tail_shadow_failed = false;
                                                                                             
                                                                                 
        if (live_reg_n >= 3 && tail_shadow_cache) {
          auto tail_shadow_recon =
              std::make_shared<colmap::Reconstruction>(*s->live_recon);
          ResetLocalBundleTraceV1();
          std::thread shadow_thread([&] {
            aether::official::ba::ScopedBaSessionAggregateBindingV1
                ba_session_binding(&s->ba_ptol_aggregate,
                                   &s->ba_ptol_receipts);
            try {
              colmap::IncrementalMapper shadow_mapper(tail_shadow_cache);
              shadow_mapper.BeginReconstruction(tail_shadow_recon);
              shadow_mapper.IterativeLocalRefinement(
                  official_options.ba_local_max_refinements,
                  official_options.ba_local_max_refinement_change,
                  official_options.Mapper(),
                  official_options.LocalBundleAdjustment(),
                  official_options.Triangulation(), image_id);
              shadow_mapper.EndReconstruction(            false);
              tail_shadow_executed = true;
            } catch (...) {
              tail_shadow_failed = true;
            }
          });
          shadow_thread.join();
          tail_shadow_trace = CaptureLocalBundleTraceV1();
        }

                                                                              
                                                                               
        aether_ilr_find_ms = aether_ilr_setup_ms = aether_ilr_solve_ms = 0.0;
        aether_ilr_merge_ms = aether_ilr_filter_ms = 0.0;
        aether_ilr_preproc_ms = aether_ilr_minim_ms = aether_ilr_postproc_ms =
            0.0;
        aether_ilr_jac_ms = aether_ilr_lin_ms = aether_ilr_resid_ms = 0.0;
        aether_ilr_num_resid = aether_ilr_num_param = 0;
        aether_ilr_rounds = aether_ilr_solves = aether_ilr_iters = 0;
        aether_ilr_term_conv = aether_ilr_term_nocnv = aether_ilr_term_other =
            0;
        aether_ilr_ord_applied = aether_ilr_ord_skipped = 0;
        ResetLocalBundleTraceV1();
        const double t2_l0 = NowMs();
        if (live_reg_n >= 3) {
          live_local_ba_attempted = true;
          mapper.IterativeLocalRefinement(
              official_options.ba_local_max_refinements,
              official_options.ba_local_max_refinement_change,
              official_options.Mapper(),
              official_options.LocalBundleAdjustment(),
              official_options.Triangulation(), image_id);
          live_local_ba_succeeded = true;
        }
        t2_lba_ms += NowMs() - t2_l0;
        const LocalBundleTraceSnapshotV1 fresh_trace =
            CaptureLocalBundleTraceV1();
        AppendTailLocalBundleTraceV1(s, frame_id, tail_cache_reused,
                                     tail_cache_generation, "fresh",
                                     fresh_trace);
        if (tail_shadow_cache) {
          AppendTailLocalBundleTraceV1(s, frame_id,                  1,
                                       tail_cache_generation, "mutable",
                                       tail_shadow_trace);
          const bool exact = tail_shadow_executed && !tail_shadow_failed &&
                             LocalBundleTraceExactV1(fresh_trace,
                                                     tail_shadow_trace);
          std::string compare_line =
              "{\"t\":" +
              std::to_string(static_cast<long long>(EpochMs())) +
              ",\"type\":\"tail_shadow_compare_v1\",\"fid\":" +
              std::to_string(frame_id) + ",\"status\":\"" +
              (exact ? "EXACT" : "INVALID_TAIL_BUNDLE_SEQUENCE") +
              "\",\"fresh_calls\":" +
              std::to_string(fresh_trace.raw_calls) +
              ",\"mutable_calls\":" +
              std::to_string(tail_shadow_trace.raw_calls) + "}";
          AppendMatchFailJsonl(s, compare_line);
          if (!exact) {
            TailCacheMarkDirty(
                s, aether::sfm::TailCacheDirtyReasonV1::kCacheInconsistency);
          }
        }
        mapper.EndReconstruction(            false);
      } catch (const std::exception&) {
                                                                             
                                                        
        TailCacheMarkDirty(
            s, aether::sfm::TailCacheDirtyReasonV1::kExceptionRetry);
      }
    }

                                                                                            
                                                                                                    
                                                           
                                                                                        
                                                                                             
                                                  
                                                                                               
                                             
    if (PairDraftEnabled() && !pair_cloud_enabled && rec.has_pose &&
        s->live_recon_ready && s->live_recon && live_reg_n == 2 &&
        StreamingLocalBaEnabled()) {
      const double t_draft0 = NowMs();
      bool draft_failed = false;
      auto draft_recon =
          std::make_shared<colmap::Reconstruction>(*s->live_recon);
                                                                                               
                                                                                    
                                                                                            
                                                                                   
                                                                    
      std::shared_ptr<colmap::DatabaseCache> draft_cache;
      try {
        colmap::DatabaseCache::Options draft_cache_options;
        draft_cache_options.min_num_matches = 15;
        draft_cache_options.load_all_images = true;
        draft_cache = colmap::DatabaseCache::Create(*s->db, draft_cache_options);
      } catch (...) {
        draft_failed = true;
      }
      if (!draft_failed && draft_cache) {
        std::thread draft_thread([&] {
          try {
            colmap::IncrementalPipelineOptions draft_options;
            draft_options.min_num_matches = 15;
            draft_options.triangulation.ignore_two_view_tracks =
                TriIgnoreTwoViewTracks();
            draft_options.load_all_images = true;
            colmap::IncrementalMapper draft_mapper(draft_cache);
            draft_mapper.BeginReconstruction(draft_recon);
            draft_mapper.TriangulateImage(draft_options.Triangulation(),
                                          image_id);
            draft_mapper.EndReconstruction(            false);
          } catch (...) {
            draft_failed = true;
          }
        });
        draft_thread.join();
      }
      if (!draft_failed && draft_recon->NumPoints3D() > 0) {
        const colmap::Reconstruction& dr = *draft_recon;
        const auto& dpts = dr.Points3D();
        std::vector<aether_sfm_point_t> pts_out;
        std::vector<int32_t> offs_out;
        std::vector<aether_sfm_track_obs_t> obs_out;
        pts_out.reserve(dpts.size());
        offs_out.reserve(dpts.size() + 1);
        for (const auto& [pid, pt] : dpts) {
          aether_sfm_point_t o;
          o.x = static_cast<float>(pt.xyz.x());
          o.y = static_cast<float>(pt.xyz.y());
          o.z = static_cast<float>(pt.xyz.z());
          o.r = pt.color(0);
          o.g = pt.color(1);
          o.b = pt.color(2);
          o._pad[0] = o._pad[1] = 0;
          offs_out.push_back(static_cast<int32_t>(obs_out.size()));
          for (const auto& el : pt.track.Elements()) {
            if (!dr.ExistsImage(el.image_id)) continue;
            const auto& xy =
                dr.Image(el.image_id).Point2D(el.point2D_idx).xy;
            aether_sfm_track_obs_t t;
            t.frame_id = static_cast<int32_t>(el.image_id) - 1;
            t.x = static_cast<float>(xy.x());
            t.y = static_cast<float>(xy.y());
            obs_out.push_back(t);
          }
          pts_out.push_back(o);
        }
        offs_out.push_back(static_cast<int32_t>(obs_out.size()));
        s->draft_pair_points.swap(pts_out);
        s->draft_pair_obs_offsets.swap(offs_out);
        s->draft_pair_obs.swap(obs_out);
      }
      AppendMatchFailJsonl(
          s, "{\"t\":" + std::to_string(static_cast<long long>(EpochMs())) +
                 ",\"type\":\"pair_draft\",\"fid\":" + std::to_string(frame_id) +
                 ",\"n\":" + std::to_string(s->draft_pair_points.size()) +
                 ",\"failed\":" + std::to_string(draft_failed ? 1 : 0) +
                 ",\"ms\":" + std::to_string(NowMs() - t_draft0) + "}");
    }

                                                                             
                                                                                          
                                                                                 
                                                                           
    const int64_t incremental_refines_before = s->stat_incremental_refines;
    MaybeIncrementalGlobalRefine(s);
    const bool incremental_global_refine_succeeded =
        s->stat_incremental_refines > incremental_refines_before;

                                                                                 
                                                                         
    if (s->live_recon) {
      std::vector<Eigen::Vector3d> snap;
      snap.reserve(s->live_recon->NumPoints3D());
      for (const auto& [pid, pt] : s->live_recon->Points3D()) snap.push_back(pt.xyz);
      std::lock_guard<std::mutex> lk(s->preview_mutex);
      s->preview_points.swap(snap);
    }

    s->frames.push_back(std::move(rec));
                                                                            
                                                                              
                                           
    ordinal_guard.committed = true;
                                                                        
                                                                           
                                                                        
    try {
      if (s->live_recon) {
        std::vector<std::array<double, 3>> center_deltas;
        center_deltas.reserve(s->frames.size());
        int latest_compared_fid = -1;
        for (const FrameRecord& frame : s->frames) {
          if (!frame.has_pose || frame.image_id == 0 ||
              !s->live_recon->ExistsImage(frame.image_id)) {
            continue;
          }
          const colmap::Image& image = s->live_recon->Image(frame.image_id);
          if (!image.HasPose()) continue;
          const Eigen::Vector3d prior_center =
              frame.cam_from_world.TgtOriginInSrc();
          const Eigen::Vector3d optimized_center =
              image.CamFromWorld().TgtOriginInSrc();
          const Eigen::Vector3d delta = optimized_center - prior_center;
          if (!delta.allFinite()) continue;
          center_deltas.push_back({delta.x(), delta.y(), delta.z()});
          latest_compared_fid = frame.frame_id;
        }
        const aether::sfm::LiveCloudBaDeltaSummaryV1 diag =
            aether::sfm::SummarizeLiveCloudBaDeltasV1(center_deltas);
        if (diag.valid) {
          char dline[1152];
          std::snprintf(
              dline, sizeof(dline),
              "{\"t\":%lld,\"type\":\"ba_arkit_center_delta_v1\","
              "\"contract\":\"PW_LIVE_CLOUD_DIAG_V1_STREAM\","
              "\"stage\":\"post_live_update\",\"fid\":%d,\"n\":%zu,"
              "\"local_ba_attempted\":%s,\"local_ba_succeeded\":%s,"
              "\"incremental_global_refine_succeeded\":%s,"
              "\"median_dx_m\":%.9g,\"median_dy_m\":%.9g,"
              "\"median_dz_m\":%.9g,\"coherent_median_m\":%.9g,"
              "\"p50_m\":%.9g,\"p90_m\":%.9g,\"max_m\":%.9g,"
              "\"latest_fid\":%d,\"latest_dx_m\":%.9g,"
              "\"latest_dy_m\":%.9g,\"latest_dz_m\":%.9g,"
              "\"latest_m\":%.9g,\"residual_rms_m\":%.9g,"
              "\"observation_only\":true}",
              static_cast<long long>(EpochMs()), frame_id, diag.count,
              live_local_ba_attempted ? "true" : "false",
              live_local_ba_succeeded ? "true" : "false",
              incremental_global_refine_succeeded ? "true" : "false",
              diag.median_delta_m[0], diag.median_delta_m[1],
              diag.median_delta_m[2], diag.coherent_median_norm_m,
              diag.p50_norm_m, diag.p90_norm_m, diag.max_norm_m,
              latest_compared_fid, diag.latest_delta_m[0],
              diag.latest_delta_m[1], diag.latest_delta_m[2],
              diag.latest_norm_m, diag.residual_rms_m);
          AppendMatchFailJsonl(s, dline);
        }
      }
    } catch (...) {
                                                                            
                                                                              
    }
    AppendLiveCloudSnapshotDiagnosticsV2(s, "post_local_ba");
                                                                       
                                                                       
                                                                       
                                                                   
    if (const char* dump_dir = std::getenv("OFFICIAL_AETHER_LIVE_POSE_DUMP");
        dump_dir && dump_dir[0] && s->live_recon) {
      static const int kDumpEvery = [] {
        const char* v = std::getenv("OFFICIAL_AETHER_LIVE_POSE_DUMP_EVERY");
        const int k = v ? std::atoi(v) : 0;
        return k > 0 ? k : 0;
      }();
      const size_t nreg = s->live_recon->NumRegImages();
      if (kDumpEvery > 0 && nreg > 0 &&
          nreg % static_cast<size_t>(kDumpEvery) == 0) {
        try {
          char sub[64];
          std::snprintf(sub, sizeof(sub), "/live_reg%05zu", nreg);
          const std::string snap_dir = std::string(dump_dir) + sub;
          std::filesystem::create_directories(snap_dir);
          s->live_recon->Write(snap_dir);
        } catch (const std::exception& e) {
          LOG(WARNING) << "[aether_sfm] live evolution dump failed: "
                       << e.what();
        }
      }
    }
    s->last_extract_ms = extract_ms;
    s->last_match_ms = NowMs() - t_match0;
                                                                            
                                                       
    {
      const double t2_total = s->last_match_ms;
      char fline[1024];
      const double* ex = s->last_extract_stages;
      const int active_components = CountActiveComponentsV1(*s);
      const double t2_tail_ms =
          t2_total - t2_gpu_ms - t2_tvg_ms - t2_tri_ms - t2_lba_ms;
                                                                             
                                                                              
                                                                             
                                    
      if (tail_cache_observed) {
        std::snprintf(
            fline, sizeof(fline),
            "{\"t\":%lld,\"type\":\"frame_split\",\"fid\":%d,"
            "\"cand\":%d,\"comp\":%d,\"gpu_ms\":%.0f,\"tvg_ms\":%.0f,"
            "\"tri_ms\":%.0f,\"lba_ms\":%.0f,\"tail_ms\":%.0f,"
            "\"cache_ms\":%.1f,\"tail_cache\":%d,\"tail_gen\":%llu,"
            "\"ex\":[%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f],"
            "\"ilr_find\":%.1f,\"ilr_setup\":%.1f,\"ilr_solve\":%.1f,"
            "\"ilr_merge\":%.1f,\"ilr_filter\":%.1f,"
            "\"ilr_pre\":%.1f,\"ilr_min\":%.1f,\"ilr_post\":%.1f,"
            "\"ilr_rounds\":%d,\"ilr_solves\":%d,\"ilr_iters\":%d,"
            "\"ilr_conv\":%d,\"ilr_nocnv\":%d,\"ilr_oth\":%d,"
            "\"ilr_jac\":%.1f,\"ilr_lin\":%.1f,\"ilr_res\":%.1f,"
            "\"ilr_nres\":%lld,\"ilr_npar\":%lld,"
            "\"tvg_up\":%.1f,\"tvg_h\":%.1f,\"tvg_upn\":%d,\"tvg_hn\":%d,"
            "\"m_gpu\":%.0f,\"m_sleep\":%.0f,\"m_chunks\":%d,\"ab\":%d,"
            "\"ilr_ord\":%d,\"ilr_ordskip\":%d,"
            "\"pf\":%d,\"pf_wait\":%.1f}",
            static_cast<long long>(EpochMs()), frame_id, n_cand,
            active_components, t2_gpu_ms,
            t2_tvg_ms, t2_tri_ms, t2_lba_ms, t2_tail_ms, t2_cache_ms,
            tail_cache_reused,
            static_cast<unsigned long long>(tail_cache_generation), ex[0],
            ex[1], ex[2], ex[3], ex[4], ex[5], ex[6], ex[7], ex[8],
            aether_ilr_find_ms, aether_ilr_setup_ms, aether_ilr_solve_ms,
            aether_ilr_merge_ms, aether_ilr_filter_ms, aether_ilr_preproc_ms,
            aether_ilr_minim_ms, aether_ilr_postproc_ms, aether_ilr_rounds,
            aether_ilr_solves, aether_ilr_iters, aether_ilr_term_conv,
            aether_ilr_term_nocnv, aether_ilr_term_other,
            aether_ilr_jac_ms, aether_ilr_lin_ms, aether_ilr_resid_ms,
            static_cast<long long>(aether_ilr_num_resid),
            static_cast<long long>(aether_ilr_num_param),
            aether_tvg_upright_ms, aether_tvg_homography_ms,
            aether_tvg_upright_calls, aether_tvg_homography_calls,
            aether_match_gpu_ms, aether_match_sleep_ms, aether_match_chunks,
            ab_phase, aether_ilr_ord_applied, aether_ilr_ord_skipped,
            s->last_pf_state, s->last_pf_wait_ms);
      } else {
        std::snprintf(
            fline, sizeof(fline),
            "{\"t\":%lld,\"type\":\"frame_split\",\"fid\":%d,"
            "\"cand\":%d,\"comp\":%d,\"gpu_ms\":%.0f,\"tvg_ms\":%.0f,"
            "\"tri_ms\":%.0f,\"lba_ms\":%.0f,\"tail_ms\":%.0f,"
            "\"ex\":[%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f],"
            "\"ilr_find\":%.1f,\"ilr_setup\":%.1f,\"ilr_solve\":%.1f,"
            "\"ilr_merge\":%.1f,\"ilr_filter\":%.1f,"
            "\"ilr_pre\":%.1f,\"ilr_min\":%.1f,\"ilr_post\":%.1f,"
            "\"ilr_rounds\":%d,\"ilr_solves\":%d,\"ilr_iters\":%d,"
            "\"ilr_conv\":%d,\"ilr_nocnv\":%d,\"ilr_oth\":%d,"
            "\"ilr_jac\":%.1f,\"ilr_lin\":%.1f,\"ilr_res\":%.1f,"
            "\"ilr_nres\":%lld,\"ilr_npar\":%lld,"
            "\"tvg_up\":%.1f,\"tvg_h\":%.1f,\"tvg_upn\":%d,\"tvg_hn\":%d,"
            "\"m_gpu\":%.0f,\"m_sleep\":%.0f,\"m_chunks\":%d,\"ab\":%d,"
            "\"ilr_ord\":%d,\"ilr_ordskip\":%d,"
            "\"pf\":%d,\"pf_wait\":%.1f}",
            static_cast<long long>(EpochMs()), frame_id, n_cand,
            active_components, t2_gpu_ms,
            t2_tvg_ms, t2_tri_ms, t2_lba_ms, t2_tail_ms, ex[0], ex[1], ex[2],
            ex[3], ex[4], ex[5], ex[6], ex[7], ex[8], aether_ilr_find_ms,
            aether_ilr_setup_ms, aether_ilr_solve_ms, aether_ilr_merge_ms,
            aether_ilr_filter_ms, aether_ilr_preproc_ms, aether_ilr_minim_ms,
            aether_ilr_postproc_ms, aether_ilr_rounds, aether_ilr_solves,
            aether_ilr_iters, aether_ilr_term_conv, aether_ilr_term_nocnv,
            aether_ilr_term_other,
            aether_ilr_jac_ms, aether_ilr_lin_ms, aether_ilr_resid_ms,
            static_cast<long long>(aether_ilr_num_resid),
            static_cast<long long>(aether_ilr_num_param),
            aether_tvg_upright_ms, aether_tvg_homography_ms,
            aether_tvg_upright_calls, aether_tvg_homography_calls,
            aether_match_gpu_ms, aether_match_sleep_ms, aether_match_chunks,
            ab_phase, aether_ilr_ord_applied, aether_ilr_ord_skipped,
            s->last_pf_state, s->last_pf_wait_ms);
      }
      AppendMatchFailJsonl(s, fline);
    }
                                                                              
                                                                              
    if (probe_gate_min > 0) {
      char pline[288];
      std::snprintf(pline, sizeof(pline),
                    "{\"t\":%lld,\"type\":\"probe_gate\",\"fid\":%d,"
                    "\"min\":%d,\"att\":%d,\"skip\":%d,\"probe_ms\":%.1f,"
                    "\"cum_att\":%lld,\"cum_skip\":%lld,"
                                                                           
                                                                        
                    "\"batch\":%d,\"debt\":%lld}",
                    static_cast<long long>(EpochMs()), frame_id,
                    probe_gate_min, frame_probe_attempted, frame_probe_skipped,
                    frame_probe_ms,
                    static_cast<long long>(s->stat_probe_attempted),
                    static_cast<long long>(s->stat_probe_skipped),
                    probe_batch_ready ? 1 : 0,
                    static_cast<long long>(s->probe_skipped_pairs.size()));
      AppendMatchFailJsonl(s, pline);
    }
    AppendGpuTimestampFrameRecord(s, frame_id);
                                                                         
    s->last_add_frame_done_ms = NowMs();
    s->last_n_cand = n_cand;
    s->last_gpu_matches = gpu_matches;
    s->last_cpu_matches = cpu_matches;
    const int accepted_thermal =
        s->thermal_state.load(std::memory_order_relaxed);
    const bool accepted_thermal_valid =
        accepted_thermal >= 0 && accepted_thermal <= 3;
    aether_preclamp_instr_v1::FrameCounts preclamp_row;
    if (aether_preclamp_instr_v1::FinalizeAcceptedFrame(
            frame_id, static_cast<uint32_t>(s->frames.size()),
            accepted_thermal_valid
                ? aether_preclamp_instr_v1::FieldStatus::kValid
                : aether_preclamp_instr_v1::FieldStatus::kUnavailable,
            accepted_thermal_valid ? accepted_thermal : -1,
            &preclamp_row)) {
      (void)aether_preclamp_instr_v1::AppendSessionRecord(
          &s->preclamp_instr_records, preclamp_row);
    }
                                                                                                         
                                                    
                                                                                
                                                                                            
                                                                              
                                                                              
                                                                                                  
                                          
                                                                                                  
                                                                           
                                                                             
                                                                                              
                                                            
                                                                                                                      
    if (out_frame_id) *out_frame_id = frame_id;
    return AETHER_SFM_OK;
  } catch (const std::exception&) {
    TailCacheMarkDirty(
        s, aether::sfm::TailCacheDirtyReasonV1::kExceptionRetry);
    return AETHER_SFM_ERR_INTERNAL;
  } catch (...) {
    TailCacheMarkDirty(
        s, aether::sfm::TailCacheDirtyReasonV1::kExceptionRetry);
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                                                                                 
                                                                                                  
static bool ExtractPrefetchEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_EXTRACT_PREFETCH");
    return e && e[0] == '1' && e[1] == '\0';
  }();
  return cached;
}
                                                                                               
                                                                                            
                                                                                                            
                                                                                          
                                                                       
                                                                                           
                                                                                       
                                                                
                                                                                                   
extern "C" int aether_gpu_match_get_capture_active(void) __attribute__((weak));
static bool ExtractPrefetchDrainOnly() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_EXTRACT_PREFETCH_DRAIN_ONLY");
    return !(e && e[0] == '0' && e[1] == '\0');
  }();
  return cached;
}
                                                                                  
static int ExtractPrefetchBlockReason() {
  if (!ExtractPrefetchEnabled()) return 1;
  if (!ExtractPrefetchDrainOnly()) return 0;
  if (aether_gpu_match_get_capture_active == nullptr) return 2;
  return aether_gpu_match_get_capture_active() == 0 ? 0 : 2;
}
                                                                                           
                                                                                                   
static uint64_t PfDigestV1(const uint8_t* gray, int w, int h) {
  uint64_t hash = UINT64_C(1469598103934665603);
  const auto mix = [&hash](uint64_t v) {
    for (int k = 0; k < 8; ++k) {
      hash ^= (v >> (k * 8)) & 0xff;
      hash *= UINT64_C(1099511628211);
    }
  };
  const size_t len = static_cast<size_t>(w) * static_cast<size_t>(h);
  mix(static_cast<uint64_t>(w));
  mix(static_cast<uint64_t>(h));
  for (size_t i = 0; i < 64 && i < len; ++i) mix(gray[i]);
  for (size_t i = len >= 64 ? len - 64 : 0; i < len; ++i) mix(gray[i]);
  for (size_t i = 0; i < len; i += 4093) mix(gray[i]);
  return hash;
}
                                                                                          
                                                                                
static void PfExtractInto(aether_sfm_session_t* s, const uint8_t* gray,
                          int width, int height,
                          aether_sfm_session_t::PfResult* out) {
  const int max_features =
      s->options.max_features > 0 ? s->options.max_features : 2048;
  out->xy.assign(static_cast<size_t>(max_features) * 2, 0.0f);
  out->desc.assign(static_cast<size_t>(max_features) * 128, 0);
  out->scales.assign(static_cast<size_t>(max_features), 0.0f);
  out->n = 0;
  const bool gpu_v2_avail = s->options.use_gpu_extract &&
                            (aether_dsp_sift_extract_gpu_v2 != nullptr);
  const bool gpu_avail =
      s->options.use_gpu_extract && (aether_dsp_sift_extract_gpu != nullptr);
  const double t0 = NowMs();
  if (gpu_v2_avail) {
    out->erc = aether_dsp_sift_extract_gpu_v2(
        gray, width, height, max_features,                 0, out->xy.data(),
        out->desc.data(), out->scales.data(),                      nullptr,
        max_features, &out->n);
    out->have_scales = (out->erc == 0);
  } else if (gpu_avail) {
    out->erc = aether_dsp_sift_extract_gpu(gray, width, height, max_features,
                                                           0, out->xy.data(),
                                           out->desc.data(), max_features,
                                           &out->n);
    out->have_scales = false;
  } else {
    out->erc = aether_dsp_sift_extract_v2(
        gray, width, height, max_features, out->xy.data(), out->desc.data(),
        out->scales.data(),                      nullptr, max_features,
        &out->n);
    out->have_scales = (out->erc == 0);
  }
  out->extract_ms = NowMs() - t0;
  for (double& v : out->stages) v = 0.0;
  if ((gpu_v2_avail || gpu_avail) && out->erc == 0 &&
      aether_sed_last_stages != nullptr) {
    aether_sed_last_stages(out->stages, 9);
  }
  if (gpu_v2_avail || gpu_avail) {
    PullGpuTimestampAfterExtraction(s);
  }
}
static void PfThreadMain(aether_sfm_session_t* s) {
#if defined(__APPLE__)
  pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
#endif
  for (;;) {
    aether_sfm_session_t::PfJob job;
    bool sync = false;
    {
      std::unique_lock<std::mutex> lk(s->pf_mu);
      s->pf_cv.wait(lk, [&] {
        return s->pf_stop || s->pf_sync_job.valid || s->pf_job.valid;
      });
      if (s->pf_stop) return;
                                                                                              
      if (s->pf_sync_job.valid) {
        job = std::move(s->pf_sync_job);
        s->pf_sync_job = {};
        sync = true;
      } else {
        job = std::move(s->pf_job);
        s->pf_job = {};
      }
    }
    aether_sfm_session_t::PfResult r;
    r.digest = job.digest;
    r.w = job.w;
    r.h = job.h;
    const uint8_t* gray = job.borrow ? job.borrow : job.gray.data();
    PfExtractInto(s, gray, job.w, job.h, &r);
    r.valid = true;
    {
      std::lock_guard<std::mutex> lk(s->pf_mu);
      if (sync) {
        s->pf_sync_result = std::move(r);                                
      } else {
        s->pf_result = std::move(r);
      }
    }
    s->pf_cv.notify_all();
  }
}
static void PfEnsureThread(aether_sfm_session_t* s) {
  if (s->pf_started) return;
  s->pf_started = true;
  s->pf_thread = std::thread(PfThreadMain, s);
}
                                                                                                
                                                                                                   
static bool PfAdoptOrRun(aether_sfm_session_t* s, const uint8_t* gray,
                         int width, int height,
                         aether_sfm_session_t::PfResult* out) {
  s->last_pf_state = 0;
                                                                                        
                                                                                                            
  if (const int why = ExtractPrefetchBlockReason(); why != 0) {
    s->last_pf_state = (why == 2) ? 4 : 0;
    return false;
  }
                                                                                              
                                                                                                 
  if (AbPeriod() > 0 &&
      ((static_cast<int>(s->frames.size()) / AbPeriod()) & 1) == 1) {
    return false;
  }
  PfEnsureThread(s);
  if (!s->pf_thread.joinable()) return false;
  const uint64_t digest = PfDigestV1(gray, width, height);
  std::unique_lock<std::mutex> lk(s->pf_mu);
  if (std::getenv("OFFICIAL_AETHER_PF_DIAG")) {
    std::fprintf(stderr,
                 "[PF-DIAG] adopt w=%d h=%d digest=%llu | have valid=%d w=%d h=%d digest=%llu\n",
                 width, height, (unsigned long long)digest,
                 (int)s->pf_result.valid, s->pf_result.w, s->pf_result.h,
                 (unsigned long long)s->pf_result.digest);
  }
  if (s->pf_result.valid && s->pf_result.digest == digest &&
      s->pf_result.w == width && s->pf_result.h == height) {
    *out = std::move(s->pf_result);
    s->pf_result = {};
    ++s->stat_pf_hits;
    s->last_pf_wait_ms = 0.0;
    s->last_pf_state = 1;
    return true;
  }
  if (s->pf_job.valid && s->pf_job.digest == digest) {
    const double t0 = NowMs();
    s->pf_cv.wait(lk, [&] {
      return s->pf_stop || (s->pf_result.valid && s->pf_result.digest == digest);
    });
    if (s->pf_stop) return false;
    *out = std::move(s->pf_result);
    s->pf_result = {};
    ++s->stat_pf_waits;
    s->last_pf_wait_ms = NowMs() - t0;
    s->last_pf_state = 2;
    return true;
  }
                                                                                                 
                                                                        
                                                                                            
                                                                                                        
  s->pf_sync_result = {};
  s->pf_sync_job = {};
  s->pf_sync_job.borrow = gray;
  s->pf_sync_job.w = width;
  s->pf_sync_job.h = height;
  s->pf_sync_job.digest = digest;
  s->pf_sync_job.valid = true;
  s->pf_cv.notify_all();
  const double t0 = NowMs();
  s->pf_cv.wait(lk, [&] {
    return s->pf_stop ||
           (s->pf_sync_result.valid && s->pf_sync_result.digest == digest);
  });
  if (s->pf_stop) return false;
  *out = std::move(s->pf_sync_result);
  s->pf_sync_result = {};
  ++s->stat_pf_misses;
  s->last_pf_wait_ms = NowMs() - t0;
  s->last_pf_state = 3;
  return true;
}

extern "C" int aether_sfm_prefetch_frame(aether_sfm_session_t* s,
                                         const uint8_t* gray, int width,
                                         int height) {
  if (!s || !gray || width <= 0 || height <= 0) return 1;
  if (ExtractPrefetchBlockReason() != 0) return 1;
  if (AbPeriod() > 0 &&
      ((static_cast<int>(s->frames.size()) / AbPeriod()) & 1) == 1) {
    return 1;                                           
  }
  PfEnsureThread(s);
  if (!s->pf_thread.joinable()) return 1;
  std::lock_guard<std::mutex> lk(s->pf_mu);
  if (s->pf_job.valid) return 2;                                      
  s->pf_job.gray.assign(gray, gray + static_cast<size_t>(width) * height);
  s->pf_job.borrow = nullptr;
  s->pf_job.w = width;
  s->pf_job.h = height;
  s->pf_job.digest = PfDigestV1(s->pf_job.gray.data(), width, height);
  if (std::getenv("OFFICIAL_AETHER_PF_DIAG")) {
    std::fprintf(stderr, "[PF-DIAG] enqueue w=%d h=%d digest=%llu\n", width, height,
                 (unsigned long long)s->pf_job.digest);
  }
  s->pf_job.valid = true;
  s->pf_cv.notify_all();
  return 0;
}

aether_sfm_result_t aether_sfm_add_frame(aether_sfm_session_t* s,
                                         const uint8_t* gray, int width,
                                         int height, float fx, float fy,
                                         float cx, float cy,
                                         const double pose_qwxyz[4],
                                         const double pose_t[3],
                                         int* out_frame_id) {
  aether_preclamp_instr_v1::DiscardPending(
      aether_preclamp_instr_v1::PendingDiscardReason::kSessionRejected);
  if (!s || !s->db || !gray || width <= 0 || height <= 0) {
    return AETHER_SFM_ERR_INVALID_ARG;
  }
                                                                                  
                                                                             
                                                                            
                                                                               
  try {
                                                                                  
    aether_gpu_watchdog_v1::WatchdogV1::ArmScope watchdog_armed(
        s->gpu_watchdog);
    s->gpu_timestamp_pending_frame_ready = false;
    const int max_features =
        s->options.max_features > 0 ? s->options.max_features : 2048;

                                                                         
                                                                           
                                                                              
                                                                                 
                                                                             
                                                                              
                                                                         
                                                                            
                                                                             
                                                     
    std::vector<float> xy(static_cast<size_t>(max_features) * 2);
    std::vector<uint8_t> desc(static_cast<size_t>(max_features) * 128);
    std::vector<float> scales(static_cast<size_t>(max_features));
    int n = 0;
    const bool gpu_v2_avail = s->options.use_gpu_extract &&
                              (aether_dsp_sift_extract_gpu_v2 != nullptr);
    const bool gpu_avail =
        s->options.use_gpu_extract && (aether_dsp_sift_extract_gpu != nullptr);
                                                                                        
                                                                                                    
    aether_sfm_session_t::PfResult pf_res;
    const bool pf_adopted = PfAdoptOrRun(s, gray, width, height, &pf_res);
    double extract_ms = 0.0;
    int erc = 1;
    bool have_scales = false;
    if (pf_adopted) {
      s->gpu_watchdog.InProgress();                                           
      xy = std::move(pf_res.xy);
      desc = std::move(pf_res.desc);
      scales = std::move(pf_res.scales);
      n = pf_res.n;
      erc = pf_res.erc;
      have_scales = pf_res.have_scales;
      extract_ms = pf_res.extract_ms;
      std::memcpy(s->last_extract_stages, pf_res.stages,
                  sizeof(s->last_extract_stages));
    } else {
    const double t_extract0 = NowMs();
    s->gpu_watchdog.InProgress();                                          
    if (gpu_v2_avail) {
      erc = aether_dsp_sift_extract_gpu_v2(
          gray, width, height, max_features,                 0, xy.data(),
          desc.data(), scales.data(),                      nullptr,
          max_features, &n);
      have_scales = (erc == 0);
    } else if (gpu_avail) {
      erc = aether_dsp_sift_extract_gpu(gray, width, height, max_features,
                                                        0, xy.data(),
                                        desc.data(), max_features, &n);
      have_scales = false;
    } else {
      erc = aether_dsp_sift_extract_v2(
          gray, width, height, max_features, xy.data(), desc.data(),
          scales.data(),                      nullptr, max_features, &n);
      have_scales = (erc == 0);
    }
    extract_ms = NowMs() - t_extract0;
    s->gpu_watchdog.InProgress();                                          
                                                                       
                                                                       
    for (double& v : s->last_extract_stages) v = 0.0;
    if ((gpu_v2_avail || gpu_avail) && erc == 0 &&
        aether_sed_last_stages != nullptr) {
      aether_sed_last_stages(s->last_extract_stages, 9);
    }
    if (gpu_v2_avail || gpu_avail) {
      PullGpuTimestampAfterExtraction(s);
    }
    }                                          
    if (erc != 0 || n <= 0) {
                                                                            
                                                                             
      s->last_extract_ms = extract_ms;
      s->last_match_ms = 0.0;
      s->last_n_cand = s->last_gpu_matches = s->last_cpu_matches = 0;
                                                                                    
                                                                     
      {
        const char* reason =
            (aether_sed_last_fail_reason != nullptr)
                ? aether_sed_last_fail_reason()
                : "";
        std::string sanitized(reason ? reason : "");
        for (char& c : sanitized) {
          if (c == '"' || c == '\\' || c == '\n') c = ' ';
        }
        AppendMatchFailJsonl(
            s, "{\"t\":" +
                   std::to_string(static_cast<long long>(EpochMs())) +
                   ",\"type\":\"extract_fail\",\"fid\":" +
                   std::to_string(static_cast<long long>(s->frames.size())) +
                   ",\"erc\":" +
                   std::to_string(erc) + ",\"n\":" + std::to_string(n) +
                   ",\"ms\":" + std::to_string(extract_ms) +
                   ",\"reason\":\"" + sanitized + "\"}");
      }
      return AETHER_SFM_ERR_EXTRACT;
    }

    const aether_sfm_result_t add_result = AddFrameFeaturesImpl(
        s, xy.data(), desc.data(), have_scales ? scales.data() : nullptr, n,
        width, height, fx, fy, cx, cy, pose_qwxyz, pose_t, extract_ms,
        out_frame_id);
    if (add_result != AETHER_SFM_OK) {
      aether_preclamp_instr_v1::DiscardPending(
          aether_preclamp_instr_v1::PendingDiscardReason::kSessionRejected);
    }
    return add_result;
  } catch (const std::exception& e) {
    aether_preclamp_instr_v1::DiscardPending(
        aether_preclamp_instr_v1::PendingDiscardReason::kSessionRejected);
                                                                            
                                                                           
                                                                             
                                                                            
    // difference between a genuine coverage problem and an internal fault.
    LOG(WARNING) << "[aether_sfm] add_frame threw: " << e.what();
    {
                                                                            
                                                                           
                 
      char line[512];
      std::snprintf(line, sizeof(line),
                    "{\"t\":%lld,\"type\":\"add_frame_throw\",\"what\":\"%.300s\"}",
                    static_cast<long long>(EpochMs()), e.what());
      AppendMatchFailJsonl(s, line);
    }
    return AETHER_SFM_ERR_INTERNAL;
  } catch (...) {
    aether_preclamp_instr_v1::DiscardPending(
        aether_preclamp_instr_v1::PendingDiscardReason::kSessionRejected);
    LOG(WARNING) << "[aether_sfm] add_frame threw a non-std exception";
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                          
                                                                           
                                                                          
                                                                           
                                                                             
                                                                          
                                                               
aether_sfm_result_t aether_sfm_add_frame_features(
    aether_sfm_session_t* s, const float* xy, const uint8_t* desc,
    int n_keypoints, int width, int height, float fx, float fy, float cx,
    float cy, const double pose_qwxyz[4], const double pose_t[3],
    int* out_frame_id) {
  aether_preclamp_instr_v1::DiscardPending(
      aether_preclamp_instr_v1::PendingDiscardReason::kNonGpuRoute);
  if (!s || !s->db || !xy || !desc || n_keypoints <= 0 || width <= 0 ||
      height <= 0) {
    return AETHER_SFM_ERR_INVALID_ARG;
  }
  s->gpu_timestamp_pending_frame_ready = false;
                                                                            
                                                                             
  return AddFrameFeaturesImpl(s, xy, desc,            nullptr, n_keypoints,
                              width, height, fx, fy, cx, cy, pose_qwxyz,
                              pose_t,                0.0, out_frame_id);
}

                                                                                     
                                                                                                         
                                                                                                  
aether_sfm_result_t aether_sfm_remove_frame(aether_sfm_session_t* s,
                                            int frame_id, char* out_json,
                                            int out_cap) {
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  try {
                                                                   
    if (frame_id < 0 || frame_id >= static_cast<int>(s->frames.size())) {
      return AETHER_SFM_ERR_INVALID_ARG;
    }
    const colmap::image_t image_id = s->frames[frame_id].image_id;
    if (image_id == 0) return AETHER_SFM_ERR_INVALID_ARG;
                                                                        
                                                                             
                                                                              
                                                                              
                                                                              
                                                                              
                                                                      
                                                                             
                                                                           
    if (!PersistArkitPoseSnapshotWithWithdrawal(*s, frame_id)) {
      LOG(WARNING) << "[aether_sfm] remove_frame aborted: could not persist the "
                      "withdrawal marker for frame " << frame_id;
      return AETHER_SFM_ERR_INTERNAL;
    }
                                                                              
                                                                              
                                                                           
    s->visual_loop_index.RemoveFrame(frame_id);
    TailCacheMarkDirty(s,
                       aether::sfm::TailCacheDirtyReasonV1::kRemoveFrame);

    size_t removed_obs = 0, deleted_points = 0, cleared_pairs = 0;

                                                                                             
                                                                                         
                                                                                       
                                                                         
    if (s->live_recon && s->live_recon_ready) {
      const size_t pts_before = s->live_recon->NumPoints3D();
      const colmap::frame_t fid = static_cast<colmap::frame_t>(image_id);
      if (s->live_recon->ExistsFrame(fid) &&
          s->live_recon->Frame(fid).HasPose()) {
        if (s->live_recon->ExistsImage(image_id)) {
          removed_obs = s->live_recon->Image(image_id).NumPoints3D();
        }
        colmap::ObservationManager obs_mgr(*s->live_recon);
        obs_mgr.DeRegisterFrame(fid);
      }
      const size_t pts_after = s->live_recon->NumPoints3D();
      deleted_points = pts_before > pts_after ? pts_before - pts_after : 0;
                                                                                                 
      s->reg_order.erase(
          std::remove(s->reg_order.begin(), s->reg_order.end(), image_id),
          s->reg_order.end());
                                                                                        
                                                                             
      s->draft_pair_points.clear();
      s->draft_pair_obs_offsets.clear();
      s->draft_pair_obs.clear();
                                                                                       
                                                                                                          
                                                                                             
                                               
      {
        std::vector<Eigen::Vector3d> snap;
        snap.reserve(s->live_recon->NumPoints3D());
        for (const auto& [pid, pt] : s->live_recon->Points3D()) {
          snap.push_back(pt.xyz);
        }
        std::lock_guard<std::mutex> lk(s->preview_mutex);
        s->preview_points.swap(snap);
      }
    }

                                                                                     
                                                                                                           
                                                                                              
                                                            
    if (s->db) {
      for (const auto& other : s->frames) {
        if (other.image_id == 0 || other.image_id == image_id) continue;
        const colmap::image_t a = image_id, b = other.image_id;
                                                                                   
                                                                                     
                                                                                              
        if (s->db->ExistsMatches(a, b)) {
          s->db->DeleteMatches(a, b);
          ++cleared_pairs;
        }
        s->db->DeleteTwoViewGeometry(a, b);
      }
    }

                                                                                          
                                                                            
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
    if (aether_gpu_match_descriptor_residency_invalidate != nullptr &&
        s->descriptor_residency_nonce != 0) {
      aether_gpu_match_descriptor_residency_invalidate(
          s->descriptor_residency_nonce, static_cast<uint32_t>(frame_id));
    }
#endif

                                                                                   
    s->frames[frame_id].image_id = 0;
    s->frames[frame_id].descriptors.clear();
    s->frames[frame_id].descriptors.shrink_to_fit();
    s->frames[frame_id].points.clear();
    s->frames[frame_id].points.shrink_to_fit();
    s->frames[frame_id].n_keypoints = 0;
                                                                               
                                                                              
                                                                               
                                                                  

    if (out_json && out_cap > 0) {
      std::snprintf(
          out_json, static_cast<size_t>(out_cap),
          "{\"removed_obs\":%zu,\"deleted_points\":%zu,\"cleared_pairs\":%zu,"
          "\"n_registered\":%zu,\"n_points3d\":%zu}",
          removed_obs, deleted_points, cleared_pairs,
          s->live_recon ? s->live_recon->NumRegImages() : 0,
          s->live_recon ? s->live_recon->NumPoints3D() : 0);
    }
    return AETHER_SFM_OK;
  } catch (const std::exception& e) {
    LOG(WARNING) << "[aether_sfm] remove_frame failed: " << e.what();
    return AETHER_SFM_ERR_INTERNAL;
  } catch (...) {
    return AETHER_SFM_ERR_INTERNAL;
  }
}

aether_sfm_result_t aether_sfm_finalize(aether_sfm_session_t* s, char* out_json,
                                        int out_cap) {
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  try {
                                                                                         
                                                                       
    s->draft_pair_points.clear();
    s->draft_pair_obs_offsets.clear();
    s->draft_pair_obs.clear();
    TailCacheMarkDirty(
        s, aether::sfm::TailCacheDirtyReasonV1::kFinalizeMove);
    if (!RebuildFrameRecordsForResume(s) || !s->live_recon_ready ||
        !s->live_recon || s->live_recon->NumRegImages() < 2 ||
        s->live_recon->NumPoints3D() == 0) {
      return AETHER_SFM_ERR_NOT_REGISTERED;
    }
    const double t0 = NowMs();
    std::shared_ptr<colmap::Reconstruction> work = std::move(s->live_recon);
    s->live_recon_ready = false;
    work->UpdatePoint3DErrors();
                                                                            
                                                                              
                                      
    RefineGlobalBA(s, std::move(work),                true);
    std::shared_ptr<const colmap::Reconstruction> recon;
    {
      std::lock_guard<std::mutex> lock(s->recon_mutex);
      recon = s->recon;
    }
    if (!recon || s->finalize_status.load() != 2) {
      return AETHER_SFM_ERR_NOT_REGISTERED;
    }
    if (out_json && out_cap > 0) {
      std::snprintf(out_json, out_cap,
                    "{\"solve_ms\":%.1f,\"n_models\":1,"
                    "\"n_registered\":%zu,\"n_points3d\":%zu,"
                    "\"reproj_px\":%.4f,\"track_len\":%.3f,"
                    "\"phase1\":\"mandatory_arkit_reuse\"}",
                    NowMs() - t0, recon->NumRegImages(), recon->NumPoints3D(),
                    recon->ComputeMeanReprojectionError(),
                    recon->ComputeMeanTrackLength());
    }
    return AETHER_SFM_OK;
  } catch (const std::exception&) {
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                             
                                                                          
                                                                             
                                                                         
                                                                        
                                                                              
                                                                              
                                                                           
  
                                                                             
                                                                         
                                                                              
                                                                           
                                                                             
                                                                             
                                                                           
                                                                             
                                                   
                                                                              
                                                                            
                                                                       
                                                                            
                                                                        
                                                                
                                                                           
                                                            
                                                                              
                                                                             
                                                                         
                                                                                
                                                                       
                                                                               
                                                                             
                                                                     
aether_sfm_result_t aether_sfm_finalize_async(aether_sfm_session_t* s,
                                              char* out_json, int out_cap) {
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  if (s->refine_thread.joinable()) s->refine_thread.join();                    
  try {
                                                                         
    s->draft_pair_points.clear();
    s->draft_pair_obs_offsets.clear();
    s->draft_pair_obs.clear();
    TailCacheMarkDirty(
        s, aether::sfm::TailCacheDirtyReasonV1::kFinalizeMove);
    const double t_enter = NowMs();
    if (!RebuildFrameRecordsForResume(s)) {
      s->finalize_status.store(3);
      return AETHER_SFM_ERR_NOT_REGISTERED;
    }

                                                                               
                                                                           
                                                             
      
                                                                             
                                                                             
                                                                          
                                                                              
                                                                               
                                                                              
                                                                              
                                                                            
                                                                      
                                                                              
                                                                          
                                                                              
                                                                         
                                                                           
                                                       
    if (s->live_recon_ready && s->live_recon &&
        s->live_recon->NumRegImages() >= 2 &&
        s->live_recon->NumPoints3D() > 0) {
      TailCacheMarkDirty(
          s, aether::sfm::TailCacheDirtyReasonV1::kFinalizeMove);
      std::shared_ptr<colmap::Reconstruction> work = std::move(s->live_recon);
      s->live_recon.reset();                                                               
      s->live_recon_ready = false;                                
      work->UpdatePoint3DErrors();                                    
                                                                             
                                                                         
                                                                               
                                                               
      if (const char* dump_dir = std::getenv("OFFICIAL_AETHER_LIVE_POSE_DUMP");
          dump_dir && dump_dir[0]) {
        try {
          const std::string end_dir = std::string(dump_dir) + "/live_end";
          std::filesystem::create_directories(end_dir);
          work->Write(end_dir);
        } catch (const std::exception& e) {
          LOG(WARNING) << "[aether_sfm] live pose dump failed: " << e.what();
        }
      }
      if (out_json && out_cap > 0) {
        std::snprintf(
            out_json, out_cap,
            "{\"solve_ms\":%.1f,\"n_models\":1,\"n_registered\":%zu,"
            "\"n_points3d\":%zu,\"reproj_px\":%.4f,\"track_len\":%.3f,"
            "\"phase1\":\"live_reuse\"}",
            NowMs() - t_enter, work->NumRegImages(), work->NumPoints3D(),
            work->ComputeMeanReprojectionError(),
            work->ComputeMeanTrackLength());
      }
      {
        std::lock_guard<std::mutex> lk(s->recon_mutex);
        s->recon_manager.reset();                                   
        s->recon.reset();                                                  
      }
                                                                              
                                                                              
                                                                              
                                                                              
                                                                             
      {
        auto& ba_progress = aether::official::ba::GlobalBaProgressV1();
        ba_progress.stage.store(0, std::memory_order_relaxed);
        ba_progress.round.store(0, std::memory_order_relaxed);
        ba_progress.iteration.store(0, std::memory_order_relaxed);
        ba_progress.max_iterations.store(0, std::memory_order_relaxed);
      }
      s->finalize_status.store(1);                                  
      s->refine_thread =
          std::thread(RefineGlobalBA, s, std::move(work),                true);
      return AETHER_SFM_OK;
    }
    s->finalize_status.store(3);
    return AETHER_SFM_ERR_NOT_REGISTERED;
  } catch (const std::exception&) {
    s->finalize_status.store(3);
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                                
                                                       
int aether_sfm_finalize_status(aether_sfm_session_t* s) {
  if (!s) return 3;
  return s->finalize_status.load();
}

                                                                          
                                                                               
                                                                             
                                                                            
                                                                     
                                                      
                                                                              
                                                                         
                    
int aether_sfm_finalize_progress(aether_sfm_session_t* s, int* stage,
                                 int* round, int* iter, int* max_iter) {
  int v_stage = 0, v_round = 0, v_iter = 0, v_max = 0;
  if (s && s->finalize_status.load() == 1) {
    auto& p = aether::official::ba::GlobalBaProgressV1();
    v_stage = p.stage.load(std::memory_order_relaxed);
    v_round = p.round.load(std::memory_order_relaxed);
    v_iter = p.iteration.load(std::memory_order_relaxed);
    v_max = p.max_iterations.load(std::memory_order_relaxed);
                                                       
                                                                
                                                                            
    if (v_stage == 2 && aether_igr_rounds > v_round) {
      v_round = aether_igr_rounds;
    }
  }
  if (stage) *stage = v_stage;
  if (round) *round = v_round;
  if (iter) *iter = v_iter;
  if (max_iter) *max_iter = v_max;
  return 0;
}

                                                                                                                                                                                             
aether_sfm_result_t aether_sfm_frame_health(aether_sfm_session_t* s,
                                            int32_t* out_valid_pairs, int max,
                                            int* out_n) {
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  const int n = static_cast<int>(s->frames.size());
  const int m = n < max ? n : max;
  if (out_valid_pairs) {
    for (int i = 0; i < m; ++i) {
                                                                                              
                                                           
      out_valid_pairs[i] = i < static_cast<int>(s->live_win_valid.size())
                               ? static_cast<int32_t>(s->live_win_valid[i])
                               : 0;
    }
  }
  if (out_n) *out_n = m;
  return AETHER_SFM_OK;
}

aether_sfm_result_t aether_sfm_get_poses(aether_sfm_session_t* s,
                                         aether_sfm_pose_t* out_poses, int cap,
                                         int* out_count) {
  if (!s || !out_count) return AETHER_SFM_ERR_INVALID_ARG;
  std::shared_ptr<const colmap::Reconstruction> recon;
  {
    std::lock_guard<std::mutex> lk(s->recon_mutex);
    recon = s->recon;                                            
  }
  if (!recon) return AETHER_SFM_ERR_NOT_REGISTERED;
  const auto& images = recon->Images();
  int written = 0;
  int total = 0;
  for (const auto& [image_id, image] : images) {
    ++total;
    if (out_poses && written < cap) {
      aether_sfm_pose_t& p = out_poses[written];
                                                                         
      p.frame_id = static_cast<int>(image_id) - 1;
      p.registered = image.HasPose() ? 1 : 0;
      if (image.HasPose()) {
        const colmap::Rigid3d c_from_w = image.CamFromWorld();
        const Eigen::Quaterniond q = c_from_w.rotation();                             
        p.qwxyz[0] = q.w();
        p.qwxyz[1] = q.x();
        p.qwxyz[2] = q.y();
        p.qwxyz[3] = q.z();
        p.t[0] = c_from_w.translation().x();                             
        p.t[1] = c_from_w.translation().y();
        p.t[2] = c_from_w.translation().z();
      } else {
        p.qwxyz[0] = 1.0;
        p.qwxyz[1] = p.qwxyz[2] = p.qwxyz[3] = 0.0;
        p.t[0] = p.t[1] = p.t[2] = 0.0;
      }
      ++written;
    }
  }
  *out_count = total;
  return AETHER_SFM_OK;
}

                                                                                        
                                                                                           
                                                                                         
                                                                                            
                                                                                           
                                                                                          
                                                                               
                                                                                          
                                                                                         
                                                                          
                                                                                   
                                                    
                                                                                       
                                                                                                   
                                                                                  
                                                                                                     
double DeliverMinTriAngleDeg() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_DELIVER_MIN_TRI_ANGLE");
    if (e && e[0]) return std::atof(e);                     
    return 3.0;
  }();
  return cached;
}

                                                                                             
                                                                                  
                                                                                                    
static bool DeliverPointPassesTriAngle(const colmap::Reconstruction& recon,
                                       const colmap::Point3D& point,
                                       double min_cos) {
                                                                                         
                                                                                      
  constexpr int kMaxObs = 32;
  Eigen::Vector3d dirs[kMaxObs];
  int k = 0;
  for (const auto& el : point.track.Elements()) {
    if (k >= kMaxObs) break;
    if (!recon.ExistsImage(el.image_id)) continue;
    const auto& image = recon.Image(el.image_id);
    if (!image.HasPose()) continue;
    const Eigen::Vector3d v = image.ProjectionCenter() - point.xyz;
    const double norm = v.norm();
    if (!(norm > 0.0)) continue;
    dirs[k++] = v / norm;
  }
  if (k < 2) return false;
  for (int i = 0; i < k; ++i) {
    for (int j = i + 1; j < k; ++j) {
      if (dirs[i].dot(dirs[j]) <= min_cos) return true;                              
    }
  }
  return false;
}

                                                                                              
                                                                                            
                                                                                                  
                                                  
bool MirrorGhostEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_MIRROR_GHOST");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                                               
                                                                                 
std::unordered_set<colmap::point3D_t> MirrorGhostKillSet(
    aether_sfm_session* s, const colmap::Reconstruction& recon, bool filter_on,
    double min_cos) {
  std::unordered_set<colmap::point3D_t> killed;
  if (!MirrorGhostEnabled()) return killed;
  const auto& pts = recon.Points3D();
  std::vector<aether_mirror_ghost::P3> mp;
  std::vector<colmap::point3D_t> ids;
  mp.reserve(pts.size());
  ids.reserve(pts.size());
  for (const auto& [pid, point] : pts) {
    if (filter_on && !DeliverPointPassesTriAngle(recon, point, min_cos)) {
      continue;
    }
    mp.push_back({static_cast<float>(point.xyz.x()),
                  static_cast<float>(point.xyz.y()),
                  static_cast<float>(point.xyz.z())});
    ids.push_back(pid);
  }
  const auto res = aether_mirror_ghost::Detect(mp);
  for (const uint32_t i : res.kill) killed.insert(ids[i]);
  const int nk = static_cast<int>(killed.size());
  if (s && nk != s->stat_mirror_ghost_logged) {
    s->stat_mirror_ghost_logged = nk;
    char line[256];
    std::snprintf(line, sizeof(line),
                  "{\"t\":%lld,\"type\":\"mirror_ghost_summary\",\"floor_y\":%.3f,"
                  "\"candidates\":%d,\"clusters\":%d,\"killed_clusters\":%d,"
                  "\"killed\":%d,\"guard\":%d}",
                  static_cast<long long>(NowMs()), res.floor_y, res.candidates,
                  res.clusters, res.killed_clusters, nk, res.guard);
    AppendMatchFailJsonl(s, line);
  }
  return killed;
}

                                                                                              
                                                                                            
                                                                                                         
                                                                                    
                                                      
bool IsolatedFloaterEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ISOLATED_FLOATER");
    return !(e && e[0] == '0');
  }();
  return cached;
}

                                                                                                 
                                                                                    
                                                                                             
                                                                                              
                                                                                                   
std::unordered_set<colmap::point3D_t> IsolatedFloaterKillSet(
    aether_sfm_session* s, const colmap::Reconstruction& recon, bool filter_on,
    double min_cos) {
  std::unordered_set<colmap::point3D_t> killed;
  if (!IsolatedFloaterEnabled()) return killed;
  const auto& pts = recon.Points3D();
  std::vector<aether_isolated_floater::P3> mp, vd;
  std::vector<colmap::point3D_t> ids;
  mp.reserve(pts.size());
  vd.reserve(pts.size());
  ids.reserve(pts.size());
  for (const auto& [pid, point] : pts) {
    if (filter_on && !DeliverPointPassesTriAngle(recon, point, min_cos)) {
      continue;
    }
    Eigen::Vector3d acc = Eigen::Vector3d::Zero();
    for (const auto& el : point.track.Elements()) {
      if (!recon.ExistsImage(el.image_id)) continue;
      const auto& image = recon.Image(el.image_id);
      if (!image.HasPose()) continue;
      const Eigen::Vector3d v = image.ProjectionCenter() - point.xyz;
      const double nrm = v.norm();
      if (!(nrm > 0.0)) continue;
      acc += v / nrm;
    }
    const double an = acc.norm();
    aether_isolated_floater::P3 dir{0.0f, 0.0f, 0.0f};
    if (an > 0.0) {
      dir = {static_cast<float>(acc.x() / an), static_cast<float>(acc.y() / an),
             static_cast<float>(acc.z() / an)};
    }
    mp.push_back({static_cast<float>(point.xyz.x()),
                  static_cast<float>(point.xyz.y()),
                  static_cast<float>(point.xyz.z())});
    vd.push_back(dir);
    ids.push_back(pid);
  }
  const auto res = aether_isolated_floater::Detect(mp, vd);
  for (const uint32_t i : res.kill) killed.insert(ids[i]);
  const int nk = static_cast<int>(killed.size());
  if (s && nk != s->stat_isolated_floater_logged) {
    s->stat_isolated_floater_logged = nk;
    char line[256];
    std::snprintf(line, sizeof(line),
                  "{\"t\":%lld,\"type\":\"isolated_floater_summary\","
                  "\"eps\":%.6f,\"main\":%d,\"clusters\":%d,"
                  "\"killed_clusters\":%d,\"killed\":%d}",
                  static_cast<long long>(NowMs()), res.eps, res.main_size,
                  res.clusters, res.killed_clusters, nk);
    AppendMatchFailJsonl(s, line);
  }
  return killed;
}

                                                                                     
                                                             
                                                                                     
                                                                                                      
double DeliverKillCacheMs() {
  static const double cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_DELIVER_KILL_CACHE_MS");
    if (e && e[0]) return std::atof(e);
    return 2000.0;
  }();
  return cached;
}

std::unordered_set<colmap::point3D_t> DeliverKillSet(
    aether_sfm_session* s, const colmap::Reconstruction& recon, bool filter_on,
    double min_cos, int* out_mg, int* out_if) {
  const double cache_ms = DeliverKillCacheMs();
  const size_t npts = recon.Points3D().size();
  if (s && cache_ms > 0.0) {
    std::lock_guard<std::mutex> lk(s->deliver_kill_mutex);
                                                                                              
    const bool same_snapshot =
        (s->deliver_kill_recon == &recon) && (s->deliver_kill_npts == npts);
    const double age = NowMs() - s->deliver_kill_ms;
    const double drift =
        s->deliver_kill_npts
            ? std::fabs(static_cast<double>(npts) -
                        static_cast<double>(s->deliver_kill_npts)) /
                  static_cast<double>(s->deliver_kill_npts)
            : 1.0;
    if (s->deliver_kill_recon &&
        (same_snapshot || (age < cache_ms && drift < 0.02))) {
      if (out_mg) *out_mg = s->deliver_kill_mg;
      if (out_if) *out_if = s->deliver_kill_if;
      return s->deliver_kill_cache;
    }
  }
  auto kill = MirrorGhostKillSet(s, recon, filter_on, min_cos);
  const int n_mg = static_cast<int>(kill.size());
  if (out_mg) *out_mg = n_mg;
  const auto fl = IsolatedFloaterKillSet(s, recon, filter_on, min_cos);
  const int n_if = static_cast<int>(fl.size());
  if (out_if) *out_if = n_if;
  kill.insert(fl.begin(), fl.end());
  if (s && cache_ms > 0.0) {
    std::lock_guard<std::mutex> lk(s->deliver_kill_mutex);
    s->deliver_kill_cache = kill;
    s->deliver_kill_recon = &recon;
    s->deliver_kill_npts = npts;
    s->deliver_kill_ms = NowMs();
    s->deliver_kill_mg = n_mg;
    s->deliver_kill_if = n_if;
  }
  return kill;
}

aether_sfm_result_t aether_sfm_get_points(aether_sfm_session_t* s,
                                          aether_sfm_point_t** out_points,
                                          int* out_count) {
  if (!s || !out_count) return AETHER_SFM_ERR_INVALID_ARG;
  std::shared_ptr<const colmap::Reconstruction> recon;
  {
    std::lock_guard<std::mutex> lk(s->recon_mutex);
    recon = s->recon;                                            
  }
  if (!recon) return AETHER_SFM_ERR_NOT_REGISTERED;
  const auto& pts = recon->Points3D();

                                                                                     
  const double thr_deg = DeliverMinTriAngleDeg();
  const bool filter_on = thr_deg > 0.0;
  const double min_cos = std::cos(thr_deg * M_PI / 180.0);
  int n_mg = 0, n_if = 0;
  const auto mg_kill = DeliverKillSet(s, *recon, filter_on, min_cos, &n_mg, &n_if);

  int n = 0;
  for (const auto& [point_id, point] : pts) {
    if ((!filter_on || DeliverPointPassesTriAngle(*recon, point, min_cos)) &&
        !mg_kill.count(point_id)) {
      ++n;
    }
  }
  *out_count = n;
  if (!out_points) return AETHER_SFM_OK;                     

  auto* arr = static_cast<aether_sfm_point_t*>(
      std::malloc(static_cast<size_t>(n) * sizeof(aether_sfm_point_t)));
  if (!arr && n > 0) return AETHER_SFM_ERR_INTERNAL;
  int i = 0;
  for (const auto& [point_id, point] : pts) {
    if (filter_on && !DeliverPointPassesTriAngle(*recon, point, min_cos)) {
      continue;
    }
    if (mg_kill.count(point_id)) continue;
    aether_sfm_point_t& o = arr[i++];
    o.x = static_cast<float>(point.xyz.x());
    o.y = static_cast<float>(point.xyz.y());
    o.z = static_cast<float>(point.xyz.z());
    o.r = point.color(0);
    o.g = point.color(1);
    o.b = point.color(2);
    o._pad[0] = o._pad[1] = 0;
  }
  if (filter_on || !mg_kill.empty()) {
    LOG(WARNING) << "[aether_sfm] deliver-filter(get_points): kept " << n
                 << "/" << pts.size() << " (min_tri_angle=" << thr_deg
                 << "deg, mirror_ghost=" << n_mg << ", isolated_floater=" << n_if
                 << ", killed_union=" << mg_kill.size() << ")";
  }
  *out_points = arr;
  return AETHER_SFM_OK;
}

void aether_sfm_points_free(aether_sfm_point_t* points) {
  std::free(points);
}

aether_sfm_result_t aether_sfm_get_points_tracked(
    aether_sfm_session_t* s, aether_sfm_point_t** out_points, int* out_count,
    int32_t** out_obs_offsets, aether_sfm_track_obs_t** out_obs,
    int64_t* out_obs_count) {
  if (!s || !out_points || !out_count || !out_obs_offsets || !out_obs ||
      !out_obs_count) {
    return AETHER_SFM_ERR_INVALID_ARG;
  }
  std::shared_ptr<const colmap::Reconstruction> recon;
  {
    std::lock_guard<std::mutex> lk(s->recon_mutex);
    recon = s->recon;                                            
  }
  if (!recon) return AETHER_SFM_ERR_NOT_REGISTERED;
  const auto& pts = recon->Points3D();

                                                                          
  const double thr_deg = DeliverMinTriAngleDeg();
  const bool filter_on = thr_deg > 0.0;
  const double min_cos = std::cos(thr_deg * M_PI / 180.0);
  int n_mg = 0, n_if = 0;
  const auto mg_kill = DeliverKillSet(s, *recon, filter_on, min_cos, &n_mg, &n_if);

  int n = 0;
  int64_t total_obs = 0;
  for (const auto& [point_id, point] : pts) {
    if (filter_on && !DeliverPointPassesTriAngle(*recon, point, min_cos)) {
      continue;
    }
    if (mg_kill.count(point_id)) continue;
    ++n;
    total_obs += static_cast<int64_t>(point.track.Length());
  }

  auto* arr = static_cast<aether_sfm_point_t*>(
      std::malloc(static_cast<size_t>(n) * sizeof(aether_sfm_point_t)));
  auto* offs = static_cast<int32_t*>(
      std::malloc((static_cast<size_t>(n) + 1) * sizeof(int32_t)));
  auto* obs = static_cast<aether_sfm_track_obs_t*>(std::malloc(
      static_cast<size_t>(total_obs) * sizeof(aether_sfm_track_obs_t)));
  if ((n > 0 && (!arr || !offs)) || (total_obs > 0 && !obs)) {
    std::free(arr);
    std::free(offs);
    std::free(obs);
    return AETHER_SFM_ERR_INTERNAL;
  }

  int i = 0;
  int64_t w = 0;
  for (const auto& [point_id, point] : pts) {
    if (filter_on && !DeliverPointPassesTriAngle(*recon, point, min_cos)) {
      continue;
    }
    if (mg_kill.count(point_id)) continue;
    aether_sfm_point_t& o = arr[i];
    o.x = static_cast<float>(point.xyz.x());
    o.y = static_cast<float>(point.xyz.y());
    o.z = static_cast<float>(point.xyz.z());
    o.r = point.color(0);
    o.g = point.color(1);
    o.b = point.color(2);
    o._pad[0] = o._pad[1] = 0;
    offs[i] = static_cast<int32_t>(w);
    for (const auto& el : point.track.Elements()) {
                                                                          
                                                                               
      if (!recon->ExistsImage(el.image_id)) continue;
      const auto& xy = recon->Image(el.image_id).Point2D(el.point2D_idx).xy;
      aether_sfm_track_obs_t& t = obs[w++];
                                                                         
      t.frame_id = static_cast<int32_t>(el.image_id) - 1;
      t.x = static_cast<float>(xy.x());
      t.y = static_cast<float>(xy.y());
    }
    ++i;
  }
  offs[n] = static_cast<int32_t>(w);
  if (filter_on || !mg_kill.empty()) {
    LOG(WARNING) << "[aether_sfm] deliver-filter(get_points_tracked): kept "
                 << n << "/" << pts.size() << " (min_tri_angle=" << thr_deg
                 << "deg, mirror_ghost=" << n_mg << ", isolated_floater=" << n_if
                 << ", killed_union=" << mg_kill.size() << ")";
  }
  *out_points = arr;
  *out_count = n;
  *out_obs_offsets = offs;
  *out_obs = obs;
  *out_obs_count = w;
  return AETHER_SFM_OK;
}

void aether_sfm_track_obs_free(int32_t* offsets, aether_sfm_track_obs_t* obs) {
  std::free(offsets);
  std::free(obs);
}

                                                                         
                                                                         
                                                                                
                                                                               
                                             
  
                                                                               
                                                                                   
                                                                              
                                                                             
                                                                             
                                                                       
aether_sfm_result_t aether_sfm_get_preview_tracked(
    aether_sfm_session_t* s, aether_sfm_point_t** out_points, int* out_count,
    int32_t** out_obs_offsets, aether_sfm_track_obs_t** out_obs,
    int64_t* out_obs_count) {
  if (!s || !out_points || !out_count || !out_obs_offsets || !out_obs ||
      !out_obs_count) {
    return AETHER_SFM_ERR_INVALID_ARG;
  }
  if (!s->live_recon_ready || !s->live_recon) {
    return AETHER_SFM_ERR_NOT_REGISTERED;
  }
  const colmap::Reconstruction& recon = *s->live_recon;
  const auto& pts = recon.Points3D();

                                                                                           
                                                                                              
                                                                                                           
                                                                  
  if (pts.empty() && !s->draft_pair_points.empty()) {
    const int dn = static_cast<int>(s->draft_pair_points.size());
    const int64_t dobs = static_cast<int64_t>(s->draft_pair_obs.size());
    auto* darr = static_cast<aether_sfm_point_t*>(
        std::malloc(static_cast<size_t>(dn) * sizeof(aether_sfm_point_t)));
    auto* doffs = static_cast<int32_t*>(
        std::malloc((static_cast<size_t>(dn) + 1) * sizeof(int32_t)));
    auto* dob = static_cast<aether_sfm_track_obs_t*>(std::malloc(
        std::max<size_t>(1, static_cast<size_t>(dobs)) *
        sizeof(aether_sfm_track_obs_t)));
    if (!darr || !doffs || !dob) {
      std::free(darr);
      std::free(doffs);
      std::free(dob);
      return AETHER_SFM_ERR_INTERNAL;
    }
    std::memcpy(darr, s->draft_pair_points.data(),
                static_cast<size_t>(dn) * sizeof(aether_sfm_point_t));
    std::memcpy(doffs, s->draft_pair_obs_offsets.data(),
                (static_cast<size_t>(dn) + 1) * sizeof(int32_t));
    if (dobs > 0) {
      std::memcpy(dob, s->draft_pair_obs.data(),
                  static_cast<size_t>(dobs) * sizeof(aether_sfm_track_obs_t));
    }
    *out_points = darr;
    *out_count = dn;
    *out_obs_offsets = doffs;
    *out_obs = dob;
    *out_obs_count = dobs;
    return AETHER_SFM_OK;
  }
  const int n = static_cast<int>(pts.size());

  int64_t total_obs = 0;
  for (const auto& [point_id, point] : pts)
    total_obs += static_cast<int64_t>(point.track.Length());

  auto* arr = static_cast<aether_sfm_point_t*>(
      std::malloc(static_cast<size_t>(n) * sizeof(aether_sfm_point_t)));
  auto* offs = static_cast<int32_t*>(
      std::malloc((static_cast<size_t>(n) + 1) * sizeof(int32_t)));
  auto* obs = static_cast<aether_sfm_track_obs_t*>(std::malloc(
      static_cast<size_t>(total_obs) * sizeof(aether_sfm_track_obs_t)));
  if ((n > 0 && (!arr || !offs)) || (total_obs > 0 && !obs)) {
    std::free(arr);
    std::free(offs);
    std::free(obs);
    return AETHER_SFM_ERR_INTERNAL;
  }

  int i = 0;
  int64_t w = 0;
  for (const auto& [point_id, point] : pts) {
    aether_sfm_point_t& o = arr[i];
    o.x = static_cast<float>(point.xyz.x());
    o.y = static_cast<float>(point.xyz.y());
    o.z = static_cast<float>(point.xyz.z());
    o.r = point.color(0);
    o.g = point.color(1);
    o.b = point.color(2);
    o._pad[0] = o._pad[1] = 0;
    offs[i] = static_cast<int32_t>(w);
    for (const auto& el : point.track.Elements()) {
      if (!recon.ExistsImage(el.image_id)) continue;
      const auto& xy = recon.Image(el.image_id).Point2D(el.point2D_idx).xy;
      aether_sfm_track_obs_t& t = obs[w++];
                                                                                              
      t.frame_id = static_cast<int32_t>(el.image_id) - 1;
      t.x = static_cast<float>(xy.x());
      t.y = static_cast<float>(xy.y());
    }
    ++i;
  }
  offs[n] = static_cast<int32_t>(w);
  *out_points = arr;
  *out_count = n;
  *out_obs_offsets = offs;
  *out_obs = obs;
  *out_obs_count = w;
  return AETHER_SFM_OK;
}

                                                                        
                                                                           
                                                                               
                                                                               
                                                                             
                                                                      
                                                                            
                                                                       
                                                                          
                                                                        
                                                                           
                                                                           
                                                                            
                                                                           
                                                                      
                                                                  
                                                                              
                                                                
                                                                          
                                                                              
                                                                        
bool AsyncPreviewBaEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ASYNC_PREVIEW_BA");
    return e && e[0] == '1';
  }();
  return cached;
}
int AsyncPreviewBaThreads() {
  static const int cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_ASYNC_PREVIEW_BA_THREADS");
    const int v = e ? std::atoi(e) : 2;
    return v > 0 ? v : 2;
  }();
  return cached;
}
                                                                             
                                                                              

                                                                          
  
                                                                            
                                                                          
                                                                             
                                                                             
                                                                      
                                                                            
  
                                                                         
                                                                              
                                                                        
                                                                            
                                                                            
                                                                        
                                                                         
                                                                         
                                                                    
bool AdoptAsyncPreviewRefinement(aether_sfm_session* s,
                                 std::shared_ptr<colmap::Reconstruction> refined,
                                 int snapshot_nframes) {
  if (!refined) return false;
  const int n_now = static_cast<int>(s->frames.size());
                                                          
  TailCacheMarkDirty(
      s, aether::sfm::TailCacheDirtyReasonV1::kModelReplacement);
  s->live_recon = std::move(refined);
  if (snapshot_nframes >= n_now) return true;                      
                                                                       
    
                                                                           
                                                                         
                                                                            
                                                                           
                                                                    
                                                                     
                                                                    
                                                                          
                                                                       
    
                                                                    
                                                         
                                                     
                                                                           
                                                                             
                         
  std::optional<colmap::Rigid3d> world_old_from_world_new;
  for (int i = AsyncPreviewBaPropagate() ? snapshot_nframes - 1 : -1; i >= 0;
       --i) {
    const colmap::frame_t fid = static_cast<colmap::frame_t>(
        s->frames[i].image_id);                                         
    const auto snap_it = s->apvba_snapshot_poses.find(fid);
    if (snap_it == s->apvba_snapshot_poses.end()) continue;
    if (!s->live_recon->ExistsFrame(fid)) continue;                       
    const colmap::Frame& refined_ref = s->live_recon->Frame(fid);
    if (!refined_ref.HasPose()) continue;
    world_old_from_world_new =
        colmap::Inverse(snap_it->second) * refined_ref.RigFromWorld();
    const Eigen::Vector3d t = world_old_from_world_new->translation();
    s->stat_apvba_last_corr_mm = t.norm() * 1000.0;
    const double qw = std::abs(world_old_from_world_new->rotation().w());
    s->stat_apvba_last_corr_deg =
        2.0 * std::acos(std::min(1.0, qw)) * 180.0 / M_PI;
    break;
  }
                                                              
  std::vector<colmap::image_t> replayed;
  replayed.reserve(static_cast<size_t>(n_now - snapshot_nframes));
  for (int i = snapshot_nframes; i < n_now; ++i) {
    const FrameRecord& rec = s->frames[i];
    if (!rec.has_pose || rec.image_id == 0) continue;
    if (s->live_recon->ExistsImage(rec.image_id)) continue;
    s->live_recon->AddCameraWithTrivialRig(rec.camera);
    colmap::Image rimg;
    rimg.SetImageId(rec.image_id);
    char name[64];
    std::snprintf(name, sizeof(name), "frame_%06d.jpg", rec.frame_id);
    rimg.SetName(name);
    rimg.SetCameraId(rec.camera_id);
    rimg.SetPoints2D(rec.points);
                                                                           
                                                                           
                                                                               
                                                                      
    const colmap::Rigid3d pose =
        world_old_from_world_new
            ? rec.cam_from_world * (*world_old_from_world_new)
            : rec.cam_from_world;
    s->live_recon->AddImageWithTrivialFrame(std::move(rimg), pose);
    replayed.push_back(rec.image_id);
  }
  if (replayed.empty()) return true;
                                                                            
                                                                               
  colmap::DatabaseCache::Options cache_options;
  cache_options.min_num_matches = 15;
  cache_options.load_all_images = true;
  auto cache = colmap::DatabaseCache::Create(*s->db, cache_options);
  colmap::IncrementalPipelineOptions official_options;
  official_options.min_num_matches = 15;
  official_options.triangulation.ignore_two_view_tracks =
      TriIgnoreTwoViewTracks();
  official_options.load_all_images = true;
  ApplyBirthGateAB(&official_options);                               
  colmap::IncrementalMapper mapper(cache);
  mapper.BeginReconstruction(s->live_recon);
  for (const colmap::image_t image_id : replayed) {
    mapper.TriangulateImage(official_options.Triangulation(), image_id);
  }
  mapper.EndReconstruction(            false);
  return true;
}

#if defined(AETHER_TAIL_CACHE_FAULT_TEST_HOOKS)
                                                                           
                                                                             
                                                                             
                                                        
int aether_sfm_test_tail_cache_inject_v1(aether_sfm_session_t* s, int kind) {
  if (!s || !TailCacheActive()) return -1;
  if (kind == 1) {                                   
    colmap::image_pair_t pair_id = 0;
    {
      std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
      if (s->tail_cache_seen_db_pairs.empty()) return -2;
      pair_id = *s->tail_cache_seen_db_pairs.begin();
    }
    const auto [image1, image2] = colmap::PairIdToImagePair(pair_id);
    return TailCacheBeginPairWrite(s, image1, image2) ? -3 : 0;
  }
  if (kind == 2) {                                                   
    colmap::image_t image1 = 0;
    colmap::image_t image2 = 0;
    {
      std::lock_guard<std::mutex> lock(s->tail_cache_mutex);
      for (size_t i = 0; i < s->frames.size() && image1 == 0; ++i) {
        for (size_t j = i + 1; j < s->frames.size(); ++j) {
          const colmap::image_t a = s->frames[i].image_id;
          const colmap::image_t b = s->frames[j].image_id;
          if (a == 0 || b == 0 || a == b) continue;
          const colmap::image_pair_t candidate =
              colmap::ImagePairToPairId(a, b);
          if (s->tail_cache_seen_db_pairs.count(candidate) == 0) {
            image1 = a;
            image2 = b;
            break;
          }
        }
      }
    }
    if (image1 == 0 || image2 == 0) return -4;
    return TailCacheBeginPairWrite(s, image1, image2,
                                                  true)
               ? 0
               : -5;
  }
  if (kind == 3) {                                                    
    if (!s->live_recon) return -6;
    auto exact_copy =
        std::make_shared<colmap::Reconstruction>(*s->live_recon);
    return AdoptAsyncPreviewRefinement(
               s, std::move(exact_copy), static_cast<int>(s->frames.size()))
               ? 0
               : -7;
  }
  if (kind == 4) {                                                        
    TailCacheMarkDirty(
        s, aether::sfm::TailCacheDirtyReasonV1::kExceptionRetry);
    return 0;
  }
  return -8;
}
#endif                                       

aether_sfm_result_t AsyncPreviewBaTick(aether_sfm_session_t* s) {
                                                                            
                                                
  bool merged = false;
  if (s->async_pvba_done.load(std::memory_order_acquire)) {
    std::shared_ptr<colmap::Reconstruction> refined;
    {
      std::lock_guard<std::mutex> lk(s->async_pvba_mutex);
      refined.swap(s->async_pvba_result);
    }
    if (s->async_pvba_thread.joinable()) s->async_pvba_thread.join();
    s->async_pvba_done.store(false, std::memory_order_relaxed);
    s->async_pvba_running.store(false, std::memory_order_relaxed);
    if (refined) {
      const double t_m = NowMs();
      try {
        merged = AdoptAsyncPreviewRefinement(s, std::move(refined),
                                             s->apvba_snapshot_nframes);
      } catch (...) {
                                                                            
                                                                               
                                                                  
        merged = false;
      }
      s->stat_apvba_merge_ms_total += NowMs() - t_m;
      if (merged) {
        ++s->stat_apvba_merges;
        s->apvba_consecutive_drops = 0;
                                                                         
                                                                               
                                                          
        s->apvba_kick_armed = true;
      } else {
        ++s->stat_apvba_dropped;
        ++s->apvba_consecutive_drops;
      }
    } else {
      ++s->stat_apvba_dropped;
      ++s->apvba_consecutive_drops;
    }
                                                                         
                                                                                
                                                                       
    if (s->apvba_consecutive_drops >= 3) {
      s->apvba_skip_kicks = 4;
    } else if (s->apvba_consecutive_drops == 2) {
      s->apvba_skip_kicks = 2;
    }
  }
                                                                           
                                                                         
                                                    
  if (s->apvba_skip_kicks > 0) {
    --s->apvba_skip_kicks;                            
  } else if (AsyncPreviewBaMatchCadence() && !s->apvba_kick_armed) {
                                                                          
  } else if (!s->async_pvba_running.load(std::memory_order_relaxed)) {
    try {
      const double t_c = NowMs();
      auto snap_model =
          std::make_shared<colmap::Reconstruction>(*s->live_recon);
      s->stat_apvba_copy_ms_total += NowMs() - t_c;
                                                                          
                                                                  
      s->apvba_snapshot_nframes = static_cast<int>(s->frames.size());
                                                                         
      s->apvba_snapshot_poses.clear();
      for (const auto& [fid, frame] : snap_model->Frames()) {
        if (frame.HasPose()) s->apvba_snapshot_poses[fid] = frame.RigFromWorld();
      }
      s->async_pvba_running.store(true, std::memory_order_relaxed);
      s->apvba_kick_armed = false;                                   
      ++s->stat_apvba_kicks;
                                                                             
                                                                           
                                                                            
                                                                  
                                                                     
                                                                            
                                                                        
      colmap::DatabaseCache::Options kick_cache_options;
      kick_cache_options.min_num_matches = 15;
      kick_cache_options.load_all_images = true;
      auto kick_cache = colmap::DatabaseCache::Create(*s->db,
                                                      kick_cache_options);
      s->async_pvba_thread = std::thread([s, snap_model, kick_cache] {
        aether::official::ba::ScopedBaSessionAggregateBindingV1
            ba_session_binding(&s->ba_ptol_aggregate,
                               &s->ba_ptol_receipts);
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
#endif
        const double t0 = NowMs();
        try {
          auto cache = kick_cache;
          colmap::IncrementalPipelineOptions official_options;
          official_options.min_num_matches = 15;
          official_options.triangulation.ignore_two_view_tracks =
              TriIgnoreTwoViewTracks();
          official_options.load_all_images = true;
          official_options.ba_refine_focal_length = false;
          official_options.ba_refine_principal_point = false;
          official_options.ba_refine_extra_params = false;
          official_options.mapper.ba_local_num_images = 6;
          official_options.mapper.ba_global_ignore_redundant_points3D = true;
          ApplyBirthGateAB(&official_options);                               
          auto ba_opts = official_options.GlobalBundleAdjustment();
          if (ba_opts.ceres) {
            ba_opts.ceres->solver_options.num_threads = AsyncPreviewBaThreads();
          }
          colmap::IncrementalMapper mapper(cache);
          mapper.BeginReconstruction(snap_model);
          {
            aether::official::ba::ScopedGlobalBaSolveV1 global_ba_scope;
            if (AsyncPreviewBaParamOnly()) {
                                                                          
                                                                         
              mapper.AdjustGlobalBundle(official_options.Mapper(), ba_opts);
            } else {
              mapper.IterativeGlobalRefinement(
                  official_options.ba_global_max_refinements,
                  official_options.ba_global_max_refinement_change,
                  official_options.Mapper(), ba_opts,
                  official_options.Triangulation(),
                                               false);
              mapper.FilterFrames(official_options.Mapper());
            }
          }
          mapper.EndReconstruction(            false);
          snap_model->UpdatePoint3DErrors();
          {
            std::lock_guard<std::mutex> lk(s->async_pvba_mutex);
            s->async_pvba_result = snap_model;
          }
        } catch (const std::exception& e) {
                                                                              
                                                                         
          std::lock_guard<std::mutex> lk(s->async_pvba_mutex);
          s->apvba_last_error = e.what();
        } catch (...) {
          std::lock_guard<std::mutex> lk(s->async_pvba_mutex);
          s->apvba_last_error = "(non-std exception)";
        }
        s->stat_apvba_last_ba_ms = NowMs() - t0;
        s->async_pvba_done.store(true, std::memory_order_release);
      });
    } catch (...) {
      s->async_pvba_running.store(false, std::memory_order_relaxed);
    }
  }
  if (!merged) return AETHER_SFM_ERR_NOT_REGISTERED;                        
                                                                               
                                                                             
  std::vector<Eigen::Vector3d> snap;
  snap.reserve(s->live_recon->NumPoints3D());
  for (const auto& [pid, pt] : s->live_recon->Points3D()) snap.push_back(pt.xyz);
  {
    std::lock_guard<std::mutex> lk(s->preview_mutex);
    s->preview_points.swap(snap);
  }
  return AETHER_SFM_OK;
}

                                                                  
  
                                                                                
                                                                                   
                                                                              
                                                                 
aether_sfm_result_t aether_sfm_global_refine(aether_sfm_session_t* s) {
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  aether::official::ba::ScopedBaSessionAggregateBindingV1 ba_session_binding(
      &s->ba_ptol_aggregate, &s->ba_ptol_receipts);
  if (!s->live_recon_ready || !s->live_recon ||
      s->live_recon->NumRegImages() < 3)
    return AETHER_SFM_ERR_NOT_REGISTERED;
                                                                           
                                                                         
                      
  if (AsyncPreviewBaEnabled()) {
    const aether_sfm_result_t result = AsyncPreviewBaTick(s);
    if (result == AETHER_SFM_OK) {
      AppendLiveCloudSnapshotDiagnosticsV2(s, "post_global_ba");
    }
    return result;
  }
  try {
    colmap::DatabaseCache::Options cache_options;
    cache_options.min_num_matches = 15;
    cache_options.load_all_images = true;
    auto cache = colmap::DatabaseCache::Create(*s->db, cache_options);

    colmap::IncrementalPipelineOptions official_options;
    official_options.min_num_matches = 15;
    official_options.triangulation.ignore_two_view_tracks =
        TriIgnoreTwoViewTracks();
    official_options.load_all_images = true;
    official_options.ba_refine_focal_length = false;
    official_options.ba_refine_principal_point = false;
    official_options.ba_refine_extra_params = false;
    official_options.mapper.ba_local_num_images = 6;

    // capture-time solve. IncrementalMapper::AdjustGlobalBundle (upstream
                                                                             
                                                                          
                                                                        
    // coverage gain is below the threshold — and (b) a second pass that
                                                                           
                                                                              
                                                                            
    // block that OOM-killed a 126-frame capture on device. Upstream gates it
                                                                          
                                                                          
    official_options.mapper.ba_global_ignore_redundant_points3D = true;
    ApplyBirthGateAB(&official_options);                               

    colmap::IncrementalMapper mapper(cache);
    mapper.BeginReconstruction(s->live_recon);
    {
      aether::official::ba::ScopedGlobalBaSolveV1 global_ba_scope;
      mapper.IterativeGlobalRefinement(
          official_options.ba_global_max_refinements,
          official_options.ba_global_max_refinement_change,
          official_options.Mapper(), official_options.GlobalBundleAdjustment(),
          official_options.Triangulation(),
                                       false);
    }
    mapper.FilterFrames(official_options.Mapper());
    mapper.EndReconstruction(            false);
    s->live_recon->UpdatePoint3DErrors();

                                                         
    std::vector<Eigen::Vector3d> snap;
    snap.reserve(s->live_recon->NumPoints3D());
    for (const auto& [pid, pt] : s->live_recon->Points3D()) snap.push_back(pt.xyz);
    {
      std::lock_guard<std::mutex> lk(s->preview_mutex);
      s->preview_points.swap(snap);
    }
    AppendLiveCloudSnapshotDiagnosticsV2(s, "post_global_ba");
    return AETHER_SFM_OK;
  } catch (const std::exception&) {
    return AETHER_SFM_ERR_INTERNAL;                                       
  }
}

                                                                                 
                                                                                 
                                                                             
                                                                                    
                                                  
aether_sfm_result_t aether_sfm_get_preview_points(aether_sfm_session_t* s,
                                                  float* out_xyz, int cap,
                                                  int* out_count) {
  if (!s || !out_count) return AETHER_SFM_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lk(s->preview_mutex);
  const int n = static_cast<int>(s->preview_points.size());
  *out_count = n;
  if (!out_xyz) return AETHER_SFM_OK;                            
  const int m = (n < cap) ? n : cap;
  for (int i = 0; i < m; ++i) {
    const Eigen::Vector3d& p = s->preview_points[i];
    out_xyz[3 * i + 0] = static_cast<float>(p.x());
    out_xyz[3 * i + 1] = static_cast<float>(p.y());
    out_xyz[3 * i + 2] = static_cast<float>(p.z());
  }
  return AETHER_SFM_OK;
}

                                                                                
                                                                             
                                                     
                                                                                
                                                                               
void aether_sfm_debug_last(aether_sfm_session_t* s, double* extract_ms,
                           double* match_ms, int* n_cand, int* gpu_matches,
                           int* cpu_matches) {
  if (!s) return;
  if (extract_ms) *extract_ms = s->last_extract_ms;
  if (match_ms) *match_ms = s->last_match_ms;
  if (n_cand) *n_cand = s->last_n_cand;
  if (gpu_matches) *gpu_matches = s->last_gpu_matches;
  if (cpu_matches) *cpu_matches = s->last_cpu_matches;
}

                                                                                
                                                                     
                                                                               
                                                                           
                                                                              
                                                                               
void aether_sfm_stream_stats(aether_sfm_session_t* s, int64_t* tvg_pairs,
                             int64_t* raw_pairs, int64_t* grow_accepted,
                             int64_t* grow_rejected, int64_t* reproj_filtered,
                             int64_t* tri_filtered,
                             int64_t* grow_reject_cheirality,
                             int64_t* grow_reject_reproj,
                             int64_t* create_reject_cheirality,
                             int64_t* create_reject_tri_angle,
                             int64_t* create_reject_reproj,
                             int64_t* already_assigned,
                             int64_t* merge_needed,
                             int64_t* merge_accepted,
                             int64_t* merge_rejected,
                             int64_t* spatial_considered,
                             int64_t* spatial_attempted,
                             int64_t* spatial_written,
                             int64_t* spatial_inliers,
                             int64_t* spatial_anchor_attempted,
                             int64_t* spatial_anchor_passed,
                             int64_t* spatial_regions_confirmed,
                             int64_t* spatial_expanded_attempted,
                             int64_t* spatial_guided_pairs,
                             int64_t* spatial_guided_inliers,
                             int64_t* spatial_quadratic_attempted,
                             int64_t* spatial_quadratic_written,
                             int64_t* spatial_budget_skipped,
                             int64_t* temporal_detail_pairs,
                             int64_t* temporal_detail_matches,
                             int64_t* temporal_detail_created,
                             int64_t* temporal_detail_grown,
                             int64_t* temporal_detail_reject_cheirality,
                             int64_t* temporal_detail_reject_reproj,
                             int64_t* temporal_detail_reject_tri_angle,
                             int64_t* temporal_detail_conflicts) {
  if (!s) return;
  if (tvg_pairs) *tvg_pairs = s->stat_tvg_inlier_pairs;
  if (raw_pairs) *raw_pairs = s->stat_raw_pairs;
  if (grow_accepted) *grow_accepted = s->stat_grow_accepted;
  if (grow_rejected) *grow_rejected = s->stat_grow_rejected;
  if (reproj_filtered) *reproj_filtered = s->stat_reproj_filtered;
  if (tri_filtered) *tri_filtered = s->stat_tri_filtered;
  if (grow_reject_cheirality)
    *grow_reject_cheirality = s->stat_grow_reject_cheirality;
  if (grow_reject_reproj) *grow_reject_reproj = s->stat_grow_reject_reproj;
  if (create_reject_cheirality)
    *create_reject_cheirality = s->stat_create_reject_cheirality;
  if (create_reject_tri_angle)
    *create_reject_tri_angle = s->stat_create_reject_tri_angle;
  if (create_reject_reproj)
    *create_reject_reproj = s->stat_create_reject_reproj;
  if (already_assigned) *already_assigned = s->stat_already_assigned;
  if (merge_needed) *merge_needed = s->stat_merge_needed;
  if (merge_accepted) *merge_accepted = s->stat_merge_accepted;
  if (merge_rejected) *merge_rejected = s->stat_merge_rejected;
  if (spatial_considered)
    *spatial_considered = s->stat_spatial_pairs_considered;
  if (spatial_attempted) *spatial_attempted = s->stat_spatial_pairs_attempted;
  if (spatial_written) *spatial_written = s->stat_spatial_pairs_written;
  if (spatial_inliers) *spatial_inliers = s->stat_spatial_inliers;
  if (spatial_anchor_attempted)
    *spatial_anchor_attempted = s->stat_spatial_anchor_attempted;
  if (spatial_anchor_passed)
    *spatial_anchor_passed = s->stat_spatial_anchor_passed;
  if (spatial_regions_confirmed)
    *spatial_regions_confirmed = s->stat_spatial_regions_confirmed;
  if (spatial_expanded_attempted)
    *spatial_expanded_attempted = s->stat_spatial_expanded_attempted;
  if (spatial_guided_pairs)
    *spatial_guided_pairs = s->stat_spatial_guided_pairs;
  if (spatial_guided_inliers)
    *spatial_guided_inliers = s->stat_spatial_guided_inliers;
  if (spatial_quadratic_attempted)
    *spatial_quadratic_attempted = s->stat_spatial_quadratic_attempted;
  if (spatial_quadratic_written)
    *spatial_quadratic_written = s->stat_spatial_quadratic_written;
  if (spatial_budget_skipped)
    *spatial_budget_skipped = s->stat_spatial_budget_skipped;
  if (temporal_detail_pairs)
    *temporal_detail_pairs = s->stat_temporal_detail_pairs;
  if (temporal_detail_matches)
    *temporal_detail_matches = s->stat_temporal_detail_matches;
  if (temporal_detail_created)
    *temporal_detail_created = s->stat_temporal_detail_created;
  if (temporal_detail_grown)
    *temporal_detail_grown = s->stat_temporal_detail_grown;
  if (temporal_detail_reject_cheirality) {
    *temporal_detail_reject_cheirality =
        s->stat_temporal_detail_reject_cheirality;
  }
  if (temporal_detail_reject_reproj)
    *temporal_detail_reject_reproj = s->stat_temporal_detail_reject_reproj;
  if (temporal_detail_reject_tri_angle) {
    *temporal_detail_reject_tri_angle =
        s->stat_temporal_detail_reject_tri_angle;
  }
  if (temporal_detail_conflicts)
    *temporal_detail_conflicts = s->stat_temporal_detail_conflicts;
}

                                                                               
                                                                                
                                                                               
                                                                              
                                                                            
                  
void aether_sfm_live_diag(aether_sfm_session_t* s, double* mean_reproj_px,
                          int64_t* n_points, int64_t* n_track3plus,
                          int64_t* n_obs, int64_t* merge_reject_shared_image,
                          int64_t* merge_reject_reproj,
                          int64_t* merge_reject_missing) {
  if (!s) return;
  if (merge_reject_shared_image)
    *merge_reject_shared_image = s->stat_merge_reject_shared_image;
  if (merge_reject_reproj) *merge_reject_reproj = s->stat_merge_reject_reproj;
  if (merge_reject_missing)
    *merge_reject_missing = s->stat_merge_reject_missing;
  int64_t pts = 0, track3 = 0, obs = 0;
  double err_sum = 0.0;
  int64_t err_n = 0;
                                                                            
                                                                 
  if (!s->live_recon) {
    if (n_points) *n_points = 0;
    if (n_track3plus) *n_track3plus = 0;
    if (n_obs) *n_obs = 0;
    if (mean_reproj_px) *mean_reproj_px = 0.0;
    return;
  }
  try {
    for (const auto& [pid, pt] : s->live_recon->Points3D()) {
      ++pts;
      const size_t len = pt.track.Length();
      obs += static_cast<int64_t>(len);
      if (len >= 3) ++track3;
      for (const auto& el : pt.track.Elements()) {
        if (!s->live_recon->ExistsImage(el.image_id)) continue;
        const colmap::Image& image = s->live_recon->Image(el.image_id);
        if (!image.HasPose() || el.point2D_idx >= image.NumPoints2D()) continue;
        const Eigen::Vector3d x_cam = image.CamFromWorld() * pt.xyz;
        if (x_cam.z() <= 0.0) continue;
        const colmap::Camera* cam = image.CameraPtr();
        if (!cam) continue;
        const std::optional<Eigen::Vector2d> px = cam->ImgFromCam(x_cam);
        if (!px) continue;
        err_sum += (*px - image.Point2D(el.point2D_idx).xy).norm();
        ++err_n;
      }
    }
  } catch (const std::exception&) {
                                                      
  }
  if (n_points) *n_points = pts;
  if (n_track3plus) *n_track3plus = track3;
  if (n_obs) *n_obs = obs;
  if (mean_reproj_px) *mean_reproj_px = err_n > 0 ? err_sum / err_n : 0.0;
}


                                                                             
                                                                             
                                                                          
                                                         
                                                                      
void aether_sfm_candidate_stats(aether_sfm_session_t* s,
                                int64_t* spatial_first_pairs,
                                int64_t* temporal_fallback_pairs) {
  if (!s) return;
  if (spatial_first_pairs)
    *spatial_first_pairs = s->stat_cand_spatial_first_pairs;
  if (temporal_fallback_pairs)
    *temporal_fallback_pairs = s->stat_cand_temporal_fallback_pairs;
}

                                                                        
                                                                             
                                                                         
                                                                              
                                                                                  
void aether_sfm_match_fail_stats(aether_sfm_session_t* s,
                                 int64_t* gpu_fail_total,
                                 int64_t* gpu_fail_by_rc,
                                 int64_t* gpu_fail_max_streak,
                                 int64_t* rematch_starved_frames,
                                 int64_t* rematch_candidates,
                                 int64_t* rematch_attempted,
                                 int64_t* rematch_written,
                                 int64_t* rematch_inliers,
                                 int64_t* rematch_failed) {
  if (!s) return;
  if (gpu_fail_total) *gpu_fail_total = s->stat_gpu_match_fail_total;
  if (gpu_fail_by_rc) {
    for (int i = 0; i < 8; ++i) gpu_fail_by_rc[i] = s->stat_gpu_match_fail_by_rc[i];
  }
  if (gpu_fail_max_streak)
    *gpu_fail_max_streak = s->stat_gpu_match_fail_max_streak;
  if (rematch_starved_frames)
    *rematch_starved_frames = s->stat_finalize_rematch_starved_frames;
  if (rematch_candidates)
    *rematch_candidates = s->stat_finalize_rematch_candidates;
  if (rematch_attempted)
    *rematch_attempted = s->stat_finalize_rematch_attempted;
  if (rematch_written) *rematch_written = s->stat_finalize_rematch_written;
  if (rematch_inliers) *rematch_inliers = s->stat_finalize_rematch_inliers;
  if (rematch_failed) *rematch_failed = s->stat_finalize_rematch_failed;
}

                                                                         
                                                                                 
                                                                     
                                                                                        
                                                                              
                                   
void aether_sfm_set_thermal_state(aether_sfm_session_t* s, int state) {
  if (!s) return;
  s->thermal_state.store((state >= 0 && state <= 3) ? state : -1,
                         std::memory_order_relaxed);
}

                                                                            
                                                                        
                           
void aether_sfm_thermal_throttle_stats(aether_sfm_session_t* s,
                                       int64_t* throttled_frames) {
  if (throttled_frames) *throttled_frames = s ? s->stat_thermal_throttled_frames : 0;
}

                                                                           
                                                                             
                                                                           
                                                                             
                                                                          
                                                                                
                                                                            
                                                                                
                                                                            
                                                        
  
         
                                                          
                                                              
                                                                         
                                                                                
                                                                                 
                                                                            
                                                                         
                                                                         
                                                                       
                                                                             
                                                              
                                                                              
                                                                           
                                                                    
                                                                                 
                                                                             
                                                                          
                                                                         
                                                                    
                                                                     
                                                                          
                                                                        
                                                                          
                                                                            
                                                                      
                                                                           
                                              
                                                         
                                                                                
                                                                          
                                                                                  
                                                                               
                                                                          
                                                                        
                                                                               
                                                                              
                                                                                 
                                                                            
                                                                          
                                                                                 
constexpr int kRepayThermal2MaxPairs = 8;
int RepayThermal2CleanN() {
  static const int cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_REPAY_THERMAL2_CLEAN_N")) {
      const int v = std::atoi(e);
      if (v >= 0) return v;
    }
    return 4;                                                                  
  }();
  return cached;
}

                                                                                 
                                                                    
                                                                          
                                                 
static bool QuadraticPrepayEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_QUADRATIC_PREPAY");
    return !(e && e[0] == '0');
  }();
  return cached;
}

// Prepays the upstream quadratic-overlap pairs (i, i+2^k), 2^k > live K
                                                                         
                                                                           
                                                                        
                                                                    
                                                                        
                                                                               
                                                                     
                                                                   
                                                                      
                                                                           
                                                                               
                                                                            
                                                  
static int PrepayQuadraticTick(aether_sfm_session* s, int max_pairs) {
  if (!QuadraticPrepayEnabled()) return 0;
  if (!s || !s->db || max_pairs <= 0) return 0;
  const int overlap = OfficialQuadraticOverlap();
  if (overlap <= 0 || s->camera_id == 0 || s->frames.size() < 2) return 0;
                                                                        
                                                                        
                                
  if (s->thermal_state.load(std::memory_order_relaxed) >= 3) return 0;
  const bool gpu_avail =
      s->options.use_gpu_match && (aether_gpu_match_gemm_pairs != nullptr);
                                                                            
                                            
  if (s->options.use_gpu_match && !gpu_avail) return 0;
  int attempted = 0;
  try {
                                                                         
                                                                        
                                                                      
    const int K = s->options.k_neighbors > 0 ? s->options.k_neighbors : 12;
    const int n = static_cast<int>(s->frames.size());
    for (; s->prepay_scan < static_cast<size_t>(n); ++s->prepay_scan) {
      const int j = static_cast<int>(s->prepay_scan);
      for (int k = 0; k < overlap; ++k) {
        const int64_t gap = 1ll << k;
        if (gap <= K) continue;
        if (gap > j) break;                                                  
        s->prepay_due.emplace_back(j - static_cast<int>(gap), j);
      }
    }
                                                                      
                                                                        
                  
    const double ratio =
        s->options.match_max_ratio > 0 ? s->options.match_max_ratio : 0.8;
    const colmap::TwoViewGeometryOptions tvg_options;                    
    std::vector<uint32_t> pair_buf;
    while (attempted < max_pairs && !s->prepay_due.empty()) {
      const auto [i, j] = s->prepay_due.front();
      s->prepay_due.pop_front();
      const FrameRecord& fa = s->frames[i];
      const FrameRecord& fb = s->frames[j];
      const colmap::image_t img1 = fa.image_id;
      const colmap::image_t img2 = fb.image_id;
      if (img1 == 0 || img2 == 0 || img1 == img2) continue;
      if (fa.descriptors.empty() || fb.descriptors.empty() ||
          fa.n_keypoints <= 0 || fb.n_keypoints <= 0) {
        continue;                                                               
      }
      if (s->db->ExistsMatches(img1, img2) ||
          s->db->ExistsTwoViewGeometry(img1, img2)) {
        continue;
      }
      const int cap =
          fa.n_keypoints < fb.n_keypoints ? fa.n_keypoints : fb.n_keypoints;
      pair_buf.resize(static_cast<size_t>(cap) * 2);
      int num_matches = 0;
      ++attempted;
      ++s->stat_prepay_attempted;
                                                                             
      const int mrc =
          gpu_avail
              ? GpuMatchGemmPairsRetry(
                    s, fa.frame_id, fa.descriptors.data(), fa.n_keypoints,
                    fb.frame_id, fb.descriptors.data(), fb.n_keypoints, ratio,
                    pair_buf.data(), cap, &num_matches)
              : aether_sift_match_pairs(fa.descriptors.data(), fa.n_keypoints,
                                        fb.descriptors.data(), fb.n_keypoints,
                                        ratio, pair_buf.data(), cap,
                                        &num_matches);
      if (mrc != 0) continue;                                              
      if (num_matches <= 0) continue;                               
      colmap::FeatureMatches matches(num_matches);
      for (int m = 0; m < num_matches; ++m) {
        matches[m].point2D_idx1 = pair_buf[2 * m];
        matches[m].point2D_idx2 = pair_buf[2 * m + 1];
      }
      const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
          fa, fa.points, fb, fb.points, matches, tvg_options);
      if (!MandatoryGravityTvgPersistable(tvg)) continue;
      const bool tail_pair_first_write =
          TailCacheBeginPairWrite(s, img1, img2,                true);
      s->db->WriteMatches(img1, img2, matches);
      s->db->WriteTwoViewGeometry(img1, img2, tvg.geometry);
      TailCacheCommitPair(s, img1, img2, tvg.geometry,
                          tail_pair_first_write);
      ++s->stat_prepay_written;
    }
  } catch (const std::exception& e) {
                                                                             
    LOG(WARNING) << "[aether_sfm] quadratic prepay aborted: " << e.what();
  } catch (...) {
    LOG(WARNING) << "[aether_sfm] quadratic prepay aborted";
  }
  return attempted;
}

                                                                        
                                                                    
                                                                        
                                                                            
                                                                           
                                                                          
                                                                            
                                                                              
                                                                              
                                                                         
                                                                               
                                                               
                                                                
                                                          
                                                                                
                                                                              
                                                                            
                                                                             
                                                                           
                                                                   
                                                                     
                                                        
                                                                          
                                                                             
                   
static bool IdlePrepayEnabled() {
  static const bool cached = [] {
    const char* e = std::getenv("OFFICIAL_AETHER_IDLE_PREPAY");
    return !(e && e[0] == '0');
  }();
  return cached;
}
static double IdlePrepayIdleMs() {
  static const double cached = [] {
    if (const char* e = std::getenv("OFFICIAL_AETHER_IDLE_PREPAY_IDLE_MS")) {
      const double v = std::atof(e);
      if (v >= 0.0) return v;
    }
    return 2000.0;
  }();
  return cached;
}

                                                                       
                                                                            
                                                                       
                                                  
static int LiveRepayStarvedWindowTick(aether_sfm_session_t* s, int max_pairs) {
  if (s->frames.size() < 3 || s->camera_id == 0) return 0;
  const int thermal_entry =
      s->thermal_state.load(std::memory_order_relaxed);
  bool thermal2_mode = false;
  if (thermal_entry >= 3) {                                 
    ++s->stat_repay_skipped_thermal;
    return 0;
  }
  if (thermal_entry == 2) {
    const int clean_n = RepayThermal2CleanN();
    if (clean_n <= 0 || s->gpu_pairs_since_rc7 < clean_n) {
      ++s->stat_repay_skipped_thermal;
      return 0;
    }
    thermal2_mode = true;
    max_pairs = std::min(max_pairs, kRepayThermal2MaxPairs);
  }
  const bool gpu_avail =
      s->options.use_gpu_match && (aether_gpu_match_gemm_pairs != nullptr);
                                                                            
                                        
  if (s->options.use_gpu_match && !gpu_avail) return 0;
  ++s->stat_repay_calls;
  int written = 0;
  try {
    const int num_frames = static_cast<int>(s->frames.size());
    const int K = s->options.k_neighbors > 0 ? s->options.k_neighbors : 12;
    const auto starved = [&](int i) {
      const int valid = i < static_cast<int>(s->live_win_valid.size())
                            ? s->live_win_valid[i]
                            : 0;
      return s->frames[i].n_keypoints > 0 &&
             (valid < kRematchMinValidWindowPairs ||
              s->frames[i].fed_throttled);
    };
    const double ratio =
        s->options.match_max_ratio > 0 ? s->options.match_max_ratio : 0.8;
    const colmap::TwoViewGeometryOptions tvg_options;                    
    std::vector<uint32_t> pair_buf;
    int attempted = 0;
    for (int gap = 1; gap <= K && attempted < max_pairs; ++gap) {
      for (int f = gap; f < num_frames && attempted < max_pairs; ++f) {
        const int j = f - gap;
        if (gap > kRematchNearGap && !starved(f) && !starved(j)) continue;
        const FrameRecord& fj = s->frames[j];
        const FrameRecord& ff = s->frames[f];
        if (fj.n_keypoints <= 0 || ff.n_keypoints <= 0) continue;
        if (fj.descriptors.empty() || ff.descriptors.empty()) continue;
        const uint64_t key = FramePairKey(f, j);
        if (s->live_pairs_done.count(key)) continue;
        if (s->db->ExistsMatches(fj.image_id, ff.image_id)) {
          s->live_pairs_done.insert(key);
          continue;
        }
                                                                         
                                                                              
                                                                           
                                                                              
        {
          const int th_now =
              s->thermal_state.load(std::memory_order_relaxed);
          if (th_now >= 3 || (th_now >= 2 && !thermal2_mode)) {
            ++s->stat_repay_skipped_thermal;
            return written;
          }
        }
        ++attempted;
        ++s->stat_repay_attempted;
        const int cap =
            fj.n_keypoints < ff.n_keypoints ? fj.n_keypoints : ff.n_keypoints;
        pair_buf.resize(static_cast<size_t>(cap) * 2);
        int num_matches = 0;
        const int mrc =
            gpu_avail
                ? GpuMatchGemmPairsRetry(
                      s, fj.frame_id, fj.descriptors.data(), fj.n_keypoints,
                      ff.frame_id, ff.descriptors.data(), ff.n_keypoints,
                      ratio, pair_buf.data(), cap, &num_matches)
                : aether_sift_match_pairs(fj.descriptors.data(),
                                          fj.n_keypoints,
                                          ff.descriptors.data(),
                                          ff.n_keypoints, ratio,
                                          pair_buf.data(), cap, &num_matches);
        if (mrc != 0) {
          ++s->stat_repay_failed;
          if (gpu_avail && mrc == 7) s->gpu_pairs_since_rc7 = 0;
                                                                           
                                                                               
          if (thermal2_mode) return written;
          continue;
        }
        if (gpu_avail) ++s->gpu_pairs_since_rc7;
        if (num_matches <= 0) continue;                                       
        colmap::FeatureMatches matches(num_matches);
        for (int m = 0; m < num_matches; ++m) {
          matches[m].point2D_idx1 = pair_buf[2 * m];
          matches[m].point2D_idx2 = pair_buf[2 * m + 1];
        }
        const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
            fj, fj.points, ff, ff.points, matches, tvg_options);
        if (!MandatoryGravityTvgPersistable(tvg)) continue;
        const bool tail_pair_first_write =
            TailCacheBeginPairWrite(s, fj.image_id, ff.image_id,
                                                   true);
        s->db->WriteMatches(fj.image_id, ff.image_id, matches);
        s->db->WriteTwoViewGeometry(fj.image_id, ff.image_id, tvg.geometry);
        TailCacheCommitPair(s, fj.image_id, ff.image_id, tvg.geometry,
                            tail_pair_first_write);
        s->live_pairs_done.insert(key);
        ++written;
        ++s->stat_repay_written;
        s->stat_repay_inliers +=
            static_cast<int64_t>(tvg.geometry.inlier_matches.size());
        if (static_cast<int>(tvg.geometry.inlier_matches.size()) >=
            kRematchValidInlierGate) {
          if (static_cast<int>(s->live_win_valid.size()) <= f) {
            s->live_win_valid.resize(f + 1, 0);
          }
          ++s->live_win_valid[j];
          ++s->live_win_valid[f];
        }
      }
    }
                                                                             
                                                                             
                                                                            
                                                                          
                                                                              
                                                                         
                                                                          
                                                                           
                                                                            
                                                                              
    if (!s->probe_skipped_pairs.empty() && attempted < max_pairs) {
      std::vector<uint64_t> debt(s->probe_skipped_pairs.begin(),
                                 s->probe_skipped_pairs.end());
      std::sort(debt.begin(), debt.end());
      for (const uint64_t key : debt) {
        if (attempted >= max_pairs) break;
        const int f = static_cast<int>(key >> 32);
        const int j = static_cast<int>(key & 0xFFFFFFFFULL);
        if (f < 0 || f >= num_frames || j < 0 || j >= num_frames) {
          s->probe_skipped_pairs.erase(key);                                 
          continue;
        }
        const FrameRecord& fj = s->frames[j];
        const FrameRecord& ff = s->frames[f];
        if (fj.image_id == 0 || ff.image_id == 0) {
          s->probe_skipped_pairs.erase(key);                                  
          continue;
        }
        if (fj.n_keypoints <= 0 || ff.n_keypoints <= 0 ||
            fj.descriptors.empty() || ff.descriptors.empty()) {
          continue;                                                          
        }
        if (s->live_pairs_done.count(key) ||
            s->db->ExistsMatches(fj.image_id, ff.image_id)) {
          s->probe_skipped_pairs.erase(key);                               
          s->live_pairs_done.insert(key);
          continue;
        }
                                                                           
        {
          const int th_now =
              s->thermal_state.load(std::memory_order_relaxed);
          if (th_now >= 3 || (th_now >= 2 && !thermal2_mode)) {
            ++s->stat_repay_skipped_thermal;
            return written;
          }
        }
        ++attempted;
        ++s->stat_repay_attempted;
        const int cap =
            fj.n_keypoints < ff.n_keypoints ? fj.n_keypoints : ff.n_keypoints;
        pair_buf.resize(static_cast<size_t>(cap) * 2);
        int num_matches = 0;
        const int mrc =
            gpu_avail
                ? GpuMatchGemmPairsRetry(
                      s, fj.frame_id, fj.descriptors.data(), fj.n_keypoints,
                      ff.frame_id, ff.descriptors.data(), ff.n_keypoints,
                      ratio, pair_buf.data(), cap, &num_matches)
                : aether_sift_match_pairs(fj.descriptors.data(),
                                          fj.n_keypoints,
                                          ff.descriptors.data(),
                                          ff.n_keypoints, ratio,
                                          pair_buf.data(), cap, &num_matches);
        if (mrc != 0) {
          ++s->stat_repay_failed;
          if (gpu_avail && mrc == 7) s->gpu_pairs_since_rc7 = 0;
          if (thermal2_mode) return written;
          continue;                                                       
        }
        if (gpu_avail) ++s->gpu_pairs_since_rc7;
        s->probe_skipped_pairs.erase(key);
        ++s->stat_probe_debt_repaid_live;
        if (num_matches <= 0) continue;                                      
        colmap::FeatureMatches matches(num_matches);
        for (int m = 0; m < num_matches; ++m) {
          matches[m].point2D_idx1 = pair_buf[2 * m];
          matches[m].point2D_idx2 = pair_buf[2 * m + 1];
        }
        const auto tvg = EstimateMandatoryFrameTwoViewGeometry(
            fj, fj.points, ff, ff.points, matches, tvg_options);
        if (!MandatoryGravityTvgPersistable(tvg)) continue;
        const bool tail_pair_first_write =
            TailCacheBeginPairWrite(s, fj.image_id, ff.image_id,
                                                   true);
        s->db->WriteMatches(fj.image_id, ff.image_id, matches);
        s->db->WriteTwoViewGeometry(fj.image_id, ff.image_id, tvg.geometry);
        TailCacheCommitPair(s, fj.image_id, ff.image_id, tvg.geometry,
                            tail_pair_first_write);
        s->live_pairs_done.insert(key);
        ++written;
        ++s->stat_repay_written;
        s->stat_repay_inliers +=
            static_cast<int64_t>(tvg.geometry.inlier_matches.size());
                                                                          
          
                                                                                       
                                                                                 
                                                                                
                                                                                
                                                                             
                                                                           
                                                                               
                                                                                  
        // feeling coverage grow shot by shot is the point of the AR preview.
          
                                                                              
                                                                                
                                                               
                                                                  
                                                                            
                                                  
          
                                                                                
                                                                               
                                                                                
        bool grown_live_now = false;
        if (ProbeDebtGrowLiveEnabled() && s->live_recon_ready && s->live_recon &&
            !tvg.geometry.inlier_matches.empty()) {
          colmap::Reconstruction* recon = s->live_recon.get();
          if (fj.has_pose && ff.has_pose && !fj.points.empty() &&
              !ff.points.empty() && recon->ExistsImage(fj.image_id) &&
              recon->ExistsImage(ff.image_id)) {
            const colmap::Image& im1 = recon->Image(fj.image_id);
            const colmap::Image& im2 = recon->Image(ff.image_id);
            if (im1.HasPose() && im2.HasPose() &&
                im1.NumPoints2D() == fj.points.size() &&
                im2.NumPoints2D() == ff.points.size()) {
              const LiveGrowView v1{fj.image_id, &fj.points, &fj.camera,
                                    im1.CamFromWorld()};
              const LiveGrowView v2{ff.image_id, &ff.points, &ff.camera,
                                    im2.CamFromWorld()};
              std::unordered_set<PointIdPair, PointIdPairHash> merge_trials;
              std::unordered_set<colmap::point3D_t> touched;
              GrowLiveTracksFromTvgInliers(
                  s, recon, v1, v2, tvg.geometry.inlier_matches,
                  LiveCreateTriMinAngleRad(), LiveCreateMaxReprojPx(),
                  LiveGrowMaxReprojPx(),                         8.0,
                                                    false, ProbeDebtGrowMerge(),
                  ProbeDebtGrowObs(), &merge_trials, &touched);
              ++s->stat_probe_debt_grow_live_pairs;
              grown_live_now = true;
            }
          }
        }
        if (!grown_live_now && !tvg.geometry.inlier_matches.empty()) {
          s->probe_debt_grow.push_back(
              ProbeDebtGrowPair{j, f, tvg.geometry.inlier_matches});
        }
        if (static_cast<int>(tvg.geometry.inlier_matches.size()) >=
            kRematchValidInlierGate) {
          if (static_cast<int>(s->live_win_valid.size()) <= f) {
            s->live_win_valid.resize(f + 1, 0);
          }
          ++s->live_win_valid[j];
          ++s->live_win_valid[f];
        }
      }
    }
    return written;
  } catch (const std::exception& e) {
    LOG(WARNING) << "[aether_sfm] live repay aborted: " << e.what();
    return written;
  }
}

int aether_sfm_live_repay(aether_sfm_session_t* s, int max_pairs) {
  if (!s || !s->db) return -1;
  if (max_pairs <= 0) return 0;
                                                                       
                                                                            
                                                                             
                                                                           
  if (kProductionOfficialEndpointOnly) {
    const int consumed = PrepayQuadraticTick(s, max_pairs);
    if (consumed > 0) return consumed;
                                                                              
                                                                      
                                                                           
    if (!IdlePrepayEnabled()) return 0;
                                                                                                      
      
                                                                                                   
                                                                                                             
                                                                                                
                                                                                       
      
                                                                                        
                                                                                            
                                                                                      
                                                                                  
                                                                                         
    const bool starvedAlways = [] {
      const char* e = std::getenv("OFFICIAL_AETHER_STARVED_ALWAYS");
      return !(e && e[0] == '0');              
    }();
    const int thermalStop = [] {
      const char* e = std::getenv("OFFICIAL_AETHER_STARVED_THERMAL_STOP");
      const int v = e && e[0] ? std::atoi(e) : 3;
      return v <= 0 ? 3 : v;
    }();
    if (!starvedAlways) {
      if (s->last_add_frame_done_ms <= 0.0 ||
          NowMs() - s->last_add_frame_done_ms < IdlePrepayIdleMs()) {
        return 0;                                                
      }
      if (s->thermal_state.load(std::memory_order_relaxed) >= 2) {
        ++s->stat_repay_skipped_thermal;
        return 0;
      }
    } else {
      if (s->last_add_frame_done_ms <= 0.0) return 0;                               
      if (s->thermal_state.load(std::memory_order_relaxed) >= thermalStop) {
        ++s->stat_repay_skipped_thermal;
        return 0;
      }
    }
    ++s->stat_idle_prepay_ticks;
    const int64_t attempted_before = s->stat_repay_attempted;
    const int64_t written_before = s->stat_repay_written;
    const int n =
        LiveRepayStarvedWindowTick(s, std::min(max_pairs,
                                               kRepayThermal2MaxPairs));
    if (s->stat_repay_attempted != attempted_before) {
      char line[224];
      std::snprintf(
          line, sizeof(line),
          "{\"t\":%lld,\"type\":\"idle_prepay\",\"tick\":%lld,"
          "\"attempted\":%lld,\"written\":%lld,\"cum_written\":%lld}",
          static_cast<long long>(EpochMs()),
          static_cast<long long>(s->stat_idle_prepay_ticks),
          static_cast<long long>(s->stat_repay_attempted - attempted_before),
          static_cast<long long>(s->stat_repay_written - written_before),
          static_cast<long long>(s->stat_repay_written));
      AppendMatchFailJsonl(s, line);
    }
    return n;
  }
  return LiveRepayStarvedWindowTick(s, max_pairs);
}

                                                                             
                                                                
                                                    
void aether_sfm_repair_stats(aether_sfm_session_t* s, int64_t* repay_calls,
                             int64_t* repay_attempted, int64_t* repay_written,
                             int64_t* repay_inliers, int64_t* repay_failed,
                             int64_t* repay_skipped_thermal,
                             int64_t* gpu_retry_attempts,
                             int64_t* gpu_retry_recovered,
                             int64_t* enrich_budget_stopped) {
  if (!s) return;
  if (repay_calls) *repay_calls = s->stat_repay_calls;
  if (repay_attempted) *repay_attempted = s->stat_repay_attempted;
  if (repay_written) *repay_written = s->stat_repay_written;
  if (repay_inliers) *repay_inliers = s->stat_repay_inliers;
  if (repay_failed) *repay_failed = s->stat_repay_failed;
  if (repay_skipped_thermal)
    *repay_skipped_thermal = s->stat_repay_skipped_thermal;
  if (gpu_retry_attempts) *gpu_retry_attempts = s->stat_gpu_retry_attempts;
  if (gpu_retry_recovered) *gpu_retry_recovered = s->stat_gpu_retry_recovered;
  if (enrich_budget_stopped)
    *enrich_budget_stopped = s->stat_enrich_budget_stopped;
}

                                                                            
                                                                           
                                                                            
                                                                                      
                                                                       
                                                                        
                                                                      
                                                                     
                                                                          
                                                                               
                                                                      
                                                                        
                                                                           
aether_sfm_result_t aether_sfm_arbitrate(aether_sfm_session_t* s,
                                         char* out_json, int out_cap) {
  if (out_json && out_cap > 0) out_json[0] = '\0';
  if (!s) return AETHER_SFM_ERR_INVALID_ARG;
  try {
    const double t0 = NowMs();
    const std::string dir =
        std::filesystem::path(s->db_path).parent_path().string();
    aether_l1::ArbStats st;
    std::string err;
    if (!aether_l1::RunArbitration(dir, &st, &err)) {
      LOG(WARNING) << "[aether_sfm] l1_arbitrate: " << err;
      const bool absent = err.find("missing") != std::string::npos;
      return absent ? AETHER_SFM_ERR_NOT_REGISTERED : AETHER_SFM_ERR_INTERNAL;
    }
    st.pass_ms = NowMs() - t0;
    LOG(WARNING) << "[aether_sfm] l1_arbitrate: refs=" << st.n_refs_used << "/"
                 << st.n_refs_planned << " band=" << st.n_band
                 << " rescue=" << st.n_rescue << " confirm=" << st.n_confirm
                 << " abstain=" << st.n_abstain
                 << " degraded=" << (st.degraded ? 1 : 0) << " ("
                 << static_cast<int>(st.pass_ms) << "ms)";
    if (out_json && out_cap > 0) {
      std::snprintf(
          out_json, static_cast<size_t>(out_cap),
          "{\"refs_planned\":%d,\"refs_used\":%d,\"n_band\":%d,"
          "\"rescue\":%d,\"confirm\":%d,\"abstain\":%d,"
          "\"delta_mm\":%.3f,\"delta_ray_mm\":%.3f,\"degraded\":%s,"
          "\"pass_ms\":%d}",
          st.n_refs_planned, st.n_refs_used, st.n_band, st.n_rescue,
          st.n_confirm, st.n_abstain, st.delta_mm, st.delta_ray_mm,
          st.degraded ? "true" : "false", static_cast<int>(st.pass_ms));
    }
    return AETHER_SFM_OK;
  } catch (const std::exception& e) {
    LOG(WARNING) << "[aether_sfm] l1_arbitrate failed: " << e.what();
    return AETHER_SFM_ERR_INTERNAL;
  } catch (...) {
    return AETHER_SFM_ERR_INTERNAL;
  }
}

                                                                             
                                                                               
                                                                             
                                                          
void aether_sfm_final_diag(aether_sfm_session_t* s, double* mean_reproj_px,
                           int64_t* n_points, int64_t* n_track3plus,
                           int64_t* n_obs) {
  if (mean_reproj_px) *mean_reproj_px = 0.0;
  if (n_points) *n_points = 0;
  if (n_track3plus) *n_track3plus = 0;
  if (n_obs) *n_obs = 0;
  if (!s) return;
  std::shared_ptr<const colmap::Reconstruction> recon;
  {
    std::lock_guard<std::mutex> lk(s->recon_mutex);
    recon = s->recon;                                                             
  }
  if (!recon) return;
  int64_t pts = 0, track3 = 0, obs = 0;
  double err_sum = 0.0;
  int64_t err_n = 0;
  try {
    for (const auto& [pid, pt] : recon->Points3D()) {
      ++pts;
      const size_t len = pt.track.Length();
      obs += static_cast<int64_t>(len);
      if (len >= 3) ++track3;
      for (const auto& el : pt.track.Elements()) {
        if (!recon->ExistsImage(el.image_id)) continue;
        const colmap::Image& image = recon->Image(el.image_id);
        if (!image.HasPose() || el.point2D_idx >= image.NumPoints2D()) continue;
        const Eigen::Vector3d x_cam = image.CamFromWorld() * pt.xyz;
        if (x_cam.z() <= 0.0) continue;
        const colmap::Camera* cam = image.CameraPtr();
        if (!cam) continue;
        const std::optional<Eigen::Vector2d> px = cam->ImgFromCam(x_cam);
        if (!px) continue;
        err_sum += (*px - image.Point2D(el.point2D_idx).xy).norm();
        ++err_n;
      }
    }
  } catch (const std::exception&) {
                                                      
  }
  if (n_points) *n_points = pts;
  if (n_track3plus) *n_track3plus = track3;
  if (n_obs) *n_obs = obs;
  if (mean_reproj_px) *mean_reproj_px = err_n > 0 ? err_sum / err_n : 0.0;
}

aether_sfm_result_t aether_sfm_debug_dump_model(aether_sfm_session_t* s,
                                                const char* dir) {
                                                                         
                                                          
                                                                             
                                                                       
  if (!s || !dir || !dir[0]) return AETHER_SFM_ERR_INVALID_ARG;
  std::shared_ptr<const colmap::Reconstruction> recon;
  {
    std::lock_guard<std::mutex> lk(s->recon_mutex);
    recon = s->recon;
  }
  if (!recon) return AETHER_SFM_ERR_INTERNAL;
  try {
    std::filesystem::create_directories(dir);
    recon->Write(dir);
    return AETHER_SFM_OK;
  } catch (const std::exception& e) {
    LOG(WARNING) << "[aether_sfm] debug_dump_model failed: " << e.what();
    return AETHER_SFM_ERR_INTERNAL;
  }
}

void aether_sfm_free(aether_sfm_session_t* s) {
  if (!s) return;
                                                                             
  if (s->refine_thread.joinable()) s->refine_thread.join();
                                                                                     
  if (s->pf_thread.joinable()) {
    {
      std::lock_guard<std::mutex> lk(s->pf_mu);
      s->pf_stop = true;
    }
    s->pf_cv.notify_all();
    s->pf_thread.join();
  }
                                                                          
                                                                           
  if (s->async_pvba_thread.joinable()) s->async_pvba_thread.join();
                                                                                            
                                                    
  AppendBaRingJsonl(s, 0);                                                          
  AppendBaSessionAggregateJsonl(s, "session_free");
  s->gpu_watchdog.Stop();
                                                                               
                                                                            
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
  AppendDescriptorResidencyStats(s);
#endif
  TailCacheMarkDirty(
      s, aether::sfm::TailCacheDirtyReasonV1::kDeviceReset);
  AppendTailCacheStats(s);
#if AETHER_COMPILE_DESCRIPTOR_RESIDENCY_V1
  if (aether_gpu_match_descriptor_residency_clear_session != nullptr &&
      s->descriptor_residency_nonce != 0) {
    aether_gpu_match_descriptor_residency_clear_session(
        s->descriptor_residency_nonce);
  }
#endif
  if (s->db) {
    try {
      s->db->Close();
    } catch (...) {
    }
  }
  if (s->owns_db_file && !s->db_path.empty()) {
    std::remove(s->db_path.c_str());
  }
  aether_ba_clear_gravity_priors();
  delete s;
}

}               
