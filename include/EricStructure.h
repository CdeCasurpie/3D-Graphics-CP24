#ifndef ERIC_STRUCTURE_H
#define ERIC_STRUCTURE_H

#include <vector>
#include <map>
#include <cmath>
#include <iostream>
#include <queue>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @struct Vec3
 * @brief Vector 3D simple para coordenadas y normales.
 */
struct Vec3 {
    float x, y, z;
};

/**
 * @struct VertexData
 * @brief Empaqueta la posicion (X,Y,Z) y la Normal (NX,NY,NZ) para enviarla a la GPU.
 */
struct VertexData {
    Vec3 Position;
    Vec3 Normal;
};

/**
 * @class EricStructure
 * @brief Implementacion de la estructura topologica Compact Half-Edge (CHE).
 *        Maneja la conectividad de la malla usando arreglos (V y O).
 */
class EricStructure {
public:
    // --- GEOMETRIA ---
    std::vector<VertexData> G; ///< Almacena los vertices fisicos (Coordenadas y Normales)

    // --- NIVEL 0 (Sopa de Triangulos / Apex) ---
    std::vector<unsigned int> V; ///< Guarda el indice del vertice al que apunta cada half-edge (Equivale al EBO de OpenGL)

    // --- NIVEL 1 (Adyacencias) ---
    std::vector<int> O; ///< Guarda el indice del half-edge opuesto a cada half-edge (-1 si es frontera)

    EricStructure() {}

    /** @brief Obtiene el ID del triangulo al que pertenece una half-edge */
    int triangle(int he) const { return he / 3; }

    /** @brief Obtiene la siguiente half-edge dentro del mismo triangulo */
    int next(int he) const { return (3 * triangle(he)) + ((he + 1) % 3); }

    /** @brief Obtiene la half-edge previa dentro del mismo triangulo */
    int prev(int he) const { return (3 * triangle(he)) + ((he + 2) % 3); }

    /** @brief Obtiene la half-edge opuesta en el triangulo vecino */
    int opposite(int he) const { return O[he]; }

    /**
     * @brief Genera un cubo parametricamente y construye el Nivel 1.
     * @param size Tamano de las aristas del cubo.
     */
    void generateCube(float size) {
        G.clear(); V.clear(); O.clear();
        float h = size / 2.0f;
        
        G.push_back({{-h, -h,  h}, {-1, -1,  1}}); // 0
        G.push_back({{ h, -h,  h}, { 1, -1,  1}}); // 1
        G.push_back({{ h,  h,  h}, { 1,  1,  1}}); // 2
        G.push_back({{-h,  h,  h}, {-1,  1,  1}}); // 3
        G.push_back({{-h, -h, -h}, {-1, -1, -1}}); // 4
        G.push_back({{ h, -h, -h}, { 1, -1, -1}}); // 5
        G.push_back({{ h,  h, -h}, { 1,  1, -1}}); // 6
        G.push_back({{-h,  h, -h}, {-1,  1, -1}}); // 7

        unsigned int indices[] = {
            0, 1, 2,  2, 3, 0, // Front
            1, 5, 6,  6, 2, 1, // Right
            7, 6, 5,  5, 4, 7, // Back
            4, 0, 3,  3, 7, 4, // Left
            4, 5, 1,  1, 0, 4, // Bottom
            3, 2, 6,  6, 7, 3  // Top
        };
        for (int i = 0; i < 36; ++i) V.push_back(indices[i]);
        buildLevel1();
    }

    /**
     * @brief Genera una esfera parametrica usando coordenadas esfericas.
     * @param radius Radio de la esfera.
     * @param sectors Resolucion horizontal.
     * @param stacks Resolucion vertical.
     */
    void generateSphere(float radius, int sectors, int stacks) {
        G.clear(); V.clear(); O.clear();
        const float PI = 3.14159265359f;

        for(int i = 0; i <= stacks; ++i) {
            float phi = (float)i / stacks * PI; 
            for(int j = 0; j <= sectors; ++j) {
                float theta = (float)j / sectors * 2.0f * PI; 
                float x = radius * cos(theta) * sin(phi);
                float y = radius * cos(phi);
                float z = radius * sin(theta) * sin(phi);
                
                Vec3 normal = {x / radius, y / radius, z / radius};
                G.push_back({{x, y, z}, normal});
            }
        }

        for(int i = 0; i < stacks; ++i) {
            for(int j = 0; j < sectors; ++j) {
                int first = (i * (sectors + 1)) + j;
                int second = first + sectors + 1;
                V.push_back(first); V.push_back(second); V.push_back(first + 1);
                V.push_back(second); V.push_back(second + 1); V.push_back(first + 1);
            }
        }
        buildLevel1();
    }

    // =========================================================================
    // --- TAREA 4: ALGORITMO DE SIMPLIFICACIÓN (QEM LOD) ---
    // =========================================================================

    bool isValid(int he) const {
        return (he >= 0 && he < V.size() && V[he] != (unsigned int)-1);
    }

    // Almacena las matrices cuádricas (4x4) de cada vértice
    std::vector<glm::mat4> Q;

    /**
     * @brief Precalcula las métricas de error cuádrico (QEM) para todos los vértices.
     * Siguiendo a Garland-Heckbert: Se calcula el plano de cada triángulo y se suma
     * su matriz fundamental a los vértices que lo componen.
     */
    void computeInitialQuadrics() {
        Q.assign(G.size(), glm::mat4(0.0f));

        for (int i = 0; i < V.size(); i += 3) {
            if (V[i] == (unsigned int)-1) continue; // Triángulo borrado

            int v1 = V[i], v2 = V[i+1], v3 = V[i+2];
            glm::vec3 p1(G[v1].Position.x, G[v1].Position.y, G[v1].Position.z);
            glm::vec3 p2(G[v2].Position.x, G[v2].Position.y, G[v2].Position.z);
            glm::vec3 p3(G[v3].Position.x, G[v3].Position.y, G[v3].Position.z);

            // Calcular la normal del plano (Unitaria)
            glm::vec3 n = glm::normalize(glm::cross(p2 - p1, p3 - p1));
            // d = -dot(n, p)
            float d = -glm::dot(n, p1);

            // Matriz fundamental del plano: Kp = p * p^T
            glm::vec4 p(n.x, n.y, n.z, d);
            glm::mat4 Kp = glm::outerProduct(p, p);

            // Sumar a los vértices del triángulo
            Q[v1] += Kp;
            Q[v2] += Kp;
            Q[v3] += Kp;
        }
    }

    /**
     * @brief Calcula el costo QEM de colapsar la arista 'he'.
     */
    float calculateEdgeCost(int he) {
        if (!isValid(he) || opposite(he) == -1) return 999999.0f; // Fronteras o inválidos
        
        int v1_idx = V[prev(he)];
        int v2_idx = V[he];

        glm::mat4 Q_new = Q[v1_idx] + Q[v2_idx];
        
        // Evaluamos el error si pusiéramos el nuevo vértice en el punto medio
        glm::vec3 p1(G[v1_idx].Position.x, G[v1_idx].Position.y, G[v1_idx].Position.z);
        glm::vec3 p2(G[v2_idx].Position.x, G[v2_idx].Position.y, G[v2_idx].Position.z);
        glm::vec3 v_mid = (p1 + p2) * 0.5f;
        glm::vec4 v_mid4(v_mid, 1.0f);

        // Costo = v^T * Q * v
        float error = glm::dot(v_mid4, Q_new * v_mid4);
        return error;
    }

    /**
     * @brief Realiza el Edge Collapse topológico y actualiza la matriz del vértice.
     */
    void collapseEdge(int he) {
        if (!isValid(he) || opposite(he) == -1) return;
        int opp = opposite(he);
        
        int v1 = V[prev(he)];
        int v2 = V[he];

        // 1. Mover físicamente v1 al punto medio
        G[v1].Position = { (G[v1].Position.x + G[v2].Position.x) / 2.0f,
                           (G[v1].Position.y + G[v2].Position.y) / 2.0f,
                           (G[v1].Position.z + G[v2].Position.z) / 2.0f };

        // Actualizar la matriz QEM del nuevo vértice fusionado
        Q[v1] = Q[v1] + Q[v2];

        // 2. Redirigir punteros de v2 a v1
        for (size_t i = 0; i < V.size(); ++i) {
            if (V[i] == v2) V[i] = v1;
        }

        // 3. Suturar los opuestos para cerrar el hueco
        int he_next = next(he), he_prev = prev(he);
        int opp_next = next(opp), opp_prev = prev(opp);

        int O_he_next = opposite(he_next), O_he_prev = opposite(he_prev);
        int O_opp_next = opposite(opp_next), O_opp_prev = opposite(opp_prev);

        if (O_he_next != -1) O[O_he_next] = O_he_prev;
        if (O_he_prev != -1) O[O_he_prev] = O_he_next;
        if (O_opp_next != -1) O[O_opp_next] = O_opp_prev;
        if (O_opp_prev != -1) O[O_opp_prev] = O_opp_next;

        // 4. Marcar triángulos como borrados
        int t1 = triangle(he), t2 = triangle(opp);
        for(int i = 0; i < 3; i++) {
            V[t1*3 + i] = (unsigned int)-1; O[t1*3 + i] = -1;
            V[t2*3 + i] = (unsigned int)-1; O[t2*3 + i] = -1;
        }
    }

    struct EdgeRecord {
        int he;
        float cost;
        bool operator>(const EdgeRecord& other) const { return cost > other.cost; }
    };

    /**
     * @brief Bucle principal de simplificación usando una Cola de Prioridad.
     */
    void simplifyMesh(int targetTriangles) {
        computeInitialQuadrics(); // Inicializar QEM

        // Contar triángulos válidos actuales
        int currentTriangles = 0;
        for (size_t i = 0; i < V.size(); i += 3) {
            if (V[i] != (unsigned int)-1) currentTriangles++;
        }

        std::priority_queue<EdgeRecord, std::vector<EdgeRecord>, std::greater<EdgeRecord>> pq;

        // Llenar la cola inicial
        for (int he = 0; he < V.size(); ++he) {
            if (isValid(he) && opposite(he) != -1) {
                // Para evitar duplicados, solo metemos half-edges donde he < opposite(he)
                if (he < opposite(he)) {
                    pq.push({he, calculateEdgeCost(he)});
                }
            }
        }

        // Bucle de colapsos
        while (!pq.empty() && currentTriangles > targetTriangles) {
            EdgeRecord record = pq.top();
            pq.pop();

            // Si el edge sigue siendo válido (no colapsó indirectamente)
            if (isValid(record.he) && opposite(record.he) != -1) {
                // Verificar si los vértices siguen existiendo y no forman triángulos degenerados
                int v1 = V[prev(record.he)];
                int v2 = V[record.he];
                if (v1 != v2) { // Evita colapsar aristas ya fusionadas
                    collapseEdge(record.he);
                    currentTriangles -= 2; // Cada colapso elimina 2 triángulos (el diamante interior)
                }
            }
        }
        
        compactArrays(); // Limpiar la memoria
    }

private:
    /**
     * @brief Remueve físicamente los -1 del array V y E para renderizar limpio.
     */
    void compactArrays() {
        std::vector<unsigned int> newV;
        for(size_t i = 0; i < V.size(); i += 3) {
            if(V[i] != (unsigned int)-1) {
                newV.push_back(V[i]);
                newV.push_back(V[i+1]);
                newV.push_back(V[i+2]);
            }
        }
        V = newV;
        // Al compactar V, el nivel O original ya no cuadra con los índices, 
        // así que lo reconstruimos desde cero para que el render final esté estable.
        buildLevel1();
    }

private:
    /**
     * @brief Construye el arreglo 'O' (Opposites) emparejando las half-edges.
     */
    void buildLevel1() {
        O.assign(V.size(), -1);
        std::map<std::pair<int, int>, int> edgeMap;

        for(size_t he = 0; he < V.size(); ++he) {
            int v_start = V[prev(he)];
            int v_end = V[he];
            std::pair<int, int> reverseEdge = {v_end, v_start};
            
            if (edgeMap.find(reverseEdge) != edgeMap.end()) {
                int opp_he = edgeMap[reverseEdge];
                O[he] = opp_he;
                O[opp_he] = he;
                edgeMap.erase(reverseEdge);
            } else {
                edgeMap[{v_start, v_end}] = he;
            }
        }
    }
};

#endif
