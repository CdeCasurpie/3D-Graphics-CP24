#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // PASOS OBLIGATORIOS, webadas
    // ---------------------------------------
    glfwInit(); // Inicializa GLFW (lo enciendes xd)

    //Configurar GLFW para que use OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Version mayor
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Version menor
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Core profile


    // LO INTERESANTE, crear la ventana (POO)
    GLFWwindow* window = glfwCreateWindow(800,600, "First Window", NULL, NULL); // Crea la ventana
    // GLFWwindow *glfwCreateWindow(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
    // El constructor tiene varios parametros, abarcaremos eso luego

    // Verificación de que la ventana se creo correctamente (en caso no, cerramos todo)
    if (window == NULL) { std::cout << "Failed to create GLFW window" << std::endl; glfwTerminate(); return -1; }

    
    // Hacer que el contexto de OpenGL sea el de la ventana creada (esto es para que OpenGL sepa donde dibujar)
    glfwMakeContextCurrent(window);
    
    return 0;
}
