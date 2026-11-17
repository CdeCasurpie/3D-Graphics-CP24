#include "Window.h" // GLAD must precede GLFW headers.
#include "Camera.h"
#include "Shader.h"
#include "EricStructure.h"
#include "RayCaster.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <future>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace shooting;
static constexpr const char* outputPath = "src/ShootingRays/depth_map.png";

static void appendMesh(std::vector<Triangle>& triangles, const EricStructure& mesh,
                       const glm::mat4& transform) {
    for (size_t i = 0; i + 2 < mesh.V.size(); i += 3) {
        glm::vec3 points[3];
        for (int j = 0; j < 3; ++j) {
            const Vec3& p = mesh.G.at(mesh.V[i + j]).Position;
            points[j] = glm::vec3(transform * glm::vec4(p.x, p.y, p.z, 1.0f));
        }
        triangles.emplace_back(points[0], points[1], points[2]);
    }
}

static std::vector<Triangle> makeScene() {
    std::vector<Triangle> triangles;
    triangles.emplace_back(glm::vec3(-8, 0, -12), glm::vec3(8, 0, -12), glm::vec3(8, 0, 5));
    triangles.emplace_back(glm::vec3(-8, 0, -12), glm::vec3(8, 0, 5), glm::vec3(-8, 0, 5));
    EricStructure cube;
    cube.generateCube(1.0f);
    // Staggered towers supply silhouettes and occlusion over a broad depth range.
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 5; ++column) {
            const float height = 1.0f + float((column * 3 + row * 2) % 5) * 0.45f;
            const float x = (column - 2) * 2.5f + (row % 2) * 0.4f;
            const float z = 1.0f - row * 3.0f;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, height / 2, z));
            model = glm::scale(model, glm::vec3(1.1f, height, 1.1f));
            appendMesh(triangles, cube, model);
        }
    }
    EricStructure sphere;
    sphere.generateSphere(0.95f, 16, 10);
    appendMesh(triangles, sphere, glm::translate(glm::mat4(1), glm::vec3(-2, 0.95f, 3)));
    EricStructure torus;
    torus.generateTorus(0.9f, 0.3f, 20, 10);
    appendMesh(triangles, torus, glm::translate(glm::mat4(1), glm::vec3(2, 1.2f, 3)));
    return triangles;
}

static Camera defaultCamera() {
    Camera camera(23.0f, glm::vec3(0, 0.8f, -3));
    // Positive view X rotation places the orbital eye above the scene.
    camera.orientation = glm::quat(glm::vec3(glm::radians(27.0f), glm::radians(-13.0f), 0));
    return camera;
}

static View snapshot(const Camera& camera, int width, int height) {
    return {camera.getPosition(), glm::inverse(camera.getProjectionMatrix(width, height) * camera.getViewMatrix())};
}

static void saveImage(const DepthImage& image, const std::vector<uint8_t>& pixels,
                      const char* path) {
    if (!stbi_write_png(path, image.width, image.height, 1, pixels.data(), image.width))
        throw std::runtime_error(std::string("Cannot write PNG: ") + path);
    std::cout << "Saved " << path << " (" << image.width << "x" << image.height << ")\n";
}

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static int selfTest() {
    const Triangle front({-1,-1,-3}, {1,-1,-3}, {0,1,-3});
    const Ray ray{{0,0,0}, {0,0,-1}};
    float t = 0;
    require(intersect(ray, front, t) && std::abs(t - 3) < 1e-5f, "Known hit distance");
    require(!intersect({{3,0,0}, {0,0,-1}}, front, t), "Outside triangle");
    require(!intersect({{0,0,0}, {0,0,1}}, front, t), "Behind camera");
    require(!intersect({{0,0,0}, {1,0,0}}, front, t), "Parallel ray");
    require(intersect({{0,0,-4}, {0,0,1}}, front, t), "Back-face hit");
    require(!intersect(ray, Triangle({0,0,-1}, {0,0,-1}, {0,0,-1}), t), "Degenerate triangle");
    const Triangle far({-2,-2,-6}, {2,-2,-6}, {0,2,-6});
    require(std::abs(nearestHit(ray, {far, front}) - 3) < 1e-5f, "Nearest hit wins");
    require(!std::isfinite(nearestHit(ray, {})), "Empty scene misses");

    Camera camera(3, glm::vec3(0));
    const View view = snapshot(camera, 3, 3);
    const Ray center = primaryRay(view, 1, 1, 3, 3);
    require(glm::length(center.direction - glm::vec3(0,0,-1)) < 1e-5f, "Center ray");
    const Ray topLeft = primaryRay(view, 0, 0, 3, 3);
    require(topLeft.direction.x < 0 && topLeft.direction.y > 0, "Image axes");
    require(std::abs(glm::length(topLeft.direction) - 1) < 1e-5f, "Unit direction");
    DepthImage image;
    image.distances = {2, 4, std::numeric_limits<float>::infinity()};
    require(grayscale(image, 2, 4, 0) == std::vector<uint8_t>({0,255,0}), "Depth normalization");
    image.distances = {3,3};
    require(grayscale(image, 3, 3, 0)[0] == 128, "Constant depth");
    const auto empty = render({}, view, 3, 3);
    require(empty.hits == 0 && grayscale(empty, 0, 0) == std::vector<uint8_t>(9,0), "All-miss image");
    std::cout << "All ray-casting self-tests passed.\n";
    return 0;
}

static int viewer(const std::vector<Triangle>& triangles, bool smokeTest = false) {
    Window window(1280, 720, "Task 13: CPU Shooting Rays");
    // Tiling compositors can ignore the initial creation size/floating hint.
    // Advertise fixed landscape limits as well (same workaround as Project1).
    glfwSetWindowSizeLimits(window.glfwWindow, 1280, 720, 1280, 720);
    glfwSetWindowAspectRatio(window.glfwWindow, 16, 9);
    glfwSetWindowSize(window.glfwWindow, 1280, 720);
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // Never rewrite Project1's tracked imgui.ini.
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    Shader display("src/ShootingRays/shader.vert", "src/ShootingRays/shader.frag");
    GLuint texture, vao;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenVertexArrays(1, &vao);

    Camera camera = defaultCamera();
    DepthImage image;
    std::vector<uint8_t> pixels;
    std::future<DepthImage> job;
    std::atomic<int> completed{0};
    bool busy = false, stale = true, upload = false;
    float nearDepth = 0, farDepth = 1, floor = 0.08f;
    int preset = 1, renderHeight = 360;
    const int widths[] = {320, 640, 960, 1280};
    const int heights[] = {180, 360, 540, 720};
    std::string status;
    auto startRender = [&]() {
        completed = 0;
        renderHeight = heights[preset];
        const int width = widths[preset], height = heights[preset];
        const View view = snapshot(camera, width, height);
        job = std::async(std::launch::async, [&, view, width, height]() {
            return render(triangles, view, width, height, &completed);
        });
        busy = true;
        stale = false;
        status.clear();
    };
    startRender();
    while (!window.shouldClose()) {
        window.processInput();
        if (busy && job.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            image = job.get();
            busy = false;
            nearDepth = image.minimum;
            farDepth = image.maximum;
            upload = true;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Poll controls after the GLFW backend has updated ImGui's capture state.
        double x, y;
        glfwGetCursorPos(window.glfwWindow, &x, &y);
        int ww, wh;
        glfwGetWindowSize(window.glfwWindow, &ww, &wh);
        if (!busy && !ImGui::GetIO().WantCaptureMouse && ww > 0 && wh > 0) {
            if (glfwGetMouseButton(window.glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                if (!camera.isDragging) camera.startDragging(x, y, ww, wh);
                else { camera.onMouseDrag(x, y, ww, wh); stale = true; }
            } else camera.stopDragging();
            if (glfwGetMouseButton(window.glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                if (!camera.isPanning) camera.startPanning(x, y);
                else { camera.onMousePan(x, y, ww, wh); stale = true; }
            } else camera.stopPanning();
            if (ImGui::GetIO().MouseWheel != 0) {
                camera.onScroll(ImGui::GetIO().MouseWheel);
                stale = true;
            }
        } else { camera.stopDragging(); camera.stopPanning(); }

        ImGui::SetNextWindowPos(ImVec2(12,12), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(330,0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Task 13 - Shooting rays");
        ImGui::TextUnformatted("CPU rays / nearest triangle / no BVH");
        ImGui::Text("Triangles: %zu", triangles.size());
        ImGui::BeginDisabled(busy);
        if (ImGui::Combo("Resolution", &preset, "320 x 180\000640 x 360\000960 x 540\0001280 x 720\000")) stale = true;
        if (ImGui::Button("Render")) startRender();
        ImGui::SameLine();
        if (ImGui::Button("Reset camera")) { camera = defaultCamera(); stale = true; }
        ImGui::EndDisabled();
        if (busy) ImGui::ProgressBar(float(completed.load()) / renderHeight);
        if (stale) ImGui::TextUnformatted("Camera/settings changed: render again.");
        ImGui::TextUnformatted("Left drag: orbit | Right drag: pan\nScroll: zoom | Esc: quit");
        if (!image.distances.empty()) {
            ImGui::Separator();
            ImGui::Text("%d x %d | %.3f s", image.width, image.height, image.seconds);
            ImGui::Text("Hits: %zu / %zu", image.hits, image.distances.size());
            ImGui::Text("Hit range: %.3f - %.3f", image.minimum, image.maximum);
            ImGui::BeginDisabled(busy);
            upload |= ImGui::DragFloat("Near distance", &nearDepth, 0.1f, 0, 1000);
            upload |= ImGui::DragFloat("Far distance", &farDepth, 0.1f, 0, 1000);
            upload |= ImGui::SliderFloat("Near brightness", &floor, 0, 0.3f);
            if (ImGui::Button("Auto range")) { nearDepth = image.minimum; farDepth = image.maximum; upload = true; }
            if (ImGui::Button("Save PNG")) {
                try {
                    saveImage(image, grayscale(image, nearDepth, farDepth, floor), outputPath);
                    status = std::string("Saved ") + outputPath;
                } catch (const std::exception& e) { status = e.what(); }
            }
            ImGui::EndDisabled();
        }
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
        ImGui::End();

        if (upload) {
            pixels = grayscale(image, nearDepth, farDepth, floor);
            glBindTexture(GL_TEXTURE_2D, texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, image.width, image.height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
            upload = false;
        }
        int fw, fh;
        glfwGetFramebufferSize(window.glfwWindow, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!pixels.empty() && fw > 0 && fh > 0) {
            const float fit = std::min(float(fw) / image.width, float(fh) / image.height);
            const int vw = int(image.width * fit), vh = int(image.height * fit);
            glViewport((fw - vw) / 2, (fh - vh) / 2, vw, vh);
            display.use();
            display.setInt("depthImage", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        ImGui::Render();
        glViewport(0, 0, fw, fh);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (smokeTest && !busy && !pixels.empty()) {
            // Read the displayed image away from the UI panel. Compare a surface
            // pixel with its CPU source, including the top-down texture flip.
            uint8_t actual[4] = {};
            const int px = image.width / 2, py = image.height * 3 / 5;
            const float fit = std::min(float(fw) / image.width, float(fh) / image.height);
            const int vw = int(image.width * fit), vh = int(image.height * fit);
            const int sx = (fw - vw) / 2 + int((px + 0.5f) * fit);
            const int sy = (fh - vh) / 2 + vh - 1 - int((py + 0.5f) * fit);
            glReadPixels(sx, sy, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, actual);
            const int expected = pixels[size_t(py) * image.width + px];
            require(std::abs(int(actual[0]) - expected) <= 1 && actual[0] == actual[1]
                    && actual[1] == actual[2], "GPU display differs from CPU image");
            require(glGetError() == GL_NO_ERROR, "OpenGL smoke test error");
            std::cout << "Viewer smoke test passed (CPU pixel matches displayed grayscale).\n";
            glfwSetWindowShouldClose(window.glfwWindow, true);
        }
        window.update();
    }
    if (job.valid()) job.get(); // Finish CPU workers before scene data is destroyed.
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(display.ID);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") return selfTest();
        if (argc == 2 && std::string(argv[1]) == "--smoke-test") return viewer(makeScene(), true);
        if (argc > 1 && std::string(argv[1]) != "--render")
            throw std::runtime_error("Usage: ShootingRays [--self-test | --smoke-test | --render [width height [output.png]]]");
        const auto triangles = makeScene();
        if (argc > 1) {
            if (argc != 2 && argc != 4 && argc != 5) throw std::runtime_error("Use --render [width height [output.png]]");
            const int width = argc >= 4 ? std::stoi(argv[2]) : 640;
            const int height = argc >= 4 ? std::stoi(argv[3]) : 360;
            if (width < 1 || height < 1 || width > 4096 || height > 4096)
                throw std::runtime_error("Width and height must be between 1 and 4096");
            const auto image = render(triangles, snapshot(defaultCamera(), width, height), width, height);
            std::cout << triangles.size() << " triangles | " << image.hits << " hits | "
                      << image.seconds << " seconds | depth " << image.minimum << " - " << image.maximum << '\n';
            saveImage(image, grayscale(image, image.minimum, image.maximum), argc == 5 ? argv[4] : outputPath);
            return 0;
        }
        return viewer(triangles);
    } catch (const std::exception& e) {
        std::cerr << "ShootingRays: " << e.what() << '\n';
        return 1;
    }
}
