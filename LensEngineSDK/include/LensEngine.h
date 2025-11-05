#ifndef LENSENGINE_H
#define LENSENGINE_H

#include "LensEngineAPI.h"
#include "LensEngineTypes.h"
#include "SensorFusionEKF.h"
#include "ARDataProcessor.h"
#include "Lidar3DProcessor.h"
#include "CameraController.h"
#include <memory>
#include <mutex>
#include <functional>

namespace LensEngine {

/**
 * @brief Основной класс LensEngine
 * 
 * Координирует работу всех компонентов AR движка.
 */
class LensEngineCore {
public:
    LensEngineCore();
    ~LensEngineCore();

    bool initialize();
    void shutdown();

    // Обработка данных
    void processRGBData(const uint8_t* data, size_t size, uint32_t width, uint32_t height, uint64_t timestamp);
    void processLidarData(const uint8_t* depthData, size_t depthSize, 
                         const uint8_t* confidenceData, size_t confidenceSize, uint64_t timestamp);
    void processIMUData(const RawIMUData& imuData);

    // Получение результатов
    CameraPose getCurrentCameraPose() const;
    std::vector<FeaturePoint> getFeaturePoints() const;
    std::vector<glm::vec3> getLidarPoints() const;
    CameraIntrinsics getCameraIntrinsics() const;
    LightEstimation getLightEstimation() const;

    // Настройки
    void setNoiseParameters(double gyroNoise, double accelNoise, double visualNoise, double lidarNoise);
    void setCameraParameters(float focalLengthX, float focalLengthY, float principalPointX, float principalPointY);
    
    // Установка колбэков
    void setPoseCallback(std::function<void(const CameraPose&)> callback);
    void setFeaturePointsCallback(std::function<void(const std::vector<FeaturePoint>&)> callback);
    void setLidarPointsCallback(std::function<void(const std::vector<glm::vec3>&)> callback);

private:
    // Компоненты
    std::unique_ptr<SensorFusionEKF> m_sensorFusion;
    std::unique_ptr<ARDataProcessor> m_dataProcessor;
    std::unique_ptr<Lidar3DProcessor> m_lidarProcessor;
    std::unique_ptr<CameraController> m_cameraController;

    // Данные
    mutable std::mutex m_dataMutex;
    CameraPose m_currentPose;
    std::vector<FeaturePoint> m_featurePoints;
    std::vector<glm::vec3> m_lidarPoints;
    CameraIntrinsics m_intrinsics;
    LightEstimation m_lightEstimation;

    // Состояние
    bool m_initialized;
    
    // Колбэки
    std::function<void(const CameraPose&)> m_poseCallback;
    std::function<void(const std::vector<FeaturePoint>&)> m_featurePointsCallback;
    std::function<void(const std::vector<glm::vec3>&)> m_lidarPointsCallback;
    
    // Внутренние методы
    void setupCallbacks();
};

} // namespace LensEngine

#endif // LENSENGINE_H

