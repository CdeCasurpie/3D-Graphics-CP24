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

void printInstructions() {
    std::cout << "\n=======================================================\n";
    std::cout << "          ARCBALL CAMERA CON CUATERNIONES (UTEC)        \n";
    std::cout << "=======================================================\n";
    std::cout << " [Ratón]\n";
    std::cout << "  - Clic Izquierdo + Arrastrar : Rotación Arcball 3D\n";
    std::cout << "  - Rueda del Ratón (Scroll)   : Zoom In / Zoom Out\n";
    std::cout << "  - Clic Derecho (o Shift+Izq) : Paneo (Mover target)\n";
    std::cout << "\n [Teclado]\n";
    std::cout << "  - ESPACIO o tecla 'C'        : Alternar Cámara Animada (Task 07) / Manual\n";
    std::cout << "  - Tecla 'R'                  : Reiniciar Cámara\n";
    std::cout << "  - Tecla 'ESC'                : Salir\n";
    std::cout << "=======================================================\n\n";
}

int main() {
    // 1. Inicialización de Ventana y Shader
    Window window(800, 800, "Arcball Camera (Quaternions)");
    Shader shader("src/ArcballCamera/shader.vert", "src/ArcballCamera/shader.frag");

    printInstructions();

    // 2. Instancia de la clase Camera (Requisito: struct/class named camera/Camera)
    Camera camera(4.0f, glm::vec3(0.0f));
    camera.registerCallbacks(window.glfwWindow);

    // 3. Crear geometrías para la escena animada (Cubo central + Esfera orbitando)
    EricStructure cubeMesh;
    cubeMesh.generateCube(1.2f);

    EricStructure sphereMesh;
    sphereMesh.generateSphere(0.35f, 32, 32);

    // Configuración OpenGL para el Cubo
    VAO vaoCube; vaoCube.bind();
    VBO vboCube((float*)cubeMesh.G.data(), cubeMesh.G.size() * sizeof(VertexData));
    EBO eboCube(cubeMesh.V);
    vaoCube.linkAttrib(vboCube, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoCube.linkAttrib(vboCube, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoCube.unbind();

    // Configuración OpenGL para la Esfera
    VAO vaoSphere; vaoSphere.bind();
    VBO vboSphere((float*)sphereMesh.G.data(), sphereMesh.G.size() * sizeof(VertexData));
    EBO eboSphere(sphereMesh.V);
    vaoSphere.linkAttrib(vboSphere, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoSphere.linkAttrib(vboSphere, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoSphere.unbind();

    // Habilitar prueba de profundidad
    glEnable(GL_DEPTH_TEST);

    float lastFrameTime = (float)glfwGetTime();

    // 4. Bucle principal de renderizado
    while (!window.shouldClose()) {
        window.processInput();

        // Cálculo de delta time para animaciones suaves
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Actualizar la cámara (avanza la órbita automática si isAnimated == true)
        camera.update(deltaTime);

        // Limpiar pantalla
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Obtener tamaño real de framebuffer (compatible con Hyprland y Tiling WM)
        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);

        // Obtener matrices View y Projection directamente de la Cámara
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(width, height);
        glm::vec3 camPos = camera.getPosition();

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader.ID, "viewPos"), 1, glm::value_ptr(camPos));

        // Luz estática en el mundo
        glm::vec3 lightPos(3.0f, 4.0f, 3.0f);
        glUniform3fv(glGetUniformLocation(shader.ID, "lightPos"), 1, glm::value_ptr(lightPos));

        // -------------------------------------------------------------
        // Renderizar el Cubo Central (rotando suavemente sobre sí mismo)
        // -------------------------------------------------------------
        glm::mat4 modelCube = glm::mat4(1.0f);
        modelCube = glm::rotate(modelCube, currentFrameTime * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelCube));

        glm::vec3 cubeColor(0.2f, 0.7f, 0.9f); // Azul cian elegante
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(cubeColor));
        glUniform1i(glGetUniformLocation(shader.ID, "isWireframe"), 0);

        vaoCube.bind();
        glDrawElements(GL_TRIANGLES, cubeMesh.V.size(), GL_UNSIGNED_INT, 0);

        // -------------------------------------------------------------
        // Renderizar la Esfera Orbitando alrededor del cubo
        // -------------------------------------------------------------
        float orbitRadius = 2.0f;
        float sphereX = std::cos(currentFrameTime * 1.5f) * orbitRadius;
        float sphereZ = std::sin(currentFrameTime * 1.5f) * orbitRadius;
        float sphereY = std::sin(currentFrameTime * 3.0f) * 0.4f;

        glm::mat4 modelSphere = glm::mat4(1.0f);
        modelSphere = glm::translate(modelSphere, glm::vec3(sphereX, sphereY, sphereZ));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelSphere));

        glm::vec3 sphereColor(1.0f, 0.4f, 0.2f); // Naranja cálido
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(sphereColor));

        vaoSphere.bind();
        glDrawElements(GL_TRIANGLES, sphereMesh.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    return 0;
}
