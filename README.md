# WorldsWithin Official Pipeline

Source for a local 3D reconstruction pipeline. It turns photographs and
camera metadata into a textured GLB through structure from motion (SfM),
multi-view stereo (MVS), geometric fusion, meshing, and texturing.

| Source | Role |
| --- | --- |
| [lib/reconstruction](lib/reconstruction) | Stage orchestration, frame transport, and resource management |
| [python](python) | Capture preparation, reconstruction stages, and runtime bindings |
| [native](native) | C and C++ stages, GPU kernels, and native interfaces |
| [vendor/official_sfm](vendor/official_sfm) | SfM core, DSP-SIFT shaders, and interfaces |

This source release requires a host with native libraries, GPU support,
and matching deployment models and execution plans. See
[pipeline parameters](PARAMETERS.md) for the capture format, stage settings,
and local runtime inputs.

Project code is available under the [MIT License](LICENSE). See
[THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES) for third-party terms.
