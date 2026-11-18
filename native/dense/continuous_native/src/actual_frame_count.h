#pragma once
#include <stdint.h>
namespace reconstruction { constexpr bool actual_frame_count_supported(uint32_t count) noexcept { return count > 0; } }
extern "C" __attribute__((visibility("default"))) int reconstruction_continuous_mvs_frame_count_supported_v1(uint32_t);
extern "C" __attribute__((visibility("default"))) int reconstruction_whole_dawn_frame_count_supported_v1(uint32_t);
