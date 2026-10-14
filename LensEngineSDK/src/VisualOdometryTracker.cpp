#include "ARDataProcessor.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

#ifdef LENSENGINE_USE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace LensEngine {

/**
 * @brief Правильная визуальная одометрия с триангуляцией
 * 
 * Этот метод:
 * 1. Обнаруживает feature points в текущем кадре
 * 2. Отслеживает их через optical flow
 * 3. Вычисляет essential matrix
 * 4. Восстанавливает позу (R, t)
 * 5. Триангулирует 3D точки
 * 6. Использует SolvePnPRansac для следующего кадра
 */

CameraPose ARDataProcessor::processVisualOdometryFixed(
    const RGBImage &currentFrame,
    const RGBImage &previousFrame,
    const std::vector<FeaturePoint> &previousFeatures)
{
    CameraPose pose;
    pose.confidence = 0.0f;
    pose.timestamp = currentFrame.timestamp;

    if (previousFrame.data.empty() || previousFeatures.empty()) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

#ifdef LENSENGINE_USE_OPENCV
    // === ШАГИ 1-2: Feature Detection & Tracking ===
    
    // Преобразуем в OpenCV Mat (grayscale)
    // Используем ROI для ускорения (центральная часть изображения)
    cv::Mat prevImage(previousFrame.height, previousFrame.width, CV_8UC3,
                      const_cast<uint8_t*>(previousFrame.data.data()));
    cv::Mat currImage(currentFrame.height, currentFrame.width, CV_8UC3,
                      const_cast<uint8_t*>(currentFrame.data.data()));
    
    // Уменьшаем размер для ускорения обработки
    cv::Mat prevSmall, currSmall, prevGray, currGray;
    cv::resize(prevImage, prevSmall, cv::Size(prevImage.cols/2, prevImage.rows/2));
    cv::resize(currImage, currSmall, cv::Size(currImage.cols/2, currImage.rows/2));
    cv::cvtColor(prevSmall, prevGray, cv::COLOR_RGB2GRAY);
    cv::cvtColor(currSmall, currGray, cv::COLOR_RGB2GRAY);

    // Конвертируем previous features в cv::Point2f (масштабируем на 0.5)
    std::vector<cv::Point2f> prevPoints;
    for (const auto& fp : previousFeatures) {
        prevPoints.push_back(cv::Point2f(fp.screenPosition.x * 0.5f, fp.screenPosition.y * 0.5f));
    }

    // === OPTICAL FLOW: отслеживаем точки (оптимизированный) ===
    std::vector<cv::Point2f> currPoints;
    std::vector<uchar> status;
    std::vector<float> err;
    
    // Используем более быстрые параметры
    cv::calcOpticalFlowPyrLK(prevGray, currGray, prevPoints, currPoints, status, err,
                              cv::Size(15, 15), 2,  // меньше окно и уровней
                              cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 20, 0.01));

    // === Фильтруем хорошие отслеженные точки ===
    std::vector<cv::Point2f> goodPrevPoints, goodCurrPoints;
    std::vector<int> goodIndices;
    
    int smallWidth = currSmall.cols;
    int smallHeight = currSmall.rows;
    
    for (size_t i = 0; i < status.size(); ++i) {
        if (status[i] && err[i] < 30.0f) {
            float x = currPoints[i].x;
            float y = currPoints[i].y;
            if (x > 5 && x < smallWidth - 5 && y > 5 && y < smallHeight - 5) {
                // Масштабируем обратно в полный размер
                goodPrevPoints.push_back(cv::Point2f(prevPoints[i].x * 2.0f, prevPoints[i].y * 2.0f));
                goodCurrPoints.push_back(cv::Point2f(currPoints[i].x * 2.0f, currPoints[i].y * 2.0f));
                goodIndices.push_back(i);
            }
        }
    }

    if (goodPrevPoints.size() < 8) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

    // === ШАГИ 3-4: Essential Matrix & Pose Recovery ===
    
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        m_fx, 0.0, m_cx,
        0.0, m_fy, m_cy,
        0.0, 0.0, 1.0);

    cv::Mat essentialMatrix = cv::findEssentialMat(
        goodPrevPoints, goodCurrPoints, cameraMatrix,
        cv::RANSAC, 0.999, 1.0
    );

    if (essentialMatrix.empty()) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

    cv::Mat R, t;
    int inliers = cv::recoverPose(essentialMatrix, goodPrevPoints, goodCurrPoints, 
                                   cameraMatrix, R, t);

    if (inliers < 8) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

    // === ШАГИ 5: Триангуляция 3D точек ===
    
    // Матрицы проекции
    cv::Mat P1 = cameraMatrix * cv::Mat::eye(3, 4, CV_64F);
    cv::Mat P2 = cameraMatrix * (cv::Mat_<double>(3, 4) <<
        R.at<double>(0,0), R.at<double>(0,1), R.at<double>(0,2), t.at<double>(0),
        R.at<double>(1,0), R.at<double>(1,1), R.at<double>(1,2), t.at<double>(1),
        R.at<double>(2,0), R.at<double>(2,1), R.at<double>(2,2), t.at<double>(2));

    cv::Mat points4D;
    cv::triangulatePoints(P1, P2, goodPrevPoints, goodCurrPoints, points4D);

    // Конвертируем в 3D точки
    std::vector<cv::Point3f> triangulatedPoints;
    for (int i = 0; i < points4D.cols; ++i) {
        float w = points4D.at<float>(3, i);
        if (w > 0.001f) {
            cv::Point3f pt(
                points4D.at<float>(0, i) / w,
                points4D.at<float>(1, i) / w,
                points4D.at<float>(2, i) / w
            );
            
            // Фильтруем точки которые слишком далеко или слишком близко
            float depth = pt.z;
            if (depth > 0.1f && depth < 100.0f) {
                triangulatedPoints.push_back(pt);
            }
        }
    }

    if (triangulatedPoints.size() < 8) {
        pose.position = m_sensorFusion->getCurrentPose().position;
        pose.rotation = m_sensorFusion->getCurrentPose().rotation;
        return pose;
    }

    // === ШАГИ 6: SolvePnPRansac для текущего кадра ===
    
    // Используем триангулированные точки как 3D карту
    std::vector<cv::Point2f> imagePointsForPnP;
    for (size_t i = 0; i < triangulatedPoints.size(); ++i) {
        imagePointsForPnP.push_back(goodCurrPoints[i]);
    }

    cv::Mat rvec, tvec;
    cv::Mat inliersMask;
    
    bool success = cv::solvePnPRansac(
        triangulatedPoints, imagePointsForPnP,
        cameraMatrix, cv::Mat::zeros(4, 1, CV_64F),
        rvec, tvec,
        false,  // useExtrinsicGuess
        200,    // iterationsCount (больше для точности)
        4.0f,   // reprojectionError (меньше для точности)
        0.999,  // confidence (выше для надёжности)
        inliersMask,
        cv::SOLVEPNP_EPNP
    );

    if (!success || inliersMask.empty()) {
        // Fallback: используем результаты из essential matrix
        glm::mat3 rotationMat;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                rotationMat[i][j] = static_cast<float>(R.at<double>(i, j));
        pose.rotation = glm::quat_cast(rotationMat);
        
        // Масштабируем translation
        float scale = 0.05f;  // Conservative estimate
        glm::vec3 translation(
            static_cast<float>(t.at<double>(0)) * scale,
            -static_cast<float>(t.at<double>(1)) * scale,
            -static_cast<float>(t.at<double>(2)) * scale
        );
        
        glm::vec3 currentIMUPos = m_sensorFusion->getCurrentPose().position;
        pose.position = currentIMUPos + translation;
        pose.confidence = 0.3f;
        
        return pose;
    }

    // === Конвертируем результаты ===
    
    // Rotation
    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);
    
    glm::mat3 rotMat;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            rotMat[i][j] = static_cast<float>(rotationMatrix.at<double>(i, j));
    pose.rotation = glm::quat_cast(rotMat);

    // Translation с правильным преобразованием координат
    glm::vec3 translation(
        static_cast<float>(tvec.at<double>(0)),
        -static_cast<float>(tvec.at<double>(1)),
        -static_cast<float>(tvec.at<double>(2))
    );

    // Масштабируем на основе глубины триангулированных точек
    float avgDepth = 0.0f;
    for (const auto& pt : triangulatedPoints) {
        avgDepth += pt.z;
    }
    avgDepth /= triangulatedPoints.size();

    // Если средняя глубина ~1м, то масштаб примерно правильный
    float depthScale = avgDepth / 1.0f;
    translation *= depthScale;

    glm::vec3 currentIMUPos = m_sensorFusion->getCurrentPose().position;
    pose.position = currentIMUPos + translation;

    // Confidence на основе количества inliers
    int inlierCount = cv::countNonZero(inliersMask);
    pose.confidence = std::min(1.0f, static_cast<float>(inlierCount) / 30.0f);

    std::cout << "[VO] Triangulated: " << triangulatedPoints.size() 
              << " | Inliers: " << inlierCount
              << " | Depth: " << std::fixed << std::setprecision(2) << avgDepth << "m"
              << " | Pos: (" << pose.position.x << ", " << pose.position.y 
              << ", " << pose.position.z << ")" << std::endl;

    // Сохраняем tracked points для следующего кадра
    m_trackedFeatures.clear();
    for (size_t i = 0; i < goodCurrPoints.size(); ++i) {
        FeaturePoint fp;
        fp.screenPosition = glm::vec2(goodCurrPoints[i].x, goodCurrPoints[i].y);
        fp.confidence = 1.0f;
        fp.trackId = i;
        fp.position = glm::vec3(goodCurrPoints[i].x / currentFrame.width,
                                 goodCurrPoints[i].y / currentFrame.height, 1.0f);
        m_trackedFeatures.push_back(fp);
    }

#endif

    return pose;
}

} // namespace LensEngine
