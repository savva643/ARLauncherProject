#ifndef LENSENGINEAPI_H
#define LENSENGINEAPI_H

#include "LensEngineTypes.h"
#include <memory>
#include <functional>

namespace LensEngine {

// Forward declarations
class LensEngineCore;

/**
 * @brief Чистый C++ API для LensEngineSDK
 * 
 * Этот API предоставляет интерфейс для работы с AR движком
 * без зависимостей от Qt или других фреймворков.
 */
class LensEngineAPI {
public:
    LensEngineAPI();
    ~LensEngineAPI();
    
    // Запрет копирования
    LensEngineAPI(const LensEngineAPI&) = delete;
    LensEngineAPI& operator=(const LensEngineAPI&) = delete;
    
    // Инициализация и управление
    bool initialize();
    void shutdown();
    
    // Обработка данных
    void processRGBData(const uint8_t* data, size_t size, uint32_t width, uint32_t height, uint64_t timestamp);
    void processLidarData(const uint8_t* depthData, size_t depthSize, const uint8_t* confidenceData, size_t confidenceSize, uint64_t timestamp);
    void processIMUData(const RawIMUData& imuData);
    
    // Получение результатов
    CameraPose getCurrentCameraPose() const;
    std::vector<FeaturePoint> getFeaturePoints() const;
    std::vector<glm::vec3> getLidarPoints() const;
    CameraIntrinsics getCameraIntrinsics() const;
    LightEstimation getLightEstimation() const;
    
    // Колбэки для обработки результатов
    using PoseCallback = std::function<void(const CameraPose&)>;
    using FeaturePointsCallback = std::function<void(const std::vector<FeaturePoint>&)>;
    using LidarPointsCallback = std::function<void(const std::vector<glm::vec3>&)>;
    
    void setPoseCallback(PoseCallback callback);
    void setFeaturePointsCallback(FeaturePointsCallback callback);
    void setLidarPointsCallback(LidarPointsCallback callback);
    
    // Настройки
    void setNoiseParameters(double gyroNoise, double accelNoise, double visualNoise, double lidarNoise);
    void setCameraParameters(float focalLengthX, float focalLengthY, float principalPointX, float principalPointY);

private:
    std::unique_ptr<LensEngineCore> m_core;
    
    // Колбэки (для совместимости)
    PoseCallback m_poseCallback;
    FeaturePointsCallback m_featurePointsCallback;
    LidarPointsCallback m_lidarPointsCallback;
};

} // namespace LensEngine

#endif // LENSENGINEAPI_H

