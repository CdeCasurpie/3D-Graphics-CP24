#include "CudaRayCaster.h"
#include <cuda_runtime.h>
#include <chrono>
#include <climits>
#include <sstream>
#include <stdexcept>

namespace cuda_rays {
namespace {
void check(cudaError_t error, const char* operation) {
    if (error != cudaSuccess)
        throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(error));
}

template<class T> class Buffer {
public:
    T* data = nullptr;
    explicit Buffer(size_t count) {
        if (count) check(cudaMalloc(reinterpret_cast<void**>(&data), count * sizeof(T)), "cudaMalloc");
    }
    ~Buffer() { if (data) cudaFree(data); }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};

class Event {
public:
    cudaEvent_t value{};
    Event() { check(cudaEventCreate(&value), "cudaEventCreate"); }
    ~Event() { cudaEventDestroy(value); }
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
};

void selectDevice(int device) {
    int count = 0;
    check(cudaGetDeviceCount(&count), "cudaGetDeviceCount (check NVIDIA driver and GPU availability)");
    if (count == 0) throw std::runtime_error("No CUDA GPU available; no CPU fallback is used");
    if (device < 0 || device >= count) throw std::runtime_error("Invalid CUDA device index");
    check(cudaSetDevice(device), "cudaSetDevice");
}

using Clock = std::chrono::steady_clock;
double milliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

__global__ void depthKernel(const Triangle* triangles, int count, View view,
                            int width, int height, float* distances) {
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    distances[size_t(y) * width + x] = nearest(primaryRay(view,x,y,width,height), triangles,count);
}

__global__ void testKernel(const Triangle* triangles, int count, const Ray* rays,
                           int rayCount, float* distances) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < rayCount) distances[i] = nearest(rays[i],triangles,count);
}

void upload(const std::vector<Triangle>& triangles, Triangle* destination) {
    if (triangles.size() > INT_MAX) throw std::runtime_error("Too many triangles");
    if (!triangles.empty()) check(cudaMemcpy(destination, triangles.data(),
        triangles.size() * sizeof(Triangle), cudaMemcpyHostToDevice), "Upload triangles");
}
} // namespace

std::string deviceInfo(int device) {
    selectDevice(device);
    cudaDeviceProp properties{};
    check(cudaGetDeviceProperties(&properties,device), "cudaGetDeviceProperties");
    int driver = 0, runtime = 0;
    check(cudaDriverGetVersion(&driver), "cudaDriverGetVersion");
    check(cudaRuntimeGetVersion(&runtime), "cudaRuntimeGetVersion");
    std::ostringstream text;
    text << "Backend: CUDA (no CPU fallback)\nDevice " << device << ": " << properties.name
         << "\nCompute capability: " << properties.major << '.' << properties.minor
         << "\nGlobal memory: " << properties.totalGlobalMem / (1024*1024) << " MiB"
         << "\nDriver API version: " << driver << " | CUDA runtime: " << runtime;
    return text.str();
}

Result render(const std::vector<Triangle>& triangles, const View& view,
              int width, int height, int device) {
    if (width < 1 || height < 1 || width > 4096 || height > 4096)
        throw std::runtime_error("Dimensions must be 1..4096");
    selectDevice(device);
    const auto start = Clock::now();
    const size_t pixels = size_t(width) * height;
    Result result;
    result.distances.resize(pixels);
    Buffer<Triangle> deviceTriangles(triangles.size());
    Buffer<float> deviceDepth(pixels);
    const auto uploadStart = Clock::now();
    upload(triangles,deviceTriangles.data);
    result.uploadMilliseconds = milliseconds(uploadStart);
    Event begin, end;
    check(cudaEventRecord(begin.value), "Record kernel start");
    const dim3 block(16,16), grid((width+15)/16,(height+15)/16);
    depthKernel<<<grid,block>>>(deviceTriangles.data, static_cast<int>(triangles.size()), view,
                               width,height,deviceDepth.data);
    check(cudaGetLastError(), "Launch depth kernel");
    check(cudaEventRecord(end.value), "Record kernel end");
    check(cudaEventSynchronize(end.value), "Execute depth kernel");
    check(cudaEventElapsedTime(&result.kernelMilliseconds,begin.value,end.value), "Time depth kernel");
    const auto downloadStart = Clock::now();
    check(cudaMemcpy(result.distances.data(),deviceDepth.data,pixels*sizeof(float),
                     cudaMemcpyDeviceToHost), "Download depth buffer");
    result.downloadMilliseconds = milliseconds(downloadStart);
    result.totalMilliseconds = milliseconds(start);
    return result;
}

std::vector<float> traceTestRays(const std::vector<Triangle>& triangles,
                               const std::vector<Ray>& rays, int device) {
    selectDevice(device);
    if (rays.empty()) return {};
    if (rays.size() > INT_MAX) throw std::runtime_error("Too many test rays");
    Buffer<Triangle> deviceTriangles(triangles.size());
    Buffer<Ray> deviceRays(rays.size());
    Buffer<float> deviceDepth(rays.size());
    upload(triangles,deviceTriangles.data);
    check(cudaMemcpy(deviceRays.data,rays.data(),rays.size()*sizeof(Ray),cudaMemcpyHostToDevice), "Upload test rays");
    testKernel<<<(static_cast<int>(rays.size())+127)/128,128>>>(deviceTriangles.data,
        static_cast<int>(triangles.size()),deviceRays.data,static_cast<int>(rays.size()),deviceDepth.data);
    check(cudaGetLastError(), "Launch test kernel");
    check(cudaDeviceSynchronize(), "Execute test kernel");
    std::vector<float> result(rays.size());
    check(cudaMemcpy(result.data(),deviceDepth.data,result.size()*sizeof(float),cudaMemcpyDeviceToHost), "Download test results");
    return result;
}
} // namespace cuda_rays
