#include "LensEngine.h"
#include "SpatialMappingSystem.h"
#include <algorithm>
#include <stdexcept>
#include <chrono>

#ifdef LENSENGINE_USE_OPENCV
#include "HandTracker.h"
#include "ObjectDetector.h"
#include "HandGestures.h"
#endif

namespace LensEngine {

LensEngineCore::LensEngineCore()
    : m_initialized(false)
{
    m_sensorFusion = std::make_unique<SensorFusionEKF>();
    m_kalmanFilter = std::make_unique<KalmanFilter6DOF>();  // 6DOF tracking
    m_dataProcessor = std::make_unique<ARDataProcessor>();
    m_lidarProcessor = std::make_unique<Lidar3DProcessor>();
    m_dlioWrapper = std::make_unique<DLIOWrapper>();
    m_cameraController = std::make_unique<CameraController>();
    m_temporalFilter = std::make_unique<TemporalFilter>();  // Постобработка для сглаживания
    m_bundleAdjustment = std::make_unique<BundleAdjustment>();  // Глобальная оптимизация траектории
    
#ifdef LENSENGINE_USE_OPENCV
    m_handTracker = std::make_unique<HandTracker>();
    m_handGestures = std::make_unique<HandGestures>();
    m_objectDetector = std::make_unique<ObjectDetector>();
    m_physicsEngine = std::make_unique<PhysicsEngine>();
    m_sceneUnderstanding = std::make_unique<SceneUnderstanding>();
#endif
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
    std::cout << "[LE] kalmanFilter init..." << std::endl;
    m_kalmanFilter->initialize();
    std::cout << "[LE] dlioWrapper init..." << std::endl;
    m_dlioWrapper->initialize(0.05f, 64);  // DLIO: 0.05m voxel, 64 GICP iterations
    std::cout << "[LE] temporalFilter init..." << std::endl;
    m_temporalFilter->reset();  // Инициализация TemporalFilter
    std::cout << "[LE] bundleAdjustment init..." << std::endl;
    m_bundleAdjustment->reset();  // Инициализация BundleAdjustment
    m_dataProcessor->setCameraController(m_cameraController.get());
    m_dataProcessor->setKalmanFilter(m_kalmanFilter.get());
    
#ifdef LENSENGINE_USE_OPENCV
    // LEGACY HandTracker не загружается: руки теперь в HandTrackingProvider (правильный разбор MediaPipe,
    // 3D по LiDAR, жесты), модели ищутся в models/ проекта.
    
    // Инициализация SceneUnderstanding
    std::cout << "[LE] sceneUnderstanding init..." << std::endl;
    m_sceneUnderstanding->initialize();
    
    // ObjectDetector отключен - YOLOv8 ONNX несовместим с OpenCV DNN (требует TensorFlow Lite)
    // Для полноценной работы нужна конвертация моделей через ONNX-TensorFlow или TensorFlow Lite
    // std::cout << "[LE] objectDetector init..." << std::endl;
    // m_objectDetector->initialize("", "", "");
#endif
    
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
    // Включаем все фичи для проверки: feature detection, light estimation, hand tracking
    RGBImage rgbImage;
    rgbImage.data.assign(data, data + size);
    rgbImage.width = width;
    rgbImage.height = height;
    rgbImage.timestamp = timestamp;
    
    ARFrame frame;
    frame.rgbImage = rgbImage;
    frame.timestamp = timestamp;
    
    frame.cameraPose = m_sensorFusion->getCurrentPose();
    
    // Feature detection и light estimation каждый 5-й кадр
    static int featureCounter = 0;
    if (++featureCounter % 5 == 0) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_featurePoints = m_dataProcessor->extractFeaturePoints(rgbImage);
        m_intrinsics = m_dataProcessor->estimateCameraIntrinsics(rgbImage);
        m_lightEstimation = m_dataProcessor->estimateLight(rgbImage);
    }
    
#ifdef LENSENGINE_USE_OPENCV
    // Hand tracking каждый 10-й кадр для оптимизации
    static int handCounter = 0;
    if (++handCounter % 10 == 0 && width <= 640 && height <= 480) {
        cv::Mat frameMat(height, width, CV_8UC3, const_cast<uint8_t*>(data));
        auto hands = m_handTracker->detectHands(frameMat);
        
        // Gesture recognition для каждой руки
        std::vector<GestureResult> gestures;
        for (const auto& hand : hands) {
            if (m_handGestures) {
                GestureResult gesture = m_handGestures->recognizeGesture(hand);
                gestures.push_back(gesture);
            }
        }
        
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_hands = hands;
        m_gestures = gestures;
    }
    
    // Scene understanding каждый 30-й кадр для оптимизации
    static int sceneCounter = 0;
    if (++sceneCounter % 30 == 0 && width <= 640 && height <= 480 && m_sceneUnderstanding) {
        cv::Mat frameMat(height, width, CV_8UC3, const_cast<uint8_t*>(data));
        auto segments = m_sceneUnderstanding->segmentScene(frameMat);
        auto sceneType = m_sceneUnderstanding->analyzeSceneType(frameMat);
        
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_semanticSegments = segments;
        m_sceneType = sceneType;
    }
    
    // Object detection отключен - YOLOv8 ONNX несовместим с OpenCV DNN
    // static int objectCounter = 0;
    // if (++objectCounter % 20 == 0 && width <= 640 && height <= 480 && m_objectDetector) {
    //     cv::Mat frameMat(height, width, CV_8UC3, const_cast<uint8_t*>(data));
    //     auto objects = m_objectDetector->detectObjects(frameMat);
    //     
    //     std::lock_guard<std::mutex> lock(m_dataMutex);
    //     m_detectedObjects = objects;
    // }
#endif
}

void LensEngineCore::processLidarData(const uint8_t* depthData, size_t depthSize, 
                                       const uint8_t* confidenceData, size_t confidenceSize, uint64_t timestamp)
{
    std::vector<uint8_t> depthVec(depthData, depthData + depthSize);
    std::vector<uint8_t> confidenceVec;
    if (confidenceData && confidenceSize > 0) {
        confidenceVec.assign(confidenceData, confidenceData + confidenceSize);
    }
    
    auto lidarPoints = m_lidarProcessor->processDepthDataFast(depthVec);
    
    // Обновляем m_lidarPoints для getLidarPoints()
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_lidarPoints = lidarPoints;
    }
    
    // Отключаем пространственный анализ для оптимизации производительности
    /*
    // Анализ пространственного окружения (стены, пол, препятствия)
    auto spatialAnalysis = m_lidarProcessor->getLastAnalysis();
    
    // Если обнаружен пол, используем его для коррекции гравитации
    if (spatialAnalysis.hasFloor) {
        // Можно использовать floorNormal для коррекции ориентации
        // Это улучшает точность трекинга на плоских поверхностях
    }
    */
    
 // Обновляем DLIOWrapper каждый кадр с Temporal Smoothing для уменьшения дрейфинга
    if (m_dlioWrapper && !lidarPoints.empty()) {
        IMUData imuData;
        imuData.acceleration = glm::vec3(m_lastIMUData.accelX, m_lastIMUData.accelY, m_lastIMUData.accelZ);
        imuData.angularVelocity = glm::vec3(m_lastIMUData.gyroX, m_lastIMUData.gyroY, m_lastIMUData.gyroZ);
        imuData.timestamp = timestamp;
        
        auto dlioPose = m_dlioWrapper->update(lidarPoints, &imuData);
        
        if (dlioPose.confidence > 0.3f) {
            // Включаем TemporalFilter для сглаживания позиции и вращения (уменьшает дрейфинг)
            glm::vec3 filteredPos = m_temporalFilter->filterPosition(dlioPose.position);
            glm::quat filteredRot = m_temporalFilter->filterRotation(dlioPose.rotation);
            
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_currentPose.position = filteredPos;
            m_currentPose.rotation = filteredRot;
            m_currentPose.confidence = dlioPose.confidence;
            m_currentPose.timestamp = dlioPose.timestamp;
            
            // Включаем BundleAdjustment для глобальной оптимизации (каждый 5-й кадр для точности)
            static int baFrameCounter = 0;
            if (++baFrameCounter % 5 == 0) {
                BundleAdjustment::KeyFrame kf;
                kf.frameId = static_cast<int>(m_currentPose.timestamp);
                kf.position = m_currentPose.position;
                kf.rotation = m_currentPose.rotation;
                kf.points3D = lidarPoints;
                m_bundleAdjustment->addKeyFrame(kf);
                
                // Оптимизируем если достаточно кадров
                if (m_bundleAdjustment->getKeyFrameCount() >= 3) {
                    m_bundleAdjustment->optimize();
                }
            }
        }
    }
}

void LensEngineCore::processIMUData(const RawIMUData& imuData)
{
    m_lastIMUData = imuData;
    
    if (m_dlioWrapper) {
        static auto lastImuTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastImuTime).count();
        lastImuTime = now;

        if (dt > 0.001 && dt < 0.1) {
            m_dlioWrapper->feedIMU(imuData, dt);
        }
    }
    
    // Отключаем KalmanFilter и SensorFusion для теста производительности
    // Они конфликтуют с DLIOWrapper при обновлении каждый 10-й кадр
    /*
    static auto lastKalmanTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - lastKalmanTime).count();
    lastKalmanTime = now;
    
    if (dt > 0.001 && dt < 0.1) {
        m_kalmanFilter->predictIMU(imuData, dt);
    }
    
    m_sensorFusion->updateIMU(imuData);
    */
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

#ifdef LENSENGINE_USE_OPENCV
std::vector<Hand> LensEngineCore::getHands() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_hands;
}

std::vector<DetectedObject> LensEngineCore::getDetectedObjects() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_detectedObjects;
}

std::vector<GestureResult> LensEngineCore::getGestures() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_gestures;
}

std::vector<SemanticSegment> LensEngineCore::getSemanticSegments() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_semanticSegments;
}

std::string LensEngineCore::getSceneType() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_sceneType;
}
#endif

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
    m_sensorFusion->setPoseCallback([this](const CameraPose& pose) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_currentPose.rotation = pose.rotation;
        m_currentPose.timestamp = pose.timestamp;
        
        if (m_poseCallback) {
            m_poseCallback(m_currentPose);
        }
    });
    
    m_lidarProcessor->setPointsCallback([this](const std::vector<glm::vec3>& points) {
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_lidarPoints = points;
        }

        if (!points.empty() && m_dlioWrapper) {
            DLIOPose dlioPose = m_dlioWrapper->update(points);

            if (dlioPose.confidence > 0.3f) {
                std::lock_guard<std::mutex> lock(m_dataMutex);
                m_currentPose.position = dlioPose.position;
                m_currentPose.rotation = dlioPose.rotation;
                m_currentPose.confidence = dlioPose.confidence;
            }
        }

        m_sensorFusion->updateLidar(points);

        if (m_lidarPointsCallback) {
            m_lidarPointsCallback(points);
        }
    });
    
    m_dataProcessor->setFeaturePointsCallback([this](const std::vector<FeaturePoint>& points) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_featurePoints = points;
        
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

