#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"

int main()
{
    Window window(800, 800, "Hello Triangle (Framework)");

    // Compila los Shaders leyendo desde los archivos (rutas relativas a la carpeta de ejecución)
    Shader shader("src/HelloTriangle/shader.vert", "src/HelloTriangle/shader.frag");

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    }; 

    // Usando nuestros nuevos Wrappers para VAO y VBO
    VAO vao;
    vao.bind();

    VBO vbo(vertices, sizeof(vertices));

    // Enlazar los atributos (location = 0, 3 floats)
    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);

    vao.unbind();
    vbo.unbind();

    // Game Loop
    while (!window.shouldClose())
    {
        window.processInput(); 
        window.clear();        

        shader.use();
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
 
        window.update();       
    }

    vao.destroy();
    vbo.destroy();

    return 0;
}
