/*
From PyTorch:

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou, Iain Melvin, Jason Weston)
Copyright (c) 2006      Idiap Research Institute (Samy Bengio)
Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert, Samy Bengio, Johnny Mariethoz)

From Caffe2:

Copyright (c) 2016-present, Facebook Inc. All rights reserved.

All contributions by Facebook:
Copyright (c) 2016 Facebook Inc.

All contributions by Google:
Copyright (c) 2015 Google Inc.
All rights reserved.

All contributions by Yangqing Jia:
Copyright (c) 2015 Yangqing Jia
All rights reserved.

All contributions by Kakao Brain:
Copyright 2019-2020 Kakao Brain

All contributions by Cruise LLC:
Copyright (c) 2022 Cruise LLC.
All rights reserved.

All contributions by Tri Dao:
Copyright (c) 2024 Tri Dao.
All rights reserved.

All contributions by Arm:
Copyright (c) 2021, 2023-2025 Arm Limited and/or its affiliates

All contributions from Caffe:
Copyright(c) 2013, 2014, 2015, the respective contributors
All rights reserved.

All other contributions:
Copyright(c) 2015, 2016 the respective contributors
All rights reserved.

Caffe2 uses a copyright model similar to Caffe: each contributor holds
copyright over their contributions to Caffe2. The project versioning records
all such contribution and copyright details. If a contributor wants to further
mark their specific copyright on a particular contribution, they should
indicate their copyright solely in the commit message of the change when it is
committed.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories America
   and IDIAP Research Institute nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


*/
// Extracted from PyTorch 2.12.0 revision 7661cd9c6b841b62b7f411aa52ec51f05457263b.
// Installed upstream source SHA256 identities
// MT19937RNGEngine.h 8df329422d29c965f1356b511caa26f0f72f74db97b1987509e7bafa94f57467
// TransformationHelper.h 8eff8a994ada7b28c0f0ed8c6564d232160de78b211ba49e49145bc586259260
// DistributionsHelper.h 51e4995b9780f828f47fb8d18346f73c10ade7e227fc06c730ff43b1e95e47ba
// DistributionTemplates.h dce75f8036a0dbeed823f7b282245673b74782e7af182755fd6babee7708331b
// Adaptations remove ATen Tensor/mutex dispatch and use caller-owned float storage.
// c10::irange loops become ordinary integer loops. RNG, float uniform conversion,
// and generic float normal_fill_16 arithmetic retain upstream expressions.
// This helper has no Torch global state. Each frame owns one upstream MT engine.
#include "reconstruction_official_noise.h"
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "Original noise requires IEEE754 float32");
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Original noise requires little-endian output"
#endif
namespace c10 {
template <typename T>
constexpr T pi = static_cast<T>(3.141592653589793238462643383279502);
}
namespace at {

constexpr int MERSENNE_STATE_N = 624;
constexpr int MERSENNE_STATE_M = 397;
constexpr uint32_t MATRIX_A = 0x9908b0df;
constexpr uint32_t UMASK = 0x80000000;
constexpr uint32_t LMASK = 0x7fffffff;

/**
 * Note [Mt19937 Engine implementation]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Originally implemented in:
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/MTARCOK/mt19937ar-cok.c
 * and modified with C++ constructs. Moreover the state array of the engine
 * has been modified to hold 32 bit uints instead of 64 bits.
 *
 * Note that we reimplemented mt19937 instead of using std::mt19937 because,
 * at::mt19937 turns out to be faster in the pytorch codebase. PyTorch builds with -O2
 * by default and following are the benchmark numbers (benchmark code can be found at
 * https://github.com/syed-ahmed/benchmark-rngs):
 *
 * with -O2
 * Time to get 100000000 philox randoms with at::uniform_real_distribution = 0.462759s
 * Time to get 100000000 at::mt19937 randoms with at::uniform_real_distribution = 0.39628s
 * Time to get 100000000 std::mt19937 randoms with std::uniform_real_distribution = 0.352087s
 * Time to get 100000000 std::mt19937 randoms with at::uniform_real_distribution = 0.419454s
 *
 * std::mt19937 is faster when used in conjunction with std::uniform_real_distribution,
 * however we can't use std::uniform_real_distribution because of this bug:
 * http://open-std.org/JTC1/SC22/WG21/docs/lwg-active.html#2524. Plus, even if we used
 * std::uniform_real_distribution and filtered out the 1's, it is a different algorithm
 * than what's in pytorch currently and that messes up the tests in tests_distributions.py.
 * The other option, using std::mt19937 with at::uniform_real_distribution is a tad bit slower
 * than at::mt19937 with at::uniform_real_distribution and hence, we went with the latter.
 *
 * Copyright notice:
 * A C-program for MT19937, with initialization improved 2002/2/10.
 * Coded by Takuji Nishimura and Makoto Matsumoto.
 * This is a faster version by taking Shawn Cokus's optimization,
 * Matthe Bellew's simplification, Isaku Wada's real version.
 *
 * Before using, initialize the state by using init_genrand(seed)
 * or init_by_array(init_key, key_length).
 *
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   3. The names of its contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Any feedback is very welcome.
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
 */

/**
 * mt19937_data_pod is used to get POD data in and out
 * of mt19937_engine. Used in torch.get_rng_state and
 * torch.set_rng_state functions.
 */
struct mt19937_data_pod {
  uint64_t seed_;
  int left_;
  bool seeded_;
  uint32_t next_;
  std::array<uint32_t, MERSENNE_STATE_N> state_;
};

class mt19937_engine {
public:

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  inline explicit mt19937_engine(uint64_t seed = 5489) {
    init_with_uint32(seed);
  }

  inline mt19937_data_pod data() const {
    return data_;
  }

  inline void set_data(const mt19937_data_pod& data) {
    data_ = data;
  }

  inline uint64_t seed() const {
    return data_.seed_;
  }

  inline bool is_valid() {
    if ((data_.seeded_ == true)
      && (data_.left_ > 0 && data_.left_ <= MERSENNE_STATE_N)
      && (data_.next_ <= MERSENNE_STATE_N)) {
      return true;
    }
    return false;
  }

  inline uint32_t operator()() {
    if (--(data_.left_) == 0) {
        next_state();
    }
    uint32_t y = *(data_.state_.data() + data_.next_++);
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= (y >> 18);

    return y;
  }

private:
  mt19937_data_pod data_;

  inline void init_with_uint32(uint64_t seed) {
    data_.seed_ = seed;
    data_.seeded_ = true;
    data_.state_[0] = seed & 0xffffffff;
    for (int j = 1; j < MERSENNE_STATE_N; ++j) {
      data_.state_[j] = (1812433253 * (data_.state_[j-1] ^ (data_.state_[j-1] >> 30)) + j);
    }
    data_.left_ = 1;
    data_.next_ = 0;
  }

  inline uint32_t mix_bits(uint32_t u, uint32_t v) {
    return (u & UMASK) | (v & LMASK);
  }

  inline uint32_t twist(uint32_t u, uint32_t v) {
    return (mix_bits(u,v) >> 1) ^ (v & 1 ? MATRIX_A : 0);
  }

  inline void next_state() {
    uint32_t* p = data_.state_.data();
    data_.left_ = MERSENNE_STATE_N;
    data_.next_ = 0;

    for(int j = MERSENNE_STATE_N - MERSENNE_STATE_M + 1; --j; p++) {
      *p = p[MERSENNE_STATE_M] ^ twist(p[0], p[1]);
    }

    for(int j = MERSENNE_STATE_M; --j; p++) {
      *p = p[MERSENNE_STATE_M - MERSENNE_STATE_N] ^ twist(p[0], p[1]);
    }

    *p = p[MERSENNE_STATE_M - MERSENNE_STATE_N] ^ twist(p[0], data_.state_[0]);
  }

};

typedef mt19937_engine mt19937;

} // namespace at
namespace {
thread_local char last_error[512] = {};
int failure(int code, const char *message) {
  std::snprintf(last_error, sizeof(last_error), "%s", message);
  return code;
}
// CPUGeneratorImpl::random() delegates directly to its upstream MT engine.
struct Generator {
  explicit Generator(uint64_t seed) : engine(seed) {}
  uint32_t random() { return engine(); }
  at::mt19937 engine;
};
// Float specialization of upstream transformation::uniform_real and
// uniform_real_distribution. Only [0,1) float32 is used by the original draws.
float uniform_real(uint32_t val, float from, float to) {
  constexpr auto MASK = static_cast<uint32_t>(
      (static_cast<uint64_t>(1) << std::numeric_limits<float>::digits) - 1);
  constexpr auto DIVISOR = static_cast<float>(1) /
      (static_cast<uint64_t>(1) << std::numeric_limits<float>::digits);
  float x = (val & MASK) * DIVISOR;
  return (x * (to - from) + from);
}
template <typename scalar_t>
void normal_fill_16(scalar_t *data, const scalar_t mean, const scalar_t std) {
  for (int j = 0; j < 8; ++j) {
    const scalar_t u1 = 1 - data[j]; // [0, 1) -> (0, 1] for log.
    const scalar_t u2 = data[j + 8];
    const scalar_t radius = std::sqrt(-2 * std::log(u1));
    const scalar_t theta = 2.0f * c10::pi<double> * u2;
    data[j] = radius * std::cos(theta) * std + mean;
    data[j + 8] = radius * std::sin(theta) * std + mean;
  }
}


void normal_fill(float *data, size_t size, Generator *generator) {
  // The supported original stage shapes are contiguous float32 and multiples16.
  // These are the upstream float normal_fill loops with Tensor access removed.
  const float mean = 0.0f;
  const float std = 1.0f;
  for (size_t i = 0; i < size; ++i) {
    data[i] = uniform_real(generator->random(), 0.0f, 1.0f);
  }
  for (int64_t i = 0; i < static_cast<int64_t>(size) - 15; i += 16) {
    normal_fill_16<float>(data + i, mean, std);
  }
}
size_t frame_floats(uint32_t width) {
  if (width != 768 && width != 1536) return 0;
  const size_t height = width * 3 / 4;
  return (height / 4) * (width / 4) + (height / 2) * (width / 2);
}
void generate(uint64_t frame, uint32_t width, float *output) {
  Generator generator(frame);
  const size_t height = width * 3 / 4;
  const size_t stage2 = (height / 4) * (width / 4);
  normal_fill(output, stage2, &generator);
  normal_fill(output + stage2, (height / 2) * (width / 2), &generator);
}
struct FileOwner {
  int fd;
  ~FileOwner() { if (fd >= 0) close(fd); }
};
int append(const char *path, uint32_t width, uint64_t start, uint64_t count) {
  const size_t floats = frame_floats(width);
  const uint64_t bytes = floats * sizeof(float);
  if (!path || path[0] != '/' || !floats || count == 0 ||
      start > UINT64_MAX - count ||
      start + count > static_cast<uint64_t>(std::numeric_limits<off_t>::max()) / bytes) {
    return failure(EINVAL, "Absolute path, original width and representable positive frame range required");
  }
  const int flags = O_RDWR | O_NOFOLLOW | O_CLOEXEC | (start == 0 ? O_CREAT : 0);
  FileOwner file{open(path, flags, 0600)};
  if (file.fd < 0) return failure(errno, "Cannot open noise bank");
  if (flock(file.fd, LOCK_EX | LOCK_NB) != 0)
    return failure(errno, "Noise bank is already owned by another writer");
  struct stat state;
  if (fstat(file.fd, &state) != 0) return failure(errno, "Cannot stat noise bank");
  if (!S_ISREG(state.st_mode) || state.st_size != static_cast<off_t>(start * bytes))
    return failure(EINVAL, "Noise bank must contain exactly the caller-authenticated complete prefix");
  std::unique_ptr<float[]> data(new float[floats]);
  for (uint64_t frame = start; frame < start + count; ++frame) {
    generate(frame, width, data.get());
    const off_t offset = static_cast<off_t>(frame * bytes);
    size_t written = 0;
    while (written < bytes) {
      const ssize_t n = pwrite(file.fd,
          reinterpret_cast<const char *>(data.get()) + written,
          static_cast<size_t>(bytes) - written, offset + static_cast<off_t>(written));
      if (n < 0 && errno == EINTR) continue;
      if (n <= 0) {
        const int write_error = n < 0 ? errno : EIO;
        if (ftruncate(file.fd, offset) != 0)
          return failure(errno, "Noise append failed and partial frame truncation failed");
        return failure(write_error, "Noise append failed, previous complete frames retained");
      }
      written += static_cast<size_t>(n);
    }
  }
  if (fsync(file.fd) != 0) return failure(errno, "Noise bank flush failed");
  return 0;
}
} // namespace
extern "C" int reconstruction_official_noise_generate_frame_v1(uint64_t frame,
    uint32_t width, float *output, size_t floats) {
  last_error[0] = 0;
  if (!output || !frame_floats(width) || floats != frame_floats(width))
    return failure(EINVAL, "Exact original width and float32 frame storage required");
  generate(frame, width, output);
  return 0;
}
extern "C" int reconstruction_official_noise_append_v1(const char *path,
    uint32_t width, uint64_t start, uint64_t count) {
  last_error[0] = 0;
  try { return append(path, width, start, count); }
  catch (const std::bad_alloc &) { return failure(ENOMEM, "Noise frame allocation failed"); }
  catch (const std::exception &error) { return failure(EIO, error.what()); }
  catch (...) { return failure(EIO, "Unknown noise append failure"); }
}
extern "C" const char *reconstruction_official_noise_last_error_v1(void) {
  return last_error;
}
