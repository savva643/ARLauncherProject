#include "TestRunner.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace LensEngine {

TestRunner::TestRunner()
    : m_durationSeconds(10), m_fps(60), m_frameInterval(1.0 / 60.0)
{
    m_stats.totalFrames = 0;
    m_stats.successfulFrames = 0;
    m_stats.avgFPS = 0.0;
    m_stats.avgPositionError = 0.0;
    m_stats.avgRotationError = 0.0;
    m_stats.maxPositionError = 0.0;
    m_stats.maxRotationError = 0.0;
    m_stats.avgConfidence = 0.0;
}

TestRunner::~TestRunner()
{
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

bool TestRunner::initialize(int durationSeconds, int fps, const std::string& logFilename,
                            int rgbWidth, int rgbHeight, int lidarWidth, int lidarHeight)
{
    m_durationSeconds = durationSeconds;
    m_frameInterval = 1.0 / fps;
    
    // Создаем генератор сенсоров с заданными размерами
    m_sensorGenerator = std::make_unique<TestSensorGenerator>();
    if (!m_sensorGenerator->initialize("sensor_data.log", rgbWidth, rgbHeight, lidarWidth, lidarHeight)) {
        std::cerr << "[TestRunner] Failed to initialize sensor generator" << std::endl;
        return false;
    }
    
    // Создаем LensEngine через API
    m_lensEngine = std::make_unique<LensEngineAPI>();
    if (!m_lensEngine->initialize()) {
        std::cerr << "[TestRunner] Failed to initialize LensEngine" << std::endl;
        return false;
    }
    
    // Открываем лог файл
    m_logFile.open(logFilename);
    if (!m_logFile.is_open()) {
        std::cerr << "[TestRunner] Failed to open log file: " << logFilename << std::endl;
        return false;
    }
    
    // Записываем заголовок
    m_logFile << "# Test Runner Log - Received Data" << std::endl;
    m_logFile << "# Format: timestamp, seq, recv_pos_x, recv_pos_y, recv_pos_z, recv_rot_w, recv_rot_x, recv_rot_y, recv_rot_z, recv_conf, gt_pos_x, gt_pos_y, gt_pos_z, gt_rot_w, gt_rot_x, gt_rot_y, gt_rot_z, pos_error, rot_error, feat_points, lidar_points" << std::endl;
    
    std::cout << "[TestRunner] Initialized: duration=" << m_durationSeconds 
              << "s, fps=" << fps << ", log=" << logFilename 
              << ", RGB=" << rgbWidth << "x" << rgbHeight 
              << ", LiDAR=" << lidarWidth << "x" << lidarHeight << std::endl;
    
    return true;
}

void TestRunner::run()
{
    
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(m_durationSeconds);
    
    int frameCount = 0;
    
    while (std::chrono::steady_clock::now() < endTime) {
        auto frameStart = std::chrono::steady_clock::now();
        
        
        // Генерируем кадр данных
        ARFrame frame;
        if (!m_sensorGenerator->generateNextFrame(frame)) {
            std::cerr << "[TestRunner] Failed to generate frame" << std::endl;
            continue;
        }
        
        
        // Получаем ground truth позу
        CameraPose groundTruth = m_sensorGenerator->getGroundTruthPose();
        
        
        // Отправляем данные в LensEngine
        m_lensEngine->processRGBData(
            frame.rgbImage.data.data(),
            frame.rgbImage.data.size(),
            frame.rgbImage.width,
            frame.rgbImage.height,
            frame.rgbImage.timestamp
        );
        
        
        m_lensEngine->processLidarData(
            frame.lidar.depthMap.data(),
            frame.lidar.depthMap.size(),
            frame.lidar.confidenceMap.data(),
            frame.lidar.confidenceMap.size(),
            frame.lidar.timestamp
        );
        
        
        m_lensEngine->processIMUData(frame.imu);
        
        
        // Получаем результат от LensEngine
        CameraPose receivedPose = m_lensEngine->getCurrentCameraPose();
        std::vector<FeaturePoint> featurePoints = m_lensEngine->getFeaturePoints();
        std::vector<glm::vec3> lidarPoints = m_lensEngine->getLidarPoints();
        
        
        // Логируем полученные данные
        ReceivedDataLog log;
        log.timestamp = receivedPose.timestamp;
        log.sequenceNumber = frame.sequenceNumber;
        log.receivedPosition = receivedPose.position;
        log.receivedRotation = receivedPose.rotation;
        log.receivedConfidence = receivedPose.confidence;
        log.groundTruthPosition = groundTruth.position;
        log.groundTruthRotation = groundTruth.rotation;
        log.featurePointsCount = featurePoints.size();
        log.lidarPointsCount = lidarPoints.size();
        
        // Вычисляем ошибку
        log.positionError = glm::length(log.receivedPosition - log.groundTruthPosition);
        
        // Ошибка вращения (угол между кватернионами)
        float dotProduct = glm::dot(log.receivedRotation, log.groundTruthRotation);
        dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f);
        log.rotationError = 2.0f * std::acos(std::abs(dotProduct));
        
        logReceivedData(log);
        updateStatistics(log);
        
        frameCount++;
        
        // Поддерживаем целевой FPS
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration<double>(frameEnd - frameStart).count();
        if (frameDuration < m_frameInterval) {
            std::this_thread::sleep_for(std::chrono::duration<double>(m_frameInterval - frameDuration));
        }
        
        // Прогресс каждые 60 кадров
        if (frameCount % 60 == 0) {
            auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
            std::cout << "[TestRunner] Progress: " << frameCount << " frames, "
                      << std::fixed << std::setprecision(1) << elapsed << "s / " 
                      << m_durationSeconds << "s, "
                      << "Avg Pos Error: " << m_stats.avgPositionError * 100 << "cm, "
                      << "Avg Rot Error: " << m_stats.avgRotationError * 57.3 << "deg" << std::endl;
        }
        
        // Остановимся после 60 кадров для полноценного теста
        if (frameCount >= 60) {
            std::cout << "[TestRunner] Stopping after 60 frames" << std::endl;
            break;
        }
    }
    
    // Финальная статистика
    auto totalElapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    m_stats.avgFPS = frameCount / totalElapsed;
    
    std::cout << "\n[TestRunner] Test completed!" << std::endl;
    std::cout << "  Total frames: " << m_stats.totalFrames << std::endl;
    std::cout << "  Successful frames: " << m_stats.successfulFrames << std::endl;
    std::cout << "  Avg FPS: " << std::fixed << std::setprecision(1) << m_stats.avgFPS << std::endl;
    std::cout << "  Avg Position Error: " << m_stats.avgPositionError * 100 << " cm" << std::endl;
    std::cout << "  Max Position Error: " << m_stats.maxPositionError * 100 << " cm" << std::endl;
    std::cout << "  Avg Rotation Error: " << m_stats.avgRotationError * 57.3 << " deg" << std::endl;
    std::cout << "  Max Rotation Error: " << m_stats.maxRotationError * 57.3 << " deg" << std::endl;
    std::cout << "  Avg Confidence: " << m_stats.avgConfidence << std::endl;
    
    m_logFile.close();
}

void TestRunner::logReceivedData(const ReceivedDataLog& log)
{
    if (!m_logFile.is_open()) {
        return;
    }
    
    m_logFile << std::fixed << std::setprecision(6);
    m_logFile << log.timestamp << ", " << log.sequenceNumber << ", "
              << log.receivedPosition.x << ", " << log.receivedPosition.y << ", " << log.receivedPosition.z << ", "
              << log.receivedRotation.w << ", " << log.receivedRotation.x << ", "
              << log.receivedRotation.y << ", " << log.receivedRotation.z << ", "
              << log.receivedConfidence << ", "
              << log.groundTruthPosition.x << ", " << log.groundTruthPosition.y << ", " << log.groundTruthPosition.z << ", "
              << log.groundTruthRotation.w << ", " << log.groundTruthRotation.x << ", "
              << log.groundTruthRotation.y << ", " << log.groundTruthRotation.z << ", "
              << log.positionError << ", " << log.rotationError << ", "
              << log.featurePointsCount << ", " << log.lidarPointsCount << "\n";
    
    m_logFile.flush();
}

void TestRunner::updateStatistics(const ReceivedDataLog& log)
{
    m_stats.totalFrames++;
    
    if (log.receivedConfidence > 0.0f) {
        m_stats.successfulFrames++;
    }
    
    // Скользящее среднее для ошибки
    double alpha = 0.1;
    m_stats.avgPositionError = m_stats.avgPositionError * (1.0 - alpha) + log.positionError * alpha;
    m_stats.avgRotationError = m_stats.avgRotationError * (1.0 - alpha) + log.rotationError * alpha;
    m_stats.avgConfidence = m_stats.avgConfidence * (1.0 - alpha) + log.receivedConfidence * alpha;
    
    // Максимум
    m_stats.maxPositionError = std::max(m_stats.maxPositionError, static_cast<double>(log.positionError));
    m_stats.maxRotationError = std::max(m_stats.maxRotationError, static_cast<double>(log.rotationError));
}

} // namespace LensEngine
