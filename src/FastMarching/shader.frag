#version 330 core
out vec4 FragColor;

in float Distance;

uniform float maxDistance;
uniform bool isLine;

vec3 getColormap(float t) {
    float v = clamp(t, 0.0, 1.0);
    float r = clamp(1.5 - abs(2.0 * v - 1.5), 0.0, 1.0);
    float g = clamp(1.5 - abs(2.0 * v - 1.0), 0.0, 1.0);
    float b = clamp(1.5 - abs(2.0 * v - 0.5), 0.0, 1.0);
    return vec3(r, g, b);
}

void main()
{
    if (isLine) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Lineas Rojas
    } else {
        // Mapear distancia a color
        float normalizedDist = Distance / maxDistance;
        
        // Repetir el color map (isolineas) para que parezcan contornos
        float iso = fract(normalizedDist * 10.0);
        vec3 baseColor = getColormap(normalizedDist);
        if (iso < 0.05) baseColor = vec3(0.0);

        FragColor = vec4(baseColor, 1.0); // Colores planos, sin luces
    }
}
