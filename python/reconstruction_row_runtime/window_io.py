"""Exact chunk transport plus readonly bounded plane lifetimes; no numerical math."""
from pathlib import Path
from collections import OrderedDict
import hashlib
from .transport import exact_segments

class WindowLoads:
    def __init__(self,np,rows,checked,base,shape=(97,1152,1536),capacity=3):
        assert type(capacity)is int and capacity==3
        self.np=np;self.base=base;self.n,self.h,self.w=shape
        self.cache=OrderedDict();self.capacity=capacity;self.mapped=0;self.valid={};self.verification=[]
        assert len(rows)==4 and [r['plane']for r in rows]==['depth','conf0','conf1','conf2']
        segments=[exact_segments(row,shape)for row in rows]
                                                                              
                                                                             
        for row,pieces in zip(rows,segments):
            full=hashlib.sha256()
            for first,count,item in pieces:
                path=Path(checked(item));assert path.is_file()and path.stat().st_size==item['bytes']
                part=hashlib.sha256();read=0
                with path.open('rb')as stream:
                    for block in iter(lambda:stream.read(1<<20),b''):
                        part.update(block);full.update(block);read+=len(block)
                assert read==item['bytes']and part.hexdigest()==item['sha256'],'piece SHA mismatch'
                for local in range(count):
                    key=f"{base}/{row['plane']}/{first+local:04d}.npy"
                    assert key not in self.valid;self.valid[key]=(str(path),local)
            assert full.hexdigest()==row['sha256'],'ordered bank SHA mismatch'
            self.verification.append({'plane':row['plane'],'sha256':full.hexdigest(),'bytes':row['bytes'],'chunks':len(pieces),'method':'bounded stream before mapping'})
    def __contains__(self,key):return key in self.valid
    def __getitem__(self,key):
        if key not in self.valid:raise KeyError(key)
        if key in self.cache:
            value=self.cache.pop(key);self.cache[key]=value;return value
        path,local=self.valid[key]
        mapping=self.np.memmap(path,mode='r',dtype='<f4',offset=local*self.h*self.w*4,shape=(self.h,self.w))
        value=self.np.asarray(mapping)
        assert type(value)is self.np.ndarray and not value.flags.writeable and value.flags.aligned
        self.cache[key]=value;self.mapped+=1
        while len(self.cache)>self.capacity:self.cache.popitem(last=False)
        return value
    def clear(self):self.cache.clear()

class LazyDepth:
    def __init__(self,loads):self.loads=loads
    def __len__(self):return self.loads.n
    def __getitem__(self,frame):
        if type(frame)is not int or not 0<=frame<len(self):raise IndexError(frame)
        return self.loads[f'{self.loads.base}/depth/{frame:04d}.npy']
