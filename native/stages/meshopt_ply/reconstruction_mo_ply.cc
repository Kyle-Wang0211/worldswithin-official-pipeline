                                                                           
                                                                                
                                                                                 
#include "reconstruction_mo_ply.h"
#include "reconstruction_open3d.h"
#include "geometry_api.h"
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
thread_local std::string error;
using MeshOwner = std::unique_ptr<reconstruction_open3d_mesh, decltype(&reconstruction_open3d_mesh_destroy)>;
void require(int rc, const char* detail) {
    if (rc) throw std::runtime_error(detail && *detail ? detail : "original upstream call failed");
}
template<class T> void write_raw(const char* path, const std::vector<T>& values) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("raw output open failed");
    if (values.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()) / sizeof(T))
        throw std::runtime_error("raw output exceeds IO size");
    stream.write(reinterpret_cast<const char*>(values.data()),
                 static_cast<std::streamsize>(values.size() * sizeof(T)));
    stream.close();
    if (!stream) throw std::runtime_error("raw output write failed");
}
template<class T> std::vector<T> read_raw(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("raw input open failed");
    const auto size = stream.tellg();
    if (size < 0 || static_cast<uint64_t>(size) % (3 * sizeof(T)) != 0)
        throw std::runtime_error("raw input is not a row-major Nx3 array");
    if (static_cast<uint64_t>(size) > std::numeric_limits<size_t>::max() ||
        static_cast<uint64_t>(size) > static_cast<uint64_t>(std::numeric_limits<std::streamsize>::max()))
        throw std::runtime_error("raw input exceeds IO size");
    std::vector<T> values(static_cast<size_t>(size) / sizeof(T));
    stream.seekg(0);
    stream.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(size));
    if (!stream) throw std::runtime_error("raw input read failed");
    return values;
}
}
extern "C" int reconstruction_meshopt_simplify_ply(const char* input, const char* output,
    const char* pos, const char* idx, const char* out_pos, const char* out_idx,
    uint64_t* iv, uint64_t* iff, uint64_t* ov, uint64_t* of) {
    if (iv) *iv = 0; if (iff) *iff = 0; if (ov) *ov = 0; if (of) *of = 0;
    try {
        error.clear();
        const char* paths[] = {input, output, pos, idx, out_pos, out_idx};
        for (size_t i = 0; i < 6; ++i) {
            if (!paths[i] || !*paths[i]) throw std::runtime_error("missing original-route path");
            for (size_t j = 0; j < i; ++j)
                if (std::strcmp(paths[i], paths[j]) == 0) throw std::runtime_error("paths must be distinct");
        }
        static_assert(sizeof(float) == 4 && sizeof(double) == 8 && sizeof(uint32_t) == 4 && sizeof(int32_t) == 4);
        const uint32_t one = 1;
        if (*reinterpret_cast<const uint8_t*>(&one) != 1)
            throw std::runtime_error("selected original raw route requires little endian");
        reconstruction_open3d_mesh* incoming = nullptr;
        int rc = reconstruction_open3d_read_ply(input, &incoming);
        if (rc) require(rc, reconstruction_open3d_last_error());
        MeshOwner source(incoming, reconstruction_open3d_mesh_destroy);
        const size_t nv = reconstruction_open3d_vertex_count(source.get());
        const size_t nf = reconstruction_open3d_face_count(source.get());
        if (nv > std::numeric_limits<size_t>::max()/3 || nf > std::numeric_limits<size_t>::max()/3)
            throw std::runtime_error("original Nx3 arrays exceed size_t");
        const double* vertices = reconstruction_open3d_vertices(source.get());
        const int32_t* faces = reconstruction_open3d_faces(source.get());
        if ((nv && !vertices) || (nf && !faces)) throw std::runtime_error("original mesh storage is unavailable");
                                                                   
                                                                         
        std::vector<float> positions(nv*3);
        std::vector<uint32_t> indices(nf*3);
        for (size_t i=0; i<positions.size(); ++i) positions[i] = static_cast<float>(vertices[i]);
        for (size_t i=0; i<indices.size(); ++i) indices[i] = static_cast<uint32_t>(faces[i]);
        write_raw(pos, positions); write_raw(idx, indices);
        if (iv) *iv = nv; if (iff) *iff = nf;
                                                                                
        rc = reconstruction_meshopt_simplify_files(pos, idx, out_pos, out_idx);
        if (rc) require(rc, reconstruction_geometry_last_error());
        const auto simplified_positions = read_raw<float>(out_pos);
        const auto simplified_indices = read_raw<uint32_t>(out_idx);
        std::vector<double> output_xyz(simplified_positions.size());
        std::vector<int32_t> output_faces(simplified_indices.size());
                                                                       
        for (size_t i=0; i<output_xyz.size(); ++i) output_xyz[i] = static_cast<double>(simplified_positions[i]);
                                                                                      
        for (size_t i=0; i<output_faces.size(); ++i)
            std::memcpy(&output_faces[i], &simplified_indices[i], sizeof(int32_t));
        reconstruction_open3d_mesh* outgoing = nullptr;
        rc = reconstruction_open3d_from_raw(output_xyz.data(), output_xyz.size()/3,
            output_faces.data(), output_faces.size()/3, &outgoing);
        if (rc) require(rc, reconstruction_open3d_last_error());
        MeshOwner result(outgoing, reconstruction_open3d_mesh_destroy);
                                                                                  
        rc = reconstruction_open3d_write_ply(output, result.get());
        if (rc) require(rc, reconstruction_open3d_last_error());
        if (ov) *ov = output_xyz.size()/3; if (of) *of = output_faces.size()/3;
        return 0;
    } catch (const std::exception& e) { error = e.what(); return 1; }
      catch (...) { error = "unknown original-route bridge failure"; return 1; }
}
extern "C" const char* reconstruction_mo_ply_last_error(void) { return error.c_str(); }
