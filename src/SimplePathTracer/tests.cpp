#include "Renderer.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace path_tracer {
static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void selfTests() {
    const Triangle triangle{{-1,-1,-3},{2,0,0},{1,2,0}};
    float t = 0;
    require(cuda_rays::intersect({{0,0,0},{0,0,-1}},triangle,t) && fabsf(t-3)<1e-5f,"Known distance");
    require(!cuda_rays::intersect({{0,0,0},{0,0,1}},triangle,t),"Behind camera");
    require(!cuda_rays::intersect({{0,0,0},{1,0,0}},triangle,t),"Parallel ray");
    require(!cuda_rays::intersect({{3,0,0},{0,0,-1}},triangle,t),"Missed triangle");
    Scene scene;
    scene.surfaces.push_back({triangle,{0,0,1},{0.5f,0.5f,0.5f},{0,0,0}});
    scene.surfaces.push_back({{{-2,-2,-6},{4,0,0},{2,4,0}},{0,0,1},{0,1,0},{0,0,0}});
    const Ray ray{{0,0,0},{0,0,-1}};
    require(nearest(ray,scene.view()).index == 0,"Nearest surface/material");
    require(nearest({{0,0,-4},{0,0,1}},scene.view()).normal.z < 0,"Oriented normal");
    require(!occluded(ray,2,scene.view()) && occluded(ray,4,scene.view()),"Shadow segment distance");
    Random random{42};
    double mean = 0;
    for (int i = 0; i < 10000; ++i) {
        const Vec direction = cosineDirection({0,1,0},random);
        require(direction.y >= 0 && std::abs(dot(direction,direction)-1)<1e-4f,"Hemisphere sample");
        mean += direction.y;
    }
    require(std::abs(mean/10000.0-2.0/3.0)<0.02,"Cosine sampling distribution");
    Random rng{123};
    require(hash(123) == 3336997240u && uint32_t(rng.next()*16777216.0f) == 6544420u,"RNG known sequence");
    Settings settings;
    settings.width=17; settings.height=19; settings.samples=4; settings.bounces=3;
    Scene lighting;
    lighting.light={{-0.5f,2,-0.5f},{1,0,0},{0,0,1},{0,-1,0},{4,4,4},1};
    Random visibleRandom{1};
    const Vec visible=directLighting({0,0,0},{0,1,0},{0.5f,0.5f,0.5f},lighting.view(),visibleRandom);
    require(visible.x>0 && visible.x==visible.y && visible.y==visible.z,"Unoccluded diffuse lighting");
    quad(lighting,{-2,1,-2},{2,1,-2},{2,1,2},{-2,1,2},{0.5f,0.5f,0.5f});
    Random blockedRandom{1};
    require(dot(directLighting({0,0,0},{0,1,0},{0.5f,0.5f,0.5f},lighting.view(),blockedRandom),
                Vec{1,1,1})==0,"Blocked area light");
    const Scene cornell = makeScene();
    const auto a = renderCPU(cornell,settings,1);
    const auto b = renderCPU(cornell,settings,4);
    double brightness = 0;
    for (size_t i=0; i<a.pixels.size(); ++i) {
        const Vec p=a.pixels[i],q=b.pixels[i];
        require(p.x==q.x && p.y==q.y && p.z==q.z,"Thread-independent output");
        require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
                p.x>=0 && p.y>=0 && p.z>=0,"Finite nonnegative radiance");
        brightness += p.x+p.y+p.z;
    }
    require(brightness>1,"Scene is not black");
    Settings direct=settings; direct.direct=true;
    Settings oneSurface=settings; oneSurface.bounces=1;
    const auto directImage=renderCPU(cornell,direct,1);
    const auto oneSurfaceImage=renderCPU(cornell,oneSurface,2);
    for (size_t i=0;i<directImage.pixels.size();++i) {
        const Vec d=sub(directImage.pixels[i],oneSurfaceImage.pixels[i]);
        require(dot(d,d)==0,"One-surface path equals direct baseline");
    }
    Scene empty = cornell;
    empty.surfaces.clear();
    const auto black = renderCPU(empty,settings,2);
    for (Vec p:black.pixels) require(dot(p,p)==0,"Black environment");
    const Surface emitter{{{-1,-1,-3},{2,0,0},{1,2,0}},{0,0,1},{0,0,0},{4,3,2}};
    scene.surfaces = {emitter};
    Random lightRandom{1};
    require(trace(ray,scene.view(),settings,lightRandom).x==4,"Primary light visibility");
    std::cout << "Host path-tracer self-tests PASSED (not CUDA runtime validation).\n";
}
} // namespace path_tracer
