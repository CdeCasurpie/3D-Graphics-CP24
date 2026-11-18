cmake_minimum_required(VERSION 3.10)
project(3DGraphicsTutorials)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(glad/include)
include_directories(include)
include_directories(include/imgui)

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

# Project 1: Unified Interactive 3D Scene Viewer
add_executable(Project1 
    src/Project1/main.cpp 
    glad/src/gl.c
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_glfw.cpp
    src/imgui/imgui_impl_opengl3.cpp
)
target_link_libraries(Project1 glfw OpenGL::GL)

add_executable(AdvancedSamplers
    src/AdvancedSamplers/main.cpp glad/src/gl.c
    src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_glfw.cpp src/imgui/imgui_impl_opengl3.cpp
)
target_link_libraries(AdvancedSamplers glfw OpenGL::GL)

# Task 13: primary-ray depth image (CPU tracing; OpenGL only displays pixels).
find_package(Threads REQUIRED)
add_executable(ShootingRays
    src/ShootingRays/main.cpp
    glad/src/gl.c
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_glfw.cpp
    src/imgui/imgui_impl_opengl3.cpp
)
target_link_libraries(ShootingRays glfw OpenGL::GL Threads::Threads)

# Task 15 has an independent CPU-only / optional-CUDA standalone build too.
add_subdirectory(src/SimplePathTracer)

# Portable CUDA arithmetic checks can run on a laptop without the toolkit/GPU.
add_executable(CudaRayMathTests EXCLUDE_FROM_ALL src/ShootingRaysCUDA/host_tests.cpp)
target_link_libraries(CudaRayMathTests Threads::Threads)

option(ENABLE_CUDA_RAYCASTER "Build Task 14 (requires NVIDIA CUDA toolkit and GPU)" OFF)
if(ENABLE_CUDA_RAYCASTER)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "Task 14 requires CMake >= 3.24 (native GPU architecture detection)")
    endif()
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "Task 14 needs nvcc. Install/load the CUDA toolkit, or configure with -DENABLE_CUDA_RAYCASTER=OFF. See src/ShootingRaysCUDA/README.md")
    endif()
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native)
    endif()
    cmake_policy(SET CMP0104 NEW)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    add_executable(ShootingRaysCUDA
        src/ShootingRaysCUDA/main.cpp
        src/ShootingRaysCUDA/CudaRayCaster.cu
    )
    set_target_properties(ShootingRaysCUDA PROPERTIES
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
    )
    target_link_libraries(ShootingRaysCUDA CUDA::cudart Threads::Threads)
    # Favor reproducible CPU/GPU comparisons over aggressive float contraction.
    target_compile_options(ShootingRaysCUDA PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>)
endif()
