#ifndef REALTIME_PATH_TRACER_LAUNCH_PARAMS_H
#define REALTIME_PATH_TRACER_LAUNCH_PARAMS_H
#include "Core.h"
namespace realtime {
struct LaunchParams {
    unsigned long long traversable=0;
    SceneView scene{};
    Camera camera{};
    Config config{};
    Vec* output=nullptr;
    const Ray* testRays=nullptr;
    Hit* testHits=nullptr;
    int testCount=0;
};
}
#endif
