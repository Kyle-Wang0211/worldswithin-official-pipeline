#pragma once
#include "persistent_full_mvs_gpu.h"
#include <functional>
namespace aether::dense {
// IO-only adapter. All image, camera, neighbor and noise bytes belong to dense_run.
class DenseS1Pass {
 public:
 using ReadHalf = std::function<std::vector<uint16_t>(int)>;
 DenseS1Pass(); ~DenseS1Pass();
 bool Initialize(int width, const std::vector<float>& cameras,
   const std::vector<int32_t>& neighbors, const std::vector<int>& original_rows,
   int frozen_noise_fd, ReadHalf read_half, std::string* error, double* init_ms);
 bool Run(int reference, float* const output[4], size_t elements,
   double* elapsed_ms, std::string* error);
 reconstruction::ResidentCounters Counters() const;
 private: std::unique_ptr<reconstruction::PersistentFullMvsGpu> run_;
};
}
