                                                                                       
                                                                    
#include <unordered_map>
#include <mutex>
#include <cstdio>
namespace {
using PwGraphCommands = std::vector<onnxruntime::webgpu::CapturedCommandInfo>;
struct PwCapturedCopy { size_t before_dispatch; wgpu::Buffer src, dst; size_t bytes; };
struct PwCopyRegistry { std::mutex mutex; std::unordered_map<const PwGraphCommands*, std::vector<PwCapturedCopy>> copies; };
PwCopyRegistry& PwGraphCopies(){ static auto* r = new PwCopyRegistry; return *r; }
}
