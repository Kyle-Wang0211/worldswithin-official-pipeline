#pragma once
#include "core/framework/kernel_registry.h"
namespace onnxruntime { namespace webgpu {
inline constexpr char kPwGridDomain[]="reconstruction.experimental";
KernelCreateInfo CreatePwGridSampleGroupMeanKernelInfo();
KernelCreateInfo CreatePwCostAccumulate3KernelInfo();
KernelCreateInfo CreatePwWeightedAccumulateKernelInfo();
KernelCreateInfo CreatePwConcatConv7KernelInfo();
}}
extern "C" __attribute__((visibility("default"))) int ReconstructionRegisterExperimentalSchema() noexcept;
