cmake_minimum_required(VERSION 3.10)
project(HelloTriangle)

set(CMAKE_CXX_STANDARD 17)

# Find GLFW and OpenGL
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Include glad headers
include_directories(glad/include)

# Add executable
add_executable(HelloTriangle first.cpp glad/src/gl.c)

# Link libraries
target_link_libraries(HelloTriangle glfw OpenGL::GL)
