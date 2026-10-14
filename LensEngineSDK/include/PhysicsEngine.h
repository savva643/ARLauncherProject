#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <string>

#ifdef USE_BULLET_PHYSICS
#include <btBulletDynamicsCommon.h>
#endif

namespace LensEngine {

// Типы коллизионных форм
enum class CollisionShapeType {
    BOX,
    SPHERE,
    CYLINDER,
    CAPSULE,
    MESH,
    PLANE
};

// Физическое тело
struct RigidBody {
    std::string id;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 velocity;
    glm::vec3 angularVelocity;
    float mass;
    bool isStatic;
    bool isActive;
    
#ifdef USE_BULLET_PHYSICS
    btRigidBody* btBody;
#endif
};

// Результат физического симуляции
struct PhysicsState {
    std::vector<RigidBody> bodies;
    float deltaTime;
};

class PhysicsEngine {
public:
    PhysicsEngine();
    ~PhysicsEngine();

    // Инициализация физического движка
    bool initialize();

    // Очистка
    void shutdown();

    // Обновление симуляции
    void update(float deltaTime);

    // Создание физического тела
    std::string createRigidBody(
        const std::string& id,
        CollisionShapeType shapeType,
        const glm::vec3& position,
        const glm::quat& rotation,
        const glm::vec3& size,
        float mass,
        bool isStatic = false
    );

    // Удаление физического тела
    void removeRigidBody(const std::string& id);

    // Получение состояния тела
    RigidBody getRigidBody(const std::string& id) const;

    // Получение всех тел
    std::vector<RigidBody> getAllBodies() const;

    // Применение силы к телу
    void applyForce(const std::string& id, const glm::vec3& force);
    void applyTorque(const std::string& id, const glm::vec3& torque);

    // Установка скорости
    void setVelocity(const std::string& id, const glm::vec3& velocity);
    void setAngularVelocity(const std::string& id, const glm::vec3& angularVelocity);

    // Установка гравитации
    void setGravity(const glm::vec3& gravity);

    // Получение гравитации
    glm::vec3 getGravity() const;

    // Добавление плоскости (пол/стена)
    void addPlane(const glm::vec3& normal, float distance);

private:
#ifdef USE_BULLET_PHYSICS
    // Bullet Physics компоненты
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btCollisionConfiguration> m_collisionConfig;
    std::unique_ptr<btDispatcher> m_dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    
    // Хранилище коллизионных форм
    std::vector<std::unique_ptr<btCollisionShape>> m_collisionShapes;
#endif

    // Хранилище физических тел
    std::vector<RigidBody> m_bodies;

    // Гравитация
    glm::vec3 m_gravity;

    // Флаг инициализации
    bool m_initialized;
};

} // namespace LensEngine
