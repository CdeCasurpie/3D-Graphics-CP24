#ifndef VAO_H
#define VAO_H

#include <glad/gl.h>
#include "VBO.h"

class VAO {
public:
    unsigned int ID;

    VAO() {
        glGenVertexArrays(1, &ID);
    }

    // Configura cómo leer el VBO. (Por ahora, asume un layout básico de floats)
    void linkAttrib(VBO& VBO, unsigned int layout, unsigned int numComponents, GLenum type, GLsizeiptr stride, void* offset) {
        VBO.bind();
        glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
        glEnableVertexAttribArray(layout);
        VBO.unbind();
    }

    void bind() {
        glBindVertexArray(ID);
    }

    void unbind() {
        glBindVertexArray(0);
    }

    void destroy() {
        glDeleteVertexArrays(1, &ID);
    }
};

#endif
