#ifndef SHOOTING_RAYS_CUDA_HOST_SUPPORT_H
#define SHOOTING_RAYS_CUDA_HOST_SUPPORT_H

#include "../ShootingRays/Scene.h"
#include "CudaMath.h"
#include <iostream>
#include <stdexcept>

namespace cuda_rays {
inline Vec pack(glm::vec3 v) { return {v.x,v.y,v.z}; }
inline std::vector<Triangle> pack(const std::vector<shooting::Triangle>& source) {
    std::vector<Triangle> result;
    result.reserve(source.size());
    for (const auto& t : source) result.push_back({pack(t.v0),pack(t.edge1),pack(t.edge2)});
    return result;
}
inline View pack(const shooting::View& source) {
    View result{};
    result.eye = pack(source.eye);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) result.inverseVP[c*4+r] = source.inverseViewProjection[c][r];
    return result;
}
inline void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
inline bool close(float a, float b) {
    return std::abs(a-b) <= 2e-4f + 2e-5f * std::abs(a);
}
inline void compare(const std::vector<float>& cpu, const std::vector<float>& gpu) {
    require(cpu.size() == gpu.size(), "Depth buffer sizes differ");
    size_t masks = 0, errors = 0;
    float maximum = 0;
    for (size_t i = 0; i < cpu.size(); ++i) {
        if (std::isnan(gpu[i]) || gpu[i] <= 0) { ++errors; continue; }
        const bool a = std::isfinite(cpu[i]), b = std::isfinite(gpu[i]);
        if (a != b) { ++masks; continue; }
        if (!a) continue;
        maximum = std::max(maximum, std::abs(cpu[i]-gpu[i]));
        if (!close(cpu[i],gpu[i])) ++errors;
    }
    std::cout << "Compared " << cpu.size() << " pixels: hit-mask mismatches=" << masks
              << ", distance failures=" << errors << ", max absolute error=" << maximum << '\n';
    require(masks == 0 && errors == 0, "CPU/CUDA validation failed; do not submit the image");
}

// The same cases run either as host math checks or through an actual CUDA kernel.
template<class Trace> void intersectionTests(Trace trace) {
    const Triangle front{{-1,-1,-3},{2,0,0},{1,2,0}};
    const Triangle far{{-2,-2,-6},{4,0,0},{2,4,0}};
    const std::vector<Ray> rays = {
        {{0,0,0},{0,0,-1}}, {{3,0,0},{0,0,-1}},
        {{0,0,0},{0,0,1}}, {{0,0,0},{1,0,0}}, {{0,0,-4},{0,0,1}}
    };
    const auto hits = trace(std::vector<Triangle>{front}, rays);
    require(hits.size() == rays.size(), "Test output size");
    require(close(hits[0],3) && close(hits[4],1), "Known distance or back-face test");
    for (int i = 1; i <= 3; ++i) require(std::isinf(hits[i]) && hits[i] > 0, "Expected a missed ray");
    require(close(trace(std::vector<Triangle>{far,front}, std::vector<Ray>{rays[0]})[0],3), "Nearest hit test");
    const Triangle degenerate{{0,0,-1},{0,0,0},{0,0,0}};
    require(std::isinf(trace(std::vector<Triangle>{degenerate}, std::vector<Ray>{rays[0]})[0]), "Degenerate test");
    require(std::isinf(trace(std::vector<Triangle>{}, std::vector<Ray>{rays[0]})[0]), "Empty scene test");
}

inline void cameraTests() {
    Camera camera = shooting::defaultCamera();
    const auto reference = shooting::snapshot(camera,317,179);
    const auto view = pack(reference);
    for (int y = 0; y < 179; y += 7) {
        for (int x = 0; x < 317; x += 7) {
            const auto a = shooting::primaryRay(reference,x,y,317,179);
            const auto b = primaryRay(view,x,y,317,179);
            require(glm::length(a.direction - glm::vec3(b.direction.x,b.direction.y,b.direction.z)) < 2e-5f,
                    "Portable ray math differs from CPU camera");
            require(std::abs(dot(b.direction,b.direction)-1) < 1e-5f, "Direction not normalized");
        }
    }
}

inline shooting::DepthImage depthImage(std::vector<float> distances, int width, int height) {
    shooting::DepthImage image;
    image.width = width;
    image.height = height;
    image.distances = std::move(distances);
    require(image.distances.size() == size_t(width)*height, "Invalid depth buffer size");
    float minimum = INFINITY, maximum = 0;
    for (float d : image.distances) {
        require(!std::isnan(d) && d > 0, "Invalid GPU depth");
        if (!std::isfinite(d)) continue;
        ++image.hits;
        minimum = std::min(minimum,d);
        maximum = std::max(maximum,d);
    }
    if (image.hits) { image.minimum = minimum; image.maximum = maximum; }
    return image;
}
} // namespace cuda_rays
#endif
