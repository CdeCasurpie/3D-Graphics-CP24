cmake_minimum_required(VERSION 3.10)
project(3DGraphicsTutorials)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(glad/include)

# Target 1: Hello Triangle
add_executable(HelloTriangle HelloTriangle/main.cpp glad/src/gl.c)
target_link_libraries(HelloTriangle glfw OpenGL::GL)

# Target 2: Sierpinski - Chaos Game
add_executable(ChaosAlgorithm Sierpinski/ChaosAlgorithm/main.cpp glad/src/gl.c)
target_link_libraries(ChaosAlgorithm glfw OpenGL::GL)

# Target 3: Sierpinski - Recursive
add_executable(RecursiveAlgorithm Sierpinski/RecursiveAlgorithm/main.cpp glad/src/gl.c)
target_link_libraries(RecursiveAlgorithm glfw OpenGL::GL)
