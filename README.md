# 3D Graphics - OpenGL Framework and CHE Implementation

This repository contains the assignments and a custom OpenGL framework developed for the 3D Graphics course. It demonstrates fundamental computer graphics concepts, including the implementation of the Compact Half-Edge (CHE) topological data structure for efficient mesh representation.

## Technologies Used
* **C++17**
* **OpenGL 3.3 Core Profile**
* **GLFW**: Window management and input handling
* **GLAD**: OpenGL function pointer loading
* **CMake**: Build system generation
* **Make**: Build automation

---

## Project Structure

* `include/`: Contains the core framework wrappers (`Window`, `Shader`, `VAO`, `VBO`, `EBO`) and the data structure `EricStructure.h` (CHE).
* `src/`: Contains the specific implementation for each assignment.
  * `HelloTriangle/`: Basic OpenGL setup and rendering pipeline test.
  * `Sierpinski/`: 2D fractal rendering algorithms (Chaos Game and Recursive).
  * `SphereCHE/`: Main implementation of the Compact Half-Edge structure for rendering parametric spheres and cubes.

---

## Build and Execution Instructions

The project uses CMake to generate the build files, but a Makefile is provided in the root directory for convenience.

### 1. Build the Project
To configure CMake and build all executable targets, run the following command in the root directory:

```bash
make all
```

### 2. Run the Applications
After building, you can execute any of the generated targets using the corresponding make command:

**CHE Data Structure (Main Assignment):**
```bash
make run-SphereCHE
```

**Previous Assignments:**
```bash
make run-HelloTriangle
make run-ChaosAlgorithm
make run-RecursiveAlgorithm
```

---

## Application Controls (SphereCHE)

When running `make run-SphereCHE`, use the following keyboard controls to interact with the application:

**Geometry Selection:**
* `C` : Load Cube mesh
* `V` : Load Parametric Sphere mesh

**Rendering Modes:**
* `1` : Point Cloud (`GL_POINTS`)
* `2` : Wireframe / Mesh (`GL_LINE`)
* `3` : Solid with diffuse lighting (`GL_FILL`)

**Lighting Controls:**
* `W` / `S` : Move light along the Y axis (Up/Down)
* `A` / `D` : Move light along the X axis (Left/Right)
* `Q` / `E` : Move light along the Z axis (Forward/Backward)
