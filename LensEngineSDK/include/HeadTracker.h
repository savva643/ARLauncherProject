#ifndef HEADTRACKER_H
#define HEADTRACKER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <deque>
#include <mutex>

namespace LensEngine {

/**
 * @brief Трекер ориентации головы с минимальной задержкой (как «3DoF-ядро» ARKit/Quest).
 *
 * Работает на сырых данных IMU iPhone (гироскоп с bias + акселерометр):
 *  - интегрирование гироскопа на каждом сэмпле (100 Гц), точное по аппаратным таймстемпам
 *  - коррекция наклона по гравитации (Mahony), только когда нет сильных линейных ускорений
 *  - оценка bias гироскопа (в т.ч. по рысканью) в моменты неподвижности
 *  - история ориентаций: позу можно взять на момент экспозиции кадра (для репроекции видео)
 *  - предсказание на будущий момент (момент показа кадра на дисплее) по угловой скорости
 *  - коррекция рысканья/позиции от VIO (OpenVINS), если он есть
 *
 * Мир: Z вверх (гравитация -Z), ось устройства — как в CoreMotion (x вправо, y вверх по экрану, z из экрана).
 * Время — часы телефона в наносекундах.
 */
class HeadTracker {
public:
    struct Config {
        double accelGain = 0.6;             // Kp коррекции наклона (1/с)
        double accelBiasGain = 0.004;       // Ki (оценка bias по наклону)
        double accelGateMs2 = 0.8;          // |‖a‖ - g| больше этого — коррекция выключена
        double stillGyroThreshold = 0.04;   // rad/s — «неподвижно»
        double stillTimeSec = 0.6;          // сколько держать неподвижность до обновления bias
        double stillBiasAlpha = 0.02;       // скорость обучения bias в покое
        double maxPredictSec = 0.08;        // ограничение экстраполяции
        size_t historySize = 400;           // ~4 с при 100 Гц
    };

    struct State {
        bool initialized = false;
        int64_t tNs = 0;
        glm::dquat orientation{1, 0, 0, 0};   // R_world_from_device
        glm::dvec3 angularVelocity{0.0};      // rad/s, оси устройства, без bias
        glm::dvec3 position{0.0};             // м, мир (если есть VIO)
        glm::dvec3 velocity{0.0};
        bool hasPosition = false;
    };

    HeadTracker();
    explicit HeadTracker(const Config& cfg);

    void reset();

    /// Сэмпл IMU в осях устройства: gyro rad/s, accel m/s^2 (specific force; в покое ‖a‖ = g вверх)
    void addImu(int64_t tNs, const glm::dvec3& gyro, const glm::dvec3& accel, bool hasAccel);

    /// Ориентация на момент t (история + slerp, либо экстраполяция вперёд)
    bool orientationAt(int64_t tNs, glm::dquat& out) const;

    /// Поза, предсказанная на момент t (обычно — момент показа кадра)
    State predict(int64_t tNs) const;

    /// Последнее состояние без предсказания
    State latest() const;

    /// Обнулить рысканье: текущее направление взгляда камеры становится «вперёд» (+Y мира)
    void recenter();

    /// Поза от VIO (тот же мир Z-up, но со своим рысканьем). Мягко подтягивает рысканье и задаёт позицию.
    void addVioPose(int64_t tNs, const glm::dquat& orientation, const glm::dvec3& position, const glm::dvec3& velocity);

    glm::dvec3 gyroBias() const;
    bool isStill() const;

private:
    struct Entry {
        int64_t tNs;
        glm::dquat q;
        glm::dvec3 omega;
    };

    static glm::dquat expMap(const glm::dvec3& w, double dt);
    static double yawOf(const glm::dquat& q);
    void initFromAccel(const glm::dvec3& accel);
    void applyRecenterLocked();

    Config m_cfg;
    mutable std::mutex m_mutex;

    bool m_initialized = false;
    int64_t m_lastT = 0;
    glm::dquat m_q{1, 0, 0, 0};
    glm::dvec3 m_bias{0.0};
    glm::dvec3 m_biasInt{0.0};
    glm::dvec3 m_omega{0.0};
    glm::dvec3 m_omegaPrev{0.0};
    double m_stillSec = 0.0;
    bool m_recenterPending = true;
    std::deque<Entry> m_history;

    // init: усредняем акселерометр
    glm::dvec3 m_initAccelSum{0.0};
    int m_initCount = 0;

    // VIO
    bool m_vioValid = false;
    int64_t m_vioT = 0;
    glm::dvec3 m_vioPos{0.0};
    glm::dvec3 m_vioVel{0.0};
    double m_vioYawOffset = 0.0;          // yaw(tracker) - yaw(vio)
    bool m_vioYawOffsetInit = false;
};

} // namespace LensEngine

#endif // HEADTRACKER_H
