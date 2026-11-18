// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// Isolated exact-node consumer-read adapter; no floating arithmetic authored here.
#include "pw_s3_direct_read.h"
#include "core/providers/webgpu/math/gemm_utils.h"
#include "core/providers/webgpu/webgpu_utils.h"
#include "core/providers/webgpu/webgpu_context.h"
#include "core/graph/graph.h"
#include "onnx/defs/schema.h"
#include "onnx/defs/shape_inference.h"
#include "absl/strings/str_join.h"
#include <iostream>
#include <limits>
namespace onnxruntime { namespace webgpu { namespace high_profile {
                                                               
                                                                                     
                                                                  
Status PwS3DirectReadProgram::GenerateShaderCode(ShaderHelper& shader) const {
  const auto& a = shader.AddInput("a", ShaderUsage::UseUniform | ShaderUsage::UseIndicesTypeAlias | ShaderUsage::UseValueTypeAlias | ShaderUsage::UseElementTypeAlias);
  const auto& b = shader.AddInput("b", ShaderUsage::UseUniform | ShaderUsage::UseIndicesTypeAlias | ShaderUsage::UseValueTypeAlias);

  const bool need_split_k = NeedSplitK();
  ShaderUsage output_usage = ShaderUsage::UseUniform | ShaderUsage::UseIndicesTypeAlias | ShaderUsage::UseValueTypeAlias | ShaderUsage::UseElementTypeAlias;
  if (need_split_k) {
                                                                                               
                                                                                                  
    output_usage |= ShaderUsage::UseIndicesToOffset | ShaderUsage::UseShapeAndStride;
  }
  const auto& output = shader.AddOutput("output", output_usage);

  const auto& batch_dims = shader.AddIndices("batch_dims", ShaderUsage::UseUniform | ShaderUsage::UseIndicesTypeAlias);

  const ShaderVariableHelper* bias = nullptr;
  if (has_bias_) {
    bias = &shader.AddInput("bias", ShaderUsage::UseUniform);
  }
  std::string apply_activation = GetActivationSnippet(activation_, "output_value_t", "output_element_t");
  ProgramVariableDataType output_var_type = this->Outputs()[0].var_type;
                                         
  if (direct_read_) {
                                                                        
                                                           
    auto& code = shader.AdditionalImplementation();
    code << "fn mm_readA(batch: i32, row: i32, colIn: i32, batch_indices: batch_dims_indices_t) -> vec4<output_element_t> {\n"
         << "  var value = vec4<output_element_t>(0);\n"
         << "  let col = colIn * 4;\n"
         << "  if(row < i32(uniforms.dim_a_outer) && col < i32(uniforms.dim_inner)) {\n"
         << "    let p = ((u32(row) / 384u * 2u) * 768u + (u32(row) % 384u * 2u)) * 8u + u32(colIn);\n";
    for (uint32_t lane = 0; lane < 4; ++lane) {
      const std::string off = "p + " + std::to_string((lane / 2) * 768 * 8 + (lane % 2) * 8) + "u";
      code << "    value[" << lane << "] = (" << a.GetByOffset("(" + off + ") / 4u") << ")[(" << off << ") % 4u];\n";
    }
    code << "  }\n  return value;\n}\n\n";
    MatMulReadFnSource(shader, "mm_readB", b, "b", &batch_dims, "uniforms.dim_inner", "uniforms.dim_b_outer", false);
  } else {
    MatMulReadFnSource(shader, a, b, &batch_dims,               false,               false);
  }
  if (need_split_k) {
    MatMulWriteFnSourceWithSplitK(shader, output,                false, output_var_type);
  } else {
    MatMulWriteFnSourceForMatMul(shader, output, bias, apply_activation, is_channels_last_);
  }
  std::string data_type = "a_element_t";
                               
  if (is_vec4_) {
    ORT_RETURN_IF_ERROR(MakeMatMulPackedVec4Source(
        shader, elements_per_thread_, WorkgroupSizeX(), WorkgroupSizeY(), data_type, &batch_dims,
                      false,               false,              1.f,                           true,
                                 4,                   32, need_split_k, split_dim_inner_));
  } else {
    ORT_RETURN_IF_ERROR(MakeMatMulPackedSource(shader, elements_per_thread_, WorkgroupSizeX(), WorkgroupSizeY(), data_type, &batch_dims));
  }
  return Status::OK();
}

bool PwS3DirectReadProgram::NeedSplitK() const {
  return split_dim_inner_ > 1;
}

Status ComputePwS3DirectReadMatMul(ComputeContext* context,
                     const Activation& activation, std::vector<const Tensor*>& inputs, Tensor* output_tensor, bool is_channels_last,
                     const TensorShape& input_a_reshape,
                     const TensorShape& input_b_reshape, bool direct_read) {
  const auto* a = inputs[0];
  const auto* b = inputs[1];
  bool has_bias = inputs.size() > 2;
  TensorShape a_shape = input_a_reshape.NumDimensions() > 0 ? input_a_reshape : a->Shape();
  TensorShape b_shape = input_b_reshape.NumDimensions() > 0 ? input_b_reshape : b->Shape();

  MatMulComputeHelper helper;
  ORT_THROW_IF_ERROR(helper.Compute(a_shape, b_shape));
  int64_t batchA = a_shape.SizeToDimension(a_shape.NumDimensions() - 2);
  int64_t batchB = b_shape.SizeToDimension(b_shape.NumDimensions() - 2);

  TensorShape output_shape = helper.OutputShape();

                                                                                    
                                             
  if (batchA != 1 && batchB == 1) {
                                         
    int64_t batchAndM = a_shape.SizeToDimension(a_shape.NumDimensions() - 1);
    TensorShapeVector dims_a = {batchAndM, helper.K()};
                              
    TensorShapeVector dims_b = {helper.K(), helper.N()};

    a_shape = TensorShape(dims_a);
    b_shape = TensorShape(dims_b);
    output_shape = {batchAndM, helper.N()};
  }

                                
  TensorShape outer_dims_a = a_shape.NumDimensions() > 2
                                 ? a_shape.Slice(0, a_shape.NumDimensions() - 2)
                                 : TensorShape({});

  TensorShape outer_dims_b = b_shape.NumDimensions() > 2
                                 ? b_shape.Slice(0, b_shape.NumDimensions() - 2)
                                 : TensorShape({});

  TensorShape outer_dims = output_shape.NumDimensions() > 2
                               ? output_shape.Slice(0, output_shape.NumDimensions() - 2)
                               : TensorShape({});

  const int64_t batch_size = outer_dims.Size();

                                                              
  const uint32_t dim_a_outer = narrow<uint32_t>(a_shape[a_shape.NumDimensions() - 2]);                                 
  const uint32_t dim_inner = narrow<uint32_t>(a_shape[a_shape.NumDimensions() - 1]);                                  
  const uint32_t dim_b_outer = narrow<uint32_t>(b_shape[b_shape.NumDimensions() - 1]);                                 

  const bool is_vec4 = dim_inner % 4 == 0 && dim_b_outer % 4 == 0;

  InlinedVector<int64_t> elements_per_thread = dim_a_outer <= 8
                                                   ? InlinedVector<int64_t>({4, 1, 1})
                                                   : InlinedVector<int64_t>({4, 4, 1});

  const uint32_t dispatch_x = narrow<uint32_t>((dim_b_outer + MatMul::MATMUL_PACKED_WORKGROUP_SIZE_X * elements_per_thread[0] - 1) /
                                               (MatMul::MATMUL_PACKED_WORKGROUP_SIZE_X * elements_per_thread[0]));
  const uint32_t dispatch_y = narrow<uint32_t>((dim_a_outer + MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Y * elements_per_thread[1] - 1) /
                                               (MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Y * elements_per_thread[1]));
  uint32_t dispatch_z = narrow<uint32_t>((static_cast<uint32_t>(batch_size) + MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Z * elements_per_thread[2] - 1) /
                                         (MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Z * elements_per_thread[2]));

  const int components = is_vec4 ? 4 : 1;
  const TensorShape a_shape_temp = CreateMatMulIntermediateShape(outer_dims_a, dim_a_outer, dim_inner, components);
  const TensorShape b_shape_temp = CreateMatMulIntermediateShape(outer_dims_b, dim_inner, dim_b_outer, components);
  const TensorShape output_shape_temp = TensorShape({batch_size, dim_a_outer, dim_b_outer / components});

  ProgramOutput output(output_tensor, ProgramTensorMetadataDependency::Rank, output_shape_temp, components);
  const Tensor* bias = has_bias ? inputs[2] : nullptr;
  bool use_bias_in_matmul = has_bias;
  uint32_t split_dim_inner = 1;
  uint32_t splits_per_batch = 1;

                                                                                             
  if (!context->KernelContext().GetUseDeterministicCompute()) {
    const SplitKConfig& split_k_config = context->GetSplitKConfig();
    const bool need_split_k = split_k_config.UseSplitK(is_vec4, activation.activation_kind_, batch_size, dim_a_outer, dim_b_outer, dim_inner, is_channels_last);
    if (need_split_k) {
      ORT_ENFORCE(is_vec4, "Split-K MatMul requires vec4 packing.");

      if (has_bias) {
        ORT_ENFORCE(is_channels_last, "Split-K MatMul only supports channels-last format.");
      }

                                                                                             
      const auto fill_bias_program = CreateMatMulFillBiasOrZeroBeforeSplitKProgram(bias, output_tensor,             false,          1.0f,                     4, output_shape_temp, narrow<uint32_t>(batch_size));
      ORT_RETURN_IF_ERROR(context->RunProgram(fill_bias_program));

                                                                                                
                                         
      use_bias_in_matmul = false;

                                                                                          
                                                                                                
      split_dim_inner = split_k_config.GetSplitDimInner();
      splits_per_batch = (dim_inner + split_dim_inner - 1) / split_dim_inner;
      const uint64_t dispatch_z_u64 = static_cast<uint64_t>(batch_size) * static_cast<uint64_t>(splits_per_batch);
      ORT_ENFORCE(dispatch_z_u64 <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
                  "dispatch_z exceeds uint32_t range: ", dispatch_z_u64);
      dispatch_z = narrow<uint32_t>(dispatch_z_u64);

                                                                                               
                            
      output.is_atomic = true;
    }
  }

  PwS3DirectReadProgram matmul_program{activation, use_bias_in_matmul, is_vec4, elements_per_thread, is_channels_last, split_dim_inner, direct_read};
  matmul_program
      .CacheHint("physical_NHWC_1_576_768_8_logical_1_110592_32_v1", direct_read, activation.ToString(), absl::StrJoin(elements_per_thread, "-"), std::to_string(is_vec4), components, is_channels_last, split_dim_inner)
      .AddInputs({{a, ProgramTensorMetadataDependency::TypeAndRank, a_shape_temp, components},
                  {b, ProgramTensorMetadataDependency::TypeAndRank, b_shape_temp, components}})
      .AddUniformVariables({{dim_a_outer}, {dim_b_outer}, {dim_inner}, {dispatch_x}, {dispatch_y}, {dispatch_z}, {splits_per_batch}})
      .AddIndices(outer_dims)
      .SetDispatchGroupSize(dispatch_x, dispatch_y, dispatch_z)
      .SetWorkgroupSize(MatMul::MATMUL_PACKED_WORKGROUP_SIZE_X, MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Y, MatMul::MATMUL_PACKED_WORKGROUP_SIZE_Z)
      .AddOutput(std::move(output));

  if (use_bias_in_matmul) {
    auto bias_components = is_channels_last ? components : 1;
    TensorShape reduced_bias_shape = ReduceShapeByComponents(bias->Shape(), bias_components);
    matmul_program.AddInput({bias, ProgramTensorMetadataDependency::Rank, reduced_bias_shape, bias_components});
  }

  return context->RunProgram(matmul_program);
}


Status PwSelectedS3DirectReadCompute(ComputeContext& context){
    const auto* x=context.Input<Tensor>(0);const auto* w=context.Input<Tensor>(1);const auto* bias=context.Input<Tensor>(2);
    ORT_RETURN_IF_NOT(x->IsDataType<float>()&&w->IsDataType<float>()&&bias->IsDataType<float>(),"S3 f32 only");
    const TensorShape expected=true?TensorShape({1,576,768,8}):TensorShape({1,288,384,32});
    ORT_RETURN_IF_NOT(x->Shape()==expected&&w->Shape()==TensorShape({1,32,8})&&bias->Shape()==TensorShape({8}),"S3 exact NHWC logical/physical guard");
    auto* out=context.Output(0,TensorShape({1,288,384,8}));
    std::vector<const Tensor*> inputs{x,w,bias};
    std::cerr<<"PW_S3_DIRECT_READ mode="<<true<<" physical=1,576,768,8 logical=1,110592,32 workgroup=8,8,1 tile=32 K=32\n";
    return ComputePwS3DirectReadMatMul(&context,Activation(),inputs,out,true,TensorShape({1,110592,32}),TensorShape({1,32,8}),true!=0);
  
}
}}}
