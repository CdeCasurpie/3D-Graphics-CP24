#include "Window.h"
#include "Shader.h"

// Vertex Shader simplificado
const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)glsl";

// Fragment Shader simplificado
const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
void main() {
   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)glsl";

int main()
{
    // 1. Inicializamos la ventana usando nuestro Wrapper
    Window window(800, 600, "Hello Triangle (Framework)");

    // 2. Compilamos los Shaders en una sola línea
    Shader shader(vertexShaderSource, fragmentShaderSource);

    // 3. Geometría
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    }; 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    // 4. Game Loop
    while (!window.shouldClose())
    {
        window.processInput(); // Chequea el ESC
        window.clear();        // Limpia la pantalla de color

        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
 
        window.update();       // Swaps buffers and polls events
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}
