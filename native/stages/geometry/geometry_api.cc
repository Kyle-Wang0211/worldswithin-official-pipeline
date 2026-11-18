#include "geometry_api.h"
#include <lagrange/SurfaceMesh.h>
#include <lagrange/io/load_mesh_ply.h>
#include <lagrange/io/save_mesh_ply.h>
#include <lagrange/mesh_cleanup/close_small_holes.h>
#include <lagrange/mesh_cleanup/resolve_nonmanifoldness.h>
#include <array>
#include <exception>
#include <filesystem>
#include <string>

// The frozen original file is compiled with main=reconstruction_original_meshopt_main only.
int reconstruction_original_meshopt_main(int argc, char** argv);
namespace {
thread_local std::string last_error;
bool valid_raw_file(const char* path, uintmax_t stride) {
    if (!path || !*path) return false;
    std::error_code ec;
    auto n = std::filesystem::file_size(path, ec);
    return !ec && n > 0 && n % stride == 0;
}
}
extern "C" const char* reconstruction_geometry_last_error(void) { return last_error.c_str(); }
extern "C" int reconstruction_meshopt_simplify_files(const char* pos, const char* idx,
                                          const char* out_pos, const char* out_idx) {
    last_error.clear();
    if (!valid_raw_file(pos, 3 * sizeof(float)) ||
        !valid_raw_file(idx, 3 * sizeof(uint32_t)) ||
        !out_pos || !*out_pos || !out_idx || !*out_idx) {
        last_error = "Invalid original meshoptimizer file arguments";
        return 1;
    }
    try {
        std::array<std::string, 6> args = {
            "mo_simplify", pos, idx, "100000", out_pos, out_idx};
        std::array<char*, 6> argv;
        for (size_t i = 0; i < args.size(); ++i) argv[i] = args[i].data();
        return reconstruction_original_meshopt_main(static_cast<int>(argv.size()), argv.data());
    } catch (const std::exception& e) { last_error = e.what(); return 2; }
      catch (...) { last_error = "Unknown original meshoptimizer exception"; return 3; }
}
extern "C" int reconstruction_lagrange_fill_ply(const char* input, const char* output,
                                      uint64_t* iv, uint64_t* iff,
                                      uint64_t* ov, uint64_t* of) {
    last_error.clear();
    if (!input || !*input || !output || !*output) {
        last_error = "Invalid Lagrange PLY path"; return 1;
    }
    try {
        // Python SurfaceMesh bindings use double, uint32_t. Its .ply dispatch calls
        // these same official load/save routines. All LoadOptions stay at defaults.
        using Mesh = lagrange::SurfaceMesh<double, uint32_t>;
        auto mesh = lagrange::io::load_mesh_ply<Mesh>(lagrange::fs::path(input));
        if (iv) *iv = mesh.get_num_vertices();
        if (iff) *iff = mesh.get_num_facets();
        lagrange::resolve_nonmanifoldness(mesh);
        lagrange::close_small_holes(mesh); // Official defaults: size=16, triangulate=true.
        if (ov) *ov = mesh.get_num_vertices();
        if (of) *of = mesh.get_num_facets();
        lagrange::io::SaveOptions opts;
        opts.encoding = lagrange::io::FileEncoding::Binary;
        opts.attribute_conversion_policy =
            lagrange::io::SaveOptions::AttributeConversionPolicy::ExactMatchOnly;
        opts.output_attributes = lagrange::io::SaveOptions::OutputAttributes::All;
        lagrange::io::save_mesh_ply(lagrange::fs::path(output), mesh, opts);
        return 0;
    } catch (const std::exception& e) { last_error = e.what(); return 2; }
      catch (...) { last_error = "Unknown official Lagrange exception"; return 3; }
}
