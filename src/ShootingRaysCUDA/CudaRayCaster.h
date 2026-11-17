#ifndef SHOOTING_RAYS_CUDA_RAY_CASTER_H
#define SHOOTING_RAYS_CUDA_RAY_CASTER_H

#include "CudaMath.h"
#include <string>
#include <vector>

namespace cuda_rays {
struct Result {
    std::vector<float> distances;
    float kernelMilliseconds = 0;
    double uploadMilliseconds = 0, downloadMilliseconds = 0, totalMilliseconds = 0;
};

// All operations throw on CUDA failure. There is intentionally no CPU fallback.
std::string deviceInfo(int device);
Result render(const std::vector<Triangle>& triangles, const View& view,
              int width, int height, int device);
std::vector<float> traceTestRays(const std::vector<Triangle>& triangles,
                               const std::vector<Ray>& rays, int device);
} // namespace cuda_rays
#endif
