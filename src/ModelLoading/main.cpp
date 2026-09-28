#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

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
    Window window(1280, 720, "Task 13: Model Loading");

    Shader textureShader("src/TexturesAndMaps/texture.vert", "src/TexturesAndMaps/texture.frag");
    Shader lightShader("src/ShadowMapping/light.vert", "src/ShadowMapping/light.frag");

    // Set camera distance to 30 and target to the origin (since the model is centered)
    Camera camera(30.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    camera.registerCallbacks(window.glfwWindow);

    std::cout << "TAREA 13: Carga de Modelos OBJ (tinyobjloader)\n";

    // 1. Load Model (User's thesis OBJ, now centered locally)
    Model loadedModel("assets/models/thesis/03_corner.obj");

    // 2. Setup Light Marker (small sphere)
    EricStructure lightMarker;
    lightMarker.generateSphere(0.5f, 16, 16); // Make light marker bigger
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
        // Change background to a softer color to see better
        glClearColor(0.5f, 0.6f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(width, height);

        // Light config: Orbit around the large building
        float lightRadius = 25.0f;
        float lightAngle = currentTime * 0.5f;
        // The model spans Y from -8 to 8 roughly, so put the light high up at Y=10.0f
        glm::vec3 lightPos(std::sin(lightAngle) * lightRadius, 10.0f, std::cos(lightAngle) * lightRadius);
        // Make the light brighter
        glm::vec3 lightColor(2.0f, 2.0f, 2.0f);

        textureShader.use();
        textureShader.setFloat("material.shininess", 32.0f);
        glUniform3fv(glGetUniformLocation(textureShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(textureShader.ID, "viewPos"), 1, glm::value_ptr(camera.getPosition()));
        glUniform3fv(glGetUniformLocation(textureShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Draw Model
        glm::mat4 model = glm::mat4(1.0f);
        // The OBJ specifies "# metres; Z up", so we rotate -90 degrees around X to make Y up!
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // Rotate slowly around the new Y axis (which was Z in the model) so we can see all sides
        model = glm::rotate(model, currentTime * 0.2f, glm::vec3(0.0f, 0.0f, 1.0f)); 

        glUniformMatrix4fv(glGetUniformLocation(textureShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        loadedModel.draw(textureShader);

        // Draw Light Marker
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
