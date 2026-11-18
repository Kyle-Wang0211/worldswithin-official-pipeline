// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Aether3D. All rights reserved.

#ifndef AETHER_CPP_TOOLS_DAWN_KERNEL_HARNESS_H
#define AETHER_CPP_TOOLS_DAWN_KERNEL_HARNESS_H

                                                                                         
  
                                                                           
                                                                          
                                                                          
                                                                     
                                                                        
  
                                                                       
                                                                              
                                                                              
                                                                           
                                                              
  
                          
                                                                    
                                                                              
                                                                  
  
                                                                          
                                                                   
                                                                      
                                                               

#include <webgpu/webgpu_cpp.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <vector>

namespace aether {
namespace tools {

class DawnKernelHarness {
public:
    DawnKernelHarness();
    ~DawnKernelHarness();

                    
    DawnKernelHarness(const DawnKernelHarness&) = delete;
    DawnKernelHarness& operator=(const DawnKernelHarness&) = delete;

                                                                          
                                                                      
                                                                      
                                      
    bool init();

                                                                                            
                    
      
                                                                                         
                                                                                             
                                                                                                
                                                                                             
                                                                                               
      
                                                                                                   
                                                                   
    static bool take_device_error(std::string* msg = nullptr);

                                                                            
                                                                        
                                                                         
                                 
    wgpu::Buffer upload(const void* data, size_t size, wgpu::BufferUsage usage);

                                                                     
                                                                    
                                                                      
                                                                       
                                 
    wgpu::Buffer alloc(size_t size, wgpu::BufferUsage usage);

                                                                       
                                                
    wgpu::Buffer alloc_staging_for_readback(size_t size);

                                                            
                                                                          
                                                                     
                                                                          
    wgpu::ComputePipeline load_compute(std::string_view wgsl_source,
                                        const char* entry_point = "main");

                                                                              
                                                                          
                                                                     
                                                                          
    void dispatch(const wgpu::ComputePipeline& pipeline,
                  const std::vector<wgpu::Buffer>& bindings,
                  uint32_t wg_x, uint32_t wg_y = 1, uint32_t wg_z = 1);

                                                                                      
                                                                                     
                                                                                             
                                                                                            
    struct BufBinding {
        wgpu::Buffer buffer;
        uint64_t offset = 0;
        uint64_t size = WGPU_WHOLE_SIZE;
        explicit BufBinding(wgpu::Buffer b, uint64_t off = 0,
                            uint64_t sz = WGPU_WHOLE_SIZE)
            : buffer(std::move(b)), offset(off), size(sz) {}
    };
    void dispatch(const wgpu::ComputePipeline& pipeline,
                  const std::vector<BufBinding>& bindings,
                  uint32_t wg_x, uint32_t wg_y = 1, uint32_t wg_z = 1);

                                                                                        
                                                                             
                                                                           
                                                                        
                                                                               
                                                                          
                                                                                 
                                                                                  
                                                                        
    void begin_batch();
    void dispatch_batched(const wgpu::ComputePipeline& pipeline,
                          const std::vector<wgpu::Buffer>& bindings,
                          uint32_t wg_x, uint32_t wg_y = 1, uint32_t wg_z = 1);
    void dispatch_batched(const wgpu::ComputePipeline& pipeline,
                          const std::vector<BufBinding>& bindings,
                          uint32_t wg_x, uint32_t wg_y = 1, uint32_t wg_z = 1);
                                                                              
                                                               
    void copy_region_batched(const wgpu::Buffer& src, uint64_t src_offset,
                             const wgpu::Buffer& dst, uint64_t dst_offset,
                             size_t size);
    void end_batch();                            
                                                                                                                    
                                                                                                              
    struct AsyncBatch { wgpu::Future future{}; std::shared_ptr<bool> done; bool valid = false; std::vector<wgpu::BindGroup> keep; };
    AsyncBatch end_batch_async();
    bool wait_async(AsyncBatch& b);

                                                                             
                                                                            
                                                                              
                                                                      
                                                                                 
                                                                              
                                                                              
                                                                          
                                                                           
    void dispatch_indirect(const wgpu::ComputePipeline& pipeline,
                           const std::vector<wgpu::Buffer>& bindings,
                           const wgpu::Buffer& indirect_buffer,
                           uint64_t indirect_offset = 0);

                                                                              
                                                                               
                                                                     
                                                                        
    wgpu::Buffer alloc_indirect_args();

                                                                        
                                                  
    void copy_to_staging(const wgpu::Buffer& src, const wgpu::Buffer& dst,
                         size_t size);

                                                                             
                                                                            
                                                                                
    void copy_region(const wgpu::Buffer& src, uint64_t src_offset,
                     const wgpu::Buffer& dst, uint64_t dst_offset, size_t size);

                                                                          
                                                                      
                                                                        
                                 
    std::vector<uint8_t> readback(const wgpu::Buffer& buf, size_t size);

                                                                                                  

                                                                
                                                                         
                                                                  
                                                            
      
                                                                        
                                                                          
                                                                          
                                                                          
    wgpu::Texture alloc_render_target(uint32_t w, uint32_t h,
                                       wgpu::TextureFormat format);

                                                                      
                                                                         
                                                               
                                                                     
                                                         
    wgpu::RenderPipeline load_render_pipeline(
        std::string_view wgsl_source,
        const char* vs_entry,
        const char* fs_entry,
        wgpu::TextureFormat color_format,
        wgpu::PrimitiveTopology topology = wgpu::PrimitiveTopology::TriangleList);

                                                                     
                                                                       
                                                                    
                                                                      
                                               
    void dispatch_render_pass(
        const wgpu::RenderPipeline& pipeline,
        const wgpu::Texture& target,
        const std::vector<wgpu::Buffer>& bindings,
        uint32_t vertex_count,
        uint32_t instance_count);

                                                                       
                                                                       
                                                                     
                                                                
      
                                                                      
                                                                  
                                                                  
                  
    std::vector<uint8_t> readback_texture(
        const wgpu::Texture& tex,
        uint32_t w, uint32_t h,
        uint32_t bytes_per_pixel);

                                                                          
                                                                                          
                                                                                                      
                                                           
    bool healthy() const { return device_healthy_; }

                                                                                          
                                                                                        
                                                                                                 
                                                                               
    void mark_unhealthy() { device_healthy_ = false; }

                                                                          
                                                                            
                                                                                
                                                                 
    bool has_f16() const { return has_f16_; }
                                                                                                      
                                                                                                 
                                                                                                
                                                                 
    void set_strict_math(bool v) { strict_math_ = v; }
    bool strict_math() const { return strict_math_ && has_strict_math_; }
    bool has_strict_math() const { return has_strict_math_; }

                                      
    const wgpu::Instance& instance() const { return instance_; }
    const wgpu::Adapter&  adapter()  const { return adapter_; }
    const wgpu::Device&   device()   const { return device_; }
    const wgpu::Queue&    queue()    const { return queue_; }

                                                                                          
      
                                                                           
                                                                            
                                                                               
                                                                               
                                                                         
                                                                            
                                                                             
                   
      
                                                                          
                                                                            
                                                                              
                                                                            
                         
      
                                                                            
                                                                        
                                           
    bool gpu_ts_enabled() const { return ts_enabled_; }
    uint32_t gpu_ts_status() const { return ts_status_; }
    uint32_t gpu_ts_capability() const { return ts_capability_; }
                                                                            
                                                                               
                                                          
    bool gpu_ts_feature_request_to_device() const {
        return ts_feature_request_to_device_;
    }
    bool gpu_ts_feature_granted() const { return ts_feature_granted_; }
                                                                     
    void gpu_ts_reset();
                                                                                
                                                                        
    uint32_t gpu_ts_pass_count() const { return ts_next_ / 2; }
                                                                               
                                                                               
                                                  
    bool gpu_ts_resolve(std::vector<uint64_t>* out_ns);
                                                                             
                                                                              
                                                                           
    bool gpu_ts_finalize_frame(const uint32_t* cumulative_pass_counts,
                               uint32_t stage_count);
                                                                               
                                                                            
                                                                            
                                     
    static void gpu_ts_clear_caller_thread_frame();
    bool gpu_ts_stash_frame_for_caller_thread();

                                                                                    
      
                                                                              
                                                                            
                                                                           
                                            
                                                                                
                                                      
                                                                           
                              
      
                                                                           
                                                               
    struct HostBreakdown {
        double upload_ms = 0;                                    
        double encode_ms = 0;                                             
        double submit_wait_ms = 0;                                         
        double map_ms = 0;                                             
                                                                                    
                                                                               
                                                                                    
                                                                                
                                                                               
                                                                            
                                                                               
                                                   
        double create_ms = 0;
        uint32_t n_upload = 0;
        uint32_t n_dispatch = 0;
                                                                                    
                                                                                        
                                                                                         
                                                                              
        uint32_t n_submit = 0;
        uint32_t n_copy = 0;
        uint32_t n_readback = 0;
        uint32_t n_create = 0;                          
    };
                                                                                             
                                                                                       
                                                                                               
    size_t pipeline_cache_size() const { return pipeline_cache_.size(); }

                                                                                                
                                                                                
    void dawn_histogram(const char* name, double* out_ms, uint32_t* out_n) const;

                                                                                             
                                                                                                           
                                                                                                      
    int dump_msl(const char* dir = nullptr) const;

    const HostBreakdown& host_breakdown() const { return hb_; }
    void host_breakdown_reset() { hb_ = HostBreakdown{}; }

private:
                                                                              
                                                                           
                                               
    const wgpu::ComputePassDescriptor* ts_pass_desc(
        wgpu::ComputePassDescriptor* storage,
        wgpu::PassTimestampWrites* writes);

    wgpu::Instance instance_;
    wgpu::Adapter adapter_;
    wgpu::Device device_;
    wgpu::Queue queue_;

                                                                           
                                                                           
    wgpu::CommandEncoder batch_encoder_;
    std::vector<wgpu::BindGroup> batch_bind_groups_;

                                                                            
                                                                         
                                                                      
                                                                             
                                                                             
                                                                               
    std::unordered_map<std::string, wgpu::ComputePipeline> pipeline_cache_;

    void* hist_sink_ = nullptr;                                                
    bool has_f16_ = false;                                             
    bool has_strict_math_ = false;                                                              
    bool strict_math_ = false;
    bool init_called_ = false;
                                                                         
    bool device_healthy_ = true;

                                                                               
                                                                             
                                                                            
                                                                         
    static constexpr uint32_t kTsCapacity = 1024;
    bool ts_requested_ = false;
    bool ts_feature_request_to_device_ = false;
    bool ts_feature_granted_ = false;
    bool ts_enabled_ = false;
                                                                                                 
                                                                                       
                                                                                           
    bool hb_enabled_ = false;
    uint32_t ts_drop_count_ = 0;
    uint32_t ts_capability_ = 0;
    uint32_t ts_status_ = 0;
    uint32_t ts_reason_code_ = 0;
    uint32_t ts_next_ = 0;
    uint32_t ts_resolve_attempt_count_ = 0;
    uint64_t ts_probe_instance_id_ = 0;
    uint64_t ts_extraction_ordinal_ = 0;
    bool ts_pending_ready_ = false;
    uint64_t ts_pending_probe_instance_id_ = 0;
    uint64_t ts_pending_extraction_ordinal_ = 0;
    uint32_t ts_pending_resolve_attempt_count_ = 0;
    std::vector<uint64_t> ts_pending_ticks_;
    wgpu::QuerySet ts_qset_;
    wgpu::Buffer ts_resolve_;                             
    wgpu::Buffer ts_readback_;                       
    HostBreakdown hb_;                                                     
};

}                    
}                     

#endif                                           
