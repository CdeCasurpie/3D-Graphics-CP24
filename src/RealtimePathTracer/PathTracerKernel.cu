#include <optix.h>
#include <optix_device.h>
#include "LaunchParams.h"

extern "C" { __constant__ realtime::LaunchParams params; }
namespace realtime {
struct OptixQuery {
    __device__ Hit nearest(Ray ray) const {
        if (!params.traversable) return {};
        unsigned primitive=0xffffffffu,distance=__float_as_uint(FLT_MAX),u=0,v=0;
        optixTrace(OptixTraversableHandle(params.traversable),
            make_float3(ray.origin.x,ray.origin.y,ray.origin.z),
            make_float3(ray.direction.x,ray.direction.y,ray.direction.z),
            1e-5f,FLT_MAX,0,OptixVisibilityMask(255),OPTIX_RAY_FLAG_DISABLE_ANYHIT,
            0,2,0,primitive,distance,u,v);
        return {primitive==0xffffffffu?-1:int(primitive),__uint_as_float(distance),__uint_as_float(u),__uint_as_float(v)};
    }
    __device__ bool occluded(Ray ray,float far) const {
        if (!params.traversable) return false;
        unsigned blocked=1;
        optixTrace(OptixTraversableHandle(params.traversable),
            make_float3(ray.origin.x,ray.origin.y,ray.origin.z),
            make_float3(ray.direction.x,ray.direction.y,ray.direction.z),
            1e-5f,far,0,OptixVisibilityMask(255),
            OPTIX_RAY_FLAG_DISABLE_ANYHIT|OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT|OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,
            1,2,1,blocked);
        return blocked!=0;
    }
};
}
extern "C" __global__ void __raygen__main() {
    const uint3 index=optixGetLaunchIndex();
    const realtime::OptixQuery query;
    if (params.testCount>0) {
        if (index.x<unsigned(params.testCount)) params.testHits[index.x]=query.nearest(params.testRays[index.x]);
        return;
    }
    if (index.x<unsigned(params.config.width) && index.y<unsigned(params.config.height))
        params.output[size_t(index.y)*params.config.width+index.x]=
            realtime::pixel(params.scene,params.camera,params.config,int(index.x),int(index.y),query);
}
extern "C" __global__ void __closesthit__radiance() {
    optixSetPayload_0(optixGetPrimitiveIndex());
    optixSetPayload_1(__float_as_uint(optixGetRayTmax()));
    const float2 uv=optixGetTriangleBarycentrics();
    optixSetPayload_2(__float_as_uint(uv.x)); optixSetPayload_3(__float_as_uint(uv.y));
}
extern "C" __global__ void __closesthit__shadow() { optixSetPayload_0(1); }
extern "C" __global__ void __miss__radiance() { optixSetPayload_0(0xffffffffu); }
extern "C" __global__ void __miss__shadow() { optixSetPayload_0(0); }
