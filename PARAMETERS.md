# Reconstruction parameters

The pipeline takes a local capture and produces a textured GLB. A host can
render a PNG cover from the result and remove temporary artifacts after use.

## Capture and SfM

Give the host a `photos_highres/` directory with original `.jpg` images and
same-stem `.json` camera sidecars. The host can also read an
`official_photo_bundle.json` file at the capture root. Preserve the image
sizes, intrinsics, poses, and timestamps.

| Input field | Value |
| --- | --- |
| `image_w`, `image_h` | Original JPEG dimensions in pixels |
| `intrinsics_fxfycxcy` | Focal lengths and principal point as `[fx, fy, cx, cy]` in pixels |
| `extrinsic` | 16-element camera transform in the capture format's coordinate convention |
| `t`, `save_target_t` | Capture timestamp and optional save-target timestamp, which defaults to `t` |
| Frame order | Filename capture sequence, then capture timestamp, then path |
| Capture range | 20–150 prepared frames |
| SfM stream | 13,312 max features, 12 neighbors, 0.8 match ratio, GPU extraction and matching enabled |
| CPU DSP-SIFT fallback | Affine shape and domain-size pooling, peak threshold 0.004, edge threshold 15 |

The host first reconstructs the sparse point cloud from the capture. It then
passes the sparse PLY, camera data, and original images to the dense stages.

## Dense reconstruction and output

| Order | Stage | Settings and outputs |
| --- | --- | --- |
| 1 | Low-resolution MVS | 768 × 576, nine source views, depth and three confidence planes |
| 2 | High-resolution MVS | 1536 × 1152, nine source views, depth and three confidence planes |
| 3 | Depth merge | Relative depth threshold 0.02 |
| 4 | Geometric fusion | Pixel threshold 1.0, depth threshold 0.01, mask threshold 2, SAT tolerance 0.00001, minimum triangulation angle 3°, all-view checks on, photo-first off. Outputs PLY and retained D16 depth at scale 5000 with camera and order records |
| 5 | VDBFusion TSDF | Voxel size 0.003, truncation 0.04, space carving off, hole filling on, minimum extraction weight 0.45, unit weighting off |
| 6 | Mesh trim | Open3D connected components, 100-triangle cutoff |
| 7 | Simplification | meshoptimizer, nominal target 100,000 triangles |
| 8 | Mesh cleanup | Lagrange nonmanifold cleanup and triangulation of holes up to size 16 |
| 9 | Color texture | 1920 × 1440 inputs, GMI data term, Gaussian clamping, one thread, intermediate results disabled. Outputs OBJ, MTL, and color PNGs |
| 10 | Visibility | Write the AliceVision SfM scene and mesh visibility bindings |
| 11 | Scene conversion | Convert the SfM scene to Alembic |
| 12 | Normal texture | AliceVision with texture side 2048, downscale 1, OBJ, PNG normals, subdivision ratio 0, UDIM off, one core |
| 13 | GLB | Trimesh export preserves material and geometry ordering and embeds color and normal textures |

Depth bounds use the sparse P2 and P99.5 quantiles multiplied by 0.70 and
1.50, respectively. They require at least eight points and fall back to
0.3–4 m. Source-view selection uses a 5° target angle, 1° and 10° angular
scales, and a 0.06 m minimum baseline. Depth from the capture platform and
frame gating are both disabled. The timestamp tolerance is 0.10 s where
frame gating is enabled.

The [noise generator](tools/generate_noise.py) seeds each frame with
`torch.manual_seed(frame_index)`, drawing stage 2 before stage 3. The two
noise resources use little-endian storage and hold up to 150 frames. A run
consumes only the rows for its actual frame count. The runtime releases SfM
image pyramids after use.

## Host inputs

| Input | Supply for your platform |
| --- | --- |
| Filesystem | Capture, work, and output directories |
| Runtime | Native SfM and downstream libraries, compatible GPU backend, and local package profile |
| Model files | Low- and high-resolution ONNX feature and depth graphs, matching execution plans, and generated noise resources |
| Texturing | AliceVision root and OpenColorIO configuration |

The host supplies the deployment model files `feature_low.onnx`,
`feature_high.onnx`, `rest_low.onnx`, and `rest_high.onnx`, and low- and
high-resolution execution plans that match the graph ordering and tensor
lifetimes. The runtime binds the plans and executes both whole graphs.

The [ONNX exporter](tools/export_reconstruction_onnx.py) takes the matching
CasDiffMVS source, training checkpoint, and prepared fixture for each
resolution. Each fixture has `frames.json`, `images.f16`, `cams.f32`, and
`neighbors.i32`. Use `--fuse-conv3d` for the fused 3D convolution path.
Deployment uses split feature and depth graphs with the corresponding
mixed-precision conversion.
