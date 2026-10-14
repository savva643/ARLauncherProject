#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uiTexture;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    float opacity;
} pc;

void main() {
    vec4 c = texture(uiTexture, fragUV);
    c.a *= pc.opacity;
    outColor = c;
}
