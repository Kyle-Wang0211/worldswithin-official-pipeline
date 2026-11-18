// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/providers/webgpu/webgpu_external_header.h"

#include "core/common/common.h"
#include "core/providers/webgpu/buffer_manager.h"
#include "core/providers/webgpu/program_manager.h"
#include "core/providers/webgpu/webgpu_utils.h"

#if defined(ENABLE_PIX_FOR_WEBGPU_EP)
#include "core/providers/webgpu/webgpu_pix_frame_generator.h"
#endif                             

namespace onnxruntime {
class Tensor;

namespace webgpu {
class WebGpuContext;
class ComputeContextBase;
class ComputeContext;
class ProgramBase;

                                                                        
struct PendingKernelInfo {
  PendingKernelInfo(std::string_view kernel_name,
                    std::string_view kernel_type,
                    std::string_view program_name,
                    std::string_view cache_key,
                    const std::vector<ProgramInput>& inputs,
                    const std::vector<ProgramOutput>& outputs)
      : name{absl::StrJoin({kernel_name, kernel_type, program_name}, "&")}, cache_key{cache_key} {
                                                                                             
    input_shapes.reserve(inputs.size());
    for (const auto& input : inputs) {
      input_shapes.emplace_back(input.use_override_shape ? input.override_shape : input.tensor->Shape());
    }
    output_shapes.reserve(outputs.size());
    for (const auto& output : outputs) {
      output_shapes.emplace_back(output.use_override_shape ? output.override_shape : output.tensor->Shape());
    }
  }

  PendingKernelInfo(const PendingKernelInfo&) = default;
  PendingKernelInfo& operator=(const PendingKernelInfo&) = default;
  PendingKernelInfo(PendingKernelInfo&&) = default;
  PendingKernelInfo& operator=(PendingKernelInfo&&) = default;

  std::string name;
  std::string cache_key;
  std::vector<TensorShape> input_shapes;
  std::vector<TensorShape> output_shapes;
};

                                                                                
                                                                                                
                                                                                      
struct PendingPipelineBuild {
  std::string name;
  std::vector<int> shape_uniform_ranks;
  wgpu::BindGroupLayout bind_group_layout;
  std::unique_ptr<PipelineCallbackContext> callback_context;
  wgpu::Future future;
};

                                                                                             
  
                                                                    
                                                              
                                                                        
  
                                                               
                                                                   
  
                                                               
                                                                                                  
                                                                                   
  
                                                              
                                                                                              
                                                                                           
  
                                                              
                                                                                            
                                                                                       
  
                                                                                               
                                                                     
struct CapturedCommandInfo {
  std::string program_key;
  std::optional<PendingPipelineBuild> pending_build;
  std::optional<wgpu::ComputePipeline> compute_pipeline;
  wgpu::BindGroup bind_group;
  std::array<uint32_t, 3> dispatch_group{1, 1, 1};
                                                                             
  WGPUBuffer indirect_buffer = nullptr;
                            
  std::optional<PendingKernelInfo> pending_kernel_info;
};

struct WebGpuBufferCacheConfig {
  struct ConfigEntry {
    BufferCacheMode mode;
    std::string config_string;                                                             
  };
  ConfigEntry storage{BufferCacheMode::Bucket, {}};
  ConfigEntry uniform{BufferCacheMode::Simple, {}};
  ConfigEntry query_resolve{BufferCacheMode::Disabled, {}};
  ConfigEntry default_entry{BufferCacheMode::Disabled, {}};
};

             
                                                                      
              
struct WebGpuContextConfig {
  int context_id{0};
  WGPUInstance instance{nullptr};
  WGPUDevice device{nullptr};
  const void* dawn_proc_table{nullptr};
  ValidationMode validation_mode{
#ifndef NDEBUG
      webgpu::ValidationMode::Full                                                       
#else
      webgpu::ValidationMode::Basic                                                          
#endif            
  };
  bool validation_mode_explicitly_set{false};
  bool preserve_device{false};
                                                                                                      
  // can only be used for graph transformation, not execution. Derived from kOrtSessionOptionCompileOnly.
  bool compile_only{false};
  uint32_t max_num_pending_dispatches{16};
  uint64_t max_storage_buffer_binding_size{0};
  WebGpuBufferCacheConfig buffer_cache_config{};
  int power_preference{static_cast<int>(WGPUPowerPreference_HighPerformance)};
  int backend_type{
#ifdef _WIN32
                                                                        
#if defined(DAWN_ENABLE_D3D12)
      static_cast<int>(WGPUBackendType_D3D12)
#elif defined(DAWN_ENABLE_VULKAN)
      static_cast<int>(WGPUBackendType_Vulkan)
#else
      0
#endif
#else
      0
#endif
  };
};

class WebGpuContextFactory {
 public:
  struct WebGpuContextInfo {
    std::unique_ptr<WebGpuContext> context;
    int ref_count;
  };

               
                                                                                                                            
                
  static WebGpuContext& CreateContext(const WebGpuContextConfig& config);

               
                                                                                
                
  static WebGpuContext& GetContext(int context_id);

               
                                                   
                
  static void ReleaseContext(int context_id);

  static void Cleanup();

               
                                                        
                
  static WebGpuContext& DefaultContext();

 private:
  WebGpuContextFactory() {}

  static std::mutex mutex_;
  static std::once_flag init_default_flag_;

                                                                                
                                                                                  
                                                                     
                                                                                  
                                                                                   
                                                                    
                                                                     
  static std::unordered_map<int32_t, WebGpuContextInfo>* contexts_;
  static WGPUInstance default_instance_;
};

                                                                        
class WebGpuContext final {
 public:
  ~WebGpuContext() noexcept;
  Status Wait(wgpu::Future f);

  const wgpu::Device& Device() const { return device_; }

  const wgpu::AdapterInfo& AdapterInfo() const { return adapter_info_; }
  const wgpu::Limits& DeviceLimits() const { return device_limits_; }
  bool DeviceHasFeature(wgpu::FeatureName feature) const { return device_features_.contains(feature); }
#if !defined(__wasm__)
  const wgpu::AdapterPropertiesSubgroupMatrixConfigs& SubgroupMatrixConfigs() const { return subgroup_matrix_configs_; }
#endif

  const wgpu::CommandEncoder& GetCommandEncoder() {
    if (!current_command_encoder_) {
      current_command_encoder_ = device_.CreateCommandEncoder();
    }
    return current_command_encoder_;
  }

  const wgpu::ComputePassEncoder& GetComputePassEncoder() {
    if (!current_compute_pass_encoder_) {
      auto& command_encoder = GetCommandEncoder();

      wgpu::ComputePassDescriptor compute_pass_desc{};

      if (is_profiling_ && query_type_ == TimestampQueryType::AtPasses && graph_capture_state_ != GraphCaptureState::Capturing) {
        wgpu::PassTimestampWrites timestampWrites = {
            nullptr,
            query_set_,
            num_pending_dispatches_ * 2,
            num_pending_dispatches_ * 2 + 1};
        compute_pass_desc.timestampWrites = &timestampWrites;
      }

      current_compute_pass_encoder_ = command_encoder.BeginComputePass(&compute_pass_desc);
    }
    return current_compute_pass_encoder_;
  }

  void EndComputePass() {
    if (current_compute_pass_encoder_) {
      current_compute_pass_encoder_.End();
      current_compute_pass_encoder_ = nullptr;
    }
  }
  void CaptureBegin(std::vector<webgpu::CapturedCommandInfo>* captured_commands, const webgpu::BufferManager& buffer_manager);
  void CaptureEnd();
  void Replay(const std::vector<webgpu::CapturedCommandInfo>& captured_commands, const webgpu::BufferManager& buffer_manager);
  void ReleaseGraphResources(std::vector<webgpu::CapturedCommandInfo>& captured_commands);

  Status Flush(const webgpu::BufferManager& buffer_mgr);

     
                            
     
  webgpu::BufferManager& BufferManager() const { return *buffer_mgr_; }

     
                                        
    
                                                                           
     
  webgpu::BufferManager& InitializerBufferManager() const { return *initializer_buffer_mgr_; }

  inline webgpu::ValidationMode ValidationMode() const {
    return validation_mode_;
  }

                                                                                                        
                                                                                             
  inline bool HasDevice() const { return device_ != nullptr; }

    
                               
    
  const SplitKConfig& GetSplitKConfig() const {
    return *split_k_config_;
  }

                                                                                  
                                                                                       
                                                                                  
                                           
  void SetProfilingStartTime(TimePoint profiling_start_time) { profiling_start_time_ = profiling_start_time; }
  void StartProfiling();
                                                                     
  void CollectProfilingData(profiling::Events& events);
                                                                                   
  void CollectProfilingData();
  void EndProfiling(TimePoint, profiling::Events& events);

    
                      
    
                                                           
    
  void PushErrorScope();

    
                     
    
                                                           
    
  Status PopErrorScope();

  Status Run(ComputeContextBase& context, const ProgramBase& program);

#if defined(ENABLE_PIX_FOR_WEBGPU_EP)
  std::unique_ptr<WebGpuPIXFrameGenerator> CreatePIXFrameGenerator() {
    return std::make_unique<WebGpuPIXFrameGenerator>(instance_,
                                                     Device());
  }
#endif                             

 private:
  enum class TimestampQueryType {
    None = 0,
    InsidePasses,
    AtPasses
  };

  WebGpuContext(WGPUInstance instance,
                WGPUDevice device,
                webgpu::ValidationMode validation_mode,
                bool validation_mode_explicitly_set,
                bool preserve_device,
                uint64_t max_storage_buffer_binding_size)
      : instance_{instance},
        device_{device},
        validation_mode_{validation_mode},
        validation_mode_explicitly_set_{validation_mode_explicitly_set},
        query_type_{TimestampQueryType::None},
        preserve_device_{preserve_device},
        max_storage_buffer_binding_size_{max_storage_buffer_binding_size} {
    ORT_ENFORCE(max_storage_buffer_binding_size_ == 0 || max_storage_buffer_binding_size_ >= 134217728,
                "max_storage_buffer_binding_size must be 0 or at least 128MB");
  }
  ORT_DISALLOW_COPY_ASSIGNMENT_AND_MOVE(WebGpuContext);

  void Initialize(const WebGpuContextConfig& config);

  wgpu::BindGroup CreateBindGroup(const std::vector<WGPUBuffer>& bind_buffers,
                                  const std::vector<uint32_t>& bind_buffers_segments,
                                  const wgpu::BindGroupLayout& bind_group_layout,
                                  std::string_view label) const;
  void DispatchCommand(const webgpu::CapturedCommandInfo& command);
  Status EncodeDeferredDispatches();

  std::vector<const char*> GetEnabledAdapterToggles() const;
  std::vector<const char*> GetEnabledDeviceToggles() const;
  std::vector<const char*> GetDisabledDeviceToggles() const;
  std::vector<wgpu::FeatureName> GetAvailableRequiredFeatures(const wgpu::Adapter& adapter) const;
  wgpu::Limits GetRequiredLimits(const wgpu::Adapter& adapter) const;
  void WriteTimestamp(uint32_t query_index);

  struct PendingQueryInfo {
    PendingQueryInfo(std::vector<PendingKernelInfo>&& kernels, wgpu::Buffer query_buffer)
        : kernels{std::move(kernels)}, query_buffer{query_buffer} {}

    PendingQueryInfo(PendingQueryInfo&&) = default;
    PendingQueryInfo& operator=(PendingQueryInfo&&) = default;
    ORT_DISALLOW_COPY_AND_ASSIGNMENT(PendingQueryInfo);

    std::vector<PendingKernelInfo> kernels;
    wgpu::Buffer query_buffer;
  };

                                                                         
  PendingPipelineBuild* FindPendingPipelineBuild(std::string_view key);
  Status WaitForDeferredPipelineBuilds();

  friend class BufferManager;
  friend class ComputeContext;
  friend class WebGpuContextFactory;

  std::once_flag init_flag_;

  wgpu::Instance instance_;
  wgpu::Device device_;

  webgpu::ValidationMode validation_mode_;
  bool validation_mode_explicitly_set_;

  wgpu::Queue device_queue_;
  wgpu::AdapterInfo adapter_info_;
  wgpu::Limits device_limits_;
  std::unordered_set<wgpu::FeatureName> device_features_;
#if !defined(__wasm__)
  wgpu::AdapterPropertiesSubgroupMatrixConfigs subgroup_matrix_configs_;
#endif

  wgpu::CommandEncoder current_command_encoder_;
  wgpu::ComputePassEncoder current_compute_pass_encoder_;

  std::unique_ptr<webgpu::BufferManager> buffer_mgr_;
  std::unique_ptr<webgpu::BufferManager> initializer_buffer_mgr_;
  std::unique_ptr<ProgramManager> program_mgr_;

  uint32_t num_pending_dispatches_ = 0;
  uint32_t max_num_pending_dispatches_ = 16;

                                                                                                 
  std::vector<webgpu::CapturedCommandInfo> deferred_dispatches_;

  std::unique_ptr<SplitKConfig> split_k_config_;

              
  TimestampQueryType query_type_;
  wgpu::QuerySet query_set_;
  wgpu::Buffer query_resolve_buffer_;

                                                          
  std::vector<PendingKernelInfo> pending_kernels_;
                                                          
  std::vector<PendingQueryInfo> pending_queries_;

  uint64_t gpu_timestamp_offset_ = 0;
                                                                                    
                                              
  TimePoint profiling_start_time_;
                                                                                      
                                                                                       
  int64_t profiling_first_submit_cpu_offset_us_ = -1;
  bool is_profiling_ = false;
                                                         
  profiling::Events events_;
  bool preserve_device_;
  uint64_t max_storage_buffer_binding_size_;
  GraphCaptureState graph_capture_state_{GraphCaptureState::Default};

                                                            
  std::vector<webgpu::CapturedCommandInfo>* external_captured_commands_ = nullptr;
};

}                     
}                          
