# Task 15: Simple diffuse path tracer (offline CPU and CUDA)

**Before the lab:** both backends are implemented. CPU tests can run locally.
NVCC compilation, actual GPU execution, GPU images, and same-machine benchmark
results must be verified/generated on the lab's NVIDIA computer. No GPU timings
or CUDA images are claimed from the CPU-only laptop.

## Tomorrow: one-command workflow

Bring the updated repository, including `src/ShootingRaysCUDA/CudaMath.h` and
`include/stb_image_write.h`. Do not copy laptop build directories to the lab.
From the repository root:

```bash
git switch project
make lab-SimplePathTracer
```

Equivalent: `bash scripts/run_simple_path_tracer.sh`. It runs without a window
or display server. Requirements: NVIDIA GPU/driver, CUDA Toolkit (`nvcc` on
PATH), a toolkit-supported host compiler, CMake **3.24+**, and standard C++17
build tools. The standalone build does **not** need GLM, GLFW, OpenGL, or Python.

The script checks the environment, builds both executables, runs CPU/GPU tests
and RGB validation, optionally runs Compute Sanitizer, then measures both
backends on this computer in direct and multi-bounce modes. It renders a final
960 × 720, 256-sample, five-surface path-traced PNG on the GPU.

Results go into a fresh `src/SimplePathTracer/output/lab-<UTC timestamp>-<PID>/`:

| File | Meaning |
| --- | --- |
| `submission.png` and `.png.txt` | Final GPU image and render settings/timing |
| `direct/cpu.png`, `direct/cuda.png` | Matching direct-light benchmark images |
| `path/cpu.png`, `path/cuda.png` | Matching multi-bounce benchmark images |
| `direct/benchmark.csv`, `path/benchmark.csv` | Individual measured trials, seconds |
| `direct/summary.txt`, `path/summary.txt` | Median times and speedup |
| `lab_run.log` | Hardware/toolchain/build/validation evidence |
| `SUCCESS.txt` | Created only after all required steps succeed |

Inspect the image and check `SUCCESS.txt`. If the script fails, keep the log,
investigate, and rerun into a new directory. Never submit a leftover image as
evidence of a successful run. Outputs are not automatically committed.

### Overrides

```bash
CPU_THREADS=8 BUILD_JOBS=4 make lab-SimplePathTracer
WIDTH=480 HEIGHT=360 SAMPLES=32 FINAL_SAMPLES=128 make lab-SimplePathTracer
CUDA_DEVICE_INDEX=0 PATH_TRACER_BUILD_DIR=build-cuda/path-tracer-fresh make lab-SimplePathTracer
```

Other overrides: `BOUNCES`, `TRIALS` (3..30), `SEED`, `FINAL_WIDTH`,
`FINAL_HEIGHT`, `FINAL_BOUNCES`, `OUTPUT_DIR` (must not exist),
`CUDA_ARCHITECTURES`, `CUDA_HOST_COMPILER`. Device indices are relative to
`CUDA_VISIBLE_DEVICES`. Do not assume a particular GPU architecture.

## Manual build / local checks

CPU-only on the laptop (CUDA is optional and off by default):

```bash
make test-SimplePathTracer
./build/path-tracer/SimplePathTracerCPU --render \
  --width 320 --height 240 --samples 16 --bounces 4 \
  --output /tmp/opencode/path-preview.png
```

Portable standalone build for any machine:

```bash
cmake -S src/SimplePathTracer -B build/path-tracer -DCMAKE_BUILD_TYPE=Release
cmake --build build/path-tracer --target SimplePathTracerCPU -j 4
```

CUDA in the lab:

```bash
cmake -S src/SimplePathTracer -B build-cuda/path-tracer \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA_PATH_TRACER=ON
cmake --build build-cuda/path-tracer \
  --target SimplePathTracerCPU SimplePathTracerCUDA -j 4

./build-cuda/path-tracer/SimplePathTracerCPU --self-test
./build-cuda/path-tracer/SimplePathTracerCUDA --device-info
./build-cuda/path-tracer/SimplePathTracerCUDA --self-test
./build-cuda/path-tracer/SimplePathTracerCUDA --validate
./build-cuda/path-tracer/SimplePathTracerCUDA --benchmark \
  --width 640 --height 480 --samples 64 --bounces 4 --trials 3 \
  --output-dir src/SimplePathTracer/output/manual-benchmark
```

The CUDA executable's `--benchmark` measures **both** CPU and GPU in one run,
using the same scene, settings, seed, and compiler build. CPU-only
`--benchmark` is also available but cannot provide a GPU comparison.
`--render` selects the executable's backend. There is no CUDA-to-CPU fallback.

Both targets are also registered in root CMake; enable
`ENABLE_CUDA_PATH_TRACER` there to build the GPU target. This option is separate
from Task 14's `ENABLE_CUDA_RAYCASTER`; neither changes previous assignments.
Root builds still require the repository's graphics dependencies.

### CLI

Use `--help` for commands. Shared render settings:

- `--width`, `--height`: 1..4096; default 640 × 360.
- `--samples`: 1..65536, default 32. More samples reduce Monte Carlo noise.
- `--bounces`: 1..32, default 4; counts diffuse surface evaluations, including
  the first primary hit. One means direct lighting only.
- `--mode path|direct`: default path; direct forces one diffuse evaluation.
- `--seed`: uint32, default 42; fixed across backend and benchmark trials.
- `--threads`: CPU workers, defaults to hardware concurrency, capped by rows.
- `--device`: CUDA device ordinal, default 0.
- `--exposure`: positive finite number, default 1; affects PNG only.
- `--output`: render PNG path. Parent directories are created.
- `--output-dir`: benchmark folder / default render destination.
- `--trials`: measured benchmark trials, default 3 (CLI permits 1 for smoke tests).

Explicit output files are overwritten. Default render output is
`src/SimplePathTracer/output/cpu.png` or `cuda.png`, with `.png.txt` metadata.
Image rows are top-down; every output pixel contains RGB, not depth.

## Rendering method

All non-emissive surfaces are Lambertian diffuse. A 36-triangle procedural
Cornell box has red/green walls, two rotated boxes, and a rectangular area light.
The scene is deterministic and needs no model downloads. Task 14's portable
vector and Moller–Trumbore intersection functions are reused unchanged.

1. Each sample jitters the primary ray within its pixel.
2. Scan all triangles for the closest positive intersection; no BVH/k-d tree.
3. Uniformly sample the area light and cast a finite-length shadow ray. Apply
   `albedo/pi * emittedRadiance * cosSurface * cosLight * area / distance²`.
4. In path mode, sample a cosine-weighted hemisphere and continue. The PDF
   cancels the Lambertian cosine/pi, so throughput is multiplied by albedo.
5. Explicit light sampling accounts for diffuse-to-light paths. Emission from
   a light intersection is added only for primary camera visibility, avoiding
   double counting with the light estimator. The light is one-sided.
6. Stop at the fixed surface limit; no Russian roulette, denoiser, textures,
   ambient term, specular reflection, or importance acceleration is used.

Secondary origins are offset by 0.0002 world units. Shadows terminate before
the sampled light. The black environment emits no light. Geometry uses
two-sided intersections with normals oriented against incoming rays; the area
light retains its geometric emission normal.

This is a finite-depth diffuse path tracer with next-event estimation. Direct
mode is the primary/shadow-ray baseline, not a claim of indirect illumination.
Path mode adds indirect light and color bleeding. Finite samples produce noise;
raising samples is preferable to hiding it with an undocumented filter.

Linear RGB is averaged before display conversion: exposure, Reinhard
`c/(1+c)`, then the standard sRGB transfer and 8-bit PNG quantization. No
per-image min/max normalization is used, so both backends have equal exposure.

## CPU/CUDA fairness and timing

Both backends execute the same `pixel()`/`trace()` functions and integer RNG.
RNG streams depend on seed, pixel, and sample, not scheduling. Random draws
are explicitly sequenced. CPU output is deterministic across worker counts.
CUDA uses one thread per pixel, 16 × 16 blocks, and float radiance. No
fast-math; CUDA FMA contraction is disabled for reproducibility.

The CUDA benchmark alternates CPU/GPU order, performs one small and one full
configuration warm-up, then records each trial. Default is three trials;
summary uses medians. Same seed deliberately keeps the workload identical.

- **CPU render wall:** output allocation, worker creation, tracing, and joins.
- **CUDA render wall:** output/device allocation, scene upload, kernel,
  download, and device/event cleanup; device selection/context initialization
  is outside the warmed scope.
- **CUDA kernel:** CUDA events around the rendering kernel only.
- Scene creation, warm-up, validation, tone mapping, PNG encoding, and file I/O
  are excluded from both render timings.

Primary speedup is `CPU median wall / CUDA median wall`. Kernel-only ratio is
explicitly labeled and is not end-to-end speedup. A very small workload may
not be faster on a GPU; record actual results rather than assume an outcome.
Hardware is recorded in the lab log, settings in CSV/summary. Report both CPU
thread count and GPU model with the results. Use the lab CPU results, not
laptop timings, for the requested comparison.

## Tests and validation

Host tests cover known ray distances, misses, normals/material selection,
finite shadow segments, cosine sampling and its mean, a known integer RNG
sequence, deterministic threaded rendering, finite/nonnegative radiance,
black environment, and primary emitter visibility.

CUDA self-tests actually launch a kernel checking intersection, hemisphere
sampling, shadow limits, and the RNG sequence. `--validate` compares raw linear
RGB at 37 × 29 with 16 samples in direct and four-surface path modes, plus empty
1 × 1 and 17 × 19 cases. These exercise non-multiple block bounds.

Validation requires finite nonnegative values and, per RGB component,
`abs(cpu-gpu) <= 0.05 + 0.02*abs(cpu)`, plus aggregate
`RMSE <= 0.002 + 0.003*RMS(cpu)`. It prints max error, RMSE, and failures.
The final matched benchmark images are checked too. Float roundoff can change
near-edge/shadow/bounce decisions; images are not required to be byte-identical.
Do not loosen thresholds simply to pass: preserve logs and investigate a failure.

## Lab troubleshooting

- `nvidia-smi` without `nvcc` is not enough: load the lab's toolkit/module.
- Unsupported host compiler: choose a toolkit-supported compiler using `CXX`
  and `CUDA_HOST_COMPILER`, with a fresh `PATH_TRACER_BUILD_DIR`. Do not start
  by bypassing NVCC compiler checks or installing new drivers.
- Native architecture detection fails: set `CUDA_ARCHITECTURES` to the actual
  GPU capability supported by the toolkit. Task 14 documents the same issue.
- CPU/GPU mismatch: preserve the log; start with `--self-test`, then
  `--validate`. Run Compute Sanitizer if available. No fake/CPU CUDA output.
- Slow run: lower `WIDTH`, `HEIGHT`, and `SAMPLES` equally for both backends;
  keep at least three benchmark trials. Increase final image samples separately.
- There is no viewer to resize: open the saved PNG with an image viewer.

## Submission

1. Finish a lab run with `SUCCESS.txt`, then inspect `submission.png`.
2. Upload that PNG to Task 15.
3. Commit/push source and the chosen lab images, benchmark CSVs, and summaries.
   Include a brief CPU-vs-CUDA timing comparison; no measurements are invented.
4. Review `lab_run.log` for account/machine details before publishing it.
5. Submit the `project` branch link or exact commit link. Do not accidentally
   include unrelated `imgui.ini` or local build changes.

Task 14 remains separately reproducible with `make lab-ShootingRaysCUDA`.
