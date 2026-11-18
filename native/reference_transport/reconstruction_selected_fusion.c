/* Synchronous transport. BoxFilter classifies geometry and NumPy scripts project and fuse it. */
#include <Python.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "reconstruction_selected_fusion.h"
#include "reconstruction_selection_core.h"

#ifdef __cplusplus
static thread_local char selected_error[512];
#else
static _Thread_local char selected_error[512];
#endif
typedef struct { double box[15]; } SelectedMaskContext;
static const char *selected_capsule_name = "reconstruction.selected_box15.v1";

static void selected_context_free(PyObject *capsule) {
    void *pointer = PyCapsule_GetPointer(capsule, selected_capsule_name);
    if (pointer) PyMem_Free(pointer);
}

static PyObject *selected_mask_call(PyObject *capsule, PyObject *points) {
    SelectedMaskContext *context = (SelectedMaskContext *)PyCapsule_GetPointer(capsule, selected_capsule_name);
    Py_buffer buffer = {0};
    PyObject *result = NULL;
    uint64_t count = 0;
    char error[512] = {0};
    int status = 1;
    if (!context) return NULL;
    if (!PyMemoryView_Check(points)) {
        PyErr_SetString(PyExc_TypeError, "read-only contiguous world-f4 byte memoryview required");
        return NULL;
    }
    if (PyObject_GetBuffer(points, &buffer, PyBUF_FULL_RO) != 0) return NULL;
    if (!buffer.readonly || buffer.ndim != 1 || buffer.itemsize != 1 ||
        !buffer.format || strcmp(buffer.format, "B") != 0 ||
        !PyBuffer_IsContiguous(&buffer, 'C') || buffer.len <= 0 || buffer.len % 12 ||
        buffer.len / 12 > 1536 * 1152) {
        PyErr_SetString(PyExc_ValueError, "invalid read-only world-f4 byte layout/count");
        goto done;
    }
    count = (uint64_t)buffer.len / 12;
    result = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)count);
    if (!result) goto done;
    status = reconstruction_selection_contains_f32((const float *)buffer.buf, count,
        context->box, (uint8_t *)PyBytes_AS_STRING(result), count, error, sizeof error);
    if (status) {
        PyErr_SetString(PyExc_ValueError, error[0] ? error : "original selection predicate rejected input");
        Py_CLEAR(result);
    }
done:
    PyBuffer_Release(&buffer);
    return result;
}

static PyObject *selected_mask_callback_create(const double *box15) {
    SelectedMaskContext *context = NULL;
    PyObject *capsule = NULL, *callback = NULL;
    static PyMethodDef method = {"_selected_mask_v1", selected_mask_call, METH_O,
        "Classify immutable official projected f4 rows using the original177 predicate."};
    if (!box15) { PyErr_SetString(PyExc_ValueError, "box15 required"); return NULL; }
    for (int i = 0; i < 15; ++i) {
        if (!isfinite(box15[i]) || (i >= 3 && i < 6 && box15[i] <= 0)) {
            PyErr_SetString(PyExc_ValueError, "finite box15 and positive full side lengths required");
            return NULL;
        }
    }
    context = (SelectedMaskContext *)PyMem_Malloc(sizeof *context);
    if (!context) return PyErr_NoMemory();
    memcpy(context->box, box15, sizeof context->box);
    capsule = PyCapsule_New(context, selected_capsule_name, selected_context_free);
    if (!capsule) { PyMem_Free(context); return NULL; }
    callback = PyCFunction_NewEx(&method, capsule, NULL);
    Py_DECREF(capsule);
    return callback;
}

typedef struct {
    reconstruction_selected_chunk_fn callback;
    void *user;
    uint32_t frame_count;
} SelectedLiveContext;
static const char *selected_live_capsule_name = "reconstruction.selected_live.v2";

static void selected_live_context_free(PyObject *capsule) {
    void *pointer = PyCapsule_GetPointer(capsule, selected_live_capsule_name);
    if (pointer) PyMem_Free(pointer);
}

static int selected_live_buffer_valid(const Py_buffer *buffer, Py_ssize_t stride) {
    uintptr_t address = (uintptr_t)buffer->buf;
    return buffer->readonly && buffer->ndim == 1 && buffer->itemsize == 1 &&
        buffer->format && !strcmp(buffer->format, "B") &&
        PyBuffer_IsContiguous(buffer, 'C') && buffer->len >= 0 &&
        buffer->len % stride == 0 && buffer->len / stride <= 1536 * 1152 &&
        (!buffer->len || (buffer->buf &&
            (uint64_t)buffer->len <= (uint64_t)(UINTPTR_MAX - address)));
}

static PyObject *selected_live_call(PyObject *capsule, PyObject *args) {
    SelectedLiveContext *context = (SelectedLiveContext *)PyCapsule_GetPointer(
        capsule, selected_live_capsule_name);
    PyObject *index_object = NULL, *points = NULL, *colors = NULL;
    Py_buffer point_buffer = {0}, color_buffer = {0};
    PyObject *result = NULL;
    long index;
    int32_t callback_status;
    if (!context) return NULL;
    if (!PyArg_UnpackTuple(args, "_selected_live_v2", 3, 3,
                          &index_object, &points, &colors)) return NULL;
    if (!PyLong_CheckExact(index_object)) {
        PyErr_SetString(PyExc_TypeError, "selected frame index must be an exact integer");
        return NULL;
    }
    index = PyLong_AsLong(index_object);
    if (PyErr_Occurred()) return NULL;
    if (index < 0 || (uint64_t)index >= context->frame_count) {
        PyErr_SetString(PyExc_ValueError, "selected frame index out of range");
        return NULL;
    }
    if (!PyMemoryView_Check(points) || !PyMemoryView_Check(colors)) {
        PyErr_SetString(PyExc_TypeError, "read-only contiguous selected point/RGB byte memoryviews required");
        return NULL;
    }
    if (PyObject_GetBuffer(points, &point_buffer, PyBUF_FULL_RO)) return NULL;
    if (PyObject_GetBuffer(colors, &color_buffer, PyBUF_FULL_RO)) goto done;
    if (!selected_live_buffer_valid(&point_buffer, 12) ||
        !selected_live_buffer_valid(&color_buffer, 3) ||
        point_buffer.len / 12 != color_buffer.len / 3 ||
        (point_buffer.len && (uintptr_t)point_buffer.buf % sizeof(float))) {
        PyErr_SetString(PyExc_ValueError, "invalid selected read-only f4/RGB byte layout/count/alignment");
        goto done;
    }
    callback_status = context->callback((int32_t)index,
        (const float *)point_buffer.buf, (const uint8_t *)color_buffer.buf,
        (int32_t)(point_buffer.len / 12), context->user);
    if (callback_status && !PyErr_Occurred()) {
        PyErr_Format(PyExc_RuntimeError, "selected live callback failed/cancelled with status %d",
                     (int)callback_status);
    }
    if (!PyErr_Occurred()) { result = Py_None; Py_INCREF(result); }
done:
    if (color_buffer.obj) PyBuffer_Release(&color_buffer);
    PyBuffer_Release(&point_buffer);
    return result;
}

static PyObject *selected_live_callback_create(reconstruction_selected_chunk_fn callback,
                                              void *user, uint32_t frame_count) {
    SelectedLiveContext *context;
    PyObject *capsule, *result;
    static PyMethodDef method = {"_selected_live_v2", selected_live_call, METH_VARARGS,
        "Deliver borrowed selected original prequantization f4/RGB rows synchronously."};
    if (!callback || !frame_count || frame_count > 97) {
        PyErr_SetString(PyExc_ValueError, "selected live callback and valid frame count required");
        return NULL;
    }
    context = (SelectedLiveContext *)PyMem_Malloc(sizeof *context);
    if (!context) return PyErr_NoMemory();
    context->callback = callback; context->user = user; context->frame_count = frame_count;
    capsule = PyCapsule_New(context, selected_live_capsule_name, selected_live_context_free);
    if (!capsule) { PyMem_Free(context); return NULL; }
    result = PyCFunction_NewEx(&method, capsule, NULL);
    Py_DECREF(capsule);
    return result;
}

static PyObject *selected_view(const void *pointer, uint64_t bytes) {
    if (!pointer || bytes > (uint64_t)PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_ValueError, "invalid selected raw view");
        return NULL;
    }
    return PyMemoryView_FromMemory((char *)pointer, (Py_ssize_t)bytes, PyBUF_READ);
}

static void selected_capture_error(void) {
    PyObject *type = NULL, *value = NULL, *trace = NULL, *message = NULL;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_NormalizeException(&type, &value, &trace);
    message = value ? PyObject_Str(value) : NULL;
    const char *text = message ? PyUnicode_AsUTF8(message) : NULL;
    snprintf(selected_error, sizeof selected_error, "%s", text ? text : "selected transport failed");
    Py_XDECREF(message); Py_XDECREF(trace); Py_XDECREF(value); Py_XDECREF(type);
    PyErr_Clear();
}

static int selected_absolute(const char *path) {
    if (!path || path[0] != '/') return 0;
    for (size_t i = 1; i < 16384; ++i) if (!path[i]) return 1;
    return 0;
}

const char *reconstruction_selected_fusion_last_error(void) { return selected_error; }

int reconstruction_original_fuse_pack_selected_v2(const RECONSTRUCTIONOriginalFusionPack *pack,
    const char *original_directory, const char *selected_directory,
    const double *box15, const uint32_t *original_indices, const char *plan_sha,
    uint64_t *original_kept_total, uint64_t *selected_kept_total,
    reconstruction_selected_chunk_fn callback, void *user) {
    PyObject *module = NULL, *fn = NULL, *result = NULL, *a[14] = {0};
    PyGILState_STATE gil;
    uint64_t original_kept = 0, selected_kept = 0;
    int status = 1;
    selected_error[0] = 0;
    if (!pack || !box15 || !original_indices || !original_kept_total || !selected_kept_total ||
        original_kept_total == selected_kept_total || !pack->frame_count || pack->frame_count > 97 ||
        pack->width != 1536 || pack->height != 1152 || !pack->cams_f32 || !pack->frame_ids_f64 ||
        !pack->original_jpeg_paths || !selected_absolute(original_directory) ||
        !selected_absolute(selected_directory) || !strcmp(original_directory, selected_directory) || !plan_sha) {
        snprintf(selected_error, sizeof selected_error, "invalid selected fusion input contract");
        return 1;
    }
    for (int i = 0; i < 64; ++i) if (!((plan_sha[i] >= '0' && plan_sha[i] <= '9') ||
        (plan_sha[i] >= 'a' && plan_sha[i] <= 'f'))) {
        snprintf(selected_error, sizeof selected_error, "exact lowercase selection-plan SHA256 required"); return 1;
    }
    if (plan_sha[64]) { snprintf(selected_error, sizeof selected_error, "invalid selection-plan hash length"); return 1; }
    for (int i = 0; i < 4; ++i) if (!pack->banks[i]) {
        snprintf(selected_error, sizeof selected_error, "missing original selected fusion bank"); return 1;
    }
    for (uint32_t i = 0; i < pack->frame_count; ++i) {
        if (original_indices[i] >= 97 || (i && original_indices[i] <= original_indices[i-1]) ||
            !selected_absolute(pack->original_jpeg_paths[i])) {
            snprintf(selected_error, sizeof selected_error, "invalid original frame/JPEG mapping"); return 1;
        }
    }
    if (!Py_IsInitialized()) { snprintf(selected_error, sizeof selected_error, "existing CPython owner required"); return 1; }
    gil = PyGILState_Ensure();
    module = PyImport_ImportModule("reconstruction_selected_fusion");
    fn = module ? PyObject_GetAttrString(module, "fuse_pack_selected") : NULL;
    a[0] = PyTuple_New(4);
    a[1] = selected_view(pack->cams_f32, (uint64_t)pack->frame_count * 36 * 4);
    a[2] = Py_None; Py_INCREF(a[2]);
    a[3] = PyLong_FromUnsignedLong(pack->rgb_width);
    a[4] = PyLong_FromUnsignedLong(pack->rgb_height);
    a[5] = selected_view(pack->frame_ids_f64, (uint64_t)pack->frame_count * 8);
    a[6] = PyUnicode_DecodeFSDefault(original_directory);
    a[7] = Py_None; Py_INCREF(a[7]);
    a[8] = PyTuple_New(pack->frame_count);
    a[9] = PyUnicode_DecodeFSDefault(selected_directory);
    a[10] = selected_mask_callback_create(box15);
    a[11] = selected_view(original_indices, (uint64_t)pack->frame_count * 4);
    a[12] = PyUnicode_FromString(plan_sha);
    if (callback) a[13] = selected_live_callback_create(callback, user, pack->frame_count);
    else { a[13] = Py_None; Py_INCREF(a[13]); }
    if (!fn) goto done;
    for (int i = 0; i < 14; ++i) if (!a[i]) goto done;
    for (int i = 0; i < 4; ++i) {
        PyObject *item = selected_view(pack->banks[i], (uint64_t)pack->frame_count * 1152 * 1536 * 4);
        if (!item) goto done;
        PyTuple_SET_ITEM(a[0], i, item);
    }
    for (uint32_t i = 0; i < pack->frame_count; ++i) {
        PyObject *item = PyUnicode_DecodeFSDefault(pack->original_jpeg_paths[i]);
        if (!item) goto done;
        PyTuple_SET_ITEM(a[8], i, item);
    }
    result = PyObject_CallFunctionObjArgs(fn, a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11],a[12],a[13],NULL);
    if (!result) goto done;
    if (!PyTuple_CheckExact(result) || PyTuple_GET_SIZE(result) != 2 ||
        !PyLong_CheckExact(PyTuple_GET_ITEM(result,0)) || !PyLong_CheckExact(PyTuple_GET_ITEM(result,1))) {
        PyErr_SetString(PyExc_TypeError, "separate original and selected integer counts required"); goto done;
    }
    original_kept = PyLong_AsUnsignedLongLong(PyTuple_GET_ITEM(result,0));
    selected_kept = PyLong_AsUnsignedLongLong(PyTuple_GET_ITEM(result,1));
    if (PyErr_Occurred()) goto done;
    if (selected_kept > original_kept || original_kept > (uint64_t)pack->frame_count * 1536 * 1152) {
        PyErr_SetString(PyExc_ValueError, "selected count cannot replace or exceed original fusion count"); goto done;
    }
    *original_kept_total = original_kept; *selected_kept_total = selected_kept;
    status = 0;
done:
    if (status) selected_capture_error();
    Py_XDECREF(result);
    for (int i = 13; i >= 0; --i) Py_XDECREF(a[i]);
    Py_XDECREF(fn); Py_XDECREF(module); PyGILState_Release(gil);
    return status;
}

int reconstruction_original_fuse_pack_selected_v1(const RECONSTRUCTIONOriginalFusionPack *pack,
    const char *original_directory, const char *selected_directory,
    const double *box15, const uint32_t *original_indices, const char *plan_sha,
    uint64_t *original_kept_total, uint64_t *selected_kept_total) {
    return reconstruction_original_fuse_pack_selected_v2(pack, original_directory, selected_directory,
        box15, original_indices, plan_sha, original_kept_total, selected_kept_total, NULL, NULL);
}
