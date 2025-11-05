#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <string>

class Camera;
class Renderer;

/**
 * @brief 3D объект в сцене
 */
struct SceneObject {
    uint32_t id;        // ID объекта в сцене
    uint32_t meshId;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    uint32_t textureId;
    bool visible;
    
    SceneObject() : id(0), meshId(0), position(0.0f), rotation(1.0f, 0, 0, 0), 
                    scale(1.0f), textureId(0), visible(true) {}
};

/**
 * @brief 3D сцена с объектами
 */
class Scene {
public:
    Scene();
    ~Scene();

    bool initialize();
    void shutdown();
    
    // Объекты
    uint32_t addObject(const SceneObject& object);
    void removeObject(uint32_t objectId);
    SceneObject* getObject(uint32_t objectId);
    
    // Камера
    Camera* getCamera() const { return m_camera.get(); }
    
    // Обновление
    void update(float deltaTime);
    
    // Получение данных для рендеринга
    std::vector<glm::mat4> getObjectTransforms() const;
    std::vector<uint32_t> getVisibleMeshIds() const;
    
    // Создание демо-объектов
    void createDemoScene(Renderer* renderer);
    
    // Синхронизация с реальной камерой
    void updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation);

private:
    std::unique_ptr<Camera> m_camera;
    std::vector<SceneObject> m_objects;
    uint32_t m_nextObjectId;
    
    // Демо-объекты
    uint32_t m_cubeMeshId;
    uint32_t m_sphereMeshId;
};

#endif // SCENE_H

