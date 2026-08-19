#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
    // 1. Iluminación Ambiental (para que el lado oscuro no sea 100% negro)
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;
    
    // 2. Iluminación Difusa (Direccional)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Producto punto entre la normal y la dirección de la luz (qué tan directo le pega la luz)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Color final = (Luz ambiental + Luz directa) * Color del objeto
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
