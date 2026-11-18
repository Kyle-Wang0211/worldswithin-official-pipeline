#pragma once
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
                                                                             
                                                                                
                                                                   
namespace pw37_original80d1 {
struct Range { uint64_t offset, size; };
inline std::vector<Range> LogicalBindingRanges(uint64_t logical_bytes,
    uint64_t physical_capacity, uint64_t max_binding, uint64_t offset_alignment) {
  if (!logical_bytes || logical_bytes > std::numeric_limits<uint64_t>::max()-15 ||
      !max_binding || !offset_alignment || max_binding % offset_alignment)
    throw std::invalid_argument("invalid logical binding dimensions");
  const auto padded = (logical_bytes+15)/16*16;
  if (padded > physical_capacity || max_binding % 16)
    throw std::invalid_argument("logical binding exceeds owner or invalid limit");
  std::vector<Range> out;
  for (uint64_t offset=0; offset<padded;) {
    const auto size = padded-offset < max_binding ? padded-offset : max_binding;
    out.push_back({offset,size});offset+=size;
  }
  return out;
}
}
