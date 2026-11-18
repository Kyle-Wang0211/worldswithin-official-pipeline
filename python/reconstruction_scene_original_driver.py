"""Camera-only IO adapter: exact recovered original source spans.
No JPG decode/resize/encoding. Corrected1920 JPEG production remains external.
"""
import builtins
import glob
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import sys

_ORIGINAL_SHA = "2763ac2da869a8bc3ee9cd8e9a72b8715bb6598227ec09b339d8b4cec3eb8e31"
_CAMERA_SHA = "998f16b3a3228717a3fa9313fd1427cdac1861bc2bac57b6fe6301634fec0652"
_CORRECTION_SHA = "878e6602bce696b63675c9bec183afda995f6d08d73e0e6fd701be94afffc9e5"
_CORRECTION_SOURCE = 'for cam in sorted(glob.glob(f"{src}/*.cam")):\n    b = os.path.basename(cam)[:-4]; l1, l2 = open(cam).read().strip().split("\\n")\n    f, d0, d1, pa, ppx, ppy = [float(x) for x in l2.split()]\n    ppx2, ppy2 = ppx - 0.5 / 768, ppy - 0.5 / 576          # (cx+0.5)/768 -> cx/768 ; (cy+0.5)/576 -> cy/576\n    open(f"{dst}/{b}.cam", "w").write(l1 + "\\n" + " ".join(f"{v:.17g}" for v in (f, d0, d1, pa, ppx2, ppy2)) + "\\n")\n'
_SHA = re.compile(r"[0-9a-f]{64}\Z")


def _codes():
    path = Path(__file__).with_name("selected_original_mve_scene.py")
    raw = path.read_bytes()
    span = b"".join(raw.splitlines(keepends=True)[12:18])
    if hashlib.sha256(raw).hexdigest() != _ORIGINAL_SHA or hashlib.sha256(span).hexdigest() != _CAMERA_SHA:
        raise RuntimeError("recorded original camera source/span identity mismatch")
    correction = _CORRECTION_SOURCE.encode("utf-8")
    if hashlib.sha256(correction).hexdigest() != _CORRECTION_SHA:
        raise RuntimeError("recorded original correction span identity mismatch")
    return compile(span, "mve_scene.py:13-18", "exec"), compile(correction, "recorded_camera_correction:8-12", "exec")


def _absolute(path):
    if not isinstance(path, str) or not path.startswith("/") or path == "/" or "\x00" in path:
        raise ValueError("absolute nonroot IO path required")
    p = Path(path)
    if str(p) != path or any(part in (".", "..") for part in p.parts):
        raise ValueError("canonical IO path required")
    for item in (p, *p.parents):
        if item.is_symlink():
            raise ValueError("symlink IO path rejected")
    return p


def _artifact(root, path, role):
    path = _absolute(str(path))
    rel = path.relative_to(root).as_posix()
    if not path.is_file():
        raise ValueError("regular actual material required")
    digest = hashlib.sha256()
    count = 0
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            count += len(block); digest.update(block)
    if count <= 0:
        raise ValueError("empty material")
    return {"path": rel, "role": role, "bytes": count, "sha256": digest.hexdigest()}


def _same(expected, actual):
    if any(expected.get(key) != actual[key] for key in ("path", "role", "bytes", "sha256")):
        raise ValueError("source JPEG material identity mismatch")


def _json(stream, document):
    stream.seek(0)
    json.dump(document, stream, ensure_ascii=True, indent=2)
    stream.write("\n"); stream.truncate(); stream.flush(); os.fsync(stream.fileno())


def _fresh_json(path, document):
    with path.open("x", encoding="utf-8") as stream:
        _json(stream, document)


def _camera_frame_count(camera_buffer):
    if (sys.byteorder != "little" or not isinstance(camera_buffer, memoryview)
            or not camera_buffer.readonly or not camera_buffer.c_contiguous
            or camera_buffer.nbytes % (36 * 4)):
        raise ValueError("exact readonly little endian camera f32 rows required")
    count = camera_buffer.nbytes // (36 * 4)
    if count <= 0:
        raise ValueError("actual camera count must be positive")
    return count


def _ordered_transport_count(frames, sources, manifest, count):
    if (not isinstance(frames, list) or len(frames) != count
            or not isinstance(sources, list) or len(sources) != count
            or not isinstance(manifest.get("frame_id_f64_bits"), list)
            or len(manifest["frame_id_f64_bits"]) != count):
        raise ValueError("exact actual-N ordered frame/source transport required")
    for index, (frame, source) in enumerate(zip(frames, sources)):
        frame_id = frame["frame_id"]
        if (isinstance(frame_id, bool) or not isinstance(frame_id, (int, float))
                or not math.isfinite(frame_id)
                or manifest["frame_id_f64_bits"][index] != struct.pack(">d", frame_id).hex()
                or frame["jpeg_path"] != source["path"]):
            raise ValueError("binary64 frame/source order mismatch")
    return count

def _execute(np, camera_buffer, manifest_path, input_root, original_dir, corrected_dir, receipt_path):
    count = _camera_frame_count(camera_buffer)
    root = _absolute(input_root); manifest_path = _absolute(manifest_path)
    original = _absolute(original_dir); corrected = _absolute(corrected_dir); receipt = _absolute(receipt_path)
    original.relative_to(root); corrected.relative_to(root); receipt.relative_to(root); manifest_path.relative_to(root)
    if original == corrected or original in corrected.parents or corrected in original.parents:
        raise ValueError("disjoint camera output directories required")
    source_path = Path(str(receipt) + ".sources.json")
    command_path = Path(str(receipt) + ".command.json")
    for path in (original, corrected, receipt, source_path, command_path):
        if path.exists() or path.is_symlink() or not path.parent.is_dir():
            raise FileExistsError("fresh camera output with existing parent required")
    manifest_bytes = manifest_path.read_bytes(); manifest = json.loads(manifest_bytes)
    transport = manifest["dense_frames_json"].encode("utf-8")
    frame_sha = hashlib.sha256(transport).hexdigest()
    camera_sha = hashlib.sha256(camera_buffer).hexdigest()
    if manifest.get("schema") != "RECONSTRUCTION_PRODUCTION_CAMERA_SOURCE_V1" or manifest.get("dense_frames_sha256") != frame_sha or manifest.get("cameras_f32_sha256") != camera_sha:
        raise ValueError("actual frame/camera transport hash binding required")
    frames = json.loads(transport); sources = manifest["source_jpegs"]
    _ordered_transport_count(frames, sources, manifest, count)
    previous = None
    for i, (frame, source) in enumerate(zip(frames, sources)):
        frame_id = frame["frame_id"]
        if isinstance(frame_id, bool) or not isinstance(frame_id, (int, float)) or not math.isfinite(frame_id) or previous is not None and frame_id <= previous:
            raise ValueError("unique increasing finite frame IDs required")
        previous = frame_id
        bits = struct.pack(">d", frame_id).hex()
        if manifest["frame_id_f64_bits"][i] != bits or frame["jpeg_path"] != source["path"]:
            raise ValueError("binary64 frame/source order mismatch")
        if any(not math.isfinite(v) for key in ("q_wxyz", "t") for v in frame[key]):
            raise ValueError("nonfinite real refined pose")
        for key in ("fx", "fy", "cx", "cy", "image_w", "image_h"):
            if not math.isfinite(frame[key]): raise ValueError("nonfinite real camera intrinsics")
        if len(frame["q_wxyz"]) != 4 or len(frame["t"]) != 3:
            raise ValueError("real pose arity mismatch")
        _same(source, _artifact(root, root / source["path"], "source_jpeg"))
    CM = np.frombuffer(camera_buffer, np.float32).reshape(count, 36).astype(np.float64)
    if not np.isfinite(CM).all() or (CM[:, 0] <= 0).any() or (CM[:, 4] <= 0).any() or not (CM[:, 26] == 768).all() or not (CM[:, 27] == 576).all():
        raise ValueError("finite selected768x576 prepared camera rows required")
    camera_code, correction_code = _codes()
    original.mkdir(mode=0o700); corrected.mkdir(mode=0o700)
    doc = {"schema": "RECONSTRUCTION_PRODUCTION_CAMERA_RECEIPT_V1", "status": "in_progress", "return_code": 1,
           "ordered_frames": count, "dense_frames_sha256": frame_sha, "cameras_f32_sha256": camera_sha,
           "input_manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(), "material_outputs": [],
           "image_producer_status": "REQUIRED_EXTERNAL_ORIGINAL_OPENCV1920",
           "original_camera_normalization": "768x576 intermediate; reference_pipeline texture and normal consumers use corrected normalized CAM bytes",
           "runtime": {"python": sys.version, "numpy": np.__version__},
           "source": {"original_sha256": _ORIGINAL_SHA, "camera_span_sha256": _CAMERA_SHA, "correction_span_sha256": _CORRECTION_SHA}}
    with receipt.open("x", encoding="utf-8") as report:
        _json(report, doc)
        try:
                                                                                
                                                                    
            def camera_open(path, mode="r", *args, **kwargs):
                p = _absolute(path)
                if mode == "w" and p.parent in (original, corrected):
                    return builtins.open(p, "x", *args, **kwargs)
                if mode == "r" and p.parent == original:
                    return builtins.open(p, mode, *args, **kwargs)
                raise ValueError("unexpected original camera IO")
            scope = {"CM": CM, "NF": count, "W": 768, "H": 576, "OUT": str(original), "open": camera_open}
            try: exec(camera_code, scope)
            finally: scope.clear()
            scope = {"os": os, "glob": glob, "src": str(original), "dst": str(corrected), "open": camera_open}
            try: exec(correction_code, scope)
            finally: scope.clear()
            for i, source in enumerate(sources):
                _same(source, _artifact(root, root / source["path"], "source_jpeg"))
                doc["material_outputs"].extend((_artifact(root, original / f"{i:08d}.cam", "original_uncorrected_camera_intermediate"),
                                                _artifact(root, corrected / f"{i:08d}.cam", "texture_camera")))
            if hashlib.sha256(camera_buffer).hexdigest() != camera_sha or manifest_path.read_bytes() != manifest_bytes:
                raise RuntimeError("camera/manifest input changed during original producer")
            _fresh_json(source_path, {"schema": "RECONSTRUCTION_PRODUCTION_SOURCE_MANIFEST_V1", "dense_frames_sha256": frame_sha, "source_jpegs": sources})
            _fresh_json(command_path, {"schema": "RECONSTRUCTION_ACTUAL_CAMERA_COMMAND_V1", "native_entry": "reconstruction_scene_cameras_prepare", "driver": "reconstruction_scene_original_driver.prepare",
                "arguments": {"cameras_f32_count": count * 36, "input_manifest_path": str(manifest_path), "input_root": str(root), "original_camera_dir": str(original), "corrected_camera_dir": str(corrected), "receipt_path": str(receipt)},
                "sources": doc["source"], "driver_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), "runtime": doc["runtime"],
                "transport_adaptations": ["np.fromfile replaced by borrowed readonly native f32 buffer; exactreshape and astype(float64)", "originaljpg symlinks and print diagnostics omitted in camera-only span", "open(w) becomes exclusiveopen(x); caller-selected path globals", "uncorrected768-normalized camera retained only as intermediate evidence; reference_pipeline av_vis_k reads texture_scene_fix"], "return_code": 0})
            doc["source_manifest"] = _artifact(root, source_path, "source_manifest")
            doc["camera_producer_command"] = _artifact(root, command_path, "camera_producer_command")
            doc["status"] = f"complete{count}_cameras_only"; doc["return_code"] = 0
            _json(report, doc)
            return count
        except Exception as error:
            doc["status"] = "failed"; doc["error"] = type(error).__name__ + ": " + str(error)
            _json(report, doc)
            raise


def prepare(camera_buffer, manifest_path, input_root, original_dir, corrected_dir, receipt_path):
    import numpy as np
    if (sys.version_info[:3], np.__version__) != ((3, 10, 21), "1.26.4"):
        raise RuntimeError("original camera CPython3.10.21/NumPy1.26.4 runtime required")
    return _execute(np, camera_buffer, manifest_path, input_root, original_dir, corrected_dir, receipt_path)
