#ifndef SHLIGHTESTIMATOR_H
#define SHLIGHTESTIMATOR_H

#include "LensEngineTypes.h"
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace LensEngine {

/**
 * @brief Spherical-harmonic lighting estimator
 * 
 * Оценивает освещение сцены используя 9 коэффициентов сферических гармоник.
 * Основано на алгоритме из photorealistic-ar, но без OpenCV зависимости.
 */
class SHLightEstimator {
public:
    static constexpr int kShCoeffs = 9;
    using Sh9Rgb = std::array<glm::vec3, kShCoeffs>;

    SHLightEstimator();
    ~SHLightEstimator();

    /**
     * @brief Обновляет оценку освещения из RGB изображения
     * @param rgbImage Входное RGB изображение
     * @param fx Фокусное расстояние X
     * @param fy Фокусное расстояние Y
     * @param cx Главная точка X
     * @param cy Главная точка Y
     */
    void updateFromRGBImage(const RGBImage& rgbImage, float fx, float fy, float cx, float cy);

    /**
     * @brief Возвращает текущие SH коэффициенты
     */
    const Sh9Rgb& coefficients() const { return coeffs_; }

    /**
     * @brief Возвращает true если есть валидная оценка
     */
    bool hasValidEstimate() const { return valid_; }

    /**
     * @brief Конвертирует SH коэффициенты в LightEstimation структуру
     */
    LightEstimation toLightEstimation() const;

    /**
     * @brief Устанавливает EMA коэффициент сглаживания (1 = без сглаживания)
     */
    void setTemporalAlpha(float a) { alpha_ = a; }

    /**
     * @brief Устанавливает шаг downsample (выше = быстрее, но менее точно)
     */
    void setDownsampleStride(int n) { downsampleStride_ = (n > 0 ? n : 1); }

private:
    float fx_, fy_, cx_, cy_;
    bool hasIntrinsics_ = false;

    int downsampleStride_ = 8;
    float alpha_ = 0.3f;

    Sh9Rgb coeffs_{};
    bool valid_ = false;

    /**
     * @brief Оценивает 9 SH базисных функций в направлении d
     */
    void evaluateSHBasis(const glm::vec3& d, float Y[9]) const;
};

} // namespace LensEngine

#endif // SHLIGHTESTIMATOR_H
