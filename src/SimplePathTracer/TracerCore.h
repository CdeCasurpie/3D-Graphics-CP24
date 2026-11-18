#ifndef SIMPLE_PATH_TRACER_CORE_H
#define SIMPLE_PATH_TRACER_CORE_H

#include "../ShootingRaysCUDA/CudaMath.h"
#include <cstdint>

#ifdef __CUDACC__
#define PT_HD __host__ __device__
#else
#define PT_HD
#endif

namespace path_tracer {
using cuda_rays::Vec;
using cuda_rays::Ray;
using cuda_rays::Triangle;
using cuda_rays::add;
using cuda_rays::sub;
using cuda_rays::mul;
using cuda_rays::dot;
using cuda_rays::cross;
using cuda_rays::normalize;
constexpr float pi = 3.14159265358979323846f;
constexpr float epsilon = 0.0002f; // Scene is approximately four world units wide.

PT_HD inline Vec product(Vec a, Vec b) { return {a.x*b.x, a.y*b.y, a.z*b.z}; }

struct Surface {
    Triangle triangle;
    Vec normal;
    Vec albedo;
    Vec emission;
};
struct Light {
    Vec corner, edgeU, edgeV, normal, emission;
    float area;
};
struct Camera {
    Vec eye, forward, right, up;
    float tanHalfFov;
};
struct Settings {
    int width = 640, height = 360, samples = 32, bounces = 4;
    uint32_t seed = 42;
    bool direct = false;
};
struct SceneView {
    const Surface* surfaces;
    int count;
    Light light;
    Camera camera;
};
static_assert(std::is_trivially_copyable<Surface>::value, "Surface must support CUDA upload");
static_assert(std::is_trivially_copyable<SceneView>::value, "SceneView must support kernel arguments");
static_assert(std::is_trivially_copyable<Settings>::value, "Settings must support kernel arguments");
struct Hit {
    int index = -1;
    float distance = FLT_MAX;
    Vec normal{};
};

// Unsigned integer arithmetic is identical on host and device. RNG state is
// per pixel/sample, never per worker, so scheduling cannot change the samples.
PT_HD inline uint32_t hash(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
struct Random {
    uint32_t state;
    PT_HD float next() {
        state += 0x9e3779b9u;
        return float(hash(state) >> 8) * (1.0f / 16777216.0f); // [0,1)
    }
};

PT_HD inline Hit nearest(const Ray& ray, SceneView scene) {
    Hit hit;
    for (int i = 0; i < scene.count; ++i) {
        float t;
        if (cuda_rays::intersect(ray, scene.surfaces[i].triangle, t) && t < hit.distance) {
            hit.distance = t;
            hit.index = i;
        }
    }
    if (hit.index >= 0) {
        hit.normal = scene.surfaces[hit.index].normal;
        if (dot(hit.normal, ray.direction) > 0) hit.normal = mul(hit.normal, -1);
    }
    return hit;
}

PT_HD inline bool occluded(const Ray& ray, float limit, SceneView scene) {
    for (int i = 0; i < scene.count; ++i) {
        float t;
        if (cuda_rays::intersect(ray, scene.surfaces[i].triangle, t) && t < limit) return true;
    }
    return false;
}

PT_HD inline Vec cosineDirection(Vec normal, Random& random) {
    const float r = sqrtf(random.next());
    const float angle = 2*pi*random.next();
    const Vec reference = fabsf(normal.x) > 0.9f ? Vec{0,1,0} : Vec{1,0,0};
    const Vec tangent = normalize(cross(reference, normal));
    const Vec bitangent = cross(normal, tangent);
    return normalize(add(add(mul(tangent, r*cosf(angle)), mul(bitangent, r*sinf(angle))),
                         mul(normal, sqrtf(fmaxf(0, 1-r*r)))));
}

PT_HD inline Ray primaryRay(Camera camera, Settings settings, int x, int y, Random& random) {
    const float sx = (2*(x + random.next()) / settings.width - 1) *
                     (float(settings.width) / settings.height) * camera.tanHalfFov;
    const float sy = (1 - 2*(y + random.next()) / settings.height) * camera.tanHalfFov;
    return {camera.eye, normalize(add(camera.forward, add(mul(camera.right,sx), mul(camera.up,sy))))};
}

PT_HD inline Vec directLighting(Vec position, Vec normal, Vec albedo,
                                 SceneView scene, Random& random) {
    const Light& light = scene.light;
    // Explicit sequencing: C++ does not specify function-argument evaluation
    // order, which could otherwise exchange the CPU/CUDA random samples.
    const float lightU = random.next();
    const float lightV = random.next();
    const Vec lightPoint = add(light.corner, add(mul(light.edgeU,lightU), mul(light.edgeV,lightV)));
    // Use the offset origin consistently for the shadow direction and distance.
    const Vec origin = add(position, mul(normal,epsilon));
    const Vec difference = sub(lightPoint,origin);
    const float distanceSquared = dot(difference,difference);
    if (distanceSquared <= epsilon*epsilon) return {0,0,0};
    const float distance = sqrtf(distanceSquared);
    const Vec direction = mul(difference,1/distance);
    const float surfaceCos = fmaxf(0,dot(normal,direction));
    const float lightCos = fmaxf(0,-dot(light.normal,direction));
    if (surfaceCos <= 0 || lightCos <= 0 || occluded({origin,direction},distance-epsilon,scene))
        return {0,0,0};
    // Lambert BRDF albedo/pi; uniform area PDF=1/area; geometry term cos*cos/r².
    return mul(product(albedo,light.emission), surfaceCos*lightCos*light.area / (pi*distanceSquared));
}

PT_HD inline Vec trace(Ray ray, SceneView scene, Settings settings, Random& random) {
    Vec radiance{0,0,0}, throughput{1,1,1};
    const int limit = settings.direct ? 1 : settings.bounces;
    for (int bounce = 0; bounce < limit; ++bounce) {
        const Hit hit = nearest(ray,scene);
        if (hit.index < 0) break; // Black environment.
        const Surface& surface = scene.surfaces[hit.index];
        if (dot(surface.emission,surface.emission) > 0) {
            // Explicit light sampling accounts for every diffuse->light path.
            // Only primary visibility adds emission, avoiding double counting.
            if (bounce == 0 && dot(surface.normal,ray.direction) < 0)
                radiance = add(radiance,product(throughput,surface.emission));
            break;
        }
        const Vec position = add(ray.origin,mul(ray.direction,hit.distance));
        radiance = add(radiance,product(throughput,directLighting(position,hit.normal,surface.albedo,scene,random)));
        if (bounce+1 == limit) break;
        // cosine-weighted PDF cancels the Lambertian cosine/pi factor.
        throughput = product(throughput,surface.albedo);
        ray = {add(position,mul(hit.normal,epsilon)),cosineDirection(hit.normal,random)};
    }
    return radiance;
}

PT_HD inline Vec pixel(SceneView scene, Settings settings, int x, int y) {
    Vec sum{0,0,0};
    const uint32_t index = uint32_t(y)*uint32_t(settings.width)+uint32_t(x);
    for (int sample = 0; sample < settings.samples; ++sample) {
        Random random{hash(settings.seed ^ hash(index) ^ hash(uint32_t(sample)+0xa511e9b3u))};
        const Ray ray = primaryRay(scene.camera,settings,x,y,random);
        sum = add(sum,trace(ray,scene,settings,random));
    }
    return mul(sum,1.0f/settings.samples);
}
} // namespace path_tracer
#undef PT_HD
#endif
