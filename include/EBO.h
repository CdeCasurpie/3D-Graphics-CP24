#ifndef EBO_H
#define EBO_H

#include <glad/gl.h>
#include <vector>

/**
 * @class EBO
 * @brief Wrapper para Element Buffer Object. Almacena los indices (triangulos) para el dibujado eficiente.
 */
class EBO {
public:
    unsigned int ID;

    /**
     * @brief Constructor que sube un vector de indices (Level 0 CHE) a la GPU.
     * @param indices Arreglo de indices enteros sin signo.
     */
    EBO(const std::vector<unsigned int>& indices) {
        glGenBuffers(1, &ID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    /** @brief Activa el EBO */
    void bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); }

    /** @brief Desactiva el EBO */
    void unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

    /** @brief Elimina el buffer de la VRAM */
    void destroy() { glDeleteBuffers(1, &ID); }
};

#endif
