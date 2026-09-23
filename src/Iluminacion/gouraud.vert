#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 LightingColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform int illuminationModel; // 0 = Phong, 1 = Blinn-Phong
uniform float shininess;

void main() {
    // Gouraud Shading: El cálculo de iluminación completo se realiza por vértice
    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
    vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);

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

    // El color total del vértice se pasa al Fragment Shader para ser interpolado linealmente
    LightingColor = ambient + diffuse + specular;

    gl_Position = projection * view * vec4(fragPos, 1.0);
}
