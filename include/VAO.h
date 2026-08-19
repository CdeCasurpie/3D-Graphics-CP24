#ifndef VAO_H
#define VAO_H

#include <glad/gl.h>
#include "VBO.h"

/**
 * @class VAO
 * @brief Wrapper para Vertex Array Object. Administra el estado de los VBOs y EBOs.
 */
class VAO {
public:
    unsigned int ID;

    /** @brief Constructor que genera el VAO en OpenGL */
    VAO() {
        glGenVertexArrays(1, &ID);
    }

    /**
     * @brief Conecta un VBO a este VAO especificando como interpretar los datos de los vertices.
     * @param VBO El Vertex Buffer Object a leer.
     * @param layout El 'location' en el Vertex Shader (ej. location = 0).
     * @param numComponents Cantidad de valores (ej. 3 para vec3).
     * @param type Tipo de dato (ej. GL_FLOAT).
     * @param stride Bytes totales de un vertice (sizeof(VertexData)).
     * @param offset Offset en bytes de donde empieza este atributo dentro del vertice.
     */
    void linkAttrib(VBO& VBO, unsigned int layout, unsigned int numComponents, GLenum type, GLsizeiptr stride, void* offset) {
        VBO.bind();
        glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
        glEnableVertexAttribArray(layout);
        VBO.unbind();
    }

    /** @brief Activa el VAO */
    void bind() { glBindVertexArray(ID); }

    /** @brief Desactiva el VAO */
    void unbind() { glBindVertexArray(0); }

    /** @brief Destruye el VAO de la VRAM */
    void destroy() { glDeleteVertexArrays(1, &ID); }
};

#endif
