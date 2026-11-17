#ifndef SHOOTING_RAYS_SCENE_H
#define SHOOTING_RAYS_SCENE_H

#include "Camera.h"
#include "EricStructure.h"
#include "RayCaster.h"

namespace shooting {

inline void appendMesh(std::vector<Triangle>& triangles, const EricStructure& mesh,
                       const glm::mat4& transform) {
    for (size_t i = 0; i + 2 < mesh.V.size(); i += 3) {
        glm::vec3 points[3];
        for (int j = 0; j < 3; ++j) {
            const Vec3& p = mesh.G.at(mesh.V[i + j]).Position;
            points[j] = glm::vec3(transform * glm::vec4(p.x, p.y, p.z, 1.0f));
        }
        triangles.emplace_back(points[0], points[1], points[2]);
    }
}

inline std::vector<Triangle> makeScene() {
    std::vector<Triangle> triangles;
    triangles.emplace_back(glm::vec3(-8, 0, -12), glm::vec3(8, 0, -12), glm::vec3(8, 0, 5));
    triangles.emplace_back(glm::vec3(-8, 0, -12), glm::vec3(8, 0, 5), glm::vec3(-8, 0, 5));
    EricStructure cube;
    cube.generateCube(1.0f);
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 5; ++column) {
            const float height = 1.0f + float((column * 3 + row * 2) % 5) * 0.45f;
            const float x = (column - 2) * 2.5f + (row % 2) * 0.4f;
            const float z = 1.0f - row * 3.0f;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, height / 2, z));
            model = glm::scale(model, glm::vec3(1.1f, height, 1.1f));
            appendMesh(triangles, cube, model);
        }
    }
    EricStructure sphere;
    sphere.generateSphere(0.95f, 16, 10);
    appendMesh(triangles, sphere, glm::translate(glm::mat4(1), glm::vec3(-2, 0.95f, 3)));
    EricStructure torus;
    torus.generateTorus(0.9f, 0.3f, 20, 10);
    appendMesh(triangles, torus, glm::translate(glm::mat4(1), glm::vec3(2, 1.2f, 3)));
    return triangles;
}

inline Camera defaultCamera() {
    Camera camera(23.0f, glm::vec3(0, 0.8f, -3));
    camera.orientation = glm::quat(glm::vec3(glm::radians(27.0f), glm::radians(-13.0f), 0));
    return camera;
}

inline View snapshot(const Camera& camera, int width, int height) {
    return {camera.getPosition(), glm::inverse(camera.getProjectionMatrix(width, height) * camera.getViewMatrix())};
}

} // namespace shooting
#endif
