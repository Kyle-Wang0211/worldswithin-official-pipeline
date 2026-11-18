"""IO-only raw-buffer adapter; numerical span is the original selected source."""
import hashlib
import pathlib
import sys

_FULL_SHA256 = "16a139fdb91e8aa9cca33fd96ecda78983e81b3b8b1b85b48037148d5168a563"
_SPAN_SHA256 = "660eba5535d276ab9c4070ce073ea4dbc9a41b2f56432d9b436d0be9cd189f02"
_BEGIN = 'd = z["d16"].astype(np.float64) / 5000.0; K = z["intr"]; E = z["extr"]'
_END = '    vdb_volume.integrate(np.ascontiguousarray(Xw), extrinsic=cam_center)'

def preprocess(depth_buffer, width, height, k_buffer, e_buffer):
    import numpy as np
    if (sys.version_info[:3], np.__version__) != ((3, 10, 21), "1.26.4"):
        raise RuntimeError("selected original TSDF NumPy runtime mismatch")
    if sys.byteorder != "little" or np.dtype(np.float64).itemsize != 8:
        raise RuntimeError("selected original raw input ABI mismatch")
    if not isinstance(width, int) or not isinstance(height, int) or width <= 0 or height <= 0:
        raise ValueError("invalid depth shape")
    if width > sys.maxsize // height or width * height > sys.maxsize // 2:
        raise OverflowError("depth buffer byte length overflow")
    for view, expected in ((depth_buffer, width * height * 2), (k_buffer, 72), (e_buffer, 128)):
        if not isinstance(view, memoryview) or not view.readonly or not view.c_contiguous or view.nbytes != expected:
            raise ValueError("readonly raw input buffer shape/length mismatch")
    source = pathlib.Path(__file__).with_name("selected_original_tsdf_driver.py").read_bytes()
    if hashlib.sha256(source).hexdigest() != _FULL_SHA256:
        raise RuntimeError("selected original TSDF source identity mismatch")
    text = source.decode("utf-8")
    if text.count(_BEGIN) != 1 or text.count(_END) != 1:
        raise RuntimeError("selected original NumPy source anchors mismatch")
                                                                         
    original = text[text.index(_BEGIN):text.index(_END, text.index(_BEGIN))].splitlines()
    if len(original) != 6 or any(not line.startswith("    ") for line in original[1:]):
        raise RuntimeError("selected original NumPy span layout mismatch")
    selected = original[0] + "\n" + "\n".join(line[4:] for line in original[1:]) + "\n"
    if hashlib.sha256(selected.encode("utf-8")).hexdigest() != _SPAN_SHA256:
        raise RuntimeError("selected original NumPy span identity mismatch")
    z = {
        "d16": np.frombuffer(depth_buffer, dtype="<u2", count=width * height).reshape((height, width)),
        "intr": np.frombuffer(k_buffer, dtype=np.float64, count=9).reshape((3, 3)),
        "extr": np.frombuffer(e_buffer, dtype=np.float64, count=16).reshape((4, 4)),
    }
    scope = {"np": np, "z": z}
    exec(compile(selected, "selected_original_numpy_preprocess_span.py", "exec"), scope)
                                                                                  
    return np.ascontiguousarray(scope["Xw"]), scope["cam_center"]
