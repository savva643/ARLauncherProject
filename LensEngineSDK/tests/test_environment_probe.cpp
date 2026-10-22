// Карта окружения и свет по видео: камера осматривает комнату с лампой, проверяем направление
// света, покрытие сферы и что объект освещается со стороны лампы (SH).

#include "EnvironmentProbe.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

const glm::vec3 kLamp = glm::normalize(glm::vec3(0.5f, 0.8f, -0.4f));

// Цвет (sRGB 0..1) окружения в направлении d (мир рендера, Y вверх)
glm::vec3 environment(const glm::vec3& d)
{
    if (glm::dot(d, kLamp) > std::cos(glm::radians(12.0f))) return glm::vec3(1.0f, 0.97f, 0.9f);
    if (d.y < -0.2f) return glm::vec3(0.45f, 0.32f, 0.22f);   // деревянный пол
    return glm::vec3(0.35f, 0.36f, 0.38f);                    // стены
}

float shIrradianceLum(const std::array<glm::vec3, 9>& L, const glm::vec3& n)
{
    const float c1 = 0.429043f, c2 = 0.511664f, c3 = 0.743125f, c4 = 0.886227f, c5 = 0.247708f;
    glm::vec3 e = c1 * L[8] * (n.x * n.x - n.y * n.y) + c3 * L[6] * n.z * n.z + c4 * L[0] - c5 * L[6] +
                  2.0f * c1 * (L[4] * n.x * n.y + L[7] * n.x * n.z + L[5] * n.y * n.z) +
                  2.0f * c2 * (L[3] * n.x + L[1] * n.y + L[2] * n.z);
    return 0.2126f * e.r + 0.7152f * e.g + 0.0722f * e.b;
}

} // namespace

int main()
{
    const int w = 320, h = 240;
    const float f = 260.0f, cx = w * 0.5f, cy = h * 0.5f;
    std::vector<uint8_t> y(size_t(w) * h), uv(size_t(w) * h / 2);
    LensEngine::EnvironmentProbe probe;

    float coverageAfterOne = 0.0f;
    int frames = 0;
    for (int pitchDeg : {25, -20, 60}) {
        for (int yawDeg = 0; yawDeg < 360; yawDeg += 30) {
            // Камера OpenCV (x вправо, y вниз, z вперёд) в мире Y-up: базовый взгляд по -Z
            const glm::mat3 base(glm::vec3(1, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
            const glm::mat3 yawM = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(float(yawDeg)), glm::vec3(0, 1, 0)));
            const glm::mat3 pitchM = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(float(pitchDeg)), glm::vec3(1, 0, 0)));
            const glm::mat3 rwc = yawM * pitchM * base;
            for (int v = 0; v < h; ++v)
                for (int u = 0; u < w; ++u) {
                    const glm::vec3 d = glm::normalize(rwc * glm::vec3((u - cx) / f, (v - cy) / f, 1.0f));
                    const glm::vec3 c = environment(d);
                    const float Y = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                    y[size_t(v) * w + u] = uint8_t(std::lround(std::clamp(Y, 0.0f, 1.0f) * 255));
                    if ((u & 1) == 0 && (v & 1) == 0) {
                        uint8_t* p = uv.data() + size_t(v / 2) * w + u;
                        p[0] = uint8_t(std::lround(std::clamp((c.b - Y) / 1.8556f + 0.5f, 0.0f, 1.0f) * 255));
                        p[1] = uint8_t(std::lround(std::clamp((c.r - Y) / 1.5748f + 0.5f, 0.0f, 1.0f) * 255));
                    }
                }
            probe.addFrame(y.data(), w, uv.data(), w, w, h, f, f, cx, cy, glm::quat_cast(rwc), true, true);
            if (++frames == 1) coverageAfterOne = probe.lighting().coverage;
        }
    }

    const auto L = probe.lighting();
    const float lightErr = glm::degrees(std::acos(std::clamp(glm::dot(L.lightDir, kLamp), -1.0f, 1.0f)));
    const float towardLamp = shIrradianceLum(L.sh, kLamp);
    const float awayLamp = shIrradianceLum(L.sh, -kLamp);
    std::vector<uint8_t> tex;
    const uint64_t ver = probe.copyTexture(tex);

    std::printf("coverage: %.0f%% after 1 frame, %.0f%% after %d frames\n", coverageAfterOne * 100, L.coverage * 100, frames);
    std::printf("light direction error %.1f deg, light color (%.2f %.2f %.2f), ambient (%.2f %.2f %.2f)\n", lightErr,
                L.lightColor.r, L.lightColor.g, L.lightColor.b, L.ambient.r, L.ambient.g, L.ambient.b);
    std::printf("SH irradiance toward lamp %.3f vs away %.3f\n", towardLamp, awayLamp);
    std::printf("env texture %zu bytes, version %llu\n", tex.size(), (unsigned long long)ver);

    const bool ok = L.valid && lightErr < 10.0f && L.coverage > 0.6f && coverageAfterOne < 0.15f &&
                    towardLamp > awayLamp * 1.1f && L.lightColor.r > 0.2f && ver == uint64_t(frames) &&
                    tex.size() == size_t(LensEngine::EnvironmentProbe::kWidth) * LensEngine::EnvironmentProbe::kHeight * 4 &&
                    L.ambient.r > L.ambient.b;   // тёплый пол чуть подкрашивает окружение
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
