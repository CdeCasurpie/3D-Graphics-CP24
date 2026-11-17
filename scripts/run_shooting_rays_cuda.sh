#!/usr/bin/env bash
# Run from anywhere; all generated deliverables stay in the assignment folder.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

for tool in nvidia-smi nvcc cmake; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing $tool. Load the lab CUDA/toolchain environment; see src/ShootingRaysCUDA/README.md" >&2
        exit 1
    fi
done

device="${CUDA_DEVICE_INDEX:-0}"
width="${WIDTH:-960}"
height="${HEIGHT:-540}"
build="${CUDA_BUILD_DIR:-build-cuda}"
output="src/ShootingRaysCUDA/depth_map.png"
log="src/ShootingRaysCUDA/lab_run.log"

# pipefail preserves a failed configure/test/render even when tee succeeds.
{
    date -u '+Lab run UTC: %Y-%m-%dT%H:%M:%SZ'
    git rev-parse HEAD
    git status --short
    nvidia-smi
    nvcc --version
    cmake --version
    configure=(-S . -B "$build" -DENABLE_CUDA_RAYCASTER=ON -DCMAKE_BUILD_TYPE=Release)
    if [[ -n "${CUDA_ARCHITECTURES:-}" ]]; then
        configure+=("-DCMAKE_CUDA_ARCHITECTURES=$CUDA_ARCHITECTURES")
    fi
    cmake "${configure[@]}"
    cmake --build "$build" --target ShootingRaysCUDA CudaRayMathTests -j "${BUILD_JOBS:-4}"
    "$build/CudaRayMathTests"
    "$build/ShootingRaysCUDA" --device "$device" --device-info
    "$build/ShootingRaysCUDA" --device "$device" --self-test
    "$build/ShootingRaysCUDA" --device "$device" --validate
    if command -v compute-sanitizer >/dev/null 2>&1; then
        compute-sanitizer --tool memcheck --error-exitcode 1 \
            "$build/ShootingRaysCUDA" --device "$device" --self-test
        compute-sanitizer --tool memcheck --error-exitcode 1 \
            "$build/ShootingRaysCUDA" --device "$device" --validate
    else
        echo "compute-sanitizer not found: optional GPU memory check skipped."
    fi
    "$build/ShootingRaysCUDA" --device "$device" --render "$width" "$height" "$output"
    echo "SUCCESS: CUDA validation passed and a new GPU-rendered image was generated."
    echo "Inspect $output before submitting. No commit or push has been performed."
} 2>&1 | tee "$log"
