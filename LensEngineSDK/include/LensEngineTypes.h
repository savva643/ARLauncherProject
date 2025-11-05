#ifndef LENSENGINETYPES_H
#define LENSENGINETYPES_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace LensEngine {

// Типы данных от iPhone
enum class DataType : uint8_t {
    RGB_CAMERA = 0x01,           // RGB данные камеры
    LIDAR_DEPTH = 0x02,          // LiDAR данные
    RAW_IMU = 0x03,              // Сырые IMU данные
    FEATURE_POINTS = 0x04,       // Feature Points
    CAMERA_INTRINSICS = 0x05,    // Camera Intrinsics
    LIGHT_ESTIMATION = 0x06      // Light Estimation
};

// Сырые IMU данные
struct RawIMUData {
    uint64_t timestamp;
    double accelX, accelY, accelZ;    // Акселерометр (m/s²)
    double gyroX, gyroY, gyroZ;       // Гироскоп (rad/s)
    double gravityX, gravityY, gravityZ; // Гравитация (нормализованный)
    double magX, magY, magZ;          // Магнитометр (μT)
    
    RawIMUData() : timestamp(0), accelX(0), accelY(0), accelZ(0),
        gyroX(0), gyroY(0), gyroZ(0), gravityX(0), gravityY(0), gravityZ(0),
        magX(0), magY(0), magZ(0) {}
};

// Feature Point
struct FeaturePoint {
    glm::vec3 position;          // 3D позиция
    glm::vec2 screenPosition;     // 2D позиция на экране
    float confidence;            // Уверенность (0-1)
    uint64_t trackId;             // ID для отслеживания
    
    FeaturePoint() : position(0.0f), screenPosition(0.0f), confidence(0.0f), trackId(0) {}
};

// Camera Intrinsics
struct CameraIntrinsics {
    glm::mat3 matrix;            // Матрица калибровки 3x3
    glm::vec2 resolution;         // Разрешение камеры
    float focalLengthX;          // Фокусное расстояние X
    float focalLengthY;          // Фокусное расстояние Y
    float principalPointX;       // Главная точка X
    float principalPointY;       // Главная точка Y
    bool isValid;
    
    CameraIntrinsics() : matrix(1.0f), resolution(0.0f), focalLengthX(0), focalLengthY(0),
        principalPointX(0), principalPointY(0), isValid(false) {}
};

// Light Estimation
struct LightEstimation {
    float ambientIntensity;      // Интенсивность окружающего света
    float colorTemperature;      // Цветовая температура
    glm::vec3 ambientColor;      // Цвет окружающего света
    glm::vec3 primaryLightDirection; // Направление основного света
    bool isValid;
    
    LightEstimation() : ambientIntensity(0), colorTemperature(6500),
        ambientColor(1.0f), primaryLightDirection(0.0f, 1.0f, 0.0f), isValid(false) {}
};

// Camera Pose (результат сенсорного фьюжна)
struct CameraPose {
    glm::vec3 position;          // Позиция в мировых координатах
    glm::quat rotation;           // Ориентация
    glm::mat4 viewMatrix;         // Матрица вида
    glm::mat4 projectionMatrix;  // Матрица проекции
    float confidence;            // Уверенность позы (0-1)
    uint64_t timestamp;
    
    CameraPose() : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f),
        viewMatrix(1.0f), projectionMatrix(1.0f), confidence(0), timestamp(0) {}
};

// LiDAR данные
struct LidarData {
    std::vector<uint8_t> depthMap;      // Карта глубины (сырая)
    std::vector<uint8_t> confidenceMap; // Карта уверенности
    std::vector<uint8_t> pointCloud;    // Облако точек (сырое)
    std::vector<glm::vec3> points3D;    // Обработанные 3D точки
    uint64_t sequenceNumber;
    uint64_t timestamp;
    
    LidarData() : sequenceNumber(0), timestamp(0) {}
};

// RGB изображение (для обработки)
struct RGBImage {
    std::vector<uint8_t> data;   // RGB данные
    uint32_t width;
    uint32_t height;
    uint64_t timestamp;
    
    RGBImage() : width(0), height(0), timestamp(0) {}
};

// Полный AR кадр
struct ARFrame {
    RGBImage rgbImage;                    // RGB кадр
    std::vector<FeaturePoint> featurePoints; // Feature points из RGB
    RawIMUData imu;                       // IMU данные
    LidarData lidar;                      // LiDAR данные
    CameraIntrinsics intrinsics;          // Параметры камеры
    LightEstimation light;                // Освещение
    CameraPose cameraPose;                // Рассчитанная поза камеры
    uint64_t sequenceNumber;
    uint64_t timestamp;
    
    ARFrame() : sequenceNumber(0), timestamp(0) {}
};

} // namespace LensEngine

#endif // LENSENGINETYPES_H

