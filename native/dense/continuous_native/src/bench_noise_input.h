#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// Storage-only bench option. Float bytes and the per-job frame layout are
// unchanged; mapping lifetime must cover dense_run and every external pointer.
class BenchNoiseInput {
public:
    BenchNoiseInput() = default;
    ~BenchNoiseInput() { close(); }
    BenchNoiseInput(const BenchNoiseInput&) = delete;
    BenchNoiseInput& operator=(const BenchNoiseInput&) = delete;

    static bool expected_bytes(size_t frames, size_t width, size_t height,
                               size_t* bytes, std::string* error);
    bool open(const std::string& path, size_t expected_bytes, std::string* error);
    const float* data() const { return static_cast<const float*>(mapping_); }
    size_t bytes() const { return bytes_; }
    void close() noexcept;

private:
    void* mapping_ = nullptr;
    size_t bytes_ = 0;
};

inline bool BenchNoiseInput::expected_bytes(size_t frames, size_t width, size_t height,
                                           size_t* bytes, std::string* error) {
    static_assert(sizeof(float) == 4, "noise files contain 4-byte floats");
    if (error) error->clear();
    if (bytes) *bytes = 0;
    const auto fail = [&](const char* message) { if (error) *error = message; return false; };
    if (!bytes || !frames || width < 4 || height < 4 || width % 4 || height % 4) return fail("invalid noise dimensions");
    const size_t limit = std::numeric_limits<size_t>::max();
    if (width / 4 > limit / (height / 4) || width / 2 > limit / (height / 2)) return fail("noise size overflow");
    const size_t n2 = (height / 4) * (width / 4), n3 = (height / 2) * (width / 2);
    if (n2 > limit - n3 || frames > limit / (n2 + n3) || frames * (n2 + n3) > limit / sizeof(float)) return fail("noise size overflow");
    *bytes = frames * (n2 + n3) * sizeof(float);
    return true;
}
inline bool BenchNoiseInput::open(const std::string& path, size_t expected, std::string* error) {
    close();
    if (error) error->clear();
    const auto fail = [&](const char* message) { if (error) *error = message; return false; };
    if (!expected || expected % sizeof(float)) return fail("noise expected size must be nonzero whole floats");
    // O_NONBLOCK prevents a nonregular FIFO from blocking before fstat rejects it.
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) return fail("noise file open failed");
    struct stat info;
    if (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size <= 0 ||
        static_cast<uintmax_t>(info.st_size) != expected || info.st_size % sizeof(float)) {
        ::close(fd); return fail("noise file must be regular with exact nonzero float byte size");
    }
    void* mapped = ::mmap(nullptr, expected, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (mapped == MAP_FAILED) return fail("noise mmap failed");
    mapping_ = mapped;
    bytes_ = expected;
    return true;
}
inline void BenchNoiseInput::close() noexcept {
    if (mapping_) ::munmap(mapping_, bytes_);
    mapping_ = nullptr; bytes_ = 0;
}

// Storage-only frozen-noise reader. Descriptors are borrowed by DenseJob; owners
// outlive dense_run and its joined prefetch workers. No mapping or generated fallback.
class BenchNoiseFileInput {
public:
    BenchNoiseFileInput() = default;
    ~BenchNoiseFileInput() { if (fd_ >= 0) ::close(fd_); }
    BenchNoiseFileInput(const BenchNoiseFileInput&) = delete;
    BenchNoiseFileInput& operator=(const BenchNoiseFileInput&) = delete;
    int fd() const { return fd_; }
    static bool validate_fd(int fd, size_t expected, std::string* error) {
        struct stat info{};
        if (fd < 0 || !expected || expected % sizeof(float) ||
            expected > static_cast<uintmax_t>(std::numeric_limits<off_t>::max()) ||
            ::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) ||
            info.st_size <= 0 || static_cast<uintmax_t>(info.st_size) != expected) {
            if (error) *error = "noise file must be regular with exact nonzero float byte size";
            return false;
        }
        return true;
    }
    bool open(const std::string& path, size_t expected, std::string* error) {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
        if (error) error->clear();
        fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        if (fd_ < 0) { if (error) *error = "noise file open failed"; return false; }
        if (!validate_fd(fd_, expected, error)) { ::close(fd_); fd_ = -1; return false; }
        return true;
    }
    static bool read_frame(int fd, size_t frame, size_t frames, size_t n2, size_t n3,
                           std::vector<float>& noise2, std::vector<float>& noise3,
                           std::string* error) {
        const size_t limit = std::numeric_limits<size_t>::max();
        if (error) error->clear();
        if (fd < 0 || !frames || frame >= frames || !n2 || !n3 ||
            n2 > limit - n3 || frames > limit / (n2 + n3) ||
            frames * (n2 + n3) > limit / sizeof(float) ||
            frames * (n2 + n3) * sizeof(float) >
                static_cast<uintmax_t>(std::numeric_limits<off_t>::max())) {
            if (error) *error = "invalid frozen noise row range";
            return false;
        }
        noise2.resize(n2); noise3.resize(n3);
        off_t offset = static_cast<off_t>(frame * (n2 + n3) * sizeof(float));
        auto read_all = [&](void* dst, size_t bytes) {
            auto* p = static_cast<unsigned char*>(dst);
            while (bytes) {
                const ssize_t k = ::pread(fd, p, bytes, offset);
                if (k < 0 && errno == EINTR) continue;
                if (k <= 0) { if (error) *error = "frozen noise row read failed or short"; return false; }
                p += k; bytes -= static_cast<size_t>(k); offset += k;
            }
            return true;
        };
        return read_all(noise2.data(), n2 * sizeof(float)) &&
               read_all(noise3.data(), n3 * sizeof(float));
    }
private:
    int fd_ = -1;
};
