#include "ARDataProcessor.h"
#include <algorithm>
#include <future>

#ifdef LENSENGINE_USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#endif

namespace LensEngine {

ARDataProcessor::ARDataProcessor()
    : m_cameraController(nullptr)
{
    m_sensorFusion = std::make_unique<SensorFusionEKF>();
    m_spatialMapping = std::make_unique<SpatialMappingSystem>();
    
    m_sensorFusion->initialize();
    m_spatialMapping->initialize();
    
    // Подключение колбэков от SensorFusion
    m_sensorFusion->setPoseCallback([this](const CameraPose& pose) {
        if (m_poseCallback) {
            m_poseCallback(pose);
        }
    });
}

ARDataProcessor::~ARDataProcessor()
{
    if (m_processingFuture.valid()) {
        m_processingFuture.wait();
    }
}

std::vector<FeaturePoint> ARDataProcessor::extractFeaturePoints(const RGBImage &rgbImage)
{
    return extractFeaturePointsFast(rgbImage);
}

CameraIntrinsics ARDataProcessor::estimateCameraIntrinsics(const RGBImage &rgbImage)
{
    CameraIntrinsics intrinsics;
    
    // Базовые параметры для iPhone камеры
    intrinsics.resolution = glm::vec2(static_cast<float>(rgbImage.width), static_cast<float>(rgbImage.height));
    intrinsics.focalLengthX = static_cast<float>(rgbImage.width) * 0.7f;
    intrinsics.focalLengthY = static_cast<float>(rgbImage.height) * 0.7f;
    intrinsics.principalPointX = static_cast<float>(rgbImage.width) * 0.5f;
    intrinsics.principalPointY = static_cast<float>(rgbImage.height) * 0.5f;
    
    // Матрица калибровки
    intrinsics.matrix = glm::mat3(
        intrinsics.focalLengthX, 0.0f, intrinsics.principalPointX,
        0.0f, intrinsics.focalLengthY, intrinsics.principalPointY,
        0.0f, 0.0f, 1.0f
    );
    
    intrinsics.isValid = true;
    return intrinsics;
}

LightEstimation ARDataProcessor::estimateLight(const RGBImage &rgbImage)
{
    return estimateLightFast(rgbImage);
}

CameraPose ARDataProcessor::processVisualOdometry(const RGBImage &currentFrame,
                                                   const RGBImage &previousFrame,
                                                   const std::vector<FeaturePoint> &previousFeatures)
{
    // TODO: Реализовать визуальную одометрию
    CameraPose pose;
    pose.confidence = 0.0f;
    return pose;
}

void ARDataProcessor::processIMUData(const RawIMUData &imuData)
{
    m_sensorFusion->updateIMU(imuData);
}

void ARDataProcessor::setCameraController(CameraController* controller)
{
    m_cameraController = controller;
}

void ARDataProcessor::processFrameAsync(const ARFrame &frame)
{
    if (m_processingFuture.valid() && m_processingFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        return; // Предыдущая обработка еще идет
    }
    
    m_processingFuture = std::async(std::launch::async, [this, frame]() {
        processFrameInternal(frame);
    });
}

void ARDataProcessor::updateSpatialMapping(const Lidar3DProcessor::SpatialAnalysisResult &analysis,
                                          const std::vector<glm::vec3> &points)
{
    if (m_spatialMapping) {
        m_spatialMapping->updateFromLiDAR(analysis, points);
    }
}

void ARDataProcessor::addVirtualObjectToScene(const glm::vec3 &position, const glm::vec3 &size)
{
    if (m_spatialMapping) {
        m_spatialMapping->addVirtualObject(position, size, true);
    }
}

void ARDataProcessor::updateVirtualObjectPosition(const std::string &id, const glm::vec3 &position)
{
    if (m_spatialMapping) {
        m_spatialMapping->updateVirtualObjectPosition(id, position);
    }
}

void ARDataProcessor::setFrameProcessedCallback(FrameProcessedCallback callback)
{
    m_frameProcessedCallback = callback;
}

void ARDataProcessor::setFeaturePointsCallback(FeaturePointsCallback callback)
{
    m_featurePointsCallback = callback;
}

void ARDataProcessor::setIntrinsicsCallback(IntrinsicsCallback callback)
{
    m_intrinsicsCallback = callback;
}

void ARDataProcessor::setLightCallback(LightCallback callback)
{
    m_lightCallback = callback;
}

void ARDataProcessor::setPoseCallback(PoseCallback callback)
{
    m_poseCallback = callback;
}

void ARDataProcessor::setSensorDataCallback(SensorDataCallback callback)
{
    m_sensorDataCallback = callback;
}

std::vector<FeaturePoint> ARDataProcessor::extractFeaturePointsFast(const RGBImage &rgbImage)
{
    std::vector<FeaturePoint> features;
    
#ifdef LENSENGINE_USE_OPENCV
    // Преобразуем RGBImage в cv::Mat
    cv::Mat image(rgbImage.height, rgbImage.width, CV_8UC3, const_cast<uint8_t*>(rgbImage.data.data()));
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);
    
    // Используем FAST детектор
    cv::Ptr<cv::FastFeatureDetector> detector = cv::FastFeatureDetector::create();
    std::vector<cv::KeyPoint> keypoints;
    detector->detect(gray, keypoints);
    
    for (const auto& kp : keypoints) {
        FeaturePoint fp;
        fp.screenPosition = glm::vec2(kp.pt.x, kp.pt.y);
        fp.confidence = 1.0f;
        fp.trackId = static_cast<uint64_t>(kp.class_id);
        // TODO: Преобразовать 2D в 3D позицию
        fp.position = glm::vec3(kp.pt.x / rgbImage.width, kp.pt.y / rgbImage.height, 1.0f);
        features.push_back(fp);
    }
#endif
    
    return features;
}

LightEstimation ARDataProcessor::estimateLightFast(const RGBImage &rgbImage)
{
    LightEstimation light;
    
    if (rgbImage.data.empty()) {
        return light;
    }
    
    // Простая оценка освещения на основе среднего значения яркости
    uint64_t sum = 0;
    size_t pixelCount = rgbImage.width * rgbImage.height;
    
    for (size_t i = 0; i < pixelCount * 3; i += 3) {
        // RGB -> Luminance
        uint8_t r = rgbImage.data[i];
        uint8_t g = rgbImage.data[i + 1];
        uint8_t b = rgbImage.data[i + 2];
        sum += static_cast<uint64_t>(0.299f * r + 0.587f * g + 0.114f * b);
    }
    
    float avgLuminance = static_cast<float>(sum) / pixelCount / 255.0f;
    light.ambientIntensity = avgLuminance;
    light.colorTemperature = 6500.0f; // Нейтральный белый
    light.ambientColor = glm::vec3(1.0f);
    light.primaryLightDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    light.isValid = true;
    
    return light;
}

void ARDataProcessor::processFrameInternal(const ARFrame &frame)
{
    ARFrame processedFrame = frame;

    // Всегда получаем актуальную позу от IMU
    processedFrame.cameraPose = m_sensorFusion->getCurrentPose();

    // Визуальный анализ (только если есть RGB)
    if (!frame.rgbImage.data.empty()) {
        processedFrame.featurePoints = extractFeaturePointsFast(frame.rgbImage);
        processedFrame.light = estimateLightFast(frame.rgbImage);
        processedFrame.intrinsics = estimateCameraIntrinsics(frame.rgbImage);
    }

    // Обновление Spatial Mapping (только если есть LiDAR точки)
    if (!frame.lidar.points3D.empty() && m_spatialMapping) {
        Lidar3DProcessor::SpatialAnalysisResult analysis;
        analysis.hasFloor = true;
        analysis.floorHeight = 0.0f;
        analysis.floorNormal = glm::vec3(0, 1, 0);

        m_spatialMapping->updateFromLiDAR(analysis, frame.lidar.points3D);

        // Помощь позиционированию от LiDAR в EKF
        if (m_sensorFusion) {
            m_sensorFusion->updateLidar(frame.lidar.points3D);
        }
    }

    // Вызываем колбэки
    if (m_frameProcessedCallback) {
        m_frameProcessedCallback(processedFrame);
    }

    if (!processedFrame.featurePoints.empty() && m_featurePointsCallback) {
        m_featurePointsCallback(processedFrame.featurePoints);
    }

    if (processedFrame.light.isValid && m_lightCallback) {
        m_lightCallback(processedFrame.light);
    }

    if (processedFrame.intrinsics.isValid && m_intrinsicsCallback) {
        m_intrinsicsCallback(processedFrame.intrinsics);
    }
}

} // namespace LensEngine

