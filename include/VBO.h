#ifndef VBO_H
#define VBO_H

#include <glad/gl.h>
#include <vector>

class VBO {
public:
    unsigned int ID;

    // Constructor que genera el buffer y le manda los datos
    VBO(const std::vector<float>& vertices) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ARRAY_BUFFER, ID);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    }
    
    // Sobrecarga para arreglos estáticos (C-style arrays)
    VBO(float* vertices, GLsizeiptr size) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ARRAY_BUFFER, ID);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    void bind() {
        glBindBuffer(GL_ARRAY_BUFFER, ID);
    }

    void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void destroy() {
        glDeleteBuffers(1, &ID);
    }
};

#endif
