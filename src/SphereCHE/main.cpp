#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"

int main() {
    Window window(800, 600, "Esfera con Iluminación Estática");
    Shader shader("src/SphereCHE/shader.vert", "src/SphereCHE/shader.frag");

    EricStructure eric;
    eric.generateSphere(0.5f, 36, 18);

    VAO vao;
    vao.bind();

    // Ahora G guarda estructuras VertexData (Posición y Normal)
    VBO vbo((float*)eric.G.data(), eric.G.size() * sizeof(VertexData));
    EBO ebo(eric.V); 

    // Atributo 0: Posición (vec3)
    vao.linkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    // Atributo 1: Normal (vec3) - el offset es el tamaño de un Vec3 (los bytes que ocupa Position antes de llegar a Normal)
    vao.linkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));

    vao.unbind();
    vbo.unbind();
    ebo.unbind();

    // Dibujar sólido
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while (!window.shouldClose()) {
        window.processInput();
        window.clear(0.1f, 0.1f, 0.1f, 1.0f); // Fondo oscuro

        shader.use();
        
        // PASAR VARIABLES DE LUZ AL FRAGMENT SHADER
        // Posición de la luz: estática arriba, a la derecha y un poco hacia nosotros
        shader.setFloat("lightPos", 1.0f); // Usamos setFloat pero para vec3 toca crear una función o usar glUniform3f directo
        // Hagámoslo directamente con OpenGL por simplicidad:
        glUniform3f(glGetUniformLocation(shader.ID, "lightPos"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shader.ID, "lightColor"), 1.0f, 1.0f, 1.0f); // Luz blanca
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 1.0f, 0.0f, 0.0f); // Objeto rojo
        
        vao.bind();
        glDrawElements(GL_TRIANGLES, eric.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    vao.destroy();
    vbo.destroy();
    ebo.destroy();

    return 0;
}
