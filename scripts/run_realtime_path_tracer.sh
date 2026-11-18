#!/usr/bin/env bash
# Task 16: validate actual Embree/OptiX backends, benchmark FPS, record camera motion.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
for tool in nvidia-smi nvcc cmake python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing $tool. Load the lab NVIDIA/CUDA/toolchain environment; see src/RealtimePathTracer/README.md" >&2
        exit 1
    fi
done
if [[ -z "${OPTIX_ROOT:-}" || ! -f "$OPTIX_ROOT/include/optix.h" ]]; then
    echo 'Set OPTIX_ROOT to an OptiX SDK/header checkout containing include/optix.h (SDK >= 7.7).' >&2
    exit 1
fi
record="${RECORD_DEMO:-1}"
viewer=ON
if [[ "$record" == 1 ]]; then
    if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
        echo 'No graphical session. Use RECORD_DEMO=0 for headless tests; record the camera demo later.' >&2
        exit 1
    fi
    if ! command -v ffmpeg >/dev/null 2>&1; then
        echo 'ffmpeg is required for automatic MP4 recording (or set RECORD_DEMO=0).' >&2
        exit 1
    fi
else
    viewer=OFF
fi
build="${REALTIME_BUILD_DIR:-build-cuda/realtime-path-tracer}"
output="${OUTPUT_DIR:-src/RealtimePathTracer/output/lab-$(date -u +%Y%m%dT%H%M%SZ)-$$}"
if [[ -e "$output" ]]; then echo "Choose a fresh OUTPUT_DIR: $output already exists" >&2; exit 1; fi
mkdir -p "$output"
{
    date -u '+Task 16 lab UTC: %Y-%m-%dT%H:%M:%SZ'
    git rev-parse HEAD
    git status --short
    nvidia-smi
    nvcc --version
    cmake --version
    if command -v lscpu >/dev/null 2>&1; then lscpu; fi
    configure=(-S src/RealtimePathTracer -B "$build" -DCMAKE_BUILD_TYPE=Release
               -DENABLE_OPTIX=ON "-DBUILD_REALTIME_VIEWER=$viewer" "-DOPTIX_ROOT=$OPTIX_ROOT"
               "-DOPTIX_COMPUTE_ARCH=${OPTIX_COMPUTE_ARCH:-75}")
    if [[ -n "${EMBREE_DIR:-}" ]]; then configure+=("-Dembree_DIR=$EMBREE_DIR"); fi
    if [[ -n "${CUDA_HOST_COMPILER:-}" ]]; then configure+=("-DCMAKE_CUDA_HOST_COMPILER=$CUDA_HOST_COMPILER"); fi
    cmake "${configure[@]}"
    targets=(PathTracer)
    if [[ "$record" == 1 ]]; then targets+=(PathTracerViewer); fi
    cmake --build "$build" --target "${targets[@]}" -j "${BUILD_JOBS:-4}"
    tracer="$build/PathTracer"
    common=(--device "${CUDA_DEVICE_INDEX:-0}")
    if [[ -n "${CPU_THREADS:-}" ]]; then common+=(--threads "$CPU_THREADS"); fi
    "$tracer" --self-test --backend embree "${common[@]}"
    "$tracer" --self-test --backend optix "${common[@]}"
    "$tracer" --validate "${common[@]}"
    if command -v compute-sanitizer >/dev/null 2>&1; then
        compute-sanitizer --tool memcheck --error-exitcode 1 "$tracer" --self-test --backend optix "${common[@]}"
    fi
    for backend in embree optix; do
        "$tracer" --benchmark --backend "$backend" "${common[@]}" \
            --width "${WIDTH:-640}" --height "${HEIGHT:-480}" --samples "${SPP:-1}" \
            --bounces "${BOUNCES:-8}" --frames "${FRAMES:-120}" --warmup "${WARMUP:-10}" \
            --output-dir "$output/$backend"
        "$tracer" --render --backend "$backend" "${common[@]}" \
            --width "${FINAL_WIDTH:-960}" --height "${FINAL_HEIGHT:-720}" \
            --samples "${FINAL_SAMPLES:-256}" --bounces "${BOUNCES:-8}" \
            --output "$output/$backend/render.png"
    done
    python3 - "$output" <<'PY'
import csv, pathlib, statistics, sys
root=pathlib.Path(sys.argv[1]); results={}
for backend in ('embree','optix'):
    with (root/backend/'benchmark.csv').open() as f:
        values=[float(row['wall_seconds']) for row in csv.DictReader(f)]
    results[backend]=1/statistics.median(values)
text=(f"Embree median frame FPS: {results['embree']:.6f}\n"
      f"OptiX median frame FPS: {results['optix']:.6f}\n"
      f"OptiX/Embree median-frame-FPS ratio: {results['optix']/results['embree']:.6f}x\n"
      "Same machine/settings. Headless render + readback only, not display FPS.\n"
      "See backend summaries for dimensions, spp, bounce limit and build times.\n")
(root/'comparison.txt').write_text(text); print(text)
PY
    if [[ "$record" == 1 ]]; then
        for backend in embree optix; do
            "$build/PathTracerViewer" --smoke-test --backend "$backend" "${common[@]}"
            "$build/PathTracerViewer" --viewer --backend "$backend" "${common[@]}" --tour \
                --samples "${DEMO_SPP:-2}" --capture-seconds "${DEMO_SECONDS:-15}" --capture-dir "$output/$backend/capture"
            encode=(scripts/encode_realtime_demo.py "$output/$backend/capture" "$output/$backend/demo.mp4")
            if [[ "${KEEP_CAPTURE_FRAMES:-0}" != 1 ]]; then encode+=(--remove-frames); fi
            python3 "${encode[@]}"
        done
        # Paths are relative to this manifest in the newly-created result folder.
        printf "file 'embree/demo.mp4'\nfile 'optix/demo.mp4'\n" > "$output/demo.ffconcat"
        ffmpeg -hide_banner -loglevel warning -y -f concat -safe 0 -i "$output/demo.ffconcat" \
            -c copy -movflags +faststart "$output/demo.mp4"
        echo "Inspect and upload $output/demo.mp4; automated camera tours are labeled honestly."
    else
        echo "HEADLESS ONLY: camera interaction video is still required for Task 16 submission."
    fi
    printf 'SUCCESS: validation, FPS benchmarks and requested outputs completed.\n' > "$output/SUCCESS.txt"
    echo "SUCCESS: $output (no commit, push, or upload performed)"
} 2>&1 | tee "$output/lab_run.log"
