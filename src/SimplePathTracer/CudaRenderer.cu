#include "Renderer.h"
#include <cuda_runtime.h>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace path_tracer {
namespace {
void check(cudaError_t status, const char* operation) {
    if (status != cudaSuccess)
        throw std::runtime_error(std::string(operation)+": "+cudaGetErrorString(status));
}
template<class T> struct Buffer {
    T* data = nullptr;
    explicit Buffer(size_t count) {
        if (count) check(cudaMalloc(reinterpret_cast<void**>(&data),count*sizeof(T)),"cudaMalloc");
    }
    ~Buffer() { if (data) cudaFree(data); }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};
struct Event {
    cudaEvent_t value{};
    Event() { check(cudaEventCreate(&value),"cudaEventCreate"); }
    ~Event() { cudaEventDestroy(value); }
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};
void selectDevice(int device) {
    int count = 0;
    check(cudaGetDeviceCount(&count),"cudaGetDeviceCount (check driver/GPU)");
    if (device < 0 || device >= count) throw std::runtime_error("No CUDA GPU at selected index; no CPU fallback");
    check(cudaSetDevice(device),"cudaSetDevice");
}
__global__ void renderKernel(SceneView scene, Settings settings, Vec* output) {
    const int x = blockIdx.x*blockDim.x+threadIdx.x;
    const int y = blockIdx.y*blockDim.y+threadIdx.y;
    if (x < settings.width && y < settings.height)
        output[size_t(y)*settings.width+x] = pixel(scene,settings,x,y);
}
__global__ void mathTestKernel(int* result) {
    const Triangle triangle{{-1,-1,-3},{2,0,0},{1,2,0}};
    float t = 0;
    result[0] = cuda_rays::intersect({{0,0,0},{0,0,-1}},triangle,t) && fabsf(t-3)<1e-5f;
    result[1] = !cuda_rays::intersect({{0,0,0},{0,0,1}},triangle,t);
    Random random{42};
    result[2] = 1;
    for (int i = 0; i < 1000; ++i) {
        const Vec d = cosineDirection({0,1,0},random);
        if (d.y < 0 || fabsf(dot(d,d)-1)>1e-4f) result[2] = 0;
    }
    const Surface surface{triangle,{0,0,1},{0.7f,0.7f,0.7f},{0,0,0}};
    const SceneView view{&surface,1,{}, {}};
    result[3] = !occluded({{0,0,0},{0,0,-1}},2,view) && occluded({{0,0,0},{0,0,-1}},4,view);
    Random rng{123};
    result[4] = hash(123) == 3336997240u && uint32_t(rng.next()*16777216.0f) == 6544420u;
}
} // namespace

std::string deviceInfo(int device) {
    selectDevice(device);
    cudaDeviceProp properties{};
    check(cudaGetDeviceProperties(&properties,device),"cudaGetDeviceProperties");
    int runtime=0,driver=0;
    check(cudaRuntimeGetVersion(&runtime),"cudaRuntimeGetVersion");
    check(cudaDriverGetVersion(&driver),"cudaDriverGetVersion");
    std::ostringstream text;
    text << "CUDA device " << device << ": " << properties.name << " | compute "
         << properties.major << '.' << properties.minor << " | memory MiB "
         << properties.totalGlobalMem/(1024*1024) << " | runtime " << runtime << " | driver " << driver;
    return text.str();
}

Result renderCUDA(const Scene& scene, Settings settings, int device) {
    selectDevice(device); // Context/device initialization is outside warmed render timing.
    const auto start = std::chrono::steady_clock::now();
    Result result;
    result.pixels.resize(size_t(settings.width)*settings.height);
    {
        Buffer<Surface> surfaces(scene.surfaces.size());
        Buffer<Vec> image(result.pixels.size());
        if (!scene.surfaces.empty())
            check(cudaMemcpy(surfaces.data,scene.surfaces.data(),scene.surfaces.size()*sizeof(Surface),
                             cudaMemcpyHostToDevice),"Upload scene");
        SceneView view = scene.view();
        view.surfaces = surfaces.data;
        Event begin,end;
        check(cudaEventRecord(begin.value),"Record kernel start");
        const dim3 block(16,16),grid((settings.width+15)/16,(settings.height+15)/16);
        renderKernel<<<grid,block>>>(view,settings,image.data);
        check(cudaGetLastError(),"Launch path-tracing kernel");
        check(cudaEventRecord(end.value),"Record kernel end");
        check(cudaEventSynchronize(end.value),"Execute path-tracing kernel");
        float ms = 0;
        check(cudaEventElapsedTime(&ms,begin.value,end.value),"Measure kernel");
        result.kernelSeconds = ms/1000.0;
        check(cudaMemcpy(result.pixels.data(),image.data,result.pixels.size()*sizeof(Vec),
                         cudaMemcpyDeviceToHost),"Download linear RGB");
    } // Include freeing GPU buffers/events in wall time.
    result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    return result;
}

void cudaSelfTests(int device) {
    selectDevice(device);
    Buffer<int> output(5);
    mathTestKernel<<<1,1>>>(output.data);
    check(cudaGetLastError(),"Launch GPU self-tests");
    check(cudaDeviceSynchronize(),"Execute GPU self-tests");
    int results[5]{};
    check(cudaMemcpy(results,output.data,sizeof(results),cudaMemcpyDeviceToHost),"Read GPU tests");
    for (int i = 0; i < 5; ++i)
        if (!results[i]) throw std::runtime_error("CUDA self-test failed at case "+std::to_string(i));
}
} // namespace path_tracer
