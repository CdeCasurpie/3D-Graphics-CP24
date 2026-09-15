#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

int main() {
    Window window(800, 800, "Task 06: Fast Marching Method");

    Shader shader("src/FastMarching/shader.vert", "src/FastMarching/shader.frag");

    EricStructure mesh;
    mesh.generateSphere(1.0f, 60, 60);

    std::cout << "Ejecutando Fast Marching Algorithm..." << std::endl;
    std::vector<float> distances = mesh.fastMarching(0); // Desde el vertice 0
    
    float maxDist = 0.0f;
    for (float d : distances) {
        if (d < 999000.0f && d > maxDist) maxDist = d;
    }
    std::cout << "Distancia geodesica maxima calculada: " << maxDist << std::endl;

    VAO vao; vao.bind();

    VBO vboGeom((float*) mesh.G.data(), mesh.G.size() * sizeof(VertexData));
    vao.linkAttrib(vboGeom, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vao.linkAttrib(vboGeom, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)(sizeof(Vec3)));

    VBO vboDist(distances.data(), distances.size() * sizeof(float));
    vao.linkAttrib(vboDist, 2, 1, GL_FLOAT, sizeof(float), (void*)0);

    EBO ebo(mesh.V);

    vao.unbind();
    vboGeom.unbind();
    vboDist.unbind();
    ebo.unbind();

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while (!window.shouldClose()) {
        window.processInput();
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        float time = glfwGetTime();
        float radius = 3.0f;
        float camX = sin(time * 0.5f) * radius;
        float camZ = cos(time * 0.5f) * radius;
        
        glm::mat4 view = glm::lookAt(glm::vec3(camX, 0.5f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform3f(glGetUniformLocation(shader.ID, "lightPos"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"), camX, 0.5f, camZ);
        glUniform1f(glGetUniformLocation(shader.ID, "maxDistance"), maxDist);

        vao.bind();
        glDrawElements(GL_TRIANGLES, mesh.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    return 0;
}
