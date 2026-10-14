#ifndef BUNDLE_ADJUSTMENT_H
#define BUNDLE_ADJUSTMENT_H

#include "LensEngineTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <deque>
#include <memory>

namespace LensEngine {

/**
 * @brief Bundle Adjustment - оптимизирует траекторию камеры и 3D точки
 * 
 * Использует Ceres Solver для минимизации ошибки репроекции
 * Улучшает точность позиции на 2-3x
 */
class BundleAdjustment {
public:
    struct KeyFrame {
        int frameId;
        glm::vec3 position;
        glm::quat rotation;
        
        std::vector<glm::vec3> points3D;      // 3D точки в world frame
        std::vector<glm::vec2> points2D;      // 2D проекции в image frame
        std::vector<int> pointIds;            // ID точек для отслеживания
        
        // Camera intrinsics
        float fx, fy, cx, cy;
    };
    
    BundleAdjustment();
    ~BundleAdjustment();
    
    // Добавить ключевой кадр
    void addKeyFrame(const KeyFrame& kf);
    
    // Оптимизировать последние N кадров
    void optimize();
    
    // Получить оптимизированные позы
    std::vector<glm::vec3> getOptimizedPositions() const;
    std::vector<glm::quat> getOptimizedRotations() const;
    
    // Получить оптимизированные 3D точки
    std::vector<glm::vec3> getOptimizedPoints() const;
    
    // Сброс
    void reset();
    
    // Получить количество кадров
    size_t getKeyFrameCount() const { return m_keyframes.size(); }

private:
    std::deque<KeyFrame> m_keyframes;
    
    // Максимальное количество кадров для оптимизации
    static constexpr size_t MAX_KEYFRAMES = 20;
    
    // Минимальное количество кадров перед оптимизацией
    static constexpr size_t MIN_KEYFRAMES_FOR_BA = 5;
};

} // namespace LensEngine

#endif // BUNDLE_ADJUSTMENT_H
