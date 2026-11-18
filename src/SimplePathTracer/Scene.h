#ifndef SIMPLE_PATH_TRACER_SCENE_H
#define SIMPLE_PATH_TRACER_SCENE_H
#include "TracerCore.h"
#include <vector>

namespace path_tracer {
struct Scene {
    std::vector<Surface> surfaces;
    Light light;
    Camera camera;
    SceneView view() const { return {surfaces.data(),int(surfaces.size()),light,camera}; }
};

inline void triangle(Scene& scene, Vec a, Vec b, Vec c, Vec color, Vec emission = {0,0,0}) {
    const Vec e1 = sub(b,a), e2 = sub(c,a);
    scene.surfaces.push_back({{a,e1,e2},normalize(cross(e1,e2)),color,emission});
}
inline void quad(Scene& scene, Vec a, Vec b, Vec c, Vec d, Vec color, Vec emission = {0,0,0}) {
    triangle(scene,a,b,c,color,emission);
    triangle(scene,a,c,d,color,emission);
}
inline void box(Scene& scene, Vec center, Vec size, float angle, Vec color) {
    Vec p[8];
    for (int i = 0; i < 8; ++i) {
        const float x = ((i&1) ? 0.5f : -0.5f)*size.x;
        const float y = ((i&2) ? 0.5f : -0.5f)*size.y;
        const float z = ((i&4) ? 0.5f : -0.5f)*size.z;
        p[i] = add(center,{cosf(angle)*x+sinf(angle)*z,y,-sinf(angle)*x+cosf(angle)*z});
    }
    quad(scene,p[0],p[4],p[6],p[2],color);
    quad(scene,p[1],p[3],p[7],p[5],color);
    quad(scene,p[0],p[1],p[5],p[4],color);
    quad(scene,p[2],p[6],p[7],p[3],color);
    quad(scene,p[0],p[2],p[3],p[1],color);
    quad(scene,p[4],p[5],p[7],p[6],color);
}
inline Scene makeScene() {
    Scene scene;
    const Vec white{0.73f,0.73f,0.73f};
    quad(scene,{-2,0,2},{2,0,2},{2,0,-2},{-2,0,-2},white);
    quad(scene,{-2,4,-2},{2,4,-2},{2,4,2},{-2,4,2},white);
    quad(scene,{-2,0,-2},{2,0,-2},{2,4,-2},{-2,4,-2},white);
    quad(scene,{-2,0,2},{-2,0,-2},{-2,4,-2},{-2,4,2},{0.75f,0.12f,0.10f});
    quad(scene,{2,0,-2},{2,0,2},{2,4,2},{2,4,-2},{0.12f,0.65f,0.16f});
    box(scene,{-0.8f,0.65f,0.5f},{1.15f,1.3f,1.15f},-0.25f,white);
    box(scene,{0.75f,1.2f,-0.65f},{1.15f,2.4f,1.15f},0.3f,white);
    scene.light = {{-0.65f,3.98f,-0.65f},{1.3f,0,0},{0,0,1.3f},{0,-1,0},{12,12,12},1.69f};
    const auto& l = scene.light;
    quad(scene,l.corner,add(l.corner,l.edgeU),add(add(l.corner,l.edgeU),l.edgeV),
         add(l.corner,l.edgeV),{0,0,0},l.emission);
    scene.camera.eye = {0,2,7.6f};
    scene.camera.forward = normalize(sub(Vec{0,1.9f,0},scene.camera.eye));
    scene.camera.right = normalize(cross(scene.camera.forward,{0,1,0}));
    scene.camera.up = cross(scene.camera.right,scene.camera.forward);
    scene.camera.tanHalfFov = tanf(39*pi/360);
    return scene;
}
} // namespace path_tracer
#endif
