"""CER input allocation adapter; selected numerical source is unchanged.

Only exact immutable native/aligned/C-contiguous float32 planes are borrowed.
The synchronous caller retains their storage. All other casts use ndarray.astype.
"""
from reconstruction_original_dense_raw_driver import ArrayIO

COUNTERS = {'eligible': 0, 'borrowed': 0, 'fallback': 0}


def eligible(np, array):
    return (type(array) is np.ndarray and array.ndim == 2 and
            all(size > 0 for size in array.shape) and
            array.dtype == np.dtype(np.float32) and array.dtype.isnative and
            array.flags.c_contiguous and array.flags.aligned and
            not array.flags.writeable)


def identity(array):
    return (array.shape, array.strides, array.dtype.str, array.nbytes,
            array.__array_interface__['data'][0])


class _ReadonlyCerLoad:
    __slots__ = ('_np', '_array', '_identity')

    def __init__(self, np, array):
        self._np, self._array, self._identity = np, array, identity(array)

    def astype(self, *args, **kwargs):
        if (len(args) == 1 and args[0] is self._np.float32 and not kwargs and
                eligible(self._np, self._array) and
                identity(self._array) == self._identity):
            COUNTERS['borrowed'] += 1
            return self._array
        COUNTERS['fallback'] += 1
        return self._array.astype(*args, **kwargs)


class CerInputArrayIO(ArrayIO):
    def load(self, path):
        array = super().load(path)
        if eligible(self.np, array):
            COUNTERS['eligible'] += 1
            return _ReadonlyCerLoad(self.np, array)
        COUNTERS['fallback'] += 1
        return array
