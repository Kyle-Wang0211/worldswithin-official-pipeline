#!/usr/bin/env python3









import sys, json, time
import numpy as np
import vdbfusion
import trimesh
voxel, out = float(sys.argv[1]), sys.argv[2]
min_w = float(sys.argv[3]) if len(sys.argv) > 3 else 5.0
C = sys.argv[4] if len(sys.argv) > 4 else "capture"
order = json.load(open(f"{C}/order.json"))["order"]
t0 = time.time()


import os; trunc = float(os.environ.get("SDF_TRUNC", "0.04")); print(f"sdf_trunc {trunc}", flush=True)
vdb_volume = vdbfusion.VDBVolume(voxel_size=voxel, sdf_trunc=trunc, space_carving=False)
npts = 0
for i, v in enumerate(order):
    z = np.load(f"{C}/{v:08d}.npz"); d = z["d16"].astype(np.float64) / 5000.0; K = z["intr"]; E = z["extr"]
    vs, us = np.nonzero(d > 0); dd = d[vs, us]
    Xc = np.stack([(us - K[0, 2]) / K[0, 0] * dd, (vs - K[1, 2]) / K[1, 1] * dd, dd], 1)
    R, t = E[:3, :3], E[:3, 3]
    Xw = (Xc - t) @ R
    cam_center = -R.T @ t
    vdb_volume.integrate(np.ascontiguousarray(Xw), extrinsic=cam_center)
    npts += len(dd)
    if i % 20 == 0: print(f"  [{i+1}/{len(order)}] points {npts:,}  {time.time()-t0:.0f}s", flush=True)
print(f"integrated points {npts:,} (mesh A valid pixels 36,233,053)  {time.time()-t0:.0f}s", flush=True)
vertices, faces = vdb_volume.extract_triangle_mesh(min_weight=min_w)
print(f"extract_triangle_mesh(min_weight={min_w}): {len(vertices):,} v / {len(faces):,} f  {time.time()-t0:.0f}s", flush=True)
geo_mesh = trimesh.Trimesh(vertices, faces)
geo_mesh.export(out)
print(f"trimesh: {len(geo_mesh.vertices):,} v / {len(geo_mesh.faces):,} f -> {out}  {time.time()-t0:.0f}s", flush=True)
