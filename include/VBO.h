#ifndef VBO_H
#define VBO_H

#include <glad/gl.h>
#include <vector>

/**
 * @class VBO
 * @brief Wrapper para Vertex Buffer Object. Almacena arreglos crudos de vertices en la VRAM.
 */
class VBO {
public:
    unsigned int ID;

    /**
     * @brief Constructor que sube datos a la VRAM desde un vector.
     * @param vertices Arreglo crudo casteado a float*.
     * @param sizeInBytes Tamano total en bytes del arreglo a subir.
     */
    VBO(float* vertices, GLsizeiptr sizeInBytes) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ARRAY_BUFFER, ID);
        glBufferData(GL_ARRAY_BUFFER, sizeInBytes, vertices, GL_STATIC_DRAW);
    }

    /** @brief Activa el VBO */
    void bind() { glBindBuffer(GL_ARRAY_BUFFER, ID); }

    /** @brief Desactiva el VBO */
    void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

    /** @brief Elimina el buffer de la VRAM */
    void destroy() { glDeleteBuffers(1, &ID); }
};

#endif
