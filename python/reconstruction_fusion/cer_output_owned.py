"""Scoped output allocation facade; numerical source and save copy stay original."""
from .cer_input_borrow import CerInputArrayIO, identity
OUTPUT_COUNTERS={'eligible':0,'returned':0,'fallback':0}
def eligible(np,a):
    return (type(a) is np.ndarray and a.ndim==2 and all(x>0 for x in a.shape)
            and a.dtype==np.dtype(np.float32) and a.dtype.isnative
            and a.flags.c_contiguous and a.flags.aligned and a.flags.owndata
            and a.flags.writeable and a.base is None)
class _OwnedWhere:
    __slots__=('_np','_array','_identity','_available')
    def __init__(self,np,a):
        self._np,self._array,self._identity,self._available=np,a,identity(a),True
    def astype(self,*args,**kwargs):
        if (self._available and len(args)==1 and args[0] is self._np.float32 and not kwargs
                and eligible(self._np,self._array) and identity(self._array)==self._identity):
            self._available=False
            OUTPUT_COUNTERS['returned']+=1
            return self._array
        OUTPUT_COUNTERS['fallback']+=1
        return self._array.astype(*args,**kwargs)
class CerOutputArrayIO(CerInputArrayIO):
    def where(self,*args,**kwargs):
        a=self.np.where(*args,**kwargs)
        if eligible(self.np,a):
            OUTPUT_COUNTERS['eligible']+=1
            return _OwnedWhere(self.np,a)
        OUTPUT_COUNTERS['fallback']+=1
        return a
