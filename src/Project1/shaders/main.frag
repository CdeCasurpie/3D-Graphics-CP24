#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in float Distance;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D shadowMap;

uniform bool useDiffuseMap;
uniform bool useSpecularMap;
uniform vec3 baseDiffuse;
uniform vec3 baseSpecular;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform float ambientStrength;
uniform float materialShininess;

// Visualization Modes
uniform int renderMode; // 0 = Normal/Textured, 1 = Distance Heatmap, 2 = Normals, 3 = False-color Depth
uniform float maxDistance;

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

vec3 getHeatmapColor(float value) {
    // value is from 0.0 to 1.0 (0=red, 1=blue)
    value = clamp(value, 0.0, 1.0);
    vec3 color1 = vec3(1.0, 0.0, 0.0); // Red
    vec3 color2 = vec3(0.0, 1.0, 0.0); // Green
    vec3 color3 = vec3(0.0, 0.0, 1.0); // Blue
    if(value < 0.5) return mix(color1, color2, value * 2.0);
    return mix(color2, color3, (value - 0.5) * 2.0);
}

void main() {
    if(renderMode == 2) {
        // Normals Mode
        FragColor = vec4(normalize(Normal) * 0.5 + 0.5, 1.0);
        return;
    }
    if(renderMode == 3) {
        // Depth Mode
        float depth = gl_FragCoord.z;
        FragColor = vec4(vec3(depth), 1.0);
        return;
    }
    if(renderMode == 1) {
        // Distance Heatmap Mode
        if (Distance > 990000.0) {
            FragColor = vec4(0.2, 0.2, 0.2, 1.0); // Not reached
        } else {
            float val = maxDistance > 0.0 ? (Distance / maxDistance) : 0.0;
            FragColor = vec4(getHeatmapColor(val), 1.0);
        }
        return;
    }

    // Default mode: Blinn-Phong + Shadows
    vec3 color = useDiffuseMap ? texture(diffuseMap, TexCoords).rgb : baseDiffuse;
    vec3 specularColor = useSpecularMap ? texture(specularMap, TexCoords).rgb : baseSpecular;
    
    vec3 ambient = ambientStrength * color;
    
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor * color;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), materialShininess);
    vec3 specular = spec * lightColor * specularColor;    
    
    float shadow = ShadowCalculation(FragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));    
    
    FragColor = vec4(lighting, 1.0);
}
