#include "CameraController.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

namespace LensEngine {

CameraController::CameraController()
    : m_position(0.0f)
    , m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
    , m_initialPosition(0.0f)
    , m_worldScale(1.0f)
    , m_initialCalibration(false)
{
}

CameraController::~CameraController()
{
}

glm::vec3 CameraController::forward() const
{
    return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 CameraController::up() const
{
    return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 CameraController::right() const
{
    return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);
}

void CameraController::updateFromSensorFusion(const glm::vec3 &position, const glm::quat &rotation)
{
    if (!m_initialCalibration) {
        m_initialPosition = position;
        m_initialCalibration = true;
    }

    glm::vec3 worldPosition = position - m_initialPosition;
    m_position = worldPosition * m_worldScale;
    m_rotation = glm::normalize(rotation);

    if (m_cameraUpdatedCallback) {
        m_cameraUpdatedCallback(m_position, m_rotation);
    }
}

void CameraController::updateFromCameraPose(const CameraPose &cameraPose)
{
    m_position = cameraPose.position;
    m_rotation = cameraPose.rotation;

    if (m_cameraUpdatedCallback) {
        m_cameraUpdatedCallback(m_position, m_rotation);
    }
}

glm::mat4 CameraController::viewMatrix() const
{
    glm::mat4 rotation = glm::mat4_cast(glm::conjugate(m_rotation));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
    return rotation * translation;
}

glm::mat4 CameraController::projectionMatrix(float fov, float aspect, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void CameraController::resetCamera()
{
    m_position = glm::vec3(0.0f);
    m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_initialPosition = glm::vec3(0.0f);
    m_initialCalibration = false;
}

void CameraController::recalibrate()
{
    m_initialPosition = m_position;
    m_initialCalibration = true;
}

void CameraController::setWorldScale(float scale)
{
    m_worldScale = scale;
}

void CameraController::setCameraUpdatedCallback(CameraUpdatedCallback callback)
{
    m_cameraUpdatedCallback = callback;
}

} // namespace LensEngine

