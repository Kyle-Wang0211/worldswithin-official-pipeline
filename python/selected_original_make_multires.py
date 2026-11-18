#!/usr/bin/env python3






import os, sys, numpy as np, cv2
LO, HI, OUT = sys.argv[1], sys.argv[2], sys.argv[3]; TH = float(sys.argv[4]) if len(sys.argv) > 4 else 0.02
for sub in ("depth", "conf0", "conf1", "conf2"): os.makedirs(f"{OUT}/{sub}", exist_ok=True)
share = []
for f in range(97):
    im1 = np.load(f"{LO}/depth/{f:04d}.npy").astype(np.float32); im2 = np.load(f"{HI}/depth/{f:04d}.npy").astype(np.float32)
    im1 = cv2.resize(im1, im2.shape[::-1])
    mask = np.abs(im1 - im2) < TH * im1
    np.save(f"{OUT}/depth/{f:04d}.npy", np.where(mask, im2, im1).astype(np.float32))
    for k in range(3):
        c1 = cv2.resize(np.load(f"{LO}/conf{k}/{f:04d}.npy").astype(np.float32), im2.shape[::-1], interpolation=cv2.INTER_NEAREST)
        c2 = np.load(f"{HI}/conf{k}/{f:04d}.npy").astype(np.float32)
        if c2.shape != im2.shape: c2 = cv2.resize(c2, im2.shape[::-1], interpolation=cv2.INTER_NEAREST)
        np.save(f"{OUT}/conf{k}/{f:04d}.npy", np.where(mask, c2, c1).astype(np.float32))
    share.append(mask.mean())
print(f"multires th={TH}: high-res depth kept on {np.mean(share)*100:.1f}% of pixels (per view min {np.min(share)*100:.1f}%, max {np.max(share)*100:.1f}%) -> {OUT}")
