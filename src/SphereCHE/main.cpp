#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"

void printControls() {
    std::cout << "\n============================================\n";
    std::cout << " CONTROLES DE LA APLICACION (CHE Structure)\n";
    std::cout << "============================================\n";
    std::cout << " GEOMETRIA:\n";
    std::cout << "  [C] -> Cargar Cubo (Test inicial CHE)\n";
    std::cout << "  [V] -> Cargar Esfera Parametrica\n";
    std::cout << "\n MODO DE RENDERIZADO:\n";
    std::cout << "  [1] -> Nube de Puntos (GL_POINTS)\n";
    std::cout << "  [2] -> Malla de Lineas / Wireframe (GL_LINE)\n";
    std::cout << "  [3] -> Solido con Iluminacion (GL_FILL)\n";
    std::cout << "\n ILUMINACION (Mover la luz):\n";
    std::cout << "  [W/S] -> Arriba / Abajo\n";
    std::cout << "  [A/D] -> Izquierda / Derecha\n";
    std::cout << "  [Q/E] -> Adentro / Afuera\n";
    std::cout << "============================================\n\n";
}

void processAppInput(Window& window, EricStructure& eric, VAO& vao, VBO*& vbo, EBO*& ebo, 
                     bool& isSphere, int& renderMode, float& lightX, float& lightY, float& lightZ) 
{
    window.processInput(); // Chequeo global (ESC para salir)
    
    // --- CONTROLES DE GEOMETRIA ---
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_C) == GLFW_PRESS && isSphere) {
        isSphere = false;
        eric.generateCube(0.8f);
        vao.bind();
        vbo->destroy(); ebo->destroy(); delete vbo; delete ebo;
        vbo = new VBO((float*)eric.G.data(), eric.G.size() * sizeof(VertexData));
        ebo = new EBO(eric.V);
        vao.linkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
        vao.linkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
        vao.unbind();
    }
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_V) == GLFW_PRESS && !isSphere) {
        isSphere = true;
        eric.generateSphere(0.5f, 36, 18);
        vao.bind();
        vbo->destroy(); ebo->destroy(); delete vbo; delete ebo;
        vbo = new VBO((float*)eric.G.data(), eric.G.size() * sizeof(VertexData));
        ebo = new EBO(eric.V);
        vao.linkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
        vao.linkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
        vao.unbind();
    }

    // --- CONTROLES DE RENDERIZADO ---
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_1) == GLFW_PRESS) renderMode = 1;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_2) == GLFW_PRESS) renderMode = 2;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_3) == GLFW_PRESS) renderMode = 3;

    if (renderMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    else if (renderMode == 2) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- CONTROLES DE LUZ ---
    float lightSpeed = 0.05f;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_W) == GLFW_PRESS) lightY += lightSpeed;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_S) == GLFW_PRESS) lightY -= lightSpeed;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_A) == GLFW_PRESS) lightX -= lightSpeed;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_D) == GLFW_PRESS) lightX += lightSpeed;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_Q) == GLFW_PRESS) lightZ -= lightSpeed;
    if (glfwGetKey(window.glfwWindow, GLFW_KEY_E) == GLFW_PRESS) lightZ += lightSpeed;
}

int main() {
    Window window(800, 800, "EricStructure (CHE) - Tarea 3D");
    Shader shader("src/SphereCHE/shader.vert", "src/SphereCHE/shader.frag");

    printControls();

    EricStructure eric;
    eric.generateSphere(0.5f, 36, 18); // Default a esfera

    VAO vao;
    vao.bind();
    VBO* vbo = new VBO((float*)eric.G.data(), eric.G.size() * sizeof(VertexData));
    EBO* ebo = new EBO(eric.V); 
    vao.linkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vao.linkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vao.unbind();

    float lightX = 1.0f, lightY = 1.0f, lightZ = 1.0f;
    int renderMode = 3; // 1: Points, 2: Wireframe, 3: Solid
    bool isSphere = true;

    while (!window.shouldClose()) {
        processAppInput(window, eric, vao, vbo, ebo, isSphere, renderMode, lightX, lightY, lightZ);
        
        window.clear(0.1f, 0.1f, 0.1f, 1.0f);

        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID, "lightPos"), lightX, lightY, lightZ);
        glUniform3f(glGetUniformLocation(shader.ID, "lightColor"), 1.0f, 1.0f, 1.0f);
        
        if (isSphere) glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 1.0f, 0.0f, 0.0f);
        else glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.0f, 1.0f, 0.0f);
        
        glEnable(GL_PROGRAM_POINT_SIZE);
        glPointSize(4.0f); // Para ver los puntos en el modo 1

        vao.bind();
        if (renderMode == 1) {
            glDrawArrays(GL_POINTS, 0, eric.G.size());
        } else {
            glDrawElements(GL_TRIANGLES, eric.V.size(), GL_UNSIGNED_INT, 0);
        }

        window.update();
    }

    vao.destroy();
    vbo->destroy(); delete vbo;
    ebo->destroy(); delete ebo;

    return 0;
}
