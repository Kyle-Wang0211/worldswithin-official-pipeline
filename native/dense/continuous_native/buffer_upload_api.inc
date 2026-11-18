#pragma once
#include <cstddef>
extern "C" int pw_upload_fixed_input_begin(size_t bytes) noexcept;
extern "C" void pw_upload_fixed_input_end() noexcept;
namespace pw_upload {
// Only construct directly around one immutable HIGH input tensor allocation.
// A successful begin must pair with end even if allocation/upload throws.
class FixedInputScope final {
 bool armed_;
 public:
 explicit FixedInputScope(size_t bytes) noexcept : armed_(pw_upload_fixed_input_begin(bytes)==0) {}
 ~FixedInputScope(){if(armed_)pw_upload_fixed_input_end();}
 bool armed()const noexcept{return armed_;}
 FixedInputScope(const FixedInputScope&)=delete;
 FixedInputScope&operator=(const FixedInputScope&)=delete;
};
}

#include <cstdlib>
#include <utility>
#include <cstring>
namespace pw_upload {
template<class Allocate>
decltype(auto) AllocateHighFixedInput(int width,int height,bool material,size_t bytes,Allocate&& allocate) {
 const char*enabled=std::getenv("RECONSTRUCTION_BENCH_UMA_INPUT_UPLOAD");
 if(width!=1536 || height!=1152 || !material || !enabled || std::strcmp(enabled,"1")!=0) return std::forward<Allocate>(allocate)();
 FixedInputScope scope(bytes);
 return std::forward<Allocate>(allocate)();
}
}

namespace pw_upload {
template<class Allocate>
decltype(auto) AllocateWhole36HighFixedInput(int scene_frames,int width,int height,bool material,size_t bytes,Allocate&&allocate){
 return AllocateHighFixedInput(width,height,material,bytes,std::forward<Allocate>(allocate));
}
}
