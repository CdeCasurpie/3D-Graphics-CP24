#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int main(int argc, char const *argv[])
{
    Window window(800, 800, "Cubo y Esfera Orbitando");
    Shader shader("src/Transformaciones/shader.vert", "src/Transformaciones/shader.frag");

    // Generamos DOS estructuras independientes
    EricStructure ericCube;
    ericCube.generateCube(1.0f);
    
    EricStructure ericSphere;
    ericSphere.generateSphere(0.2f, 20, 20);

    // VAO, VBO, EBO para el CUBO
    VAO vaoCube; vaoCube.bind();
    VBO vboCube((float*) ericCube.G.data(), ericCube.G.size() * sizeof(VertexData));
    EBO eboCube(ericCube.V);
    vaoCube.linkAttrib(vboCube, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoCube.linkAttrib(vboCube, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoCube.unbind();

    // VAO, VBO, EBO para la ESFERA
    VAO vaoSphere; vaoSphere.bind();
    VBO vboSphere((float*) ericSphere.G.data(), ericSphere.G.size() * sizeof(VertexData));
    EBO eboSphere(ericSphere.V);
    vaoSphere.linkAttrib(vboSphere, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoSphere.linkAttrib(vboSphere, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoSphere.unbind();

    glm::vec3 lightPositions[] = {
        glm::vec3( 2.0f,  2.0f,  2.0f),
        glm::vec3(-2.0f, -2.0f, -2.0f),
        glm::vec3(-2.0f,  2.0f,  2.0f),
        glm::vec3( 2.0f, -2.0f, -2.0f)
    };

    glm::vec3 lightColors[] = {
        glm::vec3(0.7f, 0.7f, 0.7f),
        glm::vec3(0.7f, 0.7f, 0.7f),
        glm::vec3(0.7f, 0.7f, 0.7f),
        glm::vec3(0.7f, 0.7f, 0.7f)
    };

    glm::vec3 cubeColor = glm::vec3(0.7f, 0.7f, 0.7f); // gris para el cubo
    glm::vec3 sphereColor = glm::vec3(1.0f, 0.0f, 0.0f); // rojo para la esfera

    glEnable(GL_DEPTH_TEST); 
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while(!window.shouldClose()) {
        window.processInput();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        shader.use();

        // 1. Matrices comunes (View y Projection)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.width / (float)window.height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(4.0f, 3.0f, 3.0f), 
                                    glm::vec3(0.0f, 0.0f, 0.0f), 
                                    glm::vec3(0.0f, 1.0f, 0.0f)); 

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Enviamos las luces
        for (int i = 0; i < 4; i++) {
            std::string posName = "lightPositions[" + std::to_string(i) + "]";
            std::string colorName = "lightColors[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader.ID, posName.c_str()), 1, glm::value_ptr(lightPositions[i]));
            glUniform3fv(glGetUniformLocation(shader.ID, colorName.c_str()), 1, glm::value_ptr(lightColors[i]));
        }

        float timeValue = glfwGetTime();

        // ---------------- DIBUJAR CUBO ----------------
        glm::mat4 cubeModel = glm::mat4(1.0f); 
        cubeModel = glm::rotate(cubeModel, timeValue * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f)); 

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(cubeColor));

        vaoCube.bind();
        glDrawElements(GL_TRIANGLES, ericCube.V.size(), GL_UNSIGNED_INT, 0);


        // ---------------- DIBUJAR ESFERA ----------------
        glm::mat4 sphereModel = glm::mat4(1.0f); 
        // Para que la esfera orbite ALREDEDOR del origen, primero rotamos y LUEGO trasladamos
        sphereModel = glm::rotate(sphereModel, timeValue * glm::radians(100.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
        sphereModel = glm::translate(sphereModel, glm::vec3(2.0f, 0.0f, 0.0f)); 

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(sphereModel));
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, glm::value_ptr(sphereColor));

        vaoSphere.bind();
        glDrawElements(GL_TRIANGLES, ericSphere.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    vaoCube.destroy(); vboCube.destroy(); eboCube.destroy();
    vaoSphere.destroy(); vboSphere.destroy(); eboSphere.destroy();

    return 0;
}
