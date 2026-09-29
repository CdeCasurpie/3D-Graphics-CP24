#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "EricStructure.h"

// ImGui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

// --- Vertex Picking Logic ---
int getClosestVertex(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const EricStructure& mesh, const glm::mat4& modelMatrix) {
    int bestVertex = -1;
    float bestDist = 1e9f;
    float minRayDist = 0.05f; // click precision threshold

    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
    glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

    for (size_t i = 0; i < mesh.G.size(); ++i) {
        // Only consider valid vertices that are part of the mesh
        bool isActive = false;
        for (unsigned int v : mesh.V) {
            if (v == i) { isActive = true; break; }
        }
        if (!isActive) continue;

        glm::vec3 pos = glm::vec3(mesh.G[i].Position.x, mesh.G[i].Position.y, mesh.G[i].Position.z);
        glm::vec3 vToP = pos - localOrigin;
        
        // Project onto ray
        float t = glm::dot(vToP, localDir);
        if (t < 0.0f) continue; // Behind camera

        glm::vec3 projPos = localOrigin + localDir * t;
        float dist = glm::length(pos - projPos);

        if (dist < minRayDist && t < bestDist) {
            bestDist = t;
            bestVertex = i;
        }
    }
    return bestVertex;
}

// Global Camera
Camera cam;

// We will route GLFW callbacks manually to handle both ImGui and Camera
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
    Window window(1280, 720, "Project 1: 3D Scene Engine");
    
    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Overwrite ImGui's GLFW callbacks (we use our own that checks WantCaptureMouse)
    glfwSetMouseButtonCallback(window.glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(window.glfwWindow, cursorPosCallback);
    glfwSetScrollCallback(window.glfwWindow, scrollCallback);

    cam.reset(4.0f);

    // TODO: Setup Unified Shader, Geometry, FMM Data
    
    while (!window.shouldClose()) {
        window.processInput();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // GUI
        ImGui::Begin("Engine Controls");
        ImGui::Text("Hello Project 1!");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.update();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
