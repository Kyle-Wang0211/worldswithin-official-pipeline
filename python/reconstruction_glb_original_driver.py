"""IO-only adapter to the frozen selected 38 Python driver.

Not an iOS runtime or parity acceptance. ARM NumPy sorting differs from the
historical AVX512 route; final-phone byte gates remain mandatory.
"""
import hashlib
import pathlib
import sys

def export(obj_path, normal_directory, output_glb_path):
    import numpy
    import trimesh
    from PIL import Image
    if (sys.version_info[:3], numpy.__version__, trimesh.__version__, Image.__version__) != ((3, 10, 21), "1.26.4", "5.1.0", "12.3.0"):
        raise RuntimeError("selected official Python package versions mismatch")
    raw = pathlib.Path(__file__).with_name("selected_original_glb_driver.py").read_bytes()
    if hashlib.sha256(raw).hexdigest() != "ef1df4e66bbda6c5dc28c9f2593ac8b70daf307be9d518f1d920eff6697f5ce9":
        raise RuntimeError("selected original GLB driver source mismatch")
    source = raw.decode("utf-8")
    replacements = [
        ('sc = trimesh.load(f"{O}/tex_{m}_1920/{m}.obj", force="scene", process=False)',
         'sc = trimesh.load(' + repr(str(obj_path)) + ', force="scene", process=False)'),
        ('O = "workspace"; m = "mesh"; N = "normals"',
         'O = "workspace"; m = "mesh"; N = ' + repr(str(normal_directory))),
        ('p = f"output.glb"; out.export(p)',
         'p = ' + repr(str(output_glb_path)) + '; out.export(p)'),
    ]
    for old, new in replacements:
        if source.count(old) != 1:
            raise RuntimeError("selected original driver IO anchor mismatch")
        source = source.replace(old, new, 1)
    exec(compile(source, "selected_original_glb_driver.py", "exec"), {"__name__": "__main__"})

