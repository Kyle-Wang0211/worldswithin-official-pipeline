"""IO-only entry to the complete, byte-pinned original fixture producer.

The embedding owner must serialize this call with every other user of process
argv/environment. This module neither creates an interpreter nor computes camera,
neighbor, depth, resize, or JPEG formulas. Failed outputs are always retained.
"""
from __future__ import annotations

import hashlib
import json
import math
import os
from pathlib import Path
import platform
import runpy
import shutil
import sys
import threading
import time

SOURCE_SHA256 = "e18aa44182c1916364f9a7c0a9fc116d57d92cfce902abbbd7862019b23535b3"
SOURCE_NAME = "selected_original_prep_phone_fixture.py"
ENV = {"FIX_W": "768", "FIX_H": "576", "FIX_NSRC": "9", "USE_ARKIT": "0",
       "FRAME_GATE": "0", "DR_LO": "0.70", "DR_HI": "1.50", "DT_MAX": "0.10"}
PROFILE_V2 = "original_numpy_opencv_retained_v2"
LEGACY_FIXTURE_FRAME_COUNT = 97
RUNTIME = {"python": "3.10.21", "numpy": "1.26.4", "Pillow": "12.3.0"}
_LOCK = threading.RLock()


def _digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def _canonical(path, *, exists=True):
    path = Path(path)
    if not path.is_absolute() or str(path) == "/":
        raise ValueError("absolute nonroot path required")
    if any(item.is_symlink() for item in (path, *path.parents)):
        raise ValueError("symlink IO path rejected")
    if path.resolve(strict=exists) != path:
        raise ValueError("canonical IO path required")
    return path


def _row(path, relative, role):
    path = _canonical(path)
    if not path.is_file() or path.stat().st_size <= 0:
        raise ValueError("nonempty actual regular file required: " + str(path))
    size = path.stat().st_size
    sha = _digest(path)
    if path.stat().st_size != size:
        raise RuntimeError("input changed while hashing: " + str(path))
    return {"path": relative, "role": role, "bytes": size, "sha256": sha}


def _finite(values, count):
    return (isinstance(values, list) and len(values) == count
            and all(isinstance(x, (int, float)) and not isinstance(x, bool)
                    and math.isfinite(x) for x in values))


def _selection(metadata_path, fed_path, expected_frames):
    metadata = json.loads(Path(metadata_path).read_bytes())
    poses = [p for p in metadata["poses"] if p.get("registered")]
    poses.sort(key=lambda p: p["frame_id"])
    if len(poses) != expected_frames:
        raise ValueError("exact selected frame count required")
    fed = {}
    for line in Path(fed_path).read_text().splitlines():
        if line.strip():
            value = json.loads(line)
            fed[value["frameId"]] = value
    selected, ids, basenames = [], set(), set()
    for pose in poses:
        fid = pose["frame_id"]
        if (isinstance(fid, bool) or not isinstance(fid, (int, float))
                or not math.isfinite(fid) or int(fid) != fid or fid in ids
                or not _finite(pose["quat_wxyz"], 4) or not _finite(pose["t"], 3)):
            raise ValueError("unique exact-integer real refined frame identities required")
        ids.add(fid)
        name = os.path.basename(fed[fid]["jpegPath"])
        if not name.endswith(".jpg") or name in basenames:
            raise ValueError("distinct original JPEG basenames required")
        basenames.add(name)
        selected.append((fid, name))
    return selected


def _validate_sidecar(path):
    sc = json.loads(Path(path).read_bytes())
    if (not _finite(sc["intrinsics_fxfycxcy"], 4)
            or not _finite(sc["extrinsic"], 16)
            or not _finite([sc["image_w"], sc["image_h"]], 2)):
        raise ValueError("real complete original sidecar fields required")


def capture_manifest(capture_root, *, expected_frames=97):
    """Hash existing original-layout actual inputs without copying photo bytes."""
    root = _canonical(capture_root)
    if not root.is_dir():
        raise ValueError("existing original-layout input root required")
    rows = []
    for relative, role in (("official_sfm_sparse_meta.json", "refined_metadata"),
                           ("official_sfm_fed_frames.jsonl", "fed_photo_mapping"),
                           ("official_sfm_sparse.ply", "original_sparse_ply")):
        rows.append(_row(root / relative, relative, role))
    selected = _selection(root / rows[0]["path"], root / rows[1]["path"], expected_frames)
    for fid, name in selected:
        sidecar = "sidecars/" + name.replace(".jpg", ".json")
        _validate_sidecar(root / sidecar)
        rows.append(_row(root / sidecar, sidecar, "original_full_sidecar"))
        photo = "photos_jpg/" + name
        rows.append(_row(root / photo, photo, "actual_resolved_jpeg"))
    return {"schema": "RECONSTRUCTION_ORIGINAL_CAPTURE_INPUTS_V1", "root": str(root),
            "selected_frames": len(selected),
            "ordered_source_names": [name for _, name in selected],
            "frame_ids": [fid for fid, _ in selected], "artifacts": rows}


def snapshot_capture(metadata_root, sidecars_root, resolved_jpegs_by_basename,
                     destination_root, *, expected_frames=97):
    """Optional byte-copy snapshot for production whose actual layout differs.

    Every sidecar is copied intact. No fields are invented or reformatted. For an
    existing complete original layout, call capture_manifest instead to reuse it.
    """
    source = _canonical(metadata_root)
    sidecars = _canonical(sidecars_root)
    destination = _canonical(destination_root, exists=False)
    if destination.exists() or source == destination or source in destination.parents:
        raise FileExistsError("fresh disjoint snapshot root required")
    selected = _selection(source / "official_sfm_sparse_meta.json",
                          source / "official_sfm_fed_frames.jsonl", expected_frames)
    copies = [(source / name, name) for name in
              ("official_sfm_sparse_meta.json", "official_sfm_fed_frames.jsonl",
               "official_sfm_sparse.ply")]
    for _, name in selected:
        side = name.replace(".jpg", ".json")
        _validate_sidecar(sidecars / side)
        copies.extend(((sidecars / side, "sidecars/" + side),
                       (_canonical(resolved_jpegs_by_basename[name]), "photos_jpg/" + name)))
    before = [_row(src, rel, "snapshot_source") for src, rel in copies]
    destination.mkdir()
    (destination / "sidecars").mkdir()
    (destination / "photos_jpg").mkdir()
    for (src, rel), identity in zip(copies, before):
        target = destination / rel
        with src.open("rb") as input_file, target.open("xb") as output_file:
            shutil.copyfileobj(input_file, output_file, 1024 * 1024)
        if (_row(src, rel, "snapshot_source") != identity
                or _row(target, rel, "snapshot_source") != identity):
            raise RuntimeError("snapshot byte identity mismatch; partial snapshot retained")
    return capture_manifest(destination, expected_frames=expected_frames)


def _run_whole_script(source, argv, env):
    """Shared control-flow boundary; tests use complete synthetic script files."""
    with _LOCK:
        saved_argv = sys.argv
        saved_environment = dict(os.environ)
        try:
            sys.argv = list(argv)
            os.environ.update(env)

            runpy.run_path(str(source), run_name="__main__")
        finally:
            sys.argv = saved_argv
            os.environ.clear()
            os.environ.update(saved_environment)


def _fresh_json(path, document):
    with Path(path).open("x", encoding="utf-8") as stream:
        json.dump(document, stream, indent=2, ensure_ascii=False)
        stream.write("\n")


def _verify_source():
    source = _canonical(Path(__file__).with_name(SOURCE_NAME))
    if _digest(source) != SOURCE_SHA256:
        raise RuntimeError("whole original producer source identity mismatch")
    return source


def _runtime_identity():
    import numpy
    import PIL
    result = {"python": platform.python_version(), "numpy": numpy.__version__,
              "Pillow": PIL.__version__}
    if result != RUNTIME:
        raise RuntimeError("registered existing provider versions required: " + repr(result))
    for name, module in (("numpy", numpy), ("Pillow", PIL)):
        result[name + "_module"] = _row(Path(module.__file__).resolve(),
                                         str(Path(module.__file__).resolve()), "runtime_module")
    result["executable"] = sys.executable
    return result


def _manifest_frame_count(inputs):
    count = inputs["selected_frames"]
    if type(count) is not int or count <= 0:
        raise ValueError("production V2 actual positive frame count required")
    return count


def _validate_output_layout(output, inputs, rows, *, expected_frames=97, max_output_bytes=104857600):
    count = expected_frames
    expected = {"cams.f32", "neighbors.i32", "images.f16", "frames.json"}
    expected.update("rgb/%08d.jpg" % index for index in range(count))
    if len(rows) != len(expected) or {row["path"] for row in rows} != expected:
        raise RuntimeError("actual complete original output file set required")
    sizes = {row["path"]: row["bytes"] for row in rows}
    if (sizes["cams.f32"] != count * 36 * 4 or sizes["neighbors.i32"] != count * 9 * 4
            or sizes["images.f16"] != count * 576 * 768 * 2):
        raise RuntimeError("actual original output transport lengths differ")
    if max_output_bytes is not None and sum(sizes.values()) > max_output_bytes:
        raise RuntimeError("registered output byte budget exceeded; outputs retained")
    metadata = json.loads((output / "frames.json").read_bytes())
    if (metadata["count"] != count or metadata["width"] != 768
            or metadata["height"] != 576 or metadata["num_src"] != 9
            or metadata["names"] != inputs["ordered_source_names"]
            or metadata["frame_ids"] != inputs["frame_ids"]):
        raise RuntimeError("actual original frame row identity differs")


def _validate_output_layout_v2(output, inputs, rows):
    _validate_output_layout(output, inputs, rows,
                            expected_frames=_manifest_frame_count(inputs),
                            max_output_bytes=None)


def compare_output_bytes(output_root, reference_root, receipt_path):
    """Exact file-size/SHA comparison; inputs/reference provenance stays external."""
    output, reference = _canonical(output_root), _canonical(reference_root)
    def rows(root):
        return {p.relative_to(root).as_posix(): _row(p, p.relative_to(root).as_posix(), "fixture")
                for p in sorted(root.rglob("*")) if p.is_file()}
    actual, expected = rows(output), rows(reference)
    differences = [{"path": key, "actual": actual.get(key), "reference": expected.get(key)}
                   for key in sorted(actual.keys() | expected.keys())
                   if actual.get(key) != expected.get(key)]
    result = {"schema": "RECONSTRUCTION_FIXTURE_BYTE_COMPARISON_V1", "output_root": str(output),
              "reference_root": str(reference), "actual_files": len(actual),
              "reference_files": len(expected), "differences": differences,
              "byte_equal": not differences, "reference_provenance": "caller_must_bind"}
    _fresh_json(_canonical(receipt_path, exists=False), result)
    return result


def run_fixture(capture_root, output_root, expected_input_manifest, receipt_path,
                *, input_binding=None, _actual_frames_v2=None):
    """Embedded entry; receipt records observed success/failure, then rethrows.

    Call with a pre-registered input manifest in the existing process-owned
    CPython provider. A successful call proves execution, never historical parity.
    """
    with _LOCK:
        source = _verify_source()
        root = _canonical(capture_root)
        output = _canonical(output_root, exists=False)
        receipt = _canonical(receipt_path, exists=False)
        if (output.exists() or receipt.exists() or root == output
                or root in output.parents or output in root.parents
                or root in receipt.parents or output in receipt.parents):
            raise FileExistsError("fresh disjoint output and evidence paths required")
        actual = (capture_manifest(root) if _actual_frames_v2 is None else
                  capture_manifest(root, expected_frames=_actual_frames_v2))
        if actual != expected_input_manifest:
            raise RuntimeError("frozen original capture input manifest differs")
        runtime = _runtime_identity()

        contract = {"schema": "RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_CONTRACT_V1",
                    "source_sha256": SOURCE_SHA256, "wrapper_sha256": _digest(__file__),
                    "inputs": actual, "runtime": runtime, "environment": ENV,
                    "input_manifest_binding": input_binding,
                    "argv": [str(source), str(root), str(output)],
                    "expected_outputs": ["cams.f32", "neighbors.i32", "frames.json", "images.f16", "rgb/*.jpg"],
                    "classification": "actual_execution_no_parity_claim",
                    "parent_run_id": "fixture_preparation"}
        if _actual_frames_v2 is not None:
            contract.update(schema="RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_CONTRACT_V2",
                            producer_profile=PROFILE_V2, actual_frames=_actual_frames_v2,
                            frame_count_policy="actual_positive_count",
                            legacy_noise_fixture_capacity=LEGACY_FIXTURE_FRAME_COUNT)
        contract_path = Path(str(receipt) + ".contract.json")
        _fresh_json(contract_path, contract)
        started = time.time()
        error = None
        try:
            _run_whole_script(source, contract["argv"], ENV)
        except BaseException as caught:
            error = caught
            raise
        finally:
            doc = {"schema": "RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_RECEIPT_V1",
                   "started_unix": started, "ended_unix": time.time(),
                   "contract_sha256": _digest(contract_path), "status": "failed" if error else "returned",
                   "error_type": type(error).__name__ if error else None,
                   "system_exit_code": error.code if isinstance(error, SystemExit) else None,
                   "outputs_retained": True, "historical_parity": "not_checked"}
            if _actual_frames_v2 is not None:
                doc.update(schema="RECONSTRUCTION_INTACT_FIXTURE_EXECUTION_RECEIPT_V2",
                           producer_profile=PROFILE_V2, actual_frames=_actual_frames_v2)
            verification_error = None
            try:
                doc["source_and_inputs_unchanged"] = (
                    _digest(source) == SOURCE_SHA256 and
                    (capture_manifest(root) if _actual_frames_v2 is None else
                     capture_manifest(root, expected_frames=_actual_frames_v2)) == actual)
                doc["outputs"] = [_row(p, p.relative_to(output).as_posix(), "actual_producer_output")
                                  for p in sorted(output.rglob("*")) if p.is_file()] if output.is_dir() else []
                if not doc["source_and_inputs_unchanged"]:
                    raise RuntimeError("source/input mutation; output and failure evidence retained")
                if error is None:
                    if _actual_frames_v2 is None:
                        _validate_output_layout(output, actual, doc["outputs"])
                    else:
                        _validate_output_layout_v2(output, actual, doc["outputs"])
            except BaseException as caught:
                verification_error = caught
                doc["verification_error_type"] = type(caught).__name__
                doc["status"] = "failed"
            try:
                _fresh_json(receipt, doc)
            except BaseException:

                if error is None:
                    raise
            if verification_error is not None and error is None:
                raise verification_error
        return doc


def prepare(capture_root, output_root, manifest_path, manifest_sha256, receipt_path):
    """Five UTF-8 path/hash arguments for an existing-provider native adapter.

    Returns integer97 only after whole-script return and actual output/identity
    gates. The process owner initializes/serializes its existing interpreter.
    """
    manifest_path = _canonical(manifest_path)
    raw = manifest_path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != manifest_sha256:
        raise RuntimeError("frozen input manifest file identity mismatch")
    error = None
    try:
        document = run_fixture(capture_root, output_root, json.loads(raw), receipt_path,
            input_binding={"path": str(manifest_path), "sha256": manifest_sha256,
                           "bytes": len(raw)})
    except BaseException as caught:
        error = caught
        raise
    finally:
        try:
            if manifest_path.read_bytes() != raw:
                raise RuntimeError("input manifest changed during original producer")
        except BaseException:
            if error is None:
                raise
    if document["status"] != "returned" or not document["source_and_inputs_unchanged"]:
        raise RuntimeError("actual intact original producer did not pass its IO gates")
    return 97



def run_fixture_v2(capture_root, output_root, expected_input_manifest, receipt_path,
                   *, input_binding=None):
    """Explicit V2: actual N is mandatory in the authenticated frozen manifest."""
    count = _manifest_frame_count(expected_input_manifest)
    return run_fixture(capture_root, output_root, expected_input_manifest, receipt_path,
                       input_binding=input_binding, _actual_frames_v2=count)


def prepare_v2(capture_root, output_root, manifest_path, manifest_sha256, receipt_path):
    """Existing five-string native binding, positive actual-N result; no fallback97."""
    manifest_path = _canonical(manifest_path)
    raw = manifest_path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != manifest_sha256:
        raise RuntimeError("frozen input manifest file identity mismatch")
    manifest = json.loads(raw)
    count = _manifest_frame_count(manifest)
    error = None
    try:
        document = run_fixture_v2(capture_root, output_root, manifest, receipt_path,
            input_binding={"path": str(manifest_path), "sha256": manifest_sha256,
                           "bytes": len(raw)})
    except BaseException as caught:
        error = caught
        raise
    finally:
        try:
            if manifest_path.read_bytes() != raw:
                raise RuntimeError("input manifest changed during original producer")
        except BaseException:
            if error is None:
                raise
    if document["status"] != "returned" or not document["source_and_inputs_unchanged"]:
        raise RuntimeError("actual intact original producer did not pass its IO gates")
    return count
