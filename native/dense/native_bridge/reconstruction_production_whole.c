#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "reconstruction_production_whole.h"
#ifndef RECONSTRUCTION_HOST_TRANSPORT_TEST
#if PY_VERSION_HEX != 0x030A15F0
#error "Production whole bridge requires existing CPython 3.10.21 final"
#endif
#endif
static _Thread_local char whole_error[2048];
static atomic_flag active = ATOMIC_FLAG_INIT;
const char* reconstruction_production_whole_last_error_v1(void) { return whole_error; }
static void capture_error(void) {
  PyObject *type=NULL,*value=NULL,*trace=NULL;
  PyErr_Fetch(&type,&value,&trace);
  PyErr_NormalizeException(&type,&value,&trace);
  PyObject* text=value?PyObject_Str(value):NULL;
  const char* utf8=text?PyUnicode_AsUTF8(text):NULL;
  snprintf(whole_error,sizeof(whole_error),"%s",utf8?utf8:"production whole driver failed");
  Py_XDECREF(text); Py_XDECREF(type); Py_XDECREF(value); Py_XDECREF(trace);
  PyErr_Clear();
}
static void capture_driver_status(PyObject* module,long code) {
  // Preserve an already-pending exception before attempting any more Python IO.
  if(PyErr_Occurred()) {
    capture_error();
    char original[sizeof(whole_error)];
    snprintf(original,sizeof(original),"%s",whole_error);
    snprintf(whole_error,sizeof(whole_error),"production whole driver status %ld: %s",code,original);
    return;
  }
  PyObject* getter=PyObject_GetAttrString(module,"last_error");
  PyObject* detail=getter?PyObject_CallObject(getter,NULL):NULL;
  const char* utf8=NULL;
  if(detail) {
    if(!PyUnicode_Check(detail))
      PyErr_SetString(PyExc_TypeError,"production whole last_error must return text");
    else utf8=PyUnicode_AsUTF8(detail);
  }
  // Copy borrowed UTF-8 immediately, while detail and the GIL are still alive.
  if(utf8)
    snprintf(whole_error,sizeof(whole_error),"production whole driver status %ld: %s",code,utf8);
  else {
    capture_error();
    char extraction[sizeof(whole_error)];
    snprintf(extraction,sizeof(extraction),"%s",whole_error);
    snprintf(whole_error,sizeof(whole_error),"production whole driver status %ld; last_error extraction: %s",code,extraction);
  }
  Py_XDECREF(detail); Py_XDECREF(getter);
}
int reconstruction_production_whole_run_v1(const char* request,const char* result) {
  whole_error[0]=0;
  if(!request||!result||request[0]!='/'||result[0]!='/'||
      !strcmp(request,result)) {
    snprintf(whole_error,sizeof(whole_error),"distinct absolute request/result JSON paths required");
    return 22;
  }
  if(!Py_IsInitialized()) {
    snprintf(whole_error,sizeof(whole_error),"existing CPython runtime is not initialized");
    return 1;
  }
  if(atomic_flag_test_and_set(&active)) {
    snprintf(whole_error,sizeof(whole_error),"production whole run already active");
    return 16;
  }
  PyGILState_STATE gil=PyGILState_Ensure();
  PyObject* module=PyImport_ImportModule("reconstruction_production_whole.production_whole_driver");
  PyObject* fn=module?PyObject_GetAttrString(module,"run_json"):NULL;
  PyObject* a=PyUnicode_DecodeFSDefault(request);
  PyObject* b=PyUnicode_DecodeFSDefault(result);
  PyObject* returned=NULL;
  if(fn&&a&&b) returned=PyObject_CallFunctionObjArgs(fn,a,b,NULL);
  int status=1;
  if(returned) {
    if(!PyLong_CheckExact(returned))
      PyErr_SetString(PyExc_TypeError,"production whole run_json must return integer status 0");
    else {
      long code=PyLong_AsLong(returned);
      if(!PyErr_Occurred()) {
        if(code==0) status=0;
        else capture_driver_status(module,code);
      }
    }
  }
  if(status&&!whole_error[0]) capture_error();
  Py_XDECREF(returned); Py_XDECREF(b); Py_XDECREF(a);
  Py_XDECREF(fn); Py_XDECREF(module);
  PyGILState_Release(gil);
  atomic_flag_clear(&active);
  if(!status) whole_error[0]=0;
  return status;
}
