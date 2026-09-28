cmake_minimum_required(VERSION 3.10)
project(3DGraphicsTutorials)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(glad/include)
include_directories(include)

# Target 1: Hello Triangle
add_executable(HelloTriangle src/HelloTriangle/main.cpp glad/src/gl.c)
target_link_libraries(HelloTriangle glfw OpenGL::GL)

# Target 2: Sierpinski - Chaos Game
add_executable(ChaosAlgorithm src/Sierpinski/ChaosAlgorithm/main.cpp glad/src/gl.c)
target_link_libraries(ChaosAlgorithm glfw OpenGL::GL)

# Target 3: Sierpinski - Recursive
add_executable(RecursiveAlgorithm src/Sierpinski/RecursiveAlgorithm/main.cpp glad/src/gl.c)
target_link_libraries(RecursiveAlgorithm glfw OpenGL::GL)

# Target 4: Sphere CHE
add_executable(SphereCHE src/SphereCHE/main.cpp glad/src/gl.c)
target_link_libraries(SphereCHE glfw OpenGL::GL)

# Target 5: Transformaciones (GLM)
add_executable(Transformaciones src/Transformaciones/main.cpp glad/src/gl.c)
target_link_libraries(Transformaciones glfw OpenGL::GL)

# Target 6: Camara (Ejercicio de camara animada)
add_executable(Camara src/Camara/main.cpp glad/src/gl.c)
target_link_libraries(Camara glfw OpenGL::GL)

# Target 7: Simplificacion (Quadric Error Metrics LOD)
add_executable(Simplificacion src/Simplificacion/main.cpp glad/src/gl.c)
target_link_libraries(Simplificacion glfw OpenGL::GL)

# Target 8: Sierpinski - Tetrahedron 3D (Framework del profesor)
find_package(gproshan QUIET)
if(gproshan_FOUND)
    include_directories(SYSTEM ${gproshan_INCLUDE_DIR})
    add_executable(Tetrahedron src/Sierpinski/Tetrahedron/main.cpp)
    target_link_libraries(Tetrahedron gproshan)
endif()
add_executable(FastMarching src/FastMarching/main.cpp glad/src/gl.c)
target_link_libraries(FastMarching glfw OpenGL::GL)

# Target 9: Arcball Camera (Quaternions + Mouse Drag/Scroll)
add_executable(ArcballCamera src/ArcballCamera/main.cpp glad/src/gl.c)
target_link_libraries(ArcballCamera glfw OpenGL::GL)

# Target 10: Iluminacion (Phong & Blinn-Phong con Gouraud & Phong Shading)
add_executable(Iluminacion src/Iluminacion/main.cpp glad/src/gl.c)
target_link_libraries(Iluminacion glfw OpenGL::GL)

# Target 11: Shadow Mapping (2-pass)
add_executable(ShadowMapping src/ShadowMapping/main.cpp glad/src/gl.c)
target_link_libraries(ShadowMapping glfw OpenGL::GL)

# Target 12: Textures & Lighting Maps
add_executable(TexturesAndMaps src/TexturesAndMaps/main.cpp glad/src/gl.c)
target_link_libraries(TexturesAndMaps glfw OpenGL::GL)

# Target 13: Model Loading
add_executable(ModelLoading src/ModelLoading/main.cpp glad/src/gl.c)
target_link_libraries(ModelLoading glfw OpenGL::GL)
