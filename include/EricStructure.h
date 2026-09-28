#ifndef ERIC_STRUCTURE_H
#define ERIC_STRUCTURE_H

#include <vector>
#include <map>
#include <cmath>
#include <iostream>
#include <queue>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * @struct Vec3
 * @brief Simple 3D vector for coordinates and normals.
 */
struct Vec3 {
    float x, y, z;
};

/**
 * @struct Vec2
 * @brief Simple 2D vector for texture coordinates.
 */
struct Vec2 {
    float x, y;
};

/**
 * @struct VertexData
 * @brief Packs Position (X,Y,Z), Normal (NX,NY,NZ) and TexCoords (U,V) to be sent to the GPU.
 */
struct VertexData {
    Vec3 Position;
    Vec3 Normal;
    Vec2 TexCoords;
};

/**
 * @class EricStructure
 * @brief Implementation of the Compact Half-Edge (CHE) topological structure.
 *        Manages mesh connectivity using arrays (V and O).
 */
class EricStructure {
public:
    // --- GEOMETRY ---
    std::vector<VertexData> G; ///< Stores physical vertices (Coordinates and Normals)

    // --- LEVEL 0 (Triangle Soup / Apex) ---
    std::vector<unsigned int> V; ///< Stores the vertex index that each half-edge points to (Equivalent to OpenGL EBO)

    // --- LEVEL 1 (Adjacencies) ---
    std::vector<int> O; ///< Stores the opposite half-edge index for each half-edge (-1 if boundary)

    EricStructure() {}

    /** @brief Gets the ID of the triangle to which a half-edge belongs */
    int triangle(int he) const { return he / 3; }

    /** @brief Gets the next half-edge within the same triangle */
    int next(int he) const { return (3 * triangle(he)) + ((he + 1) % 3); }

    /** @brief Gets the previous half-edge within the same triangle */
    int prev(int he) const { return (3 * triangle(he)) + ((he + 2) % 3); }

    /** @brief Gets the opposite half-edge in the adjacent triangle */
    int opposite(int he) const { return O[he]; }

    /**
     * @brief Generates a cube with 24 vertices for proper texture and normal mapping per face.
     *        Note: This breaks the CHE manifold property (creates 6 disconnected quads).
     * @param size Size of the cube.
     */
    void generateTexturedCube(float size) {
        G.clear(); V.clear(); O.clear();
        float h = size / 2.0f;
        
        // Front face (Z = h)
        G.push_back({{-h, -h,  h}, {0, 0, 1}, {0.0f, 0.0f}}); // 0
        G.push_back({{ h, -h,  h}, {0, 0, 1}, {1.0f, 0.0f}}); // 1
        G.push_back({{ h,  h,  h}, {0, 0, 1}, {1.0f, 1.0f}}); // 2
        G.push_back({{-h,  h,  h}, {0, 0, 1}, {0.0f, 1.0f}}); // 3
        
        // Right face (X = h)
        G.push_back({{ h, -h,  h}, {1, 0, 0}, {0.0f, 0.0f}}); // 4
        G.push_back({{ h, -h, -h}, {1, 0, 0}, {1.0f, 0.0f}}); // 5
        G.push_back({{ h,  h, -h}, {1, 0, 0}, {1.0f, 1.0f}}); // 6
        G.push_back({{ h,  h,  h}, {1, 0, 0}, {0.0f, 1.0f}}); // 7
        
        // Back face (Z = -h)
        G.push_back({{ h, -h, -h}, {0, 0, -1}, {0.0f, 0.0f}}); // 8
        G.push_back({{-h, -h, -h}, {0, 0, -1}, {1.0f, 0.0f}}); // 9
        G.push_back({{-h,  h, -h}, {0, 0, -1}, {1.0f, 1.0f}}); // 10
        G.push_back({{ h,  h, -h}, {0, 0, -1}, {0.0f, 1.0f}}); // 11
        
        // Left face (X = -h)
        G.push_back({{-h, -h, -h}, {-1, 0, 0}, {0.0f, 0.0f}}); // 12
        G.push_back({{-h, -h,  h}, {-1, 0, 0}, {1.0f, 0.0f}}); // 13
        G.push_back({{-h,  h,  h}, {-1, 0, 0}, {1.0f, 1.0f}}); // 14
        G.push_back({{-h,  h, -h}, {-1, 0, 0}, {0.0f, 1.0f}}); // 15
        
        // Top face (Y = h)
        G.push_back({{-h,  h,  h}, {0, 1, 0}, {0.0f, 0.0f}}); // 16
        G.push_back({{ h,  h,  h}, {0, 1, 0}, {1.0f, 0.0f}}); // 17
        G.push_back({{ h,  h, -h}, {0, 1, 0}, {1.0f, 1.0f}}); // 18
        G.push_back({{-h,  h, -h}, {0, 1, 0}, {0.0f, 1.0f}}); // 19
        
        // Bottom face (Y = -h)
        G.push_back({{-h, -h, -h}, {0, -1, 0}, {0.0f, 0.0f}}); // 20
        G.push_back({{ h, -h, -h}, {0, -1, 0}, {1.0f, 0.0f}}); // 21
        G.push_back({{ h, -h,  h}, {0, -1, 0}, {1.0f, 1.0f}}); // 22
        G.push_back({{-h, -h,  h}, {0, -1, 0}, {0.0f, 1.0f}}); // 23

        unsigned int indices[] = {
            0, 1, 2,  2, 3, 0, // Front
            4, 5, 6,  6, 7, 4, // Right
            8, 9, 10, 10, 11, 8, // Back
            12, 13, 14, 14, 15, 12, // Left
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20  // Bottom
        };
        for (int i = 0; i < 36; ++i) V.push_back(indices[i]);
        buildLevel1();
    }

    /**
     * @brief Genera un cubo parametricamente y construye el Nivel 1.
     * @param size Tamano de las aristas del cubo.
     */
    void generateCube(float size) {
        G.clear(); V.clear(); O.clear();
        float h = size / 2.0f;
        
        G.push_back({{-h, -h,  h}, {-1, -1,  1}, {0, 0}}); // 0
        G.push_back({{ h, -h,  h}, { 1, -1,  1}, {0, 0}}); // 1
        G.push_back({{ h,  h,  h}, { 1,  1,  1}, {0, 0}}); // 2
        G.push_back({{-h,  h,  h}, {-1,  1,  1}, {0, 0}}); // 3
        G.push_back({{-h, -h, -h}, {-1, -1, -1}, {0, 0}}); // 4
        G.push_back({{ h, -h, -h}, { 1, -1, -1}, {0, 0}}); // 5
        G.push_back({{ h,  h, -h}, { 1,  1, -1}, {0, 0}}); // 6
        G.push_back({{-h,  h, -h}, {-1,  1, -1}, {0, 0}}); // 7

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
                float u = (float)j / sectors;
                float v = (float)i / stacks;
                G.push_back({{x, y, z}, normal, {u, v}});
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

    /**
     * @brief Genera un Toroide (Dona) paramétrico. Puede tener baches (bumped) para pruebas.
     */
    void generateTorus(float R, float r, int sectorCount, int stackCount, bool bumped = false) {
        G.clear();
        V.clear();

        float sectorStep = 2 * M_PI / sectorCount;
        float stackStep = 2 * M_PI / stackCount;

        // Generar vértices SIN duplicar la costura final
        for(int i = 0; i < stackCount; ++i) {
            float u = i * stackStep;
            for(int j = 0; j < sectorCount; ++j) {
                float v = j * sectorStep;

                float current_r = r;
                if (bumped) {
                    current_r += 0.05f * sinf(8.0f * u) * cosf(16.0f * v);
                }

                float x = (R + current_r * cosf(u)) * cosf(v);
                float y = current_r * sinf(u);
                float z = (R + current_r * cosf(u)) * sinf(v);
                
                float nx = cosf(u) * cosf(v);
                float ny = sinf(u);
                float nz = cosf(u) * sinf(v);

                float texU = (float)j / sectorCount;
                float texV = (float)i / stackCount;

                G.push_back({{x, y, z}, {nx, ny, nz}, {texU, texV}});
            }
        }

        // Generar topología completamente unida (soldada)
        for(int i = 0; i < stackCount; ++i) {
            int next_i = (i + 1) % stackCount;
            for(int j = 0; j < sectorCount; ++j) {
                int next_j = (j + 1) % sectorCount;
                
                int k1 = i * sectorCount + j;
                int k1_next = i * sectorCount + next_j;
                int k2 = next_i * sectorCount + j;
                int k2_next = next_i * sectorCount + next_j;

                V.push_back(k1); V.push_back(k2); V.push_back(k1_next);
                V.push_back(k1_next); V.push_back(k2); V.push_back(k2_next);
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

        for (size_t i = 0; i < V.size(); i += 3) {
            if (V[i] == (unsigned int)-1) continue; // Triángulo borrado

            int v1 = V[i], v2 = V[i+1], v3 = V[i+2];
            glm::vec3 p1(G[v1].Position.x, G[v1].Position.y, G[v1].Position.z);
            glm::vec3 p2(G[v2].Position.x, G[v2].Position.y, G[v2].Position.z);
            glm::vec3 p3(G[v3].Position.x, G[v3].Position.y, G[v3].Position.z);

            // Calcular la normal del plano (Unitaria)
            glm::vec3 n = glm::normalize(glm::cross(p2 - p1, p3 - p1));
            // Si el triángulo es degenerado, saltarlo
            if (std::isnan(n.x)) continue;

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
     * @brief Check Link Condition to avoid non-manifold topology during edge collapse.
     */
    bool checkLinkCondition(int v1, int v2) const {
        std::vector<int> N1;
        std::vector<int> N2;
        
        for(size_t i = 0; i < V.size(); i += 3) {
            if(V[i] == (unsigned int)-1) continue;
            int tv0 = V[i], tv1 = V[i+1], tv2 = V[i+2];
            if(tv0 == v1) { N1.push_back(tv1); N1.push_back(tv2); }
            if(tv1 == v1) { N1.push_back(tv0); N1.push_back(tv2); }
            if(tv2 == v1) { N1.push_back(tv0); N1.push_back(tv1); }
            
            if(tv0 == v2) { N2.push_back(tv1); N2.push_back(tv2); }
            if(tv1 == v2) { N2.push_back(tv0); N2.push_back(tv2); }
            if(tv2 == v2) { N2.push_back(tv0); N2.push_back(tv1); }
        }
        
        std::sort(N1.begin(), N1.end());
        N1.erase(std::unique(N1.begin(), N1.end()), N1.end());
        std::sort(N2.begin(), N2.end());
        N2.erase(std::unique(N2.begin(), N2.end()), N2.end());
        
        int common = 0;
        for(int n : N1) {
            if(std::binary_search(N2.begin(), N2.end(), n)) {
                common++;
            }
        }
        
        return (common <= 2);
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
     * @brief Main simplification loop using a Priority Queue.
     * @param targetTriangles The target number of triangles to reach.
     */
    void simplifyMesh(int targetTriangles) {
        computeInitialQuadrics(); // Initialize QEM

        // Count current valid triangles
        int currentTriangles = 0;
        for (size_t i = 0; i < V.size(); i += 3) {
            if (V[i] != (unsigned int)-1) currentTriangles++;
        }

        std::priority_queue<EdgeRecord, std::vector<EdgeRecord>, std::greater<EdgeRecord>> pq;

        // Populate initial queue
        for (int he = 0; he < V.size(); ++he) {
            if (isValid(he) && opposite(he) != -1) {
                // To avoid duplicates, only add half-edges where he < opposite(he)
                if (he < opposite(he)) {
                    pq.push({he, calculateEdgeCost(he)});
                }
            }
        }

        // Collapse loop
        while (!pq.empty() && currentTriangles > targetTriangles) {
            EdgeRecord record = pq.top();
            pq.pop();

            // If the edge is still valid (not collapsed indirectly)
            if (isValid(record.he) && opposite(record.he) != -1) {
                int v1 = V[prev(record.he)];
                int v2 = V[record.he];
                
                if (v1 != v2) { // Avoid collapsing already merged edges
                    if (!checkLinkCondition(v1, v2)) {
                        continue; // Skip this collapse to preserve manifold topology
                    }

                    // TRICK: Lazy Update. Since we can't update costs inside the priority_queue,
                    // we recalculate the cost NOW. If it changed significantly (because vertices moved 
                    // in previous collapses), we re-insert it with the correct cost and ignore it for now.
                    float currentCost = calculateEdgeCost(record.he);
                    if (currentCost > record.cost + 0.0001f) {
                        pq.push({record.he, currentCost});
                        continue;
                    }

                    collapseEdge(record.he);
                    currentTriangles -= 2; // Each collapse removes 2 triangles
                }
            }
        }
        
        compactArrays(); // Clean up memory
        recalculateNormals(); // Update normals for the new geometry
    }

    /**
     * @brief Recalculates vertex normals by averaging face normals.
     */
    void recalculateNormals() {
        for(auto& v : G) {
            v.Normal = {0.0f, 0.0f, 0.0f};
        }
        for(size_t i = 0; i < V.size(); i += 3) {
            if(V[i] == (unsigned int)-1) continue;
            int v1 = V[i], v2 = V[i+1], v3 = V[i+2];
            glm::vec3 p1(G[v1].Position.x, G[v1].Position.y, G[v1].Position.z);
            glm::vec3 p2(G[v2].Position.x, G[v2].Position.y, G[v2].Position.z);
            glm::vec3 p3(G[v3].Position.x, G[v3].Position.y, G[v3].Position.z);
            glm::vec3 n = glm::cross(p2 - p1, p3 - p1);
            if(!std::isnan(n.x) && glm::length(n) > 1e-6f) {
                G[v1].Normal.x += n.x; G[v1].Normal.y += n.y; G[v1].Normal.z += n.z;
                G[v2].Normal.x += n.x; G[v2].Normal.y += n.y; G[v2].Normal.z += n.z;
                G[v3].Normal.x += n.x; G[v3].Normal.y += n.y; G[v3].Normal.z += n.z;
            }
        }
        for(auto& v : G) {
            glm::vec3 n(v.Normal.x, v.Normal.y, v.Normal.z);
            if(glm::length(n) > 1e-6f) {
                n = glm::normalize(n);
                v.Normal = {n.x, n.y, n.z};
            }
        }
    }

    // =========================================================================
    // --- TAREA 6: FAST MARCHING ALGORITHM (FMM) ---
    // =========================================================================

    /**
     * @brief Resuelve la ecuación Eikonal en un triángulo para aproximar la distancia superficial.
     * Basado en la Sección 3 del paper de Parametric Surfaces.
     */
    float eikonalUpdate(int v0, int v1, int v2, float t1, float t2) {
        Vec3 p0 = G[v0].Position;
        Vec3 p1 = G[v1].Position;
        Vec3 p2 = G[v2].Position;

        glm::vec3 x1(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
        glm::vec3 x2(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z);

        // Matriz X^T * X
        float a = glm::dot(x1, x1);
        float b = glm::dot(x1, x2);
        float c = glm::dot(x2, x2);
        float det = a * c - b * b;

        // Fallback a Dijkstra normal si el triángulo está muy degenerado
        if (det < 1e-6f) {
            return std::min(t1 + sqrt(a), t2 + sqrt(c));
        }

        // Matriz inversa Q = (X^T * X)^-1
        float invDet = 1.0f / det;
        float q11 = c * invDet;
        float q12 = -b * invDet;
        float q21 = -b * invDet;
        float q22 = a * invDet;

        // Ecuación cuadrática: A * t0^2 - 2B * t0 + C - 1 = 0
        float A = q11 + q12 + q21 + q22;
        float B = t1 * (q11 + q21) + t2 * (q12 + q22);
        float C = t1 * t1 * q11 + t1 * t2 * q12 + t2 * t1 * q21 + t2 * t2 * q22;

        float discriminant = B * B - A * (C - 1.0f);

        if (discriminant >= 0.0f) {
            float t0 = (B + sqrt(discriminant)) / A;

            // Condición de consistencia: Q * (t - t0*1) < 0
            float u1 = t1 - t0;
            float u2 = t2 - t0;
            float check1 = q11 * u1 + q12 * u2;
            float check2 = q21 * u1 + q22 * u2;

            if (check1 < 0.0f && check2 < 0.0f) {
                return t0; // El frente de onda viene desde adentro del triángulo. ¡Éxito!
            }
        }

        // Fallback a Dijkstra normal si falla la condición (Dijkstra por los bordes)
        return std::min(t1 + sqrt(a), t2 + sqrt(c));
    }

    struct FMMNode {
        int vertex;
        float distance;
        bool operator>(const FMMNode& other) const { return distance > other.distance; }
    };

    /**
     * @brief Computes geodesic distances from a source vertex to all other vertices.
     * @param sourceVertex The index of the starting vertex.
     * @return A vector of geodesic distances for each vertex.
     */
    std::vector<float> fastMarching(int sourceVertex) {
        enum State { FAR, FRONT, FROZEN };
        std::vector<float> distances(G.size(), 999999.0f);
        std::vector<State> states(G.size(), FAR);

        std::priority_queue<FMMNode, std::vector<FMMNode>, std::greater<FMMNode>> pq;

        distances[sourceVertex] = 0.0f;
        states[sourceVertex] = FRONT;
        pq.push({sourceVertex, 0.0f});

        // Pre-compute vertex connectivity to accelerate iteration (Vertices to Half-Edges)
        std::vector<std::vector<int>> vertexToHalfEdges(G.size());
        for (int i = 0; i < V.size(); ++i) {
            if (V[i] != (unsigned int)-1) {
                vertexToHalfEdges[V[i]].push_back(i);
            }
        }

        while (!pq.empty()) {
            FMMNode current = pq.top();
            pq.pop();

            int u = current.vertex;

            // Skip if already frozen (due to lazy updates in the priority queue)
            if (states[u] == FROZEN) continue;
            states[u] = FROZEN;

            for (int he : vertexToHalfEdges[u]) {
                int he_prev = prev(he);
                int he_next = next(he);
                int v1 = V[he_prev]; 
                int v2 = V[he_next]; 

                // 1. Dijkstra Edge Relaxation (1D)
                // Allows the wavefront to propagate along the edges.
                Vec3 pu = G[u].Position, pv1 = G[v1].Position, pv2 = G[v2].Position;
                
                float edgeDist1 = distances[u] + sqrt(pow(pu.x-pv1.x,2) + pow(pu.y-pv1.y,2) + pow(pu.z-pv1.z,2));
                if (edgeDist1 < distances[v1]) {
                    distances[v1] = edgeDist1;
                    states[v1] = FRONT;
                    pq.push({v1, edgeDist1});
                }

                float edgeDist2 = distances[u] + sqrt(pow(pu.x-pv2.x,2) + pow(pu.y-pv2.y,2) + pow(pu.z-pv2.z,2));
                if (edgeDist2 < distances[v2]) {
                    distances[v2] = edgeDist2;
                    states[v2] = FRONT;
                    pq.push({v2, edgeDist2});
                }

                // 2. Fast Marching Eikonal Update (2D)
                // Finds a shortcut across the triangle face if two vertices are already frozen.
                if (states[v1] != FROZEN && states[v2] == FROZEN) {
                    float newDist = eikonalUpdate(v1, u, v2, distances[u], distances[v2]);
                    if (newDist < distances[v1]) {
                        distances[v1] = newDist;
                        states[v1] = FRONT;
                        pq.push({v1, newDist});
                    }
                }
                
                if (states[v2] != FROZEN && states[v1] == FROZEN) {
                    float newDist = eikonalUpdate(v2, u, v1, distances[u], distances[v1]);
                    if (newDist < distances[v2]) {
                        distances[v2] = newDist;
                        states[v2] = FRONT;
                        pq.push({v2, newDist});
                    }
                }
            }
        }

        return distances;
    }

private:
    /**
     * @brief Remueve físicamente los -1 del array V y E para renderizar limpio.
     */
    void compactArrays() {
        std::vector<unsigned int> newV;
        for(size_t i = 0; i < V.size(); i += 3) {
            if(V[i] != (unsigned int)-1) {
                if (V[i] == V[i+1] || V[i+1] == V[i+2] || V[i] == V[i+2]) continue;
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
