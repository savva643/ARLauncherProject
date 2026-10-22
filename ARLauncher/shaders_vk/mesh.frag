#version 450
#extension GL_GOOGLE_include_directive : require
#include "scene_common.glsl"

// Виртуальные объекты в реальной комнате:
//  - окклюзия: LiDAR даёт расстояние до реальной поверхности вдоль того же луча, по которому
//    passthrough берёт пиксель видео; если реальное ближе — объект за ним (мягкий край)
//  - диффузный свет — SH карты окружения (свет комнаты, собранный по видео)
//  - отражения — карта окружения (equirect, мир), блик — от самого яркого источника
//  - режим «тень»: затемнённое видео под проекцией объекта на пол

layout(location = 0) in vec3 vPosEye;
layout(location = 1) in vec3 vNormalEye;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texY;
layout(set = 0, binding = 1) uniform sampler2D texUV;
layout(set = 0, binding = 2) uniform sampler2D texDepth;
layout(set = 1, binding = 1) uniform sampler2D envMap;

const float PI = 3.14159265;

vec3 toLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 toGamma(vec3 c) { return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2)); }

// Луч глаза -> координаты в кадре камеры (и в карте LiDAR — она выровнена с кадром)
bool cameraUv(vec3 dirEye, out vec2 uv, out vec3 c) {
    c = mat3(f.cvFromEye) * dirEye;
    uv = vec2(0.0);
    if (c.z <= 1e-4) return false;
    uv = vec2(f.cam.x * c.x / c.z + f.cam.z, f.cam.y * c.y / c.z + f.cam.w);
    return all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)));
}

vec3 videoRgb(vec2 uv) {
    float y = texture(texY, uv).r;
    vec2 cbcr = texture(texUV, uv).rg - vec2(0.5);
    int vf = int(f.params.y + 0.5);
    if ((vf & 1) == 0) {
        y = (y - 16.0 / 255.0) * (255.0 / 219.0);
        cbcr *= 255.0 / 224.0;
    }
    vec3 rgb = ((vf & 2) != 0)
        ? vec3(y + 1.5748 * cbcr.y, y - 0.1873 * cbcr.x - 0.4681 * cbcr.y, y + 1.8556 * cbcr.x)
        : vec3(y + 1.402 * cbcr.y, y - 0.344136 * cbcr.x - 0.714136 * cbcr.y, y + 1.772 * cbcr.x);
    return clamp(rgb, 0.0, 1.0);
}

// Освещённость от SH (Ramamoorthi & Hanrahan), делённая на π: «сколько света падает» на нормаль n
vec3 shDiffuse(vec3 n) {
    const float c1 = 0.429043, c2 = 0.511664, c3 = 0.743125, c4 = 0.886227, c5 = 0.247708;
    vec3 e = c1 * f.sh[8].rgb * (n.x * n.x - n.y * n.y) + c3 * f.sh[6].rgb * n.z * n.z + c4 * f.sh[0].rgb
           - c5 * f.sh[6].rgb
           + 2.0 * c1 * (f.sh[4].rgb * n.x * n.y + f.sh[7].rgb * n.x * n.z + f.sh[5].rgb * n.y * n.z)
           + 2.0 * c2 * (f.sh[3].rgb * n.x + f.sh[1].rgb * n.y + f.sh[2].rgb * n.z);
    return max(e, vec3(0.0)) / PI;
}

vec3 envColor(vec3 dirWorld) {
    float u = atan(dirWorld.x, -dirWorld.z) / (2.0 * PI) + 0.5;
    float v = acos(clamp(dirWorld.y, -1.0, 1.0)) / PI;
    return toLinear(texture(envMap, vec2(u, v)).rgb);
}

void main() {
    int flags = int(f.params.x + 0.5);
    bool shadowMode = o.material.z > 0.5;

    // --- окклюзия реальными предметами
    float visible = 1.0;
    if ((flags & 3) == 3) {
        vec2 uv;
        vec3 c;
        if (cameraUv(vPosEye, uv, c)) {
            float d = texture(texDepth, uv).r;
            if (d > 0.05) {
                float realRange = d * length(c) / c.z;   // глубина по оси -> расстояние вдоль луча
                float virtRange = length(vPosEye);
                float soft = f.params.z;
                visible = smoothstep(-soft, soft, realRange - virtRange + o.material.w);
            }
        }
    }
    if (visible < 0.01) discard;

    if (shadowMode) {
        vec2 uv;
        vec3 c;
        vec3 under = vec3(0.0);
        if ((flags & 4) != 0 && cameraUv(vPosEye, uv, c)) under = videoRgb(uv);
        float s = f.lightColor.w * o.color.a * visible;
        outColor = vec4(under * (1.0 - s), 1.0);   // blend MIN с уже нарисованным видео
        return;
    }

    vec3 N = normalize(vNormalEye);
    vec3 V = normalize(-vPosEye);
    if (dot(N, V) < 0.0) N = -N;   // двусторонние меши
    mat3 worldFromEye = mat3(f.worldFromEye);
    vec3 Nw = worldFromEye * N;

    vec3 albedo = toLinear(o.color.rgb);
    float metallic = clamp(o.material.x, 0.0, 1.0);
    float rough = clamp(o.material.y, 0.04, 1.0);

    // Диффуз: свет комнаты (SH уже содержит и лампы, и стены)
    vec3 diffuse = albedo * shDiffuse(Nw);

    // Отражение окружения: чёткое у гладких, у шершавых — размытое (SH)
    float NdV = max(dot(N, V), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - NdV, 5.0);
    vec3 Rw = worldFromEye * reflect(-V, N);
    vec3 env = shDiffuse(Rw);
    if ((flags & 24) == 24) env = mix(envColor(Rw), env, rough * rough);
    else if ((flags & 8) == 0) env = vec3(0.0);

    // Блик от главного источника
    vec3 L = normalize(f.lightDir.xyz);
    vec3 H = normalize(L + V);
    float shininess = 2.0 / (rough * rough * rough * rough) - 2.0;
    float spec = pow(max(dot(N, H), 0.0), max(shininess, 1.0)) * (shininess + 8.0) / (8.0 * PI) * max(dot(N, L), 0.0);

    vec3 color = diffuse * (1.0 - metallic) + env * F + f.lightColor.rgb * spec * F;
    outColor = vec4(toGamma(color), o.color.a * visible * f.params.w);
}
