// Selected official Open3D methods are called unchanged; no component-filter implementation.
#include "reconstruction_open3d.h"
#include "open3d/geometry/TriangleMesh.h"
#include "open3d/io/TriangleMeshIO.h"
#include <exception>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <cmath>
struct reconstruction_open3d_mesh { open3d::geometry::TriangleMesh upstream; };
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
void read(const char* path, open3d::geometry::TriangleMesh& mesh) {
    if (!path || !open3d::io::ReadTriangleMeshFromPLY(path, mesh, {}))
        throw std::runtime_error("official Open3D PLY reader failed");
}
void write(const char* path, const open3d::geometry::TriangleMesh& mesh) {
    // Original defaults: binary, no compression, normals/colors/UV enabled if present,
    // print_progress=false. PLY reader's enable_post_processing is unused officially.
    if (!path || !open3d::io::WriteTriangleMeshToPLY(path, mesh, false, false,
                                                   true, true, true, false))
        throw std::runtime_error("official Open3D PLY writer failed");
}
}
extern "C" int reconstruction_open3d_read_ply(const char* input, reconstruction_open3d_mesh** out) {
    if (!out) { error = "null output"; return 1; }
    *out = nullptr;
    return checked([&] {
        auto mesh = std::make_unique<reconstruction_open3d_mesh>();
        read(input, mesh->upstream);
        *out = mesh.release();
    });
}
extern "C" int reconstruction_open3d_from_raw(const double* xyz, size_t nv,
                                     const int32_t* faces, size_t nf,
                                     reconstruction_open3d_mesh** out) {
    if (!out || (nv && !xyz) || (nf && !faces) ||
        nv > std::numeric_limits<size_t>::max() / (3 * sizeof(double)) ||
        nf > std::numeric_limits<size_t>::max() / (3 * sizeof(int32_t))) {
        error = "invalid raw geometry ABI input"; return 1;
    }
    *out = nullptr;
    return checked([&] {
        auto mesh = std::make_unique<reconstruction_open3d_mesh>();
        mesh->upstream.vertices_.reserve(nv);
        mesh->upstream.triangles_.reserve(nf);
        for (size_t i = 0; i < nv; ++i)
            mesh->upstream.vertices_.emplace_back(xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
        for (size_t i = 0; i < nf; ++i)
            mesh->upstream.triangles_.emplace_back(faces[3*i], faces[3*i+1], faces[3*i+2]);
        *out = mesh.release();
    });
}
extern "C" void reconstruction_open3d_mesh_destroy(reconstruction_open3d_mesh* mesh) { delete mesh; }
extern "C" size_t reconstruction_open3d_vertex_count(const reconstruction_open3d_mesh* mesh) {
    return mesh ? mesh->upstream.vertices_.size() : 0;
}
extern "C" size_t reconstruction_open3d_face_count(const reconstruction_open3d_mesh* mesh) {
    return mesh ? mesh->upstream.triangles_.size() : 0;
}
extern "C" const double* reconstruction_open3d_vertices(const reconstruction_open3d_mesh* mesh) {
    return mesh && !mesh->upstream.vertices_.empty()
        ? mesh->upstream.vertices_.front().data() : nullptr;
}
extern "C" const int32_t* reconstruction_open3d_faces(const reconstruction_open3d_mesh* mesh) {
    return mesh && !mesh->upstream.triangles_.empty()
        ? mesh->upstream.triangles_.front().data() : nullptr;
}
extern "C" int reconstruction_open3d_has_vertex_colors(const reconstruction_open3d_mesh* mesh) {
    return mesh && mesh->upstream.HasVertexColors() ? 1 : 0;
}
extern "C" const double* reconstruction_open3d_vertex_colors(const reconstruction_open3d_mesh* mesh) {
    return mesh && mesh->upstream.HasVertexColors()
        ? mesh->upstream.vertex_colors_.front().data() : nullptr;
}
extern "C" int reconstruction_open3d_av_vis_colors(const reconstruction_open3d_mesh* mesh,
                                          uint8_t* rgb, size_t capacity,
                                          size_t* required, int* has_colors) {
    if (required) *required = 0;
    if (has_colors) *has_colors = 0;
    if (!mesh || mesh->upstream.vertices_.size() > std::numeric_limits<size_t>::max() / 3) {
        error = "invalid av_vis color ABI input"; return 1;
    }
    const size_t bytes = mesh->upstream.vertices_.size() * 3;
    const bool colors = mesh->upstream.HasVertexColors();
    if (required) *required = bytes;
    if (has_colors) *has_colors = colors ? 1 : 0;
    if (!rgb && !capacity) { error.clear(); return 0; }
    if ((!rgb && bytes) || capacity < bytes) {
        error = "av_vis color output capacity mismatch"; return 1;
    }
    return checked([&] {
        // original_source/av_vis_k.py:29: multiply FP64 colors, then uint8 cast.
        // The exact original no-color fallback is 200.
        for (size_t i = 0; i < mesh->upstream.vertices_.size(); ++i)
            for (size_t j = 0; j < 3; ++j) {
                if (!colors) { rgb[3*i+j] = 200; continue; }
                const double color = mesh->upstream.vertex_colors_[i][static_cast<int>(j)];
                if (!std::isfinite(color) || color < 0.0 || color > 1.0)
                    throw std::runtime_error("color is outside the original selected byte-RGB invariant");
                const double scaled = color * 255.0;
                // The original NumPy uint8 cast truncates; it does not round.
                rgb[3*i+j] = static_cast<uint8_t>(scaled);
            }
    });
}
extern "C" int reconstruction_open3d_write_ply(const char* output, const reconstruction_open3d_mesh* mesh) {
    if (!mesh) { error = "null mesh"; return 1; }
    return checked([&] { write(output, mesh->upstream); });
}
extern "C" int reconstruction_open3d_clean_ply(const char* input, const char* output,
                                       reconstruction_open3d_clean_stats* stats) {
    if (stats) *stats = {};
    return checked([&] {
        open3d::geometry::TriangleMesh original;
        read(input, original);
        auto clusters = original.ClusterConnectedTriangles();
        const auto& labels = std::get<0>(clusters);
        const auto& counts = std::get<1>(clusters);
        // Preserve copy.deepcopy(mesh), including original vertex/face attributes.
        open3d::geometry::TriangleMesh mesh = original;
        std::vector<bool> remove(labels.size());
        size_t removed = 0, kept = 0;
        for (size_t i = 0; i < labels.size(); ++i) {
            remove[i] = counts.at(static_cast<size_t>(labels[i])) < 100;
            removed += remove[i] ? 1 : 0;
        }
        for (size_t count : counts) kept += count >= 100 ? 1 : 0;
        mesh.RemoveTrianglesByMask(remove);
        mesh.RemoveUnreferencedVertices();
        write(output, mesh);
        if (stats) *stats = {counts.size(), kept, labels.size(), removed,
                            mesh.vertices_.size(), mesh.triangles_.size()};
    });
}
extern "C" int reconstruction_open3d_rewrite_ply(const char* input, const char* output) {
    return checked([&] {
        open3d::geometry::TriangleMesh mesh;
        read(input, mesh);
        write(output, mesh);
    });
}
extern "C" const char* reconstruction_open3d_last_error(void) { return error.c_str(); }
