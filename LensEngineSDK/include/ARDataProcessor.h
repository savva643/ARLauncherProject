#ifndef ARDATAPROCESSOR_H
#define ARDATAPROCESSOR_H

#include "LensEngineTypes.h"
#include "SensorFusionEKF.h"
#include "Lidar3DProcessor.h"
#include "CameraController.h"
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <string>

#include "SpatialMappingSystem.h"

namespace LensEngine {

/**
 * @brief Главный процессор AR данных
 * 
 * Обрабатывает все типы AR данных и координирует работу
 * различных компонентов движка.
 */
class ARDataProcessor {
public:
    ARDataProcessor();
    ~ARDataProcessor();

    // Обработка сырых данных
    std::vector<FeaturePoint> extractFeaturePoints(const RGBImage &rgbImage);
    CameraIntrinsics estimateCameraIntrinsics(const RGBImage &rgbImage);
    LightEstimation estimateLight(const RGBImage &rgbImage);
    CameraPose processVisualOdometry(const RGBImage &currentFrame,
                                     const RGBImage &previousFrame,
                                     const std::vector<FeaturePoint> &previousFeatures);

    // Обработка IMU
    void processIMUData(const RawIMUData &imuData);

    // Установка контроллера камеры
    void setCameraController(CameraController* controller);

    // Асинхронная обработка полного кадра
    void processFrameAsync(const ARFrame &frame);

    // Обновление Spatial Mapping
    void updateSpatialMapping(const Lidar3DProcessor::SpatialAnalysisResult &analysis,
                              const std::vector<glm::vec3> &points);
    void addVirtualObjectToScene(const glm::vec3 &position, const glm::vec3 &size);
    void updateVirtualObjectPosition(const std::string &id, const glm::vec3 &position);

    // Колбэки
    using FrameProcessedCallback = std::function<void(const ARFrame&)>;
    using FeaturePointsCallback = std::function<void(const std::vector<FeaturePoint>&)>;
    using IntrinsicsCallback = std::function<void(const CameraIntrinsics&)>;
    using LightCallback = std::function<void(const LightEstimation&)>;
    using PoseCallback = std::function<void(const CameraPose&)>;
    using SensorDataCallback = std::function<void(float, float, float, float, float, float)>;
    
    void setFrameProcessedCallback(FrameProcessedCallback callback);
    void setFeaturePointsCallback(FeaturePointsCallback callback);
    void setIntrinsicsCallback(IntrinsicsCallback callback);
    void setLightCallback(LightCallback callback);
    void setPoseCallback(PoseCallback callback);
    void setSensorDataCallback(SensorDataCallback callback);

private:
    std::unique_ptr<SensorFusionEKF> m_sensorFusion;
    std::vector<FeaturePoint> m_previousFeatures;
    RGBImage m_previousFrame;
    CameraPose m_previousPose;

    // Асинхронная обработка
    std::future<void> m_processingFuture;
    void processFrameInternal(const ARFrame &frame);

    std::unique_ptr<SpatialMappingSystem> m_spatialMapping;

    // Быстрые методы
    std::vector<FeaturePoint> extractFeaturePointsFast(const RGBImage &rgbImage);
    LightEstimation estimateLightFast(const RGBImage &rgbImage);

    CameraController* m_cameraController;
    
    // Колбэки
    FrameProcessedCallback m_frameProcessedCallback;
    FeaturePointsCallback m_featurePointsCallback;
    IntrinsicsCallback m_intrinsicsCallback;
    LightCallback m_lightCallback;
    PoseCallback m_poseCallback;
    SensorDataCallback m_sensorDataCallback;
};

} // namespace LensEngine

#endif // ARDATAPROCESSOR_H

