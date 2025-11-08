#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
const std::vector<float> kCubeVertices = {
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
};

const std::vector<uint32_t> kCubeIndices = {
    0, 1, 2,  2, 3, 0,
    4, 5, 6,  6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20
};
}

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
    static float time = 0.0f;
    time += deltaTime;
    
    // Анимация вращения кубов
    for (auto& obj : m_objects) {
        if (obj.meshId == m_cubeMeshId) {
            // Вращаем кубы вокруг оси Y
            float rotationSpeed = 1.0f; // радиан в секунду
            glm::quat rotationY = glm::angleAxis(time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
            obj.rotation = rotationY;
        } else if (obj.meshId == m_sphereMeshId) {
            // Вращаем сферу вокруг оси X и Y
            float rotationSpeedX = 0.5f;
            float rotationSpeedY = 0.8f;
            glm::quat rotationX = glm::angleAxis(time * rotationSpeedX, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::quat rotationY = glm::angleAxis(time * rotationSpeedY, glm::vec3(0.0f, 1.0f, 0.0f));
            obj.rotation = rotationY * rotationX;
        }
    }
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

void Scene::createDemoScene(Renderer* renderer)
{
    if (!renderer) {
        std::cerr << "Scene::createDemoScene: renderer is null" << std::endl;
        return;
    }
    
    if (m_cubeMeshId == 0) {
        m_cubeMeshId = renderer->createMesh(kCubeVertices, kCubeIndices);
    }
    
    // Создание одного куба (квадрат)
    SceneObject cube1;
    cube1.position = glm::vec3(0.0f, 0.0f, -1.0f); // На 1 метр перед камерой
    cube1.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube1.scale = glm::vec3(0.2f); // 20 см куб
    cube1.visible = true;
    cube1.meshId = m_cubeMeshId;
    addObject(cube1);
    
    // Создаем правильную сферу через параметрическое уравнение
    std::vector<float> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    
    const float radius = 0.5f;
    const int segments = 32; // Количество сегментов для гладкой сферы
    const int rings = 16;    // Количество колец
    
    // Генерируем вершины сферы
    for (int ring = 0; ring <= rings; ++ring) {
        float theta = static_cast<float>(ring) * 3.14159f / static_cast<float>(rings);
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int seg = 0; seg <= segments; ++seg) {
            float phi = static_cast<float>(seg) * 2.0f * 3.14159f / static_cast<float>(segments);
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            glm::vec3 position(
                radius * sinTheta * cosPhi,
                radius * cosTheta,
                radius * sinTheta * sinPhi
            );
            
            glm::vec3 normal = glm::normalize(position);
            
            sphereVertices.push_back(position.x);
            sphereVertices.push_back(position.y);
            sphereVertices.push_back(position.z);
            sphereVertices.push_back(normal.x);
            sphereVertices.push_back(normal.y);
            sphereVertices.push_back(normal.z);
            sphereVertices.push_back(static_cast<float>(seg) / static_cast<float>(segments)); // U
            sphereVertices.push_back(static_cast<float>(ring) / static_cast<float>(rings));    // V
        }
    }
    
    // Генерируем индексы для треугольников
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            int first = ring * (segments + 1) + seg;
            int second = first + segments + 1;
            
            // Первый треугольник
            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);
            
            // Второй треугольник
            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }
    
    m_sphereMeshId = renderer->createMesh(sphereVertices, sphereIndices);
    
    // Добавляем две сферы (круга) в сцену
    SceneObject sphere1;
    sphere1.position = glm::vec3(-0.4f, 0.0f, -1.0f); // Слева
    sphere1.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    sphere1.scale = glm::vec3(0.15f);
    sphere1.visible = true;
    sphere1.meshId = m_sphereMeshId;
    addObject(sphere1);
    
    SceneObject sphere2;
    sphere2.position = glm::vec3(0.4f, 0.0f, -1.0f); // Справа
    sphere2.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    sphere2.scale = glm::vec3(0.15f);
    sphere2.visible = true;
    sphere2.meshId = m_sphereMeshId;
    addObject(sphere2);
    
    std::cout << "[OK] Demo scene created: " << m_objects.size() << " objects" << std::endl;
}

uint32_t Scene::addCube(Renderer* renderer, const glm::vec3& position, const glm::vec3& scale)
{
    if (!renderer) {
        std::cerr << "Scene::addCube: renderer is null" << std::endl;
        return 0;
    }

    if (m_cubeMeshId == 0) {
        m_cubeMeshId = renderer->createMesh(kCubeVertices, kCubeIndices);
    }

    SceneObject cube;
    cube.position = position;
    cube.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube.scale = scale;
    cube.visible = true;
    cube.meshId = m_cubeMeshId;
    return addObject(cube);
}

void Scene::updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation)
{
    if (m_camera) {
        m_camera->updateFromAR(position, rotation);
    }
}

