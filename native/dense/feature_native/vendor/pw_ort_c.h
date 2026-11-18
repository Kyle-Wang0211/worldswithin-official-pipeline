// Source metadata always from actual ORT session. Dense float32 ONNX order; no packed layout.
#pragma once
#include <stddef.h>
#include <stdint.h>

#define PW_ORT_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pw_ort_model pw_ort_model;

enum { PW_ORT_BACKEND_WEBGPU = 1 };
// Compatibility parameter: only precision=1 accepted, float32 models/tensors.
enum { PW_ORT_PRECISION_NORMAL = 0, PW_ORT_PRECISION_HIGH = 1, PW_ORT_PRECISION_LOW = 2 };

PW_ORT_API const char* pw_ort_version(void);

// Returns NULL on failure and writes a message into err (if err != NULL).
// WebGPU only; threads must be 0 to preserve production default.
// Call pw_ort_set_runtime with the exact Bench runtime absolute path before load.
PW_ORT_API int pw_ort_set_runtime(const char* dylib_path, char* err, int err_len);
PW_ORT_API pw_ort_model* pw_ort_load(const char* onnx_path, int backend, int precision, int threads, char* err, int err_len);
PW_ORT_API void pw_ort_destroy(pw_ort_model* m);

PW_ORT_API int pw_ort_num_inputs(const pw_ort_model* m);
PW_ORT_API int pw_ort_num_outputs(const pw_ort_model* m);
PW_ORT_API const char* pw_ort_input_name(const pw_ort_model* m, int i);
PW_ORT_API const char* pw_ort_output_name(const pw_ort_model* m, int i);

// Copies `count` floats into the named input with the given shape. Returns 0 on success.
PW_ORT_API int pw_ort_input_shape(const pw_ort_model* m, const char* name, int32_t* shape, int max_dims);
PW_ORT_API int pw_ort_set_input(pw_ort_model* m, const char* name, const float* data, const int32_t* shape, int ndim, size_t count);

// Runs the graph and waits until every output is resident on the host. *ms (optional) = wall time of
// forward + device->host sync of all outputs (inputs are uploaded inside forward). Returns 0 on success.
PW_ORT_API int pw_ort_run(pw_ort_model* m, double* ms);

// After pw_ort_run: output shape (returns ndim, -1 on error) and data copy (returns 0 on success).
PW_ORT_API int pw_ort_output_shape(const pw_ort_model* m, const char* name, int32_t* shape, int max_dims);
PW_ORT_API int pw_ort_get_output(const pw_ort_model* m, const char* name, float* dst, size_t count);

// Ends optional PWDENSE_ORT_PROFILE capture and returns the actual profile path.
PW_ORT_API int pw_ort_end_profile(pw_ort_model* m, char* buf, int len);

// JSON description: ORT version, requested WebGPU, available EPs, effective assignment unknown until profile inspected.
PW_ORT_API int pw_ort_info(const pw_ort_model* m, char* buf, int len);

PW_ORT_API const char* pw_ort_last_error(const pw_ort_model* m);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
// Explicit device path. No CPU feature allocation/readback in run_gpu/bind.
// run_gpu synchronizes outputs on WebGPU; finite-value checking requires get_output.
// Bind retains OrtValue, its source session/allocator, and Env, across source reruns/destroy.
// Calls on a model must be serialized by the caller. Source/target must share Env.
PW_ORT_API int pw_ort_run_gpu(pw_ort_model* m, double* ms);
PW_ORT_API int pw_ort_bind_feature_output(pw_ort_model* source, pw_ort_model* target, const char* source_output, const char* target_input);
// Direct official CPU->GPU copy; caller may release data when this returns.
PW_ORT_API int pw_ort_set_input_gpu(pw_ort_model* m, const char* name, const float* data, const int32_t* shape, int ndim, size_t count);
// Explicit reference RGB reuse from an uploaded source input; no second upload.
PW_ORT_API int pw_ort_bind_gpu_input(pw_ort_model* source, pw_ort_model* target, const char* source_input, const char* target_input);
// Release a target's retained input once no longer needed (including last-frame features).
PW_ORT_API int pw_ort_clear_input(pw_ort_model* m, const char* name);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
// ONNX tensor enum FLOAT=1,FLOAT16=10; -1 for invalid model/name.
PW_ORT_API int pw_ort_input_dtype(const pw_ort_model* m,const char* name);
PW_ORT_API int pw_ort_output_dtype(const pw_ort_model* m,const char* name);
// Static checked byte count; input must be1 or0. No dtype conversion.
PW_ORT_API int pw_ort_tensor_bytes(const pw_ort_model* m,int input,const char* name,size_t* bytes);
#ifdef __cplusplus
}
#endif
