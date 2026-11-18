#include "pw_texture38.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

// This is the unmodified selected apps/texrecon/texrecon.cpp, compiled with
// -Dmain=pw_texture38_upstream_main for that translation unit only.
int pw_texture38_upstream_main(int argc, char **argv);

namespace {
std::mutex stage_mutex;
void write_error(char *error, size_t capacity, const char *message) {
    if (!error || capacity == 0) return;
    const size_t length = std::min(capacity - 1, std::strlen(message));
    std::memcpy(error, message, length);
    error[length] = '\0';
}
bool absolute_nonempty(const char *path) { return path && path[0] == '/' && path[1]; }
}

int pw_texture38_run(const char *scene_directory, const char *mesh_ply,
                     const char *output_prefix, char *error, size_t error_capacity) {
    if (error && error_capacity) error[0] = '\0';
    if (!absolute_nonempty(scene_directory) || !absolute_nonempty(mesh_ply)
        || !absolute_nonempty(output_prefix)) {
        write_error(error, error_capacity, "scene, mesh and output prefix must be absolute nonempty paths");
        return -1;
    }
    try {
        std::lock_guard<std::mutex> single_owner(stage_mutex);
        std::vector<std::string> arguments = {
            "texrecon38", scene_directory, mesh_ply, output_prefix,
            "--data_term=gmi", "--outlier_removal=gauss_clamping",
            "--num_threads=1", "--no_intermediate_results"
        };
        std::vector<char *> argv;
        argv.reserve(arguments.size() + 1);
        for (std::string &argument : arguments) argv.push_back(&argument[0]);
        argv.push_back(nullptr);
        return pw_texture38_upstream_main(static_cast<int>(arguments.size()), argv.data());
    } catch (const std::exception &exception) {
        write_error(error, error_capacity, exception.what());
        return -2;
    } catch (...) {
        write_error(error, error_capacity, "unhandled original texrecon exception");
        return -3;
    }
}
