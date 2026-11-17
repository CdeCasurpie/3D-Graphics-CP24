#ifndef SHOOTING_RAYS_RAY_CASTER_H
#define SHOOTING_RAYS_RAY_CASTER_H

#include <glm/glm.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

namespace shooting {

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction; // Unit length: t measures world-space distance.
};

struct Triangle {
    glm::vec3 v0, edge1, edge2;
    Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c)
        : v0(a), edge1(b - a), edge2(c - a) {}
};

// Two-sided Moller-Trumbore test. No back-face culling or spatial index.
inline bool intersect(const Ray& ray, const Triangle& triangle, float& t) {
    const glm::vec3 p = glm::cross(ray.direction, triangle.edge2);
    const float determinant = glm::dot(triangle.edge1, p);
    const float scale = glm::length(triangle.edge1) * glm::length(triangle.edge2);
    if (scale == 0.0f || std::abs(determinant) <= 1e-7f * scale) return false;
    const float inverseDet = 1.0f / determinant;
    const glm::vec3 offset = ray.origin - triangle.v0;
    const float u = glm::dot(offset, p) * inverseDet;
    if (u < 0.0f || u > 1.0f) return false;
    const glm::vec3 q = glm::cross(offset, triangle.edge1);
    const float v = glm::dot(ray.direction, q) * inverseDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float distance = glm::dot(triangle.edge2, q) * inverseDet;
    if (!std::isfinite(distance) || distance <= 1e-5f) return false;
    t = distance;
    return true;
}

inline float nearestHit(const Ray& ray, const std::vector<Triangle>& triangles) {
    float nearest = std::numeric_limits<float>::infinity();
    for (const Triangle& triangle : triangles) {
        float t;
        if (intersect(ray, triangle, t) && t < nearest) nearest = t;
    }
    return nearest;
}

struct View {
    glm::vec3 eye;
    glm::mat4 inverseViewProjection;
};

// CPU buffers are top-down. Pixel centers and the output aspect ratio are used.
inline Ray primaryRay(const View& view, int x, int y, int width, int height) {
    const float nx = 2.0f * (x + 0.5f) / width - 1.0f;
    const float ny = 1.0f - 2.0f * (y + 0.5f) / height;
    glm::vec4 point = view.inverseViewProjection * glm::vec4(nx, ny, -1.0f, 1.0f);
    point /= point.w;
    return {view.eye, glm::normalize(glm::vec3(point) - view.eye)};
}

struct DepthImage {
    int width = 0, height = 0;
    std::vector<float> distances;
    size_t hits = 0;
    float minimum = 0.0f, maximum = 0.0f;
    double seconds = 0.0;
};

inline DepthImage render(const std::vector<Triangle>& triangles, const View& view,
                         int width, int height, std::atomic<int>* completed = nullptr) {
    const auto start = std::chrono::steady_clock::now();
    DepthImage result;
    result.width = width;
    result.height = height;
    result.distances.resize(size_t(width) * height);
    std::atomic<int> nextRow{0};
    const unsigned workers = std::min(8u, std::max(1u, std::thread::hardware_concurrency()));
    auto traceRows = [&]() {
        for (int y = nextRow.fetch_add(1); y < height; y = nextRow.fetch_add(1)) {
            for (int x = 0; x < width; ++x) {
                result.distances[size_t(y) * width + x] =
                    nearestHit(primaryRay(view, x, y, width, height), triangles);
            }
            if (completed) completed->fetch_add(1);
        }
    };
    std::vector<std::thread> threads;
    for (unsigned i = 1; i < workers; ++i) threads.emplace_back(traceRows);
    traceRows();
    for (auto& thread : threads) thread.join();

    float minimum = std::numeric_limits<float>::infinity(), maximum = 0.0f;
    for (float distance : result.distances) {
        if (!std::isfinite(distance)) continue;
        ++result.hits;
        minimum = std::min(minimum, distance);
        maximum = std::max(maximum, distance);
    }
    if (result.hits) {
        result.minimum = minimum;
        result.maximum = maximum;
    }
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return result;
}

inline std::vector<uint8_t> grayscale(const DepthImage& image, float nearDepth,
                                     float farDepth, float floor = 0.08f) {
    std::vector<uint8_t> pixels(image.distances.size(), 0);
    const float span = farDepth - nearDepth;
    floor = std::clamp(floor, 0.0f, 1.0f);
    for (size_t i = 0; i < pixels.size(); ++i) {
        if (!std::isfinite(image.distances[i])) continue;
        const float normalized = span > 1e-6f ?
            std::clamp((image.distances[i] - nearDepth) / span, 0.0f, 1.0f) : 0.5f;
        pixels[i] = uint8_t(std::lround(255.0f * (floor + (1.0f - floor) * normalized)));
    }
    return pixels;
}

} // namespace shooting
#endif
