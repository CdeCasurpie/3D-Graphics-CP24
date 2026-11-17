#include "HostSupport.h"
#include "CudaRayCaster.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

static constexpr const char* defaultOutput = "src/ShootingRaysCUDA/depth_map.png";

static int integer(const std::string& text) {
    size_t consumed = 0;
    const int value = std::stoi(text,&consumed);
    if (consumed != text.size()) throw std::runtime_error("Not an integer: " + text);
    return value;
}

static void validate(int device) {
    cuda_rays::cameraTests();
    const auto scene = shooting::makeScene();
    const auto triangles = cuda_rays::pack(scene);
    // Odd dimensions exercise out-of-bounds threads in the 16x16 grid.
    for (int pass = 0; pass < 3; ++pass) {
        const int width = pass == 0 ? 317 : (pass == 1 ? 160 : 1);
        const int height = pass == 0 ? 179 : (pass == 1 ? 90 : 1);
        Camera camera = shooting::defaultCamera();
        if (pass == 1) {
            camera.orientation = glm::quat(glm::vec3(glm::radians(40.0f),glm::radians(25.0f),0));
            camera.distance = 20;
        }
        const auto view = shooting::snapshot(camera,width,height);
        const auto cpu = shooting::render(scene,view,width,height);
        const auto gpu = cuda_rays::render(triangles,cuda_rays::pack(view),width,height,device);
        cuda_rays::compare(cpu.distances,gpu.distances);
    }
    const auto view = cuda_rays::pack(shooting::snapshot(shooting::defaultCamera(),17,19));
    const auto empty = cuda_rays::render({},view,17,19,device);
    for (float d : empty.distances) cuda_rays::require(std::isinf(d) && d > 0,"Empty GPU scene should miss");
    std::cout << "CUDA validation PASSED.\n";
}

int main(int argc, char** argv) {
    try {
        std::vector<std::string> args;
        int device = 0;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--device") {
                if (++i == argc) throw std::runtime_error("--device needs an index");
                device = integer(argv[i]);
            } else args.emplace_back(argv[i]);
        }
        if (args.empty() || args[0] == "--help") {
            std::cout << "Usage: ShootingRaysCUDA [--device N] MODE\n"
                         "  --device-info\n  --self-test\n  --validate\n"
                         "  --render [width height [output.png]]\n"
                         "PNG default: " << defaultOutput << '\n';
            return 0;
        }
        const std::string mode = args[0];
        if (mode != "--device-info" && mode != "--self-test" && mode != "--validate" && mode != "--render")
            throw std::runtime_error("Unknown mode; use --help");
        if (mode != "--render" && args.size() != 1) throw std::runtime_error("Unexpected arguments");
        if (mode == "--render" && args.size() != 1 && args.size() != 3 && args.size() != 4)
            throw std::runtime_error("Use --render [width height [output.png]]");
        const std::string gpuInfo = cuda_rays::deviceInfo(device);
        std::cout << gpuInfo << '\n';
        if (mode == "--device-info") return 0;
        if (mode == "--self-test") {
            cuda_rays::intersectionTests([device](const auto& triangles, const auto& rays) {
                return cuda_rays::traceTestRays(triangles,rays,device);
            });
            cuda_rays::cameraTests();
            std::cout << "CUDA kernel intersection self-tests PASSED.\n";
            return 0;
        }
        if (mode == "--validate") { validate(device); return 0; }
        const int width = args.size() >= 3 ? integer(args[1]) : 960;
        const int height = args.size() >= 3 ? integer(args[2]) : 540;
        if (width < 1 || width > 4096 || height < 1 || height > 4096)
            throw std::runtime_error("Dimensions must be 1..4096");
        const std::string path = args.size() == 4 ? args[3] : defaultOutput;
        const auto scene = shooting::makeScene();
        const auto triangles = cuda_rays::pack(scene);
        const auto view = shooting::snapshot(shooting::defaultCamera(),width,height);
        // Warm up runtime/JIT and kernel before reporting measured timing.
        cuda_rays::render(triangles,cuda_rays::pack(view),32,18,device);
        const auto start = std::chrono::steady_clock::now();
        auto result = cuda_rays::render(triangles,cuda_rays::pack(view),width,height,device);
        auto image = cuda_rays::depthImage(std::move(result.distances),width,height);
        cuda_rays::require(image.hits > 0,"Default scene unexpectedly has no GPU hits");
        const auto pixels = shooting::grayscale(image,image.minimum,image.maximum,0.08f);
        const double endToEnd = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::ostringstream report;
        report << std::fixed << std::setprecision(4) << gpuInfo << '\n'
               << "Image: " << width << 'x' << height << " | Triangles: " << triangles.size()
               << " | Hits: " << image.hits << '\n'
               << "Depth range: " << image.minimum << " - " << image.maximum << '\n'
               << "Mapping: near dark / far light; floor=0.08; misses=black\n"
               << "Warm-up: one 32x18 GPU render\n"
               << "Upload ms: " << result.uploadMilliseconds << '\n'
               << "CUDA kernel ms (events): " << result.kernelMilliseconds << '\n'
               << "Download ms: " << result.downloadMilliseconds << '\n'
               << "Render call ms (allocation + upload + kernel + download; excludes cleanup): " << result.totalMilliseconds << '\n'
               << "Wall ms (render + normalization; excludes scene, warm-up, PNG encoding): " << endToEnd << '\n';
        if (!stbi_write_png(path.c_str(),width,height,1,pixels.data(),width))
            throw std::runtime_error("Cannot write PNG: " + path);
        std::ofstream metadata(path + ".txt");
        if (!metadata) throw std::runtime_error("Cannot write render metadata: " + path + ".txt");
        metadata << report.str();
        metadata.close();
        if (!metadata) throw std::runtime_error("Render metadata write failed");
        std::cout << report.str() << "Saved CUDA image: " << path << "\nSaved metadata: " << path << ".txt\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ShootingRaysCUDA: " << e.what() << "\nNo CPU fallback was used.\n";
        return 1;
    }
}
