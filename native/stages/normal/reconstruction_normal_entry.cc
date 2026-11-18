// Invocation adapters only. All conversion, mesh/image I/O, visibility remapping,
// tangent transforms, ray queries, rasterization and PNG writing stay in the
// unmodified AliceVision 3.3.0 official source and its pinned dependencies.
#include "reconstruction_normal_entry.h"

#include <string>
#include <vector>

int reconstruction_av_convert_wrapped_main(int argc, char** argv);
int reconstruction_av_texturing_wrapped_main(int argc, char** argv);

namespace {
int invoke(int (*entry)(int, char**), std::vector<std::string> args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args) argv.push_back(arg.data());
    return entry(static_cast<int>(argv.size()), argv.data());
}

bool valid_path(const char* value) { return value && value[0]; }
}  // namespace

extern "C" int reconstruction_av_convert_sfm_to_abc(const char* sfm_json_path,
                                           const char* abc_path) {
    if (!valid_path(sfm_json_path) || !valid_path(abc_path)) return 1;
    // Preserve the selected Alembic export arguments.
    return invoke(reconstruction_av_convert_wrapped_main,
                  {"aliceVision_convertSfMFormat", "-i", sfm_json_path,
                   "-o", abc_path, "--describerTypes", "unknown"});
}

extern "C" int reconstruction_av_bake_normal(const char* abc_path, const char* low_obj_path,
                                    const char* high_ply_path,
                                    const char* images_folder,
                                    const char* output_folder) {
    if (!valid_path(abc_path) || !valid_path(low_obj_path) ||
        !valid_path(high_ply_path) || !valid_path(images_folder) ||
        !valid_path(output_folder)) return 1;
    // Defaults not named here are the selected official 3.3.0 CLI defaults.
    // In particular colorMappingFileType remains NONE, as in the original run.
    return invoke(reconstruction_av_texturing_wrapped_main,
                  {"aliceVision_texturing", "-i", abc_path,
                   "--inputMesh", low_obj_path, "--inputRefMesh", high_ply_path,
                   "--imagesFolder", images_folder, "-o", output_folder,
                   "--textureSide", "2048", "--downscale", "1",
                   "--outputMeshFileType", "obj", "--bumpType", "Normal",
                   "--normalFileType", "png", "--subdivisionTargetRatio", "0",
                   "--useUDIM", "0", "--maxCoresAvailable", "1"});
}
