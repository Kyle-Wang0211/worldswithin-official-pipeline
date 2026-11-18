#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdint>
extern "C" void reconstruction_official_object_value_enter(uintptr_t frame,size_t value_index) noexcept;
extern "C" void reconstruction_official_object_value_leave() noexcept;
extern "C" bool reconstruction_official_object_frame_known(uintptr_t frame) noexcept;
extern "C" void reconstruction_official_object_frame_register(uintptr_t frame,const size_t* values,size_t count);
extern "C" bool reconstruction_official_object_has_value(uintptr_t frame,size_t value_index) noexcept;
extern "C" void reconstruction_official_object_frame_close(uintptr_t frame) noexcept;
extern "C" bool reconstruction_official_object_buffer_shared(uintptr_t buffer) noexcept;
namespace reconstruction_official_object {
inline bool Enabled(){const char*p=std::getenv("RECONSTRUCTION_OFFICIAL_OBJECT_POOL");return p&&p[0]=='1'&&!p[1];}
struct ValueAllocationScope {
 ValueAllocationScope(uintptr_t frame,size_t value){reconstruction_official_object_value_enter(frame,value);}
 ~ValueAllocationScope(){reconstruction_official_object_value_leave();}
 ValueAllocationScope(const ValueAllocationScope&)=delete;
};
}
