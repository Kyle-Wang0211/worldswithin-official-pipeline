#pragma once
#include <cstdint>
namespace reconstructions1 {
inline bool would_exceed(uint64_t current,uint64_t request,uint64_t limit) {
 return !limit || current>limit || request>limit-current;
}
}
