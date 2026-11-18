                                                                          
                                                                                
                                                                               
#pragma once
#if defined(PWDENSE_ENABLE_FUSED_DEPTH_RETENTION)
#include "dense_fuse.h"
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace aether::dense {
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559, "IEEE754 binary64 required");
struct FusedDepthRetention {
    std::string dir;
                                                                            
    std::vector<int> original_indices;
    std::vector<double> frame_ids;
};

                                                                                 
                                                                                 
                                                                                
                                                                              
inline bool fused_depth_quantize(double d_avg, bool final_mask, uint16_t& out) {
    if (!final_mask) { out = 0; return true; }
    if (std::fegetround() != FE_TONEAREST) return false;
    const double scaled = d_avg * 5000.0;
    if (!std::isfinite(scaled)) return false;
    const double rounded = std::nearbyint(scaled);
    if (rounded < -2147483648.0 || rounded >= 2147483648.0) return false;
    out = static_cast<uint16_t>(static_cast<int32_t>(rounded));
    return true;
}

namespace fused_depth_io_detail {
inline bool write_all(int fd, const void* data, size_t size) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    while (size) {
        const ssize_t n = ::write(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        p += n; size -= static_cast<size_t>(n);
    }
    return true;
}
struct File {
    int fd = -1;
    File() = default;
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File() { if (fd >= 0) ::close(fd); }
    bool open_new(const std::string& path) {
        fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        return fd >= 0;
    }
    bool finish() {
        if (fd < 0) return false;
        const bool synced = ::fsync(fd) == 0;
        const int old = fd; fd = -1;
        const bool closed = ::close(old) == 0;
        return synced && closed;
    }
};
inline uint64_t bits(double x) { uint64_t v; std::memcpy(&v, &x, sizeof v); return v; }
inline bool write_f64_le(int fd, double x) {
    const uint64_t v = bits(x); uint8_t b[8];
    for (unsigned k = 0; k < 8; ++k) b[k] = static_cast<uint8_t>(v >> (8 * k));
    return write_all(fd, b, sizeof b);
}
inline bool text(int fd, const char* s) { return write_all(fd, s, std::strlen(s)); }
inline bool valid(const FusedDepthRetention& r, size_t count) {
    if (r.dir.empty() || !count || r.original_indices.size() != count || r.frame_ids.size() != count) return false;
    for (size_t i = 0; i < count; ++i) {
        if (r.original_indices[i] < 0 || !std::isfinite(r.frame_ids[i])) return false;
        for (size_t j = 0; j < i; ++j)
            if (r.original_indices[j] == r.original_indices[i] || r.frame_ids[j] == r.frame_ids[i]) return false;
    }
    return true;
}
}                                   

                                                                              
                                                                             
inline bool fused_depth_retention_begin(const FusedDepthRetention& r, size_t count) {
    return fused_depth_io_detail::valid(r, count) && std::fegetround() == FE_TONEAREST && ::mkdir(r.dir.c_str(), 0755) == 0;
}

inline bool fused_depth_retention_write(const FusedDepthRetention& r, int pack_index,
                                       int W, int H, const FuseCamera& camera, const FrameFusion& fr) {
    using namespace fused_depth_io_detail;
    if (pack_index < 0 || static_cast<size_t>(pack_index) >= r.original_indices.size() ||
        r.frame_ids.size() != r.original_indices.size() || W <= 0 || H <= 0 ||
        fr.final_mask.size() != static_cast<size_t>(W) * static_cast<size_t>(H) ||
        fr.d_avg.size() != fr.final_mask.size() || std::fegetround() != FE_TONEAREST) return false;
    char name[32];
    const int len = std::snprintf(name, sizeof name, "%08d", r.original_indices[pack_index]);
    if (len < 0 || static_cast<size_t>(len) >= sizeof name) return false;
    const std::string base = r.dir + "/" + name;
    File d16;
    if (!d16.open_new(base + ".d16")) return false;
    uint8_t buf[8192]; size_t used = 0;
    for (size_t i = 0; i < fr.d_avg.size(); ++i) {
        uint16_t value;
        if (!fused_depth_quantize(fr.d_avg[i], fr.final_mask[i] != 0, value)) return false;
        buf[used++] = static_cast<uint8_t>(value);
        buf[used++] = static_cast<uint8_t>(value >> 8);
        if (used == sizeof buf) { if (!write_all(d16.fd, buf, used)) return false; used = 0; }
    }
    if ((used && !write_all(d16.fd, buf, used)) || !d16.finish()) return false;
    File cam;
    if (!cam.open_new(base + ".camera.f64le")) return false;
                                                                                         
    for (int i = 0; i < 9; ++i) if (!write_f64_le(cam.fd, camera.K[i])) return false;
    for (int i = 0; i < 16; ++i) if (!write_f64_le(cam.fd, camera.E[i])) return false;
    if (!cam.finish()) return false;
    char entry[1024];
    const int n = std::snprintf(entry, sizeof entry,
        "{\"schema\":1,\"pack_index\":%d,\"original_index\":%d,\"frame_id_f64_bits\":\"%016llx\","
        "\"width\":%d,\"height\":%d,\"depth_scale\":5000,\"d16\":\"%s.d16\",\"d16_bytes\":%zu,"
        "\"camera\":\"%s.camera.f64le\",\"camera_bytes\":200,\"camera_layout\":\"K9_then_E16_row_major_f64le\"}\n",
        pack_index, r.original_indices[pack_index], static_cast<unsigned long long>(bits(r.frame_ids[pack_index])),
        W, H, name, fr.d_avg.size() * 2, name);
    if (n < 0 || static_cast<size_t>(n) >= sizeof entry) return false;
    File meta;
    return meta.open_new(base + ".json") && write_all(meta.fd, entry, static_cast<size_t>(n)) && meta.finish();
}

                                                                           
                                                                               
inline bool fused_depth_retention_finish(const FusedDepthRetention& r, int W, int H) {
    using namespace fused_depth_io_detail;
    if (!valid(r, r.original_indices.size()) || W <= 0 || H <= 0) return false;
                                                                                  
    for (int original : r.original_indices) {
        char base[32];
        const int n = std::snprintf(base, sizeof base, "%08d", original);
        if (n < 0 || static_cast<size_t>(n) >= sizeof base) return false;
        struct stat s;
        if (::stat((r.dir + "/" + base + ".d16").c_str(), &s) != 0 || !S_ISREG(s.st_mode) ||
            s.st_size < 0 || static_cast<uint64_t>(s.st_size) != static_cast<uint64_t>(W) * H * 2) return false;
        if (::stat((r.dir + "/" + base + ".camera.f64le").c_str(), &s) != 0 || !S_ISREG(s.st_mode) || s.st_size != 200) return false;
        if (::stat((r.dir + "/" + base + ".json").c_str(), &s) != 0 || !S_ISREG(s.st_mode) || s.st_size <= 0) return false;
    }
    File order;
    const std::string partial = r.dir + "/order.json.partial";
    if (!order.open_new(partial)) return false;
    char row[512];
    int n = std::snprintf(row, sizeof row, "{\"schema\":1,\"status\":\"complete\",\"width\":%d,\"height\":%d,\"depth_scale\":5000,\"entries\":[", W, H);
    if (n < 0 || static_cast<size_t>(n) >= sizeof row || !write_all(order.fd, row, static_cast<size_t>(n))) return false;
    for (size_t i = 0; i < r.original_indices.size(); ++i) {
        char base[32];
        n = std::snprintf(base, sizeof base, "%08d", r.original_indices[i]);
        if (n < 0 || static_cast<size_t>(n) >= sizeof base) return false;
        n = std::snprintf(row, sizeof row, "%s{\"pack_index\":%zu,\"original_index\":%d,\"frame_id_f64_bits\":\"%016llx\",\"metadata\":\"%s.json\"}",
            i ? "," : "", i, r.original_indices[i], static_cast<unsigned long long>(bits(r.frame_ids[i])), base);
        if (n < 0 || static_cast<size_t>(n) >= sizeof row || !write_all(order.fd, row, static_cast<size_t>(n))) return false;
    }
    if (!text(order.fd, "]}\n") || !order.finish()) return false;
                                                                                     
                                                                                    
    return ::link(partial.c_str(), (r.dir + "/order.json").c_str()) == 0;
}
}                           
#endif
