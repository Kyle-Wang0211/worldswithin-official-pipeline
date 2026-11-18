                                                                     
                                                                               
                                                                               
                                                                      
                                                                                 
                                                                            
#include "reconstruction_normal_entry.h"

#include <array>
#include <charconv>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Camera { double f, cx, cy; std::array<double, 3> t; std::array<double, 9> R; };
struct Observation { int64_t camera; double x, y; };

std::string quoted(const std::string& s) {
    std::string out = "\"";
    const char hex[] = "0123456789abcdef";
    for (unsigned char c : s) {
                                                                                 
                                                                                
        if (c >= 128) throw std::runtime_error("Non-ASCII path outside selected adapter contract");
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += char(c);
    }
    return out + '"';
}

                                                                                 
                                                                                 
                                                                              
std::string pyrepr(double v) {
    if (!std::isfinite(v)) throw std::runtime_error("Non-finite value outside frozen input contract");
    if (v == 0) return std::signbit(v) ? "-0.0" : "0.0";
    char buf[128];
    auto result = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::scientific);
    if (result.ec != std::errc()) throw std::runtime_error("Float serialization failed");
    std::string value(buf, result.ptr);
    size_t epos = value.find('e');
    if (epos == std::string::npos) throw std::runtime_error("Scientific serialization failed");
    int exponent = std::stoi(value.substr(epos + 1));
    bool negative = value[0] == '-';
    std::string digits = value.substr(negative ? 1 : 0, epos - (negative ? 1 : 0));
    size_t dot = digits.find('.');
    if (dot != std::string::npos) digits.erase(dot, 1);
    std::string sign = negative ? "-" : "";
    if (exponent < -4 || exponent >= 16) {
        std::string mantissa = digits.substr(0, 1);
        if (digits.size() > 1) mantissa += '.' + digits.substr(1);
        std::string exp = std::to_string(std::abs(exponent));
        if (exp.size() < 2) exp = '0' + exp;
        return sign + mantissa + 'e' + (exponent < 0 ? '-' : '+') + exp;
    }
    int decimal = exponent + 1;
    if (decimal <= 0) return sign + "0." + std::string(-decimal, '0') + digits;
    if (decimal >= static_cast<int>(digits.size()))
        return sign + digits + std::string(decimal - digits.size(), '0') + ".0";
    return sign + digits.substr(0, decimal) + '.' + digits.substr(decimal);
}

Camera read_camera(const char* path) {
    if (!path || !path[0]) throw std::runtime_error("Missing original camera path");
    std::ifstream in(path);
    in.imbue(std::locale::classic());
    std::string first, second;
    if (!std::getline(in, first) || !std::getline(in, second))
        throw std::runtime_error("Original camera file read failed");
    std::istringstream a(first), b(second);
    a.imbue(std::locale::classic()); b.imbue(std::locale::classic());
    Camera c{};
    for (double& t : c.t) if (!(a >> t)) throw std::runtime_error("Camera translation parse failed");
    for (double& r : c.R) if (!(a >> r)) throw std::runtime_error("Camera rotation parse failed");
    std::array<double, 6> p{};
    for (double& x : p) if (!(b >> x)) throw std::runtime_error("Camera intrinsic parse failed");
    c.f = p[0] * 4032.0; c.cx = p[4] * 4032.0; c.cy = p[5] * 3024.0;
    return c;
}

double row_dot3(const double* row, const double* vertex) {
                                                           
                                                                             
                                                                            
    return (vertex[0] * row[0] + vertex[1] * row[1]) + vertex[2] * row[2];
}

template<class T> void raw(std::ofstream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    if (!out) throw std::runtime_error("Byte-gate artifact write failed");
}

std::ofstream binary(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Byte-gate artifact open failed");
    return out;
}
}              

extern "C" int reconstruction_av_prepare_sfm_json(const double* vertices,
                                          const uint8_t* colors,
                                          uint64_t vertex_count,
                                          const RECONSTRUCTIONNormalFrameInput* frames,
                                          uint32_t frame_count,
                                          const char* images_folder,
                                          const char* sfm_path,
                                          const char* gate_folder) {
    try {
        if (!vertices || !frames || vertex_count == 0 || frame_count != 97 ||
            !images_folder || !sfm_path || !gate_folder ||
            std::fegetround() != FE_TONEAREST ||
            vertex_count > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) return 1;
        const uint16_t endian = 1;
        if (*reinterpret_cast<const uint8_t*>(&endian) != 1) return 1;
        std::vector<Camera> cameras;
        std::vector<std::vector<Observation>> observations(vertex_count);
        cameras.reserve(frame_count);
        for (uint32_t i = 0; i < frame_count; ++i) {
            const auto& frame = frames[i];
            if (!frame.width || !frame.height || !frame.fused_d16_le_path) return 1;
            uint64_t count = uint64_t(frame.width) * frame.height;
            if (count > std::numeric_limits<size_t>::max() / sizeof(uint16_t)) return 1;
            std::ifstream in(frame.fused_d16_le_path, std::ios::binary | std::ios::ate);
            if (!in || in.tellg() != static_cast<std::streamoff>(count * sizeof(uint16_t))) return 1;
            std::vector<uint16_t> depth(count);
            in.seekg(0);
            in.read(reinterpret_cast<char*>(depth.data()), count * sizeof(uint16_t));
            if (!in) return 1;
            cameras.push_back(read_camera(frame.original_camera_path));
            const Camera& c = cameras.back();
            for (uint64_t p = 0; p < vertex_count; ++p) {
                const double* vtx = vertices + 3 * p;
                double X[3];
                for (int r = 0; r < 3; ++r)
                    X[r] = row_dot3(frame.extr_row_major + 4 * r, vtx) + frame.extr_row_major[4 * r + 3];
                const double zc = X[2];
                if (!(zc > 0.05)) continue;
                double uf = std::nearbyint(frame.intr_row_major[0] * X[0] / zc + frame.intr_row_major[2]);
                double vf = std::nearbyint(frame.intr_row_major[4] * X[1] / zc + frame.intr_row_major[5]);
                if (!std::isfinite(uf) || !std::isfinite(vf)) return 1;
                if (uf < 0 || vf < 0 || uf >= frame.width || vf >= frame.height) continue;
                size_t u = static_cast<size_t>(uf), y = static_cast<size_t>(vf);
                const double dd = static_cast<double>(depth[y * frame.width + u]) / 5000.0;
                if (!(dd > 0 && std::abs(zc - dd) < 0.01 * dd)) continue;
                double Xh[3];
                for (int r = 0; r < 3; ++r) Xh[r] = row_dot3(c.R.data() + 3 * r, vtx) + c.t[r];
                observations[p].push_back({i, c.f * Xh[0] / Xh[2] + c.cx,
                                             c.f * Xh[1] / Xh[2] + c.cy});
            }
        }
        std::filesystem::create_directories(gate_folder);
        const std::filesystem::path gate(gate_folder);
        auto out_v = binary(gate / "V.f64le"), out_col = binary(gate / "col.u8");
        auto out_vi = binary(gate / "vi.i64le"), out_cj = binary(gate / "cj.i64le");
        auto out_ox = binary(gate / "ox.f64le"), out_oy = binary(gate / "oy.f64le");
        auto out_cams = binary(gate / "cams.f64le");
        for (const auto& c : cameras) {
            raw(out_cams, c.f); raw(out_cams, c.cx); raw(out_cams, c.cy);
            for (double x : c.t) raw(out_cams, x);
            for (double x : c.R) raw(out_cams, x);
        }
        for (uint64_t p = 0; p < vertex_count; ++p) {
            for (int r = 0; r < 3; ++r) {
                raw(out_v, vertices[3 * p + r]);
                uint8_t color = colors ? colors[3 * p + r] : 200;
                raw(out_col, color);
            }
            for (const auto& observation : observations[p]) {
                const int64_t vertex = static_cast<int64_t>(p);
                raw(out_vi, vertex); raw(out_cj, observation.camera);
                raw(out_ox, observation.x); raw(out_oy, observation.y);
            }
        }
                                                                                  
                                                                                      
        std::ofstream sfm(sfm_path);
        sfm.imbue(std::locale::classic());
        if (!sfm) return 1;
        sfm << "{\"version\": [\"1\", \"2\", \"12\"], \"featuresFolders\": \"\", \"matchesFolders\": \"\", \"views\": [";
        for (uint32_t i = 0; i < frame_count; ++i) {
            if (i) sfm << ", ";
            std::string id = std::to_string(i);
            sfm << "{\"viewId\": " << quoted(id) << ", \"poseId\": " << quoted(id)
                << ", \"intrinsicId\": " << quoted(id) << ", \"path\": "
                << quoted(std::string(images_folder) + "/" + id + ".png")
                << ", \"width\": \"4032\", \"height\": \"3024\", \"metadata\": \"\"}";
        }
        sfm << "], \"intrinsics\": [";
        for (uint32_t i = 0; i < frame_count; ++i) {
            if (i) sfm << ", ";
            const auto& c = cameras[i];
            sfm << "{\"intrinsicId\": " << quoted(std::to_string(i))
                << ", \"width\": \"4032\", \"height\": \"3024\", \"sensorWidth\": \"36\", \"sensorHeight\": \"27\", \"serialNumber\": \"\", \"type\": \"pinhole\", \"initializationMode\": \"calibrated\", \"initialFocalLength\": \"-1\", \"focalLength\": "
                << quoted(pyrepr(c.f * 36.0 / 4032.0))
                << ", \"pixelRatio\": \"1\", \"pixelRatioLocked\": \"true\", \"offsetLocked\": \"false\", \"scaleLocked\": \"false\", \"principalPoint\": ["
                << quoted(pyrepr(c.cx - 4032.0 / 2.0)) << ", " << quoted(pyrepr(c.cy - 3024.0 / 2.0))
                << "], \"distortionInitializationMode\": \"none\", \"distortionParams\": \"\", \"undistortionOffset\": [\"0\", \"0\"], \"undistortionParams\": \"\", \"distortionType\": \"none\", \"undistortionType\": \"none\", \"locked\": \"true\"}";
        }
        sfm << "], \"poses\": [";
        for (uint32_t i = 0; i < frame_count; ++i) {
            if (i) sfm << ", ";
            const auto& c = cameras[i];
            sfm << "{\"poseId\": " << quoted(std::to_string(i)) << ", \"pose\": {\"transform\": {\"rotation\": [";
            for (int r = 0; r < 3; ++r) for (int col = 0; col < 3; ++col) {
                if (r || col) sfm << ", ";
                sfm << quoted(pyrepr(c.R[3 * col + r]));                     
            }
            sfm << "], \"center\": [";
            for (int r = 0; r < 3; ++r) {
                if (r) sfm << ", ";
                                                                              
                                                                       
                const double center = (((-c.R[r]) * c.t[0] + (-c.R[3 + r]) * c.t[1]) + (-c.R[6 + r]) * c.t[2]);
                sfm << quoted(pyrepr(center));
            }
            sfm << "]}, \"locked\": \"true\", \"rotationOnly\": \"false\", \"removable\": \"true\"}}";
        }
        sfm << "], \"structure\": [";
        bool first = true;
        for (uint64_t p = 0; p < vertex_count; ++p) {
            if (observations[p].empty()) continue;
            if (!first) sfm << ", "; first = false;
            std::string id = std::to_string(p);
            sfm << "{\"landmarkId\": " << quoted(id) << ", \"descType\": \"unknown\", \"color\": [";
            for (int r = 0; r < 3; ++r) { if (r) sfm << ", "; sfm << quoted(std::to_string(colors ? colors[3 * p + r] : 200)); }
            sfm << "], \"X\": [";
            for (int r = 0; r < 3; ++r) { if (r) sfm << ", "; sfm << quoted(pyrepr(vertices[3 * p + r])); }
            sfm << "], \"observations\": [";
            for (size_t q = 0; q < observations[p].size(); ++q) {
                if (q) sfm << ", ";
                const auto& o = observations[p][q];
                sfm << "{\"observationId\": " << quoted(std::to_string(o.camera))
                    << ", \"featureId\": " << quoted(id) << ", \"x\": [" << quoted(pyrepr(o.x))
                    << ", " << quoted(pyrepr(o.y)) << "], \"scale\": \"0\"}";
            }
            sfm << "]}";
        }
        sfm << "]}";
        sfm.close();
        if (!sfm) return 1;
        for (std::ofstream* stream : {&out_v, &out_col, &out_vi, &out_cj, &out_ox, &out_oy, &out_cams}) {
            stream->close();
            if (!*stream) return 1;
        }
        return 0;
    } catch (...) { return 1; }
}
