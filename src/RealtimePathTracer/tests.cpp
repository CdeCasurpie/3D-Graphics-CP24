#include "Backend.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace realtime {
namespace {
void require(bool test,const char* message) { if (!test) throw std::runtime_error(message); }
// Independent exhaustive reference, used ONLY in tests; never production frames.
Hit reference(const Scene& scene,Ray ray) {
    Hit hit;
    for (size_t i=0;i<scene.surfaces.size();++i) {
        float t;
        if (cuda_rays::intersect(ray,scene.surfaces[i].triangle,t) && t<hit.distance) {
            hit.index=int(i); hit.distance=t;
        }
    }
    return hit;
}
void finite(const Batch& batch) {
    for (Vec p:batch.pixels) require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
                                   p.x>=0 && p.y>=0 && p.z>=0,"Invalid output radiance");
}
}
void selfTests(Backend& backend,const Scene& scene) {
    require(std::abs(fresnel(1,1,1.5f)-0.04f)<1e-5f,"Normal-incidence glass Fresnel");
    require(fresnel(0.5f,1.5f,1)==1,"Total internal reflection");
    const Vec reflected=reflect({0,-1,0},{0,1,0}),transmitted=refract({0,-1,0},{0,1,0},1/1.5f);
    require(reflected.y==1 && transmitted.y==-1,"Reflection/refraction direction");
    Random random{123};
    Config config; config.width=47; config.height=35;
    std::vector<Ray> rays;
    for (int y=0;y<config.height;++y) for (int x=0;x<config.width;++x)
        rays.push_back(cameraRay(scene.camera,config,x,y,random));
    // Include rays from inside glass and outside/behind the open room.
    rays.push_back({{0.9f,0.8f,0.65f},{1,0,0}});
    rays.push_back({{10,10,10},{1,0,0}});
    const auto hits=backend.intersect(rays);
    require(hits.size()==rays.size(),"Test hit buffer size");
    for (size_t i=0;i<rays.size();++i) {
        const Hit ref=reference(scene,rays[i]),actual=hits[i];
        require((ref.index<0)==(actual.index<0),"Accelerator/reference hit-mask mismatch");
        if (ref.index>=0) {
            require(std::abs(ref.distance-actual.distance)<0.0005f+0.00005f*ref.distance,"Accelerator/reference distance");
            require(actual.index>=0 && actual.index<int(scene.surfaces.size()),"Invalid primitive ID");
            require(scene.surfaces[ref.index].material==scene.surfaces[actual.index].material,"Material ID mismatch");
            require(actual.u>=-1e-5f && actual.v>=-1e-5f && actual.u+actual.v<=1.00001f,"Hit barycentrics");
        }
    }
    config.width=17; config.height=19; config.spp=4; config.bounces=6;
    const auto image=backend.render(scene.camera,config); finite(image);
    double light=0; for (Vec p:image.pixels) light+=p.x+p.y+p.z;
    require(light>0,"Unexpected black scene");
    const auto repeat=backend.render(scene.camera,config);
    for (size_t i=0;i<image.pixels.size();++i)
        require(dot(sub(image.pixels[i],repeat.pixels[i]),sub(image.pixels[i],repeat.pixels[i]))==0,"Repeatability");
    Config combined=config; combined.spp=2;
    Config first=combined; first.spp=1;
    Config second=first; second.sampleOffset=1;
    const auto two=backend.render(scene.camera,combined),a=backend.render(scene.camera,first),b=backend.render(scene.camera,second);
    for (size_t i=0;i<two.pixels.size();++i) {
        const Vec expected=mul(add(a.pixels[i],b.pixels[i]),0.5f),diff=sub(two.pixels[i],expected);
        require(dot(diff,diff)<1e-10f,"Progressive sample indexing");
    }
    std::cout << "PASS: " << backend.info() << "; reference intersections, Fresnel/TIR, finite RGB, accumulation.\n";
}
void compare(Backend& embree,Backend& optix,const Scene& scene) {
    Config config; config.width=63; config.height=47; config.spp=64; config.bounces=8;
    const auto a=embree.render(scene.camera,config),b=optix.render(scene.camera,config);
    finite(a); finite(b);
    double errors=0,signal=0,maximum=0;
    for (size_t i=0;i<a.pixels.size();++i) {
        const Vec d=sub(a.pixels[i],b.pixels[i]);
        errors+=dot(d,d); signal+=dot(a.pixels[i],a.pixels[i]);
        maximum=std::max(maximum,double(std::max({std::abs(d.x),std::abs(d.y),std::abs(d.z)})));
    }
    const double rmse=std::sqrt(errors/(3*a.pixels.size()));
    const double rms=std::sqrt(signal/(3*a.pixels.size()));
    std::cout << "Linear RGB comparison: RMSE=" << rmse << " reference RMS=" << rms << " max abs=" << maximum << '\n';
    // Different BVH edge decisions can diverge a glass/roulette path. Reject
    // gross image disagreement, not occasional Monte Carlo outlier components.
    require(rmse<=0.02+0.08*rms,"Embree/OptiX RGB validation failed; preserve logs and investigate");
}
} // namespace realtime
