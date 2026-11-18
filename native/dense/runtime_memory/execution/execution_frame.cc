#include "value_allocation_scope.h"
#include "official_object_profile.h"
#include <set>
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/framework/execution_frame.h"

#include <sstream>

#include "core/framework/mem_pattern_planner.h"
#include "core/framework/execution_plan_base.h"
#include "core/framework/sequential_execution_plan.h"
#include "core/framework/ort_value_pattern_planner.h"
#include "core/framework/tensorprotoutils.h"
#include "core/framework/mldata_type_utils.h"
#include "core/framework/sparse_utils.h"
#include "core/framework/node_index_info.h"
#include "core/framework/op_kernel.h"
#include "core/framework/session_state.h"
#include "core/framework/TensorSeq.h"
#include "core/framework/utils.h"
#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
#include "core/framework/memory_info.h"
#endif

#include "core/framework/bfc_arena.h"

extern "C" void reconstruction_official_object_frame_register_profile(uintptr_t,const size_t*,size_t,size_t);
using namespace onnxruntime::common;

namespace onnxruntime {
                                                                                 
static void PwOfficialPrepareFrame(const SessionState& ss, uintptr_t frame) {
 using namespace reconstruction_official_object;
 if(!Enabled()||reconstruction_official_object_frame_known(frame))return;
 std::vector<size_t> ids;const auto&names=ss.GetOrtValueNameIdxMap();const size_t count=size_t(names.MaxIdx()+1);
 size_t profile_id=0;std::span<const AllocationIdentity> frozen;
 auto image=ss.GetGraphViewer().GetNodeArg("/Gather_output_0");bool low=false;
 if(count==62){ORT_ENFORCE(image&&image->Shape()&&image->Shape()->dim_size()==4,"FNET actualimageshape");const auto&w=image->Shape()->dim(3);ORT_ENFORCE(w.has_dim_value()&&(w.dim_value()==768||w.dim_value()==1536),"FNET unsupportedactualwidth");low=w.dim_value()==768;profile_id=low?3:1;frozen=low?std::span<const AllocationIdentity>(identity_LOW_FNET):std::span<const AllocationIdentity>(identity_HIGH_FNET);}
 else if(count==65){ORT_ENFORCE(image&&image->Shape()&&image->Shape()->dim_size()==4,"mixedFNET actualimageshape");const auto&w=image->Shape()->dim(3);ORT_ENFORCE(w.has_dim_value()&&(w.dim_value()==768||w.dim_value()==1536),"mixedFNET unsupportedwidth");low=w.dim_value()==768;profile_id=low?7:6;frozen=low?std::span<const AllocationIdentity>(identity_FNET_MIXED_LOW):std::span<const AllocationIdentity>(identity_FNET_MIXED_HIGH);} else if(count==6041){profile_id=2;frozen=identity_LOW_REST;}
 else if(count==5487){profile_id=0;frozen=identity_HIGH_REST;}
 else if(count==3555){profile_id=4;frozen=identity_high_fused_update;}
 else if(count==4119){profile_id=5;frozen=identity_low_fused_update;}
 else {for(const auto& n:ss.GetGraphViewer().Nodes())ORT_ENFORCE(n.Domain()!="reconstruction.fused_update"||n.OpType()!="ReconstructionFusedUpdateBlock","Fused update model lacks the required allocation profile");reconstruction_official_object_frame_register(frame,nullptr,0);return;}
 const auto*actual_plan=ss.GetExecutionPlan();ORT_ENFORCE(actual_plan&&actual_plan->allocation_plan.size()==frozen.size(),"stage actual planarity");                                                                             
                                                                              
                                                                        
 bool identities_ok=true;size_t constant_representatives=0;
 const auto& frozen_pool=GetProfile(profile_id);
 for(const auto&i:frozen){int actual=-1;const auto&a=actual_plan->allocation_plan.at(i.id);
  bool kind_alias=size_t(a.alloc_kind)==i.kind&&size_t(a.reused_buffer)==i.reuse;
  bool exact_name=names.GetIdx(i.name,actual).IsOK()&&actual==int(i.id);
  bool pooled=false;for(const auto&v:frozen_pool.values)if(v.value==i.id){pooled=true;break;}
  bool static_constant=!pooled&&i.kind==size_t(AllocKind::kAllocateStatically)&&a.alloc_kind==AllocKind::kAllocateStatically&&ss.GetInitializedTensors().count(int(i.id))==1;
  if(!kind_alias||(!exact_name&&!static_constant)){std::string observed;(void)names.GetName(int(i.id),observed);std::fprintf(stderr,"IOS_ACTUAL_PROFILE_REJECT profile=%zu id=%zu expected=%s observed=%s expected_kind=%zu actual_kind=%zu expected_reuse=%zu actual_reuse=%zu pooled=%d initialized=%d\n",profile_id,size_t(i.id),i.name,observed.c_str(),size_t(i.kind),size_t(a.alloc_kind),size_t(i.reuse),size_t(a.reused_buffer),int(pooled),int(static_constant));identities_ok=false;}
  else if(!exact_name){
   if(constant_representatives<16){std::string observed;(void)names.GetName(int(i.id),observed);std::fprintf(stderr,"IOS_STATIC_INITIALIZER_REPRESENTATIVE profile=%zu id=%zu expected=%s observed=%s expected_lookup_id=%d kind=%zu reuse=%zu pooled=0 initialized=1\n",profile_id,size_t(i.id),i.name,observed.c_str(),actual,size_t(a.alloc_kind),size_t(a.reused_buffer));}
   ++constant_representatives;
  }
 }
 std::fprintf(stderr,"IOS_PROFILE_BINDING profile=%zu values=%zu constant_representative_names=%zu exact_kind_alias=%d\n",profile_id,frozen.size(),constant_representatives,int(identities_ok));
 ORT_ENFORCE(identities_ok,"iOS actual pooled names/index and full allocation kind/alias mismatch");

 const auto&profile=GetProfile(profile_id);
 for(const auto&v:profile.values){int actual=-1;ORT_ENFORCE(names.GetIdx(v.name,actual).IsOK()&&actual==int(v.value),"Official graph profile exact actual mapping differs");ids.push_back(v.value);}
 std::set<size_t> host;
 for(const auto& n:ss.GetGraphViewer().Nodes()) {
  const auto* kernel=ss.GetKernel(n.Index());ORT_ENFORCE(kernel,"Official pool missing actual kernel");
  const auto& kd=kernel->KernelDef();
  for(size_t i=0;i<n.InputDefs().size();++i)if(n.InputDefs()[i]&&n.InputDefs()[i]->Exists()&&kd.IsInputOnCpu(i)){int id=-1;ORT_THROW_IF_ERROR(names.GetIdx(n.InputDefs()[i]->Name(),id));host.insert(size_t(id));}
  for(size_t i=0;i<n.OutputDefs().size();++i)if(n.OutputDefs()[i]&&n.OutputDefs()[i]->Exists()&&kd.IsOutputOnCpu(i)){int id=-1;ORT_THROW_IF_ERROR(names.GetIdx(n.OutputDefs()[i]->Name(),id));host.insert(size_t(id));}
 }

 ids.clear();for(const auto&v:profile.values)if(!host.count(v.value))ids.push_back(v.value);
 reconstruction_official_object_frame_register_profile(frame,ids.data(),ids.size(),profile_id);
}

IExecutionFrame::IExecutionFrame(const OrtValueNameIdxMap& ort_value_idx_map,
                                 const NodeIndexInfo& node_index_info,
                                 gsl::span<const int> fetch_mlvalue_idxs)
    : node_index_info_(node_index_info),
      all_values_size_(static_cast<size_t>(ort_value_idx_map.MaxIdx()) + 1),
      fetch_mlvalue_idxs_(fetch_mlvalue_idxs.begin(), fetch_mlvalue_idxs.end()),
      ort_value_idx_map_(ort_value_idx_map) {
  ORT_ENFORCE(node_index_info_.GetMaxMLValueIdx() == ort_value_idx_map.MaxIdx(),
              "node_index_info and ort_value_idx_map are out of sync and cannot be used");
}

IExecutionFrame::~IExecutionFrame() = default;

#ifdef ENABLE_ATEN
Status IExecutionFrame::SetOutputMLValue(int index, const OrtValue& ort_value) {
  int ort_value_idx = GetNodeIdxToMLValueIdx(index);
  if (ort_value_idx == NodeIndexInfo::kInvalidEntry || static_cast<size_t>(ort_value_idx) >= all_values_size_) {
    return ORT_MAKE_STATUS(ONNXRUNTIME, INVALID_ARGUMENT, "invalid index ", ort_value_idx);
  }

  if (all_values_[ort_value_idx].IsAllocated()) {
    ORT_RETURN_IF_ERROR(CopyTensor(ort_value.Get<Tensor>(), *all_values_[ort_value_idx].GetMutable<Tensor>()));
  } else {
    all_values_[ort_value_idx] = ort_value;
  }
  return Status::OK();
}
#endif

#ifdef ENABLE_TRAINING
void IExecutionFrame::UpdateFeeds(gsl::span<const int> feed_mlvalue_idxs, gsl::span<const OrtValue> feeds) {
  ORT_ENFORCE(feed_mlvalue_idxs.size() == feeds.size());

  for (size_t idx = 0, end = feed_mlvalue_idxs.size(); idx < end; ++idx) {
    int ort_value_idx = feed_mlvalue_idxs[idx];
                                                               

    ORT_ENFORCE(!all_values_[ort_value_idx].IsAllocated());

    all_values_[ort_value_idx] = feeds[idx];
  }
}

void IExecutionFrame::UpdateFetches(gsl::span<const int> fetch_mlvalue_idxs,
                                    gsl::span<const OrtValue> fetches, const std::unordered_map<int, OrtValue>& initializers) {
  ORT_ENFORCE(fetch_mlvalue_idxs.size() == fetches.size());

  if (!fetches.empty()) {
    fetch_mlvalue_idxs_.assign(fetch_mlvalue_idxs.begin(), fetch_mlvalue_idxs.end());

    auto num_fetches = fetch_mlvalue_idxs_.size();

    for (size_t idx = 0; idx < num_fetches; ++idx) {
      int ort_value_idx = fetch_mlvalue_idxs_[idx];

      ORT_ENFORCE(!all_values_[ort_value_idx].IsAllocated());

      all_values_[ort_value_idx] = fetches[idx];

                                                     
      auto entry = initializers.find(ort_value_idx);
      if (entry != initializers.end()) {
        const Tensor& src = entry->second.Get<Tensor>();
        OrtValue& dest = all_values_[ort_value_idx];

        if (!dest.IsAllocated()) {
          AllocatorPtr allocator = GetAllocator(src.Location().device);
          auto p_tensor = std::make_unique<Tensor>(src.DataType(), src.Shape(), allocator);
          auto ml_tensor = DataTypeImpl::GetType<Tensor>();
          dest.Init(p_tensor.release(), ml_tensor, ml_tensor->GetDeleteFunc());
        }

        ORT_THROW_IF_ERROR(CopyTensor(src, *dest.GetMutable<Tensor>()));
      }
    }
  }
}

Status IExecutionFrame::GetOutputs(gsl::span<const int> fetch_mlvalue_idxs, std::vector<OrtValue>& fetches) {
  auto num_fetches = fetch_mlvalue_idxs.size();

  if (fetches.empty()) {
    fetches.resize(num_fetches);
  } else {
                                                           
    if (fetches.size() != num_fetches) {
      return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Fetches vector passed to GetOutputs contains ", fetches.size(),
                             " entries which doesn't match the number of fetches the frame was initialized with of ",
                             num_fetches);
    }
  }

  for (size_t idx = 0; idx < num_fetches; ++idx) {
    fetches[idx] = GetMLValue(fetch_mlvalue_idxs[idx]);
  }

  return Status::OK();
}

#endif

                                                                                 
const OrtValue* IExecutionFrame::GetNodeInputOrOutputMLValue(int index) const {
  int ort_value_idx = GetNodeIdxToMLValueIdx(index);
  return ort_value_idx != NodeIndexInfo::kInvalidEntry ? &(all_values_[ort_value_idx]) : nullptr;
}

OrtValue* IExecutionFrame::GetMutableNodeInputOrOutputMLValue(int index) {
  return const_cast<OrtValue*>(GetNodeInputOrOutputMLValue(index));
}

                             
                                  
                                                                                          

Status IExecutionFrame::GetOrCreateNodeOutputMLValue(const int output_index, int output_arg_index,
                                                     const TensorShape* shape, OrtValue*& p_ort_value,
                                                     const Node& node) {
  auto status = Status::OK();
  int ort_value_idx = GetNodeIdxToMLValueIdx(output_arg_index);

                                     
  if (ort_value_idx == NodeIndexInfo::kInvalidEntry) {
    p_ort_value = nullptr;
  } else {
    p_ort_value = &all_values_[ort_value_idx];

    if (p_ort_value->IsAllocated()) {
                                                                                               
                                                                                           
                                                                                       
                                                                                           
                                                                                         
                       
        
                                                                                         
                               
        
                                                                                           
                                                                                         
                                                                                        
                                                                                   
                                                                                
        
                                                                                   
                                                                                      
                                                                                      
                                                         
      bool shape_matched = true;

      if (p_ort_value->IsTensor()) {
        ORT_RETURN_IF_NOT(shape != nullptr, "shape must not be null for tensor output that is already allocated");
        const Tensor& tensor = p_ort_value->Get<Tensor>();
        shape_matched = (tensor.Shape() == *shape);
      } else if (p_ort_value->IsSparseTensor()) {
#if !defined(DISABLE_SPARSE_TENSORS)
        ORT_RETURN_IF_NOT(shape != nullptr, "shape must not be null for sparse tensor output that is already allocated");
        const SparseTensor& sp_tensor = p_ort_value->Get<SparseTensor>();
        shape_matched = (sp_tensor.DenseShape() == *shape);
#endif
      }

      if (!shape_matched) {
        const TensorShape& existing_shape = p_ort_value->IsTensor()
                                                ? p_ort_value->Get<Tensor>().Shape()
#if !defined(DISABLE_SPARSE_TENSORS)
                                                : p_ort_value->Get<SparseTensor>().DenseShape();
#else
                                                : *shape;                                        
#endif
        return ORT_MAKE_STATUS(ONNXRUNTIME, INVALID_ARGUMENT,
                               "The output OrtValue provided for output '",
                               node.OutputDefs()[output_index]->Name(),
                               "' of node '", node.Name(),
                               "' (", node.OpType(), ") has shape ", existing_shape,
                               " but the computed output shape for this run is ", *shape,
                               ". When calling Run() with pre-allocated output OrtValues on a model "
                               "with dynamic output shapes, either supply unallocated output OrtValues "
                               "or ensure the pre-allocated shapes match the expected output shapes "
                               "for each run.");
      }
    } else {
                                                          
      if (shape != nullptr && IsOutput(ort_value_idx)) {
        VerifyOutputSizes(output_index, node, *shape);
      }
      status = CreateNodeOutputMLValueImpl(*p_ort_value, ort_value_idx, shape);
    }
  }

  return status;
}

bool IExecutionFrame::TryGetInferredShape(int          , TensorShape&          ) const {
                                                                              
                                                                              
                                                                                        
  return false;
}

AllocatorPtr IExecutionFrame::GetAllocator(const OrtDevice& info) const {
  return GetAllocatorImpl(info);
}

Status IExecutionFrame::ReleaseMLValue(int ort_value_idx) { return ReleaseMLValueImpl(ort_value_idx); }

#ifdef ENABLE_TRAINING
void IExecutionFrame::ReleaseAllMLValues() {
  for (size_t ort_value_idx = 0; ort_value_idx < all_values_.size(); ort_value_idx++) {
    all_values_[ort_value_idx] = OrtValue();
  }
}
#endif

Status IExecutionFrame::ReleaseMLValueImpl(int ort_value_idx) {
  if (ort_value_idx == NodeIndexInfo::kInvalidEntry || static_cast<size_t>(ort_value_idx) >= all_values_size_) {
    return ORT_MAKE_STATUS(ONNXRUNTIME, INVALID_ARGUMENT, "invalid index ", ort_value_idx);
  }

  all_values_[ort_value_idx] = OrtValue();
  return Status::OK();
}

int IExecutionFrame::GetNodeIdxToMLValueIdx(int index) const {
                                                            
  int ort_value_idx = node_index_info_.GetMLValueIndex(index);
  return ort_value_idx;
}

void IExecutionFrame::Init(gsl::span<const int> feed_mlvalue_idxs, gsl::span<const OrtValue> feeds,
                           const std::unordered_map<int, OrtValue>& initializers,
                           const std::function<bool(const std::string& name)>& is_initializer_sparse_func,
                           gsl::span<const OrtValue> fetches) {
  ORT_ENFORCE(feeds.size() == feed_mlvalue_idxs.size(), "Get feed size: ", feeds.size(), " but expected feed size: ",
              feed_mlvalue_idxs.size());
  ORT_ENFORCE(fetches.empty() || fetches.size() == fetch_mlvalue_idxs_.size());

                                                    
  AllocatorPtr cpu_allocator = GetAllocator(OrtDevice());

                                    
  all_values_.resize(all_values_size_);

                                      
  if (!fetches.empty()) {
    auto num_fetches = fetch_mlvalue_idxs_.size();

    for (size_t idx = 0; idx < num_fetches; ++idx) {
      int ort_value_idx = fetch_mlvalue_idxs_[idx];
      all_values_[ort_value_idx] = fetches[idx];
    }
  }

                           
                                                                                           
                                                                                                                  
                                                                                 
                                                                                                                      
                                                                                                
                                                  
  for (const auto& entry : initializers) {
    int ort_value_index = entry.first;

                                                                                                           
                                            
      
                                                                                                                        
                                                                                                      
                                              
                                                                                                                     
                                                                                 
                                                                                               
                                   
    if (IsOutput(ort_value_index)) {
      std::string name;
      ORT_THROW_IF_ERROR(ort_value_idx_map_.GetName(ort_value_index, name));
      const Tensor& src = entry.second.Get<Tensor>();                                         
      OrtValue& dest = all_values_[ort_value_index];

#if !defined(DISABLE_SPARSE_TENSORS)
      const bool is_sparse_initializer = is_initializer_sparse_func(name);
      if (is_sparse_initializer) {
        if (!dest.IsAllocated()) {
          auto p_tensor = std::make_unique<SparseTensor>();
          auto ml_tensor = DataTypeImpl::GetType<SparseTensor>();
          dest.Init(p_tensor.release(), ml_tensor, ml_tensor->GetDeleteFunc());
        }

                                                                                                          
        AllocatorPtr allocator = GetAllocator(src.Location().device);
        constexpr bool has_linear_coo_index = true;
        ORT_THROW_IF_ERROR(sparse_utils::DenseTensorToSparseCoo(GetDataTransferManager(), src,
                                                                cpu_allocator, allocator, has_linear_coo_index,
                                                                *dest.GetMutable<SparseTensor>()));
      } else {
#else
      ORT_UNUSED_PARAMETER(is_initializer_sparse_func);
#endif                                      
        if (!dest.IsAllocated()) {
                                                                                                            
                                                                                                                  
                                                                            
          AllocatorPtr allocator = GetAllocator(src.Location().device);
          Tensor::InitOrtValue(src.DataType(), src.Shape(), std::move(allocator), dest);
        }
        ORT_THROW_IF_ERROR(CopyTensor(src, *dest.GetMutable<Tensor>()));
#if !defined(DISABLE_SPARSE_TENSORS)
      }
#endif
    } else {
      all_values_[ort_value_index] = entry.second;
    }
  }

                                                                                    
  for (size_t idx = 0, end = feed_mlvalue_idxs.size(); idx < end; ++idx) {
    int ort_value_idx = feed_mlvalue_idxs[idx];
                                                             
    all_values_[ort_value_idx] = feeds[idx];
  }
}

Status IExecutionFrame::GetOutputs(std::vector<OrtValue>& fetches) {
  auto num_fetches = fetch_mlvalue_idxs_.size();

  if (fetches.empty()) {
    fetches.resize(num_fetches);
  } else {
                                                           
    if (fetches.size() != num_fetches) {
      return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Fetches vector passed to GetOutputs contains ", fetches.size(),
                             " entries which doesn't match the number of fetches the frame was initialized with of ",
                             num_fetches);
    }
  }

  for (size_t idx = 0; idx < num_fetches; ++idx) {
    fetches[idx] = GetMLValue(fetch_mlvalue_idxs_[idx]);
  }

  return Status::OK();
}

bool IExecutionFrame::IsOutput(int ort_value_idx) const {
  return std::find(fetch_mlvalue_idxs_.begin(), fetch_mlvalue_idxs_.end(), ort_value_idx) != fetch_mlvalue_idxs_.end();
}

ExecutionFrame::ExecutionFrame(gsl::span<const int> feed_mlvalue_idxs, gsl::span<const OrtValue> feeds,
                               gsl::span<const int> fetch_mlvalue_idxs, gsl::span<const OrtValue> fetches,
                               const std::unordered_map<size_t, IExecutor::CustomAllocator>& fetch_allocators,
#ifdef ORT_ENABLE_STREAM
                               const DeviceStreamCollection* device_streams,
#endif
                               const SessionState& session_state)
    : IExecutionFrame(session_state.GetOrtValueNameIdxMap(), session_state.GetNodeIndexInfo(), fetch_mlvalue_idxs),
#ifdef ORT_ENABLE_STREAM
      device_streams_(device_streams),
#endif
      session_state_(session_state),
      mem_patterns_(nullptr) {
  Init(
      feed_mlvalue_idxs, feeds, session_state.GetInitializedTensors(),
#if !defined(DISABLE_SPARSE_TENSORS)
      [&session_state](const std::string& name) -> bool {
        int idx = -1;
        if (session_state.GetOrtValueNameIdxMap().GetIdx(name, idx).IsOK()) {
          return session_state.IsSparseInitializer(idx);
        }
        return false;
      },
#else
      [&](const std::string&         ) -> bool {
        return false;
      },
#endif
      fetches);

#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
  session_state.GetMemoryProfiler()->GetMemoryInfo().IncreaseIteration();
#endif

                                                       
  if (!fetch_allocators.empty()) {
    custom_allocators_.reserve(fetch_allocators.size());
    const auto idx_size = fetch_mlvalue_idxs.size();
    for (const auto& e : fetch_allocators) {
      if (e.first < idx_size) {
        int ort_value_idx = fetch_mlvalue_idxs[e.first];
        custom_allocators_.insert_or_assign(ort_value_idx, e.second);
      }
    }
  }

                                                      
                                                       
                                 
  if (session_state.GetEnableMemoryPattern() && session_state.GetExecutionPlan()) {
    bool all_tensors = true;
                                          
    for (const auto& feed : feeds) {
      if (!feed.IsTensor()) {
        all_tensors = false;
        break;
      }
    }

                                                                                                     
    if (all_tensors) {
      mem_patterns_ = session_state.GetMemoryPatternGroup(feeds, feed_mlvalue_idxs, inferred_shapes_);
                                                                      
      if (!mem_patterns_) {
        planner_.emplace(*session_state.GetExecutionPlan());
      } else {
                                                                  
                                                                                            
        buffers_.reserve(mem_patterns_->locations.size());
        for (size_t i = 0; i < mem_patterns_->locations.size(); i++) {
          const auto& location = mem_patterns_->locations[i];
          ORT_ENFORCE(buffers_.find(location) == buffers_.end());
          if (mem_patterns_->patterns[i].PeakSize() > 0) {
            AllocatorPtr alloc = GetAllocator(location);
            void* buffer = nullptr;
                                                                                                                       
                                                                                                              
                                                                                                                   
                                                                                                       
                                                                               
            ORT_TRY {
              auto peak_size = mem_patterns_->patterns[i].PeakSize();
                                                                     
#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
              ORT_ENFORCE(
                  static_activation_memory_sizes_in_byte_.find(location.ToString()) ==
                      static_activation_memory_sizes_in_byte_.end(),
                  "Memory type ",
                  location.ToString(),
                  " should only appear once.");
                                                                                                    
                                                                                                      
              static_activation_memory_sizes_in_byte_[location.ToString()] = peak_size;
#endif
                                                                             
#ifdef ORT_ENABLE_STREAM
                                                                                                    
                                                                                                      
              Stream* mem_pattern_stream = nullptr;
              if (alloc->IsStreamAware() && device_streams_) {
                mem_pattern_stream = device_streams_->GetStreamForDevice(location);
              }

              if (mem_pattern_stream != nullptr) {
                buffer = alloc->AllocOnStream(peak_size, mem_pattern_stream);
              } else {
                buffer = alloc->Alloc(peak_size);
              }
#else
              buffer = alloc->Alloc(peak_size);
#endif
                                                    
              if (buffer == nullptr) {
                                                                                                   
                LOGS(session_state_.Logger(), INFO) << "Allocation of memory pattern buffer for "
                                                    << location.ToString() << " returned nullptr";
              }
            }
            ORT_CATCH(const OnnxRuntimeException& ex) {
              ORT_HANDLE_EXCEPTION([&]() {
                LOGS(session_state_.Logger(), INFO) << "Allocation of memory pattern buffer for "
                                                    << location.ToString() << " failed. Error:" << ex.what();
              });
            }

            if (buffer != nullptr) {
              buffers_[location] = BufferUniquePtr(buffer, BufferDeleter(alloc));
            }
#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
                                               
            auto mem_profier_ptr = session_state.GetMemoryProfiler();
            mem_profier_ptr->GetMemoryInfo().ClearMemoryInfoPerExecution();
            if (mem_patterns_ && buffer != nullptr) {
              mem_profier_ptr->GetMemoryInfo().RecordPatternInfo(*mem_patterns_, MemoryInfo::MapType::StaticActivation);
              mem_profier_ptr->CreateEvents(
                  "static activations_" + std::to_string(mem_profier_ptr->GetMemoryInfo().GetIteration()),
                  mem_profier_ptr->GetAndIncreasePid(), MemoryInfo::MapType::StaticActivation, "", 0);
            }
#endif
                                                                                           
                                                                                                   
                                                                                          
          }
        }
      }
    }
  }
}

ExecutionFrame::~ExecutionFrame() { reconstruction_official_object_frame_close(uintptr_t(this)); }

Status ExecutionFrame::CopyTensor(const Tensor& src, Tensor& dest) const {
  return session_state_.GetDataTransferMgr().CopyTensor(src, dest);
}

const DataTransferManager& ExecutionFrame::GetDataTransferManager() const {
  return session_state_.GetDataTransferMgr();
}

Status ExecutionFrame::AllocateMLValueTensorSelfOwnBuffer(OrtValue& ort_value, int ort_value_index,
                                                          MLDataType element_type, const OrtDevice& location,
                                                          const TensorShape& shape) {
  return AllocateMLValueTensorSelfOwnBufferHelper(ort_value, ort_value_index, element_type, location, shape);
}

Stream* ExecutionFrame::GetValueStream(int ort_value_idx) const {
#ifdef ORT_ENABLE_STREAM
  const auto& value_to_stream_map = const_cast<SessionState&>(session_state_).GetExecutionPlan()->GetValueToStreamMap();
  auto it = value_to_stream_map.find(ort_value_idx);
  if (it != value_to_stream_map.end() && device_streams_ != nullptr && it->second < device_streams_->NumStreams()) {
    return device_streams_->GetStream(it->second);
  }
#else
  ORT_UNUSED_PARAMETER(ort_value_idx);
#endif
  return nullptr;
}

Status ExecutionFrame::AllocateMLValueTensorSelfOwnBufferHelper(OrtValue& ort_value, int ort_value_index,
                                                                MLDataType element_type,
                                                                const OrtDevice& location,
                                                                const TensorShape& shape) {
  if (ort_value_index == NodeIndexInfo::kInvalidEntry) {
    return Status(ONNXRUNTIME, FAIL, "Trying to allocate memory for unused optional inputs/outputs");
  }

                                                                                                 
  const auto alignment = std::max(location.GetAlignment(), kAllocAlignment);

  size_t size = 0;
  ORT_RETURN_IF_ERROR(Tensor::CalculateTensorStorageSize(element_type, shape, alignment, size));

                                             
  AllocatorPtr alloc = nullptr;
  reconstruction_official_object::ValueAllocationScope official_value_scope(uintptr_t(this),size_t(ort_value_index));

                                                                                      
                                                
  const auto& per_alloc_plan = GetAllocationPlan(ort_value_index);

  if (mem_patterns_ && per_alloc_plan.alloc_kind != AllocKind::kAllocateOutput &&
      per_alloc_plan.alloc_kind != AllocKind::kAllocatedExternally) {
    auto pattern = mem_patterns_->GetPatterns(location);
    if (pattern) {
      auto block = pattern->GetBlock(ort_value_index);
                                                          
      if (block) {
        auto it = buffers_.find(location);
        if (it != buffers_.end()) {
                                                                                        
          if (block->size_ == size) {
            void* buffer = it->second.get();
            auto status = AllocateTensorWithPreAllocateBufferHelper(
                ort_value, static_cast<void*>(static_cast<char*>(buffer) + block->offset_), element_type, location,
                shape);
            return status;
          } else {
                                                                                                                 
                                                                        
                                                                                                           
                                                                                                                      
            LOGS(session_state_.Logger(), VERBOSE) << "For ort_value with index: " << ort_value_index
                                                   << ", block in memory pattern size is: " << block->size_
                                                   << " but the actual size is: " << size
                                                   << ", fall back to default allocation behavior";
          }
        }
                                                                                                    
      }
    }
  }

                                                      
  if (!alloc) alloc = GetAllocator(location);
  ORT_ENFORCE(alloc && alloc.get() != nullptr, "Failed to get allocator for ", location.ToString());

  Stream* current_stream = GetValueStream(ort_value_index);
  if (current_stream) {
#ifdef ORT_ENABLE_STREAM
    if (alloc->IsStreamAware()) {
      size_t buffer_size = Tensor::CalculateTensorStorageSize(element_type, shape);
      void* p_data = alloc->AllocOnStream(buffer_size, current_stream);
      Tensor::InitOrtValue(element_type, shape, p_data, std::move(alloc), ort_value);
    } else {
      Tensor::InitOrtValue(element_type, shape, std::move(alloc), ort_value);
    }
#else
    ORT_THROW("Ort value is associated with a Stream but Stream is not enabled in the build.");
#endif
  } else {
    Tensor::InitOrtValue(element_type, shape, std::move(alloc), ort_value);
  }

                                 
                                                                    
                                                                       
  if (!utils::IsDataTypeString(element_type)) {
    TraceAllocate(ort_value_index, size);
  }

  {
#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
                                          
                                                                    
                                    
    std::unique_lock<std::mutex> lock(mtx_);
    dynamic_activation_memory_sizes_in_byte_[location.ToString()] += size;
    session_state_.GetMemoryProfiler()->GetMemoryInfo().SetDynamicAllocation(ort_value_index);
#endif
  }

#if !defined(ORT_MINIMAL_BUILD)
  if (session_state_.GetNodeStatsRecorder() != nullptr) {
    ort_value_to_dynamic_allocations_size_.insert_or_assign(ort_value_index, size);
  }
#endif

  return Status::OK();
}

Status ExecutionFrame::AllocateMLValueTensorPreAllocateBuffer(OrtValue& ort_value, int ort_value_index_reuse,
                                                              MLDataType element_type, const OrtDevice& location,
                                                              const TensorShape& shape,
                                                              bool is_strided_tensor) {
  OrtValue& ort_value_reuse = GetMutableMLValue(ort_value_index_reuse);

  auto* reuse_tensor = ort_value_reuse.GetMutable<Tensor>();

                                                                                                                       
                                                                                                 
#ifndef ENABLE_STRIDED_TENSORS
  ORT_ENFORCE(!is_strided_tensor);
#endif                           
  if (!is_strided_tensor) {
    auto buffer_num_elements = reuse_tensor->Shape().Size();
    auto required_num_elements = shape.Size();

                                                                                          
    if (buffer_num_elements != required_num_elements) {
                                                                                                             
                                                                                                               
      auto message = onnxruntime::MakeString(
          "Shape mismatch attempting to re-use buffer. ", reuse_tensor->Shape(), " != ", shape,
          ". Validate usage of dim_value (values should be > 0) and "
          "dim_param (all values with the same string should equate to the same size) in shapes in the model.");

                                                                                                              
      if (buffer_num_elements >= required_num_elements) {
                                                                             
                                                                                           
#ifndef ENABLE_TRAINING
        LOGS(session_state_.Logger(), WARNING) << message;
#endif                    
      } else {
        return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, message);
      }
    }
  }

  void* reuse_buffer = reuse_tensor->MutableDataRaw();

  return AllocateTensorWithPreAllocateBufferHelper(ort_value, reuse_buffer, element_type, location, shape);
}

Status ExecutionFrame::AllocateTensorWithPreAllocateBufferHelper(OrtValue& ort_value, void* pBuffer,
                                                                 MLDataType element_type,
                                                                 const OrtDevice& location,
                                                                 const TensorShape& shape) {
  Tensor::InitOrtValue(element_type, shape, pBuffer, GetAllocator(location)->Info(), ort_value, 0L);
  return Status::OK();
}

static Status AllocateTraditionalMLValue(OrtValue& ort_value, const NonTensorTypeBase& type) {
  auto creator = type.GetCreateFunc();
  ort_value.Init(creator(), &type, type.GetDeleteFunc());
  return Status::OK();
}

static Status AllocateTensorSequence(OrtValue& ort_value) {
  auto ml_tensor_sequence = DataTypeImpl::GetType<TensorSeq>();
  auto p_tensor_sequence = std::make_unique<TensorSeq>();
  ort_value.Init(p_tensor_sequence.release(), ml_tensor_sequence, ml_tensor_sequence->GetDeleteFunc());

  return Status::OK();
}

#if !defined(DISABLE_SPARSE_TENSORS)
static Status AllocateSparseTensor(OrtValue& mlvalue, const DataTypeImpl& ml_type, AllocatorPtr allocator,
                                   const TensorShape& shape,
                                   const SessionState&                  ) {
  auto element_type = ml_type.AsSparseTensorType()->GetElementType();
  SparseTensor::InitOrtValue(element_type, shape, std::move(allocator), mlvalue);

  return Status::OK();
}
#endif

Status ExecutionFrame::AllocateReusedOrtValueIfNotAllocatedHelper(int reuse_mlvalue_index, const TensorShape* shape) {
                                                                                                  
  // is not allocated (its upstream op is not executed due to the option).
                                                                                         
  OrtValue& reuse_value = GetMutableMLValue(reuse_mlvalue_index);
  if (!reuse_value.IsAllocated()) {
    ORT_RETURN_IF_ERROR(AllocateAsPerAllocationPlan(reuse_value, reuse_mlvalue_index, shape));
  }

  return Status::OK();
}

                                  
Status ExecutionFrame::AllocateAsPerAllocationPlan(OrtValue& ort_value, int ort_value_index, const TensorShape* shape) {
  PwOfficialPrepareFrame(session_state_,uintptr_t(this));
  const auto& alloc_plan = session_state_.GetPerValueAllocPlan();
  ORT_ENFORCE(ort_value_index >= 0 && static_cast<size_t>(ort_value_index) < alloc_plan.size());
  const auto& per_alloc_plan = alloc_plan[ort_value_index];

  const auto& alloc_info = per_alloc_plan.location;
  const auto* ml_type = per_alloc_plan.value_type;
  if (ml_type == nullptr) {
    return Status(
        ONNXRUNTIME, INVALID_ARGUMENT,
        "Tried to allocate without valid type information, ort_value index=" + std::to_string(ort_value_index));
  }

                                                                                          
  auto custom_alloc_entry = custom_allocators_.find(ort_value_index);
  if (custom_alloc_entry != custom_allocators_.cend()) {
    ORT_ENFORCE(shape, "We don't expect custom allocators for non-tensor types, so a shape is mandatory here.");
    bool allocated = false;
                                                    
    auto status = (custom_alloc_entry->second)(*shape, alloc_info, ort_value, allocated);
    if (allocated || !status.IsOK())
      return status;
  }

  if (ml_type->IsTensorType()
#if !defined(DISABLE_OPTIONAL_TYPE)
      || utils::IsOptionalTensor(ml_type)
#endif
  ) {
    ORT_ENFORCE(shape, "Allocation of tensor types requires a shape.");

                                 
#if !defined(DISABLE_OPTIONAL_TYPE)
    const auto* ml_data_type = ml_type->IsTensorType()
                                   ? static_cast<const TensorTypeBase*>(ml_type)->GetElementType()
                                   : utils::GetElementTypeFromOptionalTensor(ml_type);
#else
    const auto* ml_data_type = static_cast<const TensorTypeBase*>(ml_type)->GetElementType();
#endif

    AllocKind alloc_kind = per_alloc_plan.alloc_kind;
    if(reconstruction_official_object::Enabled()) {
      const auto f=uintptr_t(this);
      const bool pooled=reconstruction_official_object_has_value(f,size_t(ort_value_index));
      const bool old_optional_reuse_of_pool=alloc_kind==AllocKind::kReuse &&
          reconstruction_official_object_has_value(f,size_t(per_alloc_plan.reused_buffer));
                                                                                
                                                                              
      if(pooled||old_optional_reuse_of_pool)alloc_kind=AllocKind::kAllocate;
    }
    switch (alloc_kind) {
                                                                                
                                                                      
      case AllocKind::kAllocateOutput:
      case AllocKind::kAllocate: {
        ORT_RETURN_IF_ERROR(AllocateMLValueTensorSelfOwnBuffer(ort_value, ort_value_index, ml_data_type, alloc_info,
                                                               *shape));
        break;
      }
      case AllocKind::kReuse: {
        int reuse_mlvalue_index = per_alloc_plan.reused_buffer;

        ORT_RETURN_IF_ERROR(AllocateReusedOrtValueIfNotAllocatedHelper(reuse_mlvalue_index, shape));

        bool is_strided_tensor = false;
#ifdef ENABLE_STRIDED_TENSORS
        is_strided_tensor = per_alloc_plan.is_strided_tensor;
#endif                           
        ORT_RETURN_IF_ERROR(AllocateMLValueTensorPreAllocateBuffer(
            ort_value, reuse_mlvalue_index, ml_data_type, alloc_info, *shape, is_strided_tensor));
        break;
      }
      case AllocKind::kShare: {
        int reuse_mlvalue_index = per_alloc_plan.reused_buffer;

                                                                                                                 
        ort_value = GetMutableMLValue(reuse_mlvalue_index);
        break;
      }
      default: {
        std::ostringstream ostr;
        ostr << "Invalid allocation kind: " << static_cast<std::underlying_type<AllocKind>::type>(alloc_kind);
        return Status(ONNXRUNTIME, FAIL, ostr.str());
      }
    }

#if !defined(ORT_MINIMAL_BUILD) && defined(ORT_MEMORY_PROFILE)
    session_state_.GetMemoryProfiler()->GetMemoryInfo().RecordActivationAllocInfo(ort_value_index, ort_value);
#endif

    return Status::OK();
  } else if (ml_type->IsSparseTensorType()) {
#if !defined(DISABLE_SPARSE_TENSORS)
    return AllocateSparseTensor(ort_value, *ml_type, GetAllocator(alloc_info),
                                *shape, session_state_);
#else
                                                                  
    ORT_THROW("SparseTensor is not supported in this build.");
#endif
  } else if (ml_type->IsTensorSequenceType()
#if !defined(DISABLE_OPTIONAL_TYPE)
             || utils::IsOptionalSeqTensor(ml_type)
#endif
  ) {
    AllocKind alloc_kind = per_alloc_plan.alloc_kind;

    if (alloc_kind == AllocKind::kReuse) {
      int reuse_mlvalue_index = per_alloc_plan.reused_buffer;

      ORT_RETURN_IF_ERROR(AllocateReusedOrtValueIfNotAllocatedHelper(reuse_mlvalue_index, shape));

      OrtValue& reuse_value = GetMutableMLValue(reuse_mlvalue_index);

                                                                                                               
      ort_value = reuse_value;

      return Status::OK();
    } else {
      return AllocateTensorSequence(ort_value);
    }
  } else {
    return AllocateTraditionalMLValue(ort_value, *static_cast<const NonTensorTypeBase*>(ml_type));
  }
}

AllocatorPtr ExecutionFrame::GetAllocatorImpl(const OrtDevice& info) const {
  return session_state_.GetAllocator(info);
}

                                  
                                                                                          
Status ExecutionFrame::CreateNodeOutputMLValueImpl(OrtValue& ort_value, int ort_value_idx, const TensorShape* shape) {
#ifdef ENABLE_TRAINING
  try {
    auto status = AllocateAsPerAllocationPlan(ort_value, ort_value_idx, shape);
    return status;
  } catch (const std::exception& e) {
    LOGS(session_state_.Logger(), WARNING)
        << "Exception caught when allocating memory for ort_value with index: " << ort_value_idx
        << "so clean up all OrtValues";
    ReleaseAllMLValues();
    return Status(ONNXRUNTIME, FAIL, e.what());
  }
#else
  return AllocateAsPerAllocationPlan(ort_value, ort_value_idx, shape);
#endif
}

void ExecutionFrame::VerifyOutputSizes(int output_index, const Node& node, const TensorShape& output_shape) {
  const NodeArg* output_def = node.OutputDefs()[output_index];
  const auto* expected_shape = output_def->Shape();
  if (expected_shape == nullptr) {
                                                                                                                 
    return;
  }

  const size_t expected_rank = expected_shape->dim_size();
  bool compatible = expected_rank == output_shape.NumDimensions();
  if (compatible) {
    for (size_t i = 0; i < expected_rank; ++i) {
      const auto& expected_dim = expected_shape->dim().Get(static_cast<int>(i));
      if (expected_dim.has_dim_value() && expected_dim.dim_value() != output_shape[i]) {
        compatible = false;
        break;
      }
    }
  }

  if (!compatible) {
    LOGS(session_state_.Logger(), WARNING)
        << "Expected shape from model of " << utils::GetTensorShapeFromTensorShapeProto(*expected_shape)
        << " does not match actual shape of " << output_shape << " for output " << output_def->Name();
  }
}

                                          
Status ExecutionFrame::ReleaseMLValueImpl(int ort_value_idx) {
  ORT_RETURN_IF_ERROR(IExecutionFrame::ReleaseMLValueImpl(ort_value_idx));
  TraceFree(ort_value_idx);
  return Status::OK();
}

const AllocPlanPerValue& ExecutionFrame::GetAllocationPlan(int ort_value_idx) {
  return session_state_.GetPerValueAllocPlan()[ort_value_idx];
}

void ExecutionFrame::TraceAllocate(int ort_value_idx, size_t size) {
  if (planner_.has_value()) {
                                                          
    auto& allocation_plan = GetAllocationPlan(ort_value_idx);
    if (allocation_plan.alloc_kind == AllocKind::kAllocateOutput ||
        allocation_plan.alloc_kind == AllocKind::kAllocatedExternally) {
      return;
    }
    auto status = planner_->TraceAllocation(ort_value_idx, size);
    if (!status.IsOK()) {
      LOGS(session_state_.Logger(), WARNING) << "TraceAllocation for ort_value_idx=" << ort_value_idx
                                             << " size=" << size << " failed: " << status.ErrorMessage();
    }
  }
}

                                          
void ExecutionFrame::TraceFree(int ort_value_idx) {
                                        
  if (planner_.has_value() && !IsOutput(ort_value_idx)) {
    const SequentialExecutionPlan* p_seq_exec_plan = session_state_.GetExecutionPlan();
    const auto& alloc_plan = p_seq_exec_plan->allocation_plan;
    ORT_ENFORCE(ort_value_idx >= 0 && static_cast<size_t>(ort_value_idx) < alloc_plan.size());
    const auto& per_alloc_plan = alloc_plan[ort_value_idx];

                         
    auto ml_type = per_alloc_plan.value_type;
    if (ml_type->IsTensorType()) {
                
      auto ml_data_type = static_cast<const TensorTypeBase*>(ml_type)->GetElementType();
                                   
      if (!utils::IsDataTypeString(ml_data_type)) {
        auto status = planner_->TraceFree(ort_value_idx);
        if (!status.IsOK()) {
          LOGS(session_state_.Logger(), WARNING)
              << "TraceFree for ort_value_idx=" << ort_value_idx << " failed: " << status.ErrorMessage();
        }
      }
    }
  }
}

                                                                                              
                                            
Status ExecutionFrame::GeneratePatterns(MemoryPatternGroup& out) {
  if (!planner_.has_value()) {
    return Status(ONNXRUNTIME, FAIL, "Memory pattern planner is not enabled on this execution framework.");
  }

  return planner_->GeneratePatterns(out);
}

bool ExecutionFrame::TryGetInferredShape(int index, TensorShape& shape) const {
                                     
  int ort_value_idx = GetNodeIdxToMLValueIdx(index);

                             
  if (ort_value_idx == NodeIndexInfo::kInvalidEntry) {
    return false;
  }

                               
                                                                                        
  if (inferred_shapes_ != nullptr) {
    auto it = inferred_shapes_->find(ort_value_idx);
    if (it != inferred_shapes_->end()) {
      shape = it->second;
      return true;
    }
  }

                                                        
  return false;
}

}                          
