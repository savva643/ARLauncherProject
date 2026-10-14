#include "Lidar3DProcessor.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <future>
#include <iostream>

namespace LensEngine {

Lidar3DProcessor::Lidar3DProcessor()
    : m_cancelProcessing(false)
{
    m_planeDetector = std::make_unique<PlaneDetector>();
    m_meshGenerator = std::make_unique<MeshGenerator>();
}

Lidar3DProcessor::~Lidar3DProcessor()
{
    stopProcessing();
}

void Lidar3DProcessor::stopProcessing()
{
    m_cancelProcessing = true;
}

std::vector<glm::vec3> Lidar3DProcessor::processDepthData(const std::vector<uint8_t> &depthData)
{
    size_t totalFloats = depthData.size() / sizeof(float);
    if (totalFloats == 0 || depthData.size() % sizeof(float) != 0) {
        return std::vector<glm::vec3>();
    }

    int width = 256, height = 192;
    if (totalFloats == 256 * 192) { width = 256; height = 192; }
    else if (totalFloats == 320 * 240) { width = 320; height = 240; }
    else if (totalFloats == 240 * 180) { width = 240; height = 180; }
    else {
        height = static_cast<int>(std::sqrt(totalFloats));
        width = static_cast<int>(totalFloats / height);
    }

    const float* depthValues = reinterpret_cast<const float*>(depthData.data());

    std::vector<glm::vec3> points;
    points.reserve((width * height) / 4);

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            int index = y * width + x;
            float depth = depthValues[index];
            if (depth >= m_minDepth && depth <= m_maxDepth && !std::isnan(depth)) {
                points.push_back(depthToWorldSpace(x, y, depth, width, height));
            }
        }
    }
    return points;
}

std::vector<glm::vec3> Lidar3DProcessor::processDepthDataFast(const std::vector<uint8_t> &depthData)
{
    
    // Определяем размеры из размера данных: width * height * sizeof(float)
    size_t totalFloats = depthData.size() / sizeof(float);
    
    if (totalFloats == 0 || depthData.size() % sizeof(float) != 0) {
        return std::vector<glm::vec3>();
    }

    // Пытаемся угадать разрешение: типичные iPhone LiDAR = 256x192 (=49152 floats)
    int width = 256;
    int height = 192;
    if (totalFloats == 256 * 192) {
        width = 256; height = 192;
    } else if (totalFloats == 320 * 240) {
        width = 320; height = 240;
    } else if (totalFloats == 240 * 180) {
        width = 240; height = 180;
    } else if (totalFloats == 128 * 96) {
        width = 128; height = 96;
    } else if (totalFloats == 64 * 48) {
        width = 64; height = 48;
    } else {
        // Неизвестное разрешение — пробуем квадратное или близкое
        height = static_cast<int>(std::sqrt(totalFloats));
        width = static_cast<int>(totalFloats / height);
        if (width * height != static_cast<int>(totalFloats)) {
            // Не удалось подобрать — используем 256x192 как fallback
            width = 256; height = 192;
        }
    }
    

    const float* depthValues = reinterpret_cast<const float*>(depthData.data());

    std::vector<glm::vec3> points;
    
    // Адаптивный шаг sampling в зависимости от разрешения для оптимизации производительности
    int step = 8;  // стандартный шаг
    if (width >= 320) {
        step = 48;  // для 320x240 - агрессивный downsampling для 120 FPS
    } else if (width >= 256) {
        step = 32;  // для средних разрешений
    } else if (width >= 128) {
        step = 16;  // для малых разрешений
    }
    
    points.reserve((width * height) / (step * step));

    for (int y = 0; y < height; y += step) {
        for (int x = 0; x < width; x += step) {
            int index = y * width + x;
            float depth = depthValues[index];

            if (depth >= 0.15f && depth <= 5.0f && !std::isnan(depth)) {
                glm::vec3 point = depthToWorldSpace(x, y, depth, width, height);
                points.push_back(point);
            }
        }
    }

    return points;
}

void Lidar3DProcessor::processLidarDataAsync(const std::vector<uint8_t> &depthData, 
                                              const std::vector<uint8_t> &confidenceData)
{
    m_cancelProcessing = false;
    
    // Сохраняем future, чтобы избежать предупреждения
    auto future = std::async(std::launch::async, [this, depthData, confidenceData]() {
        processLidarInternal(depthData, confidenceData);
    });
    // Сохраняем future в член класса или игнорируем (если не нужен результат)
    (void)future; // Подавляем предупреждение о неиспользуемой переменной
}

Lidar3DProcessor::SpatialAnalysisResult Lidar3DProcessor::getLastAnalysis() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    return m_lastAnalysis;
}

std::vector<glm::vec3> Lidar3DProcessor::getLastProcessedPoints() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    return m_lastProcessedPoints;
}

void Lidar3DProcessor::setAnalysisCallback(AnalysisCallback callback)
{
    m_analysisCallback = callback;
}

void Lidar3DProcessor::setPointsCallback(PointsCallback callback)
{
    m_pointsCallback = callback;
}

void Lidar3DProcessor::setFloorCallback(FloorCallback callback)
{
    m_floorCallback = callback;
}

void Lidar3DProcessor::setObstaclesCallback(ObstaclesCallback callback)
{
    m_obstaclesCallback = callback;
}

void Lidar3DProcessor::processLidarInternal(const std::vector<uint8_t> &depthData, 
                                            const std::vector<uint8_t> &confidenceData)
{
    if (m_cancelProcessing) {
        return;
    }

    std::vector<glm::vec3> points = processDepthDataFast(depthData);
    
    if (m_cancelProcessing) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        m_lastProcessedPoints = points;
    }

    if (m_pointsCallback) {
        m_pointsCallback(points);
    }

    if (m_cancelProcessing) {
        return;
    }

    SpatialAnalysisResult analysis = analyzeSpatialEnvironmentFast(points);
    
    {
        std::lock_guard<std::mutex> lock(m_dataLock);
        m_lastAnalysis = analysis;
    }

    if (m_analysisCallback) {
        m_analysisCallback(analysis);
    }

    if (analysis.hasFloor && m_floorCallback) {
        m_floorCallback(analysis.floorNormal, analysis.floorHeight, 1.0f);
    }

    if (!analysis.obstacles.empty() && m_obstaclesCallback) {
        m_obstaclesCallback(analysis.obstacles);
    }
}

Lidar3DProcessor::SpatialAnalysisResult Lidar3DProcessor::analyzeSpatialEnvironment(const std::vector<glm::vec3> &points)
{
    return analyzeSpatialEnvironmentFast(points);
}

Lidar3DProcessor::SpatialAnalysisResult Lidar3DProcessor::analyzeSpatialEnvironmentFast(const std::vector<glm::vec3> &points)
{
    SpatialAnalysisResult result;
    
    if (points.empty()) {
        return result;
    }

    // Используем PlaneDetector для обнаружения плоскостей
    if (m_planeDetector) {
        auto detectedPlanes = m_planeDetector->detectPlanes(points, 5);
        
        for (const auto& plane : detectedPlanes) {
            if (plane.isFloor && plane.confidence > 0.3f) {
                result.hasFloor = true;
                result.floorHeight = m_planeDetector->getFloorHeight();
                result.floorNormal = plane.normal;
                result.planes.push_back(plane.normal);
            } else if (plane.isWall && plane.confidence > 0.3f) {
                result.walls.push_back(plane.normal);
            }
        }
    }

    // Fallback на старый метод если PlaneDetector не сработал
    if (!result.hasFloor) {
        PlaneResult floor = detectFloorPlane(points);
        if (floor.confidence > 0.7f) {
            result.hasFloor = true;
            result.floorHeight = floor.distance;
            result.floorNormal = floor.normal;
        }
    }

    // Обнаружение препятствий
    result.obstacles = detectObstacles(points, glm::vec3(0.0f));

    return result;
}

Lidar3DProcessor::PlaneResult Lidar3DProcessor::detectFloorPlane(const std::vector<glm::vec3> &points)
{
    return ransacPlaneDetection(points, 100);
}

std::vector<Lidar3DProcessor::PlaneResult> Lidar3DProcessor::detectWallPlanes(const std::vector<glm::vec3> &points)
{
    // TODO: Реализовать обнаружение стен
    return std::vector<PlaneResult>();
}

std::vector<glm::vec3> Lidar3DProcessor::detectObstacles(const std::vector<glm::vec3> &points, const glm::vec3 &cameraPosition)
{
    std::vector<glm::vec3> obstacles;
    
    // Простой метод: точки выше определенной высоты
    for (const auto& point : points) {
        if (point.y > 0.3f) { // Выше 30 см
            obstacles.push_back(point);
        }
    }
    
    return obstacles;
}

Lidar3DProcessor::PlaneResult Lidar3DProcessor::ransacPlaneDetection(const std::vector<glm::vec3> &points, int maxIterations)
{
    PlaneResult bestResult;
    bestResult.confidence = 0.0f;

    if (points.size() < 3) {
        return bestResult;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, points.size() - 1);

    for (int i = 0; i < maxIterations && !m_cancelProcessing; ++i) {
        // Выбираем 3 случайные точки
        int idx1 = dis(gen);
        int idx2 = dis(gen);
        int idx3 = dis(gen);
        
        if (idx1 == idx2 || idx2 == idx3 || idx1 == idx3) {
            continue;
        }

        glm::vec3 v1 = points[idx2] - points[idx1];
        glm::vec3 v2 = points[idx3] - points[idx1];
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        if (std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z)) {
            continue;
        }

        float distance = glm::dot(normal, points[idx1]);
        
        // Подсчитываем inliers
        std::vector<glm::vec3> inliers;
        for (const auto& point : points) {
            float dist = pointToPlaneDistance(point, normal, points[idx1]);
            if (std::abs(dist) < m_planeDistanceThreshold) {
                inliers.push_back(point);
            }
        }

        float confidence = static_cast<float>(inliers.size()) / points.size();
        
        if (confidence > bestResult.confidence) {
            bestResult.normal = normal;
            bestResult.center = points[idx1];
            bestResult.distance = distance;
            bestResult.confidence = confidence;
            bestResult.inliers = inliers;
        }
    }

    return bestResult;
}

std::vector<glm::vec3> Lidar3DProcessor::filterValidPoints(const std::vector<glm::vec3> &points)
{
    std::vector<glm::vec3> validPoints;
    validPoints.reserve(points.size());
    
    for (const auto& point : points) {
        if (!std::isnan(point.x) && !std::isnan(point.y) && !std::isnan(point.z)) {
            if (glm::length(point) >= m_minDepth && glm::length(point) <= m_maxDepth) {
                validPoints.push_back(point);
            }
        }
    }
    
    return validPoints;
}

std::vector<glm::vec3> Lidar3DProcessor::removeOutliers(const std::vector<glm::vec3> &points)
{
    // TODO: Реализовать удаление выбросов
    return points;
}

glm::vec3 Lidar3DProcessor::depthToWorldSpace(int x, int y, float depth, int width, int height)
{
    // iPhone LiDAR intrinsics (approximate for 256x192 and similar)
    // HFOV ≈ 57°, VFOV ≈ 43°
    float fx = static_cast<float>(width)  / (2.0f * std::tan(glm::radians(57.0f) / 2.0f));
    float fy = static_cast<float>(height) / (2.0f * std::tan(glm::radians(43.0f) / 2.0f));
    float cx = static_cast<float>(width)  / 2.0f;
    float cy = static_cast<float>(height) / 2.0f;

    float u = static_cast<float>(x) - cx;
    float v = static_cast<float>(y) - cy;

    // Camera space: z forward, x right, y down
    glm::vec3 point;
    point.x = (u * depth) / fx;
    point.y = -(v * depth) / fy;  // flip Y so +Y is up in world space
    point.z = depth;

    return point;
}

float Lidar3DProcessor::pointToPlaneDistance(const glm::vec3 &point, const glm::vec3 &planeNormal, const glm::vec3 &planePoint)
{
    return glm::dot(point - planePoint, planeNormal);
}

Mesh Lidar3DProcessor::generateEnvironmentMesh(float voxelSize, float maxEdgeLength)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    
    if (m_lastProcessedPoints.empty()) {
        std::cerr << "[Lidar3DProcessor] No points available for mesh generation" << std::endl;
        return Mesh();
    }
    
    std::cout << "[Lidar3DProcessor] Generating mesh from " << m_lastProcessedPoints.size() << " points" << std::endl;
    
    return m_meshGenerator->generateMesh(m_lastProcessedPoints, voxelSize, maxEdgeLength);
}

} // namespace LensEngine

