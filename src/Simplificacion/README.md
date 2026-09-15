# Mesh Simplification via Quadric Error Metrics on a Compact Half-Edge Structure

This document outlines the theoretical and technical implementation of the Level of Detail (LOD) algorithm utilized in Task 04.

---

## 1. Algorithmic Foundation and Error Metric Calculation

The algorithm implemented is based on the **Quadric Error Metrics (QEM)** proposed by Garland and Heckbert. The core objective is to systematically reduce the polygon count of a 3D mesh while minimizing the overall geometric distortion.

### Quadric Error Metric (QEM)
The algorithm associates a 4x4 symmetric matrix $Q$ with each vertex in the mesh. This matrix mathematically represents the sum of squared distances from the vertex to all its incident triangle planes.

For a given triangle with a plane equation $n \cdot x + d = 0$ (where $n$ is the unit normal), the fundamental error quadric $K_p$ is defined by the outer product:

$$
K_p = p p^T = \begin{bmatrix} n_x^2 & n_xn_y & n_xn_z & n_xd \\ n_yn_x & n_y^2 & n_yn_z & n_yd \\ n_zn_x & n_zn_y & n_z^2 & n_zd \\ dn_x & dn_y & dn_z & d^2 \end{bmatrix}
$$

Where $p = [n_x, n_y, n_z, d]^T$.

The initial quadric for a vertex $v$ is the sum of the fundamental quadrics of its adjacent triangles:

$$
Q_v = \sum_{p \in planes(v)} K_p
$$

When evaluating the collapse of an edge $(v_1, v_2)$ into a new unified vertex $\bar{v}$, the new quadric is simply the sum of both:

$$
Q_{\bar{v}} = Q_{v_1} + Q_{v_2}
$$

The geometric error cost $\Delta$ of this collapse is evaluated as:

$$
\Delta(\bar{v}) = \bar{v}^T Q_{\bar{v}} \bar{v}
$$

In this specific implementation, to avoid the computational overhead of inverting matrices to find the optimal position, the new vertex position $\bar{v}$ is fixed at the midpoint $\bar{v} = \frac{v_1 + v_2}{2}$.

---

## 2. Topological Modifications in the Compact Half-Edge (CHE) Structure

The mesh decimation is executed via the **Edge Collapse** operation. Since this framework utilizes a Level 1 (L1) CHE structure, the topology relies exclusively on two 1D arrays: the Vertex array `V` and the Opposites array `O`.

### Edge Collapse Operation
Collapsing a half-edge `he` connecting $v_1$ to $v_2$ entails merging both vertices and deleting the two triangles sharing this edge (`he` and `opposite(he)`).

**Topological Updates:**
1. **Geometric Update:** Vertex $v_1$ is translated to the midpoint $\bar{v}$.
2. **Vertex Redirection:** An $O(N)$ traversal over the `V` array replaces all instances of $v_2$ with $v_1$.
3. **Boundary Suturing:** The outer boundary half-edges of the two deleted triangles must be linked to prevent topological holes. Since the half-edges `next(he)` and `prev(he)` lose their internal triangle, their respective opposites must be sutured together:
   ```cpp
   O[opposite(next(he))] = opposite(prev(he));
   O[opposite(prev(he))] = opposite(next(he));
   ```
   The exact same procedure is applied symmetrically to the opposite triangle.
4. **Deletion Marking:** The 6 half-edges constituting the deleted triangles are flagged with `-1` (or `UINT_MAX`) to effectively remove them from the active topology without requiring immediate array reallocation.

### Priority Queue and Lazy Updates
To maintain an optimal sequence of collapses, valid half-edges are inserted into a Priority Queue ordered by their cost $\Delta(\bar{v})$. 

However, since edge collapses modify the geometry and topology of their neighborhoods, the costs of adjacent edges change over time. Instead of developing an updatable priority queue, a **Lazy Evaluation** approach is utilized:
Before collapsing an edge extracted from the queue, its current QEM cost is recalculated. If the current cost significantly diverges from the cached cost, the collapse is aborted, and the edge is re-inserted into the queue with the updated cost.

---

## 3. Rendering Pipeline Integration

A crucial aspect of this implementation is understanding the data flow to the Graphics Processing Unit (GPU). **Does the GPU simplify the mesh?** No. The simplification process operates entirely in user space (CPU) as a preprocessing step prior to GPU dispatch.

1. **CPU-side Simplification:** The original sphere geometry is mathematically generated in RAM. The `simplifyMesh` algorithm iterates over the CHE arrays in the CPU memory, running the Edge Collapse operations until the target triangle count (e.g., 500) is reached.
2. **Memory Compaction:** The `V` array is compacted to physically remove the `-1` flags. The Level 1 `O` array is completely rebuilt to guarantee a contiguous memory layout.
3. **GPU Dispatch:** The compacted arrays are bound to a Vertex Array Object (VAO) and uploaded to the GPU VRAM via VBO (Vertex Buffer Object) and EBO (Element Buffer Object).
4. **Rendering:** The OpenGL rendering loop executes `glDrawElements`. The graphics pipeline is completely unaware of the simplification process; it merely receives a static mesh of 500 triangles and renders it as standard geometry.
