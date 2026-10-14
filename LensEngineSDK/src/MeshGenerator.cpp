#include "MeshGenerator.h"
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/gp3.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <iostream>

namespace LensEngine {

MeshGenerator::MeshGenerator() {
}

MeshGenerator::~MeshGenerator() {
}

pcl::PointCloud<pcl::PointXYZ>::Ptr MeshGenerator::convertToPCL(const std::vector<glm::vec3>& points) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->width = points.size();
    cloud->height = 1;
    cloud->is_dense = true;
    
    for (const auto& pt : points) {
        pcl::PointXYZ p;
        p.x = pt.x;
        p.y = pt.y;
        p.z = pt.z;
        cloud->push_back(p);
    }
    
    return cloud;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr MeshGenerator::voxelFilter(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud, 
                                                                float voxelSize) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::VoxelGrid<pcl::PointXYZ> voxelFilter;
    voxelFilter.setInputCloud(cloud);
    voxelFilter.setLeafSize(voxelSize, voxelSize, voxelSize);
    voxelFilter.filter(*filtered);
    return filtered;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr MeshGenerator::removeOutliers(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
    sor.setInputCloud(cloud);
    sor.setMeanK(50);
    sor.setStddevMulThresh(1.0);
    sor.filter(*filtered);
    return filtered;
}

pcl::PointCloud<pcl::Normal>::Ptr MeshGenerator::estimateNormals(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud) {
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    
    ne.setInputCloud(cloud);
    ne.setSearchMethod(tree);
    ne.setKSearch(20);
    ne.compute(*normals);
    
    return normals;
}

Mesh MeshGenerator::generateMesh(const std::vector<glm::vec3>& points, 
                                 float voxelSize,
                                 float maxEdgeLength) {
    Mesh mesh;
    
    if (points.empty()) {
        return mesh;
    }
    
    // Convert to PCL
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud = convertToPCL(points);
    
    std::cout << "[MeshGenerator] Original points: " << cloud->size() << std::endl;
    
    // Remove outliers
    cloud = removeOutliers(cloud);
    std::cout << "[MeshGenerator] After outlier removal: " << cloud->size() << std::endl;
    
    // Voxel filter
    cloud = voxelFilter(cloud, voxelSize);
    std::cout << "[MeshGenerator] After voxel filter: " << cloud->size() << std::endl;
    
    if (cloud->size() < 100) {
        std::cerr << "[MeshGenerator] Not enough points for mesh generation" << std::endl;
        return mesh;
    }
    
    // Estimate normals
    pcl::PointCloud<pcl::Normal>::Ptr normals = estimateNormals(cloud);
    
    // Combine points and normals
    pcl::PointCloud<pcl::PointNormal>::Ptr cloudWithNormals(new pcl::PointCloud<pcl::PointNormal>);
    pcl::concatenateFields(*cloud, *normals, *cloudWithNormals);
    
    // Greedy triangulation
    pcl::search::KdTree<pcl::PointNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointNormal>);
    tree->setInputCloud(cloudWithNormals);
    
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;
    pcl::PolygonMesh triangles;
    
    gp3.setSearchRadius(maxEdgeLength);
    gp3.setMu(2.5);
    gp3.setMaximumNearestNeighbors(100);
    gp3.setMaximumSurfaceAngle(M_PI / 2); // 90 degrees
    gp3.setNormalConsistency(false);
    
    gp3.setInputCloud(cloudWithNormals);
    gp3.setSearchMethod(tree);
    gp3.reconstruct(triangles);
    
    std::cout << "[MeshGenerator] Generated mesh with " << triangles.polygons.size() << " triangles" << std::endl;
    
    // Convert to our Mesh format
    const pcl::PointCloud<pcl::PointNormal>& cloudPoints = *cloudWithNormals;
    
    for (size_t i = 0; i < cloudPoints.size(); ++i) {
        MeshVertex v;
        v.position = glm::vec3(cloudPoints[i].x, cloudPoints[i].y, cloudPoints[i].z);
        v.normal = glm::vec3(cloudPoints[i].normal_x, cloudPoints[i].normal_y, cloudPoints[i].normal_z);
        v.texCoord = glm::vec2(0.0f, 0.0f); // TODO: compute UV coordinates
        mesh.vertices.push_back(v);
        
        // Update bounds
        if (i == 0) {
            mesh.boundsMin = v.position;
            mesh.boundsMax = v.position;
        } else {
            mesh.boundsMin = glm::min(mesh.boundsMin, v.position);
            mesh.boundsMax = glm::max(mesh.boundsMax, v.position);
        }
    }
    
    for (const auto& polygon : triangles.polygons) {
        if (polygon.vertices.size() == 3) {
            MeshTriangle tri;
            tri.v0 = polygon.vertices[0];
            tri.v1 = polygon.vertices[1];
            tri.v2 = polygon.vertices[2];
            
            // Compute triangle normal
            glm::vec3 v0 = mesh.vertices[tri.v0].position;
            glm::vec3 v1 = mesh.vertices[tri.v1].position;
            glm::vec3 v2 = mesh.vertices[tri.v2].position;
            tri.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            
            mesh.triangles.push_back(tri);
        }
    }
    
    std::cout << "[MeshGenerator] Final mesh: " << mesh.vertices.size() << " vertices, " 
              << mesh.triangles.size() << " triangles" << std::endl;
    
    return mesh;
}

Mesh MeshGenerator::simplifyMesh(const Mesh& mesh, float targetReduction) {
    // TODO: Implement mesh simplification (quadric error metrics)
    // For now, just return the original mesh
    std::cout << "[MeshGenerator] Mesh simplification not yet implemented" << std::endl;
    return mesh;
}

Mesh MeshGenerator::smoothMesh(const Mesh& mesh, int iterations) {
    // TODO: Implement Laplacian smoothing
    // For now, just return the original mesh
    std::cout << "[MeshGenerator] Mesh smoothing not yet implemented" << std::endl;
    return mesh;
}

void MeshGenerator::computeNormals(Mesh& mesh) {
    // Recompute all vertex normals from triangle normals
    for (auto& vertex : mesh.vertices) {
        vertex.normal = glm::vec3(0.0f);
    }
    
    for (const auto& triangle : mesh.triangles) {
        mesh.vertices[triangle.v0].normal += triangle.normal;
        mesh.vertices[triangle.v1].normal += triangle.normal;
        mesh.vertices[triangle.v2].normal += triangle.normal;
    }
    
    for (auto& vertex : mesh.vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

} // namespace LensEngine
