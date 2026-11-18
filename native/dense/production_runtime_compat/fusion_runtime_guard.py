"""Fusion calling-thread setup; no arrays, FPCR writes, or numerical transforms."""
import ctypes
import hashlib
import os
from pathlib import Path
import plistlib
import re
import sys
import threading
import traceback
from reconstruction_distribution_identity import native_digest

BLAS_SHA = 'd892f69f07d4ba09bd9bd54e7156698c858b2e39cd283c3270efd896516cdf2c'
BLAS_CONFIG = 'OpenBLAS 0.3.23.dev  USE64BITINT NO_AFFINITY ARMV8 MAX_THREADS=2'

def _verified_bundle(original_file):
    """IO identity only: exact original-driver layout in an accepted App bundle."""
    source = Path(original_file)
    if (not source.is_absolute() or not source.is_file() or source.resolve() != source
            or any(path.is_symlink() for path in (source, *source.parents))
            or len(source.parents) < 5):
        raise RuntimeError('Fusion runtime guard: canonical_original_driver')
    bundle = source.parents[4]
    expected = bundle / 'RECONSTRUCTIONPython/lib/python3.10/site-packages/reconstruction_original_dense_raw_driver.py'
    if source != expected or not bundle.is_dir() or bundle.suffix != '.app':
        raise RuntimeError('Fusion runtime guard: original_driver_bundle_layout')
    info = bundle / 'Info.plist'
    if not info.is_file() or info.resolve() != info or info.is_symlink():
        raise RuntimeError('Fusion runtime guard: canonical_bundle_info')
    with info.open('rb') as stream:
        metadata = plistlib.load(stream)
    allowed_ids = {
        value for key in ('WORLDSWITHIN_PRODUCTION_BUNDLE_ID', 'WORLDSWITHIN_BENCH_BUNDLE_ID',
                          'WORLDSWITHIN_PRODUCT_COPY_BUNDLE_ID')
        if (value := os.environ.get(key)) and re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9.-]*', value)
    }
    if not isinstance(metadata, dict) or metadata.get('CFBundleIdentifier') not in allowed_ids:
        raise RuntimeError('Fusion runtime guard: bundle_identifier')
    return bundle

class NativeRuntime:
    def __init__(self, original, np, cv2, binding):
        self.original, self.np, self.cv2, self.binding = original, np, cv2, binding

    def observe(self, details, checkpoint):
        def guard(name, passed):
            details['guards'][name] = bool(passed)
            checkpoint()
            if not passed:
                raise RuntimeError('Fusion runtime guard: ' + name)
        details.update(thread_ident=threading.get_ident(), python=list(sys.version_info[:3]),
                       numpy=self.np.__version__, opencv=self.cv2.__version__,
                       environment={key: os.environ.get(key) for key in
                           ('OPENBLAS_NUM_THREADS','GOTO_NUM_THREADS','OMP_NUM_THREADS')}, guards={})
        guard('versions', details['python'] == [3,10,21] and details['numpy'] == '1.26.4' and details['opencv'] == '4.11.0')
        bundle = _verified_bundle(self.original.__file__)
        details['bundle'] = str(bundle)
        guard('bundle_identity', True)
        path = bundle / 'Frameworks/PWLibrary-libopenblas64--0-0e4e796a0e59.framework/PWLibrary-libopenblas64--0-0e4e796a0e59'
        details['openblas_path'] = str(path.resolve())
        details['openblas_sha256'] = native_digest(path)
        guard('openblas_sha256', details['openblas_sha256'] == BLAS_SHA)
        dyld = ctypes.CDLL(None)
        dyld._dyld_image_count.restype = ctypes.c_uint32
        dyld._dyld_get_image_name.restype = ctypes.c_char_p
        names = [dyld._dyld_get_image_name(i).decode() for i in range(dyld._dyld_image_count())]
        loaded = [str(Path(name).resolve()) for name in names if 'openblas' in name.lower()]
        details['loaded_openblas'] = loaded
        guard('unique_canonical_openblas', loaded == [str(path.resolve())])
        self.blas = ctypes.CDLL(str(path))
        self.blas.openblas_get_config64_.restype = ctypes.c_char_p
        self.blas.openblas_get_num_threads64_.restype = ctypes.c_int
        self.blas.openblas_set_num_threads64_.argtypes = [ctypes.c_int]
        self.blas.openblas_set_num_threads64_.restype = None
        details['openblas_config'] = self.blas.openblas_get_config64_().decode()
        guard('openblas_config', details['openblas_config'] == BLAS_CONFIG)
        details['threads_before'] = self.threads()
        guard('positive_previous_thread_count', details['threads_before'] > 0)
        relative = Path(self.binding['rows_library_relative'])
        guard('native_relative_path', not relative.is_absolute() and bool(relative.parts) and relative.parts[0] == 'Frameworks' and '..' not in relative.parts)
        native_path = bundle / relative
        guard('native_canonical_path', native_path.resolve().is_relative_to((bundle/'Frameworks').resolve()))
        details['native_path'] = str(native_path.resolve())
        details['native_sha256'] = hashlib.sha256(native_path.read_bytes()).hexdigest()
        guard('native_sha256', details['native_sha256'] == self.binding['signed_rows_library_sha256'])
        native = ctypes.CDLL(str(native_path))
        native.pw_read_fpcr.restype = ctypes.c_uint64
        details['fpcr_readonly'] = int(native.pw_read_fpcr())
        guard('fpcr_zero', details['fpcr_readonly'] == 0)
        return details['threads_before']

    def threads(self):
        return int(self.blas.openblas_get_num_threads64_())

    def set_threads(self, value):
        self.blas.openblas_set_num_threads64_(value)

class FusionRuntimeGuard:
    def __init__(self, runtime, details, checkpoint):
        self.runtime, self.details, self.checkpoint = runtime, details, checkpoint
        self.previous = None
        self.attempted = False
        self.restored = False

    def initialize(self):
        self.details['phase'] = 'observe_before_setup'
        try:
            self.previous = self.runtime.observe(self.details, self.checkpoint)
            self.details['phase'] = 'set_openblas_threads_one'
            self.attempted = True
            self.checkpoint()
            self.runtime.set_threads(1)
            self.details['threads_after'] = self.runtime.threads()
            self.details.setdefault('guards', {})['threads_after_one'] = self.details['threads_after'] == 1
            if self.details['threads_after'] != 1:
                raise RuntimeError('Fusion runtime guard: threads_after_one')
            self.details['phase'] = 'ready_for_original_guards'
            self.checkpoint()
        except BaseException as error:
            self.details.update(setup_error=repr(error), setup_traceback=traceback.format_exc())
            self.checkpoint()
            raise

    def restore(self):
        if not self.attempted or self.restored:
            return
        try:
            self.details['phase'] = 'restore_openblas_threads'
            self.runtime.set_threads(self.previous)
            actual = self.runtime.threads()
            self.details['threads_restored'] = actual
            self.details['restore_matches_before'] = actual == self.previous
            if actual != self.previous:
                raise RuntimeError('Fusion runtime guard: restore_matches_before')
            self.restored = True
            self.details['phase'] = 'restored'
        except BaseException as error:
            self.details.update(restoration_error=repr(error), restoration_traceback=traceback.format_exc())
            raise
        finally:
            self.checkpoint()
