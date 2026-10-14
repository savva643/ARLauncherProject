#include "TestSensorGenerator.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>

namespace LensEngine {

TestSensorGenerator::TestSensorGenerator()
    : m_rgbWidth(720), m_rgbHeight(405),
      m_lidarWidth(256), m_lidarHeight(192),
      m_sequenceNumber(0),
      m_startTime(0)
{
    m_stats.totalFrames = 0;
    m_stats.totalRGBBytes = 0;
    m_stats.totalLidarBytes = 0;
    m_stats.totalIMUSamples = 0;
    m_stats.avgFPS = 0.0;
    
    m_groundTruthPose.position = glm::vec3(0.0f);
    m_groundTruthPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_velocity = glm::vec3(0.0f);
    m_angularVelocity = glm::vec3(0.0f);
}

TestSensorGenerator::~TestSensorGenerator()
{
    closeLog();
}

bool TestSensorGenerator::initialize(const std::string& logFilename,
                                     int rgbWidth, int rgbHeight,
                                     int lidarWidth, int lidarHeight)
{
    m_rgbWidth = rgbWidth;
    m_rgbHeight = rgbHeight;
    m_lidarWidth = lidarWidth;
    m_lidarHeight = lidarHeight;
    m_logFilename = logFilename;
    
    // Инициализируем ground truth позу
    m_groundTruthPose.position = glm::vec3(0.0f);
    m_groundTruthPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_groundTruthPose.confidence = 1.0f;
    
    // Инициализируем движение
    m_velocity = glm::vec3(0.01f, 0.005f, 0.001f);
    m_angularVelocity = glm::vec3(0.001f, 0.001f, 0.001f);
    
    // Открываем лог файл
    m_logFile.open(logFilename);
    if (!m_logFile.is_open()) {
        std::cerr << "[TestSensorGenerator] Failed to open log file: " << logFilename << std::endl;
        return false;
    }
    
    // Записываем заголовок
    m_logFile << "# Test Sensor Generator Log" << std::endl;
    m_logFile << "# Format: timestamp, seq, rgb_w, rgb_h, rgb_size, lidar_w, lidar_h, lidar_size, lidar_min, lidar_max, lidar_valid, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, grav_x, grav_y, grav_z, gt_pos_x, gt_pos_y, gt_pos_z, gt_rot_w, gt_rot_x, gt_rot_y, gt_rot_z" << std::endl;
    
    std::cout << "[TestSensorGenerator] Initialized: RGB=" << m_rgbWidth << "x" << m_rgbHeight
              << ", LiDAR=" << m_lidarWidth << "x" << m_lidarHeight
              << ", Log=" << logFilename << std::endl;
    
    return true;
}

bool TestSensorGenerator::generateNextFrame(ARFrame& frame)
{
    // Обновляем ground truth позу (симулируем движение)
    updateGroundTruthPose();
    
    // Генерируем timestamp
    auto now = std::chrono::steady_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Заполняем кадр
    frame.sequenceNumber = m_sequenceNumber++;
    frame.timestamp = timestamp;
    
    // RGB данные
    frame.rgbImage.data = generateRGBData();
    frame.rgbImage.width = m_rgbWidth;
    frame.rgbImage.height = m_rgbHeight;
    frame.rgbImage.timestamp = timestamp;
    
    // LiDAR данные
    frame.lidar.depthMap = generateLidarDepthData();
    frame.lidar.confidenceMap = generateLidarConfidenceData();
    frame.lidar.sequenceNumber = frame.sequenceNumber;
    frame.lidar.timestamp = timestamp;
    
    // IMU данные
    frame.imu = generateIMUData();
    
    // Логируем данные
    SensorDataLog log;
    log.timestamp = timestamp;
    log.sequenceNumber = frame.sequenceNumber;
    log.rgbWidth = m_rgbWidth;
    log.rgbHeight = m_rgbHeight;
    log.rgbSize = frame.rgbImage.data.size();
    log.lidarDepthSize = frame.lidar.depthMap.size();
    log.lidarConfidenceSize = frame.lidar.confidenceMap.size();
    log.lidarWidth = m_lidarWidth;
    log.lidarHeight = m_lidarHeight;
    log.accelX = frame.imu.accelX;
    log.accelY = frame.imu.accelY;
    log.accelZ = frame.imu.accelZ;
    log.gyroX = frame.imu.gyroX;
    log.gyroY = frame.imu.gyroY;
    log.gyroZ = frame.imu.gyroZ;
    log.gravityX = frame.imu.gravityX;
    log.gravityY = frame.imu.gravityY;
    log.gravityZ = frame.imu.gravityZ;
    log.groundTruthPosition = m_groundTruthPose.position;
    log.groundTruthRotation = m_groundTruthPose.rotation;
    
    // Вычисляем статистику LiDAR
    const float* depthValues = reinterpret_cast<const float*>(frame.lidar.depthMap.data());
    float minDepth = 1000.0f;
    float maxDepth = 0.0f;
    int validPoints = 0;
    
    for (size_t i = 0; i < frame.lidar.depthMap.size() / sizeof(float); i++) {
        if (depthValues[i] > 0.1f && depthValues[i] < 100.0f) {
            minDepth = std::min(minDepth, depthValues[i]);
            maxDepth = std::max(maxDepth, depthValues[i]);
            validPoints++;
        }
    }
    
    log.lidarMinDepth = minDepth;
    log.lidarMaxDepth = maxDepth;
    log.lidarValidPoints = validPoints;
    
    logSensorData(log);
    
    // Обновляем статистику
    m_stats.totalFrames++;
    m_stats.totalRGBBytes += log.rgbSize;
    m_stats.totalLidarBytes += log.lidarDepthSize + log.lidarConfidenceSize;
    m_stats.totalIMUSamples++;
    
    auto frameDuration = std::chrono::duration<double>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    m_stats.avgFPS = 1.0 / frameDuration;
    
    return true;
}

std::vector<uint8_t> TestSensorGenerator::generateRGBData()
{
    size_t size = m_rgbWidth * m_rgbHeight * 3; // RGB
    std::vector<uint8_t> data(size);
    
    // Генерируем тестовый паттерн (градиент)
    for (uint32_t y = 0; y < m_rgbHeight; y++) {
        for (uint32_t x = 0; x < m_rgbWidth; x++) {
            size_t idx = (y * m_rgbWidth + x) * 3;
            
            // Красный градиент по X
            data[idx + 0] = static_cast<uint8_t>((x * 255) / m_rgbWidth);
            // Зеленый градиент по Y
            data[idx + 1] = static_cast<uint8_t>((y * 255) / m_rgbHeight);
            // Синий - шахматный паттерн
            data[idx + 2] = ((x + y) % 2) ? 255 : 0;
        }
    }
    
    return data;
}

std::vector<uint8_t> TestSensorGenerator::generateLidarDepthData()
{
    size_t size = m_lidarWidth * m_lidarHeight * sizeof(float);
    std::vector<uint8_t> data(size);
    float* depthValues = reinterpret_cast<float*>(data.data());
    
    // Генерируем реалистичную карту глубины
    // Симулируем комнату: пол на 1.5м, стены на 2-5м
    for (int y = 0; y < m_lidarHeight; y++) {
        for (int x = 0; x < m_lidarWidth; x++) {
            int idx = y * m_lidarWidth + x;
            
            // Базовая глубина (плоскость на 3м)
            float depth = 3.0f;
            
            // Добавляем вариации (симулируем объекты)
            // Центр - ближе (симулируем стол)
            float centerX = m_lidarWidth / 2.0f;
            float centerY = m_lidarHeight / 2.0f;
            float distFromCenter = std::sqrt((x - centerX) * (x - centerX) + 
                                            (y - centerY) * (y - centerY));
            
            if (distFromCenter < 30.0f) {
                depth = 1.5f; // Стол на 1.5м
            } else if (distFromCenter < 60.0f) {
                depth = 2.0f; // Края стола
            }
            
            // Добавляем шум
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise(0.0f, 0.05f); // 5cm шум
            depth += noise(gen);
            
            // Ограничиваем диапазон
            depth = std::max(0.1f, std::min(100.0f, depth));
            
            depthValues[idx] = depth;
        }
    }
    
    return data;
}

std::vector<uint8_t> TestSensorGenerator::generateLidarConfidenceData()
{
    size_t size = m_lidarWidth * m_lidarHeight;
    std::vector<uint8_t> data(size);
    
    // Генерируем простую карту уверенности (высокая уверенность для всех точек)
    for (size_t i = 0; i < size; i++) {
        data[i] = 200; // Высокая уверенность
    }
    
    return data;
}

RawIMUData TestSensorGenerator::generateIMUData()
{
    RawIMUData imu;
    
    auto now = std::chrono::steady_clock::now();
    imu.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Генерируем реалистичные IMU данные
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> noise(0.0f, 0.01f);
    
    // Акселерометр (m/s²) - гравитация + движение
    imu.accelX = 0.0f + noise(gen);
    imu.accelY = -9.81f + noise(gen); // Гравитация вниз
    imu.accelZ = 0.0f + noise(gen);
    
    // Гироскоп (rad/s) - вращение (увеличим амплитуду для заметного вращения)
    imu.gyroX = m_angularVelocity.x * 10.0f + noise(gen); // Увеличил в 10 раз
    imu.gyroY = m_angularVelocity.y * 10.0f + noise(gen);
    imu.gyroZ = m_angularVelocity.z * 10.0f + noise(gen);
    
    // Гравитация (нормализованная)
    imu.gravityX = 0.0f;
    imu.gravityY = -1.0f;
    imu.gravityZ = 0.0f;
    
    // Магнитометр (μT)
    imu.magX = 30.0f + noise(gen);
    imu.magY = -10.0f + noise(gen);
    imu.magZ = 45.0f + noise(gen);
    
    // Euler angles (rad)
    imu.pitch = m_angularVelocity.x * 0.016f; // ~16ms
    imu.yaw = m_angularVelocity.y * 0.016f;
    imu.roll = m_angularVelocity.z * 0.016f;
    imu.hasEuler = true;
    
    return imu;
}

void TestSensorGenerator::updateGroundTruthPose()
{
    // Симулируем движение телефона
    // Медленно вращаем и перемещаем
    
    float dt = 0.016f; // ~60 FPS
    
    // Вращение (медленное)
    m_angularVelocity = glm::vec3(
        0.1f * std::sin(m_sequenceNumber * 0.01f), // Pitch
        0.05f * std::cos(m_sequenceNumber * 0.01f), // Yaw
        0.02f * std::sin(m_sequenceNumber * 0.02f)  // Roll
    );
    
    // Позиция (медленное движение по кругу)
    float radius = 0.5f;
    m_velocity = glm::vec3(
        radius * 0.1f * std::cos(m_sequenceNumber * 0.01f),
        0.0f,
        radius * 0.1f * std::sin(m_sequenceNumber * 0.01f)
    );
    
    // Обновляем позицию
    m_groundTruthPose.position += m_velocity * dt;
    
    // Обновляем ротацию
    glm::quat rotationDelta = glm::quat(
        1.0f,
        m_angularVelocity.x * dt * 0.5f,
        m_angularVelocity.y * dt * 0.5f,
        m_angularVelocity.z * dt * 0.5f
    );
    m_groundTruthPose.rotation = rotationDelta * m_groundTruthPose.rotation;
    m_groundTruthPose.rotation = glm::normalize(m_groundTruthPose.rotation);
    
    m_groundTruthPose.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TestSensorGenerator::logSensorData(const SensorDataLog& log)
{
    if (!m_logFile.is_open()) {
        return;
    }
    
    m_logFile << std::fixed << std::setprecision(3);
    m_logFile << log.timestamp << ", " << log.sequenceNumber << ", "
              << log.rgbWidth << ", " << log.rgbHeight << ", " << log.rgbSize << ", "
              << log.lidarWidth << ", " << log.lidarHeight << ", " << log.lidarDepthSize << ", "
              << log.lidarMinDepth << ", " << log.lidarMaxDepth << ", " << log.lidarValidPoints << ", "
              << log.accelX << ", " << log.accelY << ", " << log.accelZ << ", "
              << log.gyroX << ", " << log.gyroY << ", " << log.gyroZ << ", "
              << log.gravityX << ", " << log.gravityY << ", " << log.gravityZ << ", "
              << log.groundTruthPosition.x << ", " << log.groundTruthPosition.y << ", " << log.groundTruthPosition.z << ", "
              << log.groundTruthRotation.w << ", " << log.groundTruthRotation.x << ", "
              << log.groundTruthRotation.y << ", " << log.groundTruthRotation.z << "\n";
    
    m_logFile.flush();
}

void TestSensorGenerator::reset()
{
    m_sequenceNumber = 0;
    m_groundTruthPose.position = glm::vec3(0.0f);
    m_groundTruthPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_velocity = glm::vec3(0.0f);
    m_angularVelocity = glm::vec3(0.0f);
    
    m_stats.totalFrames = 0;
    m_stats.totalRGBBytes = 0;
    m_stats.totalLidarBytes = 0;
    m_stats.totalIMUSamples = 0;
    m_stats.avgFPS = 0.0;
    
    std::cout << "[TestSensorGenerator] Reset" << std::endl;
}

void TestSensorGenerator::closeLog()
{
    if (m_logFile.is_open()) {
        m_logFile.close();
        std::cout << "[TestSensorGenerator] Log file closed: " << m_logFilename << std::endl;
    }
}

} // namespace LensEngine
