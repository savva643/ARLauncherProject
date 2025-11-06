#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

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
    
    // Создание меша куба
    std::vector<float> cubeVertices = {
        // Позиции (x,y,z) + Нормали (x,y,z) + UV (u,v)
        // Передняя грань
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        // Задняя грань
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        // Левая грань
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        // Правая грань
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        // Верхняя грань
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        // Нижняя грань
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    };
    
    std::vector<uint32_t> cubeIndices = {
        0, 1, 2,  2, 3, 0,  // Передняя
        4, 5, 6,  6, 7, 4,  // Задняя
        8, 9, 10, 10, 11, 8, // Левая
        12, 13, 14, 14, 15, 12, // Правая
        16, 17, 18, 18, 19, 16, // Верхняя
        20, 21, 22, 22, 23, 20  // Нижняя
    };
    
    m_cubeMeshId = renderer->createMesh(cubeVertices, cubeIndices);
    
    // Создание демо-кубов
    SceneObject cube1;
    cube1.position = glm::vec3(0.0f, 0.0f, -1.0f); // На 1 метр перед камерой
    cube1.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube1.scale = glm::vec3(0.2f); // 20 см куб
    cube1.visible = true;
    cube1.meshId = m_cubeMeshId;
    addObject(cube1);
    
    // Второй куб слева
    SceneObject cube2;
    cube2.position = glm::vec3(-0.5f, 0.0f, -1.0f);
    cube2.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube2.scale = glm::vec3(0.15f);
    cube2.visible = true;
    cube2.meshId = m_cubeMeshId;
    addObject(cube2);
    
    // Третий куб справа
    SceneObject cube3;
    cube3.position = glm::vec3(0.5f, 0.0f, -1.0f);
    cube3.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube3.scale = glm::vec3(0.15f);
    cube3.visible = true;
    cube3.meshId = m_cubeMeshId;
    addObject(cube3);
    
    // Создаем простую сферу (икоcaэдр для упрощения)
    std::vector<float> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    
    // Простая сфера через икосаэдр (12 вершин, 20 треугольников)
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // Золотое сечение
    const float radius = 0.5f;
    
    // Вершины икосаэдра
    std::vector<glm::vec3> icosahedronVertices = {
        glm::normalize(glm::vec3(-1, t, 0)) * radius,
        glm::normalize(glm::vec3(1, t, 0)) * radius,
        glm::normalize(glm::vec3(-1, -t, 0)) * radius,
        glm::normalize(glm::vec3(1, -t, 0)) * radius,
        glm::normalize(glm::vec3(0, -1, t)) * radius,
        glm::normalize(glm::vec3(0, 1, t)) * radius,
        glm::normalize(glm::vec3(0, -1, -t)) * radius,
        glm::normalize(glm::vec3(0, 1, -t)) * radius,
        glm::normalize(glm::vec3(t, 0, -1)) * radius,
        glm::normalize(glm::vec3(t, 0, 1)) * radius,
        glm::normalize(glm::vec3(-t, 0, -1)) * radius,
        glm::normalize(glm::vec3(-t, 0, 1)) * radius,
    };
    
    // Индексы для икосаэдра (упрощенная версия)
    for (size_t i = 0; i < icosahedronVertices.size(); i++) {
        const auto& v = icosahedronVertices[i];
        glm::vec3 normal = glm::normalize(v / radius);
        sphereVertices.push_back(v.x);
        sphereVertices.push_back(v.y);
        sphereVertices.push_back(v.z);
        sphereVertices.push_back(normal.x);
        sphereVertices.push_back(normal.y);
        sphereVertices.push_back(normal.z);
        sphereVertices.push_back(0.5f); // UV
        sphereVertices.push_back(0.5f);
    }
    
    // Простые индексы (для демо используем простую сферу)
    for (uint32_t i = 0; i < icosahedronVertices.size() - 2; i++) {
        sphereIndices.push_back(0);
        sphereIndices.push_back(i + 1);
        sphereIndices.push_back(i + 2);
    }
    
    m_sphereMeshId = renderer->createMesh(sphereVertices, sphereIndices);
    
    // Добавляем сферу в сцену
    SceneObject sphere;
    sphere.position = glm::vec3(0.0f, 0.3f, -1.5f); // Выше и дальше
    sphere.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    sphere.scale = glm::vec3(0.15f);
    sphere.visible = true;
    sphere.meshId = m_sphereMeshId;
    addObject(sphere);
    
    std::cout << "✅ Demo scene created: " << m_objects.size() << " objects" << std::endl;
}

void Scene::updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation)
{
    if (m_camera) {
        m_camera->updateFromAR(position, rotation);
    }
}

