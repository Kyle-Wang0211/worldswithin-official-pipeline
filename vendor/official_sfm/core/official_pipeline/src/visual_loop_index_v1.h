#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace aether::sfm {

struct VisualLoopIndexConfigV1 {
                                                                                         
                                                                                          
                                                                               
                                                                                           
                                                    
                                                                            
  int32_t query_period = 5;
  int32_t retrieve_cap = 50;
                                                                                      
                                                                                    
                                                                                            
                                                                                    
                                                                                            
                       
  int32_t topup = 8;
  int32_t recent_exclusion = 20;
  int32_t max_descriptors_per_image = 256;
};

struct VisualLoopCandidateV1 {
  int32_t frame_id = -1;
  uint64_t score = 0;
};

                                                                             
                                                                              
                                                                                 
                                                                             
                                                                   
  
                                                                               
                                                                               
        
class PortableVisualLoopIndexV1 {
 public:
  explicit PortableVisualLoopIndexV1(
      VisualLoopIndexConfigV1 config = {});

                                                                              
                                                                            
                                                                          
  std::vector<VisualLoopCandidateV1> QueryAndAdd(
      int32_t frame_id, const uint8_t* descriptors, int32_t descriptor_count,
      const std::vector<int32_t>& covered_frame_ids);

                                                                           
                                                                            
  bool RemoveFrame(int32_t frame_id);

  size_t NumImages() const { return images_.size(); }

 private:
  static constexpr int32_t kTables = 4;
  static constexpr int32_t kBitsPerTable = 10;
  static constexpr int32_t kWordsPerTable = 1 << kBitsPerTable;
  static constexpr int32_t kWordCount = kTables * kWordsPerTable;

  struct WordCount {
    uint16_t word = 0;
    uint16_t count = 0;
  };

  struct IndexedImage {
    int32_t frame_id = -1;
    std::vector<WordCount> histogram;
  };

  std::vector<WordCount> BuildHistogram(const uint8_t* descriptors,
                                        int32_t descriptor_count) const;
  uint16_t DescriptorWord(const uint8_t* descriptor, int32_t table) const;
  uint64_t Score(const std::vector<WordCount>& query,
                 const std::vector<WordCount>& indexed) const;

  VisualLoopIndexConfigV1 config_;
  int32_t valid_frames_seen_ = 0;
  std::array<uint32_t, kWordCount> document_frequency_{};
  std::vector<IndexedImage> images_;
};

}                          
