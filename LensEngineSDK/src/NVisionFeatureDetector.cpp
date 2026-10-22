#include "NVisionFeatureDetector.h"
#include <algorithm>
#include <iostream>

#ifndef LENSENGINE_HAVE_NVISION
// nvision-cpp не найден при сборке: заглушка, ARDataProcessor использует OpenCV
namespace LensEngine {
NVisionFeatureDetector::NVisionFeatureDetector() = default;
NVisionFeatureDetector::~NVisionFeatureDetector() = default;
std::vector<FeaturePoint> NVisionFeatureDetector::detectFeatures(const RGBImage&, int) { return {}; }
std::vector<FeaturePoint> NVisionFeatureDetector::detectORBFeatures(const RGBImage&, int) { return {}; }
} // namespace LensEngine
#else
namespace LensEngine {

NVisionFeatureDetector::NVisionFeatureDetector() {
}

NVisionFeatureDetector::~NVisionFeatureDetector() {
}

nvision::Image<nvision::Grayf> NVisionFeatureDetector::convertToGray(const RGBImage& rgbImage) {
    // Создаем изображение в формате nvision
    nvision::Image<nvision::Grayf> grayImage(rgbImage.height, rgbImage.width);
    
    // Конвертируем RGB в grayscale (простое среднее)
    for (size_t y = 0; y < rgbImage.height; ++y) {
        for (size_t x = 0; x < rgbImage.width; ++x) {
            size_t idx = (y * rgbImage.width + x) * 3;
            float r = static_cast<float>(rgbImage.data[idx]) / 255.0f;
            float g = static_cast<float>(rgbImage.data[idx + 1]) / 255.0f;
            float b = static_cast<float>(rgbImage.data[idx + 2]) / 255.0f;
            
            // Стандартное grayscale преобразование
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;
            grayImage(y, x) = nvision::Pixel<nvision::Grayf>(gray);
        }
    }
    
    return grayImage;
}

FeaturePoint NVisionFeatureDetector::convertToFeaturePoint(const Eigen::Vector2f& keypoint, float response) {
    FeaturePoint fp;
    fp.screenPosition = glm::vec2(static_cast<float>(keypoint.x()), static_cast<float>(keypoint.y()));
    fp.confidence = response;
    return fp;
}

std::vector<FeaturePoint> NVisionFeatureDetector::detectFeatures(const RGBImage& rgbImage, int maxFeatures) {
    std::vector<FeaturePoint> features;
    
    if (rgbImage.data.empty()) {
        return features;
    }
    
    try {
        // Конвертируем в grayscale
        nvision::Image<nvision::Grayf> grayImage = convertToGray(rgbImage);
        
        // Создаем FAST детектор
        nvision::FASTFeature<nvision::float32> detector;
        
        // Извлекаем keypoints
        using FeatureMatrix = typename nvision::FASTFeature<nvision::float32>::FeatureMatrix;
        FeatureMatrix keypoints;
        detector(grayImage, keypoints);
        
        // Ограничиваем количество features
        int numFeatures = std::min(static_cast<int>(keypoints.cols()), maxFeatures);
        
        // Конвертируем в FeaturePoint
        for (int i = 0; i < numFeatures; ++i) {
            Eigen::Vector2f keypoint = keypoints.col(i);
            features.push_back(convertToFeaturePoint(keypoint, 1.0f));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[NVisionFeatureDetector] Error: " << e.what() << std::endl;
    }
    
    return features;
}

std::vector<FeaturePoint> NVisionFeatureDetector::detectORBFeatures(const RGBImage& rgbImage, int maxFeatures) {
    std::vector<FeaturePoint> features;
    
    if (rgbImage.data.empty()) {
        return features;
    }
    
    try {
        // Конвертируем в grayscale
        nvision::Image<nvision::Grayf> grayImage = convertToGray(rgbImage);
        
        // Создаем ORB детектор
        nvision::ORBFeature<nvision::float32> detector;
        
        // Извлекаем keypoints
        using FeatureMatrix = typename nvision::ORBFeature<nvision::float32>::FeatureMatrix;
        FeatureMatrix keypoints;
        detector(grayImage, keypoints);
        
        // Ограничиваем количество features
        int numFeatures = std::min(static_cast<int>(keypoints.cols()), maxFeatures);
        
        // Конвертируем в FeaturePoint
        for (int i = 0; i < numFeatures; ++i) {
            Eigen::Vector2f keypoint = keypoints.col(i);
            features.push_back(convertToFeaturePoint(keypoint, 1.0f));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[NVisionFeatureDetector] Error: " << e.what() << std::endl;
    }
    
    return features;
}

} // namespace LensEngine

#endif // LENSENGINE_HAVE_NVISION
