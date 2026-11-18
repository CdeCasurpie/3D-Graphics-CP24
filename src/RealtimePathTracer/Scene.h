#ifndef REALTIME_PATH_TRACER_SCENE_H
#define REALTIME_PATH_TRACER_SCENE_H
#include "Core.h"
#include <vector>

namespace realtime {
struct Scene {
    std::vector<Surface> surfaces;
    std::vector<Material> materials;
    Light light;
    Camera camera;
    SceneView view() const { return {surfaces.data(),materials.data(),light}; }
};
inline void triangle(Scene& s,Vec a,Vec b,Vec c,int material) {
    const Vec e1=sub(b,a),e2=sub(c,a),n=normalize(cross(e1,e2));
    s.surfaces.push_back({{a,e1,e2},n,n,n,material});
}
inline void quad(Scene& s,Vec a,Vec b,Vec c,Vec d,int material) {
    triangle(s,a,b,c,material); triangle(s,a,c,d,material);
}
inline void box(Scene& s,Vec center,Vec size,float angle,int material) {
    Vec p[8];
    for (int i=0;i<8;++i) {
        const float x=((i&1)?0.5f:-0.5f)*size.x,y=((i&2)?0.5f:-0.5f)*size.y,z=((i&4)?0.5f:-0.5f)*size.z;
        p[i]=add(center,{cosf(angle)*x+sinf(angle)*z,y,-sinf(angle)*x+cosf(angle)*z});
    }
    quad(s,p[0],p[4],p[6],p[2],material); quad(s,p[1],p[3],p[7],p[5],material);
    quad(s,p[0],p[1],p[5],p[4],material); quad(s,p[2],p[6],p[7],p[3],material);
    quad(s,p[0],p[2],p[3],p[1],material); quad(s,p[4],p[5],p[7],p[6],material);
}
inline void sphere(Scene& s,Vec center,float radius,int material) {
    constexpr int sectors=48,rings=24;
    auto normal=[](int row,int column) {
        const float theta=pi*row/rings,phi=2*pi*column/sectors;
        return Vec{sinf(theta)*cosf(phi),cosf(theta),sinf(theta)*sinf(phi)};
    };
    auto face=[&](Vec a,Vec b,Vec c) {
        Vec pa=add(center,mul(a,radius)),pb=add(center,mul(b,radius)),pc=add(center,mul(c,radius));
        if (dot(cross(sub(pb,pa),sub(pc,pa)),a)<0) { std::swap(b,c); std::swap(pb,pc); }
        s.surfaces.push_back({{pa,sub(pb,pa),sub(pc,pa)},a,b,c,material});
    };
    for (int row=0;row<rings;++row) for (int column=0;column<sectors;++column) {
        const Vec a=normal(row,column),b=normal(row+1,column),c=normal(row+1,column+1),d=normal(row,column+1);
        if (row>0) face(a,b,d);
        if (row<rings-1) face(b,c,d);
    }
}
inline Scene makeScene() {
    Scene s;
    s.materials={{{0.73f,0.73f,0.73f},{0,0,0},Diffuse,1.5f},
                 {{0.75f,0.12f,0.10f},{0,0,0},Diffuse,1.5f},
                 {{0.12f,0.65f,0.16f},{0,0,0},Diffuse,1.5f},
                 {{0.96f,0.93f,0.86f},{0,0,0},Specular,1.5f},
                 {{1,1,1},{0,0,0},Dielectric,1.5f},
                 {{0,0,0},{12,12,12},Diffuse,1.5f}};
    quad(s,{-2,0,2},{2,0,2},{2,0,-2},{-2,0,-2},0);
    quad(s,{-2,4,-2},{2,4,-2},{2,4,2},{-2,4,2},0);
    quad(s,{-2,0,-2},{2,0,-2},{2,4,-2},{-2,4,-2},0);
    quad(s,{-2,0,2},{-2,0,-2},{-2,4,-2},{-2,4,2},1);
    quad(s,{2,0,-2},{2,0,2},{2,4,2},{2,4,-2},2);
    box(s,{0.05f,0.4f,-1.05f},{1.0f,0.8f,0.8f},0.25f,0);
    sphere(s,{-0.9f,0.8f,0.2f},0.8f,3);
    sphere(s,{0.9f,0.8f,0.65f},0.8f,4);
    s.light={{-0.65f,3.98f,-0.65f},{1.3f,0,0},{0,0,1.3f},{0,-1,0},{12,12,12},1.69f};
    const auto& l=s.light;
    quad(s,l.corner,add(l.corner,l.edgeU),add(add(l.corner,l.edgeU),l.edgeV),add(l.corner,l.edgeV),5);
    s.camera.eye={0,2,7.6f};
    s.camera.forward=normalize(sub(Vec{0,1.9f,0},s.camera.eye));
    s.camera.right=normalize(cross(s.camera.forward,{0,1,0}));
    s.camera.up=cross(s.camera.right,s.camera.forward);
    s.camera.tanHalfFov=tanf(39*pi/360);
    return s;
}
} // namespace realtime
#endif
