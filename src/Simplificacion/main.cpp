#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main() {
    // Ventana rectangular ancha para ver las dos esferas lado a lado
    Window window(1200, 600, "Algoritmo de Simplificacion (LOD)");
    Shader shader("src/Simplificacion/shader.vert", "src/Simplificacion/shader.frag");

    // 1. Esfera Original (Alta resolución)
    EricStructure originalSphere;
    originalSphere.generateSphere(1.0f, 40, 40);

    // 2. Esfera Simplificada
    EricStructure simplifiedSphere;
    simplifiedSphere.generateSphere(1.0f, 40, 40);
    
    // AQUÍ ES DONDE APLICAREMOS LA SIMPLIFICACIÓN
    // Llama a tu función para que deje la malla, por ejemplo, en 500 triángulos.
    // simplifiedSphere.simplifyMesh(500); 

    // --- VAO, VBO, EBO para la Esfera Original ---
    VAO vaoOriginal; vaoOriginal.bind();
    VBO vboOriginal((float*) originalSphere.G.data(), originalSphere.G.size() * sizeof(VertexData));
    EBO eboOriginal(originalSphere.V);
    vaoOriginal.linkAttrib(vboOriginal, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoOriginal.linkAttrib(vboOriginal, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoOriginal.unbind();

    // --- VAO, VBO, EBO para la Esfera Simplificada ---
    VAO vaoSimplified; vaoSimplified.bind();
    VBO vboSimplified((float*) simplifiedSphere.G.data(), simplifiedSphere.G.size() * sizeof(VertexData));
    EBO eboSimplified(simplifiedSphere.V);
    vaoSimplified.linkAttrib(vboSimplified, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoSimplified.linkAttrib(vboSimplified, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoSimplified.unbind();

    glEnable(GL_DEPTH_TEST);
    // Para ver bien la reducción de polígonos, vamos a renderizar en Wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 

    // Dirección de la luz (Viene de arriba a la derecha y al fondo)
    glm::vec3 lightDir = glm::vec3(-1.0f, -1.0f, -1.0f);

    while(!window.shouldClose()) {
        window.processInput();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Fondo azul muy oscuro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // --- MATRICES DE CÁMARA (Fijas por ahora) ---
        // Aspect ratio ajustado a la ventana rectangular (1200 / 600 = 2.0f)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.width / (float)window.height, 0.1f, 100.0f);
        
        // Cámara alejada para ver ambas esferas
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), 
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shader.ID, "lightDir"), 1, glm::value_ptr(lightDir));

        // Para rotar ambas esferas y apreciarlas en 3D
        float timeValue = glfwGetTime();

        // ---------------- DIBUJAR ESFERA ORIGINAL (Izquierda) ----------------
        glm::mat4 modelOriginal = glm::mat4(1.0f);
        modelOriginal = glm::translate(modelOriginal, glm::vec3(-1.2f, 0.0f, 0.0f)); // Movida a la izquierda
        modelOriginal = glm::rotate(modelOriginal, timeValue * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelOriginal));
        // Gris claro para la original
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.8f, 0.8f, 0.8f);

        vaoOriginal.bind();
        glDrawElements(GL_TRIANGLES, originalSphere.V.size(), GL_UNSIGNED_INT, 0);


        // ---------------- DIBUJAR ESFERA SIMPLIFICADA (Derecha) ----------------
        glm::mat4 modelSimplified = glm::mat4(1.0f);
        modelSimplified = glm::translate(modelSimplified, glm::vec3(1.2f, 0.0f, 0.0f)); // Movida a la derecha
        modelSimplified = glm::rotate(modelSimplified, timeValue * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelSimplified));
        // Tono amarillento/naranja para la que simplificaremos
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 1.0f, 0.7f, 0.3f);

        vaoSimplified.bind();
        // A medida que simplifiquemos el arreglo V, el tamaño bajará
        glDrawElements(GL_TRIANGLES, simplifiedSphere.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    vaoOriginal.destroy(); vboOriginal.destroy(); eboOriginal.destroy();
    vaoSimplified.destroy(); vboSimplified.destroy(); eboSimplified.destroy();

    return 0;
}
