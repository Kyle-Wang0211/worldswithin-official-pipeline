#!/usr/bin/env python3



import trimesh, os, re
from PIL import Image
O = "workspace"; m = "mesh"; N = "normals"
sc = trimesh.load(f"{O}/tex_{m}_1920/{m}.obj", force="scene", process=False)
out = trimesh.Scene()
for name, g in sc.geometry.items():
    mat = g.visual.material; mn = getattr(mat, "name", None) or name; i = int(re.search(r"material(\d{4})", mn).group(1))
    img = mat.image if hasattr(mat, "image") else mat.baseColorTexture
    pbr = trimesh.visual.material.PBRMaterial(name=f"m{i:04d}", baseColorTexture=img, normalTexture=Image.open(f"{N}/Normal_{1001+i}.png").convert("RGB"),
                                              metallicFactor=0.0, roughnessFactor=1.0)
    g.visual = trimesh.visual.TextureVisuals(uv=g.visual.uv, material=pbr); out.add_geometry(g, geom_name=f"g{i:04d}")
p = f"output.glb"; out.export(p)
print("GLB", round(os.path.getsize(p)/1e6), "MB", len(out.geometry), "meshes", sum(len(g.faces) for g in out.geometry.values()), "faces")
