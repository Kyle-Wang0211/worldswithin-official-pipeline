"""Bounded selected-implementation port. Small NumPy products and cv2 stay pinned."""
from pathlib import Path
import ctypes
import hashlib
import json
import numpy as np
from reconstruction_distribution_identity import native_digest

HERE = Path(__file__).resolve().parent
P = HERE
DT = np.ctypeslib.ndpointer(dtype=np.float64, flags='C_CONTIGUOUS')
FT = np.ctypeslib.ndpointer(dtype=np.float32, flags='C_CONTIGUOUS')
UT = np.ctypeslib.ndpointer(dtype=np.uint8, flags='C_CONTIGUOUS')


class Scalar:
    def __init__(self):
        import sys
        import cv2
        assert sys.version_info[:3] == (3, 10, 21) and np.__version__ == '1.26.4' and cv2.__version__ == '4.11.0'
                                                                                    
        import reconstruction_original_dense_raw_driver as driver
        bundle = Path(driver.__file__).resolve().parents[4]
        frameworks = bundle / 'Frameworks'
        path = frameworks / 'PWLibrary-libopenblas64--0-0e4e796a0e59.framework/PWLibrary-libopenblas64--0-0e4e796a0e59'
        assert native_digest(path) == 'd892f69f07d4ba09bd9bd54e7156698c858b2e39cd283c3270efd896516cdf2c'
        dyld = ctypes.CDLL(None)
        dyld._dyld_image_count.restype = ctypes.c_uint32
        dyld._dyld_get_image_name.restype = ctypes.c_char_p
        loaded = [dyld._dyld_get_image_name(i).decode() for i in range(dyld._dyld_image_count())
                  if 'openblas' in dyld._dyld_get_image_name(i).decode().lower()]
        assert len(loaded) == 1 and Path(loaded[0]).resolve() == path.resolve(), loaded
        blas = ctypes.CDLL(str(path))
        blas.openblas_get_config64_.restype = ctypes.c_char_p
        assert blas.openblas_get_config64_().decode() == 'OpenBLAS 0.3.23.dev  USE64BITINT NO_AFFINITY ARMV8 MAX_THREADS=2'
        assert blas.openblas_get_num_threads64_() == 1
        binding = json.loads(HERE.joinpath('BINDING.json').read_text())
        relative = Path(binding['rows_library_relative'])
        assert not relative.is_absolute() and relative.parts[0] == 'Frameworks' and '..' not in relative.parts
        native_path = bundle / relative
        assert native_path.resolve().is_relative_to(frameworks.resolve())
        expected = binding['signed_rows_library_sha256']
                                                                             
        assert hashlib.sha256(native_path.read_bytes()).hexdigest() == expected
        self.lib = ctypes.CDLL(str(native_path))
        self.fp = self.lib
        self.fp.pw_read_fpcr.restype = ctypes.c_uint64
        assert self.fp.pw_read_fpcr() == 0
        self.lib.pw_project.argtypes = [FT, ctypes.c_int, ctypes.c_int, DT, DT, DT, DT, FT, FT]
        self.lib.pw_project.restype = ctypes.c_int
        self.lib.pw_backproject.argtypes = [FT, FT, DT, ctypes.c_int, ctypes.c_int, DT, DT, DT,
            ctypes.c_float, ctypes.c_float, ctypes.c_double, ctypes.c_float, UT, FT, FT, FT, FT]
        self.lib.pw_backproject.restype = ctypes.c_int
        self.lib.pw_sees.argtypes = [FT, ctypes.c_int, ctypes.c_int, DT, DT, DT]
        self.lib.pw_sees.restype = ctypes.c_int
        self.stats = {'sees_native': 0, 'sees_fallback': 0, 'geometry_native': 0, 'geometry_fallback': 0}

    @staticmethod
    def eligible(array):
        return (type(array) is np.ndarray and array.dtype == np.float32 and array.ndim == 2
                and array.flags.c_contiguous and array.size >= 2 and np.isfinite(array).all())

    def consistency(self, original):
        import cv2
        def call(ref, kr, er, src, ks, es, maximum, minimum,
                 pixel=1.0, relative=.01, return_ref_depth_src=False):
            def fallback():
                self.stats['geometry_fallback'] += 1
                return original(ref, kr, er, src, ks, es, maximum, minimum,
                                pixel, relative, return_ref_depth_src=return_ref_depth_src)
            if (return_ref_depth_src or not self.eligible(ref) or not self.eligible(src)
                    or self.fp.pw_read_fpcr() != 0
                    or any(v not in ('warn', 'ignore') for v in np.geterr().values())):
                return fallback()
            for m, shape in ((kr, (3, 3)), (er, (4, 4)), (ks, (3, 3)), (es, (4, 4))):
                if (type(m) is not np.ndarray or m.dtype != np.float64 or m.shape != shape
                        or not np.isfinite(m).all()):
                    return fallback()
            if not all(np.isfinite(v) and abs(v) <= np.finfo(np.float32).max
                       for v in (maximum, minimum, relative)) or not np.isfinite(pixel):
                return fallback()
            h, w = ref.shape
                                                                             
            ikr = np.ascontiguousarray(np.linalg.inv(kr))
            iks = np.ascontiguousarray(np.linalg.inv(ks))
            tsr = np.ascontiguousarray(es @ np.linalg.inv(er))
            trs = np.ascontiguousarray(er @ np.linalg.inv(es))
            kr = np.ascontiguousarray(kr);ks = np.ascontiguousarray(ks)
            xy = np.empty((2, h, w), np.float64)
            x = np.empty((h, w), np.float32);y = np.empty_like(x)
            if self.lib.pw_project(ref, h, w, ikr, tsr, ks, xy, x, y):
                del xy, x, y
                return fallback()
            sampled = cv2.remap(src, x, y, interpolation=cv2.INTER_LINEAR)
            mask = np.empty((h, w), np.uint8);filtered = np.empty_like(x)
            raw = np.empty_like(x);xr = np.empty_like(x);yr = np.empty_like(x)
            if self.lib.pw_backproject(ref, sampled, xy, h, w, iks, trs, kr,
                    maximum, minimum, pixel, relative, mask, filtered, raw, xr, yr):
                del xy, x, y, sampled, mask, filtered, raw, xr, yr
                return fallback()
            self.stats['geometry_native'] += 1
            return mask.view(np.bool_), filtered, x, y
        return call

    def qualifier(self, original):
        def qualify(f, nf, w, h, ref, cam):
                                                                                
            k, e = cam(f)
            positive = np.count_nonzero(ref > 0)
            if not self.eligible(ref) or self.fp.pw_read_fpcr() != 0 or positive < 2:
                self.stats['sees_fallback'] += 1
                return original(f, nf, w, h, ref, cam)
            inverse = np.linalg.inv(e)
            result = []
            for s in range(nf):
                if s == f:
                    result.append(s)
                    continue
                ks, es = cam(s)
                transform = np.ascontiguousarray(es @ inverse)
                value = self.lib.pw_sees(ref, h, w, np.ascontiguousarray(k), transform,
                                         np.ascontiguousarray(ks))
                if value < 0:
                    self.stats['sees_fallback'] += 1
                                                                                       
                    return original(f, nf, w, h, ref, cam)
                self.stats['sees_native'] += 1
                if value:
                    result.append(s)
            return result
        return qualify


def install(scope, fusion_source, filter_source, *, pinned_runtime=True, mode='sees'):
    import sys
    e = HERE
    from . import visibility_v3_liveness as v3
    assert hashlib.sha256(Path(v3.__file__).read_bytes()).hexdigest() == '7ab1b687aa36297e68d6c0edfcdde4c33d2ef586964f2440a643b2db926a0535'
    v3.install(scope, fusion_source, filter_source, pinned_runtime=pinned_runtime)
    native = Scalar()
    if mode in ('sees', 'both'):
        scope['_reconstruction_fusionualify'] = native.qualifier(v3.prior.qualified_sources)
    if mode in ('geometry', 'both'):
        scope['check_geometric_consistency'] = native.consistency(scope['check_geometric_consistency'])
    scope['_reconstruction_native_state'] = native
    return native
