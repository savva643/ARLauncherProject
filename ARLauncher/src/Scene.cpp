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
    , m_torusMeshId(0)
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
        } else if (obj.meshId == m_torusMeshId) {
            // Вращаем тор вокруг оси Z
            float rotationSpeedZ = 1.2f;
            glm::quat rotationZ = glm::angleAxis(time * rotationSpeedZ, glm::vec3(0.0f, 0.0f, 1.0f));
            obj.rotation = rotationZ;
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

void Scene::setFloorHeight(float y)
{
    for (auto& obj : m_objects) {
        if (!obj.onFloor || obj.anchorId != 0) continue;
        obj.position.y = y + 0.5f * obj.scale.y;   // меши единичные: от -0.5 до 0.5
        obj.visible = true;
    }
}

std::vector<const SceneObject*> Scene::getVisibleObjects() const
{
    std::vector<const SceneObject*> out;
    out.reserve(m_objects.size());
    for (const auto& obj : m_objects) {
        if (obj.visible) out.push_back(&obj);
    }
    return out;
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
    
    // 🔹 ТЕСТОВЫЕ КУБИКИ ДЛЯ ПРОВЕРКИ ПОЗИЦИИ ПО ВСЕМ ОСЯМ
    // Камера находится на (0,0,0), смотрит в направлении (0,0,-1)
    
    // ========== X ОСЬ (ВЛЕВО-ВПРАВО) ==========
    // Кубик СЛЕВА (X отрицательный)
    SceneObject cubeLeft;
    cubeLeft.position = glm::vec3(-0.5f, 0.0f, -0.5f);
    cubeLeft.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeLeft.scale = glm::vec3(0.1f);
    cubeLeft.visible = true;
    cubeLeft.meshId = m_cubeMeshId;
    addObject(cubeLeft);
    
    // Кубик В ЦЕНТРЕ (X = 0)
    SceneObject cubeCenter;
    cubeCenter.position = glm::vec3(0.0f, 0.0f, -0.5f);
    cubeCenter.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeCenter.scale = glm::vec3(0.1f);
    cubeCenter.visible = true;
    cubeCenter.meshId = m_cubeMeshId;
    addObject(cubeCenter);
    
    // Кубик СПРАВА (X положительный)
    SceneObject cubeRight;
    cubeRight.position = glm::vec3(0.5f, 0.0f, -0.5f);
    cubeRight.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeRight.scale = glm::vec3(0.1f);
    cubeRight.visible = true;
    cubeRight.meshId = m_cubeMeshId;
    addObject(cubeRight);
    
    // ========== Y ОСЬ (ВВЕРХ-ВНИЗ) ==========
    // Кубик ВНИЗУ (Y отрицательный)
    SceneObject cubeDown;
    cubeDown.position = glm::vec3(0.0f, -0.5f, -0.5f);
    cubeDown.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeDown.scale = glm::vec3(0.1f);
    cubeDown.visible = true;
    cubeDown.meshId = m_cubeMeshId;
    addObject(cubeDown);
    
    // Кубик ВВЕРХУ (Y положительный)
    SceneObject cubeUp;
    cubeUp.position = glm::vec3(0.0f, 0.5f, -0.5f);
    cubeUp.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeUp.scale = glm::vec3(0.1f);
    cubeUp.visible = true;
    cubeUp.meshId = m_cubeMeshId;
    addObject(cubeUp);
    
    // ========== Z ОСЬ (ВПЕРЕД-НАЗАД) ==========
    // Кубик БЛИЗКО (Z отрицательный, близко к камере)
    SceneObject cubeNear;
    cubeNear.position = glm::vec3(0.0f, 0.0f, -0.3f);
    cubeNear.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeNear.scale = glm::vec3(0.1f);
    cubeNear.visible = true;
    cubeNear.meshId = m_cubeMeshId;
    addObject(cubeNear);
    
    // Кубик ДАЛЕКО (Z отрицательный, далеко от камеры)
    SceneObject cubeFar;
    cubeFar.position = glm::vec3(0.0f, 0.0f, -1.0f);
    cubeFar.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cubeFar.scale = glm::vec3(0.1f);
    cubeFar.visible = true;
    cubeFar.meshId = m_cubeMeshId;
    addObject(cubeFar);
    
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

    // Создаем тор (параметрическая поверхность)
    std::vector<float> torusVertices;
    std::vector<uint32_t> torusIndices;

    const float majorRadius = 0.35f; // радиус от центра до трубы
    const float minorRadius = 0.12f; // радиус трубы
    const int majorSegments = 48;
    const int minorSegments = 24;

    for (int i = 0; i <= majorSegments; ++i) {
        float u = static_cast<float>(i) / static_cast<float>(majorSegments);
        float theta = u * 2.0f * 3.14159f;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (int j = 0; j <= minorSegments; ++j) {
            float v = static_cast<float>(j) / static_cast<float>(minorSegments);
            float phi = v * 2.0f * 3.14159f;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            glm::vec3 center(majorRadius * cosTheta, majorRadius * sinTheta, 0.0f);
            glm::vec3 normal(cosTheta * cosPhi, sinTheta * cosPhi, sinPhi);
            glm::vec3 position = center + normal * minorRadius;
            normal = glm::normalize(normal);

            torusVertices.push_back(position.x);
            torusVertices.push_back(position.y);
            torusVertices.push_back(position.z);
            torusVertices.push_back(normal.x);
            torusVertices.push_back(normal.y);
            torusVertices.push_back(normal.z);
            torusVertices.push_back(u);
            torusVertices.push_back(v);
        }
    }

    const int stride = minorSegments + 1;
    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            uint32_t a = static_cast<uint32_t>(i * stride + j);
            uint32_t b = static_cast<uint32_t>((i + 1) * stride + j);
            uint32_t c = static_cast<uint32_t>((i + 1) * stride + (j + 1));
            uint32_t d = static_cast<uint32_t>(i * stride + (j + 1));

            torusIndices.push_back(a);
            torusIndices.push_back(b);
            torusIndices.push_back(d);

            torusIndices.push_back(b);
            torusIndices.push_back(c);
            torusIndices.push_back(d);
        }
    }

    m_torusMeshId = renderer->createMesh(torusVertices, torusIndices);

    SceneObject torus;
    torus.position = glm::vec3(0.0f, 0.25f, -1.0f); // Чуть выше центра
    torus.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    torus.scale = glm::vec3(0.18f);
    torus.visible = true;
    torus.meshId = m_torusMeshId;
    addObject(torus);

    // Дальние объекты — проверка окклюзии: в настоящей комнате они могут оказаться за стеной/шкафом,
    // и тогда LiDAR их прячет; обошли препятствие — снова видны
    SceneObject farSphere;
    farSphere.position = glm::vec3(0.0f, 0.0f, -2.2f);
    farSphere.scale = glm::vec3(0.25f);
    farSphere.meshId = m_sphereMeshId;
    addObject(farSphere);
    SceneObject farCube;
    farCube.position = glm::vec3(0.6f, -0.3f, -1.8f);
    farCube.scale = glm::vec3(0.2f);
    farCube.meshId = m_cubeMeshId;
    addObject(farCube);

    // На полу (появятся, когда LiDAR найдёт пол): хромовый шар перед перегородкой/мебелью
    // и куб подальше — за препятствием его не видно, пока не обойдёшь
    SceneObject floorSphere;
    floorSphere.position = glm::vec3(-0.5f, 0.0f, -2.6f);
    floorSphere.scale = glm::vec3(0.3f);
    floorSphere.meshId = m_sphereMeshId;
    floorSphere.onFloor = true;
    floorSphere.visible = false;
    addObject(floorSphere);
    SceneObject floorCube;
    floorCube.position = glm::vec3(0.3f, 0.0f, -2.3f);
    floorCube.scale = glm::vec3(0.35f);
    floorCube.meshId = m_cubeMeshId;
    floorCube.onFloor = true;
    floorCube.visible = false;
    addObject(floorCube);

    // На столе (когда LiDAR найдёт стол): золотой тор
    SceneObject tableTorus;
    tableTorus.scale = glm::vec3(0.12f);
    tableTorus.meshId = m_torusMeshId;
    tableTorus.onTable = true;
    tableTorus.visible = false;
    addObject(tableTorus);

    // Материалы: кубы — краска разных цветов, сферы — хром и золото (отражают комнату), тор — глянцевый
    const glm::vec4 paints[] = {{0.93f, 0.33f, 0.25f, 1.0f}, {0.25f, 0.70f, 0.95f, 1.0f}, {0.35f, 0.85f, 0.45f, 1.0f},
                                {0.98f, 0.80f, 0.25f, 1.0f}, {0.70f, 0.45f, 0.95f, 1.0f}, {0.95f, 0.95f, 0.95f, 1.0f},
                                {0.20f, 0.20f, 0.22f, 1.0f}, {0.95f, 0.55f, 0.20f, 1.0f}};
    int cubeIndex = 0, sphereIndex = 0;
    for (auto& obj : m_objects) {
        if (obj.meshId == m_cubeMeshId) {
            obj.color = paints[cubeIndex++ % 8];
            obj.metallic = 0.0f;
            obj.roughness = 0.45f;
        } else if (obj.meshId == m_sphereMeshId) {
            const int k = sphereIndex++ % 3;
            obj.color = k == 0 ? glm::vec4(0.95f, 0.95f, 0.97f, 1.0f)      // хром
                      : k == 1 ? glm::vec4(1.0f, 0.78f, 0.34f, 1.0f)       // золото
                               : glm::vec4(0.9f, 0.9f, 0.92f, 1.0f);       // полированный металл
            obj.metallic = 1.0f;
            obj.roughness = k == 2 ? 0.25f : 0.08f;
        } else if (obj.meshId == m_torusMeshId && obj.onTable) {
            obj.color = glm::vec4(1.0f, 0.78f, 0.34f, 1.0f);   // золото
            obj.metallic = 1.0f;
            obj.roughness = 0.12f;
        } else if (obj.meshId == m_torusMeshId) {
            obj.color = glm::vec4(0.85f, 0.12f, 0.18f, 1.0f);
            obj.metallic = 0.0f;
            obj.roughness = 0.15f;
        }
    }

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
    
    // Обновляем frustum culling после изменения камеры
    updateFrustumCulling();
}

void Scene::updateFrustumCulling()
{
    if (!m_camera) return;
    
    // Получаем матрицы камеры
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 projection = m_camera->getProjectionMatrix();
    glm::mat4 viewProjection = projection * view;
    
    // Вычисляем плоскости frustum из viewProjection матрицы
    // Порядок плоскостей: left, right, bottom, top, near, far
    glm::vec4 frustumPlanes[6];
    
    // Left plane
    frustumPlanes[0] = glm::vec4(viewProjection[0][3] + viewProjection[0][0],
                                 viewProjection[1][3] + viewProjection[1][0],
                                 viewProjection[2][3] + viewProjection[2][0],
                                 viewProjection[3][3] + viewProjection[3][0]);
    
    // Right plane
    frustumPlanes[1] = glm::vec4(viewProjection[0][3] - viewProjection[0][0],
                                 viewProjection[1][3] - viewProjection[1][0],
                                 viewProjection[2][3] - viewProjection[2][0],
                                 viewProjection[3][3] - viewProjection[3][0]);
    
    // Bottom plane
    frustumPlanes[2] = glm::vec4(viewProjection[0][3] + viewProjection[0][1],
                                 viewProjection[1][3] + viewProjection[1][1],
                                 viewProjection[2][3] + viewProjection[2][1],
                                 viewProjection[3][3] + viewProjection[3][1]);
    
    // Top plane
    frustumPlanes[3] = glm::vec4(viewProjection[0][3] - viewProjection[0][1],
                                 viewProjection[1][3] - viewProjection[1][1],
                                 viewProjection[2][3] - viewProjection[2][1],
                                 viewProjection[3][3] - viewProjection[3][1]);
    
    // Near plane
    frustumPlanes[4] = glm::vec4(viewProjection[0][3] + viewProjection[0][2],
                                 viewProjection[1][3] + viewProjection[1][2],
                                 viewProjection[2][3] + viewProjection[2][2],
                                 viewProjection[3][3] + viewProjection[3][2]);
    
    // Far plane
    frustumPlanes[5] = glm::vec4(viewProjection[0][3] - viewProjection[0][2],
                                 viewProjection[1][3] - viewProjection[1][2],
                                 viewProjection[2][3] - viewProjection[2][2],
                                 viewProjection[3][3] - viewProjection[3][2]);
    
    // Нормализуем плоскости
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustumPlanes[i]));
        frustumPlanes[i] /= length;
    }
    
    // Проверяем каждый объект
    for (auto& obj : m_objects) {
        if (!obj.visible) continue; // Если уже скрыт, пропускаем
        
        // Вычисляем максимальный радиус bounding sphere на основе scale
        float maxScale = std::max({obj.scale.x, obj.scale.y, obj.scale.z});
        float sphereRadius = obj.boundingRadius * maxScale;
        
        // Проверяем пересечение с каждой плоскостью frustum
        bool insideFrustum = true;
        for (int i = 0; i < 6; ++i) {
            // Вычисляем расстояние от центра объекта до плоскости
            float distance = glm::dot(glm::vec3(frustumPlanes[i]), obj.position) + frustumPlanes[i].w;
            
            // Если расстояние < -radius, объект полностью вне frustum
            if (distance < -sphereRadius) {
                insideFrustum = false;
                break;
            }
        }
        
        // Скрываем объект если он вне frustum
        obj.visible = insideFrustum;
    }
}

