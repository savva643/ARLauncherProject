#include "TemporalFilter.h"
#include <glm/gtc/quaternion.hpp>

namespace LensEngine {

TemporalFilter::TemporalFilter()
    : m_lastPos(0.0f),
      m_lastRot(1.0f, 0.0f, 0.0f, 0.0f),
      m_lastVel(0.0f)
{
}

glm::vec3 TemporalFilter::filterPosition(const glm::vec3& newPos)
{
    // Экспоненциальное сглаживание
    // pos_filtered = α * pos_new + (1-α) * pos_last
    m_lastPos = ALPHA_POS * newPos + (1.0f - ALPHA_POS) * m_lastPos;
    return m_lastPos;
}

glm::quat TemporalFilter::filterRotation(const glm::quat& newRot)
{
    // Сферическая линейная интерполяция (SLERP)
    // Более правильно для кватернионов чем линейная интерполяция
    m_lastRot = glm::slerp(m_lastRot, newRot, ALPHA_ROT);
    return m_lastRot;
}

glm::vec3 TemporalFilter::filterVelocity(const glm::vec3& newVel)
{
    // Экспоненциальное сглаживание для скорости
    m_lastVel = ALPHA_VEL * newVel + (1.0f - ALPHA_VEL) * m_lastVel;
    return m_lastVel;
}

void TemporalFilter::reset()
{
    m_lastPos = glm::vec3(0.0f);
    m_lastRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_lastVel = glm::vec3(0.0f);
}

} // namespace LensEngine
