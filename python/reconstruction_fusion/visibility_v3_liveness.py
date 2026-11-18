"""Distinct lifecycle variant of prior HOLD visibility-v2; no new math.

Drop one()'s private dead grid/Xc buffers before the unchanged whole-shape
visibility prepass. All prior HOLD results remain unchanged historical facts.
"""
import ast
import hashlib
from pathlib import Path
import sys

PRIOR=Path(__file__).resolve().parent
assert hashlib.sha256((PRIOR/'visibility_cache_v2.py').read_bytes()).hexdigest()=='b5e6ae535f1287ba18f0fd9d7455cd7d36bcdc7cb6bebc4613480c98b3d00cfb'
assert hashlib.sha256((PRIOR/'accepted_angle_sparse.py').read_bytes()).hexdigest()=='bb9741acb454762bf6553e7d298232571b5152255b6ac94f4d02d968cac8d2f8'
from . import visibility_cache_v2 as prior


def transformed_defs(fusion_source,filter_source):
    tree=ast.parse(prior.transformed_defs(fusion_source,filter_source))
    one=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='one')
    photo=next(n for n in one.body if isinstance(n,ast.For) and isinstance(n.target,ast.Tuple)
               and ast.unparse(n.target)=='(k, th)')
    xc=next(n for n in one.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='Xc' for t in n.targets))
    xw=next(n for n in one.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='Xw' for t in n.targets))

    same=lambda a,b:ast.dump(a,include_attributes=False)==ast.dump(b,include_attributes=False)
    assert same(xc,ast.parse('Xc = np.stack([(uu - Kr[0, 2]) / Kr[0, 0] * ref, (vv - Kr[1, 2]) / Kr[1, 1] * ref, ref], -1).reshape(-1, 3)').body[0])
    assert same(xw,ast.parse('Xw = (Xc - tr) @ Rr').body[0])
    assert ast.unparse(photo.iter)=='enumerate([0.3, 0.5, 0.5])'
    for node,statement in ((xw,'del Xc'),(xc,'del vv, uu'),(photo,'del c')):
        i=one.body.index(node);one.body[i+1:i+1]=ast.parse(statement).body

    stripped=ast.fix_missing_locations(ast.parse(ast.unparse(tree)))
    one2=next(n for n in stripped.body if isinstance(n,ast.FunctionDef) and n.name=='one')
    one2.body=[n for n in one2.body if not isinstance(n,ast.Delete)]
    assert same(stripped,ast.parse(prior.transformed_defs(fusion_source,filter_source)))
    ast.fix_missing_locations(tree)
    return ast.unparse(tree)+'\n'


def install(scope,fusion_source,filter_source,*,pinned_runtime=True):
    if pinned_runtime:prior.base.validate_runtime()
    prior.base.bind_helpers(scope)
    scope.update(_reconstruction_fusionualify=prior.qualified_sources,_reconstruction_eligible=prior.eligible)
    exec(compile(transformed_defs(fusion_source,filter_source),'<visibility-v3-dead-local-release>','exec'),scope)
