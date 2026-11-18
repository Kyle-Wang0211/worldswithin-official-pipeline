#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace aether::sfm {

                                                                            
                                                                         
                                                                         
struct PairSelectionFrameV2 {
  int32_t frame_id = -1;
  std::array<double, 3> center_xyz{};
  std::array<double, 3> forward_xyz{};
  bool pose_valid = false;
  bool matchable = false;
};

struct PairSelectionLegacyResultV2 {
  std::vector<int32_t> ordered_frame_ids;
  int32_t spatial_count = 0;
  int32_t temporal_count = 0;
};

enum class PairCandidateSourceV2 : uint32_t {
  kSpatial = 1U << 0U,
  kTemporal = 1U << 1U,
  kVisualLoop = 1U << 2U,
  kQuadraticBackstop = 1U << 3U,
};

                                                                               
                                                          
struct CanonicalPairCandidateV2 {
  int32_t first_frame_id = -1;
  int32_t second_frame_id = -1;
  uint32_t source_mask = 0;
};

struct PairSelectionConfigV2 {
  int32_t spatial_k = 20;
  int32_t temporal_lookback = 2;
  int32_t spatial_recent_exclusion = 2;
};

struct PairSelectionResultV2 {
  std::vector<CanonicalPairCandidateV2> ordered_pairs;
  int32_t spatial_count = 0;
  int32_t temporal_count = 0;
};

                                          
                                                                   
                                                          
                                                                          
                                      
  
                                                                           
                                                                          
PairSelectionLegacyResultV2 SelectLegacySpatialCandidatesV2(
    const std::vector<PairSelectionFrameV2>& history,
    const PairSelectionFrameV2& current, int32_t k, bool temporal_only);

                                                                             
                                                                               
                                         
std::vector<CanonicalPairCandidateV2> MergeCanonicalPairCandidatesV2(
    const std::vector<CanonicalPairCandidateV2>& candidates);

                                                                               
                                                                             
                                                                               
                                                                      
PairSelectionResultV2 SelectSpatialTemporalCandidatesV2(
    const std::vector<PairSelectionFrameV2>& history,
    const PairSelectionFrameV2& current,
    const PairSelectionConfigV2& config);

}                          
