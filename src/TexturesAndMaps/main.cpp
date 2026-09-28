#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"
#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

/**
 * @brief Utility function to load a 2D texture from a file using stb_image.h.
 * 
 * It automatically flips the image vertically to match OpenGL's coordinate system,
 * generates mipmaps, and configures standard wrapping and filtering parameters.
 * 
 * @param path The file path to the image texture.
 * @return unsigned int The OpenGL ID of the generated texture object.
 */
unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Configure wrapping and filtering (Linear filtering + Mipmaps)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

int main() {
    Window window(800, 800, "Task 12: Textures & Lighting Maps");

    Shader textureShader("src/TexturesAndMaps/texture.vert", "src/TexturesAndMaps/texture.frag");
    Shader lightShader("src/ShadowMapping/light.vert", "src/ShadowMapping/light.frag");

    Camera camera(5.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    camera.registerCallbacks(window.glfwWindow);

    std::cout << "TAREA 12: Texturas y Mapas de Iluminacion\n";

    // =========================================================================
    // 1. Load Textures (Diffuse, Specular, Normal)
    // =========================================================================
    unsigned int diffuseMap = loadTexture("assets/textures/container2.png");
    unsigned int specularMap = loadTexture("assets/textures/container2_specular.png");
    unsigned int brickNormalMap = loadTexture("assets/textures/brickwall_normal.jpg");
    unsigned int earthMap = loadTexture("assets/textures/earth.jpg");

    // =========================================================================
    // 2. Setup Geometries and Buffers
    // =========================================================================
    // We use generateTexturedCube to get 24 vertices (so each face has unique UVs)
    EricStructure cubeMesh;
    cubeMesh.generateTexturedCube(1.0f);
    VAO vaoCube; vaoCube.bind();
    VBO vboCube((float*)cubeMesh.G.data(), cubeMesh.G.size() * sizeof(VertexData));
    EBO eboCube(cubeMesh.V);
    vaoCube.linkAttrib(vboCube, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoCube.linkAttrib(vboCube, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoCube.linkAttrib(vboCube, 2, 2, GL_FLOAT, sizeof(VertexData), (void*)(2 * sizeof(Vec3)));
    vaoCube.unbind();

    EricStructure sphereMesh;
    sphereMesh.generateSphere(1.0f, 40, 40);
    VAO vaoSphere; vaoSphere.bind();
    VBO vboSphere((float*)sphereMesh.G.data(), sphereMesh.G.size() * sizeof(VertexData));
    EBO eboSphere(sphereMesh.V);
    vaoSphere.linkAttrib(vboSphere, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoSphere.linkAttrib(vboSphere, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoSphere.linkAttrib(vboSphere, 2, 2, GL_FLOAT, sizeof(VertexData), (void*)(2 * sizeof(Vec3)));
    vaoSphere.unbind();

    EricStructure lightMarker;
    lightMarker.generateSphere(0.1f, 16, 16);
    VAO vaoLight; vaoLight.bind();
    VBO vboLight((float*)lightMarker.G.data(), lightMarker.G.size() * sizeof(VertexData));
    EBO eboLight(lightMarker.V);
    vaoLight.linkAttrib(vboLight, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoLight.unbind();

    glEnable(GL_DEPTH_TEST);

    textureShader.use();
    textureShader.setInt("material.diffuse", 0);
    textureShader.setInt("material.specular", 1);
    textureShader.setInt("material.normalMap", 2);

    float lastTime = (float)glfwGetTime();

    while (!window.shouldClose()) {
        window.processInput();
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        camera.update(deltaTime);

        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(width, height);

        // Light config
        float lightRadius = 3.0f;
        float lightAngle = currentTime * 0.8f;
        glm::vec3 lightPos(std::sin(lightAngle) * lightRadius, 1.5f, std::cos(lightAngle) * lightRadius);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        textureShader.use();
        textureShader.setFloat("material.shininess", 32.0f);
        glUniform3fv(glGetUniformLocation(textureShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(textureShader.ID, "viewPos"), 1, glm::value_ptr(camera.getPosition()));
        glUniform3fv(glGetUniformLocation(textureShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // =====================================================================
        // 1. Draw Wooden Container (Cube)
        // =====================================================================
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brickNormalMap); // Using brick normal just to show the bump effect

        textureShader.setBool("useNormalMap", true);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5f, 0.0f, 0.0f));
        model = glm::rotate(model, currentTime * 0.2f, glm::normalize(glm::vec3(0.5, 1.0, 0.0)));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        vaoCube.bind();
        glDrawElements(GL_TRIANGLES, cubeMesh.V.size(), GL_UNSIGNED_INT, 0);

        // =====================================================================
        // 2. Draw Earth (Sphere)
        // =====================================================================
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthMap);
        glActiveTexture(GL_TEXTURE1); // Re-use diffuse as specular just to have some reflection
        glBindTexture(GL_TEXTURE_2D, earthMap); 
        textureShader.setBool("useNormalMap", false); // Earth has no normal map in our assets

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.5f, 0.0f, 0.0f));
        model = glm::rotate(model, currentTime * 0.4f, glm::vec3(0.0, 1.0, 0.0));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        vaoSphere.bind();
        glDrawElements(GL_TRIANGLES, sphereMesh.V.size(), GL_UNSIGNED_INT, 0);

        // =====================================================================
        // 3. Draw Light Source (White Sphere)
        // =====================================================================
        lightShader.use();
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(lightShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));

        vaoLight.bind();
        glDrawElements(GL_TRIANGLES, lightMarker.V.size(), GL_UNSIGNED_INT, 0);

        window.update();
    }

    return 0;
}
