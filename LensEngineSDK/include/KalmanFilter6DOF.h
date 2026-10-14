#ifndef KALMAN_FILTER_6DOF_H
#define KALMAN_FILTER_6DOF_H

#include "LensEngineTypes.h"
#include "IMUDiagnostics.h"
#include "TemporalFilter.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include <memory>

namespace LensEngine {

/**
 * @brief 6DOF Kalman Filter для AR tracking (как в ARKit)
 * 
 * Состояние: [x, y, z, vx, vy, vz, qx, qy, qz, qw]
 * 
 * Измерения:
 * - IMU: accelerometer + gyroscope
 * - VO: position + rotation (с низкой частотой)
 * 
 * Фильтрация:
 * - Prediction: IMU (100 Hz)
 * - Update: VO (30 Hz)
 */
class KalmanFilter6DOF {
public:
    KalmanFilter6DOF();
    ~KalmanFilter6DOF();

    // Инициализация
    void initialize();
    
    // Prediction step (IMU data, ~100 Hz)
    void predictIMU(const RawIMUData& imu, double dt);
    
    // Update step (VO data, ~30 Hz)
    void updateVO(const glm::vec3& voPosition, const glm::quat& voRotation, double confidence);
    
    // Update step (LiDAR data, ~30 Hz) - для коррекции масштаба и глубины
    void updateLidar(const std::vector<glm::vec3>& lidarPoints);
    
    // Получить текущее состояние
    CameraPose getCurrentPose() const;
    
    // Сброс
    void reset();

private:
    // Состояние: [x, y, z, vx, vy, vz, qx, qy, qz, qw]
    std::array<float, 10> m_state;
    
    // Ковариационная матрица (10x10)
    std::array<std::array<float, 10>, 10> m_covariance;
    
    // Шумы
    float m_processNoise = 0.01f;      // IMU noise
    float m_measurementNoise = 0.1f;   // VO noise
    
    // Параметры
    glm::vec3 m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    glm::vec3 m_accelBias = glm::vec3(0.0f);
    glm::vec3 m_gyroBias = glm::vec3(0.0f);
    
    // Вспомогательные методы
    void predictStep(const glm::vec3& accel, const glm::vec3& gyro, double dt);
    void updateStep(const glm::vec3& voPos, const glm::quat& voRot, float confidence);
    
    glm::quat quatFromGyro(const glm::vec3& gyro, double dt) const;
    glm::quat quatMultiply(const glm::quat& q1, const glm::quat& q2) const;
    glm::quat quatNormalize(const glm::quat& q) const;
    
    // Статистика
    int m_frameCount = 0;
    float m_lastVOConfidence = 0.0f;
    bool m_isStationary = false;  // Флаг неподвижности
    
    // Диагностика IMU
    std::unique_ptr<IMUDiagnostics> m_imuDiagnostics;
    
    // Temporal filtering для сглаживания
    TemporalFilter m_temporalFilter;
};

} // namespace LensEngine

#endif // KALMAN_FILTER_6DOF_H
