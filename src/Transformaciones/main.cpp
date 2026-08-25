#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"

// GLM incluye para las matemáticas de las matrices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main() {
    Window window(800, 800, "Cubo Rotando con 4 Luces (GLM)");
    Shader shader("src/Transformaciones/shader.vert", "src/Transformaciones/shader.frag");

    // Usamos nuestra estructura CHE para generar un cubo
    EricStructure eric;
    eric.generateCube(1.0f); // Cubo de tamaño 1x1x1

    VAO vao;
    vao.bind();
    VBO vbo((float*)eric.G.data(), eric.G.size() * sizeof(VertexData));
    EBO ebo(eric.V);
    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vao.linkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vao.unbind();

    // Configuración de las 4 Luces
    glm::vec3 lightPositions[] = {
        glm::vec3( 2.0f,  2.0f,  2.0f), // Luz 1: Arriba-derecha-frente
        glm::vec3(-2.0f, -2.0f, -2.0f), // Luz 2: Abajo-izquierda-atrás
        glm::vec3(-2.0f,  2.0f,  2.0f), // Luz 3: Arriba-izquierda-frente
        glm::vec3( 2.0f, -2.0f, -2.0f)  // Luz 4: Abajo-derecha-atrás
    };

    glm::vec3 lightColors[] = {
        glm::vec3(1.0f, 0.0f, 0.0f), // Luz Roja
        glm::vec3(0.0f, 1.0f, 0.0f), // Luz Verde
        glm::vec3(0.0f, 0.0f, 1.0f), // Luz Azul
        glm::vec3(1.0f, 1.0f, 1.0f)  // Luz Blanca
    };

    // Color base del cubo (Gris para que los colores de las luces se noten mejor)
    glm::vec3 objectColor = glm::vec3(0.7f, 0.7f, 0.7f);

    glEnable(GL_DEPTH_TEST); // ¡Importante para 3D real! Evita que caras traseras tapen las delanteras
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while (!window.shouldClose()) {
        window.processInput();
        
        // Limpiamos la pantalla y el buffer de profundidad
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // --- MATRICES MVP ---
        
        // 1. Matriz de Proyección (Perspectiva)
        // 45 grados de FOV, aspect ratio 1:1, clipping desde 0.1 hasta 100.0
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.width / (float)window.height, 0.1f, 100.0f);
        
        // 2. Matriz de Vista (Cámara)
        // Posicionamos la cámara un poco arriba y atrás (0, 2, 5) mirando al centro (0, 0, 0)
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.0f, 5.0f), 
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        
        // 3. Matriz de Modelo (Rotación constante)
        glm::mat4 model = glm::mat4(1.0f); // Matriz Identidad
        float timeValue = glfwGetTime();
        // Rotamos en el eje X y en el eje Y
        model = glm::rotate(model, timeValue * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, timeValue * 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));

        // --- ENVIAR DATOS A LA GPU ---
        
        // Enviamos Matrices
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // Enviamos el color del objeto
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(objectColor));

        // Enviamos las 4 luces (arrays)
        for (int i = 0; i < 4; i++) {
            std::string posName = "lightPositions[" + std::to_string(i) + "]";
            std::string colName = "lightColors[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader.ID, posName.c_str()), 1, glm::value_ptr(lightPositions[i]));
            glUniform3fv(glGetUniformLocation(shader.ID, colName.c_str()), 1, glm::value_ptr(lightColors[i]));
        }

        // --- RENDERIZADO ---
        vao.bind();
        glDrawElements(GL_TRIANGLES, eric.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    vao.destroy();
    vbo.destroy();
    ebo.destroy();

    return 0;
}
