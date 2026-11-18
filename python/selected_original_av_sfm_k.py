#!/usr/bin/env python3



import json, sys, numpy as np
O = "workspace"; k = sys.argv[5]; out = sys.argv[1]; ROT_MODE = sys.argv[2]; IMG = sys.argv[3]; with_struct = sys.argv[4] == "1"
z = np.load(f"{O}/av_vis_{k}.npz"); cams = z["cams"]; W, H = 4032, 3024
S = lambda x: repr(float(x))
views, intr, poses = [], [], []
for i, c in enumerate(cams):
    f, cx, cy = c[:3]; t = c[3:6]; R = c[6:15].reshape(3, 3); Cc = -R.T @ t
    Rs = R if ROT_MODE == "w2c" else R.T
    views.append({"viewId": str(i), "poseId": str(i), "intrinsicId": str(i), "path": f"{IMG}/{i}.png", "width": str(W), "height": str(H), "metadata": ""})
    intr.append({"intrinsicId": str(i), "width": str(W), "height": str(H), "sensorWidth": "36", "sensorHeight": "27", "serialNumber": "", "type": "pinhole",
                 "initializationMode": "calibrated", "initialFocalLength": "-1", "focalLength": S(f * 36.0 / W), "pixelRatio": "1", "pixelRatioLocked": "true",
                 "offsetLocked": "false", "scaleLocked": "false", "principalPoint": [S(cx - W / 2.0), S(cy - H / 2.0)],
                 "distortionInitializationMode": "none", "distortionParams": "", "undistortionOffset": ["0", "0"], "undistortionParams": "",
                 "distortionType": "none", "undistortionType": "none", "locked": "true"})
    poses.append({"poseId": str(i), "pose": {"transform": {"rotation": [S(x) for x in Rs.ravel()], "center": [S(x) for x in Cc]},
                  "locked": "true", "rotationOnly": "false", "removable": "true"}})
d = {"version": ["1", "2", "12"], "featuresFolders": "", "matchesFolders": "", "views": views, "intrinsics": intr, "poses": poses}
if with_struct:
    V, col, vi, cj, ox, oy = z["V"], z["col"], z["vi"], z["cj"], z["ox"], z["oy"]
    starts = np.searchsorted(vi, np.arange(len(V))); ends = np.searchsorted(vi, np.arange(len(V)), side="right"); st = []
    for p in range(len(V)):
        a, b = starts[p], ends[p]
        if a == b: continue
        st.append({"landmarkId": str(p), "descType": "unknown", "color": [str(x) for x in col[p]], "X": [S(x) for x in V[p]],
                   "observations": [{"observationId": str(cj[q]), "featureId": str(p), "x": [S(ox[q]), S(oy[q])], "scale": "0"} for q in range(a, b)]})
    d["structure"] = st
json.dump(d, open(out, "w")); print(out, len(views), "views", len(d.get("structure", [])), "landmarks")
