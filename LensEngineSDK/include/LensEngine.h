#ifndef LENSENGINE_H
#define LENSENGINE_H

#include "LensEngineAPI.h"
#include "LensEngineTypes.h"
#include "SensorFusionEKF.h"
#include "KalmanFilter6DOF.h"
#include "ARDataProcessor.h"
#include "Lidar3DProcessor.h"
#include "DLIOWrapper.h"
#include "CameraController.h"
#include "TemporalFilter.h"
#include "BundleAdjustment.h"
#include <memory>

#ifdef LENSENGINE_USE_OPENCV
#include "HandTracker.h"
#include "HandGestures.h"
#include "ObjectDetector.h"
#include "PhysicsEngine.h"
#include "SceneUnderstanding.h"
#endif

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
    
#ifdef LENSENGINE_USE_OPENCV
    // Hand tracking
    std::vector<Hand> getHands() const;
    std::vector<GestureResult> getGestures() const;
    
    // Object detection
    std::vector<DetectedObject> getDetectedObjects() const;
    
    // Scene understanding
    std::vector<SemanticSegment> getSemanticSegments() const;
    std::string getSceneType() const;
    
    // Physics
    PhysicsState getPhysicsState() const;
#endif

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
    std::unique_ptr<KalmanFilter6DOF> m_kalmanFilter;  // 6DOF tracking
    std::unique_ptr<ARDataProcessor> m_dataProcessor;
    std::unique_ptr<Lidar3DProcessor> m_lidarProcessor;
    std::unique_ptr<DLIOWrapper> m_dlioWrapper;  // Direct LiDAR Inertial Odometry
    std::unique_ptr<CameraController> m_cameraController;
    std::unique_ptr<TemporalFilter> m_temporalFilter;  // Постобработка для сглаживания
    std::unique_ptr<BundleAdjustment> m_bundleAdjustment;  // Глобальная оптимизация траектории
    
#ifdef LENSENGINE_USE_OPENCV
    std::unique_ptr<HandTracker> m_handTracker;
    std::unique_ptr<HandGestures> m_handGestures;
    std::unique_ptr<ObjectDetector> m_objectDetector;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;
    std::unique_ptr<SceneUnderstanding> m_sceneUnderstanding;
#endif

    // Данные
    mutable std::mutex m_dataMutex;
    CameraPose m_currentPose;
    std::vector<FeaturePoint> m_featurePoints;
    std::vector<glm::vec3> m_lidarPoints;
    CameraIntrinsics m_intrinsics;
    LightEstimation m_lightEstimation;
    RawIMUData m_lastIMUData;
    
#ifdef LENSENGINE_USE_OPENCV
    std::vector<Hand> m_hands;
    std::vector<DetectedObject> m_detectedObjects;
    std::vector<GestureResult> m_gestures;
    std::vector<SemanticSegment> m_semanticSegments;
    std::string m_sceneType;
    PhysicsState m_physicsState;
#endif

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

