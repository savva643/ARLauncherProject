#include "BundleAdjustment.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <iomanip>

namespace LensEngine {

BundleAdjustment::BundleAdjustment()
{
}

BundleAdjustment::~BundleAdjustment()
{
}

void BundleAdjustment::addKeyFrame(const KeyFrame& kf)
{
    m_keyframes.push_back(kf);
    
    // Сохраняем только последние MAX_KEYFRAMES кадров
    if (m_keyframes.size() > MAX_KEYFRAMES) {
        m_keyframes.pop_front();
    }
}

void BundleAdjustment::optimize()
{
    if (m_keyframes.size() < MIN_KEYFRAMES_FOR_BA) {
        return;
    }
    
    // === ПРОСТАЯ ОПТИМИЗАЦИЯ (без Ceres) ===
    // Усредняем позы соседних кадров для сглаживания
    
    // Оптимизируем только внутренние кадры (не первый и не последний)
    for (size_t i = 1; i < m_keyframes.size() - 1; ++i) {
        // Берем позы соседних кадров
        const glm::vec3& prevPos = m_keyframes[i-1].position;
        const glm::vec3& nextPos = m_keyframes[i+1].position;
        
        const glm::quat& prevRot = m_keyframes[i-1].rotation;
        const glm::quat& nextRot = m_keyframes[i+1].rotation;
        
        // Усредняем позиции
        glm::vec3 smoothedPos = (prevPos + m_keyframes[i].position + nextPos) / 3.0f;
        
        // Усредняем ротации (SLERP)
        glm::quat smoothedRot = glm::normalize(
            glm::slerp(glm::slerp(prevRot, m_keyframes[i].rotation, 0.5f),
                      nextRot, 0.5f)
        );
        
        // Обновляем кадр
        m_keyframes[i].position = smoothedPos;
        m_keyframes[i].rotation = smoothedRot;
    }
    
    // === ОПТИМИЗАЦИЯ 3D ТОЧЕК ===
    // Переносим 3D точки в соответствии с новыми позами
    
    // Для каждого кадра пересчитываем 3D точки
    for (auto& kf : m_keyframes) {
        // Преобразуем 3D точки в camera frame
        glm::mat3 rotMat = glm::mat3_cast(kf.rotation);
        
        for (auto& pt3d : kf.points3D) {
            // Преобразуем в camera frame
            glm::vec3 camPt = rotMat * (pt3d - kf.position);
            
            // Проверяем что точка перед камерой
            if (camPt.z < 0.1f) {
                pt3d = glm::vec3(0.0f);  // Инвалидная точка
            }
        }
    }
}

std::vector<glm::vec3> BundleAdjustment::getOptimizedPositions() const
{
    std::vector<glm::vec3> positions;
    for (const auto& kf : m_keyframes) {
        positions.push_back(kf.position);
    }
    return positions;
}

std::vector<glm::quat> BundleAdjustment::getOptimizedRotations() const
{
    std::vector<glm::quat> rotations;
    for (const auto& kf : m_keyframes) {
        rotations.push_back(kf.rotation);
    }
    return rotations;
}

std::vector<glm::vec3> BundleAdjustment::getOptimizedPoints() const
{
    std::vector<glm::vec3> allPoints;
    for (const auto& kf : m_keyframes) {
        allPoints.insert(allPoints.end(), kf.points3D.begin(), kf.points3D.end());
    }
    return allPoints;
}

void BundleAdjustment::reset()
{
    m_keyframes.clear();
}

} // namespace LensEngine
