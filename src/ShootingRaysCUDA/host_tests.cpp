#include "HostSupport.h"

int main() {
    try {
        cuda_rays::cameraTests();
        cuda_rays::intersectionTests([](const std::vector<cuda_rays::Triangle>& triangles,
                                       const std::vector<cuda_rays::Ray>& rays) {
            std::vector<float> result;
            for (const auto& ray : rays) result.push_back(cuda_rays::nearest(ray,triangles.data(),int(triangles.size())));
            return result;
        });
        const auto scene = shooting::makeScene();
        const auto triangles = cuda_rays::pack(scene);
        for (int pass = 0; pass < 3; ++pass) {
            const int width = pass == 0 ? 317 : (pass == 1 ? 160 : 1);
            const int height = pass == 0 ? 179 : (pass == 1 ? 90 : 1);
            Camera camera = shooting::defaultCamera();
            if (pass == 1) {
                camera.orientation = glm::quat(glm::vec3(glm::radians(40.0f),glm::radians(25.0f),0));
                camera.distance = 20;
            }
            const auto view = shooting::snapshot(camera,width,height);
            const auto portableView = cuda_rays::pack(view);
            std::vector<float> actual;
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    actual.push_back(cuda_rays::nearest(cuda_rays::primaryRay(portableView,x,y,width,height),
                                                        triangles.data(),int(triangles.size())));
            cuda_rays::compare(shooting::render(scene,view,width,height).distances,actual);
        }
        std::cout << "Host-only portable math tests passed. This does NOT test NVCC or a CUDA GPU.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
