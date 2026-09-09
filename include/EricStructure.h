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

    // =========================================================================
    // --- TAREA 4: ALGORITMO DE SIMPLIFICACIÓN (LOD) ---
    // =========================================================================

    /**
     * @brief Verifica si un half-edge sigue siendo válido (no ha sido eliminado).
     * Como no podemos borrar elementos del array V sin romper los índices, 
     * usaremos el valor V[he] = -1 (o UINT_MAX) para marcar triángulos borrados.
     */
    bool isValid(int he) const {
        return (he >= 0 && he < V.size() && V[he] != (unsigned int)-1);
    }

    /**
     * @brief Calcula el costo matemático de colapsar la arista 'he'.
     * @param he Índice del half-edge a evaluar.
     * @return El error estimado (Quadric Error Metric u otro).
     */
    float calculateEdgeCost(int he) {
        if (!isValid(he) || opposite(he) == -1) return 999999.0f; // Ignorar aristas frontera o borradas
        
        int v1_idx = V[prev(he)];
        int v2_idx = V[he];

        // --- TU TAREA: IMPLEMENTAR QUADRIC ERROR METRIC (QEM) ---
        // Por ahora, te dejo un costo "tonto" basado en la longitud de la arista (Shortest Edge First).
        // Deberías cambiar esto por el error cuádrico de Garland-Heckbert basado en matrices 4x4.
        
        Vec3 p1 = G[v1_idx].Position;
        Vec3 p2 = G[v2_idx].Position;
        float dist = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2));
        
        return dist; 
    }

    /**
     * @brief Realiza un Edge Collapse topológico en L1. (Colapsa v2 hacia v1).
     * @param he El half-edge que será colapsado.
     */
    void collapseEdge(int he) {
        if (!isValid(he) || opposite(he) == -1) return;

        int opp = opposite(he);
        
        // Vértices de la arista
        int v1 = V[prev(he)];
        int v2 = V[he];

        // 1. Mover físicamente v1 al centro entre v1 y v2 (o simplemente dejar a v1 donde está y hacer que absorba a v2)
        G[v1].Position = { (G[v1].Position.x + G[v2].Position.x) / 2.0f,
                           (G[v1].Position.y + G[v2].Position.y) / 2.0f,
                           (G[v1].Position.z + G[v2].Position.z) / 2.0f };

        // 2. Todos los half-edges en la malla que apuntaban a 'v2', ahora deben apuntar a 'v1'.
        // (Esto es ineficiente O(N), pero funciona perfecto para L1 sin estructuras adicionales).
        for (size_t i = 0; i < V.size(); ++i) {
            if (V[i] == v2) {
                V[i] = v1;
            }
        }

        // 3. Suturar los triángulos vecinos (Suturar los huecos dejados por la eliminación del rombo central)
        int he_next = next(he);
        int he_prev = prev(he);
        int opp_next = next(opp);
        int opp_prev = prev(opp);

        int O_he_next = opposite(he_next);
        int O_he_prev = opposite(he_prev);
        int O_opp_next = opposite(opp_next);
        int O_opp_prev = opposite(opp_prev);

        // Conectar los de la izquierda
        if (O_he_next != -1) O[O_he_next] = O_he_prev;
        if (O_he_prev != -1) O[O_he_prev] = O_he_next;

        // Conectar los de la derecha
        if (O_opp_next != -1) O[O_opp_next] = O_opp_prev;
        if (O_opp_prev != -1) O[O_opp_prev] = O_opp_next;

        // 4. Marcar los 6 half-edges de los dos triángulos (he y opp) como eliminados (-1)
        int t1 = triangle(he);
        int t2 = triangle(opp);
        for(int i = 0; i < 3; i++) {
            V[t1*3 + i] = (unsigned int)-1; O[t1*3 + i] = -1;
            V[t2*3 + i] = (unsigned int)-1; O[t2*3 + i] = -1;
        }
    }

    /**
     * @brief Simplifica la malla iterativamente.
     * @param targetTriangles Número de triángulos que queremos dejar al final.
     */
    void simplifyMesh(int targetTriangles) {
        // --- TU TAREA: IMPLEMENTAR EL BUCLE DE SIMPLIFICACIÓN ---
        // 1. Recorrer todos los half-edges válidos y meterlos en una std::priority_queue ordenada por calculateEdgeCost().
        // 2. Extraer el más barato, verificar que siga siendo válido, y llamar a collapseEdge().
        // 3. (Opcional pero ideal): Recalcular el costo de los half-edges vecinos afectados y actualizar la cola.
        // 4. Repetir hasta que queden 'targetTriangles'.
        // 5. Finalmente, limpiar los arrays V y O para quitar los '-1' (Compactación de la memoria).
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
