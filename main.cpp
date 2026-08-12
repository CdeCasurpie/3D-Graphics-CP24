#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main()
{
    // 1. Inicializar GLFW y configurar versión (3.3) y perfil (Core)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Crear la ventana
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL - Hello Window", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // 3. Registrar el callback para cuando se redimensiona la ventana
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 4. Inicializar GLAD para cargar los punteros a funciones de OpenGL
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 5. Bucle de renderizado (Render Loop)
    while (!glfwWindowShouldClose(window))
    {
        // 5a. Procesar entrada (ej. presionar ESC)
        processInput(window);

        // 5b. Comandos de renderizado: Limpiar la pantalla con un color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 5c. Intercambiar buffers y procesar eventos (teclado/mouse)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 6. Limpiar los recursos al terminar
    glfwTerminate();
    return 0;
}

// Función para procesar entradas del usuario
void processInput(GLFWwindow *window)
{
    // Si se presiona ESC, le decimos a GLFW que debería cerrar la ventana
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// Función que se llama automáticamente cada vez que se cambia el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Ajustamos el Viewport de OpenGL para que coincida con las nuevas dimensiones
    glViewport(0, 0, width, height);
}
