// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#ifndef AETHER_CPP_TOOLS_SIFT_EXTRACT_DAWN_H
#define AETHER_CPP_TOOLS_SIFT_EXTRACT_DAWN_H

                                                  
  
                                                             
                                                     
                                                                   
                                                                    
  
                                                                      
                                                                               
                                                                                     
                                                                             
                                                                              
                                                               
  
                                                                               
                                                                               
                                                                                
                                  

#include <cstdint>
#include <vector>

#include "aether/sfm/canonical_feature_selector_v1.h"
#include "dawn_kernel_harness.h"
#include "sift_pyramid_dawn.h"

namespace aether {
namespace tools {

class SiftExtractDawn {
public:
                                                                                 
                                                                                
                                                     
    struct Result {
        std::vector<float> xy;                                                
        std::vector<int> octave;                                      
        std::vector<int> scale;                                              
                                                                     
        // orientations derived from the oriented kp record's affine ellipse
                                                                              
                                                                                 
                                                          
        std::vector<float> kp_scale;                     
        std::vector<float> kp_orientation;                
        std::vector<float> raw_desc;                                                                 
        std::vector<uint64_t> stable_ids;                                              
        aether::sfm::FeatureSelectionPolicyV1 selection_policy =
            aether::sfm::FeatureSelectionPolicyV1::kLegacyColmapGroup;
        aether::sfm::FeatureSelectionPolicyReasonV1 selection_policy_reason =
            aether::sfm::FeatureSelectionPolicyReasonV1::kAbsent;
        int count = 0;
    };

                                                                            
                                                                             
                                                 
    static aether::sfm::FeatureSelectionPolicyDecisionV1
    feature_selection_policy();

                                                                              
                                                                              
                                                                             
                                                                          
                                                                         
                                                                              
                               
      
                                                                               
                                                                                 
                                                                                   
                                                                               
                                                                       
                                                                             
                                                                             
                       
                                                                                              
                                                                                          
                                                                                         
                                                                                     
                                                                                           
                                                                       
    static void warmup(DawnKernelHarness& harness);

    bool extract(DawnKernelHarness& harness,
                 const uint8_t* gray, int width, int height,
                 int max_features,
                 Result* out);

                                                                              
                                                                               
                                                                                 
                                                                              
    // #3803: "lower peak_threshold to get more features, increase edge_threshold").
                                                                            
    static constexpr int kOctaveResolution = SiftPyramidDawn::kOctaveResolution;      
    static double peak_threshold() { return 0.004; }
    static constexpr double kEdgeThreshold = 15.0;
    static constexpr double kSuppressTol = 0.5;
    static constexpr float kDspMinScale = 1.0f / 6.0f;
    static constexpr float kDspMaxScale = 3.0f;
                                                                           
                                                                                  
                                                                                
                                                                             
                                                                               
                                                                           
                                                                                   
                                                                             
                                                                              
                                                                            
                                                          
                                                                                    
                                                                          
                                                                           
                                                                              
                                                                               
                                                                             
                                                                               
                                                                               
    static constexpr int kDspNumScales = 6;

private:
    static constexpr uint32_t kKpStride = 8u;
                                                                                        
                                                                                                    
                                                                                              
                                                                                                 
                                                                                  
                                                      
    static constexpr uint32_t kDetectCap = 131072u;                               
                                                                                       
                                                                                  
                                                                         
    static constexpr uint32_t kOrientCap = 196608u;                       
};

}                    
}                     

#endif                                         
