#include "LensEngine.h"
#include "SpatialMappingSystem.h"
#include <algorithm>
#include <stdexcept>

namespace LensEngine {

LensEngineCore::LensEngineCore()
    : m_initialized(false)
{
    m_sensorFusion = std::make_unique<SensorFusionEKF>();
    m_dataProcessor = std::make_unique<ARDataProcessor>();
    m_lidarProcessor = std::make_unique<Lidar3DProcessor>();
    m_cameraController = std::make_unique<CameraController>();
}

LensEngineCore::~LensEngineCore()
{
    shutdown();
}

bool LensEngineCore::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    m_sensorFusion->initialize();
    m_dataProcessor->setCameraController(m_cameraController.get());
    
    setupCallbacks();
    
    m_initialized = true;
    return true;
}

void LensEngineCore::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    m_initialized = false;
}

void LensEngineCore::processRGBData(const uint8_t* data, size_t size, uint32_t width, uint32_t height, uint64_t timestamp)
{
    RGBImage rgbImage;
    rgbImage.data.assign(data, data + size);
    rgbImage.width = width;
    rgbImage.height = height;
    rgbImage.timestamp = timestamp;
    
    // Обработка через ARDataProcessor
    ARFrame frame;
    frame.rgbImage = rgbImage;
    frame.timestamp = timestamp;
    
    // Получаем текущую позу от IMU
    frame.cameraPose = m_sensorFusion->getCurrentPose();
    
    // Обрабатываем кадр асинхронно
    m_dataProcessor->processFrameAsync(frame);
    
    // Обновляем данные
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_featurePoints = m_dataProcessor->extractFeaturePoints(rgbImage);
    m_intrinsics = m_dataProcessor->estimateCameraIntrinsics(rgbImage);
    m_lightEstimation = m_dataProcessor->estimateLight(rgbImage);
}

void LensEngineCore::processLidarData(const uint8_t* depthData, size_t depthSize, 
                                       const uint8_t* confidenceData, size_t confidenceSize, uint64_t timestamp)
{
    std::vector<uint8_t> depthVec(depthData, depthData + depthSize);
    std::vector<uint8_t> confidenceVec(confidenceData, confidenceData + confidenceSize);
    
    m_lidarProcessor->processLidarDataAsync(depthVec, confidenceVec);
}

void LensEngineCore::processIMUData(const RawIMUData& imuData)
{
    m_sensorFusion->updateIMU(imuData);
}

CameraPose LensEngineCore::getCurrentCameraPose() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_currentPose;
}

std::vector<FeaturePoint> LensEngineCore::getFeaturePoints() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_featurePoints;
}

std::vector<glm::vec3> LensEngineCore::getLidarPoints() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_lidarPoints;
}

CameraIntrinsics LensEngineCore::getCameraIntrinsics() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_intrinsics;
}

LightEstimation LensEngineCore::getLightEstimation() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_lightEstimation;
}

void LensEngineCore::setNoiseParameters(double gyroNoise, double accelNoise, double visualNoise, double lidarNoise)
{
    m_sensorFusion->setNoiseParameters(gyroNoise, accelNoise, 0.001, 0.01, visualNoise, lidarNoise);
}

void LensEngineCore::setCameraParameters(float focalLengthX, float focalLengthY, float principalPointX, float principalPointY)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_intrinsics.focalLengthX = focalLengthX;
    m_intrinsics.focalLengthY = focalLengthY;
    m_intrinsics.principalPointX = principalPointX;
    m_intrinsics.principalPointY = principalPointY;
    m_intrinsics.isValid = true;
}

void LensEngineCore::setupCallbacks()
{
    // Настройка колбэков для обновления данных
    m_sensorFusion->setPoseCallback([this](const CameraPose& pose) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_currentPose = pose;
        
        // Обновляем контроллер камеры
        if (m_cameraController) {
            m_cameraController->updateFromPose(pose);
        }
        
        // Вызываем внешний колбэк
        if (m_poseCallback) {
            m_poseCallback(pose);
        }
    });
    
    m_lidarProcessor->setPointsCallback([this](const std::vector<glm::vec3>& points) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_lidarPoints = points;
        
        // Вызываем внешний колбэк
        if (m_lidarPointsCallback) {
            m_lidarPointsCallback(points);
        }
    });
    
    // Настройка колбэков от ARDataProcessor
    m_dataProcessor->setFeaturePointsCallback([this](const std::vector<FeaturePoint>& points) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_featurePoints = points;
        
        // Вызываем внешний колбэк
        if (m_featurePointsCallback) {
            m_featurePointsCallback(points);
        }
    });
    
    m_dataProcessor->setIntrinsicsCallback([this](const CameraIntrinsics& intrinsics) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_intrinsics = intrinsics;
    });
    
    m_dataProcessor->setLightCallback([this](const LightEstimation& light) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_lightEstimation = light;
    });
}

void LensEngineCore::setPoseCallback(std::function<void(const CameraPose&)> callback)
{
    m_poseCallback = callback;
}

void LensEngineCore::setFeaturePointsCallback(std::function<void(const std::vector<FeaturePoint>&)> callback)
{
    m_featurePointsCallback = callback;
}

void LensEngineCore::setLidarPointsCallback(std::function<void(const std::vector<glm::vec3>&)> callback)
{
    m_lidarPointsCallback = callback;
}

} // namespace LensEngine

