#version 450

// Passthrough-видео с репроекцией (timewarp по вращению).
// Для каждого пикселя дисплея берём луч в осях глаза на момент показа, поворачиваем
// в оси камеры на момент экспозиции кадра и проецируем интринсиками камеры.
// Так видео стоит в мире неподвижно, даже если кадр пришёл с задержкой 40–80 мс,
// а дисплей обновляется чаще камеры (120 Гц при видео 60 Гц).

layout(location = 0) in vec2 vNdc;          // Vulkan NDC: y вниз
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texY;
layout(set = 0, binding = 1) uniform sampler2D texUV;
layout(set = 0, binding = 2) uniform sampler2D texDepth;   // LiDAR, метры (карта выровнена с кадром камеры)

layout(push_constant) uniform Push {
    mat4 cvFromEye;     // 3x3: оси глаза (GL) -> оси камеры (OpenCV)
    vec4 cam;           // fx/w, fy/h, cx/w, cy/h
    vec4 eye;           // tan(halfFovX), tan(halfFovY), opacity, flags (1 = full range, 2 = BT.709)
    vec4 depth;         // x = непрозрачность наложения LiDAR (0 = выкл), y = дальность шкалы, м
} pc;

// Turbo colormap (Google, полиномиальная аппроксимация): близко — красный, далеко — синий
vec3 turbo(float t) {
    const vec4 kR4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 kG4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const vec4 kB4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const vec2 kR2 = vec2(-152.94239396, 59.28637943);
    const vec2 kG2 = vec2(4.27729857, 2.82956604);
    const vec2 kB2 = vec2(-89.90310912, 27.34824973);
    t = clamp(t, 0.0, 1.0);
    vec4 v4 = vec4(1.0, t, t * t, t * t * t);
    vec2 v2 = v4.zw * v4.z;
    return vec3(dot(v4, kR4) + dot(v2, kR2),
                dot(v4, kG4) + dot(v2, kG2),
                dot(v4, kB4) + dot(v2, kB2));
}

void main() {
    vec3 dirEye = vec3(vNdc.x * pc.eye.x, -vNdc.y * pc.eye.y, -1.0);
    vec3 c = mat3(pc.cvFromEye) * dirEye;
    if (c.z <= 1e-4) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec2 uv = vec2(pc.cam.x * c.x / c.z + pc.cam.z,
                   pc.cam.y * c.y / c.z + pc.cam.w);

    // Мягкий край там, где поле зрения камеры заканчивается (вместо резкой рамки)
    vec2 edge = min(uv, 1.0 - uv);
    float fade = clamp(min(edge.x, edge.y) * 60.0, 0.0, 1.0);
    if (fade <= 0.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float y = texture(texY, uv).r;
    vec2 cbcr = texture(texUV, uv).rg - vec2(0.5);

    int flags = int(pc.eye.w + 0.5);
    if ((flags & 1) == 0) {
        y = (y - 16.0 / 255.0) * (255.0 / 219.0);
        cbcr *= 255.0 / 224.0;
    }

    vec3 rgb;
    if ((flags & 2) != 0) {
        rgb = vec3(y + 1.5748 * cbcr.y,
                   y - 0.1873 * cbcr.x - 0.4681 * cbcr.y,
                   y + 1.8556 * cbcr.x);
    } else {
        rgb = vec3(y + 1.402 * cbcr.y,
                   y - 0.344136 * cbcr.x - 0.714136 * cbcr.y,
                   y + 1.772 * cbcr.x);
    }
    rgb = clamp(rgb, 0.0, 1.0);

    if (pc.depth.x > 0.0) {
        float d = texture(texDepth, uv).r;
        if (d > 0.05) {
            rgb = mix(rgb, turbo(1.0 - d / pc.depth.y), pc.depth.x);
        }
    }
    outColor = vec4(rgb * fade * pc.eye.z, 1.0);
}
