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
#include <vector>

// Generar lineas que apunten hacia donde se origino la onda (Backtracking/Gradiente)
std::vector<float> generateGradientLines(const EricStructure& mesh, const std::vector<float>& distances) {
    std::vector<float> lineVerts;
    for (size_t i = 0; i < mesh.V.size(); i += 3) {
        if (mesh.V[i] == (unsigned int)-1) continue;
        int v0 = mesh.V[i];
        int v1 = mesh.V[i+1];
        int v2 = mesh.V[i+2];

        glm::vec3 p0(mesh.G[v0].Position.x, mesh.G[v0].Position.y, mesh.G[v0].Position.z);
        glm::vec3 p1(mesh.G[v1].Position.x, mesh.G[v1].Position.y, mesh.G[v1].Position.z);
        glm::vec3 p2(mesh.G[v2].Position.x, mesh.G[v2].Position.y, mesh.G[v2].Position.z);

        float t0 = distances[v0];
        float t1 = distances[v1];
        float t2 = distances[v2];

        glm::vec3 e1 = p1 - p0;
        glm::vec3 e2 = p2 - p0;
        glm::vec3 n = glm::normalize(glm::cross(e1, e2));

        glm::mat3 M;
        M[0] = e1; M[1] = e2; M[2] = n;
        glm::vec3 d(t1 - t0, t2 - t0, 0.0f);
        
        // M^T * g = d  =>  g = (M^T)^-1 * d
        glm::vec3 g = glm::inverse(glm::transpose(M)) * d;

        glm::vec3 center = (p0 + p1 + p2) / 3.0f;
        
        // Direccion de propagacion (gradiente). La inversa es hacia la fuente.
        if(glm::length(g) < 1e-5f) continue; // Prevenir NaN
        glm::vec3 dir = -glm::normalize(g); 
        
        float lineLength = 0.04f;
        glm::vec3 end = center + dir * lineLength;

        lineVerts.push_back(center.x); lineVerts.push_back(center.y); lineVerts.push_back(center.z);
        lineVerts.push_back(end.x); lineVerts.push_back(end.y); lineVerts.push_back(end.z);
    }
    return lineVerts;
}

int main() {
    Window window(800, 800, "Task 06: Fast Marching Method");
    Shader shader("src/FastMarching/shader.vert", "src/FastMarching/shader.frag");

    EricStructure mesh;
    mesh.generateSphere(1.0f, 40, 40);

    std::vector<float> distances = mesh.fastMarching(0);
    float maxDist = 0.0f;
    for (float d : distances) {
        if (d < 999000.0f && d > maxDist) maxDist = d;
    }

    // VAO Esfera
    VAO vao; vao.bind();
    VBO vboGeom((float*) mesh.G.data(), mesh.G.size() * sizeof(VertexData));
    vao.linkAttrib(vboGeom, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vao.linkAttrib(vboGeom, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)(sizeof(Vec3)));
    VBO vboDist(distances.data(), distances.size() * sizeof(float));
    vao.linkAttrib(vboDist, 2, 1, GL_FLOAT, sizeof(float), (void*)0);
    EBO ebo(mesh.V);
    vao.unbind();

    // VAO Lineas de Gradiente (Flechitas rojas)
    std::vector<float> lines = generateGradientLines(mesh, distances);
    VAO vaoLines; vaoLines.bind();
    VBO vboLines(lines.data(), lines.size() * sizeof(float));
    vaoLines.linkAttrib(vboLines, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
    vaoLines.unbind();

    glEnable(GL_DEPTH_TEST);
    
    // Offset de polígonos para evitar que la esfera tape las líneas (Z-fighting)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    while (!window.shouldClose()) {
        window.processInput();
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        float time = glfwGetTime();
        float radius = 3.0f;
        float camX = sin(time * 0.5f) * radius;
        float camZ = cos(time * 0.5f) * radius;
        
        // Arreglo del estiramiento de pantalla en Hyprland
        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);
        float aspect = (float)width / (float)height;

        glm::mat4 view = glm::lookAt(glm::vec3(camX, 0.0f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1f(glGetUniformLocation(shader.ID, "maxDistance"), maxDist);

        // 1. Dibujar la esfera con colormap plano (sin luces)
        glUniform1i(glGetUniformLocation(shader.ID, "isLine"), 0);
        vao.bind();
        glDrawElements(GL_TRIANGLES, mesh.V.size(), GL_UNSIGNED_INT, 0);

        // 2. Dibujar las lineas rojas del gradiente
        glUniform1i(glGetUniformLocation(shader.ID, "isLine"), 1);
        vaoLines.bind();
        glDrawArrays(GL_LINES, 0, lines.size() / 3);

        window.update();
    }

    return 0;
}
