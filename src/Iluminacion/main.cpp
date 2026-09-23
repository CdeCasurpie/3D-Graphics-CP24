#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

// Modos de Shading y de Iluminación
enum ShadingMode {
    GOURAUD_SHADING = 0, // Per-Vertex Lighting
    PHONG_SHADING   = 1  // Per-Fragment / Per-Pixel Lighting
};

enum IlluminationModel {
    MODEL_PHONG       = 0, // (R · V)^alpha
    MODEL_BLINN_PHONG = 1  // (N · H)^beta
};

enum DisplayObject {
    OBJ_SPHERE = 0,
    OBJ_TORUS  = 1,
    OBJ_CUBE   = 2
};

// Estado global de la aplicación
ShadingMode currentShading = PHONG_SHADING;
IlluminationModel currentModel = MODEL_BLINN_PHONG;
DisplayObject currentObject = OBJ_SPHERE;
float shininess = 32.0f;
bool animateLight = true;

// Helper para detección de pulsación única (debounce)
bool checkKeyPress(GLFWwindow* window, int key, bool& state) {
    if (glfwGetKey(window, key) == GLFW_PRESS) {
        if (!state) {
            state = true;
            return true;
        }
    } else {
        state = false;
    }
    return false;
}

void printStatus() {
    std::cout << "\n=======================================================\n";
    std::cout << " [ESTADO ACTUAL]\n";
    std::cout << "  - Shading Mode        : " << (currentShading == GOURAUD_SHADING ? "GOURAUD (Per-Vertex)" : "PHONG (Per-Fragment)") << "\n";
    std::cout << "  - Illumination Model  : " << (currentModel == MODEL_PHONG ? "PHONG (R · V)" : "BLINN-PHONG (N · H)") << "\n";
    std::cout << "  - Shininess Exponent  : " << shininess << "\n";
    std::cout << "  - Objeto              : " << (currentObject == OBJ_SPHERE ? "Esfera" : (currentObject == OBJ_TORUS ? "Toroide" : "Cubo")) << "\n";
    std::cout << "  - Movimiento de Luz   : " << (animateLight ? "Animada (Orbitando)" : "Fija") << "\n";
    std::cout << "=======================================================\n";
}

void printInstructions() {
    std::cout << "\n=======================================================\n";
    std::cout << "   TAREA 9: PHONG & BLINN-PHONG (GOURAUD VS PHONG)     \n";
    std::cout << "                 3D Graphics - UTEC                    \n";
    std::cout << "=======================================================\n";
    std::cout << " [Controles de Shading e Iluminación]\n";
    std::cout << "  - Tecla '1' o 'G' : Alternar Gouraud Shading vs Phong Shading\n";
    std::cout << "  - Tecla '2' o 'B' : Alternar Phong Model vs Blinn-Phong Model\n";
    std::cout << "  - Tecla 'O'       : Cambiar Objeto (Esfera -> Toroide -> Cubo)\n";
    std::cout << "  - Tecla 'L'       : Pausar / Reanudar órbita de la luz\n";
    std::cout << "  - FLECHA ARRIBA   : Aumentar exponente especular (Shininess * 2)\n";
    std::cout << "  - FLECHA ABAJO    : Reducir exponente especular (Shininess / 2)\n";
    std::cout << "\n [Cámara Arcball con Cuaterniones]\n";
    std::cout << "  - Clic Izquierdo + Arrastrar : Rotar vista en 3D\n";
    std::cout << "  - Scroll del Ratón           : Zoom In / Zoom Out\n";
    std::cout << "  - Clic Derecho + Arrastrar   : Paneo de cámara\n";
    std::cout << "  - Tecla 'R'                  : Reiniciar cámara\n";
    std::cout << "  - Tecla 'ESC'                : Salir\n";
    std::cout << "=======================================================\n";
    printStatus();
}

int main() {
    // 1. Crear ventana
    Window window(800, 800, "Task 09: Illumination & Shading Comparison");

    // 2. Compilar Shaders
    Shader gouraudShader("src/Iluminacion/gouraud.vert", "src/Iluminacion/gouraud.frag");
    Shader phongShader("src/Iluminacion/phong.vert", "src/Iluminacion/phong.frag");
    Shader lightShader("src/Iluminacion/light.vert", "src/Iluminacion/light.frag");

    // 3. Inicializar Cámara Arcball (del framework)
    Camera camera(3.5f, glm::vec3(0.0f));
    camera.registerCallbacks(window.glfwWindow);

    printInstructions();

    // 4. Generar geometrías
    // Esfera con teselación media para apreciar claramente Gouraud vs Phong
    EricStructure sphereMesh;
    sphereMesh.generateSphere(1.0f, 28, 28);

    EricStructure torusMesh;
    torusMesh.generateTorus(1.0f, 0.4f, 40, 40, false);

    EricStructure cubeMesh;
    cubeMesh.generateCube(1.2f);

    // Pequeña esfera para representar la fuente de luz
    EricStructure lightMarker;
    lightMarker.generateSphere(0.08f, 16, 16);

    // Buffers Esfera
    VAO vaoSphere; vaoSphere.bind();
    VBO vboSphere((float*)sphereMesh.G.data(), sphereMesh.G.size() * sizeof(VertexData));
    EBO eboSphere(sphereMesh.V);
    vaoSphere.linkAttrib(vboSphere, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoSphere.linkAttrib(vboSphere, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoSphere.unbind();

    // Buffers Toroide
    VAO vaoTorus; vaoTorus.bind();
    VBO vboTorus((float*)torusMesh.G.data(), torusMesh.G.size() * sizeof(VertexData));
    EBO eboTorus(torusMesh.V);
    vaoTorus.linkAttrib(vboTorus, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoTorus.linkAttrib(vboTorus, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoTorus.unbind();

    // Buffers Cubo
    VAO vaoCube; vaoCube.bind();
    VBO vboCube((float*)cubeMesh.G.data(), cubeMesh.G.size() * sizeof(VertexData));
    EBO eboCube(cubeMesh.V);
    vaoCube.linkAttrib(vboCube, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoCube.linkAttrib(vboCube, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoCube.unbind();

    // Buffers Marcador de Luz
    VAO vaoLight; vaoLight.bind();
    VBO vboLight((float*)lightMarker.G.data(), lightMarker.G.size() * sizeof(VertexData));
    EBO eboLight(lightMarker.V);
    vaoLight.linkAttrib(vboLight, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoLight.unbind();

    glEnable(GL_DEPTH_TEST);

    // Estados para debouncing de teclas
    bool k1 = false, k2 = false, kG = false, kB = false, kO = false, kL = false, kUp = false, kDown = false;

    float lightAngle = 0.0f;
    float lastTime = (float)glfwGetTime();

    while (!window.shouldClose()) {
        window.processInput();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // -------------------------------------------------------------
        // Procesamiento de Entradas de Teclado (Cambio dinámico)
        // -------------------------------------------------------------
        if (checkKeyPress(window.glfwWindow, GLFW_KEY_1, k1) || checkKeyPress(window.glfwWindow, GLFW_KEY_G, kG)) {
            currentShading = (currentShading == GOURAUD_SHADING) ? PHONG_SHADING : GOURAUD_SHADING;
            printStatus();
        }

        if (checkKeyPress(window.glfwWindow, GLFW_KEY_2, k2) || checkKeyPress(window.glfwWindow, GLFW_KEY_B, kB)) {
            currentModel = (currentModel == MODEL_PHONG) ? MODEL_BLINN_PHONG : MODEL_PHONG;
            printStatus();
        }

        if (checkKeyPress(window.glfwWindow, GLFW_KEY_O, kO)) {
            currentObject = (DisplayObject)((currentObject + 1) % 3);
            printStatus();
        }

        if (checkKeyPress(window.glfwWindow, GLFW_KEY_L, kL)) {
            animateLight = !animateLight;
            printStatus();
        }

        if (checkKeyPress(window.glfwWindow, GLFW_KEY_UP, kUp)) {
            shininess = std::min(512.0f, shininess * 2.0f);
            printStatus();
        }

        if (checkKeyPress(window.glfwWindow, GLFW_KEY_DOWN, kDown)) {
            shininess = std::max(2.0f, shininess / 2.0f);
            printStatus();
        }

        // Posición de la luz (órbita dinámica en el plano XZ)
        if (animateLight) {
            lightAngle += deltaTime * 1.2f;
        }
        float lightRadius = 2.5f;
        glm::vec3 lightPos(std::sin(lightAngle) * lightRadius, 1.8f, std::cos(lightAngle) * lightRadius);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // Luz blanca brillante
        glm::vec3 objectColor(0.85f, 0.25f, 0.25f); // Rojo carmín para destacar el brillo especular

        // Limpiar pantalla
        glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Actualizar cámara
        camera.update(deltaTime);

        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(width, height);
        glm::vec3 viewPos = camera.getPosition();

        // -------------------------------------------------------------
        // Seleccionar Shader Activo según el método de interpolación
        // -------------------------------------------------------------
        Shader& activeShader = (currentShading == GOURAUD_SHADING) ? gouraudShader : phongShader;
        activeShader.use();

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(activeShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(activeShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(activeShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(activeShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(activeShader.ID, "viewPos"), 1, glm::value_ptr(viewPos));
        glUniform3fv(glGetUniformLocation(activeShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(activeShader.ID, "objectColor"), 1, glm::value_ptr(objectColor));

        // Parámetros de iluminación: 0 = Phong, 1 = Blinn-Phong
        glUniform1i(glGetUniformLocation(activeShader.ID, "illuminationModel"), (int)currentModel);
        glUniform1f(glGetUniformLocation(activeShader.ID, "shininess"), shininess);

        // Dibujar objeto seleccionado
        if (currentObject == OBJ_SPHERE) {
            vaoSphere.bind();
            glDrawElements(GL_TRIANGLES, sphereMesh.V.size(), GL_UNSIGNED_INT, 0);
        } else if (currentObject == OBJ_TORUS) {
            vaoTorus.bind();
            glDrawElements(GL_TRIANGLES, torusMesh.V.size(), GL_UNSIGNED_INT, 0);
        } else {
            vaoCube.bind();
            glDrawElements(GL_TRIANGLES, cubeMesh.V.size(), GL_UNSIGNED_INT, 0);
        }

        // -------------------------------------------------------------
        // Renderizar Marcador de la Fuente de Luz (Esfera Amarilla)
        // -------------------------------------------------------------
        lightShader.use();
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(lightShader.ID, "lightColor"), 1.0f, 0.95f, 0.4f); // Amarillo suave

        vaoLight.bind();
        glDrawElements(GL_TRIANGLES, lightMarker.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    return 0;
}
