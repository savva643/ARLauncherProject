#include "SensorFusionEKF.h"
#include <cmath>
#include <algorithm>
#include <chrono>

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
    m_initialized = true;
}

void SensorFusionEKF::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    initialize();
    m_poseHistory.clear();
}

void SensorFusionEKF::updateIMU(const RawIMUData &imu)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime);
    uint64_t currentTime = elapsed.count();
    
    double dt = (currentTime - m_lastUpdateTime) / 1000.0;
    m_lastUpdateTime = currentTime;

    if (dt <= 0 || dt > 0.1) dt = 0.016; // 60 FPS

    // Простой и быстрый фильтр для AR
    predictSimple(dt, imu);

    // Обновляем позу
    m_currentPose.position = stateToPosition();
    m_currentPose.rotation = stateToQuaternion();
    m_currentPose.timestamp = imu.timestamp;
    m_currentPose.confidence = calculatePoseConfidence();
    
    // Вызываем колбэк
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
    // Интегрируем гироскоп для ориентации
    glm::quat currentOrientation = stateToQuaternion();
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Преобразуем угловую скорость в кватернион
    glm::quat gyroDelta = glm::quat(1.0f, gyro.x * dt * 0.5f, gyro.y * dt * 0.5f, gyro.z * dt * 0.5f);
    glm::quat integratedOrientation = glm::normalize(currentOrientation * gyroDelta);
    quaternionToState(integratedOrientation);

    // Коррекция по гравитации
    glm::vec3 measuredGravity(imu.gravityX, imu.gravityY, imu.gravityZ);
    if (glm::length(measuredGravity) > 0.1f) {
        glm::quat orientationAfterGyro = stateToQuaternion();
        glm::vec3 predictedGravity = orientationAfterGyro * glm::vec3(0, -1, 0);
        glm::vec3 correction = (measuredGravity - predictedGravity) * 0.1f;

        glm::quat correctionQ = glm::quat(1.0f, correction.y * 0.5f, correction.x * 0.5f, correction.z * 0.5f);
        glm::quat newOrientation = glm::normalize(correctionQ * orientationAfterGyro);
        quaternionToState(newOrientation);
    }

    // Интегрируем акселерометр для позиции
    // ВАЖНО: Позиция из акселерометра накапливает ошибки, поэтому используем только для краткосрочного трекинга
    // Для долгосрочного трекинга нужна визуальная одометрия
    glm::vec3 acceleration(imu.accelX, imu.accelY, imu.accelZ);
    glm::vec3 velocity = stateToVelocity();
    glm::vec3 position = stateToPosition();
    
    // Удаляем гравитацию из акселерометра
    glm::quat orientation = stateToQuaternion();
    glm::vec3 gravityWorld = orientation * glm::vec3(0, -m_gravity, 0);
    glm::vec3 linearAccel = acceleration - gravityWorld;
    
    // Обновляем скорость и позицию
    velocity += linearAccel * static_cast<float>(dt);
    
    // Ограничиваем скорость для предотвращения накопления ошибок
    float maxVelocity = 10.0f; // м/с
    if (glm::length(velocity) > maxVelocity) {
        velocity = glm::normalize(velocity) * maxVelocity;
    }
    
    position += velocity * static_cast<float>(dt);
    
    // Ограничиваем позицию для предотвращения огромных значений
    // Если позиция слишком большая, сбрасываем её (возможно, устройство не двигалось)
    float maxPosition = 100.0f; // метры
    if (glm::length(position) > maxPosition) {
        // Сбрасываем позицию, если она слишком большая (вероятно, накопление ошибок)
        position = glm::vec3(0.0f);
        velocity = glm::vec3(0.0f);
    }
    
    // Сохраняем в состояние
    m_state[0] = position.x;
    m_state[1] = position.y;
    m_state[2] = position.z;
    m_state[3] = velocity.x;
    m_state[4] = velocity.y;
    m_state[5] = velocity.z;
}

void SensorFusionEKF::predictStep(double dt)
{
    // Базовая реализация predict step
    // Обновляем позицию и скорость на основе текущего состояния
    if (dt <= 0) return;
    
    // Простая экстраполяция позиции: x = x + v * dt
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
    if (dt <= 0) return;
    
    // Обновляем ориентацию на основе гироскопа
    glm::quat currentRot = stateToQuaternion();
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Простая интеграция угловой скорости
    glm::quat deltaRot = glm::quat(1.0f, gyro.x * dt * 0.5f, gyro.y * dt * 0.5f, gyro.z * dt * 0.5f);
    glm::quat newRot = currentRot * deltaRot;
    newRot = glm::normalize(newRot);
    
    quaternionToState(newRot);
    
    // Обновляем скорость на основе акселерометра
    glm::vec3 accel(imu.accelX, imu.accelY, imu.accelZ);
    m_state[3] += accel.x * dt; // vx
    m_state[4] += accel.y * dt; // vy
    m_state[5] += accel.z * dt; // vz
}

void SensorFusionEKF::updateVisualStep(const glm::vec3 &visualPosition, const glm::quat &visualRotation)
{
    // Базовая реализация visual update
    // Простое слияние с визуальной одометрией
    double alpha = 0.1; // Коэффициент смешивания
    
    glm::vec3 currentPos = stateToPosition();
    glm::quat currentRot = stateToQuaternion();
    
    // Смешиваем позицию
    glm::vec3 newPos = currentPos * static_cast<float>(1.0 - alpha) + visualPosition * static_cast<float>(alpha);
    m_state[0] = static_cast<double>(newPos.x);
    m_state[1] = static_cast<double>(newPos.y);
    m_state[2] = static_cast<double>(newPos.z);
    
    // Смешиваем ориентацию
    glm::quat newRot = glm::slerp(currentRot, visualRotation, static_cast<float>(alpha));
    quaternionToState(newRot);
}

void SensorFusionEKF::updateLidarStep(const std::vector<glm::vec3> &lidarPoints)
{
    // Базовая реализация LiDAR update
    if (lidarPoints.empty()) return;
    
    // Находим ближайшую точку к полу (y=0)
    glm::vec3 floorPoint(0.0f);
    float minY = std::numeric_limits<float>::max();
    
    for (const auto& point : lidarPoints) {
        if (point.y < minY && point.y > -0.5f) { // Ищем точки близко к полу
            minY = point.y;
            floorPoint = point;
        }
    }
    
    // Обновляем позицию по Z (глубина) на основе LiDAR
    if (minY < std::numeric_limits<float>::max()) {
        double alpha = 0.05;
        m_state[2] = m_state[2] * (1.0 - alpha) + floorPoint.z * alpha;
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

