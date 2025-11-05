#ifndef SPATIALMAPPINGSYSTEM_H
#define SPATIALMAPPINGSYSTEM_H

#include "LensEngineTypes.h"
#include "Lidar3DProcessor.h"
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <memory>

namespace LensEngine {

/**
 * @brief Система пространственного маппирования
 * 
 * Создает 3D карту окружения и управляет виртуальными объектами.
 */
class SpatialMappingSystem {
public:
    struct SpatialMesh {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<uint32_t> indices;
        glm::vec3 boundsMin;
        glm::vec3 boundsMax;
        float confidence;
        uint64_t timestamp;
    };

    struct VirtualObject {
        std::string id;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        bool isStatic;
        bool isOccluded;
        float occlusionFactor;
        glm::mat4 worldMatrix;
    };

    SpatialMappingSystem();
    ~SpatialMappingSystem();

    void initialize();
    void updateFromLiDAR(const Lidar3DProcessor::SpatialAnalysisResult &analysis,
                         const std::vector<glm::vec3> &points);
    void updateCameraPose(const glm::vec3 &position, const glm::quat &rotation);

    // Управление виртуальными объектами
    std::string addVirtualObject(const glm::vec3 &position, const glm::vec3 &size,
                                bool isStatic = true);
    void removeVirtualObject(const std::string &id);
    void updateVirtualObjectPosition(const std::string &id, const glm::vec3 &position);

    // Окклюзия и видимость
    void calculateOcclusion();
    bool isPointVisible(const glm::vec3 &point) const;
    bool isObjectVisible(const VirtualObject &object) const;

    // Получение данных
    std::vector<SpatialMesh> getSpatialMeshes() const;
    std::vector<VirtualObject> getVirtualObjects() const;
    std::vector<glm::vec3> getFloorPoints() const;
    std::vector<glm::vec3> getWallPoints() const;

    // Настройки
    void setOcclusionEnabled(bool enabled);
    void setMeshGenerationEnabled(bool enabled);

private:
    void generateSpatialMesh(const std::vector<glm::vec3> &points);
    void updateVirtualObjectMatrices();
    void detectRoomBounds();
    void fuseSpatialData(const std::vector<glm::vec3> &newPoints);

    // Математические методы
    bool rayIntersectsMesh(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
                           const SpatialMesh &mesh) const;
    float calculateOcclusionForObject(const VirtualObject &object) const;
    glm::vec3 projectPointToFloor(const glm::vec3 &point) const;

    // Данные
    std::vector<glm::vec3> m_spatialPoints;
    std::vector<SpatialMesh> m_spatialMeshes;
    std::map<std::string, VirtualObject> m_virtualObjects;

    // Пространственная информация
    glm::vec3 m_floorNormal;
    float m_floorHeight;
    glm::vec3 m_roomBoundsMin;
    glm::vec3 m_roomBoundsMax;
    bool m_hasFloor;

    // Текущая поза камеры
    glm::vec3 m_cameraPosition;
    glm::quat m_cameraRotation;

    // Настройки
    bool m_occlusionEnabled;
    bool m_meshGenerationEnabled;

    // Потокобезопасность
    mutable std::mutex m_dataLock;
};

} // namespace LensEngine

#endif // SPATIALMAPPINGSYSTEM_H

