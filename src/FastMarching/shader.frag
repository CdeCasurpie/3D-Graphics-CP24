#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in float Distance;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float maxDistance;

// Color map simple: Jet (Azul -> Verde -> Rojo)
vec3 getColormap(float t) {
    float v = clamp(t, 0.0, 1.0);
    float r = clamp(1.5 - abs(2.0 * v - 1.5), 0.0, 1.0);
    float g = clamp(1.5 - abs(2.0 * v - 1.0), 0.0, 1.0);
    float b = clamp(1.5 - abs(2.0 * v - 0.5), 0.0, 1.0);
    return vec3(r, g, b);
}

void main()
{
    // Iluminacion basica
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    float ambient = 0.3;
    float lighting = ambient + diff * 0.7;
    
    // Mapear distancia a color
    float normalizedDist = Distance / maxDistance;
    // Repetir el color map (isolineas) para que parezcan contornos
    float iso = fract(normalizedDist * 10.0);
    
    vec3 baseColor = getColormap(normalizedDist);
    
    // Dibujar lineas de contorno negras cada cierta distancia
    if (iso < 0.05) {
        baseColor = vec3(0.0);
    }

    FragColor = vec4(baseColor * lighting, 1.0);
}
