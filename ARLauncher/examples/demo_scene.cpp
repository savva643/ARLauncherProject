#include "Scene.h"
#include "Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

/**
 * @brief Пример создания демо-сцены с 3D объектами
 */
void createDemoScene(Scene* scene, Renderer* renderer) {
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
    };
    
    std::vector<uint32_t> cubeIndices = {
        0, 1, 2,  2, 3, 0,  // Передняя
        4, 5, 6,  6, 7, 4,  // Задняя
        0, 4, 7,  7, 3, 0,  // Левая
        1, 5, 6,  6, 2, 1,  // Правая
        3, 2, 6,  6, 7, 3,  // Верхняя
        0, 1, 5,  5, 4, 0   // Нижняя
    };
    
    uint32_t cubeMeshId = renderer->createMesh(cubeVertices, cubeIndices);
    
    // Создание объектов в сцене
    SceneObject cube1;
    cube1.meshId = cubeMeshId;
    cube1.position = glm::vec3(0.0f, 0.0f, -1.0f); // На 1 метр перед камерой
    cube1.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube1.scale = glm::vec3(0.2f); // 20 см куб
    cube1.visible = true;
    
    scene->addObject(cube1);
    
    // Второй куб слева
    SceneObject cube2;
    cube2.meshId = cubeMeshId;
    cube2.position = glm::vec3(-0.5f, 0.0f, -1.0f);
    cube2.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube2.scale = glm::vec3(0.15f);
    cube2.visible = true;
    
    scene->addObject(cube2);
    
    // Третий куб справа
    SceneObject cube3;
    cube3.meshId = cubeMeshId;
    cube3.position = glm::vec3(0.5f, 0.0f, -1.0f);
    cube3.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    cube3.scale = glm::vec3(0.15f);
    cube3.visible = true;
    
    scene->addObject(cube3);
}

