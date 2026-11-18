#ifndef SIMPLE_PATH_TRACER_RENDERER_H
#define SIMPLE_PATH_TRACER_RENDERER_H
#include "Scene.h"
#include <string>

namespace path_tracer {
struct Result {
    std::vector<Vec> pixels;
    double wallSeconds = 0;
    double kernelSeconds = 0;
};
Result renderCPU(const Scene& scene, Settings settings, unsigned threads);
unsigned defaultThreads();
void selfTests();
#ifdef PATH_TRACER_CUDA
Result renderCUDA(const Scene& scene, Settings settings, int device);
std::string deviceInfo(int device);
void cudaSelfTests(int device);
#endif
} // namespace path_tracer
#endif
