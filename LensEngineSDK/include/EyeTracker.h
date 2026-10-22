#ifndef EYETRACKER_H
#define EYETRACKER_H

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace LensEngine {

/**
 * @brief Отслеживание взгляда по ИК-камерам глаз (как в Vision Pro: смотришь на объект + щипок).
 *
 * На каждый глаз — ИК-камера (кадры GRAY8, пакет EYE протокола v2). Зрачок под ИК-подсветкой — самое
 * тёмное круглое пятно: ищется по интегральному изображению, уточняется заливкой по порогу (блики
 * от ИК-диодов внутри зрачка заполняются). Калибровка — 9 точек: полином 2-й степени
 * «центр зрачка -> углы взгляда» для каждого глаза. Итог — направление взгляда в осях головы
 * (x вправо, y вверх, взгляд -z) и точка схождения глаз.
 *
 * Без калибровки работает грубая модель (центр кадра = прямо, ±30° по краям кадра).
 */
class EyeTracker {
public:
    struct Pupil {
        bool found = false;
        glm::vec2 center{0.0f};      // нормированные координаты кадра 0..1
        float radius = 0.0f;         // доля ширины кадра
        float confidence = 0.0f;
    };

    struct Gaze {
        bool valid = false;
        int64_t tNs = 0;
        glm::vec3 direction{0.0f, 0.0f, -1.0f};   // оси головы
        glm::vec2 anglesDeg{0.0f};                // рысканье (вправо +), тангаж (вверх +)
        float confidence = 0.0f;
        bool calibrated = false;
    };

    EyeTracker();

    /// Найти зрачок на кадре (без состояния)
    static Pupil detectPupil(const uint8_t* gray, int width, int height, int stride);

    /// Кадр глаза: eye 0 — левый, 1 — правый
    Pupil addImage(int eye, int64_t tNs, const uint8_t* gray, int width, int height, int stride);

    /// Взгляд (оба глаза вместе; если виден один — по нему)
    Gaze gaze() const;

    // ---- Калибровка: смотреть на точку с известными углами, пока идут кадры
    void startCalibration();
    /// Добавить текущие положения зрачков как наблюдение точки (углы в градусах)
    bool addCalibrationSample(const glm::vec2& targetAnglesDeg);
    /// Посчитать модель; false — мало точек (нужно ≥ 6 на глаз)
    bool finishCalibration();
    bool calibrating() const { return m_calibrating; }
    bool calibrated() const;

    bool saveCalibration(const std::string& path) const;
    bool loadCalibration(const std::string& path);

private:
    static void basis(const glm::vec2& p, float out[6]);
    glm::vec2 mapToAngles(int eye, const glm::vec2& pupil) const;

    mutable std::mutex m_mutex;
    Pupil m_pupil[2];
    int64_t m_pupilT[2] = {0, 0};
    glm::vec2 m_smoothed[2]{};
    bool m_haveSmoothed[2] = {false, false};
    // модель: углы = coeff · [1, x, y, xy, x², y²] для каждого глаза
    std::array<float, 6> m_yaw[2]{};
    std::array<float, 6> m_pitch[2]{};
    bool m_model[2] = {false, false};
    bool m_calibrating = false;
    struct Sample {
        glm::vec2 pupil;
        glm::vec2 target;
    };
    std::vector<Sample> m_samples[2];
};

} // namespace LensEngine

#endif // EYETRACKER_H
