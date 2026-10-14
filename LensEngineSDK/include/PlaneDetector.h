#ifndef PLANEDETECTOR_H
#define PLANEDETECTOR_H

#include "LensEngineTypes.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace LensEngine {

/**
 * @brief Обнаружение плоскостей из LiDAR точек
 * 
 * Использует PCL RANSAC для обнаружения плоскостей (пол, стены)
 */
class PlaneDetector {
public:
    struct DetectedPlane {
        glm::vec3 normal;          // Нормаль плоскости
        float distance;            // Расстояние от начала координат
        std::vector<glm::vec3> points; // Точки плоскости
        float confidence;          // Уверенность (0-1)
        bool isFloor;              // Это пол?
        bool isWall;               // Это стена?
        
        DetectedPlane() : distance(0.0f), confidence(0.0f), isFloor(false), isWall(false) {}
    };

    PlaneDetector();
    ~PlaneDetector();

    /**
     * @brief Обнаруживает плоскости из облака точек
     * @param points3D Входное облако точек
     * @param maxPlanes Максимальное количество плоскостей
     * @return Вектор обнаруженных плоскостей
     */
    std::vector<DetectedPlane> detectPlanes(const std::vector<glm::vec3>& points3D, int maxPlanes = 5);

    /**
     * @brief Возвращает главную плоскость (обычно пол)
     */
    DetectedPlane getMainPlane() const { return m_mainPlane; }

    /**
     * @brief Возвращает true если обнаружен пол
     */
    bool hasFloor() const { return m_hasFloor; }

    /**
     * @brief Возвращает высоту пола
     */
    float getFloorHeight() const { return m_floorHeight; }

    /**
     * @brief Устанавливает порог RANSAC (меньше = точнее, но медленнее)
     */
    void setRANSACThreshold(float threshold) { m_ransacThreshold = threshold; }

    /**
     * @brief Устанавливает минимальное количество точек для плоскости
     */
    void setMinPoints(int minPoints) { m_minPoints = minPoints; }

private:
    float m_ransacThreshold = 0.02f;  // 2 см
    int m_minPoints = 100;
    
    DetectedPlane m_mainPlane;
    bool m_hasFloor = false;
    float m_floorHeight = 0.0f;

    /**
     * @brief Проверяет является ли плоскость полом (нормаль направлена вверх)
     */
    bool isFloorPlane(const glm::vec3& normal);

    /**
     * @brief Проверяет является ли плоскость стеной (нормаль горизонтальна)
     */
    bool isWallPlane(const glm::vec3& normal);
};

} // namespace LensEngine

#endif // PLANEDETECTOR_H
