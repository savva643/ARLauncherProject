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
    float boundingRadius; // Радиус ограничивающей сферы для frustum culling
    glm::vec4 color;      // sRGB + альфа
    float metallic;       // 0 — пластик/краска, 1 — металл (отражает комнату)
    float roughness;      // 0 — зеркало, 1 — матовый
    bool onFloor;         // стоит на реальном полу (высота — от LiDAR), до того как пол найден — скрыт
    bool onTable;         // ставится на найденный стол (плоскость LiDAR), до того — скрыт
    bool grabbable;       // можно взять щипком
    int anchorId;         // якорь карты мира (позиция сохраняется между запусками), 0 — нет

    SceneObject() : id(0), meshId(0), position(0.0f), rotation(1.0f, 0, 0, 0),
                    scale(1.0f), textureId(0), visible(true), boundingRadius(0.5f),
                    color(0.2f, 0.7f, 1.0f, 1.0f), metallic(0.0f), roughness(0.5f), onFloor(false),
                    onTable(false), grabbable(true), anchorId(0) {}
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
    
    // Создание объектов
    uint32_t addObject(const SceneObject& object);
    void removeObject(uint32_t objectId);
    SceneObject* getObject(uint32_t objectId);
    uint32_t addCube(Renderer* renderer, const glm::vec3& position, const glm::vec3& scale);
    
    // Камера
    Camera* getCamera() const { return m_camera.get(); }
    
    // Обновление
    void update(float deltaTime);
    
    // Получение данных для рендеринга
    std::vector<glm::mat4> getObjectTransforms() const;
    std::vector<uint32_t> getVisibleMeshIds() const;
    std::vector<const SceneObject*> getVisibleObjects() const;
    
    // Создание демо-объектов
    void createDemoScene(Renderer* renderer);

    /// Высота реального пола (мир рендера) — объекты onFloor ставятся на него
    void setFloorHeight(float y);

    uint32_t cubeMeshId() const { return m_cubeMeshId; }
    uint32_t sphereMeshId() const { return m_sphereMeshId; }
    uint32_t torusMeshId() const { return m_torusMeshId; }
    const std::vector<SceneObject>& objects() const { return m_objects; }
    SceneObject* objectAt(size_t index) { return index < m_objects.size() ? &m_objects[index] : nullptr; }
    
    // Синхронизация с реальной камерой
    void updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation);
    
    // Frustum culling для occlusion
    void updateFrustumCulling();

private:
    std::unique_ptr<Camera> m_camera;
    std::vector<SceneObject> m_objects;
    uint32_t m_nextObjectId;
    
    // Демо-объекты
    uint32_t m_cubeMeshId;
    uint32_t m_sphereMeshId;
    uint32_t m_torusMeshId;
};

#endif // SCENE_H
