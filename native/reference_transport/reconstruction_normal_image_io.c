/* IO glue only; reuse process-owned CPython and documented GIL/error APIs. */
#include <Python.h>
#include <stdio.h>
#include "reconstruction_normal_image_io.h"
#if PY_VERSION_HEX != 0x030A15F0
#error "Selected normal image IO requires CPython3.10.21 final headers"
#endif
static _Thread_local char image_io_error[2048];
RECONSTRUCTION_EXPORT const char *reconstruction_normal_images_last_error(void) { return image_io_error; }
static void capture_image_io_error(void) {
    PyObject *type = NULL, *value = NULL, *trace = NULL;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_NormalizeException(&type, &value, &trace);
    PyObject *s = value ? PyObject_Str(value) : NULL;
    const char *text = s ? PyUnicode_AsUTF8(s) : NULL;
    snprintf(image_io_error, sizeof(image_io_error), "%s",
             text ? text : "Python normal image IO failed");
    Py_XDECREF(s); Py_XDECREF(type); Py_XDECREF(value); Py_XDECREF(trace);
    PyErr_Clear();
}
RECONSTRUCTION_EXPORT int reconstruction_normal_images_prepare(const char *manifest,
                                          const char *output,
                                          const char *receipt) {
    image_io_error[0] = 0;
    if (!manifest || !output || !receipt || manifest[0] != '/' ||
        output[0] != '/' || receipt[0] != '/') {
        snprintf(image_io_error, sizeof(image_io_error), "Absolute normal image IO paths required");
        return 22;
    }
    if (!Py_IsInitialized()) {
        snprintf(image_io_error, sizeof(image_io_error), "pinned CPython runtime is not initialized");
        return 1;
    }
    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *module = PyImport_ImportModule("reconstruction_normal_image_io");
    PyObject *fn = module ? PyObject_GetAttrString(module, "prepare_v2") : NULL;
    PyObject *a = PyUnicode_DecodeFSDefault(manifest);
    PyObject *b = PyUnicode_DecodeFSDefault(output);
    PyObject *c = PyUnicode_DecodeFSDefault(receipt);
    PyObject *expected_fn = module ? PyObject_GetAttrString(module, "manifest_frame_count") : NULL;
    PyObject *expected = expected_fn && a ? PyObject_CallOneArg(expected_fn, a) : NULL;
    PyObject *result = NULL;
    long expected_count = 0;
    if (expected) {
        if (!PyLong_CheckExact(expected))
            PyErr_SetString(PyExc_TypeError, "validated manifest must bind exact actual-N integer");
        else {
            expected_count = PyLong_AsLong(expected);
            if (!PyErr_Occurred() && expected_count <= 0)
                PyErr_SetString(PyExc_ValueError, "validated manifest count must be positive");
            if (!PyErr_Occurred() && fn && a && b && c)
                result = PyObject_CallFunctionObjArgs(fn, a, b, c, NULL);
        }
    }
    int status = 1;
    if (result) {
        if (!PyLong_Check(result) || PyBool_Check(result))
            PyErr_SetString(PyExc_TypeError, "normal image IO result must be exact actual-N integer");
        else {
            long count = PyLong_AsLong(result);
            if (!PyErr_Occurred()) {
                if (count == expected_count) status = 0;
                else PyErr_SetString(PyExc_ValueError, "normal image IO returned a count different from the validated manifest");
            }
        }
    }
    if (status) capture_image_io_error();
    Py_XDECREF(result); Py_XDECREF(a); Py_XDECREF(b); Py_XDECREF(c);
    Py_XDECREF(expected); Py_XDECREF(expected_fn);
    Py_XDECREF(fn); Py_XDECREF(module);
    PyGILState_Release(gil);
    return status;
}
