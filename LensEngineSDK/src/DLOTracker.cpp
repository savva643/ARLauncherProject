#include "DLOTracker.h"
#include "LensEngineTypes.h"
#include <iostream>
#include <iomanip>
#include <pcl/registration/icp.h>
#include <pcl/registration/gicp.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/common/transforms.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace LensEngine {

DLOTracker::DLOTracker()
    : m_voxelSize(0.1f),
      m_maxRange(5.0f),
      m_maxICPIterations(50),
      m_icpFitnessThreshold(0.01f),
      m_isInitialized(false)
{
    m_previousCloud = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    m_worldCloud = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    
    m_currentPose.position = glm::vec3(0.0f);
    m_currentPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_currentPose.confidence = 0.0f;
    m_currentPose.timestamp = 0;
    
    m_smoothedPosition = glm::vec3(0.0f);
    m_smoothedRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    m_stats.isInitialized = false;
    m_stats.icpFitness = 0.0f;
    m_stats.correspondences = 0;
    m_stats.transformationError = 0.0f;
    
    m_imuPosition = glm::vec3(0.0f);
    m_imuOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_imuVelocity = glm::vec3(0.0f);
    m_imuAccelBias = glm::vec3(0.0f);
    m_imuGyroBias = glm::vec3(0.0f);
    m_lastICPTransform = Eigen::Matrix4f::Identity();
    m_hasICPResult = false;
    m_lastLidarTimestamp = 0.0;
    m_lastFrameIMUPosition = glm::vec3(0.0f);
    m_lastFrameIMUOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

DLOTracker::~DLOTracker()
{
}

void DLOTracker::feedIMU(const RawIMUData& imu, double dt)
{
    if (!m_isInitialized || dt <= 0 || dt > 0.1) return;
    propagateIMUState(imu, dt);
}

void DLOTracker::propagateIMUState(const RawIMUData& imu, double dt)
{
    // User acceleration from iPhone (already gravity-free in device frame)
    glm::vec3 userAccel(imu.accelX, imu.accelY, imu.accelZ);
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Subtract gyro bias
    gyro -= m_imuGyroBias;
    
    // Integrate gyroscope: orientation update
    float angle = glm::length(gyro) * dt;
    if (angle > 1e-8f) {
        glm::vec3 axis = gyro / glm::length(gyro);
        glm::quat deltaRot = glm::angleAxis(angle, axis);
        m_imuOrientation = glm::normalize(m_imuOrientation * deltaRot);
    }
    
    // Transform acceleration to world frame and integrate
    glm::vec3 worldAccel = m_imuOrientation * userAccel;
    
    // Velocity integration (first order)
    m_imuVelocity += worldAccel * static_cast<float>(dt);
    
    // Apply velocity damping to prevent drift when stationary
    float velMag = glm::length(m_imuVelocity);
    if (velMag > 0.001f) {
        m_imuVelocity *= 0.98f; // 2% decay per tick
    } else {
        m_imuVelocity = glm::vec3(0.0f);
    }
    
    // Position integration
    m_imuPosition += m_imuVelocity * static_cast<float>(dt);
}

Eigen::Matrix4f DLOTracker::imuPredict()
{
    Eigen::Matrix4f T = Eigen::Matrix4f::Identity();
    
    // Build rotation from IMU quaternion
    T(0, 0) = m_imuOrientation.w*m_imuOrientation.w + m_imuOrientation.x*m_imuOrientation.x - m_imuOrientation.y*m_imuOrientation.y - m_imuOrientation.z*m_imuOrientation.z;
    T(0, 1) = 2.0f*(m_imuOrientation.x*m_imuOrientation.y - m_imuOrientation.w*m_imuOrientation.z);
    T(0, 2) = 2.0f*(m_imuOrientation.x*m_imuOrientation.z + m_imuOrientation.w*m_imuOrientation.y);
    T(1, 0) = 2.0f*(m_imuOrientation.x*m_imuOrientation.y + m_imuOrientation.w*m_imuOrientation.z);
    T(1, 1) = m_imuOrientation.w*m_imuOrientation.w - m_imuOrientation.x*m_imuOrientation.x + m_imuOrientation.y*m_imuOrientation.y - m_imuOrientation.z*m_imuOrientation.z;
    T(1, 2) = 2.0f*(m_imuOrientation.y*m_imuOrientation.z - m_imuOrientation.w*m_imuOrientation.x);
    T(2, 0) = 2.0f*(m_imuOrientation.x*m_imuOrientation.z - m_imuOrientation.w*m_imuOrientation.y);
    T(2, 1) = 2.0f*(m_imuOrientation.y*m_imuOrientation.z + m_imuOrientation.w*m_imuOrientation.x);
    T(2, 2) = m_imuOrientation.w*m_imuOrientation.w - m_imuOrientation.x*m_imuOrientation.x - m_imuOrientation.y*m_imuOrientation.y + m_imuOrientation.z*m_imuOrientation.z;
    
    T(0, 3) = m_imuPosition.x;
    T(1, 3) = m_imuPosition.y;
    T(2, 3) = m_imuPosition.z;
    
    return T;
}

bool DLOTracker::initialize(float voxelSize, int maxIterations)
{
    m_voxelSize = voxelSize;
    m_maxICPIterations = maxIterations;
    m_isInitialized = true;
    m_stats.isInitialized = true;
    
    std::cout << "[DLO] Initialized with voxel size: " << voxelSize 
              << "m, max ICP iterations: " << maxIterations << std::endl;
    
    return true;
}

DLOPose DLOTracker::update(const std::vector<glm::vec3>& points, const IMUData* imuData)
{
    // Конвертируем glm::vec3 в PCL формат
    auto pclCloud = glmToPCL(points);
    return update(pclCloud, imuData);
}

DLOPose DLOTracker::update(const pcl::PointCloud<pcl::PointXYZI>::Ptr& pointCloud,
                           const IMUData* imuData)
{
    if (!m_isInitialized || !pointCloud || pointCloud->empty())
        return m_currentPose;
    
    // Rate limit to ~10 Hz
    auto now = std::chrono::steady_clock::now();
    static auto lastProcessTime = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(now - lastProcessTime).count() < 0.1)
        return m_currentPose;
    lastProcessTime = now;
    
    auto filteredCloud = filterPointCloud(pointCloud);
    if (filteredCloud->empty() || filteredCloud->size() < 20)
        return m_currentPose;
    
    // First frame: save as world-space target
    if (m_previousCloud->empty()) {
        *m_previousCloud = *filteredCloud;
        m_lastFrameIMUPosition = m_imuPosition;
        m_lastFrameIMUOrientation = m_imuOrientation;
        std::cout << "[DLO] First frame: " << filteredCloud->size() << " points" << std::endl;
        return m_currentPose;
    }
    
    // GICP: align new cloud to target
    // Target is in WORLD frame (transformed after each successful frame)
    // Source is in CAMERA frame (current raw scan)
    pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI> gicp;
    gicp.setInputSource(filteredCloud);
    gicp.setInputTarget(m_previousCloud);
    gicp.setMaximumIterations(m_maxICPIterations);
    gicp.setTransformationEpsilon(1e-6);
    gicp.setEuclideanFitnessEpsilon(1e-3);
    gicp.setMaxCorrespondenceDistance(2.0);
    
    pcl::PointCloud<pcl::PointXYZI> aligned;
    gicp.align(aligned);
    
    float fitness = gicp.getFitnessScore();
    m_stats.icpFitness = fitness;
    m_stats.correspondences = static_cast<int>(aligned.size());
    
    if (!gicp.hasConverged()) {
        m_currentPose.confidence = 0.0f;
        return m_currentPose;
    }
    
    Eigen::Matrix4f T_gicp = gicp.getFinalTransformation();
    Eigen::Vector3f deltaCamera = T_gicp.block<3,1>(0,3);
    float deltaMag = deltaCamera.norm();
    
    // Only reject massive errors
    if (deltaMag > 3.0f) {
        m_currentPose.confidence = 0.0f;
        return m_currentPose;
    }
    
    // Transform delta to world frame using current IMU rotation
    // Camera frame: X=right, Y=up, Z=forward
    // Body frame: X=right, Y=up, Z=toward user (opposite to camera Z)
    // So camera Z = -body Z: negate Z before converting
    glm::vec3 cameraDelta(deltaCamera.x(), deltaCamera.y(), -deltaCamera.z());
    glm::vec3 worldDelta = glm::conjugate(m_currentPose.rotation) * cameraDelta;
    m_currentPose.position += worldDelta;
    
    // ALWAYS update target cloud — keeps overlap high
    *m_previousCloud = *filteredCloud;
    m_currentPose.confidence = glm::clamp(1.0f - fitness / 0.1f, 0.0f, 1.0f);
    m_currentPose.timestamp = static_cast<uint64_t>(m_lastLidarTimestamp);
    m_hasICPResult = true;
    
    static int logCounter = 0;
    if (logCounter++ % 10 == 0) {
        std::cout << "[DLO] Pos: (" << std::fixed << std::setprecision(2)
                  << m_currentPose.position.x << ", "
                  << m_currentPose.position.y << ", "
                  << m_currentPose.position.z << ") "
                  << "Conf: " << m_currentPose.confidence
                  << " ICP: " << m_stats.icpFitness
                  << " Pts: " << filteredCloud->size() << std::endl;
    }
    
    return m_currentPose;
}

bool DLOTracker::performICP(const pcl::PointCloud<pcl::PointXYZI>::Ptr& source,
                            const pcl::PointCloud<pcl::PointXYZI>::Ptr& target,
                            Eigen::Matrix4f& transformation)
{
    // Используем Generalized ICP для лучшей сходимости
    pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI> gicp;
    
    gicp.setInputSource(source);
    gicp.setInputTarget(target);
    gicp.setMaximumIterations(m_maxICPIterations);
    gicp.setTransformationEpsilon(1e-8);
    gicp.setEuclideanFitnessEpsilon(1e-4);
    
    // Выравниваем облака
    pcl::PointCloud<pcl::PointXYZI> aligned;
    gicp.align(aligned);
    
    // Проверяем сходимость
    if (!gicp.hasConverged()) {
        std::cout << "[DLO] ICP did not converge!" << std::endl;
        return false;
    }
    
    // Получаем матрицу трансформации
    transformation = gicp.getFinalTransformation();
    
    // Сохраняем статистику
    m_stats.icpFitness = gicp.getFitnessScore();
    m_stats.correspondences = aligned.size();
    m_stats.transformationError = glm::length(glm::vec3(
        transformation(0, 3), transformation(1, 3), transformation(2, 3)));
    
    return true;
}

pcl::PointCloud<pcl::PointXYZI>::Ptr DLOTracker::filterPointCloud(
    const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud)
{
    if (!cloud || cloud->empty()) {
        return pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    }
    
    auto rangeFiltered = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    for (const auto& point : cloud->points) {
        float dist = std::sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
        if (dist >= 0.1f && dist <= m_maxRange) {
            rangeFiltered->push_back(point);
        }
    }
    
    if (rangeFiltered->empty()) {
        return rangeFiltered;
    }
    
    pcl::VoxelGrid<pcl::PointXYZI> voxelFilter;
    voxelFilter.setInputCloud(rangeFiltered);
    voxelFilter.setLeafSize(m_voxelSize, m_voxelSize, m_voxelSize);
    
    auto downsampled = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    voxelFilter.filter(*downsampled);
    
    return downsampled;
}

pcl::PointCloud<pcl::PointXYZI>::Ptr DLOTracker::transformPointCloud(
    const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud,
    const Eigen::Matrix4f& transformation)
{
    auto transformed = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    pcl::transformPointCloud(*cloud, *transformed, transformation);
    return transformed;
}

pcl::PointCloud<pcl::PointXYZI>::Ptr DLOTracker::glmToPCL(const std::vector<glm::vec3>& points)
{
    auto cloud = pcl::make_shared<pcl::PointCloud<pcl::PointXYZI>>();
    cloud->reserve(points.size());
    
    for (const auto& p : points) {
        pcl::PointXYZI point;
        point.x = p.x;
        point.y = p.y;
        point.z = p.z;
        point.intensity = 1.0f;
        cloud->push_back(point);
    }
    
    return cloud;
}

glm::vec3 DLOTracker::pclToGlm(const pcl::PointXYZI& point)
{
    return glm::vec3(point.x, point.y, point.z);
}

glm::mat4 DLOTracker::eigenToGlm(const Eigen::Matrix4f& mat)
{
    glm::mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = mat(j, i);  // Transpose because Eigen is row-major, GLM is column-major
        }
    }
    return result;
}

Eigen::Matrix4f DLOTracker::glmToEigen(const glm::mat4& mat)
{
    Eigen::Matrix4f result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result(j, i) = mat[i][j];  // Transpose
        }
    }
    return result;
}

void DLOTracker::reset()
{
    m_previousCloud->clear();
    m_worldCloud->clear();
    
    m_currentPose.position = glm::vec3(0.0f);
    m_currentPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_currentPose.confidence = 0.0f;
    
    m_smoothedPosition = glm::vec3(0.0f);
    m_smoothedRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    m_imuPosition = glm::vec3(0.0f);
    m_imuOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_imuVelocity = glm::vec3(0.0f);
    m_lastICPTransform = Eigen::Matrix4f::Identity();
    m_hasICPResult = false;
    m_lastLidarTimestamp = 0.0;
    
    std::cout << "[DLO] Reset" << std::endl;
}

void DLOTracker::setFilterParams(float voxelSize, float maxRange)
{
    m_voxelSize = voxelSize;
    m_maxRange = maxRange;
}

void DLOTracker::setICPParams(int maxIterations, float fitnessThreshold)
{
    m_maxICPIterations = maxIterations;
    m_icpFitnessThreshold = fitnessThreshold;
}

} // namespace LensEngine
