"""Selected reference_pipeline hook: survivor-only angle and guarded sparse accumulation.
No tiling, ROI, sees shortcut, matrix-shape change, external path, or import hook.
Caller must install AFTER original fusion_defs and BEFORE bind_original_one/main.
"""
import ast
import hashlib
import sys
import numpy as np

FUSION_SHA = 'b29a8abfb40f6957dbf119bd567ef701437e8d8208d4cc32d9947a882723ceab'
FILTER_SHA = 'b3c063519ff8afd8d4bfc607dc97e9edc177afc7f48893c36a1d74019cc1776c'
DEFS_SHA = 'dc0a17e1d586187ba0cff65edcf7bbb23b021c33d0f8ebf2c20f3b88e4dee21b'

def dense_angle(gm, d_rep, Xw, ray_r, Es, cos_min):
    ray_s = -Es[:3, :3].T @ Es[:3, 3] - Xw
    ray_s /= np.maximum(np.linalg.norm(ray_s, axis=1, keepdims=True), 1e-12)
    gm = gm & (np.sum(ray_r * ray_s, 1) <= cos_min).reshape(gm.shape)
    return gm, np.where(gm, d_rep, 0)

def selected_angle(gm, d_rep, Xw, ray_r, Es, cos_min):
    if any(v not in ('warn', 'ignore') for v in np.geterr().values()):
        return dense_angle(gm, d_rep, Xw, ray_r, Es, cos_min)
    idx = np.flatnonzero(gm)
    if idx.size:
        ray_s = -Es[:3, :3].T @ Es[:3, 3] - Xw[idx]
        ray_s /= np.maximum(np.linalg.norm(ray_s, axis=1, keepdims=True), 1e-12)
        gm = gm.copy()
        gm.ravel()[idx] = np.sum(ray_r[idx] * ray_s, 1) <= cos_min
    return gm, np.where(gm, d_rep, 0)

def accumulate(gs, acc, gm, d_rep, active):



    idx = np.flatnonzero(gm)
    values = d_rep.ravel()[idx]
    safe = (active and all(v in ('warn', 'ignore') for v in np.geterr().values())
            and np.isfinite(values).all() and not np.signbit(values).any())
    if safe:
        gs.ravel()[idx] += 1
        acc.ravel()[idx] += values
    else:
        gs += gm.astype(np.int32)
        acc += d_rep
    return bool(safe)

def validate_runtime():
    import cv2
    import PIL
    if (sys.version_info[:3] != (3, 10, 21) or sys.byteorder != 'little'
            or np.__version__ != '1.26.4' or cv2.__version__ != '4.11.0'
            or PIL.__version__ != '12.3.0'):
        raise RuntimeError('pinned mobile CPython/NumPy/OpenCV/Pillow required')

def _same(a, b):
    return ast.dump(a, include_attributes=False) == ast.dump(b, include_attributes=False)

def transformed_defs(fusion_source, filter_source):
    if not isinstance(fusion_source, bytes) or not isinstance(filter_source, bytes):
        raise TypeError('original full source bytes required')
    for raw, expected in ((fusion_source, FUSION_SHA), (filter_source, FILTER_SHA)):
        if hashlib.sha256(raw).hexdigest() != expected:
            raise RuntimeError('original full-source hash mismatch')
    raw = b''.join(fusion_source.splitlines(keepends=True)[35:74])
    if hashlib.sha256(raw).hexdigest() != DEFS_SHA:
        raise RuntimeError('original fusion_defs span mismatch')
    tree = ast.parse(raw)
    one = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'one']
    if len(one) != 1:
        raise RuntimeError('one function identity mismatch')
    one = one[0]
    original_angle = ast.parse('''if s != f and a.min_tri_angle > 0:
 ray_s = -Es[:3, :3].T @ Es[:3, 3] - Xw
 ray_s /= np.maximum(np.linalg.norm(ray_s, axis=1, keepdims=True), 1e-12)
 gm = gm & (np.sum(ray_r * ray_s, 1) <= cos_min).reshape(ref.shape)
 d_rep = np.where(gm, d_rep, 0)
''').body[0]
    loop = [n for n in one.body if isinstance(n, ast.For)]

    source_loop = [n for n in loop if isinstance(n.target, ast.Name) and n.target.id == 's']
    if len(source_loop) != 1:
        raise RuntimeError('source loop identity mismatch')
    source_loop = source_loop[0]
    if not _same(source_loop.iter, ast.parse('range(NF)', mode='eval').body):
        raise RuntimeError('source view order mismatch')
    angle_indices = [i for i,n in enumerate(source_loop.body) if _same(n, original_angle)]
    accum = ast.parse('gs += gm.astype(np.int32); acc += d_rep').body
    if len(angle_indices) != 1 or not all(_same(a,b) for a,b in zip(source_loop.body[-2:], accum)):
        raise RuntimeError('angle/accumulation AST mismatch')
    replacement = ast.parse('''if s != f and a.min_tri_angle > 0:
 gm, d_rep = _reconstruction_selected_angle(gm, d_rep, Xw, ray_r, Es, cos_min)
''').body[0]
    source_loop.body[angle_indices[0]] = replacement
    source_loop.body[-2:] = ast.parse('_reconstruction_sparse_active = _reconstruction_accumulate(gs, acc, gm, d_rep, _reconstruction_sparse_active)').body
    index = one.body.index(source_loop)
    one.body[index:index] = ast.parse('_reconstruction_sparse_active = True').body
    ast.fix_missing_locations(tree)
    return ast.unparse(tree) + '\n'

def install(scope, fusion_source, filter_source, *, pinned_runtime=True):
    if pinned_runtime:
        validate_runtime()
    text = transformed_defs(fusion_source, filter_source)


    if 'one' not in scope or 'check_geometric_consistency' not in scope:
        raise RuntimeError('install after original filter/fusion definitions')
    scope['_reconstruction_selected_angle'] = selected_angle
    scope['_reconstruction_accumulate'] = accumulate
    exec(compile(text, '<reconstruction-angle-sparse-fullshape>', 'exec'), scope)
    return {'profile': 'angle_sparse_fullshape', 'fusion_sha256': FUSION_SHA,
            'filter_sha256': FILTER_SHA, 'pinned_runtime': bool(pinned_runtime)}

def compile_defs(fusion_source, filter_source, *, pinned_runtime=True):
    """Compile once; selected IO constructor AND exec must share this object.

    SelectedFusionArrayIO deliberately checks the precise one() code identity.
    Pass result as its fusion_defs_code; do not weaken that observer guard.
    """
    if pinned_runtime:
        validate_runtime()
    return compile(transformed_defs(fusion_source, filter_source),
                   '<reconstruction-angle-sparse-fullshape>', 'exec')

def bind_helpers(scope):
    scope['_reconstruction_selected_angle'] = selected_angle
    scope['_reconstruction_accumulate'] = accumulate
