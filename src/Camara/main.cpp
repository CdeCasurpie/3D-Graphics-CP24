#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h" // Para usar generateCube

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main() {
    Window window(800, 800, "Ejercicio de Camara");
    Shader shader("src/Camara/shader.vert", "src/Camara/shader.frag");

    // 1. Generamos el cubito
    EricStructure cube;
    cube.generateCube(1.0f); // Genera un cubo en el origen
    
    // 2. VAO, VBO, EBO
    VAO vao; vao.bind();
    VBO vbo((float*) cube.G.data(), cube.G.size() * sizeof(VertexData));
    EBO ebo(cube.V);
    
    // El shader actual solo usa 'location = 0' (aPos) porque quitamos la iluminación
    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vao.unbind();

    // 3. Opciones de renderizado
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Dibujamos en wireframe para verlo mejor de frente

    // 4. Color del objeto
    glm::vec3 cubeColor = glm::vec3(0.0f, 1.0f, 0.0f); // Verde

    // --- AQUÍ PUEDES DECLARAR VARIABLES PARA TU CÁMARA LUEGO ---
    // glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    // glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    // glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    while(!window.shouldClose()) {
        window.processInput();
        // --- AQUÍ DEBES PROCESAR EL INPUT DE TECLADO PARA MOVER LA CÁMARA ---
        
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();


        // --- teclas para mover y girar camara ---
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += 0.05f * cameraFront;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= 0.05f * cameraFront;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.05f;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.05f;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_Q) == GLFW_PRESS)
            cameraPos += 0.05f * cameraUp;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_E) == GLFW_PRESS)
            cameraPos -= 0.05f * cameraUp;

        if (glfwGetKey(window.glfwWindow, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraFront = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-1.0f), cameraUp)) * cameraFront;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraFront = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), cameraUp)) * cameraFront;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraFront = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-1.0f), glm::normalize(glm::cross(cameraFront, cameraUp)))) * cameraFront;
        if (glfwGetKey(window.glfwWindow, GLFW_KEY_UP) == GLFW_PRESS)
            cameraFront = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), glm::normalize(glm::cross(cameraFront, cameraUp)))) * cameraFront;


        // --- MATRICES MVP ---
        
        // M: Modelo (Lo dejamos en el origen sin rotar ni escalar por ahora)
        glm::mat4 model = glm::mat4(1.0f);
        
        // V: Vista (Cámara estática por ahora)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); // ¡USAR LUEGO!

        // P: Proyección Ortográfica (De adelante hacia atrás como pediste)
        // (Izquierda, Derecha, Abajo, Arriba, Cerca, Lejos)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.width / (float)window.height, 0.1f, 100.0f);
        
        // En la proyección ortográfica inicial, alejamos el mundo hacia el fondo (-Z) para que caiga dentro del frustum (0.1 a 10.0)
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

        // --- ENVIAR DATOS A GPU ---
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(cubeColor));

        // --- DIBUJAR ---
        vao.bind();
        glDrawElements(GL_TRIANGLES, cube.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    vao.destroy(); vbo.destroy(); ebo.destroy();
    return 0;
}
