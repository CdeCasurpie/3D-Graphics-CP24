#include "Window.h"
#include "Shader.h"
#include <vector>
#include <cstdlib>

const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)glsl";

const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)glsl";

int main() {
    Window window(800, 600, "Sierpinski Chaos (Framework)");
    Shader shader(vertexShaderSource, fragmentShaderSource);

    float ax = -0.5f, ay = -0.5f;
    float bx = 0.5f, by = -0.5f;
    float cx = 0.0f, cy = 0.5f;
    
    float px = ax, py = ay;

    std::vector<float> vertices;
    int totalPuntos = 50000;

    for (int i = 0; i < totalPuntos; i++) {
        vertices.push_back(px);
        vertices.push_back(py);
        vertices.push_back(0.0f);

        int randomVertex = rand() % 3;
        if (randomVertex == 0) {
            px = (px + ax) / 2.0f; py = (py + ay) / 2.0f;
        } else if (randomVertex == 1) {
            px = (px + bx) / 2.0f; py = (py + by) / 2.0f;
        } else {
            px = (px + cx) / 2.0f; py = (py + cy) / 2.0f;
        }
    }

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int puntosVisibles = 1;

    while(!window.shouldClose()) {
        window.processInput();
        window.clear();

        shader.use();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glPointSize(2.0f);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, puntosVisibles);
        glBindVertexArray(0);

        if (puntosVisibles < totalPuntos) {
            puntosVisibles += 50; 
        }

        window.update();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}