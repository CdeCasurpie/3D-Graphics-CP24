#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

#include "Window.h"
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
#include <iostream>
#include <vector>

// --- Vertex Picking Logic ---
int getClosestVertex(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const EricStructure& mesh, const glm::mat4& modelMatrix) {
    int bestVertex = -1;
    float bestDist = 1e9f;
    float minRayDist = 0.1f; // click precision threshold

    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
    glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

    for (size_t i = 0; i < mesh.G.size(); ++i) {
        bool isActive = false;
        for (unsigned int v : mesh.V) {
            if (v == i) { isActive = true; break; }
        }
        if (!isActive) continue;

        glm::vec3 pos = glm::vec3(mesh.G[i].Position.x, mesh.G[i].Position.y, mesh.G[i].Position.z);
        glm::vec3 vToP = pos - localOrigin;
        
        float t = glm::dot(vToP, localDir);
        if (t < 0.0f) continue;

        glm::vec3 projPos = localOrigin + localDir * t;
        float dist = glm::length(pos - projPos);

        if (dist < minRayDist && t < bestDist) {
            bestDist = t;
            bestVertex = i;
        }
    }
    return bestVertex;
}

Camera cam;
bool performVertexPick = false;
double pickX = 0, pickY = 0;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (mods & GLFW_MOD_SHIFT) {
                cam.startPanning(x, y);
            } else if (mods & GLFW_MOD_CONTROL) {
                performVertexPick = true;
                pickX = x; pickY = y;
            } else {
                cam.startDragging(x, y, width, height);
            }
        } else if (action == GLFW_RELEASE) {
            cam.stopDragging();
            cam.stopPanning();
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            cam.startPanning(x, y);
        } else if (action == GLFW_RELEASE) {
            cam.stopPanning();
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    if (cam.isDragging) {
        cam.onMouseDrag(xpos, ypos, width, height);
    } else if (cam.isPanning) {
        cam.onMousePan(xpos, ypos, width, height);
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    cam.onScroll(yoffset);
}

int main() {
    Window window(1280, 720, "Project 1: Interactive 3D Scene Viewer & Engine");
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glfwSetMouseButtonCallback(window.glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(window.glfwWindow, cursorPosCallback);
    glfwSetScrollCallback(window.glfwWindow, scrollCallback);

    cam.reset(6.0f);

    Shader mainShader("src/Project1/shaders/main.vert", "src/Project1/shaders/main.frag");
    Shader depthShader("src/Project1/shaders/depth.vert", "src/Project1/shaders/depth.frag");

    // Load Default Model
    Model* model = new Model("assets/models/thesis/03_corner.obj");

    // Shadow Map Setup
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBO, depthMap;
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // State Variables
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    int renderMode = 0; // 0=Norm, 1=Heatmap, 2=Normals, 3=Depth
    bool isAnimatingLight = false;
    bool isAnimatingModel = false;
    float modelAngle = -90.0f; // Fix Z-up models initially
    
    int simplifyTarget = 1000;
    float globalMaxDist = 1.0f;
    int pickedVertex = -1;

    glEnable(GL_DEPTH_TEST);
    
    while (!window.shouldClose()) {
        window.processInput();

        float time = glfwGetTime();
        if (isAnimatingLight) {
            lightPos.x = sin(time) * 5.0f;
            lightPos.z = cos(time) * 5.0f;
        }
        if (isAnimatingModel) {
            modelAngle += 30.0f * 0.016f; // approx delta time
        }

        // Raycasting Logic
        if (performVertexPick) {
            performVertexPick = false;
            int width, height;
            glfwGetFramebufferSize(window.glfwWindow, &width, &height);

            // Normalized Device Coordinates
            float x = (2.0f * pickX) / width - 1.0f;
            float y = 1.0f - (2.0f * pickY) / height;
            
            glm::vec4 rayClip(x, y, -1.0f, 1.0f);
            glm::mat4 proj = cam.getProjectionMatrix(width, height);
            glm::vec4 rayEye = glm::inverse(proj) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
            
            glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(cam.getViewMatrix()) * rayEye));
            
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::rotate(modelMat, glm::radians(modelAngle), glm::vec3(1.0f, 0.0f, 0.0f));

            if (!model->meshes.empty()) {
                int selected = getClosestVertex(cam.getPosition(), rayWorld, model->meshes[0]->geometry, modelMat);
                if (selected != -1) {
                    pickedVertex = selected;
                    std::vector<float> dists = model->meshes[0]->geometry.fastMarching(selected);
                    model->meshes[0]->distances = dists;
                    model->meshes[0]->setup(); // update VBO
                    
                    globalMaxDist = 0.0f;
                    for (float d : dists) {
                        if (d < 990000.0f && d > globalMaxDist) globalMaxDist = d;
                    }
                    renderMode = 1; // switch to heatmap
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Shadow Pass
        glm::mat4 lightProjection, lightView, lightSpaceMatrix;
        float near_plane = 1.0f, far_plane = 20.0f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(modelAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        
        depthShader.use();
        depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        depthShader.setMat4("model", modelMat);
        model->draw(depthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. Normal Render Pass
        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainShader.use();
        mainShader.setMat4("projection", cam.getProjectionMatrix(width, height));
        mainShader.setMat4("view", cam.getViewMatrix());
        mainShader.setMat4("model", modelMat);
        mainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        
        mainShader.setVec3("lightPos", lightPos);
        mainShader.setVec3("viewPos", cam.getPosition());
        mainShader.setVec3("lightColor", lightColor);
        
        mainShader.setInt("renderMode", renderMode);
        mainShader.setFloat("maxDistance", globalMaxDist);

        mainShader.setInt("diffuseMap", 0);
        mainShader.setInt("specularMap", 1);
        mainShader.setInt("shadowMap", 3); // Binding 3
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        
        model->draw(mainShader);

        // --- GUI ---
        ImGui::Begin("Project 1 Engine Control");
        
        if (ImGui::CollapsingHeader("Diagnostic Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::RadioButton("Textured/Lighting", &renderMode, 0);
            ImGui::RadioButton("Distance Heatmap", &renderMode, 1);
            ImGui::RadioButton("Surface Normals", &renderMode, 2);
            ImGui::RadioButton("Shadow Depth", &renderMode, 3);
        }

        if (ImGui::CollapsingHeader("Geometry Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Hold CTRL + Left Click on model to Pick a Vertex");
            if (pickedVertex != -1) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Picked Vertex ID: %d", pickedVertex);
            } else {
                ImGui::Text("No vertex selected.");
            }
            ImGui::Separator();
            ImGui::SliderInt("Target Triangles", &simplifyTarget, 10, 10000);
            if (ImGui::Button("Simplify Mesh (QEM LOD)")) {
                if (!model->meshes.empty()) {
                    model->meshes[0]->geometry.simplifyMesh(simplifyTarget);
                    model->meshes[0]->setup();
                }
            }
        }

        if (ImGui::CollapsingHeader("Animation & Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Animate Light", &isAnimatingLight);
            ImGui::Checkbox("Animate Model", &isAnimatingModel);
            ImGui::SliderFloat3("Light Pos", glm::value_ptr(lightPos), -10.0f, 10.0f);
            ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.update();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete model;

    return 0;
}
