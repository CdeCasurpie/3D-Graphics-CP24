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
    


    // LAS COORDENADAS EN OPENGL SON EN 3D, POR ESO SE USAN VERTICES DE 3 COMPONENTES (X, Y, Z)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left  
         0.5f, -0.5f, 0.0f, // right 
         0.0f,  0.5f, 0.0f  // top   
    };


    // CREAR EL VERTEX BUFFER OBJECT (VBO) Y EL VERTEX ARRAY OBJECT (VAO)
    // -----------------------------------------------------------------
    // EL VBO es un buffer que almacena los vertices en la memoria de la GPU, para que OpenGL 
    // pueda acceder a ellos y dibujarlos.
    // Por otro lado el VAO es un objeto que almacena la configuración de los atributos de los vertices,
    // como la posición, el color, las coordenadas de textura, etc.
    unsigned int VBO, VAO;      // Estas variables guardan los IDs del VBO y VAO, que son generados por OpenGL.
    glGenVertexArrays(1, &VAO); // Genera un VAO 
    glGenBuffers(1, &VBO);      // Genera un VBO

    // Bind (atamos) el VAO, para que OpenGL sepa que vamos a configurar este VAO
    glBindVertexArray(VAO);
    // Bind (atamos) el VBO, para que OpenGL sepa que vamos a configurar este VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);


    // Copiamos los vertices al VBO, para que OpenGL pueda acceder a ellos
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // MAIN LOOP (bucle de juego)
    while(!glfwWindowShouldClose(window)) { // mientras la ventana no se cierre
        // limpiar pantalla
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Color de fondo
        glClear(GL_COLOR_BUFFER_BIT); // Limpiar el buffer de color
        // dibujar triangulo




        // INPUTS ---------------------------------------
        // Se hace referencia al window porque es el que tiene el contexto de OpenGL, 
        // y es el que recibe los inputs del teclado y mouse.
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { // asi se reconocen las teclas, si se presiona S
            std::cout << "S key is pressed" << std::endl; 
            glfwSetWindowShouldClose(window, true); // Cierra la ventana si se presiona S
        } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { // si se presiona W
            std::cout << "W key is pressed" << std::endl; 
        } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { // si se presiona A
            std::cout << "A key is pressed" << std::endl; 
        } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { // si se presiona D
            std::cout << "D key is pressed" << std::endl; 

        }


        // intercambiar buffers y verificar eventos
        glfwSwapBuffers(window);    // Esto planta el buffer, evita flickering
        glfwPollEvents();           // Esto verifica si hay eventos (teclado, mouse, etc) y los procesa
    }


    glfwTerminate(); // Siempre terminar, es como cerrar un archivo o cerrar el programa. 
    // > ¿Si no lo hago se hace automaticamente?
    // > No, si no lo haces, el programa se queda en memoria y puede causar problemas.

    return 0;
}
