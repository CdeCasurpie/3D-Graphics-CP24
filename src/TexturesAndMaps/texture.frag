#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normalMap;
    float shininess;
}; 

uniform Material material;
uniform bool useNormalMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 norm = normalize(Normal);
    if (useNormalMap) {
        norm = getNormalFromMap();
    }
    
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Ambient
    vec3 ambient = lightColor * 0.1 * vec3(texture(material.diffuse, TexCoords));

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, TexCoords));

    // Specular
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = lightColor * spec * vec3(texture(material.specular, TexCoords));

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
