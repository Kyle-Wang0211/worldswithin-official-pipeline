#pragma once
#include <cstdint>
#include <cstddef>
namespace pw_upload {
                                                                                                     
                                                                                          
struct Request { size_t bytes=0; bool armed=false; };
inline thread_local Request request;
inline bool Arm(size_t bytes) noexcept {
  if (!bytes || request.armed) return false;
  request={bytes,true}; return true;
}
inline bool Consume(size_t bytes, bool actual_device_feature, bool verified_uma, bool standard_input_usage) noexcept {
  const auto r=request; request={};
  return r.armed && r.bytes==bytes && actual_device_feature && verified_uma && standard_input_usage;
}
inline void Cancel() noexcept { request={}; }
}
extern "C" __attribute__((visibility("default"))) int pw_upload_fixed_input_begin(size_t bytes) noexcept { return pw_upload::Arm(bytes) ? 0 : -1; }
extern "C" __attribute__((visibility("default"))) void pw_upload_fixed_input_end() noexcept { pw_upload::Cancel(); }
