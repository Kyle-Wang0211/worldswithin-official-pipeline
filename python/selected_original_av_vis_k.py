#!/usr/bin/env python3






import json, sys, numpy as np, open3d as o3d
O = "workspace"; C = f"{O}/visibility_cache"; SC = "scene"; k = sys.argv[1]
m = o3d.io.read_triangle_mesh(f"{O}/mesh_{k}.ply"); V = np.asarray(m.vertices).astype(np.float64); NV = len(V)
order = json.load(open(f"{C}/order.json"))["order"]; NF = len(order)
cams = []
for i in range(NF):
    L = open(f"{SC}/{i:08d}.cam").read().split("\n"); a = list(map(float, L[0].split())); b = list(map(float, L[1].split()))
    cams.append(dict(t=np.array(a[:3]), R=np.array(a[3:]).reshape(3, 3), f=b[0] * 4032, cx=b[4] * 4032, cy=b[5] * 3024))
vi, cj, ox, oy = [], [], [], []
for i in range(NF):
    z = np.load(f"{C}/{i:08d}.npz"); d = z["d16"].astype(np.float64) / 5000.0; K = z["intr"]; E = z["extr"]
    X = V @ E[:3, :3].T + E[:3, 3]; zc = X[:, 2]; ok = zc > 0.05
    u = np.round(K[0, 0] * X[:, 0] / np.where(ok, zc, 1) + K[0, 2]).astype(int); v = np.round(K[1, 1] * X[:, 1] / np.where(ok, zc, 1) + K[1, 2]).astype(int)
    ok &= (u >= 0) & (u < d.shape[1]) & (v >= 0) & (v < d.shape[0]); idx = np.nonzero(ok)[0]
    dd = d[v[idx], u[idx]]; good = (dd > 0) & (np.abs(zc[idx] - dd) < 0.01 * dd); idx = idx[good]
    c = cams[i]; Xh = V[idx] @ c["R"].T + c["t"]
    vi.append(idx); cj.append(np.full(len(idx), i)); ox.append(c["f"] * Xh[:, 0] / Xh[:, 2] + c["cx"]); oy.append(c["f"] * Xh[:, 1] / Xh[:, 2] + c["cy"])
vi, cj, ox, oy = map(np.concatenate, (vi, cj, ox, oy)); s = np.argsort(vi, kind="stable"); vi, cj, ox, oy = vi[s], cj[s], ox[s], oy[s]
cnt = np.bincount(vi, minlength=NV)
print(f"vertices {NV:,}; observations {len(vi):,}; vertices seen by >=1 camera {np.count_nonzero(cnt):,} ({100*np.count_nonzero(cnt)/NV:.1f}%); "
      f"cameras per seen vertex median {np.median(cnt[cnt>0]):.0f}, p90 {np.percentile(cnt[cnt>0],90):.0f}")
col = (np.asarray(m.vertex_colors) * 255).astype(np.uint8) if m.has_vertex_colors() else np.full((NV, 3), 200, np.uint8)
np.savez(f"{O}/av_vis_{k}.npz", V=V, col=col, vi=vi, cj=cj, ox=ox, oy=oy,
         cams=np.array([[c["f"], c["cx"], c["cy"], *c["t"], *c["R"].ravel()] for c in cams]))
