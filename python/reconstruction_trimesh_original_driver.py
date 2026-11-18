"""Synchronous IO-only bridge to hash-guarded original Trimesh operations."""
import hashlib
import pathlib
import sys

_SOURCE_SHA256 = "16a139fdb91e8aa9cca33fd96ecda78983e81b3b8b1b85b48037148d5168a563"
_SELECTED = "geo_mesh = trimesh.Trimesh(vertices, faces)\ngeo_mesh.export(out)"

def export(vertices_buffer, nv, faces_buffer, nf, output_path):
    import numpy as np
    import trimesh
    if (sys.version_info[:3], np.__version__, trimesh.__version__) != ((3, 10, 21), "1.26.4", "5.1.0"):
        raise RuntimeError("selected original TSDF Python package versions mismatch")
    if sys.byteorder != "little" or np.dtype(np.float64).itemsize != 8 or np.dtype(np.int32).itemsize != 4:
        raise RuntimeError("selected original raw geometry ABI mismatch")
    if not isinstance(nv, int) or not isinstance(nf, int) or nv < 0 or nf < 0:
        raise ValueError("invalid raw geometry counts")
    if nv > sys.maxsize // 24 or nf > sys.maxsize // 12:
        raise OverflowError("raw geometry byte length overflow")
    if not isinstance(vertices_buffer, memoryview) or not isinstance(faces_buffer, memoryview):
        raise TypeError("raw geometry requires memoryviews")
    if not vertices_buffer.readonly or not faces_buffer.readonly or not vertices_buffer.c_contiguous or not faces_buffer.c_contiguous:
        raise ValueError("raw geometry buffers must be readonly and contiguous")
    if vertices_buffer.nbytes != nv * 24 or faces_buffer.nbytes != nf * 12:
        raise ValueError("raw geometry buffer length mismatch")
    raw = pathlib.Path(__file__).with_name("selected_original_tsdf_driver.py").read_bytes()
    if hashlib.sha256(raw).hexdigest() != _SOURCE_SHA256:
        raise RuntimeError("selected original TSDF driver source mismatch")
    source = raw.decode("utf-8")
    if source.count(_SELECTED) != 1:
        raise RuntimeError("selected original Trimesh source anchor mismatch")
                                                                              
    vertices = np.frombuffer(vertices_buffer, dtype=np.float64, count=nv * 3).reshape((nv, 3))
    faces = np.frombuffer(faces_buffer, dtype=np.int32, count=nf * 3).reshape((nf, 3))
    scope = {"trimesh": trimesh, "vertices": vertices, "faces": faces, "out": str(output_path)}
                                                                              
    selected = source[source.index(_SELECTED):source.index(_SELECTED) + len(_SELECTED)]
    exec(compile(selected, "selected_original_tsdf_driver.py", "exec"), scope)
    geo_mesh = scope["geo_mesh"]
    return len(geo_mesh.vertices), len(geo_mesh.faces)
