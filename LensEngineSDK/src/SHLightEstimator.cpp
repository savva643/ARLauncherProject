#include "SHLightEstimator.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

namespace LensEngine {

SHLightEstimator::SHLightEstimator()
    : fx_(1312.0f), fy_(1312.0f), cx_(960.0f), cy_(540.0f)
{
}

SHLightEstimator::~SHLightEstimator() {
}

void SHLightEstimator::evaluateSHBasis(const glm::vec3& d, float Y[9]) const {
    // 9 real SH basis functions for bands l = 0..2
    Y[0] = 0.282095f;
    Y[1] = 0.488603f * d.y;
    Y[2] = 0.488603f * d.z;
    Y[3] = 0.488603f * d.x;
    Y[4] = 1.092548f * d.x * d.y;
    Y[5] = 1.092548f * d.y * d.z;
    Y[6] = 0.315392f * (3.0f * d.z * d.z - 1.0f);
    Y[7] = 1.092548f * d.x * d.z;
    Y[8] = 0.546274f * (d.x * d.x - d.y * d.y);
}

void SHLightEstimator::updateFromRGBImage(const RGBImage& rgbImage, float fx, float fy, float cx, float cy) {
    if (rgbImage.data.empty()) {
        return;
    }

    // Обновляем intrinsics если переданы новые значения
    if (fx > 0 && fy > 0) {
        fx_ = fx;
        fy_ = fy;
        cx_ = cx;
        cy_ = cy;
        hasIntrinsics_ = true;
    }

    if (!hasIntrinsics_) {
        return;
    }

    // Вычисляем обратную матрицу калибровки
    glm::mat3 Kinv(0.0f);
    Kinv[0][0] = 1.0f / fx_;
    Kinv[1][1] = 1.0f / fy_;
    Kinv[2][0] = -cx_ / fx_;
    Kinv[2][1] = -cy_ / fy_;
    Kinv[2][2] = 1.0f;

    Sh9Rgb acc{};
    double weightSum = 0.0;

    // Проходим по downsampled пикселям, back-project каждый на unit ray и накапливаем SH
    for (size_t i = 0; i < rgbImage.height; i += downsampleStride_) {
        for (size_t j = 0; j < rgbImage.width; j += downsampleStride_) {
            size_t idx = (i * rgbImage.width + j) * 3;
            
            // sRGB to linear RGB
            glm::vec3 sRGB(
                static_cast<float>(rgbImage.data[idx]) / 255.0f,
                static_cast<float>(rgbImage.data[idx + 1]) / 255.0f,
                static_cast<float>(rgbImage.data[idx + 2]) / 255.0f
            );
            glm::vec3 linear = glm::vec3(
                std::pow(sRGB.r, 2.2f),
                std::pow(sRGB.g, 2.2f),
                std::pow(sRGB.b, 2.2f)
            );

            // Back-project pixel to unit direction in camera space
            glm::vec3 ray = Kinv * glm::vec3(static_cast<float>(j), static_cast<float>(i), 1.0f);
            glm::vec3 d = glm::normalize(ray);
            
            // Flip Y чтобы image-down соответствовал view-up
            d.y = -d.y;
            // НЕ flip Z: обрабатываем переднюю полусферу камеры как освещение

            // Оцениваем 9 SH базисных функций в направлении d
            float Y[9];
            evaluateSHBasis(d, Y);

            const double dOmega = 1.0;
            for (int k = 0; k < 9; k++) {
                acc[k] += linear * Y[k] * static_cast<float>(dOmega);
            }
            weightSum += 1.0;
        }
    }

    constexpr float kPi = glm::pi<float>();

    // Нормализуем чтобы яркость не зависела от downsample stride
    const float inv = 2.0f * kPi / static_cast<float>(weightSum);
    for (int i = 0; i < 9; i++) {
        acc[i] *= inv;
    }

    // Конвертируем incident radiance в irradiance через свертку с Lambertian cosine lobe
    const float A0 = kPi;
    const float A1 = 2.0f * kPi / 3.0f;
    const float A2 = kPi / 4.0f;

    acc[0] *= A0;
    acc[1] *= A1; acc[2] *= A1; acc[3] *= A1;
    acc[4] *= A2; acc[5] *= A2; acc[6] *= A2; acc[7] *= A2; acc[8] *= A2;

    // Seed на первом кадре; EMA blend на последующих
    if (!valid_) {
        coeffs_ = acc;
        valid_ = true;
    } else {
        for (int i = 0; i < 9; i++) {
            coeffs_[i] = alpha_ * acc[i] + (1.0f - alpha_) * coeffs_[i];
        }
    }
}

LightEstimation SHLightEstimator::toLightEstimation() const {
    LightEstimation light;
    
    if (!valid_) {
        return light;
    }

    // Извлекаем доминирующий цвет из SH коэффициентов
    // Y00 (индекс 0) - это среднее освещение
    glm::vec3 ambient = coeffs_[0];
    
    // Вычисляем интенсивность
    light.ambientIntensity = (ambient.r + ambient.g + ambient.b) / 3.0f;
    light.ambientColor = glm::normalize(ambient);
    
    // Оцениваем направление основного света из Y1-1, Y10, Y11 (индексы 1,2,3)
    glm::vec3 lightDir(coeffs_[3].r, coeffs_[1].r, coeffs_[2].r);
    if (glm::length(lightDir) > 0.001f) {
        light.primaryLightDirection = glm::normalize(lightDir);
    }
    
    // Цветовая температура (упрощенно)
    light.colorTemperature = 6500.0f;
    
    light.isValid = true;
    return light;
}

} // namespace LensEngine
