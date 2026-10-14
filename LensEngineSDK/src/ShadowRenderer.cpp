#include "ShadowRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace LensEngine {

ShadowRenderer::ShadowRenderer()
    : m_lightPosition(1.0f, 10.0f, 1.0f)
    , m_lightDirection(glm::normalize(glm::vec3(-1.0f, -10.0f, -1.0f)))
    , m_shadowIntensity(0.5f)
{
}

ShadowRenderer::~ShadowRenderer() {
}

glm::mat4 ShadowRenderer::computeShadowMatrix(const glm::vec3& lightPos,
                                              const glm::vec3& planeNormal,
                                              const glm::vec3& planePoint)
{
    // Shadow matrix formula: M = P - (L dot N) / (L dot N) * (L dot N)
    // Where P is the plane, L is the light position, N is the plane normal
    
    float dot = glm::dot(lightPos, planeNormal) - glm::dot(planePoint, planeNormal);
    
    if (std::abs(dot) < 0.0001f) {
        // Light is parallel to plane, no shadow
        return glm::mat4(1.0f);
    }
    
    glm::mat4 shadowMat(1.0f);
    
    // Row 0
    shadowMat[0][0] = dot - lightPos.x * planeNormal.x;
    shadowMat[1][0] = -lightPos.x * planeNormal.y;
    shadowMat[2][0] = -lightPos.x * planeNormal.z;
    shadowMat[3][0] = -lightPos.x * (-glm::dot(planePoint, planeNormal));
    
    // Row 1
    shadowMat[0][1] = -lightPos.y * planeNormal.x;
    shadowMat[1][1] = dot - lightPos.y * planeNormal.y;
    shadowMat[2][1] = -lightPos.y * planeNormal.z;
    shadowMat[3][1] = -lightPos.y * (-glm::dot(planePoint, planeNormal));
    
    // Row 2
    shadowMat[0][2] = -lightPos.z * planeNormal.x;
    shadowMat[1][2] = -lightPos.z * planeNormal.y;
    shadowMat[2][2] = dot - lightPos.z * planeNormal.z;
    shadowMat[3][2] = -lightPos.z * (-glm::dot(planePoint, planeNormal));
    
    // Row 3
    shadowMat[0][3] = -planeNormal.x;
    shadowMat[1][3] = -planeNormal.y;
    shadowMat[2][3] = -planeNormal.z;
    shadowMat[3][3] = dot - glm::dot(planePoint, planeNormal);
    
    return shadowMat / dot;
}

glm::mat4 ShadowRenderer::generateShadowMatrix(const glm::vec3& lightPosition,
                                               const glm::vec3& planeNormal,
                                               const glm::vec3& planePoint)
{
    return computeShadowMatrix(lightPosition, planeNormal, planePoint);
}

std::vector<Shadow> ShadowRenderer::generateShadows(const glm::vec3& lightPosition,
                                                   const std::vector<glm::vec3>& planeNormals,
                                                   const std::vector<glm::vec3>& planePoints)
{
    std::vector<Shadow> shadows;
    
    if (planeNormals.size() != planePoints.size()) {
        std::cerr << "[ShadowRenderer] Mismatch between plane normals and points" << std::endl;
        return shadows;
    }
    
    for (size_t i = 0; i < planeNormals.size(); ++i) {
        Shadow shadow;
        shadow.shadowMatrix = computeShadowMatrix(lightPosition, planeNormals[i], planePoints[i]);
        shadow.lightPosition = lightPosition;
        shadow.lightDirection = m_lightDirection;
        shadow.intensity = m_shadowIntensity;
        shadows.push_back(shadow);
    }
    
    m_currentShadows = shadows;
    std::cout << "[ShadowRenderer] Generated " << shadows.size() << " shadows" << std::endl;
    
    return shadows;
}

void ShadowRenderer::setLightPosition(const glm::vec3& position) {
    m_lightPosition = position;
}

void ShadowRenderer::setLightDirection(const glm::vec3& direction) {
    m_lightDirection = glm::normalize(direction);
}

void ShadowRenderer::setShadowIntensity(float intensity) {
    m_shadowIntensity = glm::clamp(intensity, 0.0f, 1.0f);
}

std::vector<Shadow> ShadowRenderer::getCurrentShadows() const {
    return m_currentShadows;
}

} // namespace LensEngine
