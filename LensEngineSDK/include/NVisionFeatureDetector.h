#ifndef NVISIONFEATUREDETECTOR_H
#define NVISIONFEATUREDETECTOR_H

#include "LensEngineTypes.h"
#ifdef LENSENGINE_HAVE_NVISION
#include <nvision/feature.h>
#include <nvision/src/core/image.h>
#include <nvision/src/core/pixel.h>
#endif
#include <vector>

namespace LensEngine {

/**
 * @brief Обертка для nvision-cpp feature detection
 * 
 * Использует FAST feature detection из nvision-cpp библиотеки
 * для быстрого извлечения feature points из RGB изображений.
 */
class NVisionFeatureDetector {
public:
    NVisionFeatureDetector();
    ~NVisionFeatureDetector();

    /**
     * @brief Извлекает feature points из RGB изображения
     * @param rgbImage Входное RGB изображение
     * @param maxFeatures Максимальное количество feature points
     * @return Вектор обнаруженных feature points
     */
    std::vector<FeaturePoint> detectFeatures(const RGBImage& rgbImage, int maxFeatures = 500);

    /**
     * @brief Извлекает feature points с использованием ORB
     * @param rgbImage Входное RGB изображение
     * @param maxFeatures Максимальное количество feature points
     * @return Вектор обнаруженных feature points с дескрипторами
     */
    std::vector<FeaturePoint> detectORBFeatures(const RGBImage& rgbImage, int maxFeatures = 500);

#ifdef LENSENGINE_HAVE_NVISION
private:
    /**
     * @brief Преобразует RGBImage в nvision::Image
     */
    nvision::Image<nvision::Grayf> convertToGray(const RGBImage& rgbImage);

    /**
     * @brief Преобразует keypoints из nvision в FeaturePoint
     */
    FeaturePoint convertToFeaturePoint(const Eigen::Vector2f& keypoint, float response);
#endif
};

} // namespace LensEngine

#endif // NVISIONFEATUREDETECTOR_H
