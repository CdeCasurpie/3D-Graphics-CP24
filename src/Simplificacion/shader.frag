#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightDir; // Luz direccional estática

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0)); // Asumiendo cámara fija al frente
    
    // Ambiental
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0); // luz blanca
    
    // Difusa
    vec3 normLightDir = normalize(-lightDir);
    float diff = max(dot(norm, normLightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
