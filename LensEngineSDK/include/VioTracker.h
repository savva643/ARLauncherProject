#ifndef VIOTRACKER_H
#define VIOTRACKER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace LensEngine {

/**
 * @brief 6DoF визуально-инерциальная одометрия (моно-камера + IMU) на базе OpenVINS (MSCKF).
 *
 * Собирается, только если найден OpenVINS (third_party/build_openvins.sh), иначе available() == false
 * и трекинг остаётся 3DoF (ориентация HeadTracker + модель шеи).
 *
 * Работает в своём потоке: IMU кладётся в очередь, из видео берётся только самый свежий кадр
 * (если VIO не успевает — кадры пропускаются, задержка не копится).
 * Интринсики и оси камеры/IMU iPhone задаются автоматически; extrinsics, интринсики и
 * сдвиг времени камера↔IMU уточняются онлайн самим OpenVINS.
 */
class VioTracker {
public:
    struct Pose {
        int64_t tNs = 0;                   // часы телефона
        glm::dquat orientation{1, 0, 0, 0};// R_world_from_device (мир Z вверх)
        glm::dvec3 position{0.0};
        glm::dvec3 velocity{0.0};
    };

    VioTracker();
    ~VioTracker();

    /// Скомпилирован ли OpenVINS
    static bool available();

    bool start();
    void stop();

    /// IMU в осях устройства (сырые: гироскоп rad/s, specific force m/s^2)
    void addImu(int64_t tNs, const glm::dvec3& gyro, const glm::dvec3& accel);

    /// Кадр (яркость). tNs — середина экспозиции, часы телефона. Интринсики в пикселях этого кадра.
    void addFrame(int64_t tNs, const uint8_t* y, int width, int height, int stride,
                  float fx, float fy, float cx, float cy);

    void setPoseCallback(std::function<void(const Pose&)> cb) { m_onPose = std::move(cb); }

    bool initialized() const { return m_initialized.load(); }

private:
    struct ImuItem { int64_t tNs; glm::dvec3 gyro; glm::dvec3 accel; };
    struct FrameItem {
        int64_t tNs = 0;
        int width = 0, height = 0;
        float fx = 0, fy = 0, cx = 0, cy = 0;
        std::vector<uint8_t> gray;
    };

    void worker();

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    std::function<void(const Pose&)> m_onPose;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};
    std::thread m_thread;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<ImuItem> m_imu;
    std::unique_ptr<FrameItem> m_frame;     // только последний
    int64_t m_lastImuT = 0;
};

} // namespace LensEngine

#endif // VIOTRACKER_H
