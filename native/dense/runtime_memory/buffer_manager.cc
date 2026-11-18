#include "official_object_pool.h"
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/webgpu/buffer_manager.h"
#include "core/providers/webgpu/webgpu_context.h"

#include "memory_runtime.inc"
#include "buffer_upload_policy.inc"
#include "buffer_upload_sidecar.inc"
#include "c2_repeat_policy.inc"
#include "c_upload_receipt.inc"
#include "c2_dawn_map_ops.inc"

namespace onnxruntime {
namespace webgpu {

namespace {

constexpr size_t NormalizeBufferSize(size_t size) {
  return (size + 15) / 16 * 16;
}

                                                                                          
constexpr size_t NormalizeCopySize(size_t size) {
  return (size + 3) / 4 * 4;
}

void EnforceBufferUnmapped(WebGpuContext& context, WGPUBuffer buffer) {
  if (context.ValidationMode() > ValidationMode::Basic) {
    ORT_ENFORCE(wgpuBufferGetMapState(buffer) == WGPUBufferMapState_Unmapped, "Buffer is still mapped.");
  }
}

}              

class DisabledCacheManager : public IBufferCacheManager {
  size_t CalculateBufferSize(size_t request_size) override {
    return NormalizeBufferSize(request_size);
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t                ) override {
                                 
    return nullptr;
  }
  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }
  void ReleaseBuffer(WGPUBuffer buffer) override {
    wgpuBufferRelease(buffer);
  }

  void OnRefresh(GraphCaptureState                        ) override {
            
  }
};

class LazyReleaseCacheManager : public IBufferCacheManager {
  size_t CalculateBufferSize(size_t request_size) override {
    return NormalizeBufferSize(request_size);
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t                ) override {
    return nullptr;
  }

  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }

  void ReleaseBuffer(WGPUBuffer buffer) override {
    pending_buffers_.emplace_back(buffer);
  }

  void OnRefresh(GraphCaptureState                        ) override {
    Release();
    pending_buffers_.clear();
  }

 public:
  ~LazyReleaseCacheManager() {
    Release();
  }

 protected:
  void Release() {
    for (auto& buffer : pending_buffers_) {
      wgpuBufferRelease(buffer);
    }
  }

  std::vector<WGPUBuffer> pending_buffers_;
};

class SimpleCacheManager : public IBufferCacheManager {
  size_t CalculateBufferSize(size_t request_size) override {
    return NormalizeBufferSize(request_size);
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t buffer_size) override {
    auto it = buffers_.find(buffer_size);
    if (it != buffers_.end() && !it->second.empty()) {
      auto buffer = it->second.back();
      it->second.pop_back();
      return buffer;
    }

    return nullptr;
  }

  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }

  void ReleaseBuffer(WGPUBuffer buffer) override {
    pending_buffers_.emplace_back(buffer);
  }

  void OnRefresh(GraphCaptureState                        ) override {
    for (auto& buffer : pending_buffers_) {
      buffers_[static_cast<size_t>(wgpuBufferGetSize(buffer))].emplace_back(buffer);
    }
    pending_buffers_.clear();
  }

 public:
  ~SimpleCacheManager() {
    for (auto& buffer : pending_buffers_) {
      wgpuBufferRelease(buffer);
    }
    for (auto& pair : buffers_) {
      for (auto& buffer : pair.second) {
        wgpuBufferRelease(buffer);
      }
    }
  }

 protected:
  std::map<size_t, std::vector<WGPUBuffer>> buffers_;
  std::vector<WGPUBuffer> pending_buffers_;
};

                                                                         
constexpr std::initializer_list<std::pair<const size_t, size_t>> BUCKET_DEFAULT_LIMIT_TABLE = {
    {64, 250},
    {128, 200},
    {256, 200},
    {512, 200},
    {2048, 230},
    {4096, 200},
    {8192, 50},
    {16384, 50},
    {32768, 50},
    {65536, 50},
    {131072, 50},
    {262144, 50},
    {524288, 50},
    {1048576, 50},
    {2097152, 30},
    {4194304, 20},
    {8388608, 10},
    {12582912, 10},
    {16777216, 10},
    {26214400, 15},
    {33554432, 22},
    {44236800, 2},
    {58982400, 6},
                                                                         
                                                                       
    {67108864, 6},
    {134217728, 6},
    {167772160, 6},
};

class BucketCacheManager : public IBufferCacheManager {
 public:
  BucketCacheManager() : buckets_limit_{BUCKET_DEFAULT_LIMIT_TABLE} {
    Initialize();
  }
  BucketCacheManager(std::unordered_map<size_t, size_t>&& buckets_limit) : buckets_limit_{buckets_limit} {
    Initialize();
  }

  size_t CalculateBufferSize(size_t request_size) override {
                        
    auto it = std::lower_bound(buckets_keys_.begin(), buckets_keys_.end(), request_size);
    if (it == buckets_keys_.end()) {
      return NormalizeBufferSize(request_size);
    } else {
      return *it;
    }
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t buffer_size) override {
    auto it = buckets_.find(buffer_size);
    if (it != buckets_.end() && !it->second.empty()) {
      auto buffer = it->second.back();
      it->second.pop_back();
      return buffer;
    }
    return nullptr;
  }

  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }

  void ReleaseBuffer(WGPUBuffer buffer) override {
    auto buffer_size = static_cast<size_t>(wgpuBufferGetSize(buffer));

    auto it = buckets_.find(buffer_size);
    if (it != buckets_.end() && it->second.size() < buckets_limit_[buffer_size]) {
      it->second.emplace_back(buffer);
    } else {
      pending_buffers_.emplace_back(buffer, buffer_size);
    }
  }

  void OnRefresh(GraphCaptureState                        ) override {
    for (auto& [buffer, buffer_size] : pending_buffers_) {
      auto it = buckets_.find(buffer_size);
      if (it != buckets_.end() && it->second.size() < buckets_limit_[buffer_size]) {
        it->second.emplace_back(buffer);
      } else {
        wgpuBufferRelease(buffer);
      }
    }
    pending_buffers_.clear();
  }

  ~BucketCacheManager() {
    for (auto& pair : buckets_) {
      for (auto& buffer : pair.second) {
        wgpuBufferRelease(buffer);
      }
    }
    for (auto& buffer_info : pending_buffers_) {
      wgpuBufferRelease(buffer_info.first);
    }
  }

 protected:
  void Initialize() {
    buckets_keys_.reserve(buckets_limit_.size());
    buckets_.reserve(buckets_limit_.size());
    for (const auto& pair : buckets_limit_) {
      buckets_keys_.push_back(pair.first);
      buckets_.emplace(pair.first, std::vector<WGPUBuffer>());
    }
    std::sort(buckets_keys_.begin(), buckets_keys_.end());

#ifndef NDEBUG                   
    ORT_ENFORCE(std::all_of(buckets_keys_.begin(), buckets_keys_.end(), [](size_t size) { return size % 16 == 0; }),
                "Bucket sizes must be multiples of 16.");

    for (size_t i = 1; i < buckets_keys_.size(); ++i) {
      ORT_ENFORCE(buckets_keys_[i] > buckets_keys_[i - 1], "Bucket sizes must be in increasing order.");
    }
#endif
  }
  std::unordered_map<size_t, size_t> buckets_limit_;
  std::unordered_map<size_t, std::vector<WGPUBuffer>> buckets_;
  std::vector<std::pair<WGPUBuffer, size_t>> pending_buffers_;
  std::vector<size_t> buckets_keys_;
};

class GraphCacheManager : public IBufferCacheManager {
 public:
  GraphCacheManager() : buckets_limit_{BUCKET_DEFAULT_LIMIT_TABLE} {
    Initialize();
  }
  GraphCacheManager(std::unordered_map<size_t, size_t>&& buckets_limit) : buckets_limit_{buckets_limit} {
    Initialize();
  }

  size_t CalculateBufferSize(size_t request_size) override {
                        
    auto it = std::lower_bound(buckets_keys_.begin(), buckets_keys_.end(), request_size);
    if (it == buckets_keys_.end()) {
      return NormalizeBufferSize(request_size);
    } else {
      return *it;
    }
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t buffer_size) override {
    auto it = buckets_.find(buffer_size);
    if (it != buckets_.end() && !it->second.empty()) {
      auto buffer = it->second.back();
      it->second.pop_back();
      return buffer;
    }
    return nullptr;
  }

  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }

  void ReleaseBuffer(WGPUBuffer buffer) override {
    auto buffer_size = static_cast<size_t>(wgpuBufferGetSize(buffer));
    auto it = buckets_.find(buffer_size);
    if (it != buckets_.end()) {
      it->second.emplace_back(buffer);
    } else {
                                                          
      buckets_[buffer_size] = std::vector<WGPUBuffer>{buffer};
    }
  }

  void OnRefresh(GraphCaptureState                        ) override {
                                              
  }

  std::vector<std::pair<size_t, WGPUBuffer>> ExtractCachedBuffers() override {
    std::vector<std::pair<size_t, WGPUBuffer>> result;
    for (auto& pair : buckets_) {
      for (auto& buffer : pair.second) {
        result.emplace_back(pair.first, buffer);
      }
      pair.second.clear();
    }
    return result;
  }

  void AbsorbCachedBuffers(std::vector<std::pair<size_t, WGPUBuffer>>&& buffers) override {
    for (auto& entry : buffers) {
      if (entry.second) {
        ReleaseBuffer(entry.second);
      }
    }
  }

  ~GraphCacheManager() {
    for (auto& pair : buckets_) {
      for (auto& buffer : pair.second) {
        wgpuBufferRelease(buffer);
      }
    }
  }

 protected:
  void Initialize() {
    buckets_keys_.reserve(buckets_limit_.size());
    buckets_.reserve(buckets_limit_.size());
    for (const auto& pair : buckets_limit_) {
      buckets_keys_.push_back(pair.first);
      buckets_.emplace(pair.first, std::vector<WGPUBuffer>());
    }
    std::sort(buckets_keys_.begin(), buckets_keys_.end());

#ifndef NDEBUG                   
    ORT_ENFORCE(std::all_of(buckets_keys_.begin(), buckets_keys_.end(), [](size_t size) { return size % 16 == 0; }),
                "Bucket sizes must be multiples of 16.");

    for (size_t i = 1; i < buckets_keys_.size(); ++i) {
      ORT_ENFORCE(buckets_keys_[i] > buckets_keys_[i - 1], "Bucket sizes must be in increasing order.");
    }
#endif
  }
  std::unordered_map<size_t, size_t> buckets_limit_;
  std::unordered_map<size_t, std::vector<WGPUBuffer>> buckets_;
  std::vector<size_t> buckets_keys_;
};

class GraphSimpleCacheManager : public IBufferCacheManager {
  size_t CalculateBufferSize(size_t request_size) override {
    return NormalizeBufferSize(request_size);
  }

  WGPUBuffer TryAcquireCachedBuffer(size_t buffer_size) override {
    auto it = buffers_.find(buffer_size);
    if (it != buffers_.end() && !it->second.empty()) {
      auto buffer = it->second.back();
      it->second.pop_back();
      return buffer;
    }

    return nullptr;
  }

  void RegisterBuffer(WGPUBuffer           , size_t                 ) override {
            
  }

  void ReleaseBuffer(WGPUBuffer buffer) override {
    pending_buffers_.emplace_back(buffer);
  }

  void OnRefresh(GraphCaptureState graph_capture_state) override {
    for (auto& buffer : pending_buffers_) {
      if (graph_capture_state == GraphCaptureState::Default) {
        buffers_[static_cast<size_t>(wgpuBufferGetSize(buffer))].emplace_back(buffer);
      } else {
        captured_buffers_.emplace_back(buffer);
      }
    }
    pending_buffers_.clear();
  }

 public:
  ~GraphSimpleCacheManager() {
    for (auto& buffer : pending_buffers_) {
      wgpuBufferRelease(buffer);
    }
    for (auto& pair : buffers_) {
      for (auto& buffer : pair.second) {
        wgpuBufferRelease(buffer);
      }
    }
    for (auto& buffer : captured_buffers_) {
      wgpuBufferRelease(buffer);
    }
  }

  std::vector<std::pair<size_t, WGPUBuffer>> ExtractCachedBuffers() override {
                                                                              
                                                                                
                                          
    std::vector<std::pair<size_t, WGPUBuffer>> result;
    for (auto& pair : buffers_) {
      for (auto& buffer : pair.second) {
        result.emplace_back(pair.first, buffer);
      }
      pair.second.clear();
    }
    buffers_.clear();
    for (auto& buffer : pending_buffers_) {
      result.emplace_back(static_cast<size_t>(wgpuBufferGetSize(buffer)), buffer);
    }
    pending_buffers_.clear();
    for (auto& buffer : captured_buffers_) {
      result.emplace_back(static_cast<size_t>(wgpuBufferGetSize(buffer)), buffer);
    }
    captured_buffers_.clear();
    return result;
  }

  void AbsorbCachedBuffers(std::vector<std::pair<size_t, WGPUBuffer>>&& buffers) override {
    for (auto& entry : buffers) {
      if (entry.second) {
        buffers_[entry.first].emplace_back(entry.second);
      }
    }
  }

 protected:
  std::map<size_t, std::vector<WGPUBuffer>> buffers_;
  std::vector<WGPUBuffer> pending_buffers_;
  std::vector<WGPUBuffer> captured_buffers_;
};

std::unique_ptr<IBufferCacheManager> CreateBufferCacheManager(BufferCacheMode cache_mode) {
  switch (cache_mode) {
    case BufferCacheMode::Disabled:
      return std::make_unique<DisabledCacheManager>();
    case BufferCacheMode::LazyRelease:
      return std::make_unique<LazyReleaseCacheManager>();
    case BufferCacheMode::Simple:
      return std::make_unique<SimpleCacheManager>();
    case BufferCacheMode::Bucket:
      return std::make_unique<BucketCacheManager>();
    case BufferCacheMode::Graph:
      return std::make_unique<GraphCacheManager>();
    case BufferCacheMode::GraphSimple:
      return std::make_unique<GraphSimpleCacheManager>();
    default:
      ORT_NOT_IMPLEMENTED("Unsupported buffer cache mode");
  }
}

std::ostream& operator<<(std::ostream& os, BufferCacheMode mode) {
  switch (mode) {
    case BufferCacheMode::Disabled:
      os << "Disabled";
      break;
    case BufferCacheMode::LazyRelease:
      os << "LazyRelease";
      break;
    case BufferCacheMode::Simple:
      os << "Simple";
      break;
    case BufferCacheMode::Bucket:
      os << "Bucket";
      break;
    case BufferCacheMode::Graph:
      os << "Graph";
      break;
    case BufferCacheMode::GraphSimple:
      os << "GraphSimple";
      break;
    default:
      os << "Unknown(" << static_cast<int>(mode) << ")";
  }
  return os;
}

BufferManager::BufferManager(WebGpuContext& context, BufferCacheMode storage_buffer_cache_mode, BufferCacheMode uniform_buffer_cache_mode, BufferCacheMode query_resolve_buffer_cache_mode, BufferCacheMode default_buffer_cache_mode)
    : context_{context},
      storage_cache_{CreateBufferCacheManager(storage_buffer_cache_mode)},
      uniform_cache_{CreateBufferCacheManager(uniform_buffer_cache_mode)},
      query_resolve_cache_{CreateBufferCacheManager(query_resolve_buffer_cache_mode)},
      default_cache_{CreateBufferCacheManager(default_buffer_cache_mode)} {
}

void BufferManager::Upload(void* src, WGPUBuffer dst, size_t size) const {
                                                          
  ORT_ENFORCE(size <= wgpuBufferGetSize(dst), "Upload destination too small");
  void* mapped_data = wgpuBufferGetMapState(dst) == WGPUBufferMapState_Mapped ? wgpuBufferGetMappedRange(dst, 0, WGPU_WHOLE_MAP_SIZE) : nullptr;
  if (mapped_data) {
    memcpy(mapped_data, src, size);
    wgpuBufferUnmap(dst);
    c_upload_receipt::Write(dst,size,true);
    return;
  }

  const char* c2_flag=std::getenv("RECONSTRUCTION_BENCH_UMA_INPUT_REWRITE_MAP");
  ORT_ENFORCE(!c2_flag || std::strcmp(c2_flag,"0")==0 || std::strcmp(c2_flag,"1")==0,"C2 opt-in must be0/1");
  const bool c2_enabled=c2_flag&&std::strcmp(c2_flag,"1")==0;
  if(c2_enabled){
  const auto c2_token=pw_c2::registry.Begin(reinterpret_cast<uintptr_t>(dst),reinterpret_cast<uintptr_t>(context_.Device().Get()),size,c2_enabled,
      SupportsUMA()&&wgpuDeviceHasFeature(context_.Device().Get(),WGPUFeatureName_BufferMapExtendedUsages),
      context_.AdapterInfo().adapterType==wgpu::AdapterType::IntegratedGPU,
      (wgpuBufferGetUsage(dst)&WGPUBufferUsage_MapWrite)!=0,
      wgpuBufferGetMapState(dst)==WGPUBufferMapState_Unmapped);
  if(c2_token.buffer){
    pw_c2::DawnMapOps ops(context_.instance_,dst,src,size,[&]{ORT_THROW_IF_ERROR(context_.Flush(*this));});
    if(pw_c2::Execute(pw_c2::registry,c2_token,ops)){c_upload_receipt::Write(dst,size,true);return;}
  }

  }

                                                               
  auto copy_size = NormalizeCopySize(size);

  wgpu::BufferDescriptor desc{};
  desc.size = copy_size;
  desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
  desc.mappedAtCreation = true;

  auto staging_buffer = S1BudgetCreate(context_.Device(),desc);
  mapped_data = staging_buffer.GetMappedRange();
  ORT_ENFORCE(mapped_data,"Upload staging map failed");
  memcpy(mapped_data, src, size);
                                                                                              
                                                                                           
                                                                                    
                                                                                           
                                               
  staging_buffer.Unmap();

  ORT_THROW_IF_ERROR(context_.EncodeDeferredDispatches());
  auto& command_encoder = context_.GetCommandEncoder();
  context_.EndComputePass();
  command_encoder.CopyBufferToBuffer(staging_buffer, 0, dst, 0, copy_size);
  ORT_THROW_IF_ERROR(context_.Flush(*this));
  c_upload_receipt::Write(dst,size,false);
}

void BufferManager::MemCpy(WGPUBuffer src, WGPUBuffer dst, size_t size) const {
  ORT_ENFORCE(src != dst, "Source and destination buffers must be different.");
  EnforceBufferUnmapped(context_, src);
  EnforceBufferUnmapped(context_, dst);

  auto copy_size = NormalizeCopySize(size);
  auto src_size = static_cast<size_t>(wgpuBufferGetSize(src));
  auto dst_size = static_cast<size_t>(wgpuBufferGetSize(dst));
  ORT_ENFORCE(copy_size <= src_size && copy_size <= dst_size,
              "Source and destination buffers must have enough space for the copy operation. src_size=",
              src_size, ", dst_size=", dst_size, ", copy_size=", copy_size, ".");

  ORT_THROW_IF_ERROR(context_.EncodeDeferredDispatches());
  auto& command_encoder = context_.GetCommandEncoder();
  context_.EndComputePass();
  command_encoder.CopyBufferToBuffer(src, 0, dst, 0, copy_size);
  context_.RecordGraphBufferCopy(src, dst, copy_size);
}

WGPUBuffer BufferManager::Create(size_t size, wgpu::BufferUsage usage) const {
#include "official_buffer_create.inc"
  const auto input_usage=wgpu::BufferUsage::Storage|wgpu::BufferUsage::CopySrc|wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::Indirect;
  const auto scoped_request=pw_upload::request;
  const bool cached_feature=SupportsUMA();
  const bool actual_feature=scoped_request.armed ? wgpuDeviceHasFeature(context_.Device().Get(),WGPUFeatureName_BufferMapExtendedUsages) : false;
  const bool mapped_input=pw_upload::Consume(size,cached_feature&&actual_feature,context_.AdapterInfo().adapterType==wgpu::AdapterType::IntegratedGPU,usage==input_usage);
  c_upload_receipt::Attempt receipt_attempt{scoped_request.armed ? c_upload_receipt::Begin(scoped_request.bytes,size,actual_feature,cached_feature,uint32_t(context_.AdapterInfo().adapterType),usage==input_usage,mapped_input,context_.Device().Get()) : 0};
  if(mapped_input)usage|=wgpu::BufferUsage::MapWrite;
  auto& cache = GetCacheManager(usage);
  auto buffer_size = cache.CalculateBufferSize(size);

  auto buffer = mapped_input ? nullptr : cache.TryAcquireCachedBuffer(buffer_size);
  if (buffer) {
    S1MemoryEvent(context_.Device().Get(),0,3);
    receipt_attempt.CompleteBuffer(buffer);
    return buffer;
  }

                                    
  wgpu::BufferDescriptor desc{};
  desc.size = buffer_size;
  desc.usage = usage;
  if (usage & wgpu::BufferUsage::MapWrite) {
    desc.mappedAtCreation = true;                                                        
  }
  if(mapped_input) {
                                                                                        
    auto owned=S1BudgetCreate(context_.Device(),desc);
    ORT_ENFORCE(owned,"Mapped input creation failed");
    pw_upload::Mark(owned.Get());
    pw_c2::registry.RegisterIfEligible(reinterpret_cast<uintptr_t>(owned.Get()),reinterpret_cast<uintptr_t>(context_.Device().Get()),size,pw_c2::Enabled(),cached_feature&&actual_feature,context_.AdapterInfo().adapterType==wgpu::AdapterType::IntegratedGPU);
    buffer=owned.MoveToCHandle();
  } else buffer = S1BudgetCreate(context_.Device(),desc).MoveToCHandle();

  ORT_ENFORCE(buffer, "Failed to create GPU buffer: size=", buffer_size, ", usage=", uint64_t(usage), ".");

  if(!mapped_input)cache.RegisterBuffer(buffer, size);
  receipt_attempt.CompleteBuffer(buffer);
  return buffer;
}

bool BufferManager::SupportsUMA() const {
#if !defined(__wasm__)
                                                                     
  return context_.DeviceHasFeature(wgpu::FeatureName::BufferMapExtendedUsages);
#else
  return false;
#endif                       
}

void BufferManager::Release(WGPUBuffer buffer) const {
  EnforceBufferUnmapped(context_, buffer);
  c_upload_receipt::Release(buffer);
  pw_c2::registry.Retire(reinterpret_cast<uintptr_t>(buffer));
  if(reconstruction_official_object::pool().client_release(uintptr_t(buffer)))wgpuBufferRelease(buffer);
  else if(pw_upload::Take(buffer))wgpuBufferRelease(buffer);
  else GetCacheManager(buffer).ReleaseBuffer(buffer);
  S1MemoryEvent(context_.Device().Get(),0,4);
}

void BufferManager::Download(WGPUBuffer src, void* dst, size_t size) const {
                                                                                                   
              
  ORT_THROW_IF_ERROR(context_.EncodeDeferredDispatches());

  EnforceBufferUnmapped(context_, src);
  auto buffer_size = NormalizeBufferSize(size);

  wgpu::BufferDescriptor desc{};
  desc.size = buffer_size;
  desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

  auto staging_buffer = S1BudgetCreate(context_.Device(),desc);
  auto& command_encoder = context_.GetCommandEncoder();
  context_.EndComputePass();
  command_encoder.CopyBufferToBuffer(src, 0, staging_buffer, 0, buffer_size);
  ORT_THROW_IF_ERROR(context_.Flush(*this));

                                       

  struct MapAsyncResult {
    wgpu::MapAsyncStatus status{};
    std::string message{};
  } map_async_result;

  ORT_THROW_IF_ERROR(context_.Wait(
      staging_buffer.MapAsync(
          wgpu::MapMode::Read, 0, buffer_size, wgpu::CallbackMode::WaitAnyOnly,
                                                    
          [](wgpu::MapAsyncStatus status, wgpu::StringView message,
             MapAsyncResult* result) noexcept {
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

  auto mapped_data = staging_buffer.GetConstMappedRange();
  memcpy(dst, mapped_data, size);
  staging_buffer.Unmap();
}

void BufferManager::RefreshPendingBuffers(GraphCaptureState graph_capture_state) const {
  storage_cache_->OnRefresh(graph_capture_state);
  uniform_cache_->OnRefresh(graph_capture_state);
  query_resolve_cache_->OnRefresh(graph_capture_state);
  default_cache_->OnRefresh(graph_capture_state);
  S1MemoryEvent(context_.Device().Get(),0,5);
}

IBufferCacheManager& BufferManager::GetCacheManager(wgpu::BufferUsage usage) const {
  if (usage & wgpu::BufferUsage::Storage) {
    return *storage_cache_;
  } else if (usage & wgpu::BufferUsage::Uniform) {
    return *uniform_cache_;
  } else if (usage & wgpu::BufferUsage::QueryResolve) {
    return *query_resolve_cache_;
  } else {
    return *default_cache_;
  }
}

IBufferCacheManager& BufferManager::GetCacheManager(WGPUBuffer buffer) const {
  auto usage = static_cast<wgpu::BufferUsage>(wgpuBufferGetUsage(buffer));
  return GetCacheManager(usage);
}

std::unique_ptr<BufferManager> BufferManagerFactory::Create(WebGpuContext& context, BufferCacheMode storage_buffer_cache_mode, BufferCacheMode uniform_buffer_cache_mode, BufferCacheMode query_resolve_buffer_cache_mode, BufferCacheMode default_buffer_cache_mode) {
  return std::make_unique<BufferManager>(context, storage_buffer_cache_mode, uniform_buffer_cache_mode, query_resolve_buffer_cache_mode, default_buffer_cache_mode);
}

}                     
}                          
