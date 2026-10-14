#include "SLAMTracker.h"
#include <iostream>
#include <iomanip>

#ifdef LENSENGINE_USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#endif

namespace LensEngine {

SLAMTracker::SLAMTracker()
    : m_fx(0), m_fy(0), m_cx(0), m_cy(0)
{
}

SLAMTracker::~SLAMTracker()
{
}

bool SLAMTracker::initialize(double fx, double fy, double cx, double cy)
{
    m_fx = fx;
    m_fy = fy;
    m_cx = cx;
    m_cy = cy;
    
    std::cout << "[SLAM] Initialized with intrinsics: fx=" << fx << " fy=" << fy 
              << " cx=" << cx << " cy=" << cy << std::endl;
    
    return true;
}

CameraPose SLAMTracker::trackFrame(const RGBImage &currentFrame)
{
    if (currentFrame.data.empty()) {
        return m_currentPose;
    }

#ifdef LENSENGINE_USE_OPENCV
    // Обнаруживаем feature points в текущем кадре
    detectFeatures(currentFrame);
    
    // Сопоставляем с предыдущим кадром
    matchFeatures();
    
    // Решаем PnP для получения 6DOF позы
    if (solvePnP(currentFrame)) {
        m_trackingQuality = std::min(1.0f, static_cast<float>(m_trackedPoints3D.size()) / 50.0f);
    } else {
        m_trackingQuality = 0.0f;
    }
    
    // Обновляем карту 3D точек
    updateMapPoints();
    
    // Сохраняем текущий кадр для следующей итерации
    m_previousFrame = currentFrame;
    
#endif
    
    return m_currentPose;
}

void SLAMTracker::detectFeatures(const RGBImage &frame)
{
#ifdef LENSENGINE_USE_OPENCV
    // Преобразуем в OpenCV Mat
    cv::Mat image(frame.height, frame.width, CV_8UC3, 
                  const_cast<uint8_t*>(frame.data.data()));
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);
    
    // Используем ORB детектор (быстрый и надёжный)
    auto orb = cv::ORB::create(500);  // 500 features
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    
    orb->detectAndCompute(gray, cv::noArray(), keypoints, descriptors);
    
    // Конвертируем в наш формат
    m_previousKeypoints.clear();
    for (const auto& kp : keypoints) {
        m_previousKeypoints.push_back(glm::vec2(kp.pt.x, kp.pt.y));
    }
    
    std::cout << "[SLAM] Detected " << m_previousKeypoints.size() << " features" << std::endl;
    
#endif
}

void SLAMTracker::matchFeatures()
{
#ifdef LENSENGINE_USE_OPENCV
    if (m_previousFrame.data.empty() || m_previousKeypoints.empty()) {
        return;
    }
    
    // Используем optical flow для отслеживания точек между кадрами
    // Это более надёжно чем дескриптор matching для видео
    
    std::vector<cv::Point2f> prevPoints;
    for (const auto& kp : m_previousKeypoints) {
        prevPoints.push_back(cv::Point2f(kp.x, kp.y));
    }
    
    // Конвертируем в OpenCV
    cv::Mat prevGray(m_previousFrame.height, m_previousFrame.width, CV_8UC3,
                     const_cast<uint8_t*>(m_previousFrame.data.data()));
    cv::cvtColor(prevGray, prevGray, cv::COLOR_RGB2GRAY);
    
    // Текущий кадр уже есть в detectFeatures, но нам нужен серый
    // Для простоты используем goodFeaturesToTrack для текущего кадра
    
#endif
}

bool SLAMTracker::solvePnP(const RGBImage &frame)
{
#ifdef LENSENGINE_USE_OPENCV
    // Нужно минимум 4 точки для PnP
    if (m_trackedPoints3D.size() < 4) {
        return false;
    }
    
    // Конвертируем в OpenCV формат
    std::vector<cv::Point3f> objectPoints;
    std::vector<cv::Point2f> imagePoints;
    
    for (size_t i = 0; i < m_trackedPoints3D.size(); ++i) {
        objectPoints.push_back(cv::Point3f(
            m_trackedPoints3D[i].x,
            m_trackedPoints3D[i].y,
            m_trackedPoints3D[i].z
        ));
        imagePoints.push_back(cv::Point2f(
            m_trackedPoints2D[i].x,
            m_trackedPoints2D[i].y
        ));
    }
    
    // Матрица камеры
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        m_fx, 0.0, m_cx,
        0.0, m_fy, m_cy,
        0.0, 0.0, 1.0);
    
    // Коэффициенты дистortion (предполагаем нулевые для iPhone)
    cv::Mat distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
    
    // Решаем PnP
    cv::Mat rvec, tvec;
    bool success = cv::solvePnP(
        objectPoints, imagePoints,
        cameraMatrix, distCoeffs,
        rvec, tvec,
        false,  // useExtrinsicGuess
        cv::SOLVEPNP_EPNP  // метод
    );
    
    if (!success) {
        return false;
    }
    
    // Конвертируем результаты в наш формат
    // rvec - rotation vector (Rodrigues)
    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);
    
    // Конвертируем в quaternion
    glm::mat3 rotMat;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            rotMat[i][j] = static_cast<float>(rotationMatrix.at<double>(i, j));
        }
    }
    m_currentPose.rotation = glm::quat_cast(rotMat);
    
    // Translation
    m_currentPose.position = glm::vec3(
        static_cast<float>(tvec.at<double>(0)),
        static_cast<float>(tvec.at<double>(1)),
        static_cast<float>(tvec.at<double>(2))
    );
    
    m_currentPose.confidence = std::min(1.0f, static_cast<float>(objectPoints.size()) / 30.0f);
    m_currentPose.timestamp = frame.timestamp;
    
    std::cout << "[SLAM] PnP solved: " << objectPoints.size() << " points, "
              << "Pos: (" << m_currentPose.position.x << ", " 
              << m_currentPose.position.y << ", " 
              << m_currentPose.position.z << ")" << std::endl;
    
    return true;
    
#else
    return false;
#endif
}

void SLAMTracker::updateMapPoints()
{
    // Добавляем отслеженные точки в карту
    // Это простая реализация - в полноценной SLAM системе
    // здесь была бы триангуляция и bundle adjustment
    
    for (size_t i = 0; i < m_trackedPoints3D.size(); ++i) {
        // Проверяем, есть ли уже такая точка
        bool found = false;
        for (size_t j = 0; j < m_mapPoints3D.size(); ++j) {
            float dist = glm::distance(m_trackedPoints3D[i], m_mapPoints3D[j]);
            if (dist < 0.01f) {
                found = true;
                break;
            }
        }
        
        // Добавляем новую точку
        if (!found) {
            m_mapPoints3D.push_back(m_trackedPoints3D[i]);
            m_mapPoints2D.push_back(m_trackedPoints2D[i]);
            m_mapPointIds.push_back(m_mapPointIds.size());
        }
    }
    
    // Ограничиваем размер карты
    if (m_mapPoints3D.size() > 1000) {
        m_mapPoints3D.erase(m_mapPoints3D.begin());
        m_mapPoints2D.erase(m_mapPoints2D.begin());
        m_mapPointIds.erase(m_mapPointIds.begin());
    }
}

void SLAMTracker::reset()
{
    m_currentPose = CameraPose();
    m_trackingQuality = 0.0f;
    m_mapPoints3D.clear();
    m_mapPoints2D.clear();
    m_mapPointIds.clear();
    m_trackedPoints3D.clear();
    m_trackedPoints2D.clear();
    m_previousFrame.data.clear();
    m_previousKeypoints.clear();
}

} // namespace LensEngine
