#ifndef CAMERA_H
#define CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cmath>
#include <algorithm>
#include <iostream>

/**
 * @class Camera
 * @brief Implementación de cámara Arcball orbital con Cuaterniones para interacción fluida 3D.
 *        Compatible con los requisitos de la tarea (struct/class named camera/Camera).
 */
class Camera {
public:
    // Estado de orientación y posición
    glm::quat orientation;
    glm::vec3 target;
    float distance;
    float minDistance;
    float maxDistance;

    // Configuración de proyección
    float fov;
    float nearPlane;
    float farPlane;

    // Estado del Arcball y ratón
    bool isDragging;
    bool isPanning;
    glm::vec3 lastMouseVec;
    double lastX;
    double lastY;

    // Modo animación automática (Task 07) vs Interacción manual (Arcball)
    bool isAnimated;
    float animationSpeed;

    /**
     * @brief Constructor por defecto de la cámara Arcball
     */
    Camera(float initialDistance = 4.0f, glm::vec3 lookAtTarget = glm::vec3(0.0f))
        : orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
          target(lookAtTarget),
          distance(initialDistance),
          minDistance(0.5f),
          maxDistance(50.0f),
          fov(45.0f),
          nearPlane(0.1f),
          farPlane(100.0f),
          isDragging(false),
          isPanning(false),
          lastMouseVec(0.0f),
          lastX(0.0),
          lastY(0.0),
          isAnimated(false),
          animationSpeed(0.8f) {}

    /**
     * @brief Mapea coordenadas 2D de la pantalla a un vector 3D en la semiesfera de radio 1.
     *        Fórmula de las diapositivas UTEC: Pythagoras si x^2+y^2 <= 1, o normalización si se sale.
     */
    glm::vec3 getArcballVector(double x, double y, int screenWidth, int screenHeight) const {
        if (screenWidth <= 0 || screenHeight <= 0) return glm::vec3(0.0f, 0.0f, 1.0f);

        // 1. Mapear a rango [-1, 1]
        glm::vec3 P = glm::vec3(
            (float)(2.0 * x / screenWidth - 1.0),
            (float)(2.0 * y / screenHeight - 1.0),
            0.0f
        );

        // Invertir Y porque en coordenadas de ventana el origen está arriba a la izquierda
        P.y = -P.y;

        // 2. Calcular coordenada Z
        float OP_squared = P.x * P.x + P.y * P.y;
        if (OP_squared <= 1.0f) {
            P.z = std::sqrt(1.0f - OP_squared); // Teorema de Pitágoras
        } else {
            P = glm::normalize(P); // Punto más cercano en el borde del círculo
        }

        return P;
    }

    /**
     * @brief Inicia el arrastre de rotación con Arcball
     */
    void startDragging(double x, double y, int width, int height) {
        isDragging = true;
        lastX = x;
        lastY = y;
        lastMouseVec = getArcballVector(x, y, width, height);
    }

    /**
     * @brief Inicia el paneo de traslación
     */
    void startPanning(double x, double y) {
        isPanning = true;
        lastX = x;
        lastY = y;
    }

    /**
     * @brief Detiene el arrastre
     */
    void stopDragging() {
        isDragging = false;
    }

    /**
     * @brief Detiene el paneo
     */
    void stopPanning() {
        isPanning = false;
    }

    /**
     * @brief Actualiza la orientación con rotación Arcball mediante cuaterniones
     */
    void onMouseDrag(double currentX, double currentY, int width, int height) {
        if (!isDragging) return;

        // Si el usuario empieza a mover con el mouse, desactivamos la animación automática
        if (isAnimated) isAnimated = false;

        glm::vec3 currentMouseVec = getArcballVector(currentX, currentY, width, height);

        // Calcular ángulo y eje
        float dotVal = glm::dot(lastMouseVec, currentMouseVec);
        float angle = std::acos(std::min(1.0f, std::max(-1.0f, dotVal)));
        glm::vec3 axis = glm::cross(lastMouseVec, currentMouseVec);

        // Solo actualizar si hay movimiento significativo
        if (glm::length(axis) > 1e-5f) {
            axis = glm::normalize(axis);
            glm::quat deltaRot = glm::angleAxis(angle, axis);

            // Aplicar rotación: q_nueva = q_delta * q_actual
            orientation = glm::normalize(deltaRot * orientation);
        }

        lastMouseVec = currentMouseVec;
        lastX = currentX;
        lastY = currentY;
    }

    /**
     * @brief Desplaza la escena en el plano de la cámara (Pan)
     */
    void onMousePan(double currentX, double currentY, int width, int height) {
        if (!isPanning) return;

        double deltaX = (currentX - lastX) / (double)width;
        double deltaY = (currentY - lastY) / (double)height;

        // Vectores derecho y arriba de la cámara
        glm::mat4 rot = glm::mat4_cast(orientation);
        glm::vec3 right = glm::vec3(rot[0][0], rot[1][0], rot[2][0]);
        glm::vec3 up    = glm::vec3(rot[0][1], rot[1][1], rot[2][1]);

        float panSpeed = distance * 1.5f;
        target -= right * (float)(deltaX * panSpeed);
        target += up * (float)(deltaY * panSpeed);

        lastX = currentX;
        lastY = currentY;
    }

    /**
     * @brief Maneja el zoom ajustando la distancia mediante el scroll del mouse
     */
    void onScroll(double yoffset) {
        float zoomFactor = 0.2f * (distance * 0.2f + 0.2f);
        distance -= (float)yoffset * zoomFactor;
        distance = std::clamp(distance, minDistance, maxDistance);
    }

    /**
     * @brief Alterna entre el modo de cámara animada automática y el modo manual con mouse
     */
    void toggleAnimation() {
        isAnimated = !isAnimated;
        std::cout << "[Camera] Modo animación automática: " << (isAnimated ? "ACTIVADO" : "DESACTIVADO") << std::endl;
    }

    /**
     * @brief Reinicia la cámara a su orientación y distancia iniciales
     */
    void reset(float defaultDistance = 4.0f, glm::vec3 defaultTarget = glm::vec3(0.0f)) {
        orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        target = defaultTarget;
        distance = defaultDistance;
        isAnimated = false;
        std::cout << "[Camera] Cámara reiniciada." << std::endl;
    }

    /**
     * @brief Actualiza la cámara por frame (avanza la animación si está activa)
     */
    void update(float deltaTime) {
        if (isAnimated) {
            // Rotación continua alrededor del eje Y global
            glm::quat deltaRot = glm::angleAxis(animationSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
            orientation = glm::normalize(deltaRot * orientation);
        }
    }

    /**
     * @brief Obtiene la matriz de Vista (View Matrix) generada a partir del cuaternión
     *        View = Translation(0, 0, -distance) * Rotation(quat) * Translation(-target)
     */
    glm::mat4 getViewMatrix() const {
        glm::mat4 rotationMatrix = glm::mat4_cast(orientation);
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -distance));
        glm::mat4 targetMatrix = glm::translate(glm::mat4(1.0f), -target);

        return translationMatrix * rotationMatrix * targetMatrix;
    }

    /**
     * @brief Obtiene la matriz de Proyección en perspectiva
     */
    glm::mat4 getProjectionMatrix(int screenWidth, int screenHeight) const {
        float aspect = (screenHeight > 0) ? (float)screenWidth / (float)screenHeight : 1.0f;
        return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    /**
     * @brief Obtiene la posición actual de la cámara en coordenadas del mundo
     */
    glm::vec3 getPosition() const {
        // En la vista orbital: eye = target + (inverse(rot) * (0, 0, distance))
        glm::mat3 invRot = glm::transpose(glm::mat3_cast(orientation));
        return target + invRot * glm::vec3(0.0f, 0.0f, distance);
    }

    // =========================================================================
    // --- INTEGRACIÓN AUTOMÁTICA CON GLFW (CALLBACKS) ---
    // =========================================================================

    /**
     * @brief Registra callbacks en una ventana GLFW para manejar el mouse y teclado automáticamente
     */
    void registerCallbacks(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);

        // Click del mouse
        glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
            Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
            if (!cam) return;

            double x, y;
            glfwGetCursorPos(w, &x, &y);
            int width, height;
            glfwGetFramebufferSize(w, &width, &height);

            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                if (action == GLFW_PRESS) {
                    if (mods & GLFW_MOD_SHIFT) {
                        cam->startPanning(x, y);
                    } else {
                        cam->startDragging(x, y, width, height);
                    }
                } else if (action == GLFW_RELEASE) {
                    cam->stopDragging();
                    cam->stopPanning();
                }
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                if (action == GLFW_PRESS) {
                    cam->startPanning(x, y);
                } else if (action == GLFW_RELEASE) {
                    cam->stopPanning();
                }
            }
        });

        // Movimiento del cursor
        glfwSetCursorPosCallback(window, [](GLFWwindow* w, double xpos, double ypos) {
            Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
            if (!cam) return;

            int width, height;
            glfwGetFramebufferSize(w, &width, &height);

            if (cam->isDragging) {
                cam->onMouseDrag(xpos, ypos, width, height);
            } else if (cam->isPanning) {
                cam->onMousePan(xpos, ypos, width, height);
            }
        });

        // Scroll de la rueda del ratón
        glfwSetScrollCallback(window, [](GLFWwindow* w, double xoffset, double yoffset) {
            Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
            if (!cam) return;
            cam->onScroll(yoffset);
        });

        // Teclado para alternar modos o resetear
        glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS) {
                Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(w));
                if (!cam) return;

                if (key == GLFW_KEY_SPACE || key == GLFW_KEY_C) {
                    cam->toggleAnimation();
                } else if (key == GLFW_KEY_R) {
                    cam->reset();
                }
            }
        });
    }
};

// Alias para satisfacer el requerimiento literal: "struct or class named camera"
using camera = Camera;

#endif // CAMERA_H
