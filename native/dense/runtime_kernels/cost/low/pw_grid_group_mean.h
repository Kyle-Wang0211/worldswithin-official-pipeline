#pragma once
#include "core/framework/kernel_registry.h"
namespace onnxruntime { namespace webgpu { namespace low_profile {
inline constexpr char kPwGridDomain[]="reconstruction.experimental";
KernelCreateInfo CreatePwGridSampleGroupMeanKernelInfo();
KernelCreateInfo CreatePwWeightedAccumulateKernelInfo();
KernelCreateInfo CreatePwConcatConv7KernelInfo();
}}}
extern "C" __attribute__((visibility("default"))) int ReconstructionRegisterExperimentalSchema() noexcept;
