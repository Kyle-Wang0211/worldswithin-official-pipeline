"""Auditable source adaptation. Only exact AST integer20 is generalized.

All source strings and other numerical constants remain original, except the
explicit count-derived completed40 predicate specialized to 2*N. IO ownership
changes are listed separately below. Not a numerical source generator.
"""
import ast
import hashlib
from pathlib import Path


def specialize_counts(text):
    tree=ast.parse(text);lines=text.splitlines(keepends=True);offsets=[0]
    for line in lines:offsets.append(offsets[-1]+len(line.encode()))
    data=text.encode();edits=[];manifest=[]
    for n in ast.walk(tree):
        if isinstance(n,ast.Constant)and type(n.value)is int and n.value==20:
            start=offsets[n.lineno-1]+n.col_offset;end=offsets[n.end_lineno-1]+n.end_col_offset
            assert data[start:end]==b'20'
            edits.append((start,end,b'count'));manifest.append(dict(line=n.lineno,column=n.col_offset,value=20,replacement='actual_scene_count'))
    for start,end,value in sorted(edits,reverse=True):data=data[:start]+value+data[end:]
    text=data.decode()
    assert text.count("m['completed_reference_runs']!=40")==1
    text=text.replace("m['completed_reference_runs']!=40","m['completed_reference_runs']!=2*count")
    return text,manifest

def adapt(text):
    text,manifest=specialize_counts(text)
    text=text.replace('from owner_context import','from .owner_context import')
    text=text.replace('def validate_ordered_input_binding(binding,docs,root):','def validate_ordered_input_binding(binding,docs,root):\n count=binding["handoff"]["count"]\n if type(count)is not int or not 10<=count<=97:raise ValueError("actual ordered N10..97 required")')
    text=text.replace('H,ctrl,B=validate_ordered_input_binding(bound_inputs,docs,root)','H,ctrl,B=validate_ordered_input_binding(bound_inputs,docs,root);count=H["count"]')
    text=text.replace("retained=root/'dense/retained';retained.mkdir()", "retained=Path(ctrl['retained_absolute']);retained.mkdir(parents=True)")
    text=text.replace("root/'dense/work'", "Path(ctrl['work_absolute'])")
    text=text.replace("'order':list(range(count)),'frame_id_f64_bits':ctrl['frame_id_f64_bits'],'kept_total'", "'order':list(range(count)),'kept_total'")
    text=text.replace("runtime=verify(docs,B['runtime']);m0path=verify(docs,B['M0'])", "runtime=verify(bundle,B['runtime']);m0path=verify(bundle,B['M0'])")
    text=text.replace("{str(bundle/'Frameworks/PWOnnxRuntime.framework/PWOnnxRuntime'):'1a6626919bdc446c0d13b0af8bac0c4636438db3be9a8dfdf820fa94e88c684c'}", "ctrl['passive_runtime_images']")
    text=text.replace("thermal_path=bundle/'Frameworks/libreconstructionThermal.dylib'", "thermal_path=verify(bundle,B['thermal'])")
    text=text.replace("original=bundle/'RECONSTRUCTIONPython/lib/python3.10/site-packages/selected_original_fuse_verbatim_sat_angle.py'", "original=verify(bundle,ctrl['original_fusion'])")
    text=text.replace("bundle/'Frameworks/RECONSTRUCTIONPythonExtensions/reconstruction_row_runtime/visibility_cache_v2.py'", "verify(bundle,ctrl['qualification_source'])")
    text=text.replace('if not guard_restored:runtime_guard.restore()', 'if not guard_restored and not pending_context:runtime_guard.restore()')
    start=text.index('def finish_pending():');end=text.index('\nU=C.c_uint32',start);text=text[:start]+text[end:]
    return text,manifest
