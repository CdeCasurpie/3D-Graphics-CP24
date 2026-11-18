#ifndef REALTIME_PATH_TRACER_BACKEND_H
#define REALTIME_PATH_TRACER_BACKEND_H
#include "Scene.h"
#include <memory>
#include <string>

namespace realtime {
struct Batch { std::vector<Vec> pixels; double seconds=0,deviceSeconds=0; };
class Backend {
public:
    virtual ~Backend()=default;
    virtual Batch render(Camera camera,Config config)=0;
    virtual std::vector<Hit> intersect(const std::vector<Ray>& rays)=0;
    virtual std::string info() const=0;
    double buildSeconds=0;
};
std::unique_ptr<Backend> makeEmbree(const Scene& scene,unsigned threads);
#ifdef REALTIME_OPTIX
std::unique_ptr<Backend> makeOptix(const Scene& scene,int device,const std::string& ptxPath);
#endif
void selfTests(Backend& backend,const Scene& scene);
void compare(Backend& embree,Backend& optix,const Scene& scene);
int viewer(const Scene& scene,std::unique_ptr<Backend> backend,std::string backendName,Config initialConfig,
           unsigned threads,int device,const std::string& ptx,bool smoke,bool tour,int captureFrames,int captureSeconds,
           const std::string& captureDirectory);
void savePNG(const std::string& path,const std::vector<Vec>& pixels,int width,int height,float exposure=1);
} // namespace realtime
#endif
