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

# Target 5: Sierpinski - Tetrahedron 3D (Framework del profesor)
find_package(gproshan QUIET)
if(gproshan_FOUND)
    include_directories(SYSTEM ${gproshan_INCLUDE_DIR})
    add_executable(Tetrahedron src/Sierpinski/Tetrahedron/main.cpp)
    target_link_libraries(Tetrahedron gproshan)
endif()
