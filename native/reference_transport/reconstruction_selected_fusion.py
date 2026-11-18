"""Additive selected-cache IO; reference_pipeline numerical code is never rewritten.

Production entry retains the old strict CPython/NumPy/OpenCV/Pillow pins.
Small host tests are transport diagnostics, not mobile or A100 parity evidence.
"""
import hashlib
import io
import json
import os
import pathlib
import stat
import struct
import sys
import types

import reconstruction_original_dense_raw_driver as old_driver

_DRIVER_SHA = '06a1b3b95021888a470108fb34a853e0756992bd42a6d8a93bedd50984f96956'
_FILTER_SHA = 'b3c063519ff8afd8d4bfc607dc97e9edc177afc7f48893c36a1d74019cc1776c'
_PROJECTION_LINES = (216, 217, 218, 220, 223, 224, 225, 226, 227)
_PROJECTION_SHA = '0819a37f14e822bd3369f762e3631c7a1e9ad4ed544722a7f4b8cac430c4bb01'
_ASSEMBLY_LINES = (230, 232, 233)
_ASSEMBLY_SHA = '2ef863cce1df72641fdb389aa90aab2109ec8cb67497fd2caa0cb519ff2094aa'
_GRID = (1152, 1536)


def source_codes():
    driver = pathlib.Path(old_driver.__file__).read_bytes()
    if hashlib.sha256(driver).hexdigest() != _DRIVER_SHA:
        raise RuntimeError('original raw driver full-source SHA mismatch')
    raw = pathlib.Path(old_driver.__file__).with_name('selected_original_filter.py').read_bytes()
    if len(raw) != 17110 or hashlib.sha256(raw).hexdigest() != _FILTER_SHA:
        raise RuntimeError('official projection full-source SHA mismatch')
    lines = raw.splitlines(keepends=True)
    result = {}
    for name, indices, expected in (
        ('projection', _PROJECTION_LINES, _PROJECTION_SHA),
        ('assembly', _ASSEMBLY_LINES, _ASSEMBLY_SHA),
    ):
        span = b''.join(lines[i-1] for i in indices)
        if hashlib.sha256(span).hexdigest() != expected:
            raise RuntimeError('official selected transport span SHA mismatch: '+name)
                                                                           
        result[name] = compile(b'if True:\n'+span, 'selected_original_filter.py:selected_'+name, 'exec')
    return result, dict(filter_full_sha256=_FILTER_SHA,
                        fusion_full_sha256=old_driver._SOURCES['fusion']['full_sha256'],
                        projection_sha256=_PROJECTION_SHA,
                        projection_lines=list(_PROJECTION_LINES),
                        assembly_sha256=_ASSEMBLY_SHA,
                        assembly_lines=list(_ASSEMBLY_LINES),
                        original_driver_sha256=_DRIVER_SHA)


def original_index_values(original_indices, count):
    view = old_driver.checked_view(original_indices, count*4)
    if type(count) is not int or not 0 < count <= 97:
        raise ValueError('positive selected frame count <=97 required')
    indices = struct.unpack('<'+'I'*count, view)
    if any(i > 96 for i in indices) or any(a >= b for a, b in zip(indices, indices[1:])):
        raise ValueError('strict ascending original indices in0..96 required')
    return indices


def _array(np, value, dtype, shape, name):
    if type(value) is not np.ndarray or value.dtype != np.dtype(dtype) or value.shape != shape or not value.flags.c_contiguous:
        raise ValueError('original selected observation dtype/shape/layout mismatch: '+name)


def project_f4(np, depth_est_averaged, final_mask, ref_intrinsics, ref_extrinsics):
    codes, _ = source_codes()
    scope = dict(np=np, depth_est_averaged=depth_est_averaged,
                 final_mask=final_mask, ref_intrinsics=ref_intrinsics,
                 ref_extrinsics=ref_extrinsics, vertexs=[])
    try:
        exec(codes['projection'], scope)
        exec(codes['assembly'], scope)
        points = scope['vertexs']
        expected = np.dtype([('x', '<f4'), ('y', '<f4'), ('z', '<f4')])
        if points.dtype != expected or points.ndim != 1 or not points.flags.c_contiguous or points.dtype.itemsize != 12:
            raise ValueError('official structured-f4 output layout mismatch')
        return points
    finally:
        scope.clear()


def _observe(np, caller, one_code, frame_index, shape, arrays, mask_callback,
             collect_live=False):
    """Read the real one() locals; return only independently owned mask bytes.

    The production adapter supplies one_code from the validated original defs
    code object. No frame/locals reference is retained after this call. Callback
    f4 rows have original final-valid pixel order; callback-selected rows match
    col[returned_pixel_mask != 0] in C-row-major order. The view is borrowed only
    for the synchronous callback; any later live display must copy its bytes.
    """
    if not isinstance(caller, types.FrameType) or caller.f_code is not one_code or one_code.co_name != 'one':
        raise ValueError('selected observer requires the bound original one() caller')
    if not callable(mask_callback) or type(frame_index) is not int or len(shape) != 2 or any(type(x) is not int or x <= 0 for x in shape):
        raise ValueError('selected callback/frame/grid required')
    local = caller.f_locals
    try:
        if type(local.get('f')) is not int or local['f'] != frame_index or type(local.get('t')) is not int or local['t'] != 2:
            raise ValueError('original frame index/threshold2 observation mismatch')
        if set(arrays) != {'d16', 'col', 'intr', 'extr'}:
            raise ValueError('original retention fields changed')
        for name, dtype, expected in (
            ('d_avg', '<f8', shape), ('final', 'bool', shape),
            ('Kr', '<f8', (3, 3)), ('Er', '<f8', (4, 4)), ('d', '<f8', shape),
        ):
            _array(np, local.get(name), dtype, expected, name)
        for name, dtype, expected in (
            ('d16', '<u2', shape), ('col', 'u1', shape+(3,)),
            ('intr', '<f8', (3, 3)), ('extr', '<f8', (4, 4)),
        ):
            _array(np, arrays[name], dtype, expected, name)
        if arrays['intr'] is not local['Kr'] or arrays['extr'] is not local['Er']:
            raise ValueError('original live camera identity changed')
        final = local['final']
        mask = np.zeros(shape, dtype='u1')
                                                                      
        if not np.any(final):
            return mask, 0, None, None
        points = project_f4(np, local['d_avg'], final, local['Kr'], local['Er'])
        points.setflags(write=False)
        view = memoryview(points).cast('B').toreadonly()
        try:
            classified = mask_callback(view)
        finally:
            view.release()
        if type(classified) is not bytes or len(classified) != len(points) or any(x not in (0, 1) for x in classified):
            raise ValueError('native selection mask requires exactly one byte0/1 per valid point')
                                                                             
        mask.reshape(-1)[np.flatnonzero(final)] = np.frombuffer(classified, dtype='u1')
        selected_points = selected_colors = None
        if collect_live:
            keep = np.frombuffer(classified, dtype='u1') != 0
            selected_points = points[keep]
            selected_colors = arrays['col'][final][keep]
            selected_points.setflags(write=False)
            selected_colors.setflags(write=False)
        return mask, int(np.count_nonzero(mask)), selected_points, selected_colors
    finally:
        del local


def live_mask(np, caller, one_code, frame_index, shape, arrays, mask_callback):
    return _observe(np, caller, one_code, frame_index, shape, arrays, mask_callback)[:2]


def _sha(array):
    return hashlib.sha256(memoryview(array).cast('B')).hexdigest()


def write_selected_frame(np, selected_dir, pack_index, original_index, frame_bits,
                         arrays, mask, plan_sha256, original_dir,
                         original_final_valid_count=None):
    shape = arrays['d16'].shape
    _array(np, mask, 'u1', shape, 'selected mask')
    if np.any(mask > 1):
        raise ValueError('selected pixel mask must contain bytes0/1')
    for name, dtype, expected in (
        ('d16', '<u2', shape), ('col', 'u1', shape+(3,)),
        ('intr', '<f8', (3, 3)), ('extr', '<f8', (4, 4)),
    ):
        _array(np, arrays[name], dtype, expected, name)
    stem = f'{pack_index:08d}'
    selected = arrays['d16'].copy()
    selected[mask == 0] = 0                                                        
    names = dict(d16=stem+'.d16', color=stem+'.col.u8',
                 camera=stem+'.camera.f64le', mask=stem+'.selection.u8')
    for name, value in (('d16', selected), ('mask', mask)):
        with (selected_dir/names[name]).open('xb') as stream:
            value.tofile(stream)
    camera_hash = hashlib.sha256()
    for name in ('intr', 'extr'):
        camera_hash.update(memoryview(arrays[name]).cast('B'))
    color_sha = _sha(arrays['col'])
                                                                               
                                                                               
    original_dir = pathlib.Path(original_dir)
    for name, expected_size, expected_sha in (
        ('color', arrays['col'].nbytes, color_sha),
        ('camera', 200, camera_hash.hexdigest()),
    ):
        source = original_dir/names[name]
        source_stat = source.lstat()
        if not stat.S_ISREG(source_stat.st_mode) or source_stat.st_size != expected_size:
            raise ValueError('original immutable retained file layout mismatch: '+name)
        digest = hashlib.sha256()
        with source.open('rb') as stream:
            for chunk in iter(lambda: stream.read(65536), b''):
                digest.update(chunk)
        if digest.hexdigest() != expected_sha:
            raise ValueError('original immutable retained file byte mismatch: '+name)
        os.link(source, selected_dir/names[name], follow_symlinks=False)
    row = dict(schema='reconstruction_selected_retained_frame/1', pack_index=pack_index,
               original_index=original_index, frame_id_f64_bits=frame_bits,
               plan_sha256=plan_sha256, width=shape[1], height=shape[0],
               depth_scale=5000, camera_layout='K9_then_E16_row_major_f64le',
               original_d16_sha256=_sha(arrays['d16']),
               selected_d16_sha256=_sha(selected), color_sha256=color_sha,
               camera_sha256=camera_hash.hexdigest(), mask_sha256=_sha(mask),
               selected_kept_count=int(np.count_nonzero(mask)),
               d16_bytes=selected.nbytes, color_bytes=arrays['col'].nbytes,
               camera_bytes=200, mask_bytes=mask.nbytes, **names)
    row['point_color_order'] = 'callback-selected f4 rows match color[selection_mask!=0] in C-row-major pixel order'
    row['original_retained_d16'] = str(original_dir/(stem+'.d16'))
    if original_final_valid_count is not None:
        row['original_final_valid_count'] = original_final_valid_count
    with (selected_dir/(stem+'.json')).open('x') as stream:
        json.dump(row, stream, separators=(',', ':')); stream.write('\n')
    return row


class SelectedFusionArrayIO(old_driver.FusionArrayIO):
    def __init__(self, np, loads, cameras, retained, frame_ids, selected_dir,
                 mask_callback, original_indices, plan_sha256, numerical_defs,
                 live_callback=None):
        if live_callback is not None and not callable(live_callback):
            raise ValueError('live callback must be callable or None')
        super().__init__(np, loads, cameras, retained, frame_ids)
        self.original_indices = original_index_values(original_indices, self.frame_count)
        self.selected_dir = pathlib.Path(selected_dir)
        self.mask_callback, self.plan_sha256 = mask_callback, plan_sha256
        self.live_callback = live_callback
        matches = [x for x in numerical_defs.co_consts if isinstance(x, types.CodeType) and x.co_name == 'one']
        if len(matches) != 1:
            raise ValueError('exact original one() code object required')
        self.original_one_code = matches[0]
        self.bound = False
        self.rows = []
        self.original_kept_total = 0
        self.selected_kept_total = 0

    def bind_original_one(self, one):
        if type(one) is not types.FunctionType or one.__code__ is not self.original_one_code or one.__globals__.get('np') is not self:
            raise ValueError('original one() function/code/IO binding mismatch')
        self.bound = True

    def savez(self, path, **arrays):
        p = pathlib.PurePosixPath(path)
        if not self.bound or str(p.parent) != '/reconstruction-original-raw/output_m2' or len(p.stem) != 8 or not p.stem.isdigit():
            raise ValueError('selected IO requires bound original save path')
        index = int(p.stem)
        if not 0 <= index < self.frame_count or index in self.written or index != len(self.rows):
            raise ValueError('selected frame order/index mismatch')
        caller = sys._getframe(1)
        try:
            mask, count, points, colors = _observe(
                self.np, caller, self.original_one_code, index, _GRID, arrays,
                self.mask_callback, collect_live=self.live_callback is not None)
            original_count = int(self.np.count_nonzero(caller.f_locals['final']))
        finally:
            del caller
                                                                              
        super().savez(path, **arrays)
        frame_bits = self.frame_ids.cast('B')[index*8:(index+1)*8].tobytes()[::-1].hex()
        row = write_selected_frame(self.np, self.selected_dir, index,
                                   self.original_indices[index], frame_bits,
                                   arrays, mask, self.plan_sha256, self.retained,
                                   original_final_valid_count=original_count)
        self.rows.append(row)
        self.original_kept_total += original_count
        self.selected_kept_total += count
        if self.live_callback is not None:
                                                                           
                                                                              
            point_view = memoryview(b'') if count == 0 else memoryview(points).cast('B').toreadonly()
            color_view = memoryview(b'') if count == 0 else memoryview(colors).cast('B').toreadonly()
            try:
                self.live_callback(index, point_view, color_view)
            finally:
                point_view.release(); color_view.release()


def _fresh(path, label):
    if not isinstance(path, str):
        raise ValueError('fresh absolute '+label+' directory required')
    target = pathlib.Path(path)
    if not target.is_absolute() or '..' in target.parts or target.exists() or target.is_symlink():
        raise ValueError('fresh absolute '+label+' directory required')
    return target


def fuse_pack_selected(banks, cameras, rgb, rgb_width, rgb_height, frame_ids,
                       retained, fused_ply, original_jpeg_paths, selected_dir,
                       mask_callback, original_indices, plan_sha256,
                       live_callback=None):
    selected_code, identity = source_codes()
    del selected_code
    code = old_driver.source_codes()
    if fused_ply is not None:
        raise RuntimeError('original selected fusedPLY producer identity unavailable; retained-only bridge supported')
    np, cv2, Image = old_driver.dependencies(fusion=True)
    if live_callback is not None and not callable(live_callback):
        raise ValueError('live callback must be callable or None')
    if not isinstance(banks, tuple) or len(banks) != 4 or type(rgb_width) is not int or type(rgb_height) is not int:
        raise ValueError('four actual-N frame banks and source RGB grid required')
    count = old_driver.transport_frame_count(cameras)
    old_driver.checked_view(frame_ids, count*8)
    indices = original_index_values(original_indices, count)
    if not callable(mask_callback) or not isinstance(plan_sha256, str) or len(plan_sha256) != 64 or any(x not in '0123456789abcdef' for x in plan_sha256):
        raise ValueError('native selection callback and lowercase plan SHA256 required')
    target, selected_target = _fresh(retained, 'original retained'), _fresh(selected_dir, 'selected retained')
    if target == selected_target or target in selected_target.parents or selected_target in target.parents:
        raise ValueError('separate nonnested original/selected directories required')
    if original_jpeg_paths is not None:
        if len(original_jpeg_paths) != count:
            raise ValueError('original JPEG and camera frame counts differ')
        image_io = old_driver.OriginalJPEGImageIO(Image, original_jpeg_paths)
        image_profile = 'original_jpeg'
    else:
        if rgb_width <= 0 or rgb_height <= 0:
            raise ValueError('diagnostic RGB grid required')
        old_driver.checked_view(rgb, count*rgb_width*rgb_height*3)
        image_io = old_driver.RawImageIO(Image, rgb, rgb_width, rgb_height)
        image_profile = 'diagnostic_rgb_only'
    loads = {}
    for k, sub in enumerate(('depth', 'conf0', 'conf1', 'conf2')):
        old_driver.checked_view(banks[k], count*1536*1152*4)
        bank = np.frombuffer(banks[k], dtype='<f4').reshape(count, 1152, 1536)
        for i in range(count):
            loads[f'{old_driver._B}/{sub}/{i:04d}.npy'] = bank[i]
    adapter = SelectedFusionArrayIO(np, loads, cameras, retained, frame_ids,
                                    selected_dir, mask_callback, original_indices,
                                    plan_sha256, code['fusion_defs'], live_callback)
    def original_open(path, mode='r', *args, **kwargs):
        if path == old_driver._FX+'/frames.json' and mode == 'r':
            return io.StringIO(json.dumps(dict(count=count, width=1536, height=1152)))
        if path == '/reconstruction-original-raw/output_m2/order.json' and mode == 'w':
            return (target/'order.json').open('x', *args, **kwargs)
        raise ValueError('unexpected original open IO')
    scope = dict(np=adapter, Image=image_io, json=json, open=original_open,
                 os=old_driver.OutputDirectoryIO(retained),
                 ProcessPoolExecutor=old_driver.SynchronousOrderedExecutor,
                 FX=old_driver._FX, B=old_driver._B, OUT='/reconstruction-original-raw/output',
                 a=types.SimpleNamespace(geo_pixel=.125, mask_thres=[2], sat_tol=1e-5, min_tri_angle=3.),
                 __name__='__main__')
    filter_scope = dict(np=np, cv2=cv2)
    try:
        selected_target.mkdir(mode=0o700)
        exec(code['filter'], filter_scope)
        scope['check_geometric_consistency'] = filter_scope['check_geometric_consistency']
        exec(code['fusion_setup'], scope)
        exec(code['fusion_defs'], scope)
        adapter.bind_original_one(scope['one'])
        exec(code['fusion_main'], scope)
        original_total = int(scope['tot'][2])
        if adapter.written != set(range(count)) or len(adapter.rows) != count or adapter.original_kept_total != original_total:
            raise RuntimeError('incomplete selected/original retained outputs or original total mismatch')
        manifest = dict(schema='reconstruction_selected_retained/1', profile='reference_pipeline_selected_transport_v1',
                        plan_sha256=plan_sha256, source_identity=identity,
                        image_profile=image_profile, order=list(range(count)),
                        runtime=dict(python=list(sys.version_info[:3]), numpy=np.__version__,
                                     opencv=cv2.__version__, pillow=Image.__version__),
                        input_identity=dict(
                            banks=[dict(kind=kind, bytes=bank.nbytes, sha256=hashlib.sha256(bank).hexdigest())
                                   for kind, bank in zip(('depth', 'conf0', 'conf1', 'conf2'), banks)],
                            cameras_sha256=hashlib.sha256(cameras).hexdigest(),
                            frame_ids_sha256=hashlib.sha256(frame_ids).hexdigest(),
                            original_indices_sha256=hashlib.sha256(original_indices).hexdigest()),
                        original_indices=list(indices),
                        frame_id_f64_bits=[frame_ids.cast('B')[i*8:(i+1)*8].tobytes()[::-1].hex() for i in range(count)],
                        kept_total=original_total, original_kept_total=original_total,
                        selected_kept_total=adapter.selected_kept_total,
                        pixel_order='C-row-major original final-valid boolean indexing',
                        original_retained=str(target), frames=adapter.rows)
        with (selected_target/'selection_manifest.json').open('x') as stream:
            json.dump(manifest, stream, separators=(',', ':')); stream.write('\n')
        return original_total, adapter.selected_kept_total
    finally:
        scope.clear(); filter_scope.clear(); loads.clear()
