#include "SensorFusionEKF.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <limits>
#include <iostream>
#include <iomanip>

namespace LensEngine {

SensorFusionEKF::SensorFusionEKF()
    : m_state(STATE_SIZE, 0.0)
    , m_covariance(STATE_SIZE * STATE_SIZE, 0.0)
    , m_processNoise(STATE_SIZE * STATE_SIZE, 0.0)
    , m_gyroNoise(0.01)
    , m_accelNoise(0.1)
    , m_gyroBiasNoise(0.001)
    , m_accelBiasNoise(0.01)
    , m_visualNoise(0.05)
    , m_lidarNoise(0.02)
    , m_gravity(9.81)
    , m_initialized(false)
    , m_lastUpdateTime(0)
    , m_wasStationary(false)
{
    initialize();
}

SensorFusionEKF::~SensorFusionEKF()
{
}

void SensorFusionEKF::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Начальное состояние
    std::fill(m_state.begin(), m_state.end(), 0.0);
    m_state[6] = 1.0; // qw = 1 (нейтральная ориентация)

    // Начальная ковариация
    std::fill(m_covariance.begin(), m_covariance.end(), 0.0);
    for (int i = 0; i < STATE_SIZE; ++i) {
        m_covariance[i * STATE_SIZE + i] = 1.0;
    }

    // Большая неопределенность для позиции и ориентации
    for (int i = 0; i < 3; ++i) {
        m_covariance[i * STATE_SIZE + i] = 10.0;           // Позиция
        m_covariance[(i+3) * STATE_SIZE + (i+3)] = 1.0;    // Скорость
        m_covariance[(i+7) * STATE_SIZE + (i+7)] = 0.1;    // Ориентация (qx,qy,qz)
        m_covariance[(i+10) * STATE_SIZE + (i+10)] = 0.1;  // Смещения гироскопа
        m_covariance[(i+13) * STATE_SIZE + (i+13)] = 0.1;  // Смещения акселерометра
    }
    m_covariance[6 * STATE_SIZE + 6] = 0.1; // qw

    // Матрица шума процесса
    std::fill(m_processNoise.begin(), m_processNoise.end(), 0.0);
    for (int i = 0; i < STATE_SIZE; ++i) {
        if (i < 3) m_processNoise[i * STATE_SIZE + i] = 0.1;   // Позиция
        else if (i < 6) m_processNoise[i * STATE_SIZE + i] = 0.5; // Скорость
        else if (i < 10) m_processNoise[i * STATE_SIZE + i] = 0.01; // Ориентация
        else if (i < 13) m_processNoise[i * STATE_SIZE + i] = 0.001; // Смещения гиро
        else m_processNoise[i * STATE_SIZE + i] = 0.01; // Смещения акселя
    }

    m_startTime = std::chrono::steady_clock::now();
    m_lastUpdateTime = 0;
    m_wasStationary = false;
    
    // ВАЖНО: Инициализируем позицию в (0,0,0) для AR
    // Позиция будет обновляться только через визуальную одометрию
    m_state[0] = 0.0; // x
    m_state[1] = 0.0; // y
    m_state[2] = 0.0; // z
    m_state[3] = 0.0; // vx
    m_state[4] = 0.0; // vy
    m_state[5] = 0.0; // vz
    
    // Инициализируем текущую позу
    m_currentPose.position = glm::vec3(0.0f);
    m_currentPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_currentPose.timestamp = 0;
    m_currentPose.confidence = 1.0f;
    
    m_initialized = true;
}

void SensorFusionEKF::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    initialize();
    m_poseHistory.clear();
    m_wasStationary = false;
}

void SensorFusionEKF::updateIMU(const RawIMUData &imu)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
    uint64_t currentTime = elapsed.count();
    
    double dt = (currentTime - m_lastUpdateTime) / 1000.0;
    m_lastUpdateTime = currentTime;

    if (dt <= 0 || dt > 0.1) dt = 0.016;

    // Always use raw filter — no dependency on iPhone-specific data
    predictSimple(dt, imu);

    m_currentPose.position = stateToPosition();
    
    glm::quat rawRotation = stateToQuaternion();
    glm::quat pitchInversion = glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0));
    m_currentPose.rotation = glm::normalize(pitchInversion * rawRotation);
    
    m_currentPose.timestamp = imu.timestamp;
    m_currentPose.confidence = calculatePoseConfidence();
    
    if (m_poseCallback) {
        m_poseCallback(m_currentPose);
    }
}

void SensorFusionEKF::updateFeaturePoints(const std::vector<FeaturePoint> &features)
{
    // TODO: Реализовать обновление по feature points
}

void SensorFusionEKF::updateLidar(const std::vector<glm::vec3> &lidarPoints)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    updateLidarStep(lidarPoints);
}

void SensorFusionEKF::updateVisualOdometry(const glm::vec3 &visualPosition, const glm::quat &visualRotation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    updateVisualStep(visualPosition, visualRotation);
}

CameraPose SensorFusionEKF::getCurrentPose() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentPose;
}

float SensorFusionEKF::getStability() const
{
    return calculateStability();
}

glm::vec3 SensorFusionEKF::getVelocity() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return stateToVelocity();
}

void SensorFusionEKF::setNoiseParameters(double gyroNoise, double accelNoise,
                                         double gyroBiasNoise, double accelBiasNoise,
                                         double visualNoise, double lidarNoise)
{
    m_gyroNoise = gyroNoise;
    m_accelNoise = accelNoise;
    m_gyroBiasNoise = gyroBiasNoise;
    m_accelBiasNoise = accelBiasNoise;
    m_visualNoise = visualNoise;
    m_lidarNoise = lidarNoise;
}

void SensorFusionEKF::setPoseCallback(PoseCallback callback)
{
    m_poseCallback = callback;
}

void SensorFusionEKF::setStabilityCallback(StabilityCallback callback)
{
    m_stabilityCallback = callback;
}

void SensorFusionEKF::predictSimple(double dt, const RawIMUData &imu)
{
    glm::quat currentOrientation = stateToQuaternion();
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    float gyroMagnitude = glm::length(gyro);
    
    // 1. Integrate gyroscope only when moving
    // When stationary: gyro noise accumulates as rotation drift — skip integration
    if (dt > 0 && dt < 0.1 && gyroMagnitude > 0.02f) {
        float halfDt = static_cast<float>(dt * 0.5);
        glm::quat gyroDelta = glm::quat(1.0f, 
            static_cast<float>(gyro.x) * halfDt, 
            static_cast<float>(gyro.y) * halfDt, 
            static_cast<float>(gyro.z) * halfDt);
        glm::quat integrated = glm::normalize(currentOrientation * gyroDelta);
        quaternionToState(integrated);
    }
    
    // 2. Gravity correction: slow, pitch/roll only, only when moving
    // This corrects long-term drift without causing jitter when stationary
    if (gyroMagnitude > 0.05f) {
        glm::vec3 measuredGravity(imu.gravityX, imu.gravityY, imu.gravityZ);
        float gravityLen = glm::length(measuredGravity);
        
        if (gravityLen > 0.5f && gravityLen < 1.5f) {
            glm::vec3 normGrav = measuredGravity / gravityLen;
            glm::quat orient = stateToQuaternion();
            glm::vec3 expected = orient * glm::vec3(0, -1, 0);
            float err = glm::length(normGrav - expected);
            
            if (err > 0.05f) {
                glm::vec3 axis = glm::cross(expected, normGrav);
                float angle = glm::asin(glm::clamp(glm::length(axis), 0.0f, 1.0f));
                
                if (glm::length(axis) > 0.001f && angle > 0.001f) {
                    axis = glm::normalize(axis);
                    axis.y = 0.0f; // Only pitch/roll, never yaw
                    if (glm::length(axis) > 0.0001f) {
                        axis = glm::normalize(axis);
                        angle *= 0.01f; // 1% per frame when moving
                        glm::quat correction = glm::angleAxis(angle, axis);
                        quaternionToState(glm::normalize(correction * orient));
                    }
                }
            }
        }
    }
    
    // Position: not from IMU (DLO provides it)
}

void SensorFusionEKF::predictStep(double dt)
{
    // Базовая реализация predict step
    // Экстраполируем позицию из скорости для краткосрочного трекинга
    // Визуальная одометрия будет корректировать долгосрочный дрифт
    if (dt <= 0) return;
    
    // Экстраполяция позиции: x = x + v * dt
    m_state[0] += m_state[3] * dt; // x
    m_state[1] += m_state[4] * dt; // y
    m_state[2] += m_state[5] * dt; // z
    
    // Обновление ковариации (упрощенное)
    for (int i = 0; i < STATE_SIZE; ++i) {
        m_covariance[i * STATE_SIZE + i] += m_processNoise[i * STATE_SIZE + i] * dt;
    }
}

void SensorFusionEKF::updateIMUStep(const RawIMUData &imu, double dt)
{
    // Базовая реализация IMU update
    // Обновляем ориентацию и позицию из IMU для краткосрочного трекинга
    if (dt <= 0) return;
    
    // Обновляем ориентацию на основе гироскопа
    glm::quat currentRot = stateToQuaternion();
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Простая интеграция угловой скорости
    glm::quat deltaRot = glm::quat(1.0f, gyro.x * dt * 0.5f, gyro.y * dt * 0.5f, gyro.z * dt * 0.5f);
    glm::quat newRot = currentRot * deltaRot;
    newRot = glm::normalize(newRot);
    
    quaternionToState(newRot);
    
    // Обновляем скорость и позицию из userAcceleration (уже без гравитации!)
    glm::vec3 userAccel(imu.accelX, imu.accelY, imu.accelZ);
    m_state[3] += userAccel.x * dt; // vx
    m_state[4] += userAccel.y * dt; // vy
    m_state[5] += userAccel.z * dt; // vz
    
    // Интегрируем скорость для позиции
    m_state[0] += m_state[3] * dt; // x
    m_state[1] += m_state[4] * dt; // y
    m_state[2] += m_state[5] * dt; // z
}

void SensorFusionEKF::updateVisualStep(const glm::vec3 &visualPosition, const glm::quat &visualRotation)
{
    // PURE SENSOR VIO: store VO position for delta computation in predictSimple

    if (std::isnan(visualPosition.x) || std::isnan(visualPosition.y) || std::isnan(visualPosition.z) ||
        std::isinf(visualPosition.x) || std::isinf(visualPosition.y) || std::isinf(visualPosition.z)) {
        return;
    }
    if (glm::length(visualPosition) > 100.0f) return;

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    if (m_lastVOTime > 0) {
        m_voDt = (now - m_lastVOTime) / 1000.0;
    }
    m_lastVOTime = now;

    if (glm::length(m_lastVOPosition) < 0.0001f) {
        m_lastVOPosition = stateToPosition();
    }

    m_voPosition = visualPosition;
    m_hasVOUpdate = true;

    // Rotation: trust IMU only (visualRotation has scale ambiguity)
}

void SensorFusionEKF::updateLidarStep(const std::vector<glm::vec3> &lidarPoints)
{
    // APPLE VISION PRO / ARKit СТИЛЬ: LiDAR для коррекции глубины и позиции
    if (lidarPoints.empty()) return;

    glm::vec3 currentPos = stateToPosition();

    // --- 1. Медианная коррекция Z (глубина) ---
    // Собираем все Z точек в пределах 0.5–5.0 метров
    std::vector<float> validZ;
    validZ.reserve(lidarPoints.size());
    for (const auto& p : lidarPoints) {
        if (p.z > 0.5f && p.z < 5.0f) {
            validZ.push_back(p.z);
        }
    }
    if (!validZ.empty()) {
        std::nth_element(validZ.begin(), validZ.begin() + validZ.size()/2, validZ.end());
        float medianZ = validZ[validZ.size() / 2];

        // Корректируем Z камеры: LiDAR говорит "впереди объект на расстоянии medianZ"
        // Но нам нужно понять, как это влияет на позицию камеры.
        // Если medianZ сильно отличается от текущего Z, корректируем с alpha=0.15
        double alpha = 0.15;
        m_state[2] = m_state[2] * (1.0 - alpha) + medianZ * alpha;
    }

    // --- 2. Коррекция высоты (Y) по полу ---
    // Ищем точки с минимальным Y (в camera space это примерно пол под камерой)
    float minY = std::numeric_limits<float>::max();
    for (const auto& p : lidarPoints) {
        if (p.y < minY && p.z > 0.3f && p.z < 3.0f) {
            minY = p.y;
        }
    }
    if (minY < std::numeric_limits<float>::max()) {
        // В camera space Y смотрит вниз. minY ≈ высота камеры над полом.
        // Корректируем Y позиции: предполагаем, что пол на y=0 в world space
        double alpha = 0.10;
        // Если minY = 1.5, значит камера на высоте ~1.5м
        // Текущая world-Y должна быть примерно minY (в зависимости от ориентации)
        // Упрощенно: считаем, что Y камеры = |minY| (для горизонтального держания)
        float targetY = std::abs(minY);
        m_state[1] = m_state[1] * (1.0 - alpha) + targetY * alpha;
    }
}

glm::quat SensorFusionEKF::stateToQuaternion() const
{
    return glm::quat(static_cast<float>(m_state[6]), 
                     static_cast<float>(m_state[7]), 
                     static_cast<float>(m_state[8]), 
                     static_cast<float>(m_state[9]));
}

void SensorFusionEKF::quaternionToState(const glm::quat &q)
{
    m_state[6] = q.w;
    m_state[7] = q.x;
    m_state[8] = q.y;
    m_state[9] = q.z;
}

glm::vec3 SensorFusionEKF::stateToPosition() const
{
    return glm::vec3(static_cast<float>(m_state[0]), 
                     static_cast<float>(m_state[1]), 
                     static_cast<float>(m_state[2]));
}

glm::vec3 SensorFusionEKF::stateToVelocity() const
{
    return glm::vec3(static_cast<float>(m_state[3]), 
                     static_cast<float>(m_state[4]), 
                     static_cast<float>(m_state[5]));
}

glm::vec3 SensorFusionEKF::stateToGyroBias() const
{
    return glm::vec3(static_cast<float>(m_state[10]), 
                     static_cast<float>(m_state[11]), 
                     static_cast<float>(m_state[12]));
}

glm::vec3 SensorFusionEKF::stateToAccelBias() const
{
    return glm::vec3(static_cast<float>(m_state[13]), 
                     static_cast<float>(m_state[14]), 
                     static_cast<float>(m_state[15]));
}

float SensorFusionEKF::calculatePoseConfidence() const
{
    return getStability();
}

float SensorFusionEKF::calculateStability() const
{
    if (m_poseHistory.size() < 2) {
        return 0.5f;
    }
    
    // Простая метрика стабильности на основе вариации последних поз
    float variance = 0.0f;
    for (size_t i = 1; i < m_poseHistory.size(); ++i) {
        glm::vec3 diff = m_poseHistory[i].position - m_poseHistory[i-1].position;
        variance += glm::dot(diff, diff);
    }
    variance /= m_poseHistory.size() - 1;
    
    return std::max(0.0f, std::min(1.0f, 1.0f - variance * 10.0f));
}

void SensorFusionEKF::matrixMultiply(const std::vector<double> &A, const std::vector<double> &B,
                                      std::vector<double> &result, int rowsA, int colsA, int colsB)
{
    result.resize(rowsA * colsB);
    std::fill(result.begin(), result.end(), 0.0);
    
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            for (int k = 0; k < colsA; ++k) {
                result[i * colsB + j] += A[i * colsA + k] * B[k * colsB + j];
            }
        }
    }
}

void SensorFusionEKF::matrixTranspose(const std::vector<double> &input, std::vector<double> &output, int rows, int cols)
{
    output.resize(rows * cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            output[j * rows + i] = input[i * cols + j];
        }
    }
}

bool SensorFusionEKF::matrixInvert3x3(const std::vector<double> &input, std::vector<double> &output)
{
    // Реализация инверсии 3x3 матрицы через правило Крамера
    if (input.size() < 9) {
        output = input;
        return false;
    }
    
    // Извлекаем элементы матрицы
    double a = input[0], b = input[1], c = input[2];
    double d = input[3], e = input[4], f = input[5];
    double g = input[6], h = input[7], i = input[8];
    
    // Вычисляем детерминант
    double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    
    if (std::abs(det) < 1e-10) {
        output = input;
        return false; // Матрица вырожденная
    }
    
    double invDet = 1.0 / det;
    
    // Вычисляем обратную матрицу
    output.resize(9);
    output[0] = (e * i - f * h) * invDet;
    output[1] = (c * h - b * i) * invDet;
    output[2] = (b * f - c * e) * invDet;
    output[3] = (f * g - d * i) * invDet;
    output[4] = (a * i - c * g) * invDet;
    output[5] = (c * d - a * f) * invDet;
    output[6] = (d * h - e * g) * invDet;
    output[7] = (b * g - a * h) * invDet;
    output[8] = (a * e - b * d) * invDet;
    
    return true;
}

} // namespace LensEngine

