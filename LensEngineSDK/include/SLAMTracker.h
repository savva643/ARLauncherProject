#ifndef SLAM_TRACKER_H
#define SLAM_TRACKER_H

#include "LensEngineTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>

namespace LensEngine {

/**
 * @brief 6DOF SLAM Tracker - использует готовые библиотеки (OpenCV SolvePnP)
 * 
 * Отслеживает 6 степеней свободы (3 позиция + 3 ротация) используя:
 * - Feature detection (ORB, AKAZE)
 * - Feature matching
 * - PnP (Perspective-n-Point) решение
 * - Bundle adjustment
 */
class SLAMTracker {
public:
    SLAMTracker();
    ~SLAMTracker();

    // Инициализация
    bool initialize(double fx, double fy, double cx, double cy);
    
    // Обработка кадра и получение 6DOF позы
    CameraPose trackFrame(const RGBImage &currentFrame);
    
    // Получение текущей позы
    CameraPose getCurrentPose() const { return m_currentPose; }
    
    // Получение статистики
    int getTrackedPointsCount() const { return m_trackedPoints3D.size(); }
    float getTrackingQuality() const { return m_trackingQuality; }
    
    // Сброс трекера
    void reset();

private:
    // Внутренние методы
    void detectFeatures(const RGBImage &frame);
    void matchFeatures();
    bool solvePnP(const RGBImage &frame);
    void updateMapPoints();
    
    // Параметры камеры
    double m_fx, m_fy, m_cx, m_cy;
    
    // Состояние
    CameraPose m_currentPose;
    float m_trackingQuality = 0.0f;
    
    // Карта 3D точек
    std::vector<glm::vec3> m_mapPoints3D;
    std::vector<glm::vec2> m_mapPoints2D;
    std::vector<uint64_t> m_mapPointIds;
    
    // Отслеживаемые точки
    std::vector<glm::vec3> m_trackedPoints3D;
    std::vector<glm::vec2> m_trackedPoints2D;
    
    // Предыдущий кадр
    RGBImage m_previousFrame;
    std::vector<glm::vec2> m_previousKeypoints;
    
    // Параметры
    int m_minTrackedPoints = 10;
    float m_reprojectionThreshold = 8.0f;
};

} // namespace LensEngine

#endif // SLAM_TRACKER_H
