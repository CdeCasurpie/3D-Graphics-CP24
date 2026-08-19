#ifndef EBO_H
#define EBO_H

#include <glad/gl.h>
#include <vector>

class EBO {
public:
    unsigned int ID;

    EBO(const std::vector<unsigned int>& indices) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    void bind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    }

    void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void destroy() {
        glDeleteBuffers(1, &ID);
    }
};

#endif
