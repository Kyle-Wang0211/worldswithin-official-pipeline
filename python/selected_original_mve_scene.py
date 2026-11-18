#!/usr/bin/env python3




import os, json
import numpy as np

FX, OUT = "capture", "scene"
meta = json.load(open(f"{FX}/frames.json")); NF, W, H = meta["count"], meta["width"], meta["height"]
CM = np.fromfile(f"{FX}/cams.f32", np.float32).reshape(NF, 36).astype(np.float64)
os.makedirs(OUT, exist_ok=True)
for i in range(NF):
    K = CM[i, 0:9].reshape(3, 3); R = CM[i, 9:18].reshape(3, 3); t = CM[i, 18:21]
    with open(f"{OUT}/{i:08d}.cam", "w") as fo:
        fo.write(" ".join(f"{v:.17g}" for v in list(t) + list(R.ravel())) + "\n")
        fo.write(" ".join(f"{v:.17g}" for v in (K[0, 0] / max(W, H), 0.0, 0.0, K[1, 1] / K[0, 0],
                                                 (K[0, 2] + 0.5) / W, (K[1, 2] + 0.5) / H)) + "\n")
    dst = f"{OUT}/{i:08d}.jpg"
    if not os.path.lexists(dst):
        os.symlink(f"{FX}/rgb/{i:08d}.jpg", dst)
print(f"{NF} views -> {OUT}; first cam:")
print(open(f"{OUT}/00000000.cam").read())
