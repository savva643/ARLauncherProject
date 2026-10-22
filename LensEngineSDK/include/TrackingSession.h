#ifndef TRACKINGSESSION_H
#define TRACKINGSESSION_H

#include "HeadTracker.h"
#include "VioTracker.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace LensEngine {

/**
 * @brief Сессия трекинга LensEngine (аналог ARSession в ARKit).
 *
 * Принимает сырые IMU и кадры камеры (время — часы телефона), выдаёт позу головы для рендера:
 *  - ориентация с задержкой ~0 (интегрирование гироскопа) + предсказание на момент показа
 *  - позиция из VIO (OpenVINS), если собран и инициализирован; иначе — модель шеи (параллакс при поворотах)
 *  - ориентация камеры на момент экспозиции кадра (для репроекции passthrough-видео)
 *
 * Мир рендера как в ARKit: Y вверх, взгляд в начале — по -Z, метры.
 * Оси «глаза» как в OpenGL/Vulkan-камере: x вправо, y вверх, смотрит в -z.
 */
class TrackingSession {
public:
    struct Options {
        bool enableVio = true;
        bool neckModel = true;
        /// Поворот телефона в шлеме: 0 — верх телефона слева (если смотреть на экран), 180 — справа, -1 — авто по гравитации
        int mountRollDeg = -1;
    };

    struct EyePose {
        bool valid = false;
        int64_t phoneTimeNs = 0;
        glm::dquat orientation{1, 0, 0, 0};   // R_world_from_eye
        glm::dvec3 position{0.0};
        glm::dvec3 angularVelocity{0.0};      // оси устройства
        bool sixDof = false;
    };

    struct TrackingStatus {
        bool imuActive = false;
        bool vioAvailable = false;
        bool vioInitialized = false;
        int mountRollDeg = 0;
        glm::dvec3 gyroBias{0.0};
        std::string mode;
    };

    TrackingSession();
    ~TrackingSession();

    void start(const Options& options);
    void start() { start(Options{}); }
    void stop();

    /// IMU в осях устройства (CoreMotion), gyro rad/s, accel m/s^2 specific force
    void addImu(int64_t phoneTimeNs, const glm::dvec3& gyro, const glm::dvec3& accel, bool hasAccel);

    /// Кадр камеры (яркость) — для VIO. tNs — середина экспозиции.
    void addFrame(int64_t phoneTimeNs, const uint8_t* y, int width, int height, int stride,
                  float fx, float fy, float cx, float cy);

    /// Поза глаза, предсказанная на момент phoneTimeNs (обычно — время показа кадра на дисплее)
    EyePose predictEye(int64_t phoneTimeNs) const;

    /// R_world_from_cameraCV на момент phoneTimeNs (камера в осях OpenCV: x вправо, y вниз, z вперёд)
    bool cameraOrientationAt(int64_t phoneTimeNs, glm::dquat& out) const;

    /// Поворот из осей «глаза» (рендер) в оси камеры OpenCV — с учётом крепления телефона
    glm::dquat eyeToCameraRotation() const;

    void recenter();
    TrackingStatus status() const;

private:
    glm::dquat deviceFromEye() const;
    void updateMount(const glm::dvec3& accel);

    Options m_options;
    HeadTracker m_head;
    std::unique_ptr<VioTracker> m_vio;
    std::atomic<int> m_mountRoll{0};
    std::atomic<bool> m_mountResolved{false};
    std::atomic<int64_t> m_lastImuT{0};
    glm::dvec3 m_mountAccelSum{0.0};
    int m_mountCount = 0;
    // Слежение за креплением после старта (телефон перевернули / вставили в шлем)
    glm::dvec3 m_gravityLp{0.0};
    int m_mountFlipCount = 0;
    int m_portraitCount = 0;
    bool m_portraitWarned = false;
};

} // namespace LensEngine

#endif // TRACKINGSESSION_H
