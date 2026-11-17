#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D depthImage;
void main() {
    float intensity = texture(depthImage, uv).r;
    color = vec4(vec3(intensity), 1.0);
}
