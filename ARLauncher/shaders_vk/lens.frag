#version 450

// Коррекция линз шлема: предыскажение «бочкой» (линзы дают «подушку») + хроматическая аберрация
// (красный, зелёный и синий преломляются по-разному — каждый канал берётся со своим масштабом).
// Сцена рендерится в отдельную картинку, этот проход раскладывает её на экран по глазам.

layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D sceneImage;

layout(push_constant) uniform Lens {
    vec4 k;      // k1, k2, хроматическая аберрация, масштаб
    vec4 view;   // x: 1 — стерео (два глаза), y: ширина/высота одного глаза, z: смещение центра линзы (доля полуширины)
} pc;

// Центр линзы в координатах окна глаза (-1..1): в стерео смещён к носу на view.z
float lensCenter(float eye, float eyes) {
    return eyes > 1.5 ? (eye == 0.0 ? pc.view.z : -pc.view.z) : 0.0;
}

vec2 toUv(vec2 p, float eye, float eyes, float aspect) {
    vec2 local = vec2(p.x + lensCenter(eye, eyes), p.y * aspect);
    return vec2(((local.x * 0.5 + 0.5) + eye) / eyes, local.y * 0.5 + 0.5);
}

void main() {
    const float eyes = pc.view.x > 0.5 ? 2.0 : 1.0;
    const float aspect = pc.view.y;
    float ex = (vNdc.x * 0.5 + 0.5) * eyes;
    float eye = min(floor(ex), eyes - 1.0);
    // координаты относительно центра линзы этого глаза; радиус 1 — край глаза по горизонтали
    vec2 p = vec2((ex - eye) * 2.0 - 1.0 - lensCenter(eye, eyes), vNdc.y / aspect);
    float r2 = dot(p, p);
    float f = (1.0 + pc.k.x * r2 + pc.k.y * r2 * r2) * pc.k.w;
    vec2 uvR = toUv(p * f * (1.0 + pc.k.z), eye, eyes, aspect);
    vec2 uvG = toUv(p * f, eye, eyes, aspect);
    vec2 uvB = toUv(p * f * (1.0 - pc.k.z), eye, eyes, aspect);
    // за пределами картинки своего глаза — чёрное
    float eyeMin = eye / eyes, eyeMax = (eye + 1.0) / eyes;
    if (uvR.x < eyeMin || uvR.x > eyeMax || uvR.y < 0.0 || uvR.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    outColor = vec4(texture(sceneImage, uvR).r,
                    texture(sceneImage, clamp(uvG, vec2(eyeMin, 0.0), vec2(eyeMax, 1.0))).g,
                    texture(sceneImage, clamp(uvB, vec2(eyeMin, 0.0), vec2(eyeMax, 1.0))).b, 1.0);
}
