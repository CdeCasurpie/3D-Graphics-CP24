#version 330 core
out vec4 FragColor;

in vec3 LightingColor;

uniform vec3 objectColor;

void main() {
    // Gouraud Shading: El fragment shader solo interpola el color calculado en los vértices
    FragColor = vec4(LightingColor * objectColor, 1.0);
}
