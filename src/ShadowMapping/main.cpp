#include "Window.h"
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "EricStructure.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

void printInstructions() {
    std::cout << "\n=======================================================\n";
    std::cout << "   TAREA 10: 2-PASS SHADOW MAPPING                     \n";
    std::cout << "                 3D Graphics - UTEC                    \n";
    std::cout << "=======================================================\n";
    std::cout << " [Controles]\n";
    std::cout << "  - Clic Izquierdo + Arrastrar : Rotar vista de la camara\n";
    std::cout << "  - Scroll del Raton           : Zoom In / Zoom Out\n";
    std::cout << "  - Clic Derecho + Arrastrar   : Paneo de camara\n";
    std::cout << "  - Tecla 'R'                  : Reiniciar camara\n";
    std::cout << "  - Tecla 'ESC'                : Salir\n";
    std::cout << "=======================================================\n";
    std::cout << " - Resolviendo Shadow Acne: Se implemento Shadow Bias.\n";
    std::cout << " - Resolviendo Peter Panning: Activado Culling de caras frontales opcional (o bias dinamico).\n";
    std::cout << " - Resolviendo Bordes Duros: Se usa PCF (Percentage-Closer Filtering).\n";
    std::cout << " - Resolviendo Sombras fuera de region: Se usa GL_CLAMP_TO_BORDER.\n";
    std::cout << "=======================================================\n";
}

int main() {
    Window window(800, 800, "Task 10: Shadow Mapping");

    // 1. Shaders
    Shader shadowMappingShader("src/ShadowMapping/shadowMapping.vert", "src/ShadowMapping/shadowMapping.frag");
    Shader simpleDepthShader("src/ShadowMapping/simpleDepthShader.vert", "src/ShadowMapping/simpleDepthShader.frag");
    Shader lightShader("src/ShadowMapping/light.vert", "src/ShadowMapping/light.frag");

    // 2. Camera
    Camera camera(8.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    camera.registerCallbacks(window.glfwWindow);

    printInstructions();

    // 3. Geometries
    // Floor (Cube scaled down in Y)
    EricStructure floorMesh;
    floorMesh.generateCube(1.0f);
    VAO vaoFloor; vaoFloor.bind();
    VBO vboFloor((float*)floorMesh.G.data(), floorMesh.G.size() * sizeof(VertexData));
    EBO eboFloor(floorMesh.V);
    vaoFloor.linkAttrib(vboFloor, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoFloor.linkAttrib(vboFloor, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoFloor.unbind();

    // Cube
    EricStructure cubeMesh;
    cubeMesh.generateCube(1.0f);
    VAO vaoCube; vaoCube.bind();
    VBO vboCube((float*)cubeMesh.G.data(), cubeMesh.G.size() * sizeof(VertexData));
    EBO eboCube(cubeMesh.V);
    vaoCube.linkAttrib(vboCube, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoCube.linkAttrib(vboCube, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoCube.unbind();

    // Torus
    EricStructure torusMesh;
    torusMesh.generateTorus(0.5f, 0.2f, 40, 40, false);
    VAO vaoTorus; vaoTorus.bind();
    VBO vboTorus((float*)torusMesh.G.data(), torusMesh.G.size() * sizeof(VertexData));
    EBO eboTorus(torusMesh.V);
    vaoTorus.linkAttrib(vboTorus, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoTorus.linkAttrib(vboTorus, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
    vaoTorus.unbind();

    // Light marker
    EricStructure lightMarker;
    lightMarker.generateSphere(0.1f, 16, 16);
    VAO vaoLight; vaoLight.bind();
    VBO vboLight((float*)lightMarker.G.data(), lightMarker.G.size() * sizeof(VertexData));
    EBO eboLight(lightMarker.V);
    vaoLight.linkAttrib(vboLight, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
    vaoLight.unbind();

    // 4. Configure Depth Map FBO
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Fix shadow regions outside the light's frustum
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Render loop setup
    glEnable(GL_DEPTH_TEST);
    
    // Shader config
    shadowMappingShader.use();
    glUniform1i(glGetUniformLocation(shadowMappingShader.ID, "shadowMap"), 0);

    float lastTime = (float)glfwGetTime();

    // Define lambda for drawing the scene objects
    auto renderScene = [&](Shader& shader) {
        // Floor
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(10.0f, 0.1f, 10.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.5f, 0.5f, 0.5f); // Gray floor
        vaoFloor.bind();
        glDrawElements(GL_TRIANGLES, floorMesh.V.size(), GL_UNSIGNED_INT, 0);
        vaoFloor.unbind();

        // Cubes
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.8f, 0.3f, 0.3f); // Red cube
        vaoCube.bind();
        glDrawElements(GL_TRIANGLES, cubeMesh.V.size(), GL_UNSIGNED_INT, 0);
        vaoCube.unbind();

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.3f, 0.8f, 0.3f); // Green cube
        vaoCube.bind();
        glDrawElements(GL_TRIANGLES, cubeMesh.V.size(), GL_UNSIGNED_INT, 0);
        vaoCube.unbind();

        // Animated Torus
        float time = (float)glfwGetTime();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, 1.0f + std::sin(time) * 0.5f, -1.0f));
        model = glm::rotate(model, time, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shader.ID, "objectColor"), 0.3f, 0.3f, 0.8f); // Blue torus
        vaoTorus.bind();
        glDrawElements(GL_TRIANGLES, torusMesh.V.size(), GL_UNSIGNED_INT, 0);
        vaoTorus.unbind();
    };

    while (!window.shouldClose()) {
        window.processInput();
        
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        camera.update(deltaTime);

        // Animate Light
        float lightRadius = 4.0f;
        float lightAngle = currentTime * 0.5f;
        glm::vec3 lightPos(std::sin(lightAngle) * lightRadius, 4.0f, std::cos(lightAngle) * lightRadius);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        // -------------------------------------------
        // PASS 1: Render depth to shadow map
        // -------------------------------------------
        float near_plane = 1.0f, far_plane = 15.0f;
        // The orthographic projection size defines the shadow area
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        simpleDepthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(simpleDepthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Front-face culling to solve peter panning, but with solid objects back-face is fine
        // glCullFace(GL_FRONT);
        renderScene(simpleDepthShader);
        // glCullFace(GL_BACK);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // -------------------------------------------
        // PASS 2: Render scene with shadows
        // -------------------------------------------
        int width, height;
        glfwGetFramebufferSize(window.glfwWindow, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadowMappingShader.use();
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(width, height);
        
        glUniformMatrix4fv(glGetUniformLocation(shadowMappingShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shadowMappingShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shadowMappingShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glUniform3fv(glGetUniformLocation(shadowMappingShader.ID, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shadowMappingShader.ID, "viewPos"), 1, glm::value_ptr(camera.getPosition()));
        glUniform3fv(glGetUniformLocation(shadowMappingShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        
        renderScene(shadowMappingShader);

        // -------------------------------------------
        // Render Light Marker
        // -------------------------------------------
        lightShader.use();
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(lightShader.ID, "lightColor"), 1, glm::value_ptr(lightColor));

        vaoLight.bind();
        glDrawElements(GL_TRIANGLES, lightMarker.V.size(), GL_UNSIGNED_INT, 0);
        vaoLight.unbind();

        window.update();
    }

    return 0;
}
