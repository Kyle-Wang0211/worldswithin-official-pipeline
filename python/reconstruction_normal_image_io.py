"""Parallel Pillow input IO for accepted regenerated full12MP normal material.
No EXIF orientation, resize, selection, pose mapping or reconstruction operation.
The existing pinned runtime imports this module from its native script directory.
"""
import hashlib
import json
import os
import re
import stat
import sys

_COUNT = 97
_LEGACY_SIZE = (4032, 3024)
_SHA = re.compile(r"[0-9a-f]{64}\Z")
                                                                   
_PNG_LEVEL = 1
_WORKERS = 4


def _absolute(path):
    if not isinstance(path, str) or not path or not os.path.isabs(path):
        raise ValueError("absolute IO path required")
    return os.path.abspath(path)


def _digest(stream):
    h = hashlib.sha256()
    count = 0
    for block in iter(lambda: stream.read(1024 * 1024), b""):
        count += len(block)
        h.update(block)
    return count, h.hexdigest()


def _receipt(stream, document):
    stream.seek(0)
    json.dump(document, stream, ensure_ascii=True, indent=2)
    stream.write("\n")
    stream.truncate()
    stream.flush()
    os.fsync(stream.fileno())


def _source_size(frame, *, actual_source_dimensions):
    size = frame.get("source_pixel_size")
    if size is None:
        return None if actual_source_dimensions else _LEGACY_SIZE
    if (not isinstance(size, list) or len(size) != 2 or
            any(type(dimension) is not int or dimension <= 0 for dimension in size)):
        raise ValueError("positive integer source_pixel_size required")
    return tuple(size)


def _prepare(input_manifest_json_path, output_directory, receipt_path, *, expected_count=97, actual_source_dimensions=False):
    """Return expected_count only after every source and generatedPNG is verified.

    Manifest: top-level array or {"frames": array}, exact expected ordered records
    {pack_index:0..N-1,path:absoluteJPEG,source_bytes:positiveInt,
     source_sha256:lowercase64hex}. Other identity fields may accompany records.
    Output dir is created exclusively, must not exist; receipt is opened with x.
    Both require existing parents. Receipt may be inside the new output dir.
    Partial PNGs, output directory and failure receipt are never removed.
    """
    output_directory = _absolute(output_directory)
    receipt_path = _absolute(receipt_path)
    input_manifest_json_path = _absolute(input_manifest_json_path)
    if os.path.lexists(output_directory):
        raise FileExistsError("normal output directory must be fresh: " + output_directory)
    if os.path.lexists(receipt_path):
        raise FileExistsError("normal receipt must be fresh: " + receipt_path)
    if receipt_path == output_directory:
        raise ValueError("receipt cannot equal output directory")
                                                                              
    os.mkdir(output_directory, 0o700)
    doc = {
        "schema": "reconstruction.normal_image_io.v1",
        "status": "in_progress",
        "accepted_regenerated_material": True,
        "producer": "official Pillow12.3.0 Image.open.convert(RGB).save(PNG,compress_level=1) x4 threads",
        "historical_png_byte_parity": "not_claimed",
        "orientation_operation": "none",
        "resize_operation": "none",
        "expected_count": expected_count,
        "source_pixel_size_policy": "actual_per_frame" if actual_source_dimensions else "legacy4032x3024",
        "input_manifest_json_path": input_manifest_json_path,
        "output_directory": output_directory,
        "frames": [],
    }
    with open(receipt_path, "x", encoding="utf-8") as report:
        _receipt(report, doc)
        current_index = None
        try:
            if sys.version_info[:3] != (3, 10, 21):
                raise RuntimeError("normal image IO requires CPython3.10.21")
            import PIL
            from PIL import Image
            if PIL.__version__ != "12.3.0":
                raise RuntimeError("normal image IO requires Pillow12.3.0")
            with open(input_manifest_json_path, "rb") as source_manifest:
                manifest_bytes = source_manifest.read()
            doc["input_manifest_sha256"] = hashlib.sha256(manifest_bytes).hexdigest()
            payload = json.loads(manifest_bytes)
            frames = payload.get("frames") if isinstance(payload, dict) else payload
            if not isinstance(frames, list) or len(frames) != expected_count:
                raise ValueError("normal manifest must contain exact expected ordered frames")
            checked = []
            for index, frame in enumerate(frames):
                if not isinstance(frame, dict):
                    raise TypeError("normal frame must be an object")
                pack = frame.get("pack_index")
                length = frame.get("source_bytes")
                digest = frame.get("source_sha256")
                if type(pack) is not int or pack != index:
                    raise ValueError("pack_index must follow actual ordered frame rows")
                if type(length) is not int or length <= 0:
                    raise ValueError("source_bytes must be a positive integer")
                if not isinstance(digest, str) or not _SHA.fullmatch(digest):
                    raise ValueError("source_sha256 must be lowercase64hex")
                path = _absolute(frame.get("path"))
                if os.path.commonpath((output_directory, path)) == output_directory:
                    raise ValueError("source JPEG cannot reside in the fresh output directory")
                checked.append((index, path, length, digest))
            from concurrent.futures import ThreadPoolExecutor
            rows = [{"pack_index": index, "source_path": path,
                     "source_bytes": expected_bytes, "source_sha256": expected_sha}
                    for index, path, expected_bytes, expected_sha in checked]
            for row, frame in zip(rows, frames):
                row["expected_source_pixel_size"] = _source_size(frame, actual_source_dimensions=actual_source_dimensions)
            with ThreadPoolExecutor(min(_WORKERS, len(checked))) as pool:
                futures = [pool.submit(_one, Image, output_directory, row) for row in rows]
                for row, future in zip(rows, futures):
                    current_index = row["pack_index"]
                    doc["frames"].append(row)
                    future.result()
                    _receipt(report, doc)
            with open(input_manifest_json_path, "rb") as final_manifest:
                if final_manifest.read() != manifest_bytes:
                    raise RuntimeError("normal source manifest changed during image IO")
            doc["status"] = "complete"
            doc["prepared_count"] = len(doc["frames"])
            doc["return_code"] = 0
            _receipt(report, doc)
            return expected_count
        except Exception as error:
            doc["status"] = "failed"
            doc["return_code"] = 1
            doc["failed_pack_index"] = current_index
            doc["prepared_count"] = sum(row.get("status") == "prepared" for row in doc["frames"])
            doc["error"] = type(error).__name__ + ": " + str(error)
            try:
                _receipt(report, doc)
            except Exception as receipt_error:
                raise RuntimeError("normal image IO failed at pack_index=%r: %s; receipt update failed: %s" %
                                   (current_index, error, receipt_error)) from error
            raise RuntimeError("normal image IO failed at pack_index=%r: %s" %
                               (current_index, error)) from error


def _one(Image, output_directory, row):
    index, path = row["pack_index"], row["source_path"]
    expected_bytes, expected_sha = row["source_bytes"], row["source_sha256"]
                                                                 
                                                                    
    with open(path, "rb") as jpeg:
        before = os.fstat(jpeg.fileno())
        if not stat.S_ISREG(before.st_mode):
            raise ValueError("source JPEG must be a regular file")
        actual_bytes, actual_sha = _digest(jpeg)
        row["actual_source_bytes"] = actual_bytes
        row["actual_source_sha256"] = actual_sha
        if actual_bytes != expected_bytes or actual_sha != expected_sha:
            raise ValueError("source JPEG bytes/SHA256 mismatch")
        jpeg.seek(0)
        with Image.open(jpeg) as image:
            if image.format != "JPEG":
                raise ValueError("source material must be JPEG")
            row["source_pixel_size"] = list(image.size)
            expected_size = row["expected_source_pixel_size"]
            if expected_size is not None and image.size != expected_size:
                raise ValueError("source JPEG dimensions differ from manifest")
            out = os.path.join(output_directory, str(index) + ".png")
            with image.convert("RGB") as rgb:
                                                                   
                with open(out, "xb") as png:
                    row["actualpath"] = out
                    rgb.save(png, format="PNG", compress_level=_PNG_LEVEL)
                    png.flush()
                    os.fsync(png.fileno())
        after = os.fstat(jpeg.fileno())
        if (before.st_size, before.st_mtime_ns, before.st_ctime_ns) != (
                after.st_size, after.st_mtime_ns, after.st_ctime_ns):
            raise RuntimeError("source JPEG changed during decode")
        jpeg.seek(0)
        final_bytes, final_sha = _digest(jpeg)
        if final_bytes != expected_bytes or final_sha != expected_sha:
            raise RuntimeError("source JPEG changed after decode")
                                                                   
    with open(out, "rb") as png:
        row["bytes"], row["sha256"] = _digest(png)
        png.seek(0)
        with Image.open(png) as generated:
            row["size"] = list(generated.size)
            if generated.format != "PNG" or list(generated.size) != row["source_pixel_size"] or generated.mode != "RGB":
                raise RuntimeError("generatedPNG format/size/mode mismatch")
            generated.verify()
    row["status"] = "prepared"



def manifest_frame_count(input_manifest_json_path):
    """Native status wrapper derives expected N from validated existing JSON rows."""
    path = _absolute(input_manifest_json_path)
    with open(path, "rb") as source:
        payload = json.loads(source.read())
    frames = payload.get("frames") if isinstance(payload, dict) else payload
    if not isinstance(frames, list) or not frames:
        raise ValueError("positive actual-N normal manifest required")
    for index, frame in enumerate(frames):
        if not isinstance(frame, dict):
            raise ValueError("normal frame object required")
        if type(frame.get("pack_index")) is not int or frame["pack_index"] != index:
            raise ValueError("normal pack_index must follow ordered actual manifest")
        if type(frame.get("source_bytes")) is not int or frame["source_bytes"] <= 0:
            raise ValueError("positive original JPEG byte length required")
        digest = frame.get("source_sha256")
        if not isinstance(digest, str) or not _SHA.fullmatch(digest):
            raise ValueError("exact original JPEG SHA256 required")
        _absolute(frame.get("path"))
        _source_size(frame, actual_source_dimensions=True)
    return len(frames)


def prepare(input_manifest_json_path, output_directory, receipt_path):
    """Legacy fixed97 entry retained."""
    return _prepare(input_manifest_json_path, output_directory, receipt_path)


def prepare_v2(input_manifest_json_path, output_directory, receipt_path):
    """Explicit actual-N entry, bound to the validated original manifest rows."""
    count = manifest_frame_count(input_manifest_json_path)
    return _prepare(input_manifest_json_path, output_directory, receipt_path,
                    expected_count=count, actual_source_dimensions=True)
