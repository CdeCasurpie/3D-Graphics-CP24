#ifndef SHOOTING_RAYS_CUDA_MATH_H
#define SHOOTING_RAYS_CUDA_MATH_H

#include <cmath>
#include <cfloat>
#include <type_traits>

#ifdef __CUDACC__
#define RAY_HD __host__ __device__
#else
#define RAY_HD
#endif

namespace cuda_rays {

// Plain float aggregates: no GLM alignment flags or OpenGL headers in NVCC.
struct Vec { float x, y, z; };
struct Triangle { Vec v0, edge1, edge2; };
struct Ray { Vec origin, direction; };
struct View { Vec eye; float inverseVP[16]; }; // Column-major, like GLM.
static_assert(std::is_trivially_copyable<Triangle>::value, "Uploadable triangles");
static_assert(sizeof(Triangle) == 9 * sizeof(float), "Stable triangle layout");

RAY_HD inline Vec add(Vec a, Vec b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
RAY_HD inline Vec sub(Vec a, Vec b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
RAY_HD inline Vec mul(Vec a, float s) { return {a.x*s, a.y*s, a.z*s}; }
RAY_HD inline float dot(Vec a, Vec b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
RAY_HD inline Vec cross(Vec a, Vec b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
RAY_HD inline Vec normalize(Vec v) { return mul(v, 1.0f / sqrtf(dot(v,v))); }

RAY_HD inline Ray primaryRay(const View& view, int x, int y, int width, int height) {
    const float nx = 2.0f * (x + 0.5f) / width - 1.0f;
    const float ny = 1.0f - 2.0f * (y + 0.5f) / height;
    const float* m = view.inverseVP;
    // Match GLM's pairwise summation. Near-plane unprojection subtracts the
    // nearby eye position, so changing operation order magnifies roundoff.
    const float w = (m[3]*nx + m[7]*ny) + (-m[11] + m[15]);
    const Vec p = {((m[0]*nx + m[4]*ny) + (-m[8] + m[12]))/w,
                   ((m[1]*nx + m[5]*ny) + (-m[9] + m[13]))/w,
                   ((m[2]*nx + m[6]*ny) + (-m[10] + m[14]))/w};
    return {view.eye, normalize(sub(p, view.eye))};
}

RAY_HD inline bool intersect(const Ray& ray, const Triangle& triangle, float& t) {
    const Vec p = cross(ray.direction, triangle.edge2);
    const float det = dot(triangle.edge1, p);
    const float scale = sqrtf(dot(triangle.edge1, triangle.edge1)) *
                        sqrtf(dot(triangle.edge2, triangle.edge2));
    if (scale == 0 || fabsf(det) <= 1e-7f * scale) return false;
    const float inverseDet = 1.0f / det;
    const Vec offset = sub(ray.origin, triangle.v0);
    const float u = dot(offset,p) * inverseDet;
    if (u < 0 || u > 1) return false;
    const Vec q = cross(offset, triangle.edge1);
    const float v = dot(ray.direction,q) * inverseDet;
    if (v < 0 || u + v > 1) return false;
    const float distance = dot(triangle.edge2,q) * inverseDet;
    if (!(distance > 1e-5f && distance <= FLT_MAX)) return false;
    t = distance;
    return true;
}

RAY_HD inline float nearest(const Ray& ray, const Triangle* triangles, int count) {
    float result = INFINITY;
    for (int i = 0; i < count; ++i) {
        float t;
        if (intersect(ray, triangles[i], t) && t < result) result = t;
    }
    return result;
}

} // namespace cuda_rays
#undef RAY_HD
#endif
