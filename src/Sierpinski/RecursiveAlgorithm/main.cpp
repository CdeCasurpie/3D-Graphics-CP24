#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include <vector>

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
    Shader shader("src/Sierpinski/RecursiveAlgorithm/shader.vert", "src/Sierpinski/RecursiveAlgorithm/shader.frag");

    std::vector<float> vertices;
    generateSierpinski(vertices, -0.8f, -0.8f, 0.8f, -0.8f, 0.0f, 0.8f, 6);

    VAO vao;
    vao.bind();

    VBO vbo(vertices);

    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

    vao.unbind();
    vbo.unbind();

    int totalTrianglesVertices = vertices.size() / 3;

    while (!window.shouldClose())
    {
        window.processInput();
        window.clear();

        shader.use();
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, totalTrianglesVertices);
 
        window.update();
    }

    vao.destroy();
    vbo.destroy();

    return 0;
}
