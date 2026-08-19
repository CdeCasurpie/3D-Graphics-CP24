#ifndef WINDOW_H
#define WINDOW_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
public:
    GLFWwindow* glfwWindow;
    unsigned int width;
    unsigned int height;

    Window(unsigned int w, unsigned int h, const char* title) : width(w), height(h) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindow = glfwCreateWindow(width, height, title, NULL, NULL);
        if (glfwWindow == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(-1);
        }
        
        glfwMakeContextCurrent(glfwWindow);
        
        // Configuramos la función de resize usando una función lambda de C++ moderna
        glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* win, int w, int h) {
            glViewport(0, 0, w, h);
        });

        if (gladLoadGL(glfwGetProcAddress) == 0) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            exit(-1);
        }
    }

    ~Window() {
        glfwTerminate();
    }

    bool shouldClose() {
        return glfwWindowShouldClose(glfwWindow);
    }

    // Procesa el ESC por defecto para cerrar
    void processInput() {
        if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(glfwWindow, true);
        }
    }

    // Actualiza la pantalla
    void update() {
        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();
    }
    
    // Limpia la pantalla con un color
    void clear(float r = 0.2f, float g = 0.3f, float b = 0.3f, float a = 1.0f) {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

#endif
