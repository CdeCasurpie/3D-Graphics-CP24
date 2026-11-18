# Task 14: Shooting rays in CUDA

CUDA counterpart of Task 13: one GPU thread per pixel, primary camera rays,
brute-force two-sided Moller–Trumbore intersections, and nearest surface distance.
No BVH, k-d tree, rasterized depth, or CPU fallback is used. The scene and camera
are shared with Task 13 through `../ShootingRays/Scene.h`.

**Verified on 2026-11-18:** built and executed with CUDA 13.0 on an NVIDIA RTX PRO
4000 Blackwell. CPU/GPU validation passed and Compute Sanitizer reported zero
errors. The actual GPU-generated [submission image](depth_map.png), [results
report](RESULTS.md), timing metadata, and lab log are included.

## Reproduce: one-command lab run

Bring this working tree to the lab (commit/push the code before leaving, or copy
the repository including these new files). Run from the repository root:

```bash
git switch project
make lab-ShootingRaysCUDA
```

Equivalent, including when invoked from another directory:

```bash
bash scripts/run_shooting_rays_cuda.sh
```

The script checks the environment, builds in `build-cuda/`, runs host checks,
actual GPU intersection tests, CPU/GPU image validation, optional Compute
Sanitizer, then renders **960 × 540**. It stops on any failure and prints
`SUCCESS` only after all required steps complete. A display server is not needed.

Generated deliverables:

| File | Purpose |
| --- | --- |
| `src/ShootingRaysCUDA/depth_map.png` | The GPU-generated image to upload |
| `src/ShootingRaysCUDA/depth_map.png.txt` | GPU identity, mapping, and measured timing |
| `src/ShootingRaysCUDA/lab_run.log` | Environment, build, and validation evidence |

Inspect the PNG before submitting. Black background, dark foreground, and lighter
background geometry are expected. It should resemble Task 13's default scene.
If the script fails, **do not submit a leftover PNG from an older run**.

Overrides (environment variables, not source edits):

```bash
WIDTH=1280 HEIGHT=720 CUDA_DEVICE_INDEX=0 make lab-ShootingRaysCUDA
BUILD_JOBS=2 bash scripts/run_shooting_rays_cuda.sh
```

`CUDA_DEVICE_INDEX` is an ordinal among devices visible to the process. If the
lab sets `CUDA_VISIBLE_DEVICES`, index 0 means its first visible GPU.

## Lab prerequisites

- NVIDIA GPU, working NVIDIA driver, and a CUDA Toolkit with C++17 support.
- `nvcc` on `PATH`; `nvidia-smi` alone does not prove the toolkit is installed.
- CMake **3.24 or newer** for this optional target's native GPU detection.
- A host C++ compiler supported by that toolkit. Do not bypass NVCC compiler
  checks with `--allow-unsupported-compiler` as the first workaround.
- Existing repository dependencies: GLM, GLFW and OpenGL development files.
  Root CMake still checks GLFW/OpenGL, although the CUDA executable does not
  create a window or link the viewer. Host scene setup uses existing headers.

Check:

```bash
nvidia-smi
nvcc --version
cmake --version
g++ --version
```

On a managed lab, load its supplied CUDA module/environment first. A typical
Debian/Ubuntu dependency list for the non-CUDA parts is `build-essential`,
`cmake`, `libglm-dev`, `libglfw3-dev`, and `libgl1-mesa-dev`; ask the lab admin
before installing system packages or changing drivers.

## Manual build and commands

```bash
cmake -S . -B build-cuda \
  -DENABLE_CUDA_RAYCASTER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-cuda --target ShootingRaysCUDA CudaRayMathTests -j 4

./build-cuda/CudaRayMathTests
./build-cuda/ShootingRaysCUDA --device-info
./build-cuda/ShootingRaysCUDA --self-test
./build-cuda/ShootingRaysCUDA --validate
./build-cuda/ShootingRaysCUDA --render 960 540
```

`--render` alone defaults to 960 × 540. Optional output path and device:

```bash
./build-cuda/ShootingRaysCUDA --device 0 --render 1280 720 /tmp/depth-cuda.png
```

Paths are relative to the current directory; run from the repo root for default
output. PNG and its adjacent `.txt` file are overwritten by a successful render.
No commit, push, or network upload happens automatically.

The normal `make all` does **not** require CUDA. On your laptop:

```bash
make test-CudaRayMath
make test-ShootingRays
```

The first command checks portable math compiled as ordinary C++, not GPU
execution. The CUDA target is intentionally unavailable until explicitly enabled.

## Implementation

- Host code generates and transforms the same 962 world-space triangles as
  Task 13 and computes inverse(projection × view) once.
- A plain float camera matrix and triangle edge arrays are uploaded. CUDA code
  has no GLM, ImGui, or OpenGL dependencies.
- `depthKernel` uses 16 × 16 thread blocks and checks both pixel bounds. Every
  thread samples a pixel center, unprojects it, normalizes its ray, scans every
  triangle, and writes the smallest positive `t` (world-space radial distance).
- Misses are positive infinity. The CPU downloads the depth array, finds finite
  min/max, normalizes, and encodes the PNG. These host postprocessing steps do
  not perform ray casting.
- Intensity is `0.08 + 0.92 * clamp((t-min)/(max-min), 0, 1)` for hits; misses
  are black. A constant hit range uses its midpoint. Image rows are top-down.
- All CUDA calls that perform work are checked; allocation/event resources use
  RAII cleanup even when operations fail. Destructors release resources without
  throwing. There is no CPU fallback on runtime failure.
- No fast-math flags; fused multiply-add contraction is disabled in NVCC for
  more reproducible comparisons with the original CPU expression ordering.
  The cached inverse matrix costs a few arithmetic operations per pixel but
  keeps the camera math directly comparable to Task 13.

## Validation and timing

`--self-test` launches a GPU kernel on known rays: expected distances, misses,
behind-camera and parallel rays, two-sided hits, nearest hit, degenerate
triangles, and an empty scene.

`--validate` compares raw float depths with the independent Task 13 CPU renderer
at 317 × 179, a changed-camera 160 × 90 view, and 1 × 1; it also checks a 17 × 19
empty scene. Odd dimensions test the launch bounds. It requires identical hit
masks and absolute depth error <= `0.0002 + 0.00002 * abs(cpuDepth)`, prints maximum
observed error, and fails on any out-of-tolerance pixel. NaNs and nonpositive
distances fail. Do not loosen validation merely to get a submission image.

Optional GPU memory check (also run by the script if installed):

```bash
compute-sanitizer --tool memcheck --error-exitcode 1 \
  ./build-cuda/ShootingRaysCUDA --self-test
compute-sanitizer --tool memcheck --error-exitcode 1 \
  ./build-cuda/ShootingRaysCUDA --validate
```

Rendering warms up with one 32 × 18 launch before timing. Reports distinguish:

- Synchronous triangle upload time.
- Kernel time measured using CUDA events.
- Synchronous depth download time.
- Render-call wall time including allocations, excluding resource cleanup.
- Render + normalization wall time including cleanup, excluding scene setup,
  warm-up, PNG encoding, and file I/O.

No speedup claim is made before measurements. CPU total time and GPU kernel-only
time are not equivalent metrics.

## Troubleshooting in the lab

- **Missing nvcc:** load the toolkit module or add its `bin` directory to PATH.
  If necessary configure with `-DCMAKE_CUDA_COMPILER=/path/to/nvcc` and
  `-DCUDAToolkit_ROOT=/path/to/cuda` in a fresh build directory.
- **Unsupported host compiler:** select a toolkit-supported compiler at first
  configuration with `-DCMAKE_CUDA_HOST_COMPILER=/path/to/g++` (and a compatible
  `CMAKE_CXX_COMPILER`). Do not reuse a cache initialized with another compiler.
- **Architecture detection:** default architecture is `native`. To specify the
  lab GPU's supported compute capability explicitly, use, for example,
  `CUDA_ARCHITECTURES=86 make lab-ShootingRaysCUDA` **only for an appropriate GPU**.
  Choose the value for the actual device/toolkit; do not assume every lab GPU
  is architecture 86. If an older GPU is no longer supported by the toolkit,
  use the lab's compatible toolkit rather than forcing an unrelated architecture.
- **Different machine/toolkit cached:** use a fresh directory such as
  `CUDA_BUILD_DIR=build-cuda/lab-fresh make lab-ShootingRaysCUDA`.
- **No device / insufficient driver / no kernel image:** inspect `nvidia-smi`,
  driver/toolkit compatibility, selected device, and compiled architecture.
- **Validation failure:** keep `lab_run.log`, note the GPU/toolkit/compiler,
  and investigate before submitting. CPU/GPU output is not required to be
  byte-identical, but it must pass the documented numerical checks.
- **No GUI:** expected; use the headless commands and open/copy the saved PNG.

## Submission checklist

1. Finish the lab script with `SUCCESS` and inspect `depth_map.png`.
2. Upload **`src/ShootingRaysCUDA/depth_map.png`** to Task 14, not Task 13's image.
3. Commit/push the new source, shared scene header, build/script/docs changes,
   and the generated PNG plus metadata. The lab log is optional evidence;
   review it for machine/account information before sharing it.
4. Submit your repository's `project` branch link (or exact commit link).
   Do not include unrelated local changes such as `imgui.ini` accidentally.
