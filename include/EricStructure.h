#ifndef ERIC_STRUCTURE_H
#define ERIC_STRUCTURE_H

#include <vector>
#include <map>
#include <cmath>
#include <iostream>

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
