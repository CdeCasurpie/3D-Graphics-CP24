#include "Window.h"
#include "Shader.h"
#include <vector>

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

void generateSierpinski(std::vector<float>& vertices, 
                        float ax, float ay, 
                        float bx, float by, 
                        float cx, float cy, 
                        int depth)
{
    if (depth == 0) {
        vertices.push_back(ax); vertices.push_back(ay); vertices.push_back(0.0f);
        vertices.push_back(bx); vertices.push_back(by); vertices.push_back(0.0f);
        vertices.push_back(cx); vertices.push_back(cy); vertices.push_back(0.0f);
    } else {
        float abx = (ax + bx) / 2.0f; float aby = (ay + by) / 2.0f;
        float bcx = (bx + cx) / 2.0f; float bcy = (by + cy) / 2.0f;
        float cax = (cx + ax) / 2.0f; float cay = (cy + ay) / 2.0f;

        generateSierpinski(vertices, ax, ay, abx, aby, cax, cay, depth - 1);
        generateSierpinski(vertices, abx, aby, bx, by, bcx, bcy, depth - 1);
        generateSierpinski(vertices, cax, cay, bcx, bcy, cx, cy, depth - 1);
    }
}

int main()
{
    Window window(800, 600, "Sierpinski Recursive (Framework)");
    Shader shader(vertexShaderSource, fragmentShaderSource);

    std::vector<float> vertices;
    generateSierpinski(vertices, -0.8f, -0.8f, 0.8f, -0.8f, 0.0f, 0.8f, 6);

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

    int totalTrianglesVertices = vertices.size() / 3;

    while (!window.shouldClose())
    {
        window.processInput();
        window.clear();

        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, totalTrianglesVertices);
 
        window.update();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}
