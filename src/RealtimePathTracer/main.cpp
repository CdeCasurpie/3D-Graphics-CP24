#include "Backend.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifndef REALTIME_PTX_PATH
#define REALTIME_PTX_PATH ""
#endif
namespace realtime {
namespace {
int number(const std::string& text,int low,int high) {
    size_t used=0; const auto n=std::stoll(text,&used);
    if (used!=text.size() || n<low || n>high) throw std::runtime_error("Invalid numeric value: "+text);
    return int(n);
}
double quantile(std::vector<double> values,double fraction) {
    std::sort(values.begin(),values.end());
    const double index=fraction*(values.size()-1);
    const auto low=size_t(index),high=std::min(low+1,values.size()-1);
    return values[low]+(values[high]-values[low])*(index-low);
}
void write(const std::string& path,const std::string& text) {
    std::ofstream out(path); out<<text; out.close();
    if (!out) throw std::runtime_error("Cannot write "+path);
}
}
int run(int argc,char** argv) {
    std::string command="render",backendName="embree",ptx=REALTIME_PTX_PATH;
    std::string directory="src/RealtimePathTracer/output",output,capture;
    Config config; unsigned threads=std::max(1u,std::thread::hardware_concurrency());
    int device=0,frames=120,warmup=10,captureFrames=0,captureSeconds=0; bool tour=false,smoke=false;
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        auto value=[&]() -> std::string { if (++i==argc) throw std::runtime_error("Missing value for "+arg); return argv[i]; };
        if (arg=="--render" || arg=="--self-test" || arg=="--validate" || arg=="--benchmark" || arg=="--viewer" || arg=="--help") command=arg.substr(2);
        else if (arg=="--smoke-test") { command="viewer"; smoke=true; }
        else if (arg=="--backend") backendName=value();
        else if (arg=="--width") config.width=number(value(),1,4096);
        else if (arg=="--height") config.height=number(value(),1,4096);
        else if (arg=="--samples" || arg=="--spp") config.spp=number(value(),1,65536);
        else if (arg=="--bounces") config.bounces=number(value(),1,32);
        else if (arg=="--seed") config.seed=uint32_t(number(value(),0,2147483647));
        else if (arg=="--threads") threads=unsigned(number(value(),1,1024));
        else if (arg=="--device") device=number(value(),0,1024);
        else if (arg=="--frames") frames=number(value(),1,10000);
        else if (arg=="--warmup") warmup=number(value(),1,1000);
        else if (arg=="--ptx") ptx=value();
        else if (arg=="--output-dir") directory=value();
        else if (arg=="--output") output=value();
        else if (arg=="--tour") tour=true;
        else if (arg=="--capture-dir") capture=value();
        else if (arg=="--capture-frames") captureFrames=number(value(),1,10000);
        else if (arg=="--capture-seconds") captureSeconds=number(value(),1,300);
        else throw std::runtime_error("Unknown argument: "+arg);
    }
    if (command=="help") {
        std::cout << "Task 16: Intel Embree / NVIDIA OptiX path tracing\n"
          "Commands: --render, --self-test, --validate (OptiX build), --benchmark, --viewer, --smoke-test\n"
          "--backend embree|optix --width 480 --height 360 --samples 1 --bounces 8 --seed 42\n"
          "--threads N --device 0 --ptx file --output image.png --output-dir directory\n"
          "Benchmark: --frames 120 --warmup 10; viewer: --tour --capture-dir directory --capture-seconds 15\n";
        return 0;
    }
    const auto scene=makeScene();
    auto make=[&](const std::string& name) -> std::unique_ptr<Backend> {
        if (name=="embree") return makeEmbree(scene,threads);
#ifdef REALTIME_OPTIX
        if (name=="optix") return makeOptix(scene,device,ptx);
#endif
        throw std::runtime_error("Backend unavailable: "+name+". OptiX requires an ENABLE_OPTIX build; no fallback is used.");
    };
    auto backend=make(backendName);
    std::cout << backend->info() << " | acceleration/context build=" << backend->buildSeconds << " s\n";
    if (command=="self-test") { selfTests(*backend,scene); return 0; }
    if (command=="validate") {
#ifdef REALTIME_OPTIX
        auto cpu=make("embree"),gpu=make("optix");
        selfTests(*cpu,scene); selfTests(*gpu,scene); compare(*cpu,*gpu,scene); return 0;
#else
        throw std::runtime_error("--validate needs an OptiX build; --self-test validates Embree locally");
#endif
    }
    if (command=="viewer") {
#ifdef REALTIME_VIEWER
        return viewer(scene,std::move(backend),backendName,config,threads,device,ptx,smoke,tour,captureFrames,captureSeconds,capture);
#else
        throw std::runtime_error("Viewer not built. Configure -DBUILD_REALTIME_VIEWER=ON and use PathTracerViewer.");
#endif
    }
    std::ostringstream settings;
    settings << backend->info() << "\nTriangles=" << scene.surfaces.size() << " | " << config.width << 'x' << config.height
             << " | samples/frame=" << config.spp << " | bounces=" << config.bounces << " | seed=" << config.seed << '\n'
             << "Acceleration/context build seconds=" << backend->buildSeconds << '\n';
    if (command=="benchmark") {
        std::filesystem::create_directories(directory);
        for (int i=0;i<warmup;++i) { config.sampleOffset=uint32_t(i*config.spp); backend->render(scene.camera,config); }
        std::ostringstream csv; csv << "frame,sample_offset,wall_seconds,wall_fps,device_seconds\n" << std::setprecision(10);
        std::vector<double> times; double sum=0; Batch last;
        for (int i=0;i<frames;++i) {
            config.sampleOffset=uint32_t(i*config.spp);
            last=backend->render(scene.camera,config); times.push_back(last.seconds); sum+=last.seconds;
            csv << i << ',' << config.sampleOffset << ',' << last.seconds << ',' << 1/last.seconds << ',' << last.deviceSeconds << '\n';
            if ((i+1)%30==0) std::cout << "Benchmark " << i+1 << '/' << frames << std::endl;
        }
        settings << "Frames=" << frames << " | warmup=" << warmup << "\nMean FPS=" << frames/sum
                 << "\nMedian frame seconds=" << quantile(times,0.5) << "\nMedian frame FPS=" << 1/quantile(times,0.5)
                 << "\np95 frame seconds=" << quantile(times,0.95) << '\n'
                 << "Scope: output allocation + tracing + readback; excludes BVH/pipeline build, warmup, PNG and display.\n"
                 << "OptiX retains scene/output allocations across frames; CPU includes thread startup/join.\n";
        write(directory+"/benchmark.csv",csv.str()); write(directory+"/summary.txt",settings.str());
        savePNG(directory+"/benchmark-last-frame.png",last.pixels,config.width,config.height);
        std::cout << settings.str(); return 0;
    }
    const auto image=backend->render(scene.camera,config);
    const auto path=output.empty()?directory+"/"+backendName+".png":output;
    savePNG(path,image.pixels,config.width,config.height);
    settings << "Render seconds=" << image.seconds << " | FPS=" << 1/image.seconds << '\n';
    write(path+".txt",settings.str()); std::cout << settings.str() << "Saved " << path << '\n';
    return 0;
}
} // namespace realtime
int main(int argc,char** argv) {
    try { return realtime::run(argc,argv); }
    catch (const std::exception& error) { std::cerr << "RealtimePathTracer: " << error.what() << '\n'; return 1; }
}
