# Task 16: real-time path tracing with Embree and OptiX

Progressive path tracing with **diffuse walls, an ideal specular mirror sphere,
and a dielectric glass sphere**. Embree 4 accelerates CPU intersections; OptiX
accelerates GPU intersections. Neither production backend scans triangles.

**Verified lab run (November 18, 2026):** Embree and OptiX compiled and passed
actual CPU/GPU validation on computer `014`. Matching FPS benchmarks, rendered
stills, and camera-demo recordings were generated. See [RESULTS.md](RESULTS.md)
for measured performance, evidence paths, and submission instructions.

## Reproduce: quickest lab run

Requirements:

- NVIDIA GPU supported by the selected OptiX SDK and its required driver.
  An RTX-class GPU is recommended; verify exact SDK/device compatibility.
- Full CUDA Toolkit with `nvcc`, supported host compiler, CMake **3.24+**.
- **Embree 4** development package / CMake config (`embree-config.cmake`).
- OptiX SDK **7.7 or newer**, with `include/optix.h`, `optix_stubs.h`, and
  `optix_stack_size.h`. CUDA alone does **not** include these headers.
- For the viewer/video: GLFW, OpenGL, GLM development headers, a graphical
  session, Python 3 and FFmpeg with the `libx264` encoder.

Set the SDK path and run from the repository root:

```bash
export OPTIX_ROOT=/path/to/OptiX-SDK
make lab-RealtimePathTracer
```

Equivalent: `bash scripts/run_realtime_path_tracer.sh`. No package installation,
driver changes, SDK downloads, commit, push, or upload happen automatically.
Ask the lab administrator before changing system software.

If the lab does not have the SDK, obtain NVIDIA's SDK and follow its license,
or obtain NVIDIA's official public header repository before the session:

```bash
git clone https://github.com/NVIDIA/optix-dev.git /path/to/optix-dev
export OPTIX_ROOT=/path/to/optix-dev
```

The public repository's latest SDK may require a newer driver than the lab
provides. Select a documented compatible SDK/release; don't assume "latest"
works everywhere. Do not commit downloaded SDK/toolkit files to this repo.

The script:

1. Checks NVIDIA/toolchain/SDK/display/recording prerequisites.
2. Builds Embree + OptiX and generates the OptiX IR device module.
3. Runs independent reference-intersection/material tests for both backends.
4. Compares linear RGB at fixed settings; optionally runs Compute Sanitizer.
5. Benchmarks **640 × 480, 1 spp, 8 bounces**, 120 frames after 10 warm-up frames
   for both backends on the **same lab computer**.
6. Saves **960 × 720, 256 spp** reference images from both backends.
7. Opens each viewer, tests camera motion, then records a 15-second camera tour
   with the actual framebuffer, backend label and FPS overlay.
8. Encodes both clips and combines them into `demo.mp4`.

Results are placed in a fresh directory:

```text
src/RealtimePathTracer/output/lab-<UTC timestamp>-<PID>/
    embree/benchmark.csv, summary.txt, render.png, render.png.txt
    optix/benchmark.csv, summary.txt, render.png, render.png.txt
    embree/demo.mp4, optix/demo.mp4
    embree/capture/viewer.csv, optix/capture/viewer.csv
    comparison.txt
    demo.mp4
    lab_run.log
    SUCCESS.txt
```

`SUCCESS.txt` is only written after requested checks complete. Inspect images
and video before submitting. A failed run keeps its logs; rerun into a new
folder rather than confuse stale outputs with successful results.

### Useful overrides

```bash
CPU_THREADS=8 BUILD_JOBS=4 make lab-RealtimePathTracer
WIDTH=480 HEIGHT=360 FRAMES=120 FINAL_SAMPLES=128 make lab-RealtimePathTracer
REALTIME_BUILD_DIR=build-cuda/realtime-fresh CUDA_HOST_COMPILER=/usr/bin/g++-12 make lab-RealtimePathTracer
```

Also supported: `CUDA_DEVICE_INDEX`, `EMBREE_DIR`, `OPTIX_COMPUTE_ARCH`, `SPP`,
`BOUNCES`, `WARMUP`, `FINAL_WIDTH`, `FINAL_HEIGHT`, `DEMO_SECONDS` (1..300), `DEMO_SPP` (default 2),
`OUTPUT_DIR` (must not exist), `KEEP_CAPTURE_FRAMES=1`.

Default device target is **compute_75**, suitable for Turing and newer devices
with compatible SDK/driver/toolkit. Override `OPTIX_COMPUTE_ARCH` for the
actual GPU/toolkit. Some recent toolkits dropped older offline targets.
The selected OptiX SDK can impose stricter GPU requirements than the device target.
Device index is relative to `CUDA_VISIBLE_DEVICES`, not necessarily the
physical index shown by `nvidia-smi`.

## Laptop / Embree-only build

```bash
make test-RealtimePathTracer
make run-RealtimePathTracer
```

Or explicitly:

```bash
cmake -S src/RealtimePathTracer -B build/realtime-path-tracer -DCMAKE_BUILD_TYPE=Release
cmake --build build/realtime-path-tracer -j 4
./build/realtime-path-tracer/PathTracer --self-test
./build/realtime-path-tracer/PathTracerViewer --viewer
```

Embree 4 is required. For a nonstandard installation set `-Dembree_DIR=...`.
The assignment has a standalone CMake build so older root targets remain
unchanged and do not acquire Embree or OptiX requirements.

Headless CPU build (no GLFW, GLM or OpenGL dependency):

```bash
cmake -S src/RealtimePathTracer -B build/realtime-headless \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_REALTIME_VIEWER=OFF
cmake --build build/realtime-headless -j 4
./build/realtime-headless/PathTracer --render --backend embree \
  --width 640 --height 480 --samples 128 --output /tmp/path16.png
```

## Manual OptiX build

```bash
cmake -S src/RealtimePathTracer -B build-cuda/realtime-path-tracer \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_OPTIX=ON -DOPTIX_ROOT="$OPTIX_ROOT"
cmake --build build-cuda/realtime-path-tracer -j 4
./build-cuda/realtime-path-tracer/PathTracer --self-test --backend optix
./build-cuda/realtime-path-tracer/PathTracer --validate
./build-cuda/realtime-path-tracer/PathTracerViewer --viewer --backend optix
```

The device module's absolute build path is embedded in the executable; rebuild after
moving the build directory, or pass `--ptx /path/to/PathTracerKernel.ptx`.
Don't copy laptop binaries/build caches to a different machine.

The current build generates `PathTracerKernel.optixir` with `nvcc --optix-ir`.
The legacy `--ptx` option accepts either PTX or OptiX IR. CUDA 13 invokes
`ptxas` validation for PTX output and can reject the intentionally unresolved
OptiX intrinsics, even with relocatable device code; OptiX IR avoids this.
On the Blackwell lab machine use `OPTIX_COMPUTE_ARCH=120`.

No display in the lab? `RECORD_DEMO=0 make lab-RealtimePathTracer` builds/runs
headless validation and benchmarks. It explicitly reports that the camera video
is still missing. Finish recording on a graphical NVIDIA session afterward.

## Viewer controls and real-time behavior

- Left drag: orbit. Right drag: pan. Wheel: zoom. Escape: exit.
- Select Embree/OptiX in an OptiX-enabled viewer. Backend changes wait until the
  current batch finishes; only one backend renders at a time.
- While moving: **320 × 240** preview. After stopping: selected still resolution
  and progressive accumulation. Settings/camera/backend changes reset samples.
- Samples/frame and bounce count affect quality/performance equally on both
  backends. Exposure affects display only; no per-image min/max normalization.
- Pause, reset camera, save clean PNG, or enable a scripted camera tour. Tours
  pause between moves to demonstrate progressive refinement, not just noisy
  moving previews.
- Landscape window is fixed at 1280 × 720; rendered resolution is independent.
  If Hyprland still tiles it, focus it and use the floating-window shortcut.
- The viewer never writes the tracked `imgui.ini`.

**New-image FPS** counts intervals between completed rendered images, not the
fast UI refresh loop. **Tracing/readback FPS** is 1/backend batch wall time (OptiX includes
readback). The UI stays responsive while a worker renders. One completed old
camera preview can be shown while a newer view is being computed; stale views
are never mixed into the new accumulation.

CPU frames use multiple workers and Embree's BVH; GPU frames use OptiX GAS
triangle traversal. Scene/BVH/pipeline are built once. OptiX device buffers are
reused. RGB is copied to the host, accumulated, then uploaded to an OpenGL
texture. This is intentionally simple and portable; it is not CUDA/OpenGL
zero-copy interop. Resolution can be reduced if the device cannot sustain the
desired interactive FPS. "Real-time" is measured, not promised for all hardware.

## Recording the required submission URL

The lab script's `demo.mp4` contains actual framebuffer captures from scripted
camera tours, not offline renders played at an invented FPS. Frame timestamps
come from `viewer.csv`; capture is throttled to at most 15 Hz without forcing
the renderer to run at 15 FPS. Recording/disk overhead affects interactive FPS;
use the separate headless benchmark for the controlled comparison.

For **manual camera interaction** (recommended for the instructor), run:

```bash
./build-cuda/realtime-path-tracer/PathTracerViewer --viewer --backend optix \
  --capture-dir src/RealtimePathTracer/output/manual/capture --capture-seconds 45
# During those 45 seconds, drag/zoom, pause to accumulate, and optionally switch backend.
python3 scripts/encode_realtime_demo.py \
  src/RealtimePathTracer/output/manual/capture \
  src/RealtimePathTracer/output/manual/demo.mp4 --remove-frames
```

Always use a fresh capture directory (don't mix old PPMs/CSV). Recording PPMs
can temporarily use hundreds of MB. The encoder only removes its explicitly
encoded PPM files after successful MP4 encoding. CSV and MP4 are preserved.
You can use OBS instead if preferred.

Upload the MP4 to an **unlisted YouTube video** or an instructor-accessible
Drive link. Check playback and sharing permissions from another browser/session.
No account upload or URL creation is automated.

## FPS comparison and CLI

`PathTracer --help` lists settings. Example:

```bash
./build-cuda/realtime-path-tracer/PathTracer --benchmark --backend embree \
  --width 640 --height 480 --samples 1 --bounces 8 --frames 120 --warmup 10 \
  --output-dir src/RealtimePathTracer/output/manual/embree
```

Repeat with `--backend optix` and matching settings. CLI accepts `--threads`,
`--device`, `--seed`, `--output`, and `--output-dir`. Rendering supports up to
4096 × 4096 and 65536 samples; large CPU renders can take time.

Headless timing includes output allocation, shading/tracing, worker startup/
join for Embree, launch-parameter upload and RGB readback for OptiX. It excludes
construction/BVH/JIT, warmup, tone mapping, file I/O and display. OptiX launch
event time is a separate CSV field, never mislabeled as end-to-end FPS.
Report mean FPS (`frames / total seconds`), median-frame FPS, and p95 frame
seconds. Build time is reported separately. Hardware and toolchain go in the
lab log. CPU/GPU use identical camera/scene/seed/spp/bounces/resolution, with
progressing sample indices so each measured frame does new stochastic work.

## Rendering details / scope

The procedural scene has 4440 triangles and six materials. Smooth normals are
interpolated for the two spheres. Embree and OptiX return the same primitive
indices and barycentrics to a shared CPU/device path integrator.

- Diffuse: Lambertian, cosine-weighted BSDF sampling plus uniform area-light
  sampling and finite shadow rays.
- Specular: **ideal delta mirror**, not rough GGX. This satisfies specular
  surfaces without introducing an unnecessary roughness/MIS estimator.
- Dielectric: ideal lossless glass, IOR 1.5, exact unpolarized Fresnel,
  total internal reflection, and eta² radiance transport on transmission.
  Glass spheres are closed; nested media are not supported.
- Emitters are counted after delta events/camera visibility. Direct
  diffuse-to-light hits are suppressed because explicit light sampling already
  estimates those paths. With only diffuse and delta BSDFs, no general MIS
  weighting is needed to avoid double counting.
- Russian roulette after the fourth interaction, configurable maximum path
  length, geometric-normal origin offsets, black environment, no denoiser.
- Glass is opaque to a straight shadow-occlusion query; refracted illumination
  is instead sampled by continuation paths. Caustics can therefore be noisy.
- Fixed exposure + Reinhard + sRGB for PNG and viewer; raw linear RGB for tests.

OptiX uses a single triangle GAS, a raygen path loop, closest-hit payloads
(primitive/distance/barycentrics), two miss programs and two ray types. Opaque
shadow rays terminate at the first intersection with hit shading disabled;
an any-hit shader is unnecessary for this scene. Stack sizes come from OptiX's
SDK utilities. SDK/driver errors fail explicitly; there is no brute-force or
CPU fallback labeled as OptiX. Only reference **tests** scan all triangles.

## Validation / troubleshooting

`--self-test` checks accelerated nearest-hit distances/material IDs against
independent exhaustive intersections, including an inside-glass ray; Fresnel,
total internal reflection, finite RGB, deterministic repeats and progressive
sample indexing. Odd image sizes exercise launch bounds. `--validate` runs both
backends and compares 63 × 47, 64 spp linear RGB. It requires
`RMSE <= 0.02 + 0.08 * referenceRMS`; maximum error is also printed. Different
BVH edge decisions can diverge glass/roulette paths. Never loosen tolerances
just to produce a submission; preserve logs and investigate failures.

- Missing Embree: load the lab's Embree 4 package/module; set `EMBREE_DIR` to
  its CMake-config folder if needed. Embree 3 has a different API and is not
  silently substituted.
- `optixInit` fails: check SDK/driver compatibility and GPU support.
- Missing device module: build `RealtimeDeviceModule` or correct `--ptx`
  (normally points to `PathTracerKernel.optixir`).
- NVCC rejects host compiler: select a toolkit-supported compiler using
  `CUDA_HOST_COMPILER` and `CXX`, with a fresh `REALTIME_BUILD_DIR`; don't begin
  by bypassing checks or installing drivers.
- Validation failure: preserve GPU model, SDK/toolkit/driver versions and log;
  run Compute Sanitizer and diagnose before submitting.
- Remote/headless machine: run tests there, but the camera video needs an
  actual graphics-capable session. Don't fabricate an interactive recording.

## Submission checklist

1. Complete the lab script and inspect `SUCCESS.txt`, images, FPS summaries.
2. Prefer a short manual-interaction recording showing mirror/glass, orbit,
   zoom, and backend/FPS labels; upload and obtain a viewable URL.
3. Paste that video URL into Task 16's **Web URL** field.
4. Commit/push source plus representative image(s), benchmark CSVs/summaries,
   and the video link in a short result note. Avoid committing huge raw PPMs,
   SDKs, build directories or unrelated `imgui.ini` changes.
5. Review logs for machine/account details before publishing them.

Tasks 13–15 remain separately runnable and unchanged.
