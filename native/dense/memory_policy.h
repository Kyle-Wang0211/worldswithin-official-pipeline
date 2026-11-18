#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
// Explicit product-only observer policy. Empty/default remains the isolated
// Bench 2 GB admission policy; INT64_MAX is valid in every existing memory ABI.
namespace reconstruction_memory_policy {
constexpr uint64_t kBenchLimit = 2000000000ULL;
constexpr uint64_t kObserveLimit = 9223372036854775807ULL;
inline bool valid() noexcept {
 const char* p = std::getenv("RECONSTRUCTION_MEMORY_POLICY");
 return !p || !*p || std::strcmp(p,"validation_memory_limit")==0 ||
        std::strcmp(p,"production_observe_only_v1")==0;
}
inline bool observe_only() noexcept {
 const char* p = std::getenv("RECONSTRUCTION_MEMORY_POLICY");
 return p && std::strcmp(p,"production_observe_only_v1")==0;
}
inline uint64_t limit() noexcept { return observe_only() ? kObserveLimit : kBenchLimit; }
}
