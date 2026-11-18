                                                                             
#pragma once
#include "core/framework/kernel_registry.h"
#include "core/providers/webgpu/math/matmul_packed.h"
#include "core/providers/webgpu/math/matmul.h"
namespace onnxruntime { namespace webgpu { namespace high_profile {
class PwS3DirectReadProgram final : public Program<PwS3DirectReadProgram> {
 public:
  PwS3DirectReadProgram(const Activation& activation, bool bias, bool is_vec4, const gsl::span<int64_t>& elements_per_thread, bool is_channels_last = false, uint32_t split_dim_inner = 1, bool direct_read = true) : Program{"high_profile::PwS3DirectReadMatMul_v1"},
                                                                                                                                                                                     activation_(activation),
                                                                                                                                                                                     has_bias_{bias},
                                                                                                                                                                                     is_vec4_{is_vec4},
                                                                                                                                                                                     elements_per_thread_(elements_per_thread.begin(), elements_per_thread.end()),
                                                                                                                                                                                     is_channels_last_(is_channels_last),
                                                                                                                                                                                     split_dim_inner_(split_dim_inner), direct_read_(direct_read) {}

  Status GenerateShaderCode(ShaderHelper& sh) const override;
  WEBGPU_PROGRAM_DEFINE_UNIFORM_VARIABLES({"dim_a_outer", ProgramUniformVariableDataType::Uint32},
                                          {"dim_b_outer", ProgramUniformVariableDataType::Uint32},
                                          {"dim_inner", ProgramUniformVariableDataType::Uint32},
                                          {"logical_dispatch_x", ProgramUniformVariableDataType::Uint32},
                                          {"logical_dispatch_y", ProgramUniformVariableDataType::Uint32},
                                          {"logical_dispatch_z", ProgramUniformVariableDataType::Uint32},
                                          {"splits_per_batch", ProgramUniformVariableDataType::Uint32});

  bool NeedSplitK() const;

 private:
  const Activation activation_;
  const bool has_bias_;
  const bool is_vec4_;
  const InlinedVector<int64_t> elements_per_thread_;
  bool is_channels_last_ = false;
  uint32_t split_dim_inner_ = 1;
  const bool direct_read_;
};

KernelCreateInfo CreatePwS3DirectReadKernelInfo();
void RegisterPwS3DirectReadSchema();
}}}
