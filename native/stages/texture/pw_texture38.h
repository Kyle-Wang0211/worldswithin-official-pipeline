#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Original selected texrecon main, fixed recipe; caller supplies isolated IO paths.
 * Original upstream std::exit failure paths still terminate the owning test app.
 * The controller must record process exit and preserve original-stage failures.
 */
__attribute__((visibility("default")))
int pw_texture38_run(const char *scene_directory, const char *mesh_ply,
                     const char *output_prefix, char *error, size_t error_capacity);

#ifdef __cplusplus
}
#endif
