#include "Renderer.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace path_tracer;
namespace {
struct Options {
    Settings settings;
    unsigned threads = defaultThreads();
    int device = 0, trials = 3;
    float exposure = 1;
    std::string command = "render";
    std::string output;
    std::string directory = "src/SimplePathTracer/output";
};

long long integer(const std::string& value) {
    size_t consumed = 0;
    const long long result = std::stoll(value,&consumed);
    if (consumed != value.size()) throw std::runtime_error("Invalid integer: "+value);
    return result;
}
int bounded(const std::string& value, int low, int high) {
    const auto n = integer(value);
    if (n < low || n > high) throw std::runtime_error("Value out of range: "+value);
    return int(n);
}
Options parse(int argc, char** argv) {
    Options options;
    bool commandSet = false;
    for (int i=1; i<argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i == argc) throw std::runtime_error("Missing value for "+arg);
            return argv[i];
        };
        if (arg=="--self-test" || arg=="--validate" || arg=="--benchmark" || arg=="--render" ||
            arg=="--device-info" || arg=="--help") {
            if (commandSet) throw std::runtime_error("Choose one command");
            options.command=arg.substr(2); commandSet=true;
        } else if (arg=="--width") options.settings.width=bounded(value(),1,4096);
        else if (arg=="--height") options.settings.height=bounded(value(),1,4096);
        else if (arg=="--samples") options.settings.samples=bounded(value(),1,65536);
        else if (arg=="--bounces") options.settings.bounces=bounded(value(),1,32);
        else if (arg=="--threads") options.threads=unsigned(bounded(value(),1,1024));
        else if (arg=="--trials") options.trials=bounded(value(),1,30);
        else if (arg=="--device") options.device=bounded(value(),0,1024);
        else if (arg=="--seed") {
            const auto seed=integer(value());
            if (seed<0 || seed>4294967295LL) throw std::runtime_error("Seed must be uint32");
            options.settings.seed=uint32_t(seed);
        } else if (arg=="--mode") {
            const auto mode=value();
            if (mode!="path" && mode!="direct") throw std::runtime_error("Mode must be path or direct");
            options.settings.direct=mode=="direct";
        } else if (arg=="--output") options.output=value();
        else if (arg=="--output-dir") options.directory=value();
        else if (arg=="--exposure") {
            const auto text=value(); size_t used=0;
            options.exposure=std::stof(text,&used);
            if (used!=text.size() || !std::isfinite(options.exposure) || options.exposure<=0)
                throw std::runtime_error("Exposure must be positive and finite");
        } else throw std::runtime_error("Unknown argument: "+arg);
    }
    return options;
}

std::string describe(const Options& options) {
    const auto& s=options.settings;
    std::ostringstream text;
    text << "Scene: diffuse Cornell box | " << s.width << 'x' << s.height
         << " | samples/pixel=" << s.samples << " | mode=" << (s.direct?"direct":"path")
         << " | max diffuse surfaces=" << (s.direct?1:s.bounces) << " | seed=" << s.seed
         << " | CPU threads=" << std::min(options.threads,unsigned(s.height))
         << " | exposure=" << options.exposure << '\n';
    text << "PNG: Reinhard(exposure * linear RGB), then sRGB transfer; raw RGB used for validation.\n";
    return text.str();
}
void checkPixels(const Result& result) {
    for (Vec p:result.pixels)
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || p.x<0 || p.y<0 || p.z<0)
            throw std::runtime_error("Invalid linear radiance in output");
}
uint8_t encode(float value, float exposure) {
    const double scaled=double(value)*exposure;
    const double mapped=scaled/(1+scaled);
    const double srgb=mapped<=0.0031308 ? 12.92*mapped : 1.055*std::pow(mapped,1.0/2.4)-0.055;
    return uint8_t(std::lround(std::clamp(srgb,0.0,1.0)*255));
}
void save(const Result& result, const Options& options, const std::string& path) {
    checkPixels(result);
    const std::filesystem::path destination(path);
    if (!destination.parent_path().empty()) std::filesystem::create_directories(destination.parent_path());
    std::vector<uint8_t> pixels(result.pixels.size()*3);
    for (size_t i=0; i<result.pixels.size(); ++i) {
        pixels[3*i]=encode(result.pixels[i].x,options.exposure);
        pixels[3*i+1]=encode(result.pixels[i].y,options.exposure);
        pixels[3*i+2]=encode(result.pixels[i].z,options.exposure);
    }
    if (!stbi_write_png(path.c_str(),options.settings.width,options.settings.height,3,
                        pixels.data(),options.settings.width*3)) throw std::runtime_error("Cannot save "+path);
    std::cout << "Saved " << path << '\n';
}
void writeText(const std::string& path, const std::string& contents) {
    std::ofstream stream(path);
    stream << contents;
    stream.close();
    if (!stream) throw std::runtime_error("Cannot write "+path);
}
double median(std::vector<double> values) {
    std::sort(values.begin(),values.end());
    const size_t n=values.size();
    return n%2 ? values[n/2] : (values[n/2-1]+values[n/2])*0.5;
}

#ifdef PATH_TRACER_CUDA
void compare(const Result& reference, const Result& actual) {
    checkPixels(reference); checkPixels(actual);
    if (reference.pixels.size()!=actual.pixels.size()) throw std::runtime_error("RGB size mismatch");
    double squares=0,referenceSquares=0,maximum=0;
    size_t failures=0;
    for (size_t i=0; i<reference.pixels.size(); ++i) {
        const Vec a=reference.pixels[i],b=actual.pixels[i];
        const float av[]={a.x,a.y,a.z},bv[]={b.x,b.y,b.z};
        for (int c=0;c<3;++c) {
            const double error=std::abs(double(av[c])-bv[c]);
            squares+=error*error; referenceSquares+=double(av[c])*av[c];
            maximum=std::max(maximum,error);
            if (error>0.05+0.02*std::abs(av[c])) ++failures;
        }
    }
    const double count=double(reference.pixels.size())*3;
    const double rms=std::sqrt(squares/count),referenceRms=std::sqrt(referenceSquares/count);
    std::cout << "Linear RGB comparison: max_abs=" << maximum << " RMSE=" << rms
              << " reference_RMS=" << referenceRms << " component_failures=" << failures << '\n';
    if (failures || rms>0.002+0.003*referenceRms)
        throw std::runtime_error("CPU/CUDA RGB comparison failed; inspect before submitting");
}
void validate(const Scene& scene, const Options& options) {
    for (bool direct:{true,false}) {
        Settings settings=options.settings;
        settings.width=37; settings.height=29; settings.samples=16;
        settings.bounces=4; settings.direct=direct;
        compare(renderCPU(scene,settings,options.threads),renderCUDA(scene,settings,options.device));
    }
    Scene empty=scene; empty.surfaces.clear();
    Settings settings; settings.width=1; settings.height=1; settings.samples=1;
    compare(renderCPU(empty,settings,1),renderCUDA(empty,settings,options.device));
    settings.width=17; settings.height=19;
    compare(renderCPU(empty,settings,1),renderCUDA(empty,settings,options.device));
    std::cout << "CUDA path-tracer validation PASSED.\n";
}
#endif

int run(const Options& options) {
    if (options.command=="help") {
        std::cout << "SimplePathTracerCPU / SimplePathTracerCUDA\n"
          "Commands: --render (default), --self-test, --validate (CUDA), --benchmark, --device-info\n"
          "Settings: --width 640 --height 360 --samples 32 --bounces 4 --seed 42\n"
          "          --mode path|direct --threads N --device 0 --exposure 1\n"
          "Output:   --output image.png | --output-dir src/SimplePathTracer/output\n"
          "Benchmark: --trials 3 (warm-up excluded; CUDA binary measures BOTH CPU and GPU)\n";
        return 0;
    }
    if (options.command=="self-test") {
        selfTests();
#ifdef PATH_TRACER_CUDA
        cudaSelfTests(options.device);
        std::cout << "CUDA kernel self-tests PASSED.\n";
#endif
        return 0;
    }
    std::string hardware="CPU hardware concurrency: "+std::to_string(defaultThreads());
#ifdef PATH_TRACER_CUDA
    hardware+='\n'+deviceInfo(options.device);
#endif
    std::cout << hardware << '\n';
    if (options.command=="device-info") return 0;
    const Scene scene=makeScene();
    if (options.command=="validate") {
#ifdef PATH_TRACER_CUDA
        validate(scene,options); return 0;
#else
        throw std::runtime_error("--validate requires SimplePathTracerCUDA; use --self-test on CPU");
#endif
    }
    std::cout << describe(options) << "Triangles: " << scene.surfaces.size() << '\n';
    // Warm up a small image to initialize thread/runtime/JIT machinery. Each
    // benchmark backend also receives a full-configuration unmeasured render.
    Settings warm=options.settings; warm.width=16; warm.height=16; warm.samples=1;
    renderCPU(scene,warm,options.threads);
#ifdef PATH_TRACER_CUDA
    renderCUDA(scene,warm,options.device);
#endif
    if (options.command=="benchmark") {
        std::filesystem::create_directories(options.directory);
        std::ostringstream csv;
        csv << std::setprecision(10)
            << "backend,trial,width,height,samples,bounces,mode,seed,cpu_threads,wall_seconds,kernel_seconds\n";
        std::vector<double> cpuTimes,gpuTimes,kernelTimes;
        Result cpu,gpu;
        std::cout << "Full-configuration warm-up (not timed in results)..." << std::endl;
        renderCPU(scene,options.settings,options.threads);
#ifdef PATH_TRACER_CUDA
        renderCUDA(scene,options.settings,options.device);
#endif
        auto row=[&](const char* backend,int trial,const Result& result) {
            const auto& s=options.settings;
            csv << backend << ',' << trial << ',' << s.width << ',' << s.height << ',' << s.samples << ','
                << (s.direct?1:s.bounces) << ',' << (s.direct?"direct":"path") << ',' << s.seed << ','
                << std::min(options.threads,unsigned(s.height)) << ',' << result.wallSeconds << ',';
            if (std::string(backend)=="CUDA") csv << result.kernelSeconds;
            csv << '\n';
            std::cout << backend << " trial " << trial << ": " << result.wallSeconds << " s" << std::endl;
        };
        // Alternate backend order to reduce systematic ordering bias.
        for (int trial=1;trial<=options.trials;++trial) {
            auto cpuRun=[&]() { cpu=renderCPU(scene,options.settings,options.threads); checkPixels(cpu);
                cpuTimes.push_back(cpu.wallSeconds); row("CPU",trial,cpu); };
#ifdef PATH_TRACER_CUDA
            auto gpuRun=[&]() { gpu=renderCUDA(scene,options.settings,options.device); checkPixels(gpu);
                gpuTimes.push_back(gpu.wallSeconds); kernelTimes.push_back(gpu.kernelSeconds); row("CUDA",trial,gpu); };
            if (trial%2) { cpuRun(); gpuRun(); } else { gpuRun(); cpuRun(); }
#else
            cpuRun();
#endif
            writeText(options.directory+"/benchmark.csv",csv.str());
        }
        std::ostringstream report;
        report << std::setprecision(9) << hardware << '\n' << describe(options)
               << "Trials: " << options.trials << " | full-configuration warm-up excluded\n"
               << "CPU median render seconds: " << median(cpuTimes) << '\n';
#ifdef PATH_TRACER_CUDA
        compare(cpu,gpu); // Validate final benchmark images before labeling success.
        report << "CUDA median render seconds: " << median(gpuTimes) << '\n'
               << "CUDA median kernel seconds: " << median(kernelTimes) << '\n'
               << "CPU / CUDA render speedup: " << median(cpuTimes)/median(gpuTimes) << "x\n"
               << "CPU / CUDA kernel-only ratio (not end-to-end): " << median(cpuTimes)/median(kernelTimes) << "x\n";
        save(gpu,options,options.directory+"/cuda.png");
#endif
        report << "Render wall timing includes output allocation and worker/device cleanup. CUDA includes scene upload\n"
               << "and RGB download. Excludes scene construction, context initialization, warm-up, validation,\n"
               << "tone mapping, PNG encoding, and file I/O. Kernel timing uses CUDA events.\n";
        save(cpu,options,options.directory+"/cpu.png");
        writeText(options.directory+"/summary.txt",report.str());
        std::cout << report.str();
        return 0;
    }
    Result result;
    std::string backend;
#ifdef PATH_TRACER_CUDA
    result=renderCUDA(scene,options.settings,options.device); backend="cuda";
#else
    result=renderCPU(scene,options.settings,options.threads); backend="cpu";
#endif
    const auto path=options.output.empty() ? options.directory+"/"+backend+".png" : options.output;
    save(result,options,path);
    const std::string report=hardware+"\n"+describe(options)+"Render seconds: "+std::to_string(result.wallSeconds)+
        "\nCUDA kernel seconds (0 on CPU): "+std::to_string(result.kernelSeconds)+"\n";
    writeText(path+".txt",report);
    std::cout << report;
    return 0;
}
} // namespace

int main(int argc,char** argv) {
    try { return run(parse(argc,argv)); }
    catch (const std::exception& error) {
        std::cerr << "SimplePathTracer: " << error.what() << '\n';
#ifdef PATH_TRACER_CUDA
        std::cerr << "No CPU fallback was used for CUDA rendering.\n";
#endif
        return 1;
    }
}
