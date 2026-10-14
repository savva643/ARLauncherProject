#include "PlaneDetector.h"
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <algorithm>
#include <iostream>

namespace LensEngine {

PlaneDetector::PlaneDetector() {
}

PlaneDetector::~PlaneDetector() {
}

bool PlaneDetector::isFloorPlane(const glm::vec3& normal) {
    // Пол имеет нормаль направленную вверх (0, 1, 0)
    // Допускаем отклонение до 30 градусов
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    float dot = glm::dot(glm::normalize(normal), up);
    return dot > 0.866f; // cos(30°) ≈ 0.866
}

bool PlaneDetector::isWallPlane(const glm::vec3& normal) {
    // Стена имеет горизонтальную нормаль
    // Допускаем отклонение до 30 градусов от горизонтали
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    float dot = glm::abs(glm::dot(glm::normalize(normal), up));
    return dot < 0.5f; // cos(60°) = 0.5
}

std::vector<PlaneDetector::DetectedPlane> PlaneDetector::detectPlanes(
    const std::vector<glm::vec3>& points3D, int maxPlanes)
{
    std::vector<DetectedPlane> planes;
    
    if (points3D.empty()) {
        return planes;
    }

    // Конвертируем в PCL point cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->points.resize(points3D.size());
    
    for (size_t i = 0; i < points3D.size(); ++i) {
        cloud->points[i].x = points3D[i].x;
        cloud->points[i].y = points3D[i].y;
        cloud->points[i].z = points3D[i].z;
    }
    cloud->width = points3D.size();
    cloud->height = 1;
    cloud->is_dense = true;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudFiltered = cloud;
    
    for (int planeIdx = 0; planeIdx < maxPlanes; ++planeIdx) {
        if (cloudFiltered->points.size() < static_cast<size_t>(m_minPoints)) {
            break;
        }

        // RANSAC сегментация
        pcl::SACSegmentation<pcl::PointXYZ> seg;
        pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
        pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
        
        seg.setOptimizeCoefficients(true);
        seg.setModelType(pcl::SACMODEL_PLANE);
        seg.setMethodType(pcl::SAC_RANSAC);
        seg.setDistanceThreshold(m_ransacThreshold);
        seg.setMaxIterations(100);
        
        seg.setInputCloud(cloudFiltered);
        seg.segment(*inliers, *coefficients);
        
        if (inliers->indices.size() < static_cast<size_t>(m_minPoints)) {
            break;
        }

        // Создаем плоскость
        DetectedPlane plane;
        plane.normal = glm::vec3(
            coefficients->values[0],
            coefficients->values[1],
            coefficients->values[2]
        );
        plane.distance = coefficients->values[3];
        plane.confidence = static_cast<float>(inliers->indices.size()) / cloudFiltered->points.size();
        plane.isFloor = isFloorPlane(plane.normal);
        plane.isWall = isWallPlane(plane.normal);
        
        // Сохраняем точки плоскости
        for (const auto& idx : inliers->indices) {
            glm::vec3 pt(
                cloudFiltered->points[idx].x,
                cloudFiltered->points[idx].y,
                cloudFiltered->points[idx].z
            );
            plane.points.push_back(pt);
        }
        
        planes.push_back(plane);

        // Если это пол, сохраняем как главную плоскость
        if (plane.isFloor && plane.confidence > 0.3f) {
            m_mainPlane = plane;
            m_hasFloor = true;
            
            // Вычисляем высоту пола (средняя Y точек)
            float sumY = 0.0f;
            for (const auto& pt : plane.points) {
                sumY += pt.y;
            }
            m_floorHeight = sumY / plane.points.size();
        }

        // Удаляем точки этой плоскости для поиска следующей
        pcl::ExtractIndices<pcl::PointXYZ> extract;
        extract.setInputCloud(cloudFiltered);
        extract.setIndices(inliers);
        extract.setNegative(true);
        
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloudRemaining(new pcl::PointCloud<pcl::PointXYZ>);
        extract.filter(*cloudRemaining);
        cloudFiltered = cloudRemaining;
    }

    return planes;
}

} // namespace LensEngine
