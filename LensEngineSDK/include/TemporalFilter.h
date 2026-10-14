#ifndef TEMPORAL_FILTER_H
#define TEMPORAL_FILTER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <deque>

namespace LensEngine {

/**
 * @brief Temporal filtering для сглаживания позиции, ротации и скорости
 * 
 * Использует экспоненциальное сглаживание для убирания шума
 * и скачков в VO данных
 */
class TemporalFilter {
public:
    TemporalFilter();
    
    // Сглаживание позиции
    glm::vec3 filterPosition(const glm::vec3& newPos);
    
    // Сглаживание ротации (SLERP)
    glm::quat filterRotation(const glm::quat& newRot);
    
    // Сглаживание скорости
    glm::vec3 filterVelocity(const glm::vec3& newVel);
    
    // Сброс фильтра
    void reset();
    
    // Получить последние отфильтрованные значения
    glm::vec3 getLastPosition() const { return m_lastPos; }
    glm::quat getLastRotation() const { return m_lastRot; }
    glm::vec3 getLastVelocity() const { return m_lastVel; }

private:
    // Последние значения
    glm::vec3 m_lastPos;
    glm::quat m_lastRot;
    glm::vec3 m_lastVel;
    
    // Коэффициенты сглаживания (0-1)
    // Больше = больше доверия новым данным
    // Меньше = больше доверия истории
    const float ALPHA_POS = 0.4f;   // 40% новое, 60% история
    const float ALPHA_ROT = 0.3f;   // 30% новое, 70% история
    const float ALPHA_VEL = 0.6f;   // 60% новое, 40% история
};

} // namespace LensEngine

#endif // TEMPORAL_FILTER_H
