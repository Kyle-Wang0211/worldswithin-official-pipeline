/* Typed synchronous ownership/GIL glue only. All math is exact original Python. */
#include <Python.h>
#include <stdint.h>
#include <stdio.h>
#include "reconstruction_original_dense_raw.h"
#if PY_VERSION_HEX != 0x030A15F0
#error "Shared original dense bridge requires CPython3.10.21 final headers"
#endif
_Static_assert(sizeof(float)==4 && sizeof(double)==8 && sizeof(uint16_t)==2,"original raw ABI");
static _Thread_local char raw_error[2048];
RECONSTRUCTION_RAW_EXPORT const char *reconstruction_original_dense_raw_last_error(void) { return raw_error; }
static void capture_error(void) {
    PyObject *type=NULL,*value=NULL,*trace=NULL;
    PyErr_Fetch(&type,&value,&trace);PyErr_NormalizeException(&type,&value,&trace);
    PyObject *s=value?PyObject_Str(value):NULL;
    const char *text=s?PyUnicode_AsUTF8(s):NULL;
    snprintf(raw_error,sizeof(raw_error),"%s",text?text:"original raw Python bridge failed");
    Py_XDECREF(s);Py_XDECREF(type);Py_XDECREF(value);Py_XDECREF(trace);PyErr_Clear();
}
static int invalid(const char *reason) { snprintf(raw_error,sizeof(raw_error),"%s",reason);return 22; }
static int valid_shape(uint32_t w,uint32_t h,uint64_t unit) {
    return w && h && (uint64_t)w*h <= (uint64_t)PY_SSIZE_T_MAX/unit;
}
static PyObject *view(const void *p,uint64_t n,int writable) {
    if (!p || n>(uint64_t)PY_SSIZE_T_MAX) {PyErr_SetString(PyExc_ValueError,"invalid shared raw buffer");return NULL;}
    return PyMemoryView_FromMemory((char*)p,(Py_ssize_t)n,writable?PyBUF_WRITE:PyBUF_READ);
}
static PyObject *plane(const RECONSTRUCTIONOriginalPlane *p) {
    PyObject *v=view(p->data,(uint64_t)p->width*p->height*4,0);
    PyObject *w=PyLong_FromUnsignedLong(p->width),*h=PyLong_FromUnsignedLong(p->height),*out=NULL;
    if(v && w && h) out=PyTuple_Pack(3,v,w,h);
    Py_XDECREF(v);Py_XDECREF(w);Py_XDECREF(h);return out;
}
static PyObject *planes(const RECONSTRUCTIONOriginalPlane *p) {
    PyObject *table=PyTuple_New(4);if(!table)return NULL;
    for(int i=0;i<4;i++){PyObject *item=plane(p+i);if(!item){Py_DECREF(table);return NULL;}PyTuple_SET_ITEM(table,i,item);}
    return table;
}
static int initialized(void) {
    raw_error[0]=0;
    if(!Py_IsInitialized()){snprintf(raw_error,sizeof(raw_error),"selected process-owned Python runtime uninitialized");return 0;}
    return 1;
}
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_dense_dependencies_validate(void) {
    if(!initialized())return 1;
    PyGILState_STATE gil=PyGILState_Ensure();int status=1;
    PyObject *module=PyImport_ImportModule("reconstruction_original_dense_raw_driver");
    PyObject *fn=module?PyObject_GetAttrString(module,"validate_dependencies"):NULL;
    PyObject *result=fn?PyObject_CallNoArgs(fn):NULL;
    if(!result)goto failed;
    if(result!=Py_None){PyErr_SetString(PyExc_TypeError,"original readiness validator must return None");goto failed;}
    status=0;goto done;
failed: capture_error();
done:
    Py_XDECREF(result);Py_XDECREF(fn);Py_XDECREF(module);
    PyGILState_Release(gil);return status;
}
static int returned_count(PyObject *result,uint64_t *output) {
    if(!result)return 1;
    if(!PyLong_Check(result)){PyErr_SetString(PyExc_TypeError,"original result count must be an integer");return 1;}
    unsigned long long count=PyLong_AsUnsignedLongLong(result);if(PyErr_Occurred())return 1;
    if(output)*output=(uint64_t)count;return 0;
}
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_merge_frame(const RECONSTRUCTIONOriginalPlane low[4],
    const RECONSTRUCTIONOriginalPlane high[4],const RECONSTRUCTIONOriginalMutablePlane output[4],
    uint32_t frame_index,uint64_t *high_kept_count) {
    raw_error[0]=0;if(high_kept_count)*high_kept_count=0;
    if(!low || !high || !output || frame_index>=97)return invalid("invalid original merge arrays/index");
    for(int i=0;i<4;i++)if(!low[i].data || !high[i].data || !output[i].data ||
        !valid_shape(low[i].width,low[i].height,4) || !valid_shape(high[i].width,high[i].height,4) ||
        output[i].width!=high[0].width || output[i].height!=high[0].height)
        return invalid("invalid original merge plane shape/buffer");
    if(!initialized())return 1;
    PyGILState_STATE gil=PyGILState_Ensure();int status=1;
    PyObject *module=PyImport_ImportModule("reconstruction_original_dense_raw_driver");
    PyObject *fn=module?PyObject_GetAttrString(module,"merge_frame"):NULL;
    PyObject *lo=planes(low),*hi=planes(high),*outs=PyTuple_New(4),*index=PyLong_FromUnsignedLong(frame_index),*result=NULL;
    if(!fn || !lo || !hi || !outs || !index)goto failed;
    for(int i=0;i<4;i++){PyObject *item=view(output[i].data,(uint64_t)output[i].width*output[i].height*4,1);if(!item)goto failed;PyTuple_SET_ITEM(outs,i,item);}
    result=PyObject_CallFunctionObjArgs(fn,lo,hi,outs,index,NULL);
    status=returned_count(result,high_kept_count);if(status)goto failed;
    goto done;
failed: capture_error();
done:
    Py_XDECREF(result);Py_XDECREF(index);Py_XDECREF(outs);Py_XDECREF(hi);Py_XDECREF(lo);Py_XDECREF(fn);Py_XDECREF(module);
    PyGILState_Release(gil);return status;
}
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_filter_pair(const RECONSTRUCTIONOriginalPlane *reference,
    const RECONSTRUCTIONOriginalPlane *source,const double *Kr9,const double *Er16,
    const double *Ks9,const double *Es16,double depth_max,double depth_min,
    uint8_t *geo_mask,float *depth_reprojected,float *x_source,float *y_source) {
    raw_error[0]=0;
    if(!reference || !source || !reference->data || !source->data || !Kr9 || !Er16 || !Ks9 || !Es16 ||
       !geo_mask || !depth_reprojected || !x_source || !y_source ||
       !valid_shape(reference->width,reference->height,4) || !valid_shape(source->width,source->height,4))
        return invalid("invalid original filter buffers");
    if(!initialized())return 1;
    PyGILState_STATE gil=PyGILState_Ensure();int status=1;
    PyObject *module=PyImport_ImportModule("reconstruction_original_dense_raw_driver");
    PyObject *fn=module?PyObject_GetAttrString(module,"filter_pair"):NULL;
    PyObject *ref=plane(reference),*src=plane(source),*kr=view(Kr9,72,0),*er=view(Er16,128,0),*ks=view(Ks9,72,0),*es=view(Es16,128,0);
    PyObject *max=PyFloat_FromDouble(depth_max),*min=PyFloat_FromDouble(depth_min),*outs=PyTuple_New(4),*result=NULL;
    if(!fn || !ref || !src || !kr || !er || !ks || !es || !max || !min || !outs)goto failed;
    void *outdata[4]={geo_mask,depth_reprojected,x_source,y_source};
    uint64_t pixels=(uint64_t)reference->width*reference->height;
    for(int i=0;i<4;i++){PyObject *item=view(outdata[i],pixels*(i?4:1),1);if(!item)goto failed;PyTuple_SET_ITEM(outs,i,item);}
    result=PyObject_CallFunctionObjArgs(fn,ref,src,kr,er,ks,es,max,min,outs,NULL);
    if(!result)goto failed;
    if(result!=Py_None){PyErr_SetString(PyExc_TypeError,"original filter adapter must return None");goto failed;}
    status=0;goto done;
failed: capture_error();
done:
    Py_XDECREF(result);Py_XDECREF(outs);Py_XDECREF(min);Py_XDECREF(max);Py_XDECREF(es);Py_XDECREF(ks);Py_XDECREF(er);Py_XDECREF(kr);
    Py_XDECREF(src);Py_XDECREF(ref);Py_XDECREF(fn);Py_XDECREF(module);PyGILState_Release(gil);return status;
}
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fuse_pack(const RECONSTRUCTIONOriginalFusionPack *pack,
    const char *retained_directory,const char *fused_ply_path,uint64_t *kept_total) {
    raw_error[0]=0;if(kept_total)*kept_total=0;
    if(!pack || !retained_directory || !retained_directory[0] || (!pack->frame_count || pack->frame_count>97) ||
       pack->width!=1536 || pack->height!=1152 || !pack->cams_f32 || !pack->frame_ids_f64 ||
       (!pack->original_jpeg_paths && (!pack->rgb_u8 || !valid_shape(pack->rgb_width,pack->rgb_height,pack->frame_count*3))))return invalid("invalid original actual-N fusion pack");
    for(int i=0;i<4;i++)if(!pack->banks[i])return invalid("missing original aggregate bank");
    if(pack->original_jpeg_paths)for(uint32_t i=0;i<pack->frame_count;i++)if(!pack->original_jpeg_paths[i] || pack->original_jpeg_paths[i][0]!='/')return invalid("actual-N original absolute JPEG paths required");
    if(fused_ply_path)return invalid("selected archived fusion has no frozen fusedPLY producer; pass NULL");
    if(!initialized())return 1;
    PyGILState_STATE gil=PyGILState_Ensure();int status=1;
    PyObject *module=PyImport_ImportModule("reconstruction_original_dense_raw_driver");
    PyObject *fn=module?PyObject_GetAttrString(module,"fuse_pack"):NULL;
    PyObject *banks=PyTuple_New(4),*cams=view(pack->cams_f32,pack->frame_count*36*4,0),*rgb=NULL,*jpeg_paths=NULL;
    if(pack->original_jpeg_paths){rgb=Py_None;Py_INCREF(rgb);jpeg_paths=PyTuple_New(pack->frame_count);}
    else {rgb=view(pack->rgb_u8,(uint64_t)pack->frame_count*pack->rgb_width*pack->rgb_height*3,0);jpeg_paths=Py_None;Py_INCREF(jpeg_paths);}
    PyObject *w=PyLong_FromUnsignedLong(pack->rgb_width),*h=PyLong_FromUnsignedLong(pack->rgb_height),*ids=view(pack->frame_ids_f64,(uint64_t)pack->frame_count*8,0);
    PyObject *retained=PyUnicode_DecodeFSDefault(retained_directory),*result=NULL;
    if(!fn || !banks || !cams || !rgb || !jpeg_paths || !w || !h || !ids || !retained)goto failed;
    for(int i=0;i<4;i++){PyObject *item=view(pack->banks[i],(uint64_t)pack->frame_count*1152*1536*4,0);if(!item)goto failed;PyTuple_SET_ITEM(banks,i,item);}
    if(pack->original_jpeg_paths)for(uint32_t i=0;i<pack->frame_count;i++){PyObject *path=PyUnicode_DecodeFSDefault(pack->original_jpeg_paths[i]);if(!path)goto failed;PyTuple_SET_ITEM(jpeg_paths,i,path);}
    /* All input views and caller-owned buffers stay alive under this GIL until
     * exact original main completion returns. The driver clears its exec scope. */
    result=PyObject_CallFunctionObjArgs(fn,banks,cams,rgb,w,h,ids,retained,Py_None,jpeg_paths,NULL);
    status=returned_count(result,kept_total);if(status)goto failed;
    goto done;
failed: capture_error();
done:
    Py_XDECREF(result);Py_XDECREF(retained);Py_XDECREF(ids);Py_XDECREF(h);Py_XDECREF(w);Py_XDECREF(rgb);Py_XDECREF(cams);
    Py_XDECREF(jpeg_paths);Py_XDECREF(banks);Py_XDECREF(fn);Py_XDECREF(module);PyGILState_Release(gil);return status;
}

/* Additive whole-original-fixture path transport. Append to the exact frozen
 * raw bridge translation unit to reuse its thread-local error conventions. */
#include <string.h>
#include "reconstruction_original_fixture.h"

static int fixture_absolute_path(const char *value) {
    if (!value || value[0] != '/') return 0;
    /* Bound scans before constructing Python strings. No path interpretation or
     * serialization occurs here; original IO wrapper owns canonical-path gates. */
    for (size_t i = 1; i < 16384; ++i) if (!value[i]) return 1;
    return 0;
}
static int fixture_sha256(const char *value) {
    if (!value) return 0;
    for (size_t i = 0; i < 64; ++i) {
        char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
    }
    return value[64] == 0;
}
RECONSTRUCTION_RAW_EXPORT int reconstruction_original_fixture_prepare(
    const char *capture, const char *output, const char *manifest,
    const char *manifest_sha, const char *receipt) {
    raw_error[0] = 0;
    if (!fixture_absolute_path(capture) || !fixture_absolute_path(output) ||
        !fixture_absolute_path(manifest) || !fixture_absolute_path(receipt) ||
        !fixture_sha256(manifest_sha))
        return invalid("original fixture requires four absolute UTF8 paths and exact lowercase SHA256");
    if (!initialized()) return 1;
    PyGILState_STATE gil = PyGILState_Ensure();
    int status = 1;
    PyObject *module = PyImport_ImportModule("reconstruction_original_fixture_bridge");
    PyObject *fn = module ? PyObject_GetAttrString(module, "prepare_v2") : NULL;
    PyObject *a = PyUnicode_DecodeUTF8(capture, (Py_ssize_t)strlen(capture), "strict");
    PyObject *b = PyUnicode_DecodeUTF8(output, (Py_ssize_t)strlen(output), "strict");
    PyObject *c = PyUnicode_DecodeUTF8(manifest, (Py_ssize_t)strlen(manifest), "strict");
    PyObject *d = PyUnicode_DecodeUTF8(manifest_sha, 64, "strict");
    PyObject *e = PyUnicode_DecodeUTF8(receipt, (Py_ssize_t)strlen(receipt), "strict");
    PyObject *result = NULL;
    if (!fn || !a || !b || !c || !d || !e) goto failed;
    if (!PyCallable_Check(fn)) {
        PyErr_SetString(PyExc_TypeError, "whole original fixture prepare must be callable");
        goto failed;
    }
    result = PyObject_CallFunctionObjArgs(fn, a, b, c, d, e, NULL);
    if (!result) goto failed;
    if (!PyLong_CheckExact(result)) {
        PyErr_SetString(PyExc_TypeError, "whole original fixture prepare must return exact positive actual-N integer");
        goto failed;
    }
    long generated = PyLong_AsLong(result);
    if (PyErr_Occurred()) goto failed;
    if (generated <= 0) {
        PyErr_SetString(PyExc_ValueError, "whole original fixture prepare returned nonpositive actual-N");
        goto failed;
    }
    status = 0;
    goto done;
failed:
    capture_error();
done:
    Py_XDECREF(result); Py_XDECREF(e); Py_XDECREF(d); Py_XDECREF(c);
    Py_XDECREF(b); Py_XDECREF(a); Py_XDECREF(fn); Py_XDECREF(module);
    PyGILState_Release(gil);
    return status;
}
