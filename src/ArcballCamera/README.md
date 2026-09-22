# Arcball Camera con Cuaterniones (Task 09 - Computer Graphics UTEC)

Implementación de un sistema de cámara orbital interactiva basado en **Arcball** y **Cuaterniones** (`glm::quat`), diseñado de forma modular dentro de la biblioteca de utilidades (`include/Camera.h`) para ser reutilizable en todas las tareas del repositorio.

---

## 1. Fundamentos Matemáticos

### 1.1 Mapeo de Coordenadas 2D de Pantalla a la Esfera 3D
Dado un punto del ratón $(x, y)$ en la ventana de tamaño $(W, H)$, se normaliza al rango $[-1, 1]$ invirtiendo el eje $Y$:
$$P_x = \frac{2x}{W} - 1, \quad P_y = -\left(\frac{2y}{H} - 1\right), \quad P_z = 0$$

Para proyectar $P$ sobre la semiesfera unitaria ($r = 1$):
$$z = \begin{cases} \sqrt{1 - (P_x^2 + P_y^2)} & \text{si } P_x^2 + P_y^2 \le 1 \\ 0 & \text{en otro caso (proyección al borde)} \end{cases}$$

El vector resultante se normaliza: $\vec{v} = \frac{(P_x, P_y, z)}{\|(P_x, P_y, z)\|}$.

---

### 1.2 Cálculo de la Rotación con Cuaterniones
Al arrastrar el cursor entre dos posiciones sucesivas con vectores de esfera $\vec{v}_{\text{start}}$ y $\vec{v}_{\text{curr}}$:
1. **Eje de rotación:**
   $$\hat{u} = \frac{\vec{v}_{\text{start}} \times \vec{v}_{\text{curr}}}{\|\vec{v}_{\text{start}} \times \vec{v}_{\text{curr}}\|}$$
2. **Ángulo de rotación:**
   $$\theta = \arccos\left(\min(1.0, \vec{v}_{\text{start}} \cdot \vec{v}_{\text{curr}})\right)$$
3. **Cuaternión Delta ($q_{\Delta}$):**
   $$q_{\Delta} = \left(\cos\frac{\theta}{2}, \, \hat{u}\sin\frac{\theta}{2}\right)$$
4. **Composición de rotación (acumulación):**
   $$q_{\text{nueva}} = \text{normalize}(q_{\Delta} \times q_{\text{anterior}})$$

El uso de cuaterniones evita el **bloqueo del cardán (Gimbal Lock)** y asegura interpolaciones continuas y suaves desde cualquier ángulo de visión.

---

### 1.3 Generación de la Matriz de Vista (View Matrix)
A partir de la orientación $q$, la distancia orbital $d$ y el objetivo $\vec{t}$:
$$V = T(0, 0, -d) \cdot R(q) \cdot T(-\vec{t})$$
Donde $R(q)$ es la matriz de rotación $4 \times 4$ generada por `glm::mat4_cast(orientation)`.

---

## 2. Demostración Visual

![Arcball Camera Demo](arcball_demo.gif)

La animación muestra:
1. **Modo Automático:** Órbita continua alrededor de la escena (escena animada de la Tarea 05/07).
2. **Modo Manual Arcball:** Control interactivo de rotación en 3D mediante arrastre con el ratón.
3. **Zoom Interactivo:** Ajuste dinámico de distancia focal mediante la rueda de desplazamiento (*mouse scroll*).

---

## 3. Controles de Usuario

| Entrada | Acción |
| :--- | :--- |
| **Clic Izquierdo + Arrastrar** | Rotación Arcball 3D alrededor del objetivo |
| **Rueda del Ratón (Scroll)** | Zoom In / Zoom Out (ajuste de distancia focal) |
| **Clic Derecho + Arrastrar** | Paneo (desplazamiento lateral y vertical del objetivo) |
| **Shift + Clic Izquierdo** | Paneo alternativo |
| **Barra Espaciadora o Tecla 'C'** | Alternar entre Cámara Animada (automática) y Arcball Manual |
| **Tecla 'R'** | Reiniciar cámara a posición y orientación por defecto |
| **Tecla 'ESC'** | Cerrar ventana |

---

## 4. Ejecución del Proyecto

Desde la raíz del repositorio:
```bash
make run-ArcballCamera
```
O directamente con CMake:
```bash
cmake -B build
cmake --build build --target ArcballCamera
./build/ArcballCamera
```
