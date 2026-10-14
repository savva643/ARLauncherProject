#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "LensEngineAPI.h"
#include "TestSensorGenerator.h"
#include <memory>
#include <fstream>
#include <chrono>
#include <thread>

namespace LensEngine {

/**
 * @brief Тестовый раннер для проверки обработки данных
 * 
 * Генерирует данные с помощью TestSensorGenerator, отправляет в LensEngine,
 * и логирует полученные результаты для сравнения с отправленными.
 */

struct ReceivedDataLog {
    uint64_t timestamp;
    uint64_t sequenceNumber;
    
    // Полученная поза
    glm::vec3 receivedPosition;
    glm::quat receivedRotation;
    float receivedConfidence;
    
    // Ground truth поза (от генератора)
    glm::vec3 groundTruthPosition;
    glm::quat groundTruthRotation;
    
    // Ошибка
    float positionError;
    float rotationError;
    
    // Количество точек
    int featurePointsCount;
    int lidarPointsCount;
};

class TestRunner {
public:
    TestRunner();
    ~TestRunner();
    
    /**
     * @brief Инициализировать тест
     * @param durationSeconds Длительность теста
     * @param fps Целевой FPS
     * @param logFilename Имя файла для логирования
     * @param rgbWidth Ширина RGB (опционально)
     * @param rgbHeight Высота RGB (опционально)
     * @param lidarWidth Ширина LiDAR (опционально)
     * @param lidarHeight Высота LiDAR (опционально)
     */
    bool initialize(int durationSeconds, int fps, const std::string& logFilename,
                    int rgbWidth = 720, int rgbHeight = 405,
                    int lidarWidth = 256, int lidarHeight = 192);
    
    /**
     * @brief Запуск теста
     */
    void run();
    
    /**
     * @brief Получить статистику теста
     */
    struct TestStatistics {
        uint64_t totalFrames;
        uint64_t successfulFrames;
        double avgFPS;
        double avgPositionError;
        double avgRotationError;
        double maxPositionError;
        double maxRotationError;
        double avgConfidence;
    };
    TestStatistics getStatistics() const { return m_stats; }
    
private:
    void logReceivedData(const ReceivedDataLog& log);
    void updateStatistics(const ReceivedDataLog& log);
    
    // Компоненты
    std::unique_ptr<LensEngineAPI> m_lensEngine;
    std::unique_ptr<TestSensorGenerator> m_sensorGenerator;
    
    // Параметры теста
    int m_durationSeconds;
    int m_fps;
    double m_frameInterval;
    
    // Логирование
    std::ofstream m_logFile;
    std::string m_logFilename;
    
    // Статистика
    TestStatistics m_stats;
};

} // namespace LensEngine

#endif // TEST_RUNNER_H
