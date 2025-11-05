#include "LensEngineAPI.h"
#include "LensEngine.h"
#include <memory>

namespace LensEngine {

LensEngineAPI::LensEngineAPI()
    : m_core(std::make_unique<LensEngineCore>())
{
}

LensEngineAPI::~LensEngineAPI()
{
    shutdown();
}

bool LensEngineAPI::initialize()
{
    return m_core->initialize();
}

void LensEngineAPI::shutdown()
{
    m_core->shutdown();
}

void LensEngineAPI::processRGBData(const uint8_t* data, size_t size, uint32_t width, uint32_t height, uint64_t timestamp)
{
    m_core->processRGBData(data, size, width, height, timestamp);
}

void LensEngineAPI::processLidarData(const uint8_t* depthData, size_t depthSize, 
                                     const uint8_t* confidenceData, size_t confidenceSize, uint64_t timestamp)
{
    m_core->processLidarData(depthData, depthSize, confidenceData, confidenceSize, timestamp);
}

void LensEngineAPI::processIMUData(const RawIMUData& imuData)
{
    m_core->processIMUData(imuData);
}

CameraPose LensEngineAPI::getCurrentCameraPose() const
{
    return m_core->getCurrentCameraPose();
}

std::vector<FeaturePoint> LensEngineAPI::getFeaturePoints() const
{
    return m_core->getFeaturePoints();
}

std::vector<glm::vec3> LensEngineAPI::getLidarPoints() const
{
    return m_core->getLidarPoints();
}

CameraIntrinsics LensEngineAPI::getCameraIntrinsics() const
{
    return m_core->getCameraIntrinsics();
}

LightEstimation LensEngineAPI::getLightEstimation() const
{
    return m_core->getLightEstimation();
}

void LensEngineAPI::setPoseCallback(PoseCallback callback)
{
    m_poseCallback = callback;
    m_core->setPoseCallback(callback);
}

void LensEngineAPI::setFeaturePointsCallback(FeaturePointsCallback callback)
{
    m_featurePointsCallback = callback;
    m_core->setFeaturePointsCallback(callback);
}

void LensEngineAPI::setLidarPointsCallback(LidarPointsCallback callback)
{
    m_lidarPointsCallback = callback;
    m_core->setLidarPointsCallback(callback);
}

void LensEngineAPI::setNoiseParameters(double gyroNoise, double accelNoise, double visualNoise, double lidarNoise)
{
    m_core->setNoiseParameters(gyroNoise, accelNoise, visualNoise, lidarNoise);
}

void LensEngineAPI::setCameraParameters(float focalLengthX, float focalLengthY, float principalPointX, float principalPointY)
{
    m_core->setCameraParameters(focalLengthX, focalLengthY, principalPointX, principalPointY);
}

} // namespace LensEngine

