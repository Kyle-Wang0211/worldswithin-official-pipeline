"""Whole-shape visibility invariants only; source order and GEMM widths unchanged."""
import ast,hashlib
import numpy as np
from . import accepted_angle_sparse as base
BASE_SHA='bb9741acb454762bf6553e7d298232571b5152255b6ac94f4d02d968cac8d2f8'

def eligible(CM,ref):


    if any(v not in ('warn','ignore') for v in np.geterr().values()):return False
    if (type(CM) is not np.ndarray or CM.dtype!=np.float32 or CM.ndim!=2 or CM.shape[1]!=36
            or type(ref) is not np.ndarray or ref.dtype!=np.float32 or ref.ndim!=2
            or not np.isfinite(ref).all() or not np.isfinite(CM).all()
            or np.max(np.abs(CM),initial=0)>1e6):return False
    try:
        with np.errstate(all='ignore'):
            for row in CM:
                K=row[:9].reshape(3,3).astype(np.float64)
                E=np.eye(4,dtype=np.float64);E[:3,:3]=row[9:18].reshape(3,3);E[:3,3]=row[18:21]

                for m in (K,E):
                    inv=np.linalg.inv(m)
                    if not np.isfinite(inv).all() or np.max(np.abs(inv))>1e6:return False
    except (ValueError,TypeError,np.linalg.LinAlgError):return False
    return True

def qualified_sources(f,NF,W,H,ref,cam):
    cache={}
    def sees(s):
        if s==f:return True
        K,E=cam(f);Ks,Es=cam(s);d=ref
        if 'X' not in cache:
            v,u=np.nonzero(d>0)
            if len(u)==0:return False
            X=np.stack([(u-K[0,2])/K[0,0]*d[v,u],(v-K[1,2])/K[1,1]*d[v,u],d[v,u],np.ones(len(u))],0)
            cache['X']=X
            cache['invE']=np.linalg.inv(E)
            del v,u
        X=cache['X']
        Y=(Es@cache['invE']@X)[:3];ok=Y[2]>1e-6
        x=Ks[0,0]*Y[0,ok]/Y[2,ok]+Ks[0,2];y=Ks[1,1]*Y[1,ok]/Y[2,ok]+Ks[1,2]
        return bool(np.any((x>=0)&(x<=W-1)&(y>=0)&(y<=H-1)))
    result=[s for s in range(NF) if sees(s)]
    cache.clear()
    return result

def transformed_defs(fusion_source,filter_source):
    tree=ast.parse(base.transformed_defs(fusion_source,filter_source))
    one=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='one')
    source_loop=next(n for n in one.body if isinstance(n,ast.For) and isinstance(n.target,ast.Name) and n.target.id=='s')
    assert ast.dump(source_loop.iter)==ast.dump(ast.parse('range(NF)',mode='eval').body)
    first=source_loop.body[0]
    assert base._same(first,ast.parse('if not sees(f,s):\n continue').body[0])
    before=ast.parse('_reconstruction_fusionualified = _reconstruction_fusionualify(f, NF, W, H, ref, cam) if _reconstruction_eligible(CM, ref) else None').body
    i=one.body.index(source_loop);one.body[i:i]=before
    source_loop.iter=ast.parse('range(NF) if _reconstruction_fusionualified is None else _reconstruction_fusionualified',mode='eval').body
    first.test=ast.parse('_reconstruction_fusionualified is None and not sees(f,s)',mode='eval').body
    ast.fix_missing_locations(tree);return ast.unparse(tree)+'\n'

def install(scope,fusion_source,filter_source,*,pinned_runtime=True):
    if pinned_runtime:base.validate_runtime()
    base.bind_helpers(scope)
    scope.update(_reconstruction_fusionualify=qualified_sources,_reconstruction_eligible=eligible)
    exec(compile(transformed_defs(fusion_source,filter_source),'<fullshape-visibility-cache>','exec'),scope)
