#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include <vector>
#include <cstdlib>

int main() {
    Window window(800, 800, "Sierpinski Chaos (Framework)");
    Shader shader("src/Sierpinski/ChaosAlgorithm/shader.vert", "src/Sierpinski/ChaosAlgorithm/shader.frag");

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

    VAO vao;
    vao.bind();

    VBO vbo(vertices);

    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

    vao.unbind();
    vbo.unbind();

    int puntosVisibles = 1;

    while(!window.shouldClose()) {
        window.processInput();
        window.clear();

        shader.use();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glPointSize(2.0f);
        
        vao.bind();
        glDrawArrays(GL_POINTS, 0, puntosVisibles);

        if (puntosVisibles < totalPuntos) {
            puntosVisibles += 50; 
        }

        window.update();
    }

    vao.destroy();
    vbo.destroy();

    return 0;
}