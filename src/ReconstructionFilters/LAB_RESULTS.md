# Task 18 — Lab verification

Machine `014`, source commit `fba79c0`, GCC 13.3.0, CMake 3.28.3.
The CPU-only Release build passed `make test-ReconstructionFilters`.
Rendering and writing all five PNGs took **0.29 seconds wall time** in one
run (`/usr/bin/time`; build and tests excluded). No GPU was used.

Each filter reconstructed the same 160 × 120 Cornell-box render from shared
ray samples: 128 samples per image-plane cell, four diffuse surface evaluations,
seed 42. The lab PNGs were downloaded and all five SHA-256 hashes matched the
images already tracked in `output/`.

**Submit [`output/comparison.png`](output/comparison.png).** The single
968 × 604 PNG shows all four required filters with identical edge crops.
Box retains more visible grain; Triangle and Gaussian soften the edges;
Mitchell–Netravali keeps a sharper edge with slight ringing.

```bash
make test-ReconstructionFilters
make run-ReconstructionFilters
```
