"""Exact-f32 storage alias for the pinned reference_pipeline fusion load boundary.

The selected source calls ``np.load(path).astype(np.float32)`` on immutable raw
float32 planes. This token changes that IO allocation policy only. It is not an
ndarray subclass, does not replace NumPy functions, and accepts no other cast.
The synchronous native caller must retain the input mapping until fusion returns.
"""


class ReadonlyFloat32Load:
    __slots__ = ('_np', '_array', '_identity')

    def __init__(self, np, array):
        self._validate(np, array)
        object.__setattr__(self, '_np', np)
        object.__setattr__(self, '_array', array)
        object.__setattr__(self, '_identity', self._identity_of(array))

    def __setattr__(self, name, value):
        raise AttributeError('original fusion readonly load token is immutable')

    @staticmethod
    def _validate(np, array):
        if (type(array) is not np.ndarray or array.ndim != 2 or
                any(size <= 0 for size in array.shape) or
                array.dtype != np.dtype(np.float32) or not array.dtype.isnative or
                not array.flags.c_contiguous or not array.flags.aligned or
                array.flags.writeable):
            raise ValueError('original fusion load requires readonly aligned C native-float32 plane')

    @staticmethod
    def _identity_of(array):
        return (array.shape, array.strides, array.dtype.str, array.nbytes,
                array.__array_interface__['data'][0])

    def astype(self, *args, **kwargs):
        if len(args) != 1 or args[0] is not self._np.float32 or kwargs:
            raise ValueError('only exact original astype(np.float32) is supported')
        self._validate(self._np, self._array)
        if self._identity_of(self._array) != self._identity:
            raise ValueError('original fusion readonly plane identity changed')
        return self._array
