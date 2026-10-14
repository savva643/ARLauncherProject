#include "PhysicsEngine.h"
#include <iostream>
#include <algorithm>

#ifdef USE_BULLET_PHYSICS

namespace LensEngine {

PhysicsEngine::PhysicsEngine()
    : m_gravity(0.0f, -9.81f, 0.0f)
    , m_initialized(false)
{
}

PhysicsEngine::~PhysicsEngine() {
    shutdown();
}

bool PhysicsEngine::initialize() {
#ifdef USE_BULLET_PHYSICS
    try {
        // Создание компонентов Bullet Physics
        m_broadphase = std::make_unique<btDbvtBroadphase>();
        m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
        m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
        m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
            m_dispatcher.get(),
            m_broadphase.get(),
            m_solver.get(),
            m_collisionConfig.get()
        );
        
        // Установка гравитации
        m_dynamicsWorld->setGravity(btVector3(m_gravity.x, m_gravity.y, m_gravity.z));
        
        m_initialized = true;
        std::cout << "[PhysicsEngine] Initialized with Bullet Physics" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PhysicsEngine] Exception: " << e.what() << std::endl;
        return false;
    }
#else
    std::cout << "[PhysicsEngine] Bullet Physics not enabled, using fallback" << std::endl;
    m_initialized = true;
    return true;
#endif
}

void PhysicsEngine::shutdown() {
#ifdef USE_BULLET_PHYSICS
    if (m_dynamicsWorld) {
        // Удаление всех тел из мира
        for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
            btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
            if (obj) {
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState()) {
                    delete body->getMotionState();
                }
                m_dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }
        }
    }
    
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_dispatcher.reset();
    m_collisionConfig.reset();
    m_broadphase.reset();
    m_collisionShapes.clear();
#endif
    
    m_bodies.clear();
    m_initialized = false;
}

void PhysicsEngine::update(float deltaTime) {
#ifdef USE_BULLET_PHYSICS
    if (!m_initialized || !m_dynamicsWorld) return;
    
    // Симуляция физики
    m_dynamicsWorld->stepSimulation(deltaTime, 10);
    
    // Обновление состояния тел
    for (auto& body : m_bodies) {
        if (body.btBody) {
            btTransform trans;
            body.btBody->getMotionState()->getWorldTransform(trans);
            
            body.position = glm::vec3(
                trans.getOrigin().x(),
                trans.getOrigin().y(),
                trans.getOrigin().z()
            );
            
            body.rotation = glm::quat(
                trans.getRotation().w(),
                trans.getRotation().x(),
                trans.getRotation().y(),
                trans.getRotation().z()
            );
            
            btVector3 vel = body.btBody->getLinearVelocity();
            body.velocity = glm::vec3(vel.x(), vel.y(), vel.z());
            
            btVector3 angVel = body.btBody->getAngularVelocity();
            body.angularVelocity = glm::vec3(angVel.x(), angVel.y(), angVel.z());
        }
    }
#else
    // Fallback: простая симуляция без физики
    for (auto& body : m_bodies) {
        if (!body.isStatic) {
            body.position += body.velocity * deltaTime;
            body.velocity += m_gravity * deltaTime;
        }
    }
#endif
}

std::string PhysicsEngine::createRigidBody(
    const std::string& id,
    CollisionShapeType shapeType,
    const glm::vec3& position,
    const glm::quat& rotation,
    const glm::vec3& size,
    float mass,
    bool isStatic
) {
#ifdef USE_BULLET_PHYSICS
    if (!m_initialized || !m_dynamicsWorld) {
        std::cerr << "[PhysicsEngine] Not initialized" << std::endl;
        return "";
    }
    
    btCollisionShape* shape = nullptr;
    
    switch (shapeType) {
        case CollisionShapeType::BOX:
            shape = new btBoxShape(btVector3(size.x / 2, size.y / 2, size.z / 2));
            break;
        case CollisionShapeType::SPHERE:
            shape = new btSphereShape(size.x);
            break;
        case CollisionShapeType::CYLINDER:
            shape = new btCylinderShape(btVector3(size.x / 2, size.y / 2, size.z / 2));
            break;
        case CollisionShapeType::CAPSULE:
            shape = new btCapsuleShape(size.x, size.y);
            break;
        case CollisionShapeType::PLANE:
            shape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
            break;
        default:
            shape = new btBoxShape(btVector3(size.x / 2, size.y / 2, size.z / 2));
            break;
    }
    
    m_collisionShapes.push_back(std::unique_ptr<btCollisionShape>(shape));
    
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(btVector3(position.x, position.y, position.z));
    trans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
    
    btVector3 localInertia(0, 0, 0);
    if (!isStatic && mass > 0) {
        shape->calculateLocalInertia(mass, localInertia);
    }
    
    btMotionState* motionState = new btDefaultMotionState(trans);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    
    if (isStatic) {
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    }
    
    m_dynamicsWorld->addRigidBody(body);
    
    // Создание записи в m_bodies
    RigidBody rigidBody;
    rigidBody.id = id;
    rigidBody.position = position;
    rigidBody.rotation = rotation;
    rigidBody.velocity = glm::vec3(0);
    rigidBody.angularVelocity = glm::vec3(0);
    rigidBody.mass = mass;
    rigidBody.isStatic = isStatic;
    rigidBody.isActive = true;
    rigidBody.btBody = body;
    
    m_bodies.push_back(rigidBody);
    
    std::cout << "[PhysicsEngine] Created rigid body: " << id << std::endl;
    return id;
#else
    // Fallback
    RigidBody rigidBody;
    rigidBody.id = id;
    rigidBody.position = position;
    rigidBody.rotation = rotation;
    rigidBody.velocity = glm::vec3(0);
    rigidBody.angularVelocity = glm::vec3(0);
    rigidBody.mass = mass;
    rigidBody.isStatic = isStatic;
    rigidBody.isActive = true;
    
    m_bodies.push_back(rigidBody);
    
    std::cout << "[PhysicsEngine] Created rigid body (fallback): " << id << std::endl;
    return id;
#endif
}

void PhysicsEngine::removeRigidBody(const std::string& id) {
#ifdef USE_BULLET_PHYSICS
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && it->btBody) {
        m_dynamicsWorld->removeRigidBody(it->btBody);
        delete it->btBody->getMotionState();
        delete it->btBody;
        m_bodies.erase(it);
    }
#else
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end()) {
        m_bodies.erase(it);
    }
#endif
}

RigidBody PhysicsEngine::getRigidBody(const std::string& id) const {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end()) {
        return *it;
    }
    
    return RigidBody();
}

std::vector<RigidBody> PhysicsEngine::getAllBodies() const {
    return m_bodies;
}

void PhysicsEngine::applyForce(const std::string& id, const glm::vec3& force) {
#ifdef USE_BULLET_PHYSICS
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && it->btBody) {
        it->btBody->applyCentralForce(btVector3(force.x, force.y, force.z));
        it->btBody->activate();
    }
#else
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && !it->isStatic) {
        it->velocity += force / it->mass;
    }
#endif
}

void PhysicsEngine::applyTorque(const std::string& id, const glm::vec3& torque) {
#ifdef USE_BULLET_PHYSICS
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && it->btBody) {
        it->btBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
        it->btBody->activate();
    }
#endif
}

void PhysicsEngine::setVelocity(const std::string& id, const glm::vec3& velocity) {
#ifdef USE_BULLET_PHYSICS
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && it->btBody) {
        it->btBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
        it->btBody->activate();
    }
#else
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end()) {
        it->velocity = velocity;
    }
#endif
}

void PhysicsEngine::setAngularVelocity(const std::string& id, const glm::vec3& angularVelocity) {
#ifdef USE_BULLET_PHYSICS
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end() && it->btBody) {
        it->btBody->setAngularVelocity(btVector3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
        it->btBody->activate();
    }
#else
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(), 
        [&id](const RigidBody& body) { return body.id == id; });
    
    if (it != m_bodies.end()) {
        it->angularVelocity = angularVelocity;
    }
#endif
}

void PhysicsEngine::setGravity(const glm::vec3& gravity) {
    m_gravity = gravity;
#ifdef USE_BULLET_PHYSICS
    if (m_dynamicsWorld) {
        m_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }
#endif
}

glm::vec3 PhysicsEngine::getGravity() const {
    return m_gravity;
}

void PhysicsEngine::addPlane(const glm::vec3& normal, float distance) {
    createRigidBody(
        "plane_" + std::to_string(m_bodies.size()),
        CollisionShapeType::PLANE,
        glm::vec3(0),
        glm::quat(1, 0, 0, 0),
        glm::vec3(1000, 1, 1000),
        0,
        true
    );
}

} // namespace LensEngine

#endif // USE_BULLET_PHYSICS
