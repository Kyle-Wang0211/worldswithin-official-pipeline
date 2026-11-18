/* Typed synchronous IO glue only. Numerical/JSON source lives in pinned Python. */
#include <Python.h>
#include <stdio.h>
#include "reconstruction_av_original_numpy.h"
#if PY_VERSION_HEX != 0x030A15F0
#error "Original visibility bridge requires CPython 3.10.21 final headers"
#endif
_Static_assert(sizeof(double) == 8 && sizeof(uint16_t) == 2, "raw visibility ABI");
static _Thread_local char error[2048];
RECONSTRUCTION_AV_NUMPY_EXPORT const char* reconstruction_av_original_numpy_last_error(void) { return error; }
static void capture_error(void) {
    PyObject *type = NULL, *value = NULL, *trace = NULL;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_NormalizeException(&type, &value, &trace);
    PyObject *s = value ? PyObject_Str(value) : NULL;
    const char *text = s ? PyUnicode_AsUTF8(s) : NULL;
    snprintf(error, sizeof(error), "%s", text ? text : "Original Python visibility/SfM failed");
    Py_XDECREF(s); Py_XDECREF(type); Py_XDECREF(value); Py_XDECREF(trace);
    PyErr_Clear();
}
static PyObject* frame_tuple(const RECONSTRUCTIONOriginalVisibilityFrame* frame) {
    PyObject *path = PyUnicode_DecodeFSDefault(frame->fused_d16_le_path);
    PyObject *width = PyLong_FromUnsignedLong(frame->width);
    PyObject *height = PyLong_FromUnsignedLong(frame->height);
    PyObject *k = PyMemoryView_FromMemory((char*)frame->intr_row_major, 72, PyBUF_READ);
    PyObject *e = PyMemoryView_FromMemory((char*)frame->extr_row_major, 128, PyBUF_READ);
    PyObject *camera = PyUnicode_DecodeFSDefault(frame->original_camera_path);
    PyObject *result = NULL;
    if (path && width && height && k && e && camera)
        result = PyTuple_Pack(6, path, width, height, k, e, camera);
    Py_XDECREF(path); Py_XDECREF(width); Py_XDECREF(height);
    Py_XDECREF(k); Py_XDECREF(e); Py_XDECREF(camera);
    return result;
}
RECONSTRUCTION_AV_NUMPY_EXPORT int reconstruction_av_prepare_sfm_original_numpy(
    const double* vertices, const double* colors, uint64_t vertex_count,
    int32_t has_colors, const RECONSTRUCTIONOriginalVisibilityFrame* frames,
    uint32_t frame_count, const char* images, const char* sfm, const char* gate) {
    error[0] = 0;
    if (!vertices || !vertex_count || vertex_count > (uint64_t)PY_SSIZE_T_MAX / 24 ||
        (has_colors != 0 && has_colors != 1) || (has_colors && !colors) ||
        !frames || frame_count == 0 || !images || !sfm || !gate ||
        !images[0] || !sfm[0] || !gate[0]) {
        snprintf(error, sizeof(error), "invalid original visibility buffers/paths"); return 22;
    }
    for (uint32_t i = 0; i < frame_count; ++i) {
        const RECONSTRUCTIONOriginalVisibilityFrame* frame = &frames[i];
        if (!frame->width || !frame->height || !frame->fused_d16_le_path ||
            !frame->fused_d16_le_path[0] || !frame->original_camera_path ||
            !frame->original_camera_path[0] ||
            (uint64_t)frame->width * frame->height > (uint64_t)PY_SSIZE_T_MAX / 2) {
            snprintf(error, sizeof(error), "invalid original visibility frame %u", i); return 22;
        }
    }
    if (!Py_IsInitialized()) {
        snprintf(error, sizeof(error), "pinned CPython runtime is not initialized"); return 1;
    }
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *module = PyImport_ImportModule("reconstruction_av_original_numpy_driver");
    PyObject *fn = module ? PyObject_GetAttrString(module, "prepare") : NULL;
    PyObject *v = PyMemoryView_FromMemory((char*)vertices,
        (Py_ssize_t)(vertex_count * 24), PyBUF_READ);
    PyObject *c = has_colors ? PyMemoryView_FromMemory((char*)colors,
        (Py_ssize_t)(vertex_count * 24), PyBUF_READ) : Py_NewRef(Py_None);
    PyObject *count = PyLong_FromUnsignedLongLong(vertex_count);
    PyObject *table = PyTuple_New(frame_count);
    PyObject *img = PyUnicode_DecodeFSDefault(images);
    PyObject *out = PyUnicode_DecodeFSDefault(sfm);
    PyObject *folder = PyUnicode_DecodeFSDefault(gate);
    PyObject *result = NULL;
    int status = 1;
    if (!module || !fn || !v || !c || !count || !table || !img || !out || !folder)
        goto failed;
    for (uint32_t i = 0; i < frame_count; ++i) {
        PyObject *frame = frame_tuple(&frames[i]);
        if (!frame) goto failed;
        PyTuple_SET_ITEM(table, i, frame); /* table owns the frame and views */
    }
    /* Keep table, memoryviews, their caller-owned buffers and the GIL live for
     * the entire synchronous Python call. No ctypes or foreign interpreter. */
    result = PyObject_CallFunctionObjArgs(fn, v, c, count, table, img, out, folder, NULL);
    if (!result) goto failed;
    status = 0;
    goto done;
failed:
    capture_error();
done:
    Py_XDECREF(result); Py_XDECREF(folder); Py_XDECREF(out); Py_XDECREF(img);
    Py_XDECREF(table); Py_XDECREF(count); Py_XDECREF(c); Py_XDECREF(v);
    Py_XDECREF(fn); Py_XDECREF(module);
    PyGILState_Release(gil);
    return status;
}
