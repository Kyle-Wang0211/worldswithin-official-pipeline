#!/usr/bin/env python3
"""Prepare multi-view stereo inputs from capture photos, intrinsics, and camera poses.
"""






















from __future__ import annotations
import json, os, sys
import numpy as np
from PIL import Image

O = sys.argv[1] if len(sys.argv) > 1 else \
    os.environ["RECONSTRUCTION_CAPTURE_ROOT"]
OUT = sys.argv[2] if len(sys.argv) > 2 else f"{O}/mvs_inputs"
W = int(os.environ.get("FIX_W", 768))
H = int(os.environ.get("FIX_H", 576))
NSRC = int(os.environ.get("FIX_NSRC", 4))
assert abs(W/H - 4032/3024) < 1e-3, f"{W}×{H} 不是 4:3 —— 素材是 4032×3024,拉伸会引入训练分布外的形变"
assert W % 32 == 0 and H % 32 == 0, f"{W}×{H} 不是 32 的倍数,四级金字塔会错位"






THETA0, SIG1, SIG2, MIN_BASE = 5.0, 1.0, 10.0, 0.06



USE_ARKIT = os.environ.get("USE_ARKIT", "0") == "1"
DR_LO = float(os.environ.get("DR_LO", 0.70))
DR_HI = float(os.environ.get("DR_HI", 1.50))
FRAME_GATE = os.environ.get("FRAME_GATE", "0") == "1"
DT_MAX = float(os.environ.get("DT_MAX", 0.10))





os.makedirs(OUT, exist_ok=True)


def q2R(q):
    w, x, y, z = q
    return np.array([[1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)],
                     [2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x)],
                     [2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y)]])


meta = json.load(open(f"{O}/official_sfm_sparse_meta.json"))
fed = {json.loads(l)["frameId"]: json.loads(l)
       for l in open(f"{O}/official_sfm_fed_frames.jsonl") if l.strip()}
poses = [p for p in meta["poses"] if p.get("registered")]
poses.sort(key=lambda p: p["frame_id"])
NF = len(poses)
print(f"注册帧 {NF}(meta 共 {len(meta['poses'])} 条)")


raw = open(f"{O}/official_sfm_sparse.ply", "rb").read()
he = raw.index(b"end_header\n") + 11
npt = int([l for l in raw[:he].decode("latin1").split("\n")
           if "element vertex" in l][0].split()[-1])
rec = np.frombuffer(raw[he:he+npt*15],
                    dtype=[("x", "<f4"), ("y", "<f4"), ("z", "<f4"),
                           ("r", "u1"), ("g", "u1"), ("b", "u1")])
P = np.stack([rec["x"], rec["y"], rec["z"]], 1).astype(np.float64)
print(f"稀疏点 {len(P)}")

names, Ks, Rs, ts, Cs, vis = [], [], [], [], [], []
ark_depths, ark_C, bad_frame = [], [], []
for p in poses:
    fid = p["frame_id"]
    jpg = os.path.basename(fed[fid]["jpegPath"])
    sc = json.load(open(f"{O}/sidecars/{jpg.replace('.jpg','.json')}"))



    E = np.array(sc["extrinsic"], float).reshape(4, 4)
    if np.allclose(E.T[3], [0, 0, 0, 1], atol=1e-3):
        E = E.T
    ark_C.append(E[:3, 3])
    aw = sc.get("anchors_world")
    if aw:

        zc = -((np.array(aw, float) - E[:3, 3]) @ E[:3, :3])[:, 2]
        ark_depths.append(zc[zc > 0.05])
    else:
        ark_depths.append(np.array([]))
    bad_frame.append(not sc.get("is_tracking", True)
                     or sc.get("trackingStateName") != "normal"
                     or abs(sc.get("request_to_capture_dt", 0.0)) > DT_MAX)

    fx, fy, cx, cy = sc["intrinsics_fxfycxcy"]
    ow, oh = sc["image_w"], sc["image_h"]
    sx, sy = W/ow, H/oh
    K = np.array([[fx*sx, 0, cx*sx], [0, fy*sy, cy*sy], [0, 0, 1]])
    R = q2R(p["quat_wxyz"]); t = np.array(p["t"], float)
    names.append(jpg); Ks.append(K); Rs.append(R); ts.append(t)
    Cs.append(-R.T @ t)

    Xc = (R @ P.T).T + t
    z = Xc[:, 2]
    u = np.where(z > 0, K[0, 0]*Xc[:, 0]/np.where(z > 0, z, 1)+K[0, 2], -1)
    v = np.where(z > 0, K[1, 1]*Xc[:, 1]/np.where(z > 0, z, 1)+K[1, 2], -1)
    vis.append((z > 0) & (u >= 0) & (u < W) & (v >= 0) & (v < H))
Cs = np.array(Cs)
ark_C = np.array(ark_C)
bad_frame = np.array(bad_frame)





_d1 = np.linalg.norm(np.diff(Cs, axis=0), axis=1)
_d2 = np.linalg.norm(np.diff(ark_C, axis=0), axis=1)
_g = (_d1 > 1e-3) & (_d2 > 1e-3)
ARK_SCALE = float(np.median(_d1[_g] / _d2[_g])) if _g.sum() >= 10 else 1.0
_cv = float(np.std(_d1[_g] / _d2[_g]) / np.mean(_d1[_g] / _d2[_g])) if _g.sum() >= 10 else 9.9
print(f"COLMAP/ARKit 尺度 {ARK_SCALE:.4f}(变异系数 {_cv:.4f},{_g.sum()} 段)")
if USE_ARKIT and _cv > 0.15:
    sys.exit("🔴 轨迹尺度逐段不稳,ARKit 锚点不可用于深度范围 —— 停,别出个假先验")
print(f"追踪异常/曝光延迟>{DT_MAX}s 的帧 {bad_frame.sum()}/{NF}"
      + ("(FRAME_GATE 开:不做源视图,仍各自出深度图)" if FRAME_GATE else "(未启用门控)"))


neigh = np.zeros((NF, NSRC), np.int32)
drange = np.zeros((NF, 2), np.float32)
angs = []
for i in range(NF):
    sc_ = []
    for j in range(NF):
        if i == j:
            continue
        base = np.linalg.norm(Cs[i]-Cs[j])
        if base < MIN_BASE:
            continue
        if FRAME_GATE and bad_frame[j]:
            continue
        com = vis[i] & vis[j]
        if com.sum() < 20:
            continue
        X = P[com]
        v1 = X-Cs[i]; v2 = X-Cs[j]
        v1 /= np.linalg.norm(v1, axis=1, keepdims=True)+1e-12
        v2 /= np.linalg.norm(v2, axis=1, keepdims=True)+1e-12
        th = np.degrees(np.arccos(np.clip((v1*v2).sum(1), -1, 1)))
        s = np.where(th <= THETA0, np.exp(-(th-THETA0)**2/(2*SIG1**2)),
                     np.exp(-(th-THETA0)**2/(2*SIG2**2))).sum()
        sc_.append((s, float(np.median(th)), j))
    sc_.sort(reverse=True)
    picked = [j for _, _, j in sc_[:NSRC]]
    while len(picked) < NSRC:
        d = np.linalg.norm(Cs-Cs[i], axis=1); d[i] = 1e9
        for q in picked:
            d[q] = 1e9
        picked.append(int(np.argmin(d)))
    neigh[i] = picked
    if sc_:
        angs.append(np.median([a for _, a, _ in sc_[:NSRC]]))

    zc = ((Rs[i] @ P[vis[i]].T).T + ts[i])[:, 2] if vis[i].sum() else np.array([])
    zc = zc[zc > 0]
    if USE_ARKIT and ark_depths[i].size >= 30:


        zc = np.concatenate([zc, ark_depths[i] * ARK_SCALE])
    if len(zc) >= 8:
        drange[i] = (np.percentile(zc, 2)*DR_LO, np.percentile(zc, 99.5)*DR_HI)
    else:
        drange[i] = (0.3, 4.0)
neigh.tofile(f"{OUT}/neighbors.i32")
if angs:
    print(f"中位三角化角 {np.median(angs):.2f}°  (p10 {np.percentile(angs,10):.2f}° "
          f"p90 {np.percentile(angs,90):.2f}°)")
else:
    print("无角度打分配对，三角化角统计不可用")
print(f"深度范围 中位 lo={np.median(drange[:,0]):.2f}m hi={np.median(drange[:,1]):.2f}m")


cams = np.zeros((NF, 36), np.float32)
for i in range(NF):
    cams[i, 0:9] = Ks[i].reshape(-1)
    cams[i, 9:18] = Rs[i].reshape(-1)
    cams[i, 18:21] = ts[i]
    cams[i, 21:24] = Cs[i]
    cams[i, 24], cams[i, 25] = drange[i]
    cams[i, 26], cams[i, 27] = W, H
cams.tofile(f"{OUT}/cams.f32")


os.makedirs(f"{OUT}/rgb", exist_ok=True)
imgs = np.zeros((NF, H, W), np.float16)
for i, n in enumerate(names):
    im = Image.open(f"{O}/photos_jpg/{n}").convert("RGB").resize((W, H), Image.BILINEAR)
    im.save(f"{OUT}/rgb/{i:08d}.jpg", quality=95)
    imgs[i] = (np.asarray(im.convert("L"), np.float32)/255.0).astype(np.float16)
    if i % 25 == 0:
        print(f"  解码 {i}/{NF}", flush=True)
imgs.tofile(f"{OUT}/images.f16")

json.dump({"count": int(NF), "width": W, "height": H, "num_src": NSRC,
           "source": os.path.basename(O),
           "names": names,
           "frame_ids": [int(p["frame_id"]) for p in poses],
           "drange": drange.tolist(),
           "pose_convention": "refined COLMAP/OpenCV axes (投影实测判定,未补 diag(1,-1,-1))"},
          open(f"{OUT}/frames.json", "w"), ensure_ascii=False)
print(f"→ {OUT}/  images.f16({imgs.nbytes/1e6:.0f}MB) cams.f32 neighbors.i32 frames.json rgb/")
