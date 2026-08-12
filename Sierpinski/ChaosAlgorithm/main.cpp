#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>


// VERTEX SHADER: es un shader en el cual se procesan los vertices, y se le puede dar color, textura, etc.
// en este caso solo le damos la posición del vertice, y OpenGL se encarga de dibujarlo en la pantalla.
const char *vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos; // 0 porque es el primer atributo (asi como pusimos abajo en los VAO y VBO)
void main()
{
    // gl_Position es una variable especial que le dice a OpenGL donde dibujar el vertice en la pantalla
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)glsl";


// FRAGMENT SHADER: Decide el color de cada pixel DENTRO del triángulo, y se ejecuta para 
// cada pixel que forma parte del triángulo.
const char *fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor; // variable de salida que le dice a OpenGL el color del pixel
void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // color naranja
}
)glsl";
// En este caso le ponemos un color naranja a todos los pixeles del triángulo, pero se puede hacer que cada pixel tenga un color diferente, o que tenga una textura, etc.


void framebuffer_size_callback(GLFWwindow* window, int width, int height);


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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // GLAD: Cargar todos los punteros de funciones OpenGL
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cout << "Failed to initialize GLAD" << std::endl; return -1; }
    // GLAD es un cargador de funciones de OpenGL, ya que OpenGL no tiene un cargador de funciones propio. 
    // Por eso usamos GLAD para cargar todas las funciones de OpenGL y poder usarlas en nuestro programa.
    // > ¿Sirve para algo más?
    // > No, solo para cargar las funciones de OpenGL. No es necesario entender como funciona GLAD, solo usarlo.
    



    // SHADERS PARA LA GPU
    // -----------------------------------------------------------------
    // VERTEX SHADER
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER); // Crea un shader de tipo vertex shader y devuelve un ID (vertexShader) que lo identifica
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // Le pasamos el código fuente del shader (vertexShaderSource) al shader creado (vertexShader)
    glCompileShader(vertexShader);

    // Es necesario compilar los shaders, porque como habremos visto, son un código que la GPU va a ejecutar, y la GPU no entiende C++, por eso hay que compilarlo a un lenguaje que la GPU entienda (GLSL).

    // FRAGMENT SHADER
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Crea un shader de tipo fragment shader y devuelve un ID (fragmentShader) que lo identifica
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); // Le pasamos el código fuente del shader (fragmentShaderSource) al shader creado (fragmentShader)
    glCompileShader(fragmentShader);


    // glShaderSource es una función que le dice a OpenGL que el código fuente del shader es el que le pasamos.
    // Sus atributos son:
    // - el ID del shader (vertexShader o fragmentShader)
    // - el número de cadenas de código fuente (1, porque solo le pasamos una cadena de código fuente)
    // - un puntero a un array de cadenas de código fuente (&vertexShaderSource o &fragmentShaderSource)
    // - un puntero a un array de longitudes de las cadenas de código fuente (NULL, porque las cadenas de código fuente son nula


    // UNIR O LINKEAR LOS SHADERS EN UNO
    unsigned int shaderProgram = glCreateProgram(); // Crea un programa de shaders y devuelve un ID (shaderProgram) que lo identifica
    glAttachShader(shaderProgram, vertexShader); // Le pasamos el shader de tipo vertex shader al programa de shaders
    glAttachShader(shaderProgram, fragmentShader); // Le pasamos el shader de tipo fragment shader al programa de shaders
    glLinkProgram(shaderProgram); // Linkea los shaders en un solo programa de shaders, que es el que se va a ejecutar en la


    // Como ya creamos uno shaderProgram ya no son necesarios los anteriores shaders.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);




    // LAS COORDENADAS EN OPENGL SON EN 3D, POR ESO SE USAN VERTICES DE 3 COMPONENTES (X, Y, Z)
    float ax = -0.5f, ay = -0.5f, az = 0.0f; // Vertice A
    float bx = 0.5f, by = -0.5f, bz = 0.0f; // Vertice B
    float cx = 0.0f, cy = 0.5f, cz = 0.0f; // Vertice C
    

    // PUNTO AL AZAR, para lo del triangulo de la clase
    float px = ax;
    float py = ay;

    std::vector<float> vertices;
    int totalPuntos = 5000000;
    int puntosVisibles = 1;

    for (int i = 0; i < totalPuntos; i++) {
        vertices.push_back(px);
        vertices.push_back(py);
        vertices.push_back(0.0f); // Z = 0.0f,

        int randomVertex = rand() % 3; // 0, 1, 2

        // El punto medio calculado en cuestion
        if (randomVertex == 0) {
            px = (px + ax) / 2.0f;
            py = (py + ay) / 2.0f;
        } else if (randomVertex == 1) {
            px = (px + bx) / 2.0f;
            py = (py + by) / 2.0f;
        } else {
            px = (px + cx) / 2.0f;
            py = (py + cy) / 2.0f;
        }
        // y repetimos
    }

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
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);


    // Ahora hay que decirle a la GPU como interpretar la data que le hemos enviado
    // ya que los vertices son solo un array de floats, y la GPU no sabe como interpretarlos.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // los atributos de esta funcion son:
    // - el índice del atributo (0, porque es el primer atributo)
    // - el número de componentes del atributo (3, porque son 3 floats por vertice)
    // - el tipo de dato del atributo (GL_FLOAT, porque son floats)
    // - si los datos deben ser normalizados (GL_FALSE, porque no queremos normalizar)
    // - el tamaño del stride (3 * sizeof(float), porque cada vertice tiene 3 floats)
    // - el offset (0, porque los datos empiezan en el inicio del array)

    glEnableVertexAttribArray(0); // Habilitamos el atributo de posición (0, porque es el primer atributo)
    // > ¿Que es un atributo?
    // > Un atributo es una propiedad de un vertice, como la posición, el color, las coordenadas 
    //   de textura, etc. En este caso estamos habilitando el atributo de posición,

    // NOS DESATAMOS (unbind) del VBO y VAO, para que OpenGL no los modifique accidentalmente
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    // MAIN LOOP (bucle de juego)
    while(!glfwWindowShouldClose(window)) { // mientras la ventana no se cierre
        // limpiar pantalla
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Color de fondo
        glClear(GL_COLOR_BUFFER_BIT); // Limpiar el buffer de color
        // dibujar triangulo

        // LE DECIMOS DIBUJA! ---------------------------
        glUseProgram(shaderProgram); // Usar el programa de shaders
        glEnable(GL_PROGRAM_POINT_SIZE); // permite cambiar el tamaño de los puntos
        glPointSize(3.0f); // tamaño de los puntos
        glBindVertexArray(VAO); // Atamos el VAO, para que OpenGL sepa que vamos a dibujar con este VAO
        glDrawArrays(GL_POINTS, 0, totalPuntos); // Dibujar el triangulo, con 3 vertices, empezando en el vertice 0
        glBindVertexArray(0); // Desatamos el VAO, para que OpenGL no lo modifique




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


        if (puntosVisibles < totalPuntos) { // si no hemos dibujado todos los puntos
            puntosVisibles += 100; // dibujar 1000 puntos más
        } else {
            std::cout << "Todos los puntos ya fueron dibujados" << std::endl;
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


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height); // Cambia el tamaño del viewport (la ventana de OpenGL) cuando se cambia el tamaño de la ventana
}