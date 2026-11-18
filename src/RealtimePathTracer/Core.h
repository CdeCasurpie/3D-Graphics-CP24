#ifndef REALTIME_PATH_TRACER_CORE_H
#define REALTIME_PATH_TRACER_CORE_H

#include "../SimplePathTracer/TracerCore.h"

#ifdef __CUDACC__
#define RT_HD __host__ __device__
// OptiX queries are device-only. Host C++ instantiates the same integrator
// body with EmbreeQuery; NVCC must not emit a host OptixQuery instantiation.
#define RT_INTEGRATOR __device__
#else
#define RT_HD
#define RT_INTEGRATOR
#endif

namespace realtime {
using cuda_rays::Vec;
using cuda_rays::Ray;
using cuda_rays::Triangle;
using cuda_rays::add;
using cuda_rays::sub;
using cuda_rays::mul;
using cuda_rays::dot;
using cuda_rays::cross;
using cuda_rays::normalize;
using path_tracer::product;
using path_tracer::Random;
using path_tracer::hash;
using path_tracer::cosineDirection;
using path_tracer::Camera;
using path_tracer::Light;
constexpr float pi = path_tracer::pi;
constexpr float epsilon = 0.0003f;
enum MaterialType { Diffuse, Specular, Dielectric };
struct Material {
    Vec color{0.73f,0.73f,0.73f};
    Vec emission{0,0,0};
    int type = Diffuse;
    float ior = 1.5f;
};
struct Surface {
    Triangle triangle;
    Vec n0,n1,n2; // Interpolated normals, outward on closed geometry.
    int material;
};
struct Hit {
    int index = -1;
    float distance = FLT_MAX, u = 0, v = 0;
};
struct Config {
    int width=480, height=360, spp=1, bounces=8;
    uint32_t sampleOffset=0, seed=42;
};
struct SceneView {
    const Surface* surfaces;
    const Material* materials;
    Light light;
};
static_assert(std::is_trivially_copyable<Surface>::value,"Uploadable surfaces");
static_assert(std::is_trivially_copyable<Material>::value,"Uploadable materials");

RT_HD inline Vec reflect(Vec direction, Vec normal) {
    return sub(direction,mul(normal,2*dot(direction,normal)));
}
// Exact unpolarized dielectric Fresnel, including total internal reflection.
RT_HD inline float fresnel(float cosI, float etaI, float etaT) {
    cosI=fminf(1,fmaxf(0,cosI));
    const float sinT2=(etaI*etaI/(etaT*etaT))*(1-cosI*cosI);
    if (sinT2>=1) return 1;
    const float cosT=sqrtf(fmaxf(0,1-sinT2));
    const float rs=(etaI*cosI-etaT*cosT)/(etaI*cosI+etaT*cosT);
    const float rp=(etaT*cosI-etaI*cosT)/(etaT*cosI+etaI*cosT);
    return (rs*rs+rp*rp)*0.5f;
}
RT_HD inline Vec refract(Vec direction, Vec normal, float eta) {
    const float cosine=fminf(1,-dot(direction,normal));
    const Vec perpendicular=mul(add(direction,mul(normal,cosine)),eta);
    return normalize(sub(perpendicular,mul(normal,sqrtf(fmaxf(0,1-dot(perpendicular,perpendicular))))));
}
RT_HD inline Ray cameraRay(Camera camera, Config config, int x, int y, Random& random) {
    path_tracer::Settings settings;
    settings.width=config.width; settings.height=config.height;
    return path_tracer::primaryRay(camera,settings,x,y,random);
}

template<class Query>
RT_INTEGRATOR Vec trace(Ray ray, SceneView scene, Config config, Random& random, const Query& query) {
    Vec radiance{0,0,0},throughput{1,1,1};
    bool previousDelta=true;
    for (int bounce=0;bounce<config.bounces;++bounce) {
        const Hit hit=query.nearest(ray);
        if (hit.index<0) break;
        const Surface& surface=scene.surfaces[hit.index];
        const Material& material=scene.materials[surface.material];
        Vec geometric=normalize(cross(surface.triangle.edge1,surface.triangle.edge2));
        const bool entering=dot(ray.direction,geometric)<0;
        Vec normal=normalize(add(mul(surface.n0,1-hit.u-hit.v),
                                add(mul(surface.n1,hit.u),mul(surface.n2,hit.v))));
        if (dot(normal,geometric)<0) normal=mul(normal,-1);
        if (!entering) { normal=mul(normal,-1); geometric=mul(geometric,-1); }
        if (dot(normal,ray.direction)>=0) normal=geometric;
        if (dot(material.emission,material.emission)>0) {
            // NEE estimates diffuse->light paths. Delta paths cannot sample the
            // light with NEE, so retain their emitter hits (and camera hits).
            if (previousDelta && entering) radiance=add(radiance,product(throughput,material.emission));
            break;
        }
        const Vec point=add(ray.origin,mul(ray.direction,hit.distance));
        Vec nextDirection;
        if (material.type==Diffuse) {
            const Light& light=scene.light;
            const float u=random.next(),v=random.next(); // Explicit sequencing.
            const Vec target=add(light.corner,add(mul(light.edgeU,u),mul(light.edgeV,v)));
            const Vec origin=add(point,mul(geometric,epsilon));
            const Vec delta=sub(target,origin);
            const float distance=sqrtf(dot(delta,delta));
            const Vec direction=mul(delta,1/distance);
            const float cosSurface=fmaxf(0,dot(normal,direction));
            const float cosLight=fmaxf(0,-dot(light.normal,direction));
            if (dot(geometric,direction)>0 && cosSurface>0 && cosLight>0 &&
                !query.occluded({origin,direction},distance-epsilon)) {
                const Vec lighting=mul(product(material.color,light.emission),
                    cosSurface*cosLight*light.area/(pi*distance*distance));
                radiance=add(radiance,product(throughput,lighting));
            }
            nextDirection=cosineDirection(normal,random);
            if (dot(nextDirection,geometric)<=0) break;
            throughput=product(throughput,material.color);
            previousDelta=false;
        } else if (material.type==Specular) {
            nextDirection=normalize(reflect(ray.direction,normal));
            if (dot(nextDirection,geometric)<=0) nextDirection=normalize(reflect(ray.direction,geometric));
            throughput=product(throughput,material.color);
            previousDelta=true;
        } else {
            const float etaI=entering?1.0f:material.ior,etaT=entering?material.ior:1.0f;
            const float probability=fresnel(-dot(ray.direction,normal),etaI,etaT);
            if (random.next()<probability) {
                nextDirection=normalize(reflect(ray.direction,normal));
            } else {
                nextDirection=refract(ray.direction,normal,etaI/etaT);
                // Radiance transport across a refracting boundary. Entry and
                // exit factors cancel when traversing a closed glass object.
                throughput=mul(throughput,(etaI*etaI)/(etaT*etaT));
            }
            throughput=product(throughput,material.color);
            previousDelta=true;
        }
        if (bounce>=3) {
            const float survive=fminf(0.95f,fmaxf(0.05f,fmaxf(throughput.x,fmaxf(throughput.y,throughput.z))));
            if (random.next()>=survive) break;
            throughput=mul(throughput,1/survive);
        }
        const float side=dot(nextDirection,geometric)>=0?1.0f:-1.0f;
        ray={add(point,mul(geometric,side*epsilon)),nextDirection};
    }
    return radiance;
}
template<class Query>
RT_INTEGRATOR Vec pixel(SceneView scene, Camera camera, Config config, int x, int y, const Query& query) {
    Vec sum{0,0,0};
    const uint32_t index=uint32_t(y)*uint32_t(config.width)+uint32_t(x);
    for (int i=0;i<config.spp;++i) {
        Random random{hash(config.seed^hash(index)^hash(config.sampleOffset+uint32_t(i)+0xa511e9b3u))};
        const Ray ray=cameraRay(camera,config,x,y,random);
        sum=add(sum,trace(ray,scene,config,random,query));
    }
    return mul(sum,1.0f/config.spp);
}
} // namespace realtime
#undef RT_HD
#undef RT_INTEGRATOR
#endif
