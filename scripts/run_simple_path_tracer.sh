#!/usr/bin/env bash
# Task 15: same-machine CPU/CUDA validation, benchmarks, and submission image.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

for tool in nvidia-smi nvcc cmake; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing $tool. Load the lab NVIDIA/CUDA environment. See src/SimplePathTracer/README.md" >&2
        exit 1
    fi
done

build="${PATH_TRACER_BUILD_DIR:-build-cuda/path-tracer}"
output="${OUTPUT_DIR:-src/SimplePathTracer/output/lab-$(date -u +%Y%m%dT%H%M%SZ)-$$}"
device="${CUDA_DEVICE_INDEX:-0}"
width="${WIDTH:-640}"
height="${HEIGHT:-480}"
samples="${SAMPLES:-64}"
bounces="${BOUNCES:-4}"
trials="${TRIALS:-3}"
seed="${SEED:-42}"
if [[ ! "$trials" =~ ^[0-9]+$ ]] || ((10#$trials < 3 || 10#$trials > 30)); then
    echo "TRIALS must be 3..30 for the lab comparison." >&2
    exit 1
fi
# Never let a failed run be confused with older submission images.
if [[ -e "$output" ]]; then
    echo "Output already exists: $output. Choose a fresh OUTPUT_DIR." >&2
    exit 1
fi
mkdir -p "$output"
{
    date -u '+Task 15 lab run UTC: %Y-%m-%dT%H:%M:%SZ'
    git rev-parse HEAD
    git status --short
    nvidia-smi
    nvcc --version
    cmake --version
    if command -v lscpu >/dev/null 2>&1; then lscpu; fi
    configure=(-S src/SimplePathTracer -B "$build" -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA_PATH_TRACER=ON)
    if [[ -n "${CUDA_ARCHITECTURES:-}" ]]; then
        configure+=("-DCMAKE_CUDA_ARCHITECTURES=$CUDA_ARCHITECTURES")
    fi
    if [[ -n "${CUDA_HOST_COMPILER:-}" ]]; then
        configure+=("-DCMAKE_CUDA_HOST_COMPILER=$CUDA_HOST_COMPILER")
    fi
    cmake "${configure[@]}"
    cmake --build "$build" --target SimplePathTracerCPU SimplePathTracerCUDA -j "${BUILD_JOBS:-4}"
    cpu="$build/SimplePathTracerCPU"
    gpu="$build/SimplePathTracerCUDA"
    "$cpu" --self-test
    "$gpu" --device "$device" --device-info
    "$gpu" --device "$device" --self-test
    "$gpu" --device "$device" --validate
    if command -v compute-sanitizer >/dev/null 2>&1; then
        compute-sanitizer --tool memcheck --error-exitcode 1 "$gpu" --device "$device" --self-test
        compute-sanitizer --tool memcheck --error-exitcode 1 "$gpu" --device "$device" --validate
    else
        echo "Optional Compute Sanitizer unavailable; memory-check step skipped."
    fi
    common=(--device "$device" --width "$width" --height "$height" --samples "$samples"
            --bounces "$bounces" --trials "$trials" --seed "$seed")
    if [[ -n "${CPU_THREADS:-}" ]]; then common+=(--threads "$CPU_THREADS"); fi
    for mode in direct path; do
        "$gpu" --benchmark --mode "$mode" "${common[@]}" --output-dir "$output/$mode"
    done
    # Higher-quality GPU image, separate from the strictly matched benchmark.
    "$gpu" --render --mode path --device "$device" --seed "$seed" \
        --width "${FINAL_WIDTH:-960}" --height "${FINAL_HEIGHT:-720}" \
        --samples "${FINAL_SAMPLES:-256}" --bounces "${FINAL_BOUNCES:-5}" \
        --output "$output/submission.png"
    echo "SUCCESS: actual CUDA tests, matched CPU/GPU benchmarks, and final render completed."
    echo "Inspect $output/submission.png before submitting."
    echo "Timing evidence: $output/direct/summary.txt and $output/path/summary.txt"
    echo "No commit, push, or network upload was performed."
    printf 'SUCCESS\n' > "$output/SUCCESS.txt"
} 2>&1 | tee "$output/lab_run.log"
