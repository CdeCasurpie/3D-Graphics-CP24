#include "Backend.h"
#include <embree4/rtcore.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace realtime {
namespace {
RTCRay nativeRay(Ray ray,float far=FLT_MAX) {
    RTCRay r{};
    r.org_x=ray.origin.x; r.org_y=ray.origin.y; r.org_z=ray.origin.z;
    r.dir_x=ray.direction.x; r.dir_y=ray.direction.y; r.dir_z=ray.direction.z;
    r.tnear=1e-5f; r.tfar=far; r.mask=0xffffffffu;
    return r;
}
struct Query {
    RTCScene scene;
    Hit nearest(Ray ray) const {
        RTCRayHit rh{}; rh.ray=nativeRay(ray);
        rh.hit.geomID=RTC_INVALID_GEOMETRY_ID;
        rh.hit.primID=RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments arguments; rtcInitIntersectArguments(&arguments);
        rtcIntersect1(scene,&rh,&arguments);
        Hit hit;
        if (rh.hit.geomID!=RTC_INVALID_GEOMETRY_ID)
            hit={int(rh.hit.primID),rh.ray.tfar,rh.hit.u,rh.hit.v};
        return hit;
    }
    bool occluded(Ray ray,float far) const {
        RTCRay r=nativeRay(ray,far);
        RTCOccludedArguments arguments; rtcInitOccludedArguments(&arguments);
        rtcOccluded1(scene,&r,&arguments);
        return r.tfar<0;
    }
};
class Embree final:public Backend {
    RTCDevice device=nullptr;
    RTCScene handle=nullptr;
    Scene scene;
    unsigned threads;
    void check(const char* operation) {
        if (rtcGetDeviceError(device)!=RTC_ERROR_NONE) throw std::runtime_error(std::string("Embree: ")+operation);
    }
public:
    Embree(const Scene& source,unsigned workers):scene(source),threads(workers) {
        const auto start=std::chrono::steady_clock::now();
        device=rtcNewDevice(nullptr);
        if (!device) throw std::runtime_error("Cannot create Embree 4 device");
        RTCGeometry geometry=nullptr;
        try {
            handle=rtcNewScene(device); check("create scene");
            rtcSetSceneFlags(handle,RTC_SCENE_FLAG_ROBUST);
            if (!scene.surfaces.empty()) {
                geometry=rtcNewGeometry(device,RTC_GEOMETRY_TYPE_TRIANGLE); check("create geometry");
                auto* vertices=static_cast<Vec*>(rtcSetNewGeometryBuffer(geometry,RTC_BUFFER_TYPE_VERTEX,0,
                    RTC_FORMAT_FLOAT3,sizeof(Vec),scene.surfaces.size()*3));
                auto* indices=static_cast<unsigned*>(rtcSetNewGeometryBuffer(geometry,RTC_BUFFER_TYPE_INDEX,0,
                    RTC_FORMAT_UINT3,3*sizeof(unsigned),scene.surfaces.size()));
                check("allocate geometry buffers");
                for (size_t i=0;i<scene.surfaces.size();++i) {
                    const auto& t=scene.surfaces[i].triangle;
                    vertices[3*i]=t.v0; vertices[3*i+1]=add(t.v0,t.edge1); vertices[3*i+2]=add(t.v0,t.edge2);
                    for (unsigned j=0;j<3;++j) indices[3*i+j]=unsigned(3*i+j);
                }
                rtcCommitGeometry(geometry); check("commit geometry");
                rtcAttachGeometry(handle,geometry); check("attach geometry");
                rtcReleaseGeometry(geometry); geometry=nullptr;
            }
            rtcCommitScene(handle); check("build BVH");
        } catch (...) {
            if (geometry) rtcReleaseGeometry(geometry);
            if (handle) rtcReleaseScene(handle);
            rtcReleaseDevice(device); throw;
        }
        buildSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    }
    ~Embree() override { rtcReleaseScene(handle); rtcReleaseDevice(device); }
    std::string info() const override {
        return "Intel Embree " + std::string(RTC_VERSION_STRING)+" | CPU workers="+std::to_string(threads);
    }
    std::vector<Hit> intersect(const std::vector<Ray>& rays) override {
        std::vector<Hit> hits; hits.reserve(rays.size());
        const Query query{handle};
        for (const auto& ray:rays) hits.push_back(query.nearest(ray));
        check("test intersections"); return hits;
    }
    Batch render(Camera camera,Config config) override {
        const auto start=std::chrono::steady_clock::now();
        Batch batch; batch.pixels.resize(size_t(config.width)*config.height);
        std::atomic<int> next{0};
        const Query query{handle}; const auto view=scene.view();
        auto work=[&]() {
            for (int y=next.fetch_add(1);y<config.height;y=next.fetch_add(1))
                for (int x=0;x<config.width;++x)
                    batch.pixels[size_t(y)*config.width+x]=pixel(view,camera,config,x,y,query);
        };
        std::vector<std::thread> workers;
        try {
            for (unsigned i=1;i<std::min(threads,unsigned(config.height));++i) workers.emplace_back(work);
        } catch (...) { for (auto& worker:workers) worker.join(); throw; }
        work(); for (auto& worker:workers) worker.join();
        check("render intersections");
        batch.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        return batch;
    }
};
} // namespace
std::unique_ptr<Backend> makeEmbree(const Scene& scene,unsigned threads) {
    return std::make_unique<Embree>(scene,std::max(1u,threads));
}
} // namespace realtime
