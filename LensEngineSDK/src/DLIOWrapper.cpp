#include "DLIOWrapper.h"
#include "LensEngineTypes.h"
#include <iostream>
#include <iomanip>
#include <glm/gtc/quaternion.hpp>

namespace LensEngine {

DLIOWrapper::DLIOWrapper()
    : m_isInitialized(false)
{
    m_currentPose.position = glm::vec3(0.0f);
    m_currentPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_currentPose.confidence = 0.0f;
    m_currentPose.timestamp = 0;
}

DLIOWrapper::~DLIOWrapper()
{
}

bool DLIOWrapper::initialize(float voxelSize, int maxIterations)
{
    m_isInitialized = true;
    
    std::cout << "[DLIO] Initialized (standalone, no ROS) - voxel: " << voxelSize 
              << "m, GICP iter: " << maxIterations << std::endl;
    
    return true;
}

DLIOPose DLIOWrapper::update(const std::vector<glm::vec3>& points, const IMUData* imuData)
{
    // Конвертируем glm::vec3 в PCL формат
    auto pclCloud = glmToPCL(points);
    return update(pclCloud, imuData);
}

DLIOPose DLIOWrapper::update(const pcl::PointCloud<pcl::PointXYZI>::Ptr& pointCloud,
                             const IMUData* imuData)
{
    if (!m_isInitialized) {
        std::cout << "[DLIO] Not initialized!" << std::endl;
        return m_currentPose;
    }
    
    if (!pointCloud || pointCloud->empty()) {
        std::cout << "[DLIO] Empty point cloud!" << std::endl;
        return m_currentPose;
    }
    
    try {
        // Временное решение: используем простую интеграцию без DLIO node
        // так как оригинальный DLIO требует ROS
        
        // Для теста симулируем трекинг с небольшим шумом
        static int frameCount = 0;
        frameCount++;
        
        // Симулируем позицию (медленное движение)
        if (frameCount % 60 == 0) {
            m_currentPose.position.x += 0.01f;
            m_currentPose.position.y += 0.005f;
            m_currentPose.position.z += 0.001f;
            
            // Небольшое вращение (симулируем трекинг)
            glm::quat rotationDelta = glm::quat(
                1.0f, 0.001f, 0.001f, 0.001f
            );
            m_currentPose.rotation = rotationDelta * m_currentPose.rotation;
            m_currentPose.rotation = glm::normalize(m_currentPose.rotation);
            
            m_currentPose.confidence = 0.8f;
            
            std::cout << "[DLIO] Simulated pose update (frame " << frameCount << ")" << std::endl;
        } else {
            // Небольшое обновление позиции каждый кадр
            m_currentPose.position.x += 0.0001f;
            m_currentPose.position.y += 0.00005f;
            m_currentPose.position.z += 0.00001f;
            
            // Небольшое вращение каждый кадр
            glm::quat rotationDelta = glm::quat(
                1.0f, 0.0001f, 0.0001f, 0.0001f
            );
            m_currentPose.rotation = rotationDelta * m_currentPose.rotation;
            m_currentPose.rotation = glm::normalize(m_currentPose.rotation);
            
            m_currentPose.confidence = 0.7f;
        }
        
        m_currentPose.timestamp = imuData ? imuData->timestamp : 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[DLIO] Update failed: " << e.what() << std::endl;
        m_currentPose.confidence = 0.0f;
    }
    
    return m_currentPose;
}

pcl::PointCloud<pcl::PointXYZI>::Ptr DLIOWrapper::glmToPCL(const std::vector<glm::vec3>& points)
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

void DLIOWrapper::reset()
{
    m_currentPose.position = glm::vec3(0.0f);
    m_currentPose.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_currentPose.confidence = 0.0f;
    
    std::cout << "[DLIO] Reset" << std::endl;
}

void DLIOWrapper::feedIMU(const RawIMUData& imuData, double dt)
{
    // Отключено для ускорения теста
    /*
    // Для теста просто логируем IMU данные
    static int imuCount = 0;
    if (imuCount++ % 100 == 0) {
        std::cout << "[DLIO] IMU: accel=(" << imuData.accelX << ", " << imuData.accelY << ", " << imuData.accelZ 
                  << "), gyro=(" << imuData.gyroX << ", " << imuData.gyroY << ", " << imuData.gyroZ << ")" << std::endl;
    }
    */
}

} // namespace LensEngine
