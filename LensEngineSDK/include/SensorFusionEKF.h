#ifndef SENSORFUSIONEKF_H
#define SENSORFUSIONEKF_H

#include "LensEngineTypes.h"
#include <vector>
#include <mutex>
#include <chrono>
#include <memory>
#include <functional>

namespace LensEngine {

/**
 * @brief Extended Kalman Filter для сенсорного фьюжна
 * 
 * Объединяет данные от IMU, визуальной одометрии и LiDAR
 * для получения точной 6DoF позы камеры.
 */
class SensorFusionEKF {
public:
    SensorFusionEKF();
    ~SensorFusionEKF();

    // Инициализация и управление
    void initialize();
    void reset();

    // Обновление данных
    void updateIMU(const RawIMUData &imu);
    void updateFeaturePoints(const std::vector<FeaturePoint> &features);
    void updateLidar(const std::vector<glm::vec3> &lidarPoints);
    void updateVisualOdometry(const glm::vec3 &visualPosition, const glm::quat &visualRotation);

    // Получение результата
    CameraPose getCurrentPose() const;
    float getStability() const;
    glm::vec3 getVelocity() const;

    // Настройки фильтра
    void setNoiseParameters(double gyroNoise = 0.01, double accelNoise = 0.1,
                            double gyroBiasNoise = 0.001, double accelBiasNoise = 0.01,
                            double visualNoise = 0.05, double lidarNoise = 0.02);

    // Колбэки
    using PoseCallback = std::function<void(const CameraPose&)>;
    using StabilityCallback = std::function<void(float)>;
    
    void setPoseCallback(PoseCallback callback);
    void setStabilityCallback(StabilityCallback callback);

private:
    // EKF состояние: [px, py, pz, vx, vy, vz, qw, qx, qy, qz, bgx, bgy, bgz, bax, bay, baz]
    static const int STATE_SIZE = 16;

    // EKF матрицы
    std::vector<double> m_state;
    std::vector<double> m_covariance;
    std::vector<double> m_processNoise;

    // Фильтры
    void predictStep(double dt);
    void updateIMUStep(const RawIMUData &imu, double dt);
    void updateVisualStep(const glm::vec3 &visualPosition, const glm::quat &visualRotation);
    void updateLidarStep(const std::vector<glm::vec3> &lidarPoints);
    void predictSimple(double dt, const RawIMUData &imu);

    // Математические функции
    glm::quat stateToQuaternion() const;
    void quaternionToState(const glm::quat &q);
    glm::vec3 stateToPosition() const;
    glm::vec3 stateToVelocity() const;
    glm::vec3 stateToGyroBias() const;
    glm::vec3 stateToAccelBias() const;

    float calculatePoseConfidence() const;

    // Матричные операции
    void matrixMultiply(const std::vector<double> &A, const std::vector<double> &B,
                        std::vector<double> &result, int rowsA, int colsA, int colsB);
    void matrixTranspose(const std::vector<double> &input, std::vector<double> &output, int rows, int cols);
    bool matrixInvert3x3(const std::vector<double> &input, std::vector<double> &output);

    // Шумы и параметры
    double m_gyroNoise;
    double m_accelNoise;
    double m_gyroBiasNoise;
    double m_accelBiasNoise;
    double m_visualNoise;
    double m_lidarNoise;
    double m_gravity;

    // Время и история
    std::chrono::steady_clock::time_point m_startTime;
    uint64_t m_lastUpdateTime;
    CameraPose m_currentPose;

    // Стабилизация
    std::vector<CameraPose> m_poseHistory;
    float calculateStability() const;

    // Потокобезопасность
    mutable std::mutex m_mutex;

    // Флаги
    bool m_initialized;
    
    // Колбэки
    PoseCallback m_poseCallback;
    StabilityCallback m_stabilityCallback;
};

} // namespace LensEngine

#endif // SENSORFUSIONEKF_H

