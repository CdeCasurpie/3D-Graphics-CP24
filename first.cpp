#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // PASOS OBLIGATORIOS, webadas
    // -----------------------------------------------------------------
    glfwInit(); // Inicializa GLFW (lo enciendes xd)

    //Configurar GLFW para que use OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                  // Version mayor
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                  // Version menor
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // Core profile




    // LO INTERESANTE, crear la ventana (POO)
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(800,600, "First Window", NULL, NULL); // Crea la ventana
    // GLFWwindow *glfwCreateWindow(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
    // El constructor tiene varios parametros, abarcaremos eso luego

    // Verificación de que la ventana se creo correctamente (en caso no, cerramos todo)
    if (window == NULL) { std::cout << "Failed to create GLFW window" << std::endl; glfwTerminate(); return -1; }

    // Hacer que el contexto de OpenGL sea el de la ventana creada (esto es para que OpenGL sepa donde dibujar)
    glfwMakeContextCurrent(window);


    // GLAD: Cargar todos los punteros de funciones OpenGL
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cout << "Failed to initialize GLAD" << std::endl; return -1; }
    // GLAD es un cargador de funciones de OpenGL, ya que OpenGL no tiene un cargador de funciones propio. 
    // Por eso usamos GLAD para cargar todas las funciones de OpenGL y poder usarlas en nuestro programa.
    // > ¿Sirve para algo más?
    // > No, solo para cargar las funciones de OpenGL. No es necesario entender como funciona GLAD, solo usarlo.
    
    


    // MAIN LOOP (bucle de juego)
    while(!glfwWindowShouldClose(window)) { // mientras la ventana no se cierre
        // limpiar pantalla
        // dibujar triangulo



        // intercambiar buffers y verificar eventos
        glfwSwapBuffers(window);    // Esto planta el buffer, evita flickering
        glfwPollEvents();           // Esto verifica si hay eventos (teclado, mouse, etc) y los procesa
    }


    glfwTerminate(); // Siempre terminar, es como cerrar un archivo o cerrar el programa. 
    // > ¿Si no lo hago se hace automaticamente?
    // > No, si no lo haces, el programa se queda en memoria y puede causar problemas.

    return 0;
}
