#include "Window.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

unsigned int loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "EricStructure.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =====================================================================
// Globals
// =====================================================================
Camera cam;
bool performVertexPick = false;
double pickX = 0, pickY = 0;

// =====================================================================
// Vertex Picking via Raycasting
// =====================================================================
int getClosestVertex(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     const EricStructure& mesh, const glm::mat4& modelMatrix) {
    int bestVertex = -1;
    float bestT = 1e9f;

    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
    glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

    // Build set of active vertex indices for O(1) lookup
    std::set<unsigned int> activeVerts(mesh.V.begin(), mesh.V.end());
    activeVerts.erase((unsigned int)-1);

    for (unsigned int idx : activeVerts) {
        if (idx >= mesh.G.size()) continue;
        glm::vec3 pos(mesh.G[idx].Position.x, mesh.G[idx].Position.y, mesh.G[idx].Position.z);
        glm::vec3 vToP = pos - localOrigin;

        float t = glm::dot(vToP, localDir);
        if (t < 0.0f) continue;

        glm::vec3 projPos = localOrigin + localDir * t;
        float dist = glm::length(pos - projPos);

        if (dist < 0.15f && t < bestT) {
            bestT = t;
            bestVertex = (int)idx;
        }
    }
    return bestVertex;
}

// =====================================================================
// GLFW Callbacks — forward to ImGui first, then handle camera
// =====================================================================
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    // If ImGui wants the mouse, don't process camera/picking
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (mods & GLFW_MOD_CONTROL) {
                performVertexPick = true;
                pickX = x; pickY = y;
            } else if (mods & GLFW_MOD_SHIFT) {
                cam.startPanning(x, y);
            } else {
                cam.startDragging(x, y, width, height);
            }
        } else if (action == GLFW_RELEASE) {
            cam.stopDragging();
            cam.stopPanning();
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)
            cam.startPanning(x, y);
        else if (action == GLFW_RELEASE)
            cam.stopPanning();
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // Forward to ImGui
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    if (cam.isDragging)
        cam.onMouseDrag(xpos, ypos, width, height);
    else if (cam.isPanning)
        cam.onMousePan(xpos, ypos, width, height);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Forward to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    cam.onScroll(yoffset);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Forward to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void charCallback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}

// =====================================================================
// ImGui Style
// =====================================================================
void setupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.10f, 0.10f, 0.13f, 0.94f);
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.26f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.26f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.20f, 0.46f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.24f, 0.26f, 0.33f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.36f, 0.64f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_Separator]          = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);
}

// =====================================================================
// Main
// =====================================================================
int main() {
    // 16:9 landscape
    Window window(1280, 720, "Project 1: Interactive 3D Scene Viewer & Engine");

    // Force size for tiling WMs like Hyprland
    glfwSetWindowSizeLimits(window.glfwWindow, 1280, 720, 1280, 720);
    glfwSetWindowSize(window.glfwWindow, 1280, 720);
    glfwSetWindowAspectRatio(window.glfwWindow, 16, 9);

    // ImGui — install_callbacks=false, we forward manually
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    setupImGuiStyle();
    ImGui_ImplGlfw_InitForOpenGL(window.glfwWindow, false);  // false = we handle callbacks
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Install our callbacks that forward to ImGui
    glfwSetMouseButtonCallback(window.glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(window.glfwWindow, cursorPosCallback);
    glfwSetScrollCallback(window.glfwWindow, scrollCallback);
    glfwSetKeyCallback(window.glfwWindow, keyCallback);
    glfwSetCharCallback(window.glfwWindow, charCallback);

    // Camera
    cam.reset(6.0f);

    // Shaders
    Shader mainShader("src/Project1/shaders/main.vert", "src/Project1/shaders/main.frag");
    Shader depthShader("src/Project1/shaders/depth.vert", "src/Project1/shaders/depth.frag");

    // Scene 1: Thesis Model (OBJ)
    Model* model = new Model("assets/models/thesis/03_corner.obj");

    // Scene 2: Procedural Torus (for FMM demo)
    EricStructure torusMesh;
    torusMesh.generateTorus(1.0f, 0.4f, 60, 60, true);
    std::vector<float> torusDistances(torusMesh.G.size(), 999999.0f);
    float torusMaxDist = 1.0f;

    VAO torusVAO; torusVAO.bind();
    VBO torusVBO((float*)torusMesh.G.data(), torusMesh.G.size() * sizeof(VertexData));
    torusVAO.linkAttrib(torusVBO, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    torusVAO.linkAttrib(torusVBO, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    torusVAO.linkAttrib(torusVBO, 2, 2, GL_FLOAT, sizeof(VertexData), (void*)(2 * sizeof(Vec3)));
    VBO torusDistVBO(torusDistances.data(), torusDistances.size() * sizeof(float));
    torusVAO.linkAttrib(torusDistVBO, 3, 1, GL_FLOAT, sizeof(float), (void*)0);
    EBO torusEBO(torusMesh.V);
    torusVAO.unbind();

    // Shadow Map FBO
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO, depthMapTex;
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMapTex);
    glBindTexture(GL_TEXTURE_2D, depthMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // State
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    int renderMode = 0;          // 0=Textured, 1=Heatmap, 2=Normals, 3=Depth
    bool animateLight = false;
    bool animateModel = false;
    bool isPaused = false;
    float modelRotationX = -90.0f; // Z-up correction
    float modelRotationY = 0.0f;

    int simplifyTarget = 1000;
    float globalMaxDist = 1.0f;
    int pickedVertex = -1;

    int activeScene = 0; // 0=OBJ Model, 1=Torus
    int torusPickedVertex = -1;

    // Model path input buffer
    static char modelPathBuf[512] = "assets/models/thesis/03_corner.obj";
    std::string loadedModelName = "Thesis Building";

    // Lambda to reload model from a path
    auto reloadModel = [&](const std::string& path, const std::string& name) {
        std::cout << "[Load] Loading: " << path << std::endl;
        Model* newModel = new Model(path);
        if (newModel->meshes.empty()) {
            std::cerr << "[Load] Failed or empty model: " << path << std::endl;
            delete newModel;
            return;
        }
        delete model;
        model = newModel;
        loadedModelName = name;
        pickedVertex = -1;
        globalMaxDist = 1.0f;
        renderMode = 0;
        activeScene = 0;
        cam.reset(6.0f);
        strncpy(modelPathBuf, path.c_str(), sizeof(modelPathBuf) - 1);
        std::cout << "[Load] Success! Meshes: " << model->meshes.size()
                  << ", Vertices: " << model->meshes[0]->geometry.G.size() << std::endl;
    };

    float lastTime = (float)glfwGetTime();

    glEnable(GL_DEPTH_TEST);

    // =====================================================================
    // Main Loop
    // =====================================================================
    while (!window.shouldClose()) {
        window.processInput();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (!isPaused) {
            if (animateLight) {
                lightPos.x = sin(currentTime * 0.8f) * 5.0f;
                lightPos.z = cos(currentTime * 0.8f) * 5.0f;
            }
            if (animateModel) {
                modelRotationY += 25.0f * deltaTime;
            }
        }

        // ----- Vertex Picking -----
        if (performVertexPick) {
            performVertexPick = false;
            int w, h;
            glfwGetFramebufferSize(window.glfwWindow, &w, &h);

            float ndcX = (2.0f * (float)pickX) / w - 1.0f;
            float ndcY = 1.0f - (2.0f * (float)pickY) / h;

            glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
            glm::mat4 proj = cam.getProjectionMatrix(w, h);
            glm::vec4 rayEye = glm::inverse(proj) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(cam.getViewMatrix()) * rayEye));

            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(modelRotationX), glm::vec3(1, 0, 0));
            modelMat = glm::rotate(modelMat, glm::radians(modelRotationY), glm::vec3(0, 1, 0));

            if (activeScene == 0 && !model->meshes.empty()) {
                int sel = getClosestVertex(cam.getPosition(), rayWorld, model->meshes[0]->geometry, modelMat);
                if (sel != -1) {
                    pickedVertex = sel;
                    std::cout << "[Pick] Vertex " << sel << " selected on Thesis model" << std::endl;
                    auto dists = model->meshes[0]->geometry.fastMarching(sel);
                    model->meshes[0]->distances = dists;
                    model->meshes[0]->setup();
                    globalMaxDist = 0.0f;
                    for (float d : dists)
                        if (d < 990000.0f && d > globalMaxDist) globalMaxDist = d;
                    renderMode = 1;
                }
            } else if (activeScene == 1) {
                int sel = getClosestVertex(cam.getPosition(), rayWorld, torusMesh, modelMat);
                if (sel != -1) {
                    torusPickedVertex = sel;
                    std::cout << "[Pick] Vertex " << sel << " selected on Torus" << std::endl;
                    torusDistances = torusMesh.fastMarching(sel);
                    torusMaxDist = 0.0f;
                    for (float d : torusDistances)
                        if (d < 990000.0f && d > torusMaxDist) torusMaxDist = d;

                    // Update distance VBO
                    torusVAO.bind();
                    torusDistVBO = VBO(torusDistances.data(), torusDistances.size() * sizeof(float));
                    torusVAO.linkAttrib(torusDistVBO, 3, 1, GL_FLOAT, sizeof(float), (void*)0);
                    torusVAO.unbind();
                    renderMode = 1;
                }
            }
        }

        // ----- ImGui Frame -----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ----- Shadow Pass -----
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(modelRotationX), glm::vec3(1, 0, 0));
        modelMat = glm::rotate(modelMat, glm::radians(modelRotationY), glm::vec3(0, 1, 0));

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use();
        depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        depthShader.setMat4("model", modelMat);
        if (activeScene == 0) {
            model->draw(depthShader);
        } else {
            torusVAO.bind();
            glDrawElements(GL_TRIANGLES, torusMesh.V.size(), GL_UNSIGNED_INT, 0);
            torusVAO.unbind();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ----- Main Render Pass -----
        int scrW, scrH;
        glfwGetFramebufferSize(window.glfwWindow, &scrW, &scrH);
        glViewport(0, 0, scrW, scrH);
        glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainShader.use();
        mainShader.setMat4("projection", cam.getProjectionMatrix(scrW, scrH));
        mainShader.setMat4("view", cam.getViewMatrix());
        mainShader.setMat4("model", modelMat);
        mainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        mainShader.setVec3("lightPos", lightPos);
        mainShader.setVec3("viewPos", cam.getPosition());
        mainShader.setVec3("lightColor", lightColor);

        mainShader.setInt("renderMode", renderMode);
        mainShader.setFloat("maxDistance", activeScene == 0 ? globalMaxDist : torusMaxDist);

        mainShader.setInt("diffuseMap", 0);
        mainShader.setInt("specularMap", 1);
        mainShader.setInt("shadowMap", 3);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTex);

        if (activeScene == 0) {
            model->draw(mainShader);
        } else {
            mainShader.setBool("useDiffuseMap", false);
            mainShader.setVec3("baseDiffuse", glm::vec3(0.7f, 0.75f, 0.8f));
            mainShader.setBool("useSpecularMap", false);
            mainShader.setVec3("baseSpecular", glm::vec3(0.3f, 0.3f, 0.3f));
            mainShader.setBool("useNormalMap", false);
            torusVAO.bind();
            glDrawElements(GL_TRIANGLES, torusMesh.V.size(), GL_UNSIGNED_INT, 0);
            torusVAO.unbind();
        }

        // =====================================================================
        // GUI
        // =====================================================================
        ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);

        ImGui::Begin("Engine Control Panel");

        // Scene Selector & Model Loader
        if (ImGui::CollapsingHeader("Scene & Model Loader", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::RadioButton("Loaded OBJ Model", &activeScene, 0);
            ImGui::RadioButton("Procedural Torus", &activeScene, 1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextDisabled("Current: %s", loadedModelName.c_str());
            ImGui::InputText("OBJ Path", modelPathBuf, sizeof(modelPathBuf));
            if (ImGui::Button("Load Model")) {
                reloadModel(std::string(modelPathBuf), "Custom Model");
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Quick Load:");

            float btnW = (ImGui::GetContentRegionAvail().x - 8) / 2.0f;
            if (ImGui::Button("Stanford Bunny", ImVec2(btnW, 0))) {
                reloadModel("assets/models/classic/bunny.obj", "Stanford Bunny");
            }
            ImGui::SameLine();
            if (ImGui::Button("Utah Teapot", ImVec2(btnW, 0))) {
                reloadModel("assets/models/classic/teapot.obj", "Utah Teapot");
            }
            if (ImGui::Button("Suzanne", ImVec2(btnW, 0))) {
                reloadModel("assets/models/classic/suzanne.obj", "Suzanne (Monkey)");
            }
            ImGui::SameLine();
            if (ImGui::Button("Thesis Corner", ImVec2(btnW, 0))) {
                reloadModel("assets/models/thesis/03_corner.obj", "Thesis Building");
            }
        }

        ImGui::Spacing();

        // Visualization Modes
        if (ImGui::CollapsingHeader("Diagnostic Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::RadioButton("Textured / Phong", &renderMode, 0);
            ImGui::RadioButton("Distance Heatmap (FMM)", &renderMode, 1);
            ImGui::RadioButton("Surface Normals", &renderMode, 2);
            ImGui::RadioButton("Shadow Depth Map", &renderMode, 3);
        }

        ImGui::Spacing();

        // Geometry Processing
        if (ImGui::CollapsingHeader("Geometry Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextWrapped("CTRL + Left Click on the model to pick a vertex (FMM seed).");
            ImGui::Spacing();

            int currentPick = (activeScene == 0) ? pickedVertex : torusPickedVertex;
            if (currentPick != -1)
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Picked Vertex: %d", currentPick);
            else
                ImGui::TextDisabled("No vertex selected.");

            ImGui::Separator();
            ImGui::Spacing();

            if (activeScene == 0) {
                ImGui::SliderInt("Target Tris", &simplifyTarget, 50, 10000);
                if (ImGui::Button("Simplify Mesh (QEM LOD)")) {
                    if (!model->meshes.empty()) {
                        std::cout << "[QEM] Simplifying to " << simplifyTarget << " triangles..." << std::endl;
                        model->meshes[0]->geometry.simplifyMesh(simplifyTarget);
                        model->meshes[0]->geometry.recalculateNormals();
                        model->meshes[0]->setup();
                        std::cout << "[QEM] Done." << std::endl;
                    }
                }
            }
        }

        ImGui::Spacing();

        // Animation
        if (ImGui::CollapsingHeader("Animation & Timeline", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (isPaused) {
                if (ImGui::Button("Play")) isPaused = false;
            } else {
                if (ImGui::Button("Pause")) isPaused = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%.1f FPS", io.Framerate);

            ImGui::Checkbox("Orbit Light", &animateLight);
            ImGui::Checkbox("Rotate Model", &animateModel);
        }

        ImGui::Spacing();

        // Lighting
        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat3("Light Position", glm::value_ptr(lightPos), -10.0f, 10.0f);
            ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        }

        ImGui::Spacing();

        // Controls Reference
        if (ImGui::CollapsingHeader("Controls")) {
            ImGui::BulletText("Left Drag: Orbit camera");
            ImGui::BulletText("Right Drag / Shift+Left: Pan");
            ImGui::BulletText("Scroll: Zoom");
            ImGui::BulletText("CTRL + Left Click: Vertex Pick");
            ImGui::BulletText("ESC: Quit");
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.update();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete model;

    return 0;
}
