#include "ARDataProcessor.h"
#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>

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
    m_bundleAdjustment = std::make_unique<BundleAdjustment>();
#ifdef LENSENGINE_HAVE_NVISION
    m_featureDetector = std::make_unique<NVisionFeatureDetector>();
#endif
    m_lightEstimator = std::make_unique<SHLightEstimator>();
    
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
    // ПРАВИЛЬНАЯ визуальная одометрия: отслеживаем предыдущие feature points
    // через optical flow, а НЕ извлекаем новые точки на каждом кадре
    CameraPose pose;
    pose.confidence = 0.0f;
    pose.timestamp = currentFrame.timestamp;

    if (previousFrame.data.empty() || previousFeatures.empty()) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

#ifdef LENSENGINE_USE_OPENCV
    // Преобразуем предыдущие feature points в cv::Point2f
    std::vector<cv::Point2f> prevPoints;
    for (const auto& fp : previousFeatures) {
        prevPoints.push_back(cv::Point2f(fp.screenPosition.x, fp.screenPosition.y));
    }

    // Grayscale для обоих кадров
    cv::Mat prevImage(previousFrame.height, previousFrame.width, CV_8UC3,
                      const_cast<uint8_t*>(previousFrame.data.data()));
    cv::Mat currImage(currentFrame.height, currentFrame.width, CV_8UC3,
                      const_cast<uint8_t*>(currentFrame.data.data()));
    cv::Mat prevGray, currGray;
    cv::cvtColor(prevImage, prevGray, cv::COLOR_RGB2GRAY);
    cv::cvtColor(currImage, currGray, cv::COLOR_RGB2GRAY);

    // Optical Flow: отслеживаем ПРЕДЫДУЩИЕ точки на ТЕКУЩЕМ кадре
    std::vector<cv::Point2f> currPoints;
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(prevGray, currGray, prevPoints, currPoints, status, err,
                              cv::Size(21, 21), 3,
                              cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01));

    // Фильтруем только успешно отслеженные точки
    std::vector<cv::Point2f> goodPrevPoints, goodCurrPoints;
    for (size_t i = 0; i < status.size(); ++i) {
        if (status[i] && err[i] < 30.0f) {
            // Проверяем, что точка осталась внутри изображения
            float x = currPoints[i].x;
            float y = currPoints[i].y;
            if (x > 5 && x < currentFrame.width - 5 && y > 5 && y < currentFrame.height - 5) {
                goodPrevPoints.push_back(prevPoints[i]);
                goodCurrPoints.push_back(currPoints[i]);
            }
        }
    }

    if (goodPrevPoints.size() >= 8) {
        // Используем правильные intrinsics от iPhone калибровки
        cv::Mat cameraMatrix = (cv::Mat_<double>(3,3) <<
            m_fx, 0.0, m_cx,
            0.0, m_fy, m_cy,
            0.0, 0.0, 1.0);

        cv::Mat essentialMatrix = cv::findEssentialMat(
            goodPrevPoints, goodCurrPoints, cameraMatrix,
            cv::RANSAC, 0.999, 1.0
        );

        if (!essentialMatrix.empty()) {
            cv::Mat R, t;
            int inliers = cv::recoverPose(essentialMatrix, goodPrevPoints, goodCurrPoints, cameraMatrix, R, t);

            if (inliers > 0) {
                // Преобразуем rotation
                glm::mat3 rotationMat;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        rotationMat[i][j] = static_cast<float>(R.at<double>(i, j));
                pose.rotation = glm::quat_cast(rotationMat);

                // Translation из recoverPose имеет масштаб ~1.0 (unit translation)
                // Масштабируем на основе baseline и focal length
                // baseline = distance between frames (estimated from optical flow magnitude)
                // scale = baseline / (t_magnitude * focal_length)
                
                // 🔹 ПРАВИЛЬНОЕ ПРЕОБРАЗОВАНИЕ КООРДИНАТ
                // OpenCV camera frame: X=right, Y=down, Z=forward
                // World frame (ARKit): X=right, Y=up, Z=backward
                // Преобразование:
                //   World.X = OpenCV.X (right остаётся right)
                //   World.Y = -OpenCV.Y (down становится up)
                //   World.Z = -OpenCV.Z (forward становится backward)
                glm::vec3 translation(
                    static_cast<float>(t.at<double>(0)),      // X: right
                    -static_cast<float>(t.at<double>(1)),     // Y: invert (down -> up)
                    -static_cast<float>(t.at<double>(2))      // Z: invert (forward -> backward)
                );
                
                // 🔹 УЛУЧШЕННАЯ ОЦЕНКА МАСШТАБА (как в ARKit)
                // Используем несколько методов для более надёжной оценки
                
                float avgFlow = 0.0f;
                float medianFlow = 0.0f;
                std::vector<float> flows;
                
                for (size_t i = 0; i < goodPrevPoints.size(); ++i) {
                    float dx = goodCurrPoints[i].x - goodPrevPoints[i].x;
                    float dy = goodCurrPoints[i].y - goodPrevPoints[i].y;
                    float flow = std::sqrt(dx*dx + dy*dy);
                    flows.push_back(flow);
                    avgFlow += flow;
                }
                avgFlow /= goodPrevPoints.size();
                
                // Медианный flow (более робастный к выбросам)
                if (!flows.empty()) {
                    std::nth_element(flows.begin(), flows.begin() + flows.size()/2, flows.end());
                    medianFlow = flows[flows.size() / 2];
                }
                
                float tMag = glm::length(translation);
                
                // 🔹 ПРОСТАЯ ОЦЕНКА МАСШТАБА (Kalman filter будет фильтровать)
                // Используем медианный flow для оценки масштаба
                float avgDepth = 1.0f; // Значение по умолчанию
                
                // Масштаб = глубина / focal_length * optical_flow
                float scale = (medianFlow / static_cast<float>(m_fx)) * avgDepth;
                
                // Ограничиваем масштаб
                scale = glm::clamp(scale, 0.001f, 0.3f);
                
                glm::vec3 relativeTranslation = translation * scale;

                // Outlier rejection: if delta > 50cm per frame, skip
                if (glm::length(relativeTranslation) > 0.5f) {
                    pose.confidence = 0.0f;
                } else {
                    // Интегрируем относительное смещение
                    glm::vec3 currentIMUPos = m_sensorFusion->getCurrentPose().position;
                    pose.position = currentIMUPos + relativeTranslation;

                    // Уверенность: 8 точек = 0.27, 15 = 0.5, 30 = 1.0
                    pose.confidence = std::min(1.0f, static_cast<float>(goodPrevPoints.size()) / 30.0f);
                }

                // Сохраняем tracked points для следующего кадра (continuity!)
                m_trackedFeatures.clear();
                for (size_t i = 0; i < goodCurrPoints.size(); ++i) {
                    FeaturePoint fp;
                    fp.screenPosition = glm::vec2(goodCurrPoints[i].x, goodCurrPoints[i].y);
                    fp.confidence = 1.0f;
                    m_trackedFeatures.push_back(fp);
                }
            }
        }
    }

    if (pose.confidence <= 0.0f) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
    }
#else
    pose.position = m_sensorFusion->getCurrentPose().position;
    pose.rotation = m_sensorFusion->getCurrentPose().rotation;
#endif

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

void ARDataProcessor::setKalmanFilter(KalmanFilter6DOF* kalmanFilter)
{
    m_kalmanFilter = kalmanFilter;
}

void ARDataProcessor::setCameraIntrinsics(double fx, double fy, double cx, double cy)
{
    m_fx = fx;
    m_fy = fy;
    m_cx = cx;
    m_cy = cy;
    std::cout << "[INTRINSICS] Set from iPhone: fx=" << fx << " fy=" << fy 
              << " cx=" << cx << " cy=" << cy << std::endl;
}

void ARDataProcessor::processFrameAsync(const ARFrame &frame)
{
    // НЕ пропускаем кадры! Добавляем в очередь
    // Это предотвращает отставание и потерю кадров
    {
        std::lock_guard<std::mutex> lock(m_frameQueueMutex);
        m_frameQueue.push(frame);
    }
    
    // Если обработчик не работает - запускаем его
    if (!m_processingFuture.valid() || m_processingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        m_processingFuture = std::async(std::launch::async, [this]() {
            while (true) {
                ARFrame currentFrame;
                {
                    std::lock_guard<std::mutex> lock(m_frameQueueMutex);
                    if (m_frameQueue.empty()) {
                        break;  // Очередь пуста, выходим
                    }
                    currentFrame = m_frameQueue.front();
                    m_frameQueue.pop();
                }
                processFrameInternal(currentFrame);
            }
        });
    }
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

    // Сначала пробуем NVisionFeatureDetector (FAST - header-only, быстро)
    if (m_featureDetector) {
        features = m_featureDetector->detectFeatures(rgbImage, 500);
        
        // Добавляем trackId для отслеживания
        for (size_t i = 0; i < features.size(); ++i) {
            features[i].trackId = static_cast<uint64_t>(i);
            features[i].position = glm::vec3(
                features[i].screenPosition.x / rgbImage.width,
                features[i].screenPosition.y / rgbImage.height,
                1.0f
            );
        }
        
        return features;
    }

#ifdef LENSENGINE_USE_OPENCV
    // Fallback на OpenCV если NVision недоступен
    cv::Mat image(rgbImage.height, rgbImage.width, CV_8UC3, const_cast<uint8_t*>(rgbImage.data.data()));
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);

    // goodFeaturesToTrack дает "хорошие" углы, которые лучше отслеживаются optical flow
    std::vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(gray, corners, 100, 0.01, 10.0);

    for (size_t i = 0; i < corners.size(); ++i) {
        FeaturePoint fp;
        fp.screenPosition = glm::vec2(corners[i].x, corners[i].y);
        fp.confidence = 1.0f;
        fp.trackId = static_cast<uint64_t>(i);
        fp.position = glm::vec3(corners[i].x / rgbImage.width, corners[i].y / rgbImage.height, 1.0f);
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

    // Сначала пробуем SHLightEstimator (spherical-harmonic - более точно)
    if (m_lightEstimator) {
        m_lightEstimator->updateFromRGBImage(rgbImage, m_fx, m_fy, m_cx, m_cy);
        if (m_lightEstimator->hasValidEstimate()) {
            return m_lightEstimator->toLightEstimation();
        }
    }
    
    // Fallback на простую оценку освещения
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

    // Всегда получаем актуальную позу от Kalman Filter (не от IMU!)
    // Kalman Filter объединяет IMU + VO и дает более точную позу
    if (m_kalmanFilter) {
        processedFrame.cameraPose = m_kalmanFilter->getCurrentPose();
    } else {
        processedFrame.cameraPose = m_sensorFusion->getCurrentPose();
    }

    // Визуальный анализ (только если есть RGB)
    if (!frame.rgbImage.data.empty()) {
        processedFrame.featurePoints = extractFeaturePointsFast(frame.rgbImage);
        processedFrame.light = estimateLightFast(frame.rgbImage);
        processedFrame.intrinsics = estimateCameraIntrinsics(frame.rgbImage);
        
        // APPLE VISION PRO / ARKit СТИЛЬ: Визуальная одометрия с триангуляцией
        if (!m_previousFrame.data.empty() && !m_previousFeatures.empty()) {
            // Используем улучшенный метод с триангуляцией и SolvePnPRansac
            CameraPose visualPose = processVisualOdometryFixed(
                frame.rgbImage,
                m_previousFrame,
                m_previousFeatures
            );
            
            // Если визуальная одометрия дала хороший результат, используем её для коррекции
            static int voLogCounter = 0;
            if (voLogCounter++ % 30 == 0) {
                std::cout << "[VO] Features: " << m_previousFeatures.size()
                          << " Confidence: " << std::fixed << std::setprecision(2) << visualPose.confidence
                          << std::endl;
            }
            if (visualPose.confidence > 0.15f) {
                // === BUNDLE ADJUSTMENT ===
                // Добавляем ключевой кадр для оптимизации траектории
                if (m_bundleAdjustment) {
                    BundleAdjustment::KeyFrame kf;
                    kf.frameId = m_frameCount++;
                    kf.position = visualPose.position;
                    kf.rotation = visualPose.rotation;
                    kf.fx = m_fx;
                    kf.fy = m_fy;
                    kf.cx = m_cx;
                    kf.cy = m_cy;
                    
                    // Добавляем feature points как 3D точки (упрощенно)
                    for (const auto& fp : m_previousFeatures) {
                        kf.points3D.push_back(fp.position);
                        kf.points2D.push_back(glm::vec2(fp.screenPosition.x, fp.screenPosition.y));
                        kf.pointIds.push_back(fp.trackId);
                    }
                    
                    m_bundleAdjustment->addKeyFrame(kf);
                    
                    // Оптимизируем каждые 10 кадров
                    if (m_frameCount % 10 == 0) {
                        m_bundleAdjustment->optimize();
                        
                        // Получаем оптимизированные позы
                        auto optimizedPositions = m_bundleAdjustment->getOptimizedPositions();
                        auto optimizedRotations = m_bundleAdjustment->getOptimizedRotations();
                        
                        if (!optimizedPositions.empty()) {
                            // Используем последнюю оптимизированную позу
                            visualPose.position = optimizedPositions.back();
                            visualPose.rotation = optimizedRotations.back();
                        }
                    }
                }
                
                // Обновляем Kalman Filter с визуальной одометрией
                if (m_kalmanFilter) {
                    m_kalmanFilter->updateVO(visualPose.position, visualPose.rotation, visualPose.confidence);
                    processedFrame.cameraPose = m_kalmanFilter->getCurrentPose();
                } else {
                    // Fallback: используем sensor fusion если Kalman filter не установлен
                    m_sensorFusion->updateVisualOdometry(visualPose.position, visualPose.rotation);
                    processedFrame.cameraPose = m_sensorFusion->getCurrentPose();
                }
            }
        }
        
        // Сохраняем текущий кадр и tracked feature points для следующей итерации
        m_previousFrame = frame.rgbImage;
        // Если optical flow дал tracked points — используем их, иначе freshly detected
        if (!m_trackedFeatures.empty()) {
            m_previousFeatures = m_trackedFeatures;
        } else {
            m_previousFeatures = processedFrame.featurePoints;
        }
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

