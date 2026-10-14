#include "KalmanFilter6DOF.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

namespace LensEngine {

KalmanFilter6DOF::KalmanFilter6DOF()
{
    m_imuDiagnostics = std::make_unique<IMUDiagnostics>();
    initialize();
}

KalmanFilter6DOF::~KalmanFilter6DOF()
{
}

void KalmanFilter6DOF::initialize()
{
    // Инициализируем состояние
    m_state.fill(0.0f);
    m_state[6] = 1.0f;  // qw = 1 (identity quaternion)
    
    // Инициализируем ковариацию (большая неопределённость в начале)
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            m_covariance[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    m_frameCount = 0;
    m_lastVOConfidence = 0.0f;
    
    if (m_imuDiagnostics) {
        m_imuDiagnostics->reset();
    }
    
    std::cout << "[KF6DOF] Initialized" << std::endl;
}

void KalmanFilter6DOF::predictIMU(const RawIMUData& imu, double dt)
{
    if (dt < 0.001 || dt > 0.1) {
        return;  // Игнорируем неправильные dt
    }
    
    // Диагностика IMU
    if (m_imuDiagnostics) {
        m_imuDiagnostics->checkIMUData(imu);
    }
    
    // Получаем IMU данные
    glm::vec3 accel(imu.accelX, imu.accelY, imu.accelZ);
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Вычитаем bias
    accel -= m_accelBias;
    gyro -= m_gyroBias;
    
    // Фильтруем шум (низкопроходный фильтр)
    static glm::vec3 filteredAccel(0.0f);
    static glm::vec3 filteredGyro(0.0f);
    float alpha = 0.1f;
    filteredAccel = filteredAccel * (1.0f - alpha) + accel * alpha;
    filteredGyro = filteredGyro * (1.0f - alpha) + gyro * alpha;
    
    // NOTE: iPhone отправляет userAcceleration (БЕЗ гравитации)
    // Поэтому нам не нужно вычитать гравитацию
    
    // Prediction step
    predictStep(filteredAccel, filteredGyro, dt);
}

void KalmanFilter6DOF::updateVO(const glm::vec3& voPosition, const glm::quat& voRotation, double confidence)
{
    if (confidence < 0.1f) {
        return;  // Игнорируем низкоуверенные измерения
    }
    
    // Update step
    updateStep(voPosition, voRotation, static_cast<float>(confidence));
    m_lastVOConfidence = static_cast<float>(confidence);
}

CameraPose KalmanFilter6DOF::getCurrentPose() const
{
    CameraPose pose;
    pose.position = glm::vec3(m_state[0], m_state[1], m_state[2]);
    pose.rotation = glm::quat(m_state[9], m_state[6], m_state[7], m_state[8]);  // qw, qx, qy, qz
    pose.confidence = m_lastVOConfidence;
    pose.timestamp = 0;
    return pose;
}

void KalmanFilter6DOF::reset()
{
    initialize();
}

void KalmanFilter6DOF::predictStep(const glm::vec3& accel, const glm::vec3& gyro, double dt)
{
    // === PREDICTION STEP ===
    // Используем простую кинематику + IMU
    
    // Текущее состояние
    glm::vec3 pos(m_state[0], m_state[1], m_state[2]);
    glm::vec3 vel(m_state[3], m_state[4], m_state[5]);
    glm::quat rot(m_state[9], m_state[6], m_state[7], m_state[8]);
    
    // === Обновляем ротацию (gyroscope) ===
    glm::quat deltaRot = quatFromGyro(gyro, dt);
    rot = quatMultiply(rot, deltaRot);
    rot = quatNormalize(rot);
    
    // === Обновляем позицию и скорость (accelerometer) ===
    // Преобразуем ускорение в world frame
    glm::mat3 rotMat = glm::mat3_cast(rot);
    glm::vec3 worldAccel = rotMat * accel;
    
    // NOTE: iPhone уже отправляет userAcceleration (БЕЗ гравитации)
    // Поэтому не нужно вычитать гравитацию
    
    // === Детектор неподвижности ===
    float accelMag = glm::length(accel);
    float gyroMag = glm::length(gyro);
    bool isStationary = (accelMag < 0.1f && gyroMag < 0.05f);
    
    // Интегрируем скорость и позицию
    vel += worldAccel * static_cast<float>(dt);
    pos += vel * static_cast<float>(dt);
    
    // === Затухание скорости (предотвращает дрифт) ===
    // Если телефон неподвижен - очень агрессивное затухание
    float decayFactor = isStationary ? 0.90f : 0.98f;  // 10% или 2% затухание за кадр
    vel *= decayFactor;
    
    // Сохраняем флаг неподвижности для updateStep
    m_isStationary = isStationary;
    
    // === Сохраняем обновленное состояние ===
    m_state[0] = pos.x;
    m_state[1] = pos.y;
    m_state[2] = pos.z;
    m_state[3] = vel.x;
    m_state[4] = vel.y;
    m_state[5] = vel.z;
    m_state[6] = rot.x;
    m_state[7] = rot.y;
    m_state[8] = rot.z;
    m_state[9] = rot.w;
    
    m_frameCount++;
}

void KalmanFilter6DOF::updateStep(const glm::vec3& voPos, const glm::quat& voRot, float confidence)
{
    // === UPDATE STEP (VO correction) ===
    // Используем VO для коррекции дрифта
    
    // 🔹 ВАЖНО: отклоняем VO когда телефон неподвижен
    // VO ненадежна в этом случае (нет движения для отслеживания)
    if (m_isStationary) {
        return;  // Не применяем VO обновление когда телефон неподвижен
    }
    
    // Текущая позиция
    glm::vec3 currentPos(m_state[0], m_state[1], m_state[2]);
    
    // Инновация (разница между VO и фильтром)
    glm::vec3 innovation = voPos - currentPos;
    float innovationMag = glm::length(innovation);
    
    // Отклоняем слишком большие инновации (ошибки отслеживания)
    if (innovationMag > 0.5f) {
        std::cout << "[KF6DOF] Rejected large innovation: " << std::fixed << std::setprecision(3) 
                  << innovationMag << "m" << std::endl;
        return;
    }
    
    // Gain (зависит от confidence)
    // Высокая confidence → больше доверяем VO
    // Низкая confidence → больше доверяем фильтру
    float kalmanGain = 0.3f * confidence;  // 0-0.3
    
    // Обновляем позицию
    glm::vec3 correctedPos = currentPos + innovation * kalmanGain;
    
    // Обновляем ротацию (более консервативно)
    glm::quat currentRot(m_state[9], m_state[6], m_state[7], m_state[8]);
    glm::quat correctedRot = glm::slerp(currentRot, voRot, kalmanGain * 0.5f);
    correctedRot = quatNormalize(correctedRot);
    
    // === TEMPORAL FILTERING ===
    // Применяем сглаживание для убирания шума и скачков
    glm::vec3 smoothedPos = m_temporalFilter.filterPosition(correctedPos);
    glm::quat smoothedRot = m_temporalFilter.filterRotation(correctedRot);
    
    // Сохраняем обновленное состояние
    m_state[0] = smoothedPos.x;
    m_state[1] = smoothedPos.y;
    m_state[2] = smoothedPos.z;
    m_state[6] = smoothedRot.x;
    m_state[7] = smoothedRot.y;
    m_state[8] = smoothedRot.z;
    m_state[9] = smoothedRot.w;
    
    if (m_frameCount % 30 == 0) {
        std::cout << "[KF6DOF] Updated with VO (confidence: " << std::fixed << std::setprecision(2) 
                  << confidence << ", innovation: " << innovationMag << "m)" << std::endl;
    }
}

glm::quat KalmanFilter6DOF::quatFromGyro(const glm::vec3& gyro, double dt) const
{
    // Преобразуем gyro vector в quaternion (Rodrigues formula)
    float angle = glm::length(gyro) * static_cast<float>(dt);
    
    if (angle < 0.0001f) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    }
    
    glm::vec3 axis = glm::normalize(gyro);
    return glm::angleAxis(angle, axis);
}

glm::quat KalmanFilter6DOF::quatMultiply(const glm::quat& q1, const glm::quat& q2) const
{
    // q1 * q2
    return glm::quat(
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w
    );
}

glm::quat KalmanFilter6DOF::quatNormalize(const glm::quat& q) const
{
    float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 0.0001f) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
    return glm::quat(q.w / len, q.x / len, q.y / len, q.z / len);
}

void KalmanFilter6DOF::updateLidar(const std::vector<glm::vec3>& lidarPoints)
{
    if (lidarPoints.empty()) {
        return;
    }
    
    // === LIDAR FUSION ===
    // Используем LiDAR для коррекции масштаба и глубины
    
    // 1. Вычисляем среднюю глубину из LiDAR
    float lidarDepth = 0.0f;
    int validPoints = 0;
    
    for (const auto& pt : lidarPoints) {
        // Фильтруем выбросы (0.1m - 100m)
        if (pt.z > 0.1f && pt.z < 100.0f) {
            lidarDepth += pt.z;
            validPoints++;
        }
    }
    
    if (validPoints == 0) {
        return;
    }
    
    lidarDepth /= validPoints;
    
    // 2. Вычисляем ожидаемую глубину из текущей позиции
    // (расстояние от камеры до объектов)
    glm::vec3 currentPos(m_state[0], m_state[1], m_state[2]);
    float expectedDepth = glm::length(currentPos);
    
    if (expectedDepth < 0.1f) {
        expectedDepth = 1.0f;  // Если слишком близко, используем default
    }
    
    // 3. Вычисляем масштабный коэффициент
    float scaleCorrection = lidarDepth / expectedDepth;
    
    // Ограничиваем коррекцию (не более ±20%)
    scaleCorrection = glm::clamp(scaleCorrection, 0.8f, 1.2f);
    
    // 4. Исправляем позицию и скорость (масштаб влияет на всю траекторию!)
    m_state[0] *= scaleCorrection;  // x
    m_state[1] *= scaleCorrection;  // y
    m_state[2] *= scaleCorrection;  // z
    m_state[3] *= scaleCorrection;  // vx
    m_state[4] *= scaleCorrection;  // vy
    m_state[5] *= scaleCorrection;  // vz
    
    // 5. Дополнительно: коррекция Z координаты (высота над полом)
    // Находим минимальную Z (пол)
    float minZ = std::numeric_limits<float>::max();
    for (const auto& pt : lidarPoints) {
        if (pt.z > 0.1f && pt.z < 100.0f) {
            minZ = std::min(minZ, pt.z);
        }
    }
    
    if (minZ < std::numeric_limits<float>::max()) {
        // Ожидаемая высота камеры над полом (~1.5m для iPhone)
        const float EXPECTED_HEIGHT = 1.5f;
        float heightError = EXPECTED_HEIGHT - minZ;
        
        // Применяем коррекцию с малым весом (10%)
        const float ALPHA_HEIGHT = 0.1f;
        m_state[2] += heightError * ALPHA_HEIGHT;
    }
    
    if (m_frameCount % 30 == 0) {
        std::cout << "[KF6DOF] LiDAR update: depth=" << std::fixed << std::setprecision(2) 
                  << lidarDepth << "m, scale=" << scaleCorrection 
                  << ", pos=(" << m_state[0] << ", " << m_state[1] << ", " << m_state[2] << ")" << std::endl;
    }
}

} // namespace LensEngine
