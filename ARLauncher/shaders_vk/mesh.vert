#version 450
#extension GL_GOOGLE_include_directive : require
#include "scene_common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vPosEye;
layout(location = 1) out vec3 vNormalEye;

void main() {
    vec4 pe = o.modelView * vec4(inPos, 1.0);
    vPosEye = pe.xyz;
    vNormalEye = transpose(inverse(mat3(o.modelView))) * inNormal;
    gl_Position = f.proj * pe;
}
