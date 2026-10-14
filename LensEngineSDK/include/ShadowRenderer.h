#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace LensEngine {

struct Shadow {
    glm::mat4 shadowMatrix;
    glm::vec3 lightPosition;
    glm::vec3 lightDirection;
    float intensity;
};

class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();
    
    // Генерация shadow matrix для объекта на плоскости
    glm::mat4 generateShadowMatrix(const glm::vec3& lightPosition,
                                  const glm::vec3& planeNormal,
                                  const glm::vec3& planePoint);
    
    // Генерация теней для нескольких плоскостей
    std::vector<Shadow> generateShadows(const glm::vec3& lightPosition,
                                       const std::vector<glm::vec3>& planeNormals,
                                       const std::vector<glm::vec3>& planePoints);
    
    // Обновление позиции источника света
    void setLightPosition(const glm::vec3& position);
    void setLightDirection(const glm::vec3& direction);
    
    // Настройка интенсивности теней
    void setShadowIntensity(float intensity);
    
    // Получение текущих теней
    std::vector<Shadow> getCurrentShadows() const;
    
private:
    glm::vec3 m_lightPosition;
    glm::vec3 m_lightDirection;
    float m_shadowIntensity;
    std::vector<Shadow> m_currentShadows;
    
    // Вычисление shadow matrix для одной плоскости
    glm::mat4 computeShadowMatrix(const glm::vec3& lightPos,
                                  const glm::vec3& planeNormal,
                                  const glm::vec3& planePoint);
};

} // namespace LensEngine
