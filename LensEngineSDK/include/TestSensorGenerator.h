#ifndef TEST_SENSOR_GENERATOR_H
#define TEST_SENSOR_GENERATOR_H

#include "LensEngineTypes.h"
#include <vector>
#include <memory>
#include <chrono>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace LensEngine {

/**
 * @brief Тестовый генератор данных сенсоров (имитация iPhone)
 * 
 * Генерирует реалистичные данные как будто от iPhone:
 * - RGB кадры (720x405)
 * - LiDAR depth map (256x192 float)
 * - IMU данные (accel, gyro, gravity, etc.)
 * 
 * Логирует все отправленные данные для сравнения с полученными.
 */

struct SensorDataLog {
    uint64_t timestamp;
    uint64_t sequenceNumber;
    
    // RGB данные
    uint32_t rgbWidth;
    uint32_t rgbHeight;
    size_t rgbSize;
    
    // LiDAR данные
    size_t lidarDepthSize;
    size_t lidarConfidenceSize;
    int lidarWidth;
    int lidarHeight;
    float lidarMinDepth;
    float lidarMaxDepth;
    int lidarValidPoints;
    
    // IMU данные
    double accelX, accelY, accelZ;
    double gyroX, gyroY, gyroZ;
    double gravityX, gravityY, gravityZ;
    
    // Сгенерированная поза (ground truth)
    glm::vec3 groundTruthPosition;
    glm::quat groundTruthRotation;
};

class TestSensorGenerator {
public:
    TestSensorGenerator();
    ~TestSensorGenerator();
    
    /**
     * @brief Инициализировать генератор
     * @param logFilename Имя файла для логирования
     * @param rgbWidth Ширина RGB (опционально)
     * @param rgbHeight Высота RGB (опционально)
     * @param lidarWidth Ширина LiDAR (опционально)
     * @param lidarHeight Высота LiDAR (опционально)
     */
    bool initialize(const std::string& logFilename,
                    int rgbWidth = 720, int rgbHeight = 405,
                    int lidarWidth = 256, int lidarHeight = 192);
    
    /**
     * @brief Сгенерировать следующий кадр данных
     * @param frame ARFrame для заполнения
     * @return true если успешно
     */
    bool generateNextFrame(ARFrame& frame);
    
    /**
     * @brief Сгенерировать RGB кадр
     */
    std::vector<uint8_t> generateRGBData();
    
    /**
     * @brief Сгенерировать LiDAR depth map
     */
    std::vector<uint8_t> generateLidarDepthData();
    
    /**
     * @brief Сгенерировать LiDAR confidence map
     */
    std::vector<uint8_t> generateLidarConfidenceData();
    
    /**
     * @brief Сгенерировать IMU данные
     */
    RawIMUData generateIMUData();
    
    /**
     * @brief Получить текущую ground truth позу
     */
    CameraPose getGroundTruthPose() const { return m_groundTruthPose; }
    
    /**
     * @brief Сбросить генератор
     */
    void reset();
    
    /**
     * @brief Закрыть лог файл
     */
    void closeLog();
    
    /**
     * @brief Получить статистику сгенерированных данных
     */
    struct Statistics {
        uint64_t totalFrames;
        uint64_t totalRGBBytes;
        uint64_t totalLidarBytes;
        uint64_t totalIMUSamples;
        double avgFPS;
    };
    Statistics getStatistics() const { return m_stats; }
    
private:
    // Вспомогательные функции
    void logSensorData(const SensorDataLog& log);
    void updateGroundTruthPose();
    glm::vec3 generateLidarPoint(int x, int y, float depth);
    
    // Параметры генерации
    uint32_t m_rgbWidth;
    uint32_t m_rgbHeight;
    int m_lidarWidth;
    int m_lidarHeight;
    
    // Состояние
    uint64_t m_sequenceNumber;
    uint64_t m_startTime;
    CameraPose m_groundTruthPose;
    glm::vec3 m_velocity;
    glm::vec3 m_angularVelocity;
    
    // Логирование
    std::ofstream m_logFile;
    std::string m_logFilename;
    
    // Статистика
    Statistics m_stats;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};

} // namespace LensEngine

#endif // TEST_SENSOR_GENERATOR_H
