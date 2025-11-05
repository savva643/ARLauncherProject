#include "SpatialMappingSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>

namespace LensEngine {

SpatialMappingSystem::SpatialMappingSystem()
    : m_floorNormal(0.0f, 1.0f, 0.0f)
    , m_floorHeight(0.0f)
    , m_roomBoundsMin(0.0f)
    , m_roomBoundsMax(0.0f)
    , m_hasFloor(false)
    , m_cameraPosition(0.0f)
    , m_cameraRotation(1.0f, 0.0f, 0.0f, 0.0f)
    , m_occlusionEnabled(true)
    , m_meshGenerationEnabled(true)
{
}

SpatialMappingSystem::~SpatialMappingSystem()
{
}

void SpatialMappingSystem::initialize()
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    // Инициализация
}

void SpatialMappingSystem::updateFromLiDAR(const Lidar3DProcessor::SpatialAnalysisResult &analysis,
                                           const std::vector<glm::vec3> &points)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    
    m_hasFloor = analysis.hasFloor;
    m_floorHeight = analysis.floorHeight;
    m_floorNormal = analysis.floorNormal;
    
    fuseSpatialData(points);
    
    if (m_meshGenerationEnabled) {
        generateSpatialMesh(points);
    }
    
    detectRoomBounds();
}

void SpatialMappingSystem::updateCameraPose(const glm::vec3 &position, const glm::quat &rotation)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    m_cameraPosition = position;
    m_cameraRotation = rotation;
    
    if (m_occlusionEnabled) {
        calculateOcclusion();
    }
}

std::string SpatialMappingSystem::addVirtualObject(const glm::vec3 &position, const glm::vec3 &size,
                                                   bool isStatic)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    
    VirtualObject obj;
    obj.id = "obj_" + std::to_string(m_virtualObjects.size());
    obj.position = position;
    obj.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    obj.scale = size;
    obj.isStatic = isStatic;
    obj.isOccluded = false;
    obj.occlusionFactor = 0.0f;
    
    m_virtualObjects[obj.id] = obj;
    updateVirtualObjectMatrices();
    
    return obj.id;
}

void SpatialMappingSystem::removeVirtualObject(const std::string &id)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    m_virtualObjects.erase(id);
}

void SpatialMappingSystem::updateVirtualObjectPosition(const std::string &id, const glm::vec3 &position)
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    auto it = m_virtualObjects.find(id);
    if (it != m_virtualObjects.end()) {
        it->second.position = position;
        updateVirtualObjectMatrices();
    }
}

void SpatialMappingSystem::calculateOcclusion()
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    
    for (auto& [id, obj] : m_virtualObjects) {
        obj.occlusionFactor = calculateOcclusionForObject(obj);
        obj.isOccluded = obj.occlusionFactor > 0.5f;
    }
}

bool SpatialMappingSystem::isPointVisible(const glm::vec3 &point) const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    // TODO: Реализовать проверку видимости
    return true;
}

bool SpatialMappingSystem::isObjectVisible(const VirtualObject &object) const
{
    return !object.isOccluded && isPointVisible(object.position);
}

std::vector<SpatialMappingSystem::SpatialMesh> SpatialMappingSystem::getSpatialMeshes() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    return m_spatialMeshes;
}

std::vector<SpatialMappingSystem::VirtualObject> SpatialMappingSystem::getVirtualObjects() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    std::vector<VirtualObject> result;
    for (const auto& [id, obj] : m_virtualObjects) {
        result.push_back(obj);
    }
    return result;
}

std::vector<glm::vec3> SpatialMappingSystem::getFloorPoints() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    // TODO: Фильтровать точки пола
    return m_spatialPoints;
}

std::vector<glm::vec3> SpatialMappingSystem::getWallPoints() const
{
    std::lock_guard<std::mutex> lock(m_dataLock);
    // TODO: Фильтровать точки стен
    return std::vector<glm::vec3>();
}

void SpatialMappingSystem::setOcclusionEnabled(bool enabled)
{
    m_occlusionEnabled = enabled;
}

void SpatialMappingSystem::setMeshGenerationEnabled(bool enabled)
{
    m_meshGenerationEnabled = enabled;
}

void SpatialMappingSystem::generateSpatialMesh(const std::vector<glm::vec3> &points)
{
    // TODO: Реализовать генерацию меша
}

void SpatialMappingSystem::updateVirtualObjectMatrices()
{
    for (auto& [id, obj] : m_virtualObjects) {
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), obj.scale);
        glm::mat4 rotation = glm::mat4_cast(obj.rotation);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), obj.position);
        
        obj.worldMatrix = translation * rotation * scale;
    }
}

void SpatialMappingSystem::detectRoomBounds()
{
    if (m_spatialPoints.empty()) {
        return;
    }
    
    m_roomBoundsMin = m_roomBoundsMax = m_spatialPoints[0];
    
    for (const auto& point : m_spatialPoints) {
        m_roomBoundsMin = glm::min(m_roomBoundsMin, point);
        m_roomBoundsMax = glm::max(m_roomBoundsMax, point);
    }
}

void SpatialMappingSystem::fuseSpatialData(const std::vector<glm::vec3> &newPoints)
{
    // TODO: Реализовать слияние данных
    m_spatialPoints.insert(m_spatialPoints.end(), newPoints.begin(), newPoints.end());
}

bool SpatialMappingSystem::rayIntersectsMesh(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                                             const SpatialMesh &mesh) const
{
    // TODO: Реализовать проверку пересечения луча с мешем
    return false;
}

float SpatialMappingSystem::calculateOcclusionForObject(const VirtualObject &object) const
{
    // TODO: Реализовать расчет окклюзии
    return 0.0f;
}

glm::vec3 SpatialMappingSystem::projectPointToFloor(const glm::vec3 &point) const
{
    if (!m_hasFloor) {
        return point;
    }
    
    float distance = glm::dot(point - glm::vec3(0, m_floorHeight, 0), m_floorNormal);
    return point - m_floorNormal * distance;
}

} // namespace LensEngine

