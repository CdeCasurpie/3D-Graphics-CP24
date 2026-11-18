#include "Backend.h"
#include "LaunchParams.h"
#include <cuda.h>
#include <optix.h>
#include <optix_function_table_definition.h> // Exactly one definition per executable.
#include <optix_stubs.h>
#include <optix_stack_size.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#if OPTIX_VERSION < 70700
#error Task 16 requires OptiX SDK 7.7 or newer.
#endif
namespace realtime {
namespace {
void cuCheck(CUresult code,const char* operation) {
    if (code!=CUDA_SUCCESS) {
        const char* message=nullptr; cuGetErrorString(code,&message);
        throw std::runtime_error(std::string(operation)+": "+(message?message:"CUDA driver error"));
    }
}
void oxCheck(OptixResult code,const char* operation) {
    if (code!=OPTIX_SUCCESS)
        throw std::runtime_error(std::string(operation)+": "+optixGetErrorName(code)+" / "+optixGetErrorString(code));
}
struct Buffer {
    CUdeviceptr data=0; size_t size=0;
    ~Buffer() { reset(); }
    void reset() { if (data) cuMemFree(data); data=0; size=0; }
    void reserve(size_t bytes) {
        if (size>=bytes) return;
        reset(); if (bytes) { cuCheck(cuMemAlloc(&data,bytes),"cuMemAlloc"); size=bytes; }
    }
    void upload(const void* source,size_t bytes) {
        reserve(bytes); if (bytes) cuCheck(cuMemcpyHtoD(data,source,bytes),"Upload GPU buffer");
    }
    Buffer()=default; Buffer(const Buffer&)=delete; Buffer& operator=(const Buffer&)=delete;
};
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) Record { char header[OPTIX_SBT_RECORD_HEADER_SIZE]; };
void logCallback(unsigned level,const char* tag,const char* text,void*) {
    if (level<=2) std::cerr << "OptiX [" << tag << "]: " << text << '\n';
}
class Optix final:public Backend {
    CUdevice device=0;
    CUcontext cudaContext=nullptr;
    CUstream stream=nullptr;
    CUevent begin=nullptr,end=nullptr;
    OptixDeviceContext context=nullptr;
    OptixModule module=nullptr;
    OptixPipeline pipeline=nullptr;
    OptixProgramGroup groups[5]{};
    OptixShaderBindingTable sbt{};
    OptixTraversableHandle gas=0;
    Buffer vertices,gasBuffer,surfaces,materials,output,launch,records,testRays,testHits;
    Scene scene;
    std::string description;
    void current() { cuCheck(cuCtxSetCurrent(cudaContext),"Select OptiX CUDA context"); }
    void cleanup() noexcept {
        if (cudaContext) cuCtxSetCurrent(cudaContext);
        if (stream) cuStreamSynchronize(stream);
        testHits.reset(); testRays.reset(); launch.reset(); records.reset(); output.reset();
        materials.reset(); surfaces.reset(); gasBuffer.reset(); vertices.reset();
        if (pipeline) optixPipelineDestroy(pipeline);
        for (auto group:groups) if (group) optixProgramGroupDestroy(group);
        if (module) optixModuleDestroy(module);
        if (context) optixDeviceContextDestroy(context);
        if (begin) cuEventDestroy(begin);
        if (end) cuEventDestroy(end);
        if (stream) cuStreamDestroy(stream);
        if (cudaContext) cuDevicePrimaryCtxRelease(device);
    }
public:
    Optix(const Scene& source,int ordinal,const std::string& ptxPath):scene(source) {
        const auto start=std::chrono::steady_clock::now();
        try {
            cuCheck(cuInit(0),"cuInit (NVIDIA driver required)");
            cuCheck(cuDeviceGet(&device,ordinal),"Select CUDA device");
            cuCheck(cuDevicePrimaryCtxRetain(&cudaContext,device),"Retain CUDA context"); current();
            cuCheck(cuStreamCreate(&stream,CU_STREAM_DEFAULT),"Create render stream");
            cuCheck(cuEventCreate(&begin,CU_EVENT_DEFAULT),"Create start event");
            cuCheck(cuEventCreate(&end,CU_EVENT_DEFAULT),"Create end event");
            oxCheck(optixInit(),"optixInit (check NVIDIA driver and OptiX SDK compatibility)");
            OptixDeviceContextOptions options{};
            options.logCallbackFunction=logCallback; options.logCallbackLevel=3;
            oxCheck(optixDeviceContextCreate(cudaContext,&options,&context),"Create OptiX context");
            char name[256]{}; cuCheck(cuDeviceGetName(name,sizeof(name),device),"Get GPU name");
            int driver=0; cuCheck(cuDriverGetVersion(&driver),"Get driver version");
            description="NVIDIA OptiX SDK "+std::to_string(OPTIX_VERSION)+" | "+name+" | driver API="+std::to_string(driver);

            std::ifstream input(ptxPath,std::ios::binary);
            if (!input) throw std::runtime_error("Cannot read OptiX device module: "+ptxPath);
            const std::string ptx((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
            OptixModuleCompileOptions moduleOptions{};
            moduleOptions.optLevel=OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
            moduleOptions.debugLevel=OPTIX_COMPILE_DEBUG_LEVEL_NONE;
            OptixPipelineCompileOptions pipelineOptions{};
            pipelineOptions.traversableGraphFlags=OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
            pipelineOptions.numPayloadValues=4; pipelineOptions.numAttributeValues=2;
            pipelineOptions.pipelineLaunchParamsVariableName="params";
            pipelineOptions.usesPrimitiveTypeFlags=OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE;
            char log[8192]{}; size_t logSize=sizeof(log);
            const auto status=optixModuleCreate(context,&moduleOptions,&pipelineOptions,ptx.data(),ptx.size(),log,&logSize,&module);
            if (status!=OPTIX_SUCCESS) std::cerr << log << '\n';
            oxCheck(status,"Compile OptiX module");
            const char* entries[]={"__raygen__main","__miss__radiance","__miss__shadow","__closesthit__radiance","__closesthit__shadow"};
            for (int i=0;i<5;++i) {
                OptixProgramGroupDesc desc{};
                if (i==0) { desc.kind=OPTIX_PROGRAM_GROUP_KIND_RAYGEN; desc.raygen.module=module; desc.raygen.entryFunctionName=entries[i]; }
                else if (i<3) { desc.kind=OPTIX_PROGRAM_GROUP_KIND_MISS; desc.miss.module=module; desc.miss.entryFunctionName=entries[i]; }
                else { desc.kind=OPTIX_PROGRAM_GROUP_KIND_HITGROUP; desc.hitgroup.moduleCH=module; desc.hitgroup.entryFunctionNameCH=entries[i]; }
                OptixProgramGroupOptions pgOptions{}; logSize=sizeof(log);
                const auto result=optixProgramGroupCreate(context,&desc,1,&pgOptions,log,&logSize,&groups[i]);
                if (result!=OPTIX_SUCCESS) std::cerr << log << '\n';
                oxCheck(result,"Create OptiX program group");
            }
            OptixPipelineLinkOptions link{}; link.maxTraceDepth=1; logSize=sizeof(log);
            const auto linked=optixPipelineCreate(context,&pipelineOptions,&link,groups,5,log,&logSize,&pipeline);
            if (linked!=OPTIX_SUCCESS) std::cerr << log << '\n';
            oxCheck(linked,"Link OptiX pipeline");
            OptixStackSizes sizes{};
            for (auto group:groups) oxCheck(optixUtilAccumulateStackSizes(group,&sizes,pipeline),"Get pipeline stack sizes");
            unsigned directTraversal=0,directState=0,continuation=0;
            oxCheck(optixUtilComputeStackSizes(&sizes,1,0,0,&directTraversal,&directState,&continuation),"Compute pipeline stack sizes");
            oxCheck(optixPipelineSetStackSize(pipeline,directTraversal,directState,continuation,1),"Set pipeline stack sizes");
            Record hostRecords[5]{};
            for (int i=0;i<5;++i) oxCheck(optixSbtRecordPackHeader(groups[i],&hostRecords[i]),"Pack SBT record");
            records.upload(hostRecords,sizeof(hostRecords));
            sbt.raygenRecord=records.data;
            sbt.missRecordBase=records.data+sizeof(Record); sbt.missRecordCount=2; sbt.missRecordStrideInBytes=sizeof(Record);
            sbt.hitgroupRecordBase=records.data+3*sizeof(Record); sbt.hitgroupRecordCount=2; sbt.hitgroupRecordStrideInBytes=sizeof(Record);

            std::vector<Vec> hostVertices; hostVertices.reserve(scene.surfaces.size()*3);
            for (const auto& surface:scene.surfaces) {
                const auto& t=surface.triangle;
                hostVertices.push_back(t.v0); hostVertices.push_back(add(t.v0,t.edge1)); hostVertices.push_back(add(t.v0,t.edge2));
            }
            if (!hostVertices.empty()) {
                vertices.upload(hostVertices.data(),hostVertices.size()*sizeof(Vec));
                const unsigned flags=OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;
                OptixBuildInput build{}; build.type=OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
                build.triangleArray.vertexFormat=OPTIX_VERTEX_FORMAT_FLOAT3;
                build.triangleArray.vertexStrideInBytes=sizeof(Vec);
                build.triangleArray.numVertices=unsigned(hostVertices.size());
                build.triangleArray.vertexBuffers=&vertices.data;
                build.triangleArray.flags=&flags;
                build.triangleArray.numSbtRecords=1;
                OptixAccelBuildOptions accel{}; accel.buildFlags=OPTIX_BUILD_FLAG_PREFER_FAST_TRACE;
                accel.operation=OPTIX_BUILD_OPERATION_BUILD;
                OptixAccelBufferSizes bytes{};
                oxCheck(optixAccelComputeMemoryUsage(context,&accel,&build,1,&bytes),"Size OptiX GAS");
                Buffer scratch; scratch.reserve(bytes.tempSizeInBytes); gasBuffer.reserve(bytes.outputSizeInBytes);
                oxCheck(optixAccelBuild(context,stream,&accel,&build,1,scratch.data,scratch.size,
                    gasBuffer.data,gasBuffer.size,&gas,nullptr,0),"Build OptiX GAS");
                cuCheck(cuStreamSynchronize(stream),"Finish OptiX GAS build");
            }
            surfaces.upload(scene.surfaces.data(),scene.surfaces.size()*sizeof(Surface));
            materials.upload(scene.materials.data(),scene.materials.size()*sizeof(Material));
            launch.reserve(sizeof(LaunchParams));
            buildSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        } catch (...) { cleanup(); throw; }
    }
    ~Optix() override { cleanup(); }
    std::string info() const override { return description; }
    Batch render(Camera camera,Config config) override {
        current(); const auto start=std::chrono::steady_clock::now();
        Batch batch; batch.pixels.resize(size_t(config.width)*config.height);
        output.reserve(batch.pixels.size()*sizeof(Vec));
        LaunchParams p{}; p.traversable=gas; p.camera=camera; p.config=config;
        p.scene={reinterpret_cast<const Surface*>(surfaces.data),reinterpret_cast<const Material*>(materials.data),scene.light};
        p.output=reinterpret_cast<Vec*>(output.data);
        launch.upload(&p,sizeof(p));
        cuCheck(cuEventRecord(begin,stream),"Start launch timing");
        oxCheck(optixLaunch(pipeline,stream,launch.data,sizeof(p),&sbt,config.width,config.height,1),"Launch OptiX path tracer");
        cuCheck(cuEventRecord(end,stream),"End launch timing");
        cuCheck(cuEventSynchronize(end),"Execute OptiX path tracer");
        float ms=0; cuCheck(cuEventElapsedTime(&ms,begin,end),"Measure OptiX launch");
        batch.deviceSeconds=ms/1000.0;
        cuCheck(cuMemcpyDtoH(batch.pixels.data(),output.data,batch.pixels.size()*sizeof(Vec)),"Download OptiX RGB");
        batch.seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        return batch;
    }
    std::vector<Hit> intersect(const std::vector<Ray>& rays) override {
        if (rays.empty()) return {};
        current(); std::vector<Hit> hits(rays.size());
        testRays.upload(rays.data(),rays.size()*sizeof(Ray)); testHits.reserve(hits.size()*sizeof(Hit));
        LaunchParams p{}; p.traversable=gas; p.testRays=reinterpret_cast<const Ray*>(testRays.data);
        p.testHits=reinterpret_cast<Hit*>(testHits.data); p.testCount=int(rays.size());
        launch.upload(&p,sizeof(p));
        oxCheck(optixLaunch(pipeline,stream,launch.data,sizeof(p),&sbt,unsigned(rays.size()),1,1),"Launch OptiX intersection tests");
        cuCheck(cuStreamSynchronize(stream),"Execute OptiX tests");
        cuCheck(cuMemcpyDtoH(hits.data(),testHits.data,hits.size()*sizeof(Hit)),"Download OptiX test hits");
        return hits;
    }
};
} // namespace
std::unique_ptr<Backend> makeOptix(const Scene& scene,int device,const std::string& ptxPath) {
    return std::make_unique<Optix>(scene,device,ptxPath);
}
} // namespace realtime
