"""Execute SHA-checked visibility/SfM spans with actual source image dimensions.

Original source hashes authenticate the recovered inputs. Derived span hashes
authenticate the explicit source-dimension substitutions made at this boundary.
"""
import builtins
import hashlib
import json
import pathlib
import sys
import types

_SPANS = {
    "vis_mesh": ("av_vis_k.py", "a74d15d87b2870295146150f39c103d217ddfccc1fa6a91604247b68791698f8", 10, 10,
                 "43f9f8dc8fe92cb634de931269f5a6825ea9d9485f1e5887b3bb1f00f11ca82d"),
    "vis_body": ("av_vis_k.py", "a74d15d87b2870295146150f39c103d217ddfccc1fa6a91604247b68791698f8", 12, 31,
                 "6e04904c1104524e7b2411943a175983eb5234d4589b2dab57e4ee372b5bba98"),
    "sfm_body": ("av_sfm_k.py", "58fbf9612797cdbf08d79710dcb746322d025d1e80215fa530683abd0c4ea2e1", 7, 31,
                 "3e913668e6c38b25ef74ef4fc34c53407afbe59100e4da7d2df404a929e39bba"),
}
_O = "workspace"
_C = _O + "/visibility_cache"
_SC = "scene"
_KEY = "original_numpy"
_DIMENSION_SPAN_SHA256 = {
    "vis_body": "91c37f8d2903c5d380162874a5bf53c9c2880bdb0c0f902187db59a7841e3225",
    "sfm_body": "792ad32461a7fb28ea91834c5b450122c91868475685f12c1411dc02039df2f4",
}

def _span(name):
    filename, full_hash, lo, hi, span_hash = _SPANS[name]
    raw = pathlib.Path(__file__).with_name("selected_original_" + filename).read_bytes()
    if hashlib.sha256(raw).hexdigest() != full_hash:
        raise RuntimeError("original source identity mismatch: " + filename)
    selected = b"".join(raw.splitlines(keepends=True)[lo - 1:hi])
    if hashlib.sha256(selected).hexdigest() != span_hash:
        raise RuntimeError("original execution span identity mismatch: " + name)
    if name in _DIMENSION_SPAN_SHA256:
        text = selected.decode("utf-8")
        replacements = {
            "vis_body": [(
                "f=b[0] * 4032, cx=b[4] * 4032, cy=b[5] * 3024",
                "f=b[0] * SOURCE_SIZES[i][0], cx=b[4] * SOURCE_SIZES[i][0], cy=b[5] * SOURCE_SIZES[i][1]",
            )],
            "sfm_body": [
                ("W, H = 4032, 3024", "W, H = SOURCE_SIZES[0]"),
                ("for i, c in enumerate(cams):\n",
                 "for i, c in enumerate(cams):\n    W, H = SOURCE_SIZES[i]\n"),
            ],
        }
        for original, actual_dimensions in replacements[name]:
            if text.count(original) != 1:
                raise RuntimeError("source dimension substitution boundary changed: " + name)
            text = text.replace(original, actual_dimensions, 1)
        selected = text.encode("utf-8")
        if hashlib.sha256(selected).hexdigest() != _DIMENSION_SPAN_SHA256[name]:
            raise RuntimeError("actual-dimension derived span identity mismatch: " + name)
        filename += ":actual_source_dimensions"
    return compile(selected, filename + ":" + str(lo), "exec")

def _source_sizes(images, count):
    from PIL import Image
    sizes = []
    for index in range(count):
        with Image.open(pathlib.Path(images) / (str(index) + ".png")) as image:
            if (image.format != "PNG" or image.mode != "RGB" or
                    any(dimension <= 0 for dimension in image.size)):
                raise ValueError("actual positive RGB PNG source dimensions required")
            sizes.append(image.size)
            image.verify()
    return sizes

def _readonly(view, expected):
    if (not isinstance(view, memoryview) or not view.readonly or
            not view.c_contiguous or view.nbytes != expected):
        raise ValueError("readonly raw input buffer length/layout mismatch")

class _Mesh:
    def __init__(self, vertices, colors):
        self.vertices, self.vertex_colors = vertices, colors
    def has_vertex_colors(self):
        return self.vertex_colors is not None

class _ArrayIO:
    def __init__(self, np, frames, gate):
        self.np, self.frames, self.gate = np, frames, gate
        self.saved = None
    def __getattr__(self, name):
        return getattr(self.np, name)
    def load(self, path):
        if path == _O + "/av_vis_" + _KEY + ".npz":
            if self.saved is None:
                raise RuntimeError("original visibility output was not generated")
            return self.saved
        for i, (depth, width, height, k, e, _) in enumerate(self.frames):
            if path == _C + f"/{i:08d}.npz":
                return {
                    "d16": self.np.fromfile(depth, dtype="<u2").reshape(height, width),
                    "intr": self.np.frombuffer(k, dtype=self.np.float64, count=9).reshape(3, 3),
                    "extr": self.np.frombuffer(e, dtype=self.np.float64, count=16).reshape(4, 4),
                }
        raise ValueError("unexpected original np.load path")
    def savez(self, path, **arrays):
        if path != _O + "/av_vis_" + _KEY + ".npz" or self.saved is not None:
            raise ValueError("unexpected original np.savez path")
        names = {"V": ("V.f64le", "<f8"), "col": ("col.u8", "u1"),
                 "vi": ("vi.i64le", "<i8"), "cj": ("cj.i64le", "<i8"),
                 "ox": ("ox.f64le", "<f8"), "oy": ("oy.f64le", "<f8"),
                 "cams": ("cams.f64le", "<f8")}
        if set(arrays) != set(names):
            raise ValueError("original visibility output fields mismatch")
        for name, (filename, dtype) in names.items():
            array = arrays[name]
            if array.dtype != self.np.dtype(dtype):
                raise ValueError("original visibility output dtype mismatch: " + name)
                                                                                
            with (self.gate / filename).open("xb") as stream:
                array.tofile(stream)
        self.saved = arrays

def _frame_count(frames):
    if not isinstance(frames, tuple) or not frames:
        raise ValueError("positive actual-N ordered original frames required")
    return len(frames)

def _execute(np, vertex_buffer, color_buffer, count, frames, images, sfm, gate):
    """Internal IO execution; native/public callers enter through prepare only."""
    if sys.byteorder != "little" or np.dtype(np.float64).itemsize != 8 or np.dtype(int).itemsize != 8:
        raise RuntimeError("original raw FP64/int64 ABI mismatch")
    if not isinstance(count, int) or count <= 0 or count > sys.maxsize // 24:
        raise ValueError("invalid original vertex count")
    _readonly(vertex_buffer, count * 24)
    if color_buffer is not None:
        _readonly(color_buffer, count * 24)
    _frame_count(frames)
    for frame in frames:
        if not isinstance(frame, tuple) or len(frame) != 6:
            raise ValueError("original frame IO tuple mismatch")
        depth, width, height, k, e, camera = frame
        if (not isinstance(width, int) or not isinstance(height, int) or
                width <= 0 or height <= 0 or width > sys.maxsize // height // 2):
            raise ValueError("invalid original depth shape")
        _readonly(k, 72); _readonly(e, 128)
        for path in (depth, camera):
            if not isinstance(path, str) or not pathlib.Path(path).is_absolute():
                raise ValueError("absolute original input paths required")
        if pathlib.Path(depth).stat().st_size != width * height * 2:
            raise ValueError("original d16 file length mismatch")
    for path in (images, sfm, gate):
        if not isinstance(path, str) or not pathlib.Path(path).is_absolute():
            raise ValueError("absolute original output/image paths required")
    source_sizes = _source_sizes(images, len(frames))
    out, folder = pathlib.Path(sfm), pathlib.Path(gate)
    if out.exists() or out.is_symlink() or folder.exists() or folder.is_symlink():
        raise FileExistsError("original SfM/gate outputs must be fresh")
                                                                       
    code = {name: _span(name) for name in _SPANS}
    vertices = np.frombuffer(vertex_buffer, dtype=np.float64, count=count * 3).reshape(count, 3)
    colors = None if color_buffer is None else np.frombuffer(
        color_buffer, dtype=np.float64, count=count * 3).reshape(count, 3)
    mesh = _Mesh(vertices, colors)
    def read_mesh(path):
        if path != _O + "/mesh_" + _KEY + ".ply":
            raise ValueError("unexpected original mesh IO path")
        return mesh
    camera_paths = {_SC + f"/{i:08d}.cam": f[5] for i, f in enumerate(frames)}
    def source_open(path, mode="r", *args, **kwargs):
        if path in camera_paths and mode == "r":
            return builtins.open(camera_paths[path], mode, *args, **kwargs)
        if path == sfm and mode == "w":
                                                                                      
            return builtins.open(path, "x", *args, **kwargs)
        raise ValueError("unexpected original open IO path/mode")
    folder.mkdir(parents=True, exist_ok=False)
    array_io = _ArrayIO(np, frames, folder)
    scope = {"np": array_io, "json": json, "open": source_open,
             "o3d": types.SimpleNamespace(io=types.SimpleNamespace(read_triangle_mesh=read_mesh)),
             "O": _O, "C": _C, "SC": _SC, "k": _KEY,
             "NF": len(frames), "IMG": images, "out": sfm,
             "SOURCE_SIZES": source_sizes,
             "ROT_MODE": "c2w", "with_struct": True}
                                                                              
                                                                              
    try:
        exec(code["vis_mesh"], scope)
        exec(code["vis_body"], scope)
        exec(code["sfm_body"], scope)
    finally:
                                                                               
                                                                             
        scope.clear()

def prepare(vertex_buffer, color_buffer, count, frames, images, sfm, gate):
    import numpy as np
    import PIL
    if (sys.version_info[:3], np.__version__, PIL.__version__) != ((3, 10, 21), "1.26.4", "12.3.0"):
        raise RuntimeError("selected original visibility CPython/NumPy runtime mismatch")
    return _execute(np, vertex_buffer, color_buffer, count, frames, images, sfm, gate)
