#include "LensEngineAPI.h"
#include <iostream>
#include <vector>

/**
 * @brief Пример использования LensEngineSDK
 */
int main() {
    using namespace LensEngine;
    
    // Инициализация движка
    LensEngineAPI engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize LensEngine" << std::endl;
        return -1;
    }
    
    // Установка колбэков
    engine.setPoseCallback([](const CameraPose& pose) {
        std::cout << "Camera Pose: Position(" 
                  << pose.position.x << ", "
                  << pose.position.y << ", "
                  << pose.position.z << ") "
                  << "Confidence: " << pose.confidence << std::endl;
    });
    
    engine.setFeaturePointsCallback([](const std::vector<FeaturePoint>& points) {
        std::cout << "Feature Points: " << points.size() << std::endl;
    });
    
    engine.setLidarPointsCallback([](const std::vector<glm::vec3>& points) {
        std::cout << "LiDAR Points: " << points.size() << std::endl;
    });
    
    // Имитация данных (в реальности данные приходят от SensorConnector)
    RawIMUData imuData;
    imuData.timestamp = 1000;
    imuData.accelX = 0.0;
    imuData.accelY = 0.0;
    imuData.accelZ = 9.81;
    imuData.gyroX = 0.0;
    imuData.gyroY = 0.0;
    imuData.gyroZ = 0.0;
    imuData.gravityX = 0.0;
    imuData.gravityY = -1.0;
    imuData.gravityZ = 0.0;
    
    // Обработка данных
    engine.processIMUData(imuData);
    
    // Получение результатов
    CameraPose pose = engine.getCurrentCameraPose();
    std::cout << "Current Pose: (" << pose.position.x << ", "
              << pose.position.y << ", " << pose.position.z << ")" << std::endl;
    
    return 0;
}

