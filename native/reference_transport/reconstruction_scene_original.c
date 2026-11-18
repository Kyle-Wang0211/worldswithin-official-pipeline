/* Borrowed camera bytes and path transport only; no camera/image arithmetic. */
#include <Python.h>
#include <stdio.h>
#include "reconstruction_scene_original.h"
#if PY_VERSION_HEX != 0x030A15F0
#error "Original scene camera bridge requires CPython3.10.21 final headers"
#endif
_Static_assert(sizeof(float) == 4, "raw original camera f32 ABI");
static _Thread_local char scene_error[2048];
RECONSTRUCTION_EXPORT int reconstruction_scene_camera_abi_version(void) { return RECONSTRUCTION_SCENE_CAMERA_ABI; }
RECONSTRUCTION_EXPORT const char *reconstruction_scene_cameras_last_error(void) { return scene_error; }
static void capture_error(void) {
    PyObject *type = NULL, *value = NULL, *trace = NULL;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_NormalizeException(&type, &value, &trace);
    PyObject *s = value ? PyObject_Str(value) : NULL;
    const char *text = s ? PyUnicode_AsUTF8(s) : NULL;
    snprintf(scene_error, sizeof(scene_error), "%s", text ? text : "Original camera Python IO failed");
    Py_XDECREF(s); Py_XDECREF(type); Py_XDECREF(value); Py_XDECREF(trace);
    PyErr_Clear();
}
RECONSTRUCTION_EXPORT int reconstruction_scene_cameras_prepare(const float *cameras, uint64_t count,
    const char *manifest, const char *root, const char *original,
    const char *corrected, const char *receipt) {
    scene_error[0] = 0;
    if (!cameras || (count == 0 || count % 36 || count > (uint64_t)PY_SSIZE_T_MAX / 4) || !manifest || !root || !original || !corrected ||
        !receipt || manifest[0] != '/' || root[0] != '/' || original[0] != '/' ||
        corrected[0] != '/' || receipt[0] != '/') {
        snprintf(scene_error, sizeof(scene_error), "representable actual-N camera buffer and absolute IO paths required");
        return 22;
    }
    if (!Py_IsInitialized()) {
        snprintf(scene_error, sizeof(scene_error), "existing pinned CPython runtime is not initialized");
        return 1;
    }
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *module = PyImport_ImportModule("reconstruction_scene_original_driver");
    PyObject *fn = module ? PyObject_GetAttrString(module, "prepare") : NULL;
    PyObject *raw = PyMemoryView_FromMemory((char *)cameras, (Py_ssize_t)(count * 4), PyBUF_READ);
    PyObject *a = PyUnicode_DecodeFSDefault(manifest);
    PyObject *b = PyUnicode_DecodeFSDefault(root);
    PyObject *c = PyUnicode_DecodeFSDefault(original);
    PyObject *d = PyUnicode_DecodeFSDefault(corrected);
    PyObject *e = PyUnicode_DecodeFSDefault(receipt);
    PyObject *result = NULL;
    if (fn && raw && a && b && c && d && e)
        result = PyObject_CallFunctionObjArgs(fn, raw, a, b, c, d, e, NULL);
    int status = 1;
    if (result) {
        if (!PyLong_Check(result) || PyBool_Check(result))
            PyErr_SetString(PyExc_TypeError, "original camera producer must return exact actual-N integer");
        else {
            long generated = PyLong_AsLong(result);
            if (!PyErr_Occurred()) {
                if (generated == (long)(count / 36)) status = 0;
                else PyErr_SetString(PyExc_ValueError, "original camera producer did not generate actual-N camera pairs");
            }
        }
    }
    if (status) capture_error();
    /* Python exec scopes are cleared before return; no borrowed buffer retained. */
    Py_XDECREF(result); Py_XDECREF(e); Py_XDECREF(d); Py_XDECREF(c);
    Py_XDECREF(b); Py_XDECREF(a); Py_XDECREF(raw); Py_XDECREF(fn); Py_XDECREF(module);
    PyGILState_Release(gil);
    return status;
}
