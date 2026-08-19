#ifndef ERIC_STRUCTURE_H
#define ERIC_STRUCTURE_H

#include <vector>
#include <map>
#include <cmath>
#include <iostream>

struct Vec3 {
    float x, y, z;
};

// Nueva estructura que empaqueta la Posición y la Normal
struct VertexData {
    Vec3 Position;
    Vec3 Normal;
};

class EricStructure {
public:
    // --- GEOMETRÍA ---
    std::vector<VertexData> G;

    // --- NIVEL 0 (Sopa de Triángulos / Apex / EBO) ---
    std::vector<unsigned int> V; 

    // --- NIVEL 1 (Adyacencias / Half-Edges Opuestos) ---
    std::vector<int> O; 

    EricStructure() {}

    // REGLAS MATEMÁTICAS DEL CHE
    int triangle(int he) const {
        return he / 3;
    }

    int next(int he) const {
        return (3 * triangle(he)) + ((he + 1) % 3);
    }

    int prev(int he) const {
        return (3 * triangle(he)) + ((he + 2) % 3);
    }

    int opposite(int he) const {
        return O[he];
    }

    // --- GENERACIÓN PARAMÉTRICA DE LA ESFERA ---
    void generateSphere(float radius, int sectors, int stacks) {
        G.clear();
        V.clear();
        O.clear();

        const float PI = 3.14159265359f;

        // 1. Generar Vértices
        for(int i = 0; i <= stacks; ++i) {
            float phi = (float)i / stacks * PI; // De 0 a PI
            for(int j = 0; j <= sectors; ++j) {
                float theta = (float)j / sectors * 2.0f * PI; // De 0 a 2PI

                float x = radius * cos(theta) * sin(phi);
                float y = radius * cos(phi);
                float z = radius * sin(theta) * sin(phi);
                
                // Para una esfera en el origen, la normal es simplemente la posición normalizada (dividida por el radio)
                Vec3 normal = {x / radius, y / radius, z / radius};
                
                G.push_back({{x, y, z}, normal});
            }
        }

        // 2. Generar Triángulos (Nivel 0 - Arreglo V)
        for(int i = 0; i < stacks; ++i) {
            for(int j = 0; j < sectors; ++j) {
                int first = (i * (sectors + 1)) + j;
                int second = first + sectors + 1;

                // Triángulo 1
                V.push_back(first);
                V.push_back(second);
                V.push_back(first + 1);

                // Triángulo 2
                V.push_back(second);
                V.push_back(second + 1);
                V.push_back(first + 1);
            }
        }

        // 3. Construir Nivel 1 (Arreglo O)
        buildLevel1();
    }

private:
    void buildLevel1() {
        O.assign(V.size(), -1);
        std::map<std::pair<int, int>, int> edgeMap;

        for(size_t he = 0; he < V.size(); ++he) {
            // El vértice de inicio es el apex del prev, el de fin es el apex de 'he'
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
        std::cout << "[CHE] Nivel 1 construido con exito. Half-edges: " << O.size() << std::endl;
    }
};

#endif
