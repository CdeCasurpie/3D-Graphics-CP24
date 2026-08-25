#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// Vamos a usar 4 luces estáticas
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 objectColor;
uniform vec3 viewPos; // Necesario si quisiéramos brillo (specular), pero haremos solo difusa + ambiental

void main() {
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);

    // Iteramos sobre las 4 luces
    for(int i = 0; i < 4; i++) {
        // Iluminación Ambiental
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColors[i];
        
        // Iluminación Difusa
        vec3 lightDir = normalize(lightPositions[i] - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColors[i];
        
        // Sumamos la contribución de esta luz al resultado final
        result += (ambient + diffuse);
    }
    
    // Multiplicamos por el color del objeto (cubo)
    result = result * objectColor;
    
    // Aseguramos que los valores no superen 1.0
    FragColor = vec4(min(result, vec3(1.0)), 1.0);
}
