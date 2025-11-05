#ifndef LIDAR3DPROCESSOR_H
#define LIDAR3DPROCESSOR_H

#include "LensEngineTypes.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

namespace LensEngine {

/**
 * @brief Обработчик LiDAR данных для 3D пространственного анализа
 */
class Lidar3DProcessor {
public:
    struct SpatialAnalysisResult {
        bool hasFloor = false;
        float floorHeight = 0.0f;
        glm::vec3 floorNormal = glm::vec3(0, 1, 0);
        glm::vec3 gravityDirection = glm::vec3(0, -1, 0);
        std::vector<glm::vec3> planes;
        std::vector<glm::vec3> obstacles;
        std::vector<glm::vec3> walls;
    };

    Lidar3DProcessor();
    ~Lidar3DProcessor();

    // Основные методы
    std::vector<glm::vec3> processDepthData(const std::vector<uint8_t> &depthData);
    std::vector<glm::vec3> processDepthDataFast(const std::vector<uint8_t> &depthData);
    void processLidarDataAsync(const std::vector<uint8_t> &depthData, 
                               const std::vector<uint8_t> &confidenceData = std::vector<uint8_t>());

    // Получение результатов
    SpatialAnalysisResult getLastAnalysis() const;
    std::vector<glm::vec3> getLastProcessedPoints() const;

    // Управление
    void stopProcessing();

    // Колбэки
    using AnalysisCallback = std::function<void(const SpatialAnalysisResult&)>;
    using PointsCallback = std::function<void(const std::vector<glm::vec3>&)>;
    using FloorCallback = std::function<void(const glm::vec3&, float, float)>;
    using ObstaclesCallback = std::function<void(const std::vector<glm::vec3>&)>;
    
    void setAnalysisCallback(AnalysisCallback callback);
    void setPointsCallback(PointsCallback callback);
    void setFloorCallback(FloorCallback callback);
    void setObstaclesCallback(ObstaclesCallback callback);

private:
    // Внутренние методы обработки
    void processLidarInternal(const std::vector<uint8_t> &depthData, const std::vector<uint8_t> &confidenceData);
    SpatialAnalysisResult analyzeSpatialEnvironment(const std::vector<glm::vec3> &points);
    SpatialAnalysisResult analyzeSpatialEnvironmentFast(const std::vector<glm::vec3> &points);

    // Методы обнаружения
    struct PlaneResult {
        glm::vec3 normal;
        glm::vec3 center;
        float distance;
        float confidence;
        std::vector<glm::vec3> inliers;
    };

    PlaneResult detectFloorPlane(const std::vector<glm::vec3> &points);
    std::vector<PlaneResult> detectWallPlanes(const std::vector<glm::vec3> &points);
    std::vector<glm::vec3> detectObstacles(const std::vector<glm::vec3> &points, const glm::vec3 &cameraPosition);
    PlaneResult ransacPlaneDetection(const std::vector<glm::vec3> &points, int maxIterations);

    // Утилиты
    std::vector<glm::vec3> filterValidPoints(const std::vector<glm::vec3> &points);
    std::vector<glm::vec3> removeOutliers(const std::vector<glm::vec3> &points);
    glm::vec3 depthToWorldSpace(int x, int y, float depth);
    float pointToPlaneDistance(const glm::vec3 &point, const glm::vec3 &planeNormal, const glm::vec3 &planePoint);

    // Данные
    mutable std::mutex m_dataLock;
    std::vector<glm::vec3> m_lastProcessedPoints;
    SpatialAnalysisResult m_lastAnalysis;

    // Параметры обработки
    float m_minDepth = 0.1f;
    float m_maxDepth = 10.0f;
    float m_floorDetectionThreshold = 0.1f;
    float m_planeDistanceThreshold = 0.05f;
    float m_horizontalFov = 60.0f;
    float m_verticalFov = 45.0f;

    // Асинхронная обработка
    std::atomic<bool> m_cancelProcessing;
    
    // Колбэки
    AnalysisCallback m_analysisCallback;
    PointsCallback m_pointsCallback;
    FloorCallback m_floorCallback;
    ObstaclesCallback m_obstaclesCallback;
};

} // namespace LensEngine

#endif // LIDAR3DPROCESSOR_H

