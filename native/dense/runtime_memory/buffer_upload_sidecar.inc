#pragma once
#include <mutex>
#include <unordered_set>
namespace pw_upload {
inline std::mutex mapped_mu;
inline std::unordered_set<WGPUBuffer> mapped_inputs;
inline void Mark(WGPUBuffer b) { std::lock_guard<std::mutex> lock(mapped_mu); mapped_inputs.insert(b); }
inline bool Take(WGPUBuffer b) { std::lock_guard<std::mutex> lock(mapped_mu); return mapped_inputs.erase(b)!=0; }
}
