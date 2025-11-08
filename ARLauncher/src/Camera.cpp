#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera()
    : m_position(0.0f)
    , m_rotation(1.0f, 0.0f, 0.0f, 0.0f)
    , m_initialPosition(0.0f)
    , m_fov(60.0f)
    , m_aspect(16.0f / 9.0f)
    , m_nearPlane(0.01f)
    , m_farPlane(100.0f)
    , m_calibrated(false)
{
}

Camera::~Camera()
{
}

void Camera::setPosition(const glm::vec3& position)
{
    m_position = position;
}

void Camera::setRotation(const glm::quat& rotation)
{
    m_rotation = rotation;
}

void Camera::updateFromAR(const glm::vec3& position, const glm::quat& rotation)
{
    if (!m_calibrated) {
        m_initialPosition = position;
        m_calibrated = true;
    }
    
    m_position = position - m_initialPosition;
    m_rotation = rotation;
}

glm::vec3 Camera::getForward() const
{
    return m_rotation * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Camera::getUp() const
{
    return m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 Camera::getRight() const
{
    return m_rotation * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::mat4 Camera::getViewMatrix() const
{
    // Правильная матрица вида: сначала поворот, потом перемещение
    // В AR камера движется в пространстве, поэтому нужно инвертировать позицию
    glm::mat4 rotation = glm::mat4_cast(glm::conjugate(m_rotation));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_position);
    return rotation * translation;
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(m_fov), m_aspect, m_nearPlane, m_farPlane);
}

void Camera::setProjection(float fov, float aspect, float nearPlane, float farPlane)
{
    m_fov = fov;
    m_aspect = aspect;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

void Camera::reset()
{
    m_position = glm::vec3(0.0f);
    m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_initialPosition = glm::vec3(0.0f);
    m_calibrated = false;
}

void Camera::calibrate()
{
    m_initialPosition = m_position;
    m_calibrated = true;
}

