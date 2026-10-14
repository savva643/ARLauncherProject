#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(vNormal);
    float l = clamp(dot(n, normalize(vec3(0.3, 0.7, 0.2))) * 0.5 + 0.5, 0.0, 1.0);
    outColor = vec4(vec3(0.2, 0.7, 1.0) * l, 1.0);
}
