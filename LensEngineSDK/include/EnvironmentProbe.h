#ifndef ENVIRONMENTPROBE_H
#define ENVIRONMENTPROBE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

namespace LensEngine {

/**
 * @brief Карта окружения и оценка освещения по видео камеры (аналог AREnvironmentProbe + light estimation в ARKit).
 *
 * Каждый кадр камеры (NV12) с известной ориентацией «вклеивается» в equirect-карту мира: со временем, пока
 * пользователь осматривается, собирается вся комната. Из карты считаются:
 *  - 9 коэффициентов сферических гармоник (диффузное освещение объектов по реальной комнате);
 *  - направление и цвет самого яркого источника (направление теней, блики);
 *  - средний цвет окружения.
 * Сама карта (RGBA8, sRGB) — для отражений на виртуальных объектах.
 *
 * Мир — как у рендера: Y вверх, взгляд в начале по -Z. Направление -> (u, v):
 *   u = atan2(d.x, -d.z) / 2π + 0.5,  v = acos(d.y) / π  (v = 0 — зенит).
 */
class EnvironmentProbe {
public:
    static constexpr int kWidth = 128;
    static constexpr int kHeight = 64;

    struct Lighting {
        bool valid = false;
        std::array<glm::vec3, 9> sh{};        // SH радиатности (линейный RGB), порядок L00, L1-1, L10, L11, L2-2, L2-1, L20, L21, L22
        glm::vec3 lightDir{0.3f, 0.9f, 0.3f}; // к источнику, мир
        glm::vec3 lightColor{0.0f};           // линейный RGB, сверх фона
        glm::vec3 ambient{0.25f};             // средняя яркость окружения, линейный RGB
        float coverage = 0.0f;                // доля сферы, которую камера уже видела
    };

    EnvironmentProbe();

    void reset();

    /// Кадр NV12; worldFromCamCV — ориентация камеры (оси OpenCV) в мире рендера; интринсики в пикселях кадра
    void addFrame(const uint8_t* y, int yStride, const uint8_t* uv, int uvStride, int width, int height,
                  float fx, float fy, float cx, float cy, const glm::quat& worldFromCamCV,
                  bool fullRange, bool bt709);

    Lighting lighting() const;

    /// Копия карты (kWidth*kHeight RGBA8, sRGB; невиданные области заполнены по соседним широтам).
    /// Возвращает версию карты (растёт с каждым addFrame); 0 — карты ещё нет.
    uint64_t copyTexture(std::vector<uint8_t>& rgba) const;
    uint64_t version() const;

    static glm::vec3 texelDirection(float u, float v);

private:
    void rebuildLocked();

    mutable std::mutex m_mutex;
    std::vector<glm::vec3> m_radiance;    // линейный RGB
    std::vector<float> m_weight;          // 0 — ещё не видели
    std::vector<glm::vec3> m_dirs;        // направление центра каждого тексела
    std::vector<float> m_solidAngle;
    std::vector<uint8_t> m_texture;       // RGBA8, после заполнения дыр
    Lighting m_lighting;
    uint64_t m_version = 0;
};

} // namespace LensEngine

#endif // ENVIRONMENTPROBE_H
