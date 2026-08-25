#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Calculamos la posición del vértice en el mundo
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    
    // Calculamos la normal. Usamos la matriz inversa transpuesta para que la normal
    // escale y rote correctamente si el modelo sufre transformaciones no uniformes.
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Proyectamos el vértice a la pantalla (MVP)
    gl_Position = projection * view * worldPos;
}
