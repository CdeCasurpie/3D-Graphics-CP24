# 3D-Graphics-CP24

Repositorio para los laboratorios y asignaciones del curso de Computación Gráfica 3D. 
Este proyecto contiene un mini-framework propio en OpenGL crudo (usando GLFW y GLAD) y la implementación de la estructura de datos topológica **Compact Half-Edge (CHE)**.

## 📂 Estructura del Proyecto

- `include/`: Contiene las clases del framework (`Window`, `Shader`, `VAO`, `VBO`, `EBO`) y la estructura de datos `EricStructure.h` (CHE).
- `src/`: Contiene los distintos ejercicios desarrollados a lo largo del curso.
  - `HelloTriangle/`: Setup inicial de OpenGL.
  - `Sierpinski/`: Fractales en 2D generados con OpenGL.
  - `SphereCHE/`: (Tarea Principal) Implementación de Compact Half-Edge rendering interactivo.

## 🚀 Compilación y Ejecución

El proyecto utiliza **CMake** por detrás, pero está envuelto en un **Makefile** para facilitar su uso.

### 1. Compilar todo el proyecto
Para configurar CMake y compilar todos los ejercicios a la vez, simplemente ejecuta en la raíz del repositorio:

```bash
make all
```

### 2. Ejecutar un ejercicio específico
Una vez compilado, puedes usar los siguientes comandos `make run-<nombre>` para lanzar el ejecutable deseado:

- **Tarea CHE (Esfera y Cubo con iluminación interactiva):**
  ```bash
  make run-SphereCHE
  ```
- **Hello Triangle (Básico):**
  ```bash
  make run-HelloTriangle
  ```
- **Sierpinski (Chaos Game Points):**
  ```bash
  make run-ChaosAlgorithm
  ```
- **Sierpinski (Triángulos Recursivos):**
  ```bash
  make run-RecursiveAlgorithm
  ```

---

## 🎮 Controles de la Aplicación CHE (`make run-SphereCHE`)

Al lanzar el programa principal de CHE, la ventana de OpenGL responderá a los siguientes atajos de teclado:

- **`C`**: Carga y renderiza el modelo de un **Cubo** (Test Inicial de la estructura CHE).
- **`V`**: Carga y renderiza una **Esfera Paramétrica**.
- **`1`**: Modo de renderizado: Nube de Puntos (`GL_POINTS`).
- **`2`**: Modo de renderizado: Wireframe (`GL_LINE`).
- **`3`**: Modo de renderizado: Sólido con iluminación plana (`GL_FILL`).
- **`W/A/S/D/Q/E`**: Mueve dinámicamente la posición de la luz en el espacio 3D.
