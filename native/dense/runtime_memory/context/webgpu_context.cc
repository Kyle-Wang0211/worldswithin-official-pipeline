#include "value_allocation_scope.h"
#include "logical_binding_range.h"
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <memory>
#include <cmath>
#include <string>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#if !defined(__wasm__)
#if !defined(BUILD_DAWN_SHARED_LIBRARY)
#include "dawn/dawn_proc.h"
#endif
#if !defined(USE_EXTERNAL_DAWN)
#include "dawn/native/DawnNative.h"
#endif
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "core/common/common.h"
#include "core/common/path_string.h"
#include "core/platform/env.h"

#include "core/providers/webgpu/compute_context.h"
#include "core/providers/webgpu/webgpu_context.h"
#include "core/providers/webgpu/webgpu_profiler.h"
#include "core/providers/webgpu/buffer_manager.h"
#include "core/providers/webgpu/webgpu_execution_provider.h"
#include "core/providers/webgpu/program.h"
#include "core/providers/webgpu/program_cache_key.h"
#include "core/providers/webgpu/program_manager.h"
#include "core/providers/webgpu/string_macros.h"

#include "bounded_submit.inc"
#include "submit_drain.inc"
#include "device_caps.inc"
#include "graph_copy_registry.inc"

namespace onnxruntime {
namespace webgpu {

void WebGpuContext::Initialize(const WebGpuContextConfig& config) {
  std::call_once(init_flag_, [this, &config]() {
    max_num_pending_dispatches_ = config.max_num_pending_dispatches;

                                                                                        
                                                                                                           
                                                                                                              
                                                                                         
                                                                                         
                                                                                                               
                                                                                                 
    if (config.compile_only) {
                                                                                                              
                                                                           
      LOGS_DEFAULT(INFO) << "WebGPU EP context created device-free (compile-only session, no Dawn device).";
      return;
    }

    if (device_ == nullptr) {
                             
      wgpu::RequestAdapterOptions req_adapter_options = {};
      req_adapter_options.backendType = static_cast<wgpu::BackendType>(config.backend_type);
      req_adapter_options.powerPreference = static_cast<wgpu::PowerPreference>(config.power_preference);

#if !defined(__wasm__)
      auto enabled_adapter_toggles = GetEnabledAdapterToggles();

      wgpu::DawnTogglesDescriptor adapter_toggles_desc = {};
      adapter_toggles_desc.enabledToggleCount = enabled_adapter_toggles.size();
      adapter_toggles_desc.enabledToggles = enabled_adapter_toggles.data();

      req_adapter_options.nextInChain = &adapter_toggles_desc;
#endif

                                                                                  
                                                                                             
                                                                                           
                                                       
      struct RequestAdapterResult {
        wgpu::RequestAdapterStatus status = wgpu::RequestAdapterStatus::Error;
        wgpu::Adapter adapter;
        std::string message;
      };
      RequestAdapterResult adapter_result;
      ORT_ENFORCE(wgpu::WaitStatus::Success == instance_.WaitAny(instance_.RequestAdapter(
                                                                     &req_adapter_options,
                                                                     wgpu::CallbackMode::WaitAnyOnly,
                                                                     [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message,
                                                                        RequestAdapterResult* result) noexcept {
                                                                       result->status = status;
                                                                       if (status == wgpu::RequestAdapterStatus::Success) {
                                                                         result->adapter = std::move(adapter);
                                                                       } else {
                                                                         result->message = std::string{message};
                                                                       }
                                                                     },
                                                                     &adapter_result),
                                                                 UINT64_MAX));
      ORT_ENFORCE(adapter_result.status == wgpu::RequestAdapterStatus::Success,
                  "Failed to get a WebGPU adapter: ", adapter_result.message);
      wgpu::Adapter adapter = std::move(adapter_result.adapter);
      ORT_ENFORCE(adapter != nullptr, "Failed to get a WebGPU adapter.");

                            
      wgpu::DeviceDescriptor device_desc = {};

#if !defined(__wasm__)
      wgpu::DawnTogglesDescriptor device_toggles_desc = {};
      device_desc.nextInChain = &device_toggles_desc;

      auto enabled_device_toggles = GetEnabledDeviceToggles();
      device_toggles_desc.enabledToggleCount = enabled_device_toggles.size();
      device_toggles_desc.enabledToggles = enabled_device_toggles.data();

      auto disabled_device_toggles = GetDisabledDeviceToggles();
      device_toggles_desc.disabledToggleCount = disabled_device_toggles.size();
      device_toggles_desc.disabledToggles = disabled_device_toggles.data();
#endif

      std::vector<wgpu::FeatureName> required_features = GetAvailableRequiredFeatures(adapter);
      if (!required_features.empty()) {
        device_desc.requiredFeatures = required_features.data();
        device_desc.requiredFeatureCount = required_features.size();
      }
      wgpu::Limits required_limits = GetRequiredLimits(adapter);
      device_desc.requiredLimits = &required_limits;

                                              
      device_desc.SetUncapturedErrorCallback(
                                                    
          [](const wgpu::Device&           , wgpu::ErrorType type,
             wgpu::StringView message) noexcept {
            if (logging::LoggingManager::HasDefaultLogger()) {
              LOGS_DEFAULT(ERROR) << "WebGPU device error(" << int(type) << "): " << std::string_view{message};
            }
          });
                                                    
      device_desc.SetDeviceLostCallback(
          wgpu::CallbackMode::AllowSpontaneous,
                                                    
          [](const wgpu::Device&           , wgpu::DeviceLostReason reason, wgpu::StringView message) noexcept {
            if (logging::LoggingManager::HasDefaultLogger()) {
              LOGS_DEFAULT(INFO) << "WebGPU device lost (" << int(reason) << "): " << std::string_view{message};
            }
          });

      struct RequestDeviceResult {
        wgpu::RequestDeviceStatus status = wgpu::RequestDeviceStatus::Error;
        wgpu::Device device;
        std::string message;
      };
      RequestDeviceResult device_result;
      ORT_ENFORCE(wgpu::WaitStatus::Success == instance_.WaitAny(adapter.RequestDevice(
                                                                     &device_desc,
                                                                     wgpu::CallbackMode::WaitAnyOnly,
                                                                                                               
                                                                     [](wgpu::RequestDeviceStatus status,
                                                                        wgpu::Device device,
                                                                        wgpu::StringView message,
                                                                        RequestDeviceResult* result) noexcept {
                                                                       result->status = status;
                                                                       if (status == wgpu::RequestDeviceStatus::Success) {
                                                                         result->device = std::move(device);
                                                                       } else {
                                                                         result->message = std::string{message};
                                                                       }
                                                                     },
                                                                     &device_result),
                                                                 UINT64_MAX));
      ORT_ENFORCE(device_result.status == wgpu::RequestDeviceStatus::Success,
                  "Failed to get a WebGPU device: ", device_result.message);
      device_ = std::move(device_result.device);
      ORT_ENFORCE(device_ != nullptr, "Failed to get a WebGPU device.");
    }

    LOGS_DEFAULT(VERBOSE) << "WebGPU EP Context is created for: Instance=" << instance_.Get() << ", Device=" << device_.Get() << ".";

                         
    device_queue_ = device_.GetQueue();
                          
    ORT_ENFORCE(Device().GetLimits(&device_limits_));
                                                                                        
                                                                                         
    if (device_limits_.minStorageBufferOffsetAlignment > 0) {
      device_limits_.maxStorageBufferBindingSize -=
          (device_limits_.maxStorageBufferBindingSize % device_limits_.minStorageBufferOffsetAlignment);
    }
                            
    wgpu::SupportedFeatures supported_features;
    Device().GetFeatures(&supported_features);
    for (size_t i = 0; i < supported_features.featureCount; i++) {
      device_features_.insert(supported_features.features[i]);
    }
                         
#if !defined(__wasm__)
    if (DeviceHasFeature(wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix)) {
      adapter_info_.nextInChain = &subgroup_matrix_configs_;
    }
#endif
    ORT_ENFORCE(Device().GetAdapterInfo(&adapter_info_));

                            
    buffer_mgr_ = BufferManagerFactory::Create(*this,
                                               config.buffer_cache_config.storage.mode,
                                               config.buffer_cache_config.uniform.mode,
                                               config.buffer_cache_config.query_resolve.mode,
                                               config.buffer_cache_config.default_entry.mode);

                                         
    initializer_buffer_mgr_ = BufferManagerFactory::Create(*this,
                                                           BufferCacheMode::LazyRelease,
                                                           BufferCacheMode::LazyRelease,
                                                           BufferCacheMode::Disabled,
                                                           BufferCacheMode::Disabled);

                             
    program_mgr_ = std::make_unique<ProgramManager>(*this);

                            
    split_k_config_ = std::make_unique<SplitKConfig>(adapter_info_);

                     
#if !defined(__wasm__)
    if (DeviceHasFeature(wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses)) {
      query_type_ = TimestampQueryType::InsidePasses;
    } else
#endif
        if (DeviceHasFeature(wgpu::FeatureName::TimestampQuery)) {
      query_type_ = TimestampQueryType::AtPasses;
    } else {
      query_type_ = TimestampQueryType::None;
    }
                                                                                             
    const auto pw_caps = PwRecordCaps(config.context_id, device_.Get(), adapter_info_,
      DeviceHasFeature(wgpu::FeatureName::ShaderF16), DeviceHasFeature(wgpu::FeatureName::Subgroups),
      DeviceHasFeature(wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix),
      DeviceHasFeature(wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses),
      DeviceHasFeature(wgpu::FeatureName::TimestampQuery), uint32_t(query_type_), is_profiling_, subgroup_matrix_configs_);
    if (logging::LoggingManager::HasDefaultLogger()) { LOGS_DEFAULT(WARNING) << pw_caps; }
  });

  if (max_num_pending_dispatches_ != config.max_num_pending_dispatches) {
    LOGS_DEFAULT(WARNING)
        << "WebGPU context is already initialized with "
        << "maxNumPendingDispatches="
        << max_num_pending_dispatches_
        << ". Requested value "
        << config.max_num_pending_dispatches
        << " will be ignored.";
  }
}

WebGpuContext::~WebGpuContext() noexcept { PwFifoShutdownDrain(); }

Status WebGpuContext::Wait(wgpu::Future f) {
  auto status = instance_.WaitAny(f, UINT64_MAX);
  if (status == wgpu::WaitStatus::Success) {
    return Status::OK();
  }
  return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Failed to wait for the operation:", uint32_t(status));
}

PendingPipelineBuild* WebGpuContext::FindPendingPipelineBuild(std::string_view key) {
  for (auto& dispatch : deferred_dispatches_) {
    if (dispatch.program_key == key && dispatch.pending_build) {
      return &*dispatch.pending_build;
    }
  }

  return nullptr;
}

Status WebGpuContext::WaitForDeferredPipelineBuilds() {
  Status result = Status::OK();
  for (auto& dispatch : deferred_dispatches_) {
    if (dispatch.compute_pipeline) {
      continue;
    }

    const ProgramArtifact* artifact = program_mgr_->Get(dispatch.program_key);
                                                                                                    
                                                                                                  
                                    
    if (artifact != nullptr && !dispatch.pending_build) {
      dispatch.compute_pipeline = artifact->compute_pipeline;
      continue;
    }

    if (!dispatch.pending_build) {
      result = ORT_MAKE_STATUS(ONNXRUNTIME, FAIL,
                               "No cached or pending pipeline for deferred dispatch: ", dispatch.program_key);
                                                                                                 
                                                                                                   
                                                               
      continue;
    }

                                                                                                    
                                                                                                  
                                                                        
    auto& build = *dispatch.pending_build;
    Status wait_status = Wait(build.future);
    if (!wait_status.IsOK()) {
      result = wait_status;
      continue;
    }
    if (build.callback_context && !build.callback_context->status.IsOK()) {
      result = build.callback_context->status;
      continue;
    }

    if (artifact == nullptr) {
      ProgramArtifact completed_artifact{std::move(build.name), std::move(build.callback_context->pipeline),
                                         std::move(build.bind_group_layout),
                                         std::move(build.shape_uniform_ranks)};
      artifact = program_mgr_->Set(dispatch.program_key, std::move(completed_artifact));
    }
    dispatch.compute_pipeline = artifact->compute_pipeline;
    dispatch.pending_build.reset();
  }

  return result;
}

Status WebGpuContext::EncodeDeferredDispatches() {
  if (deferred_dispatches_.empty()) {
    return Status::OK();
  }

  auto reset_deferred_state = [this]() {
    deferred_dispatches_.clear();
  };

                                                                                                   
  Status result = WaitForDeferredPipelineBuilds();
  if (!result.IsOK()) {
    reset_deferred_state();
    return result;
  }

                                                                                               
  for (auto& dispatch : deferred_dispatches_) {
                                                                                               
                                                                                             
    if (is_profiling_ && dispatch.pending_kernel_info.has_value()) {
      if (graph_capture_state_ != GraphCaptureState::Capturing) {
        pending_kernels_.emplace_back(std::move(*dispatch.pending_kernel_info));
      }
    }
    DispatchCommand(dispatch);
    if (graph_capture_state_ == GraphCaptureState::Capturing) {
      ORT_ENFORCE(external_captured_commands_ != nullptr);
      external_captured_commands_->push_back(std::move(dispatch));
    }
  }

  reset_deferred_state();
  return result;
}

Status WebGpuContext::Run(ComputeContextBase& context, const ProgramBase& program) {
  const auto& inputs = program.Inputs();
  const auto& outputs = program.Outputs();

  if (outputs.empty()) {
    return Status::OK();
  }

                                                      
  if (ValidationMode() >= ValidationMode::Basic) {
    ORT_ENFORCE(std::all_of(inputs.begin(), inputs.end(), [](const ProgramInput& input) {
                  const auto* tensor = input.tensor;
                  return tensor != nullptr &&
                         tensor->Location().mem_type == OrtMemType::OrtMemTypeDefault &&
                         tensor->Location().device.Type() == OrtDevice::GPU &&
                         !strcmp(tensor->Location().name.c_str(), WEBGPU_BUFFER);
                }),
                "All inputs must be tensors on WebGPU buffers.");

    if (program.IndirectDispatchTensor() != nullptr) {
      ORT_ENFORCE(!inputs.empty() && inputs.back().tensor == program.IndirectDispatchTensor(),
                  "The indirect dispatch tensor must be the last input. "
                  "Ensure no call to program.AddInput() occurs after program.SetIndirectDispatchTensor().");
    }

    ORT_ENFORCE(std::all_of(outputs.begin(), outputs.end(), [](const ProgramOutput& output) {
                  const auto* tensor = output.tensor;
                  return tensor != nullptr &&
                         tensor->Location().mem_type == OrtMemType::OrtMemTypeDefault &&
                         tensor->Location().device.Type() == OrtDevice::GPU &&
                         !strcmp(tensor->Location().name.c_str(), WEBGPU_BUFFER);
                }),
                "All outputs must be tensors on WebGPU buffers.");
  }

  const ProgramMetadata& metadata = program.Metadata();

                              
  if (ValidationMode() >= ValidationMode::Basic) {
    const auto& [constants, overridable_constants, uniform_variables] = metadata;

                                  
    ORT_RETURN_IF(program.OverridableConstants().size() != overridable_constants.size(),
                  "Size of overridable constants mismatch in program \"", program.Name(),
                  "\", Expected: ", overridable_constants.size(),
                  ", Actual: ", program.OverridableConstants().size());

    if (ValidationMode() >= ValidationMode::Full) {
      size_t num_overridable_constants = program.OverridableConstants().size();
      for (size_t i = 0; i < num_overridable_constants; ++i) {
        const auto& override_value = program.OverridableConstants()[i];
        const auto& definition = overridable_constants[i];
        ORT_RETURN_IF(override_value.has_value && override_value.type != definition.type,
                      "Overridable override_value[", i, "] (", definition.name, ") data type mismatch in program \"", program.Name(),
                      "\", Expected: ", definition.type,
                      ", Actual: ", override_value.type);
        ORT_RETURN_IF(!override_value.has_value && !definition.has_default_value,
                      "Overridable override_value[", i, "] (", definition.name, ") no override_value specified in program \"", program.Name(),
                      "\"");
      }
    }

                              
    ORT_RETURN_IF(program.UniformVariables().size() != uniform_variables.size(),
                  "Size of uniform_value variables mismatch in program \"", program.Name(),
                  "\", Expected: ", uniform_variables.size(),
                  ", Actual: ", program.UniformVariables().size());

    if (ValidationMode() >= ValidationMode::Full) {
      size_t num_uniform_variables = program.UniformVariables().size();
      for (size_t i = 0; i < num_uniform_variables; ++i) {
        const auto& uniform_value = program.UniformVariables()[i];
        const auto& definition = uniform_variables[i];
        ORT_RETURN_IF(uniform_value.length > 0 && uniform_value.data_type != definition.data_type,
                      "Uniform variable[", i, "] (", definition.name, ") data type mismatch in program \"", program.Name(),
                      "\", Expected: ", definition.data_type,
                      ", Actual: ", uniform_value.data_type);
      }
    }
  }

                                                                         
    
                                                                                                                      
                                                                                                                
                                                  
    
                                                                                  

  std::vector<uint32_t> inputs_segments;
  std::vector<uint32_t> outputs_segments;
  ORT_RETURN_IF_ERROR(program_mgr_->CalculateSegmentsForInputsAndOutputs(program, inputs_segments, outputs_segments));

  uint32_t x = program.DispatchGroupSizeX();
  uint32_t y = program.DispatchGroupSizeY();
  uint32_t z = program.DispatchGroupSizeZ();

                                                                                                    
  if (program.IndirectDispatchTensor() == nullptr) {
    ORT_RETURN_IF_ERROR(program_mgr_->NormalizeDispatchGroupSize(x, y, z));
  } else {
    ORT_ENFORCE(x == 0 && y == 0 && z == 0,
                "Only one of SetIndirectDispatchTensor and SetDispatchGroupSize should be called for program", program.Name());
  }

  bool is_1d_dispatch = (y == 1 && z == 1);

  auto key = CalculateProgramCacheKey(program, inputs_segments, outputs_segments, is_1d_dispatch);

  LOGS(context.Logger(), INFO) << "Starting program \"" << key << "\" (" << x << ", " << y << ", " << z << ")";
                                                                        
                                                                                                       
  const auto* program_artifact = program_mgr_->Get(key);

                                                                                                  
                                                
  std::optional<PendingPipelineBuild> pending_build;
  const std::vector<int>* deferred_ranks = nullptr;
  const wgpu::BindGroupLayout* bind_group_layout = nullptr;
  if (program_artifact == nullptr) {
    PendingPipelineBuild* in_flight_build = FindPendingPipelineBuild(key);

                                                                               
    if (in_flight_build == nullptr) {
      auto& build = pending_build.emplace();
      build.name = program.Name();
      build.callback_context = std::make_unique<PipelineCallbackContext>();
      ORT_RETURN_IF_ERROR(program_mgr_->Build(program, metadata, inputs_segments, outputs_segments,
                                              key, x, y, z,
                                              build.bind_group_layout,
                                              build.shape_uniform_ranks,
                                              build.future,
                                              *build.callback_context));
      in_flight_build = &*pending_build;
    }
    deferred_ranks = &in_flight_build->shape_uniform_ranks;
    bind_group_layout = &in_flight_build->bind_group_layout;
  } else {
    bind_group_layout = &program_artifact->bind_group_layout;
  }

                                                                                   
                                                                                                 
                                     
  const std::vector<int>& shape_uniform_ranks = deferred_ranks ? *deferred_ranks
                                                               : program_artifact->shape_uniform_ranks;
  std::vector<ProgramUniformVariableValue> shape_uniforms;
  shape_uniforms.reserve(shape_uniform_ranks.size() * 2);
  if (ValidationMode() >= ValidationMode::Basic) {
    ORT_RETURN_IF_NOT(shape_uniform_ranks.size() == inputs.size() + outputs.size() + program.Indices().size(),
                      "Invalid program artifact: variable size (", shape_uniform_ranks.size(),
                      ") does not match current program (input: ", inputs.size(),
                      ", output: ", outputs.size(),
                      ", indices: ", program.Indices().size(), ")");
  }

  auto append_shape_uniforms = [&shape_uniforms, &shape_uniform_ranks](size_t i, const TensorShape& shape) {
    if (shape_uniform_ranks[i] > 0) {
      size_t expected_rank = static_cast<size_t>(shape_uniform_ranks[i]);
      ORT_RETURN_IF(expected_rank != shape.NumDimensions(),
                    "Invalid program artifact: variable[", i, "] rank mismatch. Expected: ", expected_rank,
                    ", Actual: ", shape.NumDimensions());

      std::vector<uint32_t> dims(expected_rank);
      std::vector<uint32_t> stride(expected_rank - 1);
      for (size_t j = 0; j < expected_rank; ++j) {
        dims[j] = onnxruntime::narrow<uint32_t>(shape[j]);
        if (j < expected_rank - 1) {
          stride[j] = onnxruntime::narrow<uint32_t>(shape.SizeFromDimension(j + 1));
        }
      }

      shape_uniforms.emplace_back(gsl::make_span(dims));
      if (expected_rank > 1) {
        shape_uniforms.emplace_back(gsl::make_span(stride));
      }
    }
    return Status::OK();
  };

  for (size_t i = 0; i < inputs.size(); i++) {
    ORT_RETURN_IF_ERROR(append_shape_uniforms(i,
                                              inputs[i].use_override_shape ? inputs[i].override_shape : inputs[i].tensor->Shape()));
  }
  for (size_t i = 0; i < outputs.size(); i++) {
    ORT_RETURN_IF_ERROR(append_shape_uniforms(i + inputs.size(),
                                              outputs[i].use_override_shape ? outputs[i].override_shape : outputs[i].tensor->Shape()));
  }
  for (size_t i = 0; i < program.Indices().size(); i++) {
    ORT_RETURN_IF_ERROR(append_shape_uniforms(i + inputs.size() + outputs.size(), program.Indices()[i]));
  }

  const size_t uniform_count = shape_uniforms.size() + program.UniformVariables().size();
  size_t current_offset = 0;
  std::vector<std::tuple<const ProgramUniformVariableValue&, size_t>> uniform_and_offsets;
  uniform_and_offsets.reserve(uniform_count);
  for (size_t i = 0; i < uniform_count; i++) {
    const auto& uniform = i < shape_uniforms.size() ? shape_uniforms[i]
                                                    : program.UniformVariables()[i - shape_uniforms.size()];
    size_t length = uniform.length;
    if (length == 0) {                             
      continue;
    }

                                                                
      
                                          
      
               
                                                                                               
                                                                     
                                                                     
                                                                    
                                                                    
      
                                       
                                                                                               
                                                                     
                                                                     
                                                                    
                                                                    
      

    const bool is_f16 = uniform.data_type == ProgramUniformVariableDataType::Float16;

    size_t variable_alignment = 4;                                       
    size_t variable_size = 4;                                       

    if (is_f16) {
      if (length > 6) {
        variable_alignment = 16;
        variable_size = 16 * ((length + 7) / 8);
      } else if (length > 4) {
        variable_alignment = 16;
        variable_size = 12;
      } else if (length > 2) {
        variable_alignment = 8;
        variable_size = 8;
      }
    } else {
      if (length > 3) {
        variable_alignment = 16;
        variable_size = 16 * ((length + 3) / 4);
      } else if (length > 2) {
        variable_alignment = 16;
        variable_size = 12;
      } else if (length > 1) {
        variable_alignment = 8;
        variable_size = 8;
      }
    }
    current_offset = (current_offset + variable_alignment - 1) / variable_alignment * variable_alignment;
    uniform_and_offsets.emplace_back(uniform, current_offset);

    current_offset += variable_size;
  }

                                                                                                       
                                                                                        
  constexpr size_t max_alignment_of_field = 16;
  const size_t uniform_buffer_total_size = (current_offset + max_alignment_of_field - 1) / max_alignment_of_field * max_alignment_of_field;

  WGPUBuffer uniform_buffer = nullptr;
  const webgpu::BufferManager& buffer_mgr = ComputeContextBase::BufferManagerAccessor::Get(context);
  if (uniform_buffer_total_size > 0) {
    std::vector<uint8_t> uniform_data_buffer(uniform_buffer_total_size);

    for (auto const& [uniform, offset] : uniform_and_offsets) {
      memcpy(uniform_data_buffer.data() + offset, uniform.data.data(), uniform.data.size());
    }

    uniform_buffer = buffer_mgr.Create(uniform_buffer_total_size, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform);
    device_queue_.WriteBuffer(uniform_buffer, 0, uniform_data_buffer.data(), uniform_buffer_total_size);
  }

  const size_t total_buffer_count = inputs.size() + outputs.size() + (uniform_buffer ? 1 : 0);

  std::vector<WGPUBuffer> bind_buffers;
  std::vector<uint32_t> bind_buffers_segments;
  std::vector<uint64_t> bind_logical_bytes;
  bind_buffers.reserve(total_buffer_count);
  bind_buffers_segments.reserve(total_buffer_count);
  for (size_t i = 0; i < inputs.size(); i++) {
    bind_buffers.push_back(reinterpret_cast<WGPUBuffer>(const_cast<void*>(inputs[i].tensor->DataRaw())));
    bind_buffers_segments.push_back(inputs_segments[i]);
    bind_logical_bytes.push_back(reconstruction_official_object_buffer_shared(uintptr_t(bind_buffers.back()))?inputs[i].tensor->SizeInBytes():0);
  }
  for (size_t i = 0; i < outputs.size(); i++) {
    bind_buffers.push_back(reinterpret_cast<WGPUBuffer>(outputs[i].tensor->MutableDataRaw()));
    bind_buffers_segments.push_back(outputs_segments[i]);
    bind_logical_bytes.push_back(reconstruction_official_object_buffer_shared(uintptr_t(bind_buffers.back()))?outputs[i].tensor->SizeInBytes():0);
  }
  if (uniform_buffer) {
    bind_buffers.push_back(uniform_buffer);
    bind_buffers_segments.push_back(1);                                         
    bind_logical_bytes.push_back(0);
  }

                                                                                                   
                                      
  webgpu::CapturedCommandInfo command;
  command.program_key = key;
  if (program_artifact != nullptr) {
    command.compute_pipeline = program_artifact->compute_pipeline;
  }
  command.bind_group = CreateBindGroup(bind_buffers, bind_buffers_segments, bind_logical_bytes,
                                       *bind_group_layout, program.Name());
  command.pending_build = std::move(pending_build);
  if (uniform_buffer) {
                                                                                            
    buffer_mgr.Release(uniform_buffer);
  }
  command.dispatch_group = {x, y, z};
  if (program.IndirectDispatchTensor() != nullptr) {
    command.indirect_buffer = reinterpret_cast<WGPUBuffer>(
        const_cast<void*>(program.IndirectDispatchTensor()->DataRaw()));
  }
                                                                                                 
  if (is_profiling_) {
    command.pending_kernel_info.emplace(context.NodeName(), context.OpType(), program.Name(),
                                        key, inputs, outputs);
  }
  deferred_dispatches_.push_back(std::move(command));

                                                                                              
                                                                               
  if (deferred_dispatches_.size() >= max_num_pending_dispatches_) {
    ORT_RETURN_IF_ERROR(Flush(buffer_mgr));
  }
  return Status::OK();
}

std::vector<const char*> WebGpuContext::GetEnabledAdapterToggles() const {
                                                          
                                                          
                                                           
  constexpr const char* toggles[] = {
      "use_dxc",
      "allow_unsafe_apis",
      "decompose_uniform_buffers",
#if defined(DAWN_ENABLE_VULKAN)
      "use_vulkan_memory_model",
      "vulkan_enable_f16_on_nvidia",
#endif
  };
  return std::vector<const char*>(std::begin(toggles), std::end(toggles));
}

std::vector<const char*> WebGpuContext::GetEnabledDeviceToggles() const {
                                                                    
                                                                                
  constexpr const char* toggles[] = {
      "skip_validation",
      "disable_robustness",
      "d3d_disable_ieee_strictness",
  };
#ifndef NDEBUG
                                                                                                
                                                                                 
  ORT_UNUSED_PARAMETER(validation_mode_explicitly_set_);
  return std::vector<const char*>(ValidationMode() >= ValidationMode::WGPUOnly
                                      ? std::begin(toggles) + 1
                                      : std::begin(toggles),
                                  std::end(toggles));
#else
                                                                                  
                                                 
  if (!validation_mode_explicitly_set_) {
    return std::vector<const char*>(std::begin(toggles), std::end(toggles));
  }
  return std::vector<const char*>(ValidationMode() >= ValidationMode::WGPUOnly
                                      ? std::begin(toggles) + 1
                                      : std::begin(toggles),
                                  std::end(toggles));
#endif
}

std::vector<const char*> WebGpuContext::GetDisabledDeviceToggles() const {
  constexpr const char* toggles[] = {
      "lazy_clear_resource_on_first_use",
      "timestamp_quantization",
  };
  return std::vector<const char*>(std::begin(toggles), std::end(toggles));
}

std::vector<wgpu::FeatureName> WebGpuContext::GetAvailableRequiredFeatures(const wgpu::Adapter& adapter) const {
  std::vector<wgpu::FeatureName> required_features;
  constexpr wgpu::FeatureName features[]{
#if !defined(__wasm__)
      wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses,
      wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix,
#endif
      wgpu::FeatureName::TimestampQuery,
      wgpu::FeatureName::ShaderF16,
      wgpu::FeatureName::Subgroups,
#if !defined(__wasm__)
      wgpu::FeatureName::BufferMapExtendedUsages,
#endif
  };
  for (auto feature : features) {
    if (adapter.HasFeature(feature)) {
      required_features.push_back(feature);
    }
  }
  return required_features;
}

wgpu::Limits WebGpuContext::GetRequiredLimits(const wgpu::Adapter& adapter) const {
  wgpu::Limits required_limits{};
  wgpu::Limits adapter_limits;
  ORT_ENFORCE(adapter.GetLimits(&adapter_limits));

  required_limits.maxBindGroups = adapter_limits.maxBindGroups;
  required_limits.maxComputeWorkgroupStorageSize = adapter_limits.maxComputeWorkgroupStorageSize;
  required_limits.maxComputeWorkgroupsPerDimension = adapter_limits.maxComputeWorkgroupsPerDimension;
  required_limits.maxStorageBuffersPerShaderStage = adapter_limits.maxStorageBuffersPerShaderStage;

  if (max_storage_buffer_binding_size_ == 0) {
                                                     
    required_limits.maxStorageBufferBindingSize = adapter_limits.maxStorageBufferBindingSize;
  } else {
    required_limits.maxStorageBufferBindingSize = max_storage_buffer_binding_size_;
  }

  required_limits.maxBufferSize = adapter_limits.maxBufferSize;
  required_limits.maxComputeInvocationsPerWorkgroup = adapter_limits.maxComputeInvocationsPerWorkgroup;
  required_limits.maxComputeWorkgroupSizeX = adapter_limits.maxComputeWorkgroupSizeX;
  required_limits.maxComputeWorkgroupSizeY = adapter_limits.maxComputeWorkgroupSizeY;
  required_limits.maxComputeWorkgroupSizeZ = adapter_limits.maxComputeWorkgroupSizeZ;

  return required_limits;
}

void WebGpuContext::WriteTimestamp(uint32_t query_index) {
  if (!is_profiling_ || graph_capture_state_ == GraphCaptureState::Capturing || query_type_ != TimestampQueryType::InsidePasses) {
    return;
  }

  const auto& compute_pass_encoder = GetComputePassEncoder();
  compute_pass_encoder.WriteTimestamp(query_set_, query_index);
}

void WebGpuContext::StartProfiling() {
  if (query_type_ == TimestampQueryType::None) {
    return;
  }

  is_profiling_ = true;
                                                                                     
                                                                                     
                                                         
  gpu_timestamp_offset_ = 0;
  profiling_first_submit_cpu_offset_us_ = -1;

  const uint32_t query_count = max_num_pending_dispatches_ * 2;

  if (!query_set_) {
                       
    wgpu::QuerySetDescriptor querySetDescriptor;
    querySetDescriptor.count = query_count;
    querySetDescriptor.type = wgpu::QueryType::Timestamp;
    query_set_ = device_.CreateQuerySet(&querySetDescriptor);
  }

  if (!query_resolve_buffer_) {
                            
    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = query_count * sizeof(uint64_t);
    bufferDescriptor.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc |
                             wgpu::BufferUsage::CopyDst;
    query_resolve_buffer_ = device_.CreateBuffer(&bufferDescriptor);
  }
}

void WebGpuContext::CollectProfilingData(profiling::Events& events) {
  if (!pending_queries_.empty()) {
                                                                                 
                                                                                        
                                                                               
    int64_t cpu_offset_us = profiling_first_submit_cpu_offset_us_ > 0
                                ? profiling_first_submit_cpu_offset_us_
                                : 0;

    for (const auto& pending_query : pending_queries_) {
      const auto& pending_kernels = pending_query.kernels;
      const auto& query_read_buffer = pending_query.query_buffer;

      struct MapAsyncResult {
        wgpu::MapAsyncStatus status{};
        std::string message{};
      } map_async_result;

      ORT_THROW_IF_ERROR(Wait(query_read_buffer.MapAsync(
          wgpu::MapMode::Read,
          0,
          static_cast<size_t>(query_read_buffer.GetSize()),
          wgpu::CallbackMode::WaitAnyOnly,
                                                    
          [](wgpu::MapAsyncStatus status, wgpu::StringView message, MapAsyncResult* result) noexcept {
            result->status = status;
            if (auto message_sv = static_cast<std::string_view>(message);
                !message_sv.empty()) {
              result->message = std::string{message_sv};
            }
          },
          &map_async_result)));

      ORT_ENFORCE(map_async_result.status == wgpu::MapAsyncStatus::Success,
                  "Failed to download data from buffer. wgpu::MapAsyncStatus value: ",
                  static_cast<int>(map_async_result.status), ", message: ", map_async_result.message);

      auto mapped_data = static_cast<const uint64_t*>(query_read_buffer.GetConstMappedRange());

      for (size_t i = 0; i < pending_kernels.size(); i++) {
        const PendingKernelInfo& pending_kernel_info = pending_kernels[i];
        const auto& input_shapes = pending_kernel_info.input_shapes;
        const auto& output_shapes = pending_kernel_info.output_shapes;

        SS(shapes, 128);
        for (size_t s = 0; s < input_shapes.size(); s++) {
          shapes << "inputs[" << s << "] = " << input_shapes[s].ToString() << " ";
        }
        for (size_t s = 0; s < output_shapes.size(); s++) {
          shapes << "outputs[" << s << "] = " << output_shapes[s].ToString() << " ";
        }

        if (gpu_timestamp_offset_ == 0) {
          gpu_timestamp_offset_ = mapped_data[i * 2];
        }
        uint64_t start_time = mapped_data[i * 2] - gpu_timestamp_offset_;
        uint64_t end_time = mapped_data[i * 2 + 1] - gpu_timestamp_offset_;

        InlinedHashMap<std::string, std::string> event_args = {
            {"shapes", SS_GET(shapes)},
            {"cache_key", pending_kernel_info.cache_key},
        };

        profiling::EventRecord event(profiling::API_EVENT,
                                     -1,
                                     -1,
                                     pending_kernel_info.name,
                                     static_cast<int64_t>(std::round(start_time / 1000.0)) + cpu_offset_us,
                                     static_cast<int64_t>(std::round((end_time - start_time) / 1000.0)),
                                     event_args);
        events.emplace_back(std::move(event));
      }

      query_read_buffer.Unmap();
      query_read_buffer.Destroy();
    }

    pending_queries_.clear();
  }

  is_profiling_ = false;
}

void WebGpuContext::CollectProfilingData() {
  CollectProfilingData(events_);
}

void WebGpuContext::EndProfiling(TimePoint         , profiling::Events& events) {
                                                                 
  ORT_ENFORCE(!is_profiling_, "Profiling is ongoing in an inference run.");

  if (query_type_ != TimestampQueryType::None) {
                                                                                                                              
    ORT_ENFORCE(pending_kernels_.empty() && pending_queries_.empty(), "Pending kernels or queries are not empty.");

    events.insert(events.end(),
                  std::make_move_iterator(events_.begin()),
                  std::make_move_iterator(events_.end()));
    events_.clear();
  } else {
    LOGS_DEFAULT(WARNING) << "TimestampQuery is not supported in this device.";
  }
}

void WebGpuContext::PushErrorScope() { device_.PushErrorScope(wgpu::ErrorFilter::Validation); }

Status WebGpuContext::PopErrorScope() {
  Status status{};
  ORT_RETURN_IF_ERROR(Wait(device_.PopErrorScope(
      wgpu::CallbackMode::WaitAnyOnly,
                                                
      [](wgpu::PopErrorScopeStatus pop_status, wgpu::ErrorType error_type, wgpu::StringView message,
         Status* status) noexcept {
        if (pop_status != wgpu::PopErrorScopeStatus::Success) {
          *status = ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Failed to pop WebGPU error scope. status=",
                                    static_cast<uint32_t>(pop_status));
        } else if (error_type != wgpu::ErrorType::NoError) {
          *status = ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "WebGPU validation failed. ", std::string_view(message));
        }
      },
      &status)));
  return status;
}

Status WebGpuContext::Flush(const webgpu::BufferManager& buffer_mgr) {
  ORT_RETURN_IF_ERROR(ReconstructionDrainValidatePolicy());
  auto bounded_submit_guard = PwBoundedSerializeFlush();
  ORT_RETURN_IF_ERROR(ReconstructionDrainValidateCapture(graph_capture_state_));
  if (ReconstructionDrainPolicyValue().enabled) ORT_RETURN_IF_ERROR(ReconstructionDrainRejectReadyError());
  ORT_RETURN_IF_ERROR(PwBoundedCheckPolicy());
  try {
  Status status = EncodeDeferredDispatches();
  if (!current_command_encoder_) {
    if (!status.IsOK() && PwGetBoundedPolicy().enabled) {
      PwFifoLatch();
      auto drain = PwFifoDrainLocked(PwFifoClock::now()+std::chrono::seconds(30), false);
      if (!drain.IsOK()) return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, status.ErrorMessage(), "; drain: ", drain.ErrorMessage());
    }
    return status;
  }

  EndComputePass();

  if (is_profiling_ && num_pending_dispatches_ > 0 && graph_capture_state_ != GraphCaptureState::Capturing) {
    ORT_ENFORCE(num_pending_dispatches_ == pending_kernels_.size(),
                "Number of pending dispatches (", num_pending_dispatches_,
                ") does not match pending kernels size (", pending_kernels_.size(), ")");

                                                                                       
                                                                                       
    if (profiling_first_submit_cpu_offset_us_ < 0) {
      profiling_first_submit_cpu_offset_us_ = TimeDiffMicroSeconds(profiling_start_time_);
    }

    uint32_t query_count = num_pending_dispatches_ * 2;
    current_command_encoder_.ResolveQuerySet(
        query_set_,
        0,
        query_count,
        query_resolve_buffer_,
        0);

    wgpu::BufferDescriptor bufferDescriptor;
    bufferDescriptor.size = query_count * sizeof(uint64_t);
    bufferDescriptor.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer query_read_buffer = device_.CreateBuffer(&bufferDescriptor);

    current_command_encoder_.CopyBufferToBuffer(
        query_resolve_buffer_,
        0,
        query_read_buffer,
        0,
        query_count * sizeof(uint64_t));

    pending_queries_.emplace_back(std::move(pending_kernels_), query_read_buffer);
    pending_kernels_.clear();
  }
  auto command_buffer = current_command_encoder_.Finish();
  device_queue_.Submit(1, &command_buffer);
  if (!ReconstructionDrainPolicyValue().enabled && graph_capture_state_ != GraphCaptureState::Replaying) {
    buffer_mgr.RefreshPendingBuffers(graph_capture_state_);
  }
  current_command_encoder_ = nullptr;
  num_pending_dispatches_ = 0;
  if (ReconstructionDrainPolicyValue().enabled) ORT_RETURN_IF_ERROR(ReconstructionDrainRejectReadyError());
  auto completion_status = PwBoundedAfterSubmit(instance_, device_, device_queue_, !status.IsOK());
  if (!completion_status.IsOK()) {
    return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Flush status: ", status.ErrorMessage(),
                           "; ", completion_status.ErrorMessage());
  }
  if (ReconstructionDrainPolicyValue().enabled) {
    ORT_RETURN_IF_ERROR(ReconstructionDrainRejectReadyError());
    if (!(reconstruction_workspace_phase_active_v2() && graph_capture_state_ == GraphCaptureState::Replaying && PwPhaseBoundaryEnabled()))
      ORT_RETURN_IF_ERROR(ReconstructionDrainFreshCompletion(instance_, device_, device_queue_));
    else ++pw_phase_deferred_flushes;
    if (status.IsOK() && graph_capture_state_ != GraphCaptureState::Replaying) buffer_mgr.RefreshPendingBuffers(graph_capture_state_);
    if (graph_capture_state_ != GraphCaptureState::Default) std::fprintf(stderr,"DE7_CAPTURE_FLUSH_SUCCESS state=%d %s\n",int(graph_capture_state_),ReconstructionDrainDrainJson().c_str());
  }
  return status;
  } catch (...) {
    PwFifoDrainOnException();
    throw;
  }
}

wgpu::BindGroup WebGpuContext::CreateBindGroup(const std::vector<WGPUBuffer>& bind_buffers,
                                               const std::vector<uint32_t>& bind_buffers_segments,
                                               const std::vector<uint64_t>& bind_logical_bytes,
                                               const wgpu::BindGroupLayout& bind_group_layout,
                                               std::string_view label) const {
  uint32_t entry_index = 0;
  std::vector<WGPUBindGroupEntry> bind_group_entries;

  const uint64_t kMaxBufferSize = device_limits_.maxStorageBufferBindingSize;
  for (size_t buffer_idx = 0; buffer_idx < bind_buffers.size(); ++buffer_idx) {
    WGPUBuffer buffer = bind_buffers[buffer_idx];
    const uint32_t total_segments = bind_buffers_segments[buffer_idx];
                                                                                                                  
                                                                                                                                   
                                                                                                                     
                                               
    if(bind_logical_bytes[buffer_idx]!=0) {
      const auto ranges=pw37_original80d1::LogicalBindingRanges(bind_logical_bytes[buffer_idx],wgpuBufferGetSize(buffer),kMaxBufferSize,device_limits_.minStorageBufferOffsetAlignment);
      ORT_ENFORCE(ranges.size()==total_segments,"Official pool logical segment mismatch");
      for(const auto&range:ranges)bind_group_entries.push_back({nullptr,entry_index++,buffer,range.offset,range.size,nullptr,nullptr});
    } else if (total_segments > 1) {
      uint64_t offset = 0;
      uint64_t buffer_size = wgpuBufferGetSize(buffer);
      for (uint32_t segment = 0; segment < total_segments; ++segment) {
        uint64_t segment_size = std::min(kMaxBufferSize, buffer_size - offset);
        bind_group_entries.push_back({nullptr, entry_index++, buffer, offset, segment_size, nullptr, nullptr});
        offset += segment_size;
      }
    } else {
      bind_group_entries.push_back({nullptr, entry_index++, buffer, 0, WGPU_WHOLE_SIZE, nullptr, nullptr});
    }
  }

  ORT_ENFORCE(entry_index <= device_limits_.maxBindingsPerBindGroup, "Number of bind group entries (", entry_index,
              ") exceeds device limit (", device_limits_.maxBindingsPerBindGroup, ").");

  WGPUBindGroupDescriptor bind_group_desc{};
  bind_group_desc.layout = bind_group_layout.Get();
  bind_group_desc.entryCount = bind_group_entries.size();
  bind_group_desc.entries = bind_group_entries.data();
  bind_group_desc.label = {label.data(), label.length()};

  WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(Device().Get(), &bind_group_desc);
  ORT_ENFORCE(bind_group != nullptr, "Failed to create bind group for program ", label, ".");
  return wgpu::BindGroup::Acquire(bind_group);
}

void WebGpuContext::DispatchCommand(const webgpu::CapturedCommandInfo& command) {
  ORT_ENFORCE(command.compute_pipeline.has_value());
  ORT_ENFORCE(command.bind_group != nullptr);
  const auto& compute_pass_encoder = GetComputePassEncoder();
  WriteTimestamp(num_pending_dispatches_ * 2);
  compute_pass_encoder.SetPipeline(*command.compute_pipeline);
  compute_pass_encoder.SetBindGroup(0, command.bind_group);

  if (command.indirect_buffer != nullptr) {
    compute_pass_encoder.DispatchWorkgroupsIndirect(command.indirect_buffer, 0);
  } else {
    compute_pass_encoder.DispatchWorkgroups(command.dispatch_group[0],
                                            command.dispatch_group[1],
                                            command.dispatch_group[2]);
  }
  WriteTimestamp(num_pending_dispatches_ * 2 + 1);
  ++num_pending_dispatches_;
  if (num_pending_dispatches_ >= max_num_pending_dispatches_ ||
      (is_profiling_ && query_type_ == TimestampQueryType::AtPasses)) {
    EndComputePass();
  }
}

void WebGpuContext::CaptureBegin(std::vector<webgpu::CapturedCommandInfo>* captured_commands, const webgpu::BufferManager& buffer_manager) {
  LOGS_DEFAULT(VERBOSE) << "CaptureBegin with external storage";
                                                           
  ORT_THROW_IF_ERROR(Flush(buffer_manager));

  external_captured_commands_ = captured_commands;
  { auto& r=PwGraphCopies(); std::lock_guard<std::mutex> lock(r.mutex); r.copies[captured_commands].clear(); }

                                                                     
  if (external_captured_commands_) {
    external_captured_commands_->clear();
  }

  graph_capture_state_ = GraphCaptureState::Capturing;
}

void WebGpuContext::RecordGraphBufferCopy(WGPUBuffer src, WGPUBuffer dst, size_t bytes) {
  if (graph_capture_state_ != GraphCaptureState::Capturing) return;
  ORT_ENFORCE(external_captured_commands_ != nullptr);
  wgpuBufferAddRef(src); wgpuBufferAddRef(dst);
  PwCapturedCopy copy{external_captured_commands_->size(), wgpu::Buffer::Acquire(src), wgpu::Buffer::Acquire(dst), bytes};
  auto& r=PwGraphCopies(); std::lock_guard<std::mutex> lock(r.mutex);
  auto& copies=r.copies[external_captured_commands_]; copies.push_back(std::move(copy));
  std::fprintf(stderr,"PW_GRAPH_COPY_RECORD index=%zu src=%p dst=%p bytes=%zu copies=%zu\n", copies.back().before_dispatch, (void*)src, (void*)dst, bytes, copies.size());
}

void WebGpuContext::Replay(const std::vector<webgpu::CapturedCommandInfo>& captured_commands, const webgpu::BufferManager& buffer_manager) {
  LOGS_DEFAULT(VERBOSE) << "Replay with external storage";
  graph_capture_state_ = GraphCaptureState::Replaying;
                                                          
  const size_t command_count = captured_commands.size();
  std::vector<PwCapturedCopy> copies;
  { auto& r=PwGraphCopies(); std::lock_guard<std::mutex> lock(r.mutex); auto it=r.copies.find(&captured_commands); if(it!=r.copies.end()) copies=it->second; }
  size_t next_copy=0;
  auto replay_copies_before = [&](size_t index) {
    while(next_copy<copies.size() && copies[next_copy].before_dispatch==index) {
      const auto& copy=copies[next_copy++];
      buffer_manager.MemCpy(copy.src.Get(), copy.dst.Get(), copy.bytes);
    }
  };
  std::fprintf(stderr,"PW_GRAPH_COPY_REPLAY_BEGIN dispatches=%zu copies=%zu\n",command_count,copies.size());
  for (size_t i = 0; i < command_count; ++i) {
    replay_copies_before(i);
    auto& command = captured_commands[i];

                                                                                  
                                                                              
                                    
    if (is_profiling_) {
      ORT_ENFORCE(command.pending_kernel_info.has_value(),
                  "WebGpuContext::Replay: profiling is enabled but captured command at index ",
                  i,
                  " is missing pending_kernel_info.");
      pending_kernels_.emplace_back(*command.pending_kernel_info);
    }

    DispatchCommand(command);
    if (num_pending_dispatches_ >= max_num_pending_dispatches_) {
      ORT_THROW_IF_ERROR(Flush(buffer_manager));
    }
  }

  replay_copies_before(command_count);
  ORT_ENFORCE(next_copy==copies.size(),"Captured copy boundary exceeds graph dispatch count");
  std::fprintf(stderr,"PW_GRAPH_COPY_REPLAY_END emitted=%zu dispatches=%zu\n",next_copy,command_count);
                                 
  ORT_THROW_IF_ERROR(Flush(buffer_manager));

  graph_capture_state_ = GraphCaptureState::Default;
}

extern "C" __attribute__((visibility("default"))) int reconstruction_graph_owners_v1(uint64_t*k,uint64_t*c)noexcept{try{if(!k||!c)return 1;auto&r=PwGraphCopies();std::lock_guard<std::mutex>g(r.mutex);*k=r.copies.size();*c=0;for(const auto&v:r.copies)*c+=v.second.size();return 0;}catch(...){return 1;}}
void WebGpuContext::CaptureEnd() {
  LOGS_DEFAULT(VERBOSE) << "CaptureEnd";

  graph_capture_state_ = GraphCaptureState::Default;
  external_captured_commands_ = nullptr;
}

void WebGpuContext::ReleaseGraphResources(std::vector<webgpu::CapturedCommandInfo>& captured_commands) {
  { auto& r=PwGraphCopies(); std::lock_guard<std::mutex> lock(r.mutex); r.copies.erase(&captured_commands); }
  LOGS_DEFAULT(VERBOSE) << "ReleaseGraphResources: Releasing " << captured_commands.size() << " captured command resources";

  for (auto& command : captured_commands) {
    command.bind_group = nullptr;
  }
}

std::mutex WebGpuContextFactory::mutex_;
std::once_flag WebGpuContextFactory::init_default_flag_;

std::unordered_map<int32_t, WebGpuContextFactory::WebGpuContextInfo>* WebGpuContextFactory::contexts_ = nullptr;
WGPUInstance WebGpuContextFactory::default_instance_ = nullptr;

WebGpuContext& WebGpuContextFactory::CreateContext(const WebGpuContextConfig& config) {
  const int context_id = config.context_id;
  WGPUInstance instance = config.instance;
  WGPUDevice device = config.device;

  std::call_once(init_default_flag_, [
#if !defined(__wasm__)
                                         dawn_proc_table = config.dawn_proc_table
#endif
  ]() {
                                                    

#if !defined(__wasm__)
    const DawnProcTable* dawn_procs = reinterpret_cast<const DawnProcTable*>(dawn_proc_table);
#if defined(BUILD_DAWN_SHARED_LIBRARY)
    ORT_ENFORCE(dawn_procs == nullptr, "setting DawnProcTable is not allowed when dynamically linked to webgpu_dawn.");
#else
#if !defined(USE_EXTERNAL_DAWN)
    if (dawn_procs == nullptr) {
      dawn_procs = &dawn::native::GetProcs();
    }
#else
    ORT_ENFORCE(dawn_procs != nullptr, "DawnProcTable must be provided.");
#endif
    dawnProcSetProcs(dawn_procs);
#endif
#endif
  });

  std::lock_guard<std::mutex> lock(mutex_);

  if (default_instance_ == nullptr) {
                            
    wgpu::InstanceFeatureName required_instance_features[] = {wgpu::InstanceFeatureName::TimedWaitAny};
    wgpu::InstanceDescriptor instance_desc{};
    instance_desc.requiredFeatures = required_instance_features;
    instance_desc.requiredFeatureCount = sizeof(required_instance_features) / sizeof(required_instance_features[0]);
    default_instance_ = wgpu::CreateInstance(&instance_desc).MoveToCHandle();

    ORT_ENFORCE(default_instance_ != nullptr, "Failed to create wgpu::Instance.");
  }

  if (context_id == 0) {
                                                                                                         
    ORT_ENFORCE(instance == nullptr && device == nullptr,
                "WebGPU EP default context (contextId=0) must not have custom WebGPU instance or device.");

    instance = default_instance_;
  } else {
                                                                               
    ORT_ENFORCE(instance != nullptr && device != nullptr,
                "WebGPU EP custom context (contextId>0) must have custom WebGPU instance and device.");
  }

                                                                                                    
  if (contexts_ == nullptr) {
    contexts_ = new std::unordered_map<int32_t, WebGpuContextInfo>();
  }

  auto it = contexts_->find(context_id);
  if (it == contexts_->end()) {
    GSL_SUPPRESS(r.11)
    auto context = std::unique_ptr<WebGpuContext>(new WebGpuContext(instance,
                                                                    device,
                                                                    config.validation_mode,
                                                                    config.validation_mode_explicitly_set,
                                                                    config.preserve_device,
                                                                    config.max_storage_buffer_binding_size));
    it = contexts_->emplace(context_id, WebGpuContextFactory::WebGpuContextInfo{std::move(context), 0}).first;
  } else if (context_id != 0) {
    ORT_ENFORCE(it->second.context->instance_.Get() == instance &&
                    it->second.context->device_.Get() == device,
                "WebGPU EP context ID ", context_id, " is already created with different WebGPU instance or device.");
  }
  it->second.ref_count++;

                                                                                          
                                                                                              
                                                
  ORT_TRY {
    it->second.context->Initialize(config);
  }
  ORT_CATCH(...) {
    if (--it->second.ref_count == 0) {
      contexts_->erase(it);
    }
    ORT_RETHROW;
  }

  return *it->second.context;
}

WebGpuContext& WebGpuContextFactory::GetContext(int context_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  ORT_ENFORCE(contexts_ != nullptr, "WebGPU contexts have not been initialized or have been cleaned up.");
  auto it = contexts_->find(context_id);
  ORT_ENFORCE(it != contexts_->end(), "WebGPU EP context ID ", context_id, " is not found.");

  return *it->second.context;
}

void WebGpuContextFactory::ReleaseContext(int context_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  ORT_ENFORCE(contexts_ != nullptr, "WebGPU contexts have not been initialized or have been cleaned up.");
  auto it = contexts_->find(context_id);
  ORT_ENFORCE(it != contexts_->end(), "WebGPU EP context ID ", context_id, " is not found.");

  if (--it->second.ref_count == 0 && !it->second.context->preserve_device_) {
    contexts_->erase(it);
  }
}

void WebGpuContextFactory::Cleanup() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (contexts_ != nullptr) {
    delete contexts_;
    contexts_ = nullptr;
  }

  if (default_instance_ != nullptr) {
    wgpuInstanceRelease(default_instance_);
    default_instance_ = nullptr;
  }
}

WebGpuContext& WebGpuContextFactory::DefaultContext() {
  WebGpuContextConfig config{};
  return WebGpuContextFactory::CreateContext(config);
}

void CleanupWebGpuContexts() {
  WebGpuContextFactory::Cleanup();
}

WGPUDevice GetDevice(int context_id) {
  return WebGpuContextFactory::GetContext(context_id).Device().Get();
}

}                     
}                          
