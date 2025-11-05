#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "LensEngineTypes.h"
#include <functional>

namespace LensEngine {

/**
 * @brief Контроллер камеры AR
 * 
 * Управляет позицией и ориентацией виртуальной камеры,
 * синхронизированной с реальной камерой iPhone.
 */
class CameraController {
public:
    CameraController();
    ~CameraController();

    // Основные свойства
    glm::vec3 position() const { return m_position; }
    glm::quat rotation() const { return m_rotation; }
    glm::vec3 forward() const;
    glm::vec3 up() const;
    glm::vec3 right() const;

    // Обновление данных
    void updateFromSensorFusion(const glm::vec3 &position, const glm::quat &rotation);
    void updateFromCameraPose(const CameraPose &cameraPose);

    // Матрицы
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float fov = 60.0f, float aspect = 16.0f/9.0f,
                               float nearPlane = 0.01f, float farPlane = 100.0f) const;

    // Управление
    void resetCamera();
    void recalibrate();
    void setWorldScale(float scale);

    // Колбэки
    using CameraUpdatedCallback = std::function<void(const glm::vec3&, const glm::quat&)>;
    void setCameraUpdatedCallback(CameraUpdatedCallback callback);

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_initialPosition;
    float m_worldScale;
    bool m_initialCalibration;
    
    CameraUpdatedCallback m_cameraUpdatedCallback;
};

} // namespace LensEngine

#endif // CAMERACONTROLLER_H

