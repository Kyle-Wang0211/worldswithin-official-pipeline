// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#ifndef AETHER_CPP_TOOLS_SIFT_PYRAMID_DAWN_H
#define AETHER_CPP_TOOLS_SIFT_PYRAMID_DAWN_H

                                                                 
  
                                                                              
                                    
                                                                                   
                                                                                    
                                                                          
  
                                                                                
                                                                            
                              
                                
                                                                           
                               
                       
                                                                                  
  
                                                                                   
                                                                               
                                                                               

#include <cstdint>
#include <vector>

#include "dawn_kernel_harness.h"

namespace aether {
namespace tools {

class SiftPyramidDawn {
public:
    static constexpr int kOctaveResolution = 3;
    static constexpr int kOctaveFirstSub = -1;                     
    static constexpr int kOctaveLastSub = kOctaveResolution + 1;              
    static constexpr int kLevelsPerOctave = kOctaveLastSub - kOctaveFirstSub + 1;      
    static constexpr int kMinOctaveSize = 16;

    struct LevelGeom {
        int octave;         
        int sublevel;                                             
        int width;                              
        int height;
        double sigma;                                         
    };

                                                                           
                                                                         
                                        
    bool build(DawnKernelHarness& harness,
               const uint8_t* gray, int width, int height);

    int first_octave() const { return 0; }
    int last_octave() const { return last_octave_; }
    int width() const { return width_; }
    int height() const { return height_; }

                                                                     
    LevelGeom level_geom(int octave, int sublevel) const;

                                                                           
                                                             
    std::vector<float> read_level(DawnKernelHarness& harness,
                                  int octave, int sublevel) const;

                                                                                        
                                                                                        
                                                                                           
                                                                                          
    const wgpu::Buffer& packed_buffer() const { return packed_buf_; }
                                                                                           
                                                                                         
                                                                                                        
                                                                                             
                                                                                                   
                                                                                               
                                                                     
    const wgpu::Buffer& keypoint_buffer() const { return kp_buf_; }
    bool keypoint_buffer_ready() const { return kp_buf_ != nullptr; }
                                                                              
    void sync_keypoint_buffer(DawnKernelHarness& harness);
                                                                                          
                                                                                              
    static void warmup(DawnKernelHarness& harness);

    static bool kpbuf_enabled();

                                                                                                                                
                                                                              
                                                                                  
                                                                                          
                                                   
                                                           
                                                                                                     
                                                
                                                                                                      
    static bool splitbuf_enabled();
    static constexpr bool level_in_hi(int sublevel) {
        return (sublevel - kOctaveFirstSub) >= kKeypointLevels;
    }
    const wgpu::Buffer& packed_hi_buffer() const { return packed_b_; }
    uint32_t packed_hi_elems() const { return packed_b_elems_; }
    uint32_t level_offset(int octave, int sublevel) const;                   

                                                                                               
                                                                               
                                                                                    
                                                                                          
                                                                                                 
                                                                                                     
                                                                       
                                                                                                      
                                                                                 
    static constexpr uint32_t kAlignElems = 64u;
    static constexpr uint32_t kWindowElems = 1u << 26;                              
                                                                                          
                                                                                      
                                                          
    static uint32_t window_elems();
    static constexpr int kKeypointLevels = 3;
                                                                                          
                                                                                                     
                                                                                                  
    static bool w256_enabled();
                                                                                   
                                                                     
                                                          
                                                                                     
                                                                                           
    static bool w256_bind_enabled();
                                                                                     
                                                                                          
                                                                                                    
    static void note_kp_bind(int subrange);
    static int last_kp_bind();
                                                                                                 
                                                                                                            
                                                                                                        
                                                                                                
                                                                                    
                                                                                               
    static void configure_w256_by_limit(uint64_t required_bytes,
                                        uint64_t limit_bytes);
                                                                                                 
                                                                                           
                                                                            
                                                 
    static void w256_last_decision(uint64_t* required_bytes,
                                   uint64_t* limit_bytes, int* enabled,
                                   int* forced_by_env);
    static std::vector<uint32_t> level_layout(int width, int height,
                                              int last_octave,
                                              uint32_t* total_elems,
                                              uint32_t* keypoint_region_end,
                                              uint32_t* hi_elems = nullptr);
    uint32_t packed_total_elems() const { return packed_total_elems_; }
                                                                                       
    uint32_t keypoint_region_end() const { return keypoint_region_end_; }

                                                        
    int octave_width(int octave) const;
    int octave_height(int octave) const;

                                                                                  
    struct LevelMeta {
        uint32_t offset;                                              
        uint32_t width;
        uint32_t height;
        uint32_t _pad;                                                       
    };

                                                                               
                                                                            
                                                                                 
                                                                                
                                                                               
                                                                              
    wgpu::Buffer pack_levels(DawnKernelHarness& harness,
                             std::vector<LevelMeta>* meta) const;

                                                                           
    int level_index(int octave, int sublevel) const {
        return octave * kLevelsPerOctave + (sublevel - kOctaveFirstSub);
    }

                                                     
    static double base_scale();
    static constexpr double kNominalScale = 0.5;

                                                            
    static double level_sigma(int octave, int sublevel);

private:
                                                                             
    struct OctaveBuffers {
        int width = 0;
        int height = 0;
    };

    int width_ = 0;
    int height_ = 0;
    int last_octave_ = 0;
    std::vector<OctaveBuffers> octaves_;                                      
    wgpu::Buffer packed_buf_;                                              
    wgpu::Buffer kp_buf_;                                                     
    wgpu::Buffer packed_b_;                                           
    uint32_t packed_b_elems_ = 0;                                     
    std::vector<uint32_t> level_offsets_;                                        
    uint32_t packed_total_elems_ = 0;                              
    uint32_t keypoint_region_end_ = 0;                         
};

}                    
}                     

#endif                                         
