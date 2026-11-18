#include "Renderer.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace path_tracer {
unsigned defaultThreads() { return std::max(1u,std::thread::hardware_concurrency()); }
Result renderCPU(const Scene& scene, Settings settings, unsigned count) {
    const auto start = std::chrono::steady_clock::now();
    Result result;
    result.pixels.resize(size_t(settings.width)*settings.height);
    std::atomic<int> row{0};
    const SceneView view = scene.view();
    auto work = [&]() {
        for (int y = row.fetch_add(1); y < settings.height; y = row.fetch_add(1))
            for (int x = 0; x < settings.width; ++x)
                result.pixels[size_t(y)*settings.width+x] = pixel(view,settings,x,y);
    };
    std::vector<std::thread> workers;
    count = std::min(std::max(1u,count),unsigned(settings.height));
    try {
        for (unsigned i = 1; i < count; ++i) workers.emplace_back(work);
    } catch (...) {
        for (auto& worker : workers) worker.join();
        throw;
    }
    work();
    for (auto& worker : workers) worker.join();
    result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    return result;
}
} // namespace path_tracer
