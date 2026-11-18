// ABI adapters only. Numerical integration and extraction use unchanged official source.
#include "reconstruction_tsdf.h"
#include "VDBVolume.h"
#include <exception>
#include <fstream>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

struct reconstruction_vdb_volume {
    // Constructor casts match vdb_volume.py's np.float32 conversions.
    vdbfusion::VDBVolume upstream{static_cast<float>(0.003),
                                static_cast<float>(0.04), false};
};
struct reconstruction_vdb_mesh {
    std::vector<Eigen::Vector3d> vertices;
    std::vector<Eigen::Vector3i> faces;
};
static_assert(sizeof(Eigen::Vector3d) == 3 * sizeof(double));
static_assert(sizeof(Eigen::Vector3i) == 3 * sizeof(int32_t));
static_assert(sizeof(int) == sizeof(int32_t));
namespace {
thread_local std::string error;
template<class F> int checked(F f) {
    try { error.clear(); f(); return 0; }
    catch (const std::exception& e) { error = e.what(); return 1; }
    catch (...) { error = "unknown upstream exception"; return 1; }
}
}
extern "C" int reconstruction_vdb_create(reconstruction_vdb_volume** out) {
    if (!out) { error = "null output"; return 1; }
    *out = nullptr;
    return checked([&] { *out = new reconstruction_vdb_volume(); });
}
extern "C" void reconstruction_vdb_destroy(reconstruction_vdb_volume* volume) { delete volume; }
extern "C" int reconstruction_vdb_integrate_xyz(reconstruction_vdb_volume* volume, const double* xyz,
                                       size_t count, const double origin[3]) {
    if (!volume || !origin || (count && !xyz) ||
        count > std::numeric_limits<size_t>::max() / (3 * sizeof(double))) {
        error = "invalid XYZ ABI input"; return 1;
    }
    return checked([&] {
        // The original pybind _VectorEigen3d copies rows to the same vector type.
        std::vector<Eigen::Vector3d> points;
        points.reserve(count);
        for (size_t i = 0; i < count; ++i)
            points.emplace_back(xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
        const Eigen::Vector3d sensor(origin[0], origin[1], origin[2]);
        // Exact original default overload from vdbfusion_pybind.cpp:59-64.
        volume->upstream.Integrate(points, sensor, [](float /*sdf*/) { return 1.0f; });
    });
}
extern "C" int reconstruction_vdb_integrate_d16_file(reconstruction_vdb_volume* volume, const char* path,
                                           const reconstruction_tsdf_frame* frame,
                                           size_t* integrated_points) {
    if (integrated_points) *integrated_points = 0;
    if (!volume || !path || !frame || !frame->width || !frame->height ||
        frame->width > std::numeric_limits<size_t>::max() / frame->height ||
        frame->width * frame->height > std::numeric_limits<size_t>::max() / 2) {
        error = "invalid per-view d16 ABI input"; return 1;
    }
    return checked([&] {
        const size_t pixels = frame->width * frame->height;
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input || input.tellg() != static_cast<std::streamoff>(pixels * 2))
            throw std::runtime_error("d16 stream length mismatch");
        input.seekg(0);
        std::vector<uint8_t> bytes(pixels * 2);
        if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
            throw std::runtime_error("d16 stream read failed");
        auto depth16 = [&](size_t i) -> uint16_t {
            return static_cast<uint16_t>(bytes[2*i]) |
                   static_cast<uint16_t>(static_cast<uint16_t>(bytes[2*i+1]) << 8);
        };
        size_t count = 0;
        for (size_t i = 0; i < pixels; ++i) count += depth16(i) > 0 ? 1 : 0;
        std::vector<Eigen::Vector3d> points;
        points.reserve(count);
        const double* K = frame->intrinsics;
        const double* E = frame->extrinsics;
        const double t[3] = {E[3], E[7], E[11]};
        // Select TSDF inputs in C order, matching np.nonzero.
        // This arithmetic translation has NOT passed the NumPy/OpenBLAS phone byte gate.
        for (size_t i = 0; i < pixels; ++i) {
            const uint16_t d16 = depth16(i);
            if (!d16) continue;
            const double dd = static_cast<double>(d16) / 5000.0;
            const double u = static_cast<double>(i % frame->width);
            const double v = static_cast<double>(i / frame->width);
            const double Xc[3] = {(u - K[2]) / K[0] * dd,
                                  (v - K[5]) / K[4] * dd, dd};
            const double centered[3] = {Xc[0] - t[0], Xc[1] - t[1], Xc[2] - t[2]};
            Eigen::Vector3d world;
            for (size_t j = 0; j < 3; ++j) {
                const double p0 = centered[0] * E[j];
                const double p1 = centered[1] * E[4+j];
                const double p2 = centered[2] * E[8+j];
                world[static_cast<int>(j)] = (p0 + p1) + p2;
            }
            points.push_back(world);
        }
        Eigen::Vector3d origin;
        for (size_t j = 0; j < 3; ++j) {
            // Python parses -R.T @ t as (-R.T) @ t.
            const double p0 = (-E[j]) * t[0];
            const double p1 = (-E[4+j]) * t[1];
            const double p2 = (-E[8+j]) * t[2];
            origin[static_cast<int>(j)] = (p0 + p1) + p2;
        }
        volume->upstream.Integrate(points, origin, [](float /*sdf*/) { return 1.0f; });
        if (integrated_points) *integrated_points = points.size();
    });
}
extern "C" int reconstruction_vdb_extract_raw(reconstruction_vdb_volume* volume, reconstruction_vdb_mesh** out) {
    if (!volume || !out) { error = "invalid extraction ABI input"; return 1; }
    *out = nullptr;
    return checked([&] {
        auto result = volume->upstream.ExtractTriangleMesh(true, static_cast<float>(0.45));
        auto mesh = std::make_unique<reconstruction_vdb_mesh>();
        mesh->vertices = std::move(std::get<0>(result));
        mesh->faces = std::move(std::get<1>(result));
        *out = mesh.release();
    });
}
extern "C" void reconstruction_vdb_mesh_destroy(reconstruction_vdb_mesh* mesh) { delete mesh; }
extern "C" size_t reconstruction_vdb_mesh_vertex_count(const reconstruction_vdb_mesh* mesh) {
    return mesh ? mesh->vertices.size() : 0;
}
extern "C" size_t reconstruction_vdb_mesh_face_count(const reconstruction_vdb_mesh* mesh) {
    return mesh ? mesh->faces.size() : 0;
}
extern "C" const double* reconstruction_vdb_mesh_vertices(const reconstruction_vdb_mesh* mesh) {
    return mesh && !mesh->vertices.empty() ? mesh->vertices.front().data() : nullptr;
}
extern "C" const int32_t* reconstruction_vdb_mesh_faces(const reconstruction_vdb_mesh* mesh) {
    return mesh && !mesh->faces.empty() ? mesh->faces.front().data() : nullptr;
}
extern "C" const char* reconstruction_tsdf_last_error(void) { return error.c_str(); }
