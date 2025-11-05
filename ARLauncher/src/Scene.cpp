#include "Scene.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Scene::Scene()
    : m_nextObjectId(1)
    , m_cubeMeshId(0)
    , m_sphereMeshId(0)
{
    m_camera = std::make_unique<Camera>();
}

Scene::~Scene()
{
    shutdown();
}

bool Scene::initialize()
{
    m_camera->setProjection(60.0f, 16.0f / 9.0f, 0.01f, 100.0f);
    return true;
}

void Scene::shutdown()
{
    m_objects.clear();
}

uint32_t Scene::addObject(const SceneObject& object)
{
    uint32_t id = m_nextObjectId++;
    SceneObject obj = object;
    obj.id = id;
    m_objects.push_back(obj);
    return id;
}

void Scene::removeObject(uint32_t objectId)
{
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [objectId](const SceneObject& obj) { return obj.id == objectId; }),
        m_objects.end()
    );
}

SceneObject* Scene::getObject(uint32_t objectId)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [objectId](const SceneObject& obj) { return obj.id == objectId; });
    
    if (it != m_objects.end()) {
        return &(*it);
    }
    return nullptr;
}

void Scene::update(float deltaTime)
{
    // Обновление анимаций объектов
}

std::vector<glm::mat4> Scene::getObjectTransforms() const
{
    std::vector<glm::mat4> transforms;
    transforms.reserve(m_objects.size());
    
    for (const auto& obj : m_objects) {
        if (!obj.visible) {
            continue;
        }
        
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), obj.scale);
        glm::mat4 rotation = glm::mat4_cast(obj.rotation);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), obj.position);
        
        transforms.push_back(translation * rotation * scale);
    }
    
    return transforms;
}

std::vector<uint32_t> Scene::getVisibleMeshIds() const
{
    std::vector<uint32_t> meshIds;
    meshIds.reserve(m_objects.size());
    
    for (const auto& obj : m_objects) {
        if (obj.visible) {
            meshIds.push_back(obj.meshId);
        }
    }
    
    return meshIds;
}

void Scene::createDemoScene()
{
    // Создание демо-куба на "полу"
    SceneObject cube;
    cube.position = glm::vec3(0.0f, 0.0f, -1.0f); // На 1 метр перед камерой
    cube.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube.scale = glm::vec3(0.2f); // 20 см куб
    cube.visible = true;
    cube.meshId = 1; // Предполагаем, что меш куба имеет ID 1
    
    uint32_t cubeId = addObject(cube);
    
    // Создаем второй куб слева
    SceneObject cube2;
    cube2.position = glm::vec3(-0.5f, 0.0f, -1.0f);
    cube2.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube2.scale = glm::vec3(0.15f);
    cube2.visible = true;
    cube2.meshId = 1;
    
    addObject(cube2);
    
    // Создаем третий куб справа
    SceneObject cube3;
    cube3.position = glm::vec3(0.5f, 0.0f, -1.0f);
    cube3.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube3.scale = glm::vec3(0.15f);
    cube3.visible = true;
    cube3.meshId = 1;
    
    addObject(cube3);
    
    // Сохраняем ID меша куба для дальнейшего использования
    m_cubeMeshId = 1;
}

void Scene::updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation)
{
    if (m_camera) {
        m_camera->updateFromAR(position, rotation);
    }
}

