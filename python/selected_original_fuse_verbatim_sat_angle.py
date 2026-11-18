#!/usr/bin/env python3



















import argparse, json, os, sys, numpy as np
from PIL import Image
from concurrent.futures import ProcessPoolExecutor
ap = argparse.ArgumentParser()
ap.add_argument("--fixture", required=True); ap.add_argument("--pred", required=True); ap.add_argument("--out", required=True)
ap.add_argument("--geo-pixel", type=float, default=0.125); ap.add_argument("--mask-thres", type=int, nargs="+", default=[2])
ap.add_argument("--sat-tol", type=float, default=1e-5)
ap.add_argument("--min-tri-angle", type=float, default=3.0)
a = ap.parse_args()
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from selected_original_filter import check_geometric_consistency
FX, B, OUT = a.fixture, a.pred, a.out
meta = json.load(open(f"{FX}/frames.json")); NF, W, H = meta["count"], meta["width"], meta["height"]
CM = np.fromfile(f"{FX}/cams.f32", np.float32).reshape(NF, 36)
D = [np.load(f"{B}/depth/{f:04d}.npy").astype(np.float32) for f in range(NF)]
def cam(f):
    E = np.eye(4, dtype=np.float64); E[:3, :3] = CM[f, 9:18].reshape(3, 3); E[:3, 3] = CM[f, 18:21]
    return CM[f, 0:9].reshape(3, 3).astype(np.float64), E
def sees(f, s):
    if s == f: return True
    K, E = cam(f); Ks, Es = cam(s); d = D[f]; v, u = np.nonzero(d > 0)
    if len(u) == 0: return False
    X = np.stack([(u - K[0, 2]) / K[0, 0] * d[v, u], (v - K[1, 2]) / K[1, 1] * d[v, u], d[v, u], np.ones(len(u))], 0)
    Y = (Es @ np.linalg.inv(E) @ X)[:3]; ok = Y[2] > 1e-6
    x = Ks[0, 0] * Y[0, ok] / Y[2, ok] + Ks[0, 2]; y = Ks[1, 1] * Y[1, ok] / Y[2, ok] + Ks[1, 2]
    return bool(np.any((x >= 0) & (x <= W - 1) & (y >= 0) & (y <= H - 1)))
def one(f):
    Kr, Er = cam(f); ref = D[f]; photo = np.ones_like(ref, bool)
    for k, th in enumerate([0.3, 0.5, 0.5]):
        c = np.load(f"{B}/conf{k}/{f:04d}.npy").astype(np.float32)
        if c.shape != ref.shape: c = np.asarray(Image.fromarray(c).resize((ref.shape[1], ref.shape[0]), Image.NEAREST))
        photo &= c > th
    gs = np.zeros(ref.shape, np.int32); acc = np.zeros(ref.shape, np.float64)
    vv, uu = np.mgrid[0:ref.shape[0], 0:ref.shape[1]]
    Xc = np.stack([(uu - Kr[0, 2]) / Kr[0, 0] * ref, (vv - Kr[1, 2]) / Kr[1, 1] * ref, ref], -1).reshape(-1, 3)
    Rr, tr = Er[:3, :3], Er[:3, 3]; Xw = (Xc - tr) @ Rr
    ray_r = -Rr.T @ tr - Xw; ray_r /= np.maximum(np.linalg.norm(ray_r, axis=1, keepdims=True), 1e-12)
    cos_min = np.cos(np.radians(a.min_tri_angle))
    for s in range(NF):
        if not sees(f, s): continue
        Ks, Es = cam(s)
        gm, d_rep, _, _ = check_geometric_consistency(ref, Kr, Er, D[s], Ks, Es, float(CM[f, 25]) * (1 - a.sat_tol), float(CM[f, 24]) * (1 + a.sat_tol), a.geo_pixel, 0.01)
        if s != f and a.min_tri_angle > 0:
            ray_s = -Es[:3, :3].T @ Es[:3, 3] - Xw; ray_s /= np.maximum(np.linalg.norm(ray_s, axis=1, keepdims=True), 1e-12)
            gm = gm & (np.sum(ray_r * ray_s, 1) <= cos_min).reshape(ref.shape)
            d_rep = np.where(gm, d_rep, 0)
        gs += gm.astype(np.int32); acc += d_rep
    d_avg = (acc + ref) / (gs + 1)
    col = np.asarray(Image.open(f"{FX}/rgb/{f:08d}.jpg").convert("RGB").resize((W, H)))
    kept = {}
    for t in a.mask_thres:
        final = photo & (gs >= t); d = np.where(final, d_avg, 0.0); kept[t] = int(final.sum())
        np.savez(f"{OUT}_m{t}/{f:08d}.npz", d16=np.round(d * 5000).astype(np.uint16), col=np.ascontiguousarray(col), intr=Kr, extr=Er)
    return f, kept, np.bincount(np.minimum(gs[photo], 10), minlength=11)
if __name__ == "__main__":
    for t in a.mask_thres: os.makedirs(f"{OUT}_m{t}", exist_ok=True)
    tot = {t: 0 for t in a.mask_thres}; hist = np.zeros(11, np.int64)
    with ProcessPoolExecutor(8) as ex:
        for f, k, h in ex.map(one, range(NF)):
            hist += h
            for t in a.mask_thres: tot[t] += k[t]
    for t in a.mask_thres:
        json.dump({"order": list(range(NF)), "kept_total": tot[t]}, open(f"{OUT}_m{t}/order.json", "w"))
        print(f"fusion ({a.geo_pixel} px, >= {t} votes incl. self, sat-tol {a.sat_tol}, tri-angle >= {a.min_tri_angle} deg): kept {tot[t]:,} pixels -> {OUT}_m{t}")
    print("photo-passing pixels by consistent-view count incl. self (0..9, >=10):", hist.tolist())
