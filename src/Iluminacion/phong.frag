#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform int illuminationModel; // 0 = Phong, 1 = Blinn-Phong
uniform float shininess;

void main() {
    // Phong Shading: El cálculo de iluminación completo se realiza por cada fragmento/píxel
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Componente Ambiental
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // 2. Componente Difusa (Ley de Lambert)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 3. Componente Especular
    float spec = 0.0;
    if (diff > 0.0) {
        if (illuminationModel == 0) {
            // Modelo de Phong clásico: (R · V)^alpha
            vec3 reflectDir = reflect(-lightDir, norm);
            spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        } else {
            // Modelo de Blinn-Phong: (N · H)^beta con vector medio H
            vec3 halfwayDir = normalize(lightDir + viewDir);
            spec = pow(max(dot(norm, halfwayDir), 0.0), shininess * 2.0);
        }
    }
    float specularStrength = 0.8;
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
