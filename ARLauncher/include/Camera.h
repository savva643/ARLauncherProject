#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Виртуальная камера для AR сцены
 * 
 * Синхронизируется с реальной камерой iPhone через LensEngineSDK
 */
class Camera {
public:
    Camera();
    ~Camera();

    // Позиция и ориентация
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void updateFromAR(const glm::vec3& position, const glm::quat& rotation);
    
    glm::vec3 getPosition() const { return m_position; }
    glm::quat getRotation() const { return m_rotation; }
    
    // Направления
    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;
    
    // Матрицы
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    // Параметры проекции
    void setProjection(float fov, float aspect, float nearPlane, float farPlane);
    void setFOV(float fov) { m_fov = fov; }
    void setAspect(float aspect) { m_aspect = aspect; }
    
    // Управление
    void reset();
    void calibrate();

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_initialPosition;
    
    float m_fov;
    float m_aspect;
    float m_nearPlane;
    float m_farPlane;
    
    bool m_calibrated;
};

#endif // CAMERA_H

