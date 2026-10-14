#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

namespace LensEngine {

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct MeshTriangle {
    uint32_t v0, v1, v2;
    glm::vec3 normal;
};

struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> triangles;
    glm::vec3 boundsMin;
    glm::vec3 boundsMax;
};

class MeshGenerator {
public:
    MeshGenerator();
    ~MeshGenerator();
    
    // Генерация mesh из LiDAR точек
    Mesh generateMesh(const std::vector<glm::vec3>& points, 
                      float voxelSize = 0.05f,
                      float maxEdgeLength = 0.2f);
    
    // Упрощение mesh
    Mesh simplifyMesh(const Mesh& mesh, float targetReduction = 0.5f);
    
    // Сглаживание mesh
    Mesh smoothMesh(const Mesh& mesh, int iterations = 3);
    
    // Вычисление нормалей
    void computeNormals(Mesh& mesh);
    
private:
    // PCL point cloud conversion
    pcl::PointCloud<pcl::PointXYZ>::Ptr convertToPCL(const std::vector<glm::vec3>& points);
    
    // Voxel grid filtering
    pcl::PointCloud<pcl::PointXYZ>::Ptr voxelFilter(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud, 
                                                    float voxelSize);
    
    // Normal estimation
    pcl::PointCloud<pcl::Normal>::Ptr estimateNormals(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud);
    
    // Poisson surface reconstruction (упрощенная версия)
    Mesh greedyTriangulation(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
                            const pcl::PointCloud<pcl::Normal>::Ptr& normals,
                            float maxEdgeLength);
    
    // Remove outliers
    pcl::PointCloud<pcl::PointXYZ>::Ptr removeOutliers(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud);
};

} // namespace LensEngine
