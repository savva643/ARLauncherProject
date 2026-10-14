#include "../include/TestRunner.h"
#include <iostream>

using namespace LensEngine;

int main() {
    std::cout << "=== AR 6DOF Tracking - Sensor Data Test ===" << std::endl;
    std::cout << "This test generates synthetic sensor data (like iPhone)" << std::endl;
    std::cout << "and sends it to LensEngine to verify tracking accuracy." << std::endl;
    std::cout << std::endl;
    
    TestRunner testRunner;
    
    // Инициализируем тест: 10 секунд, 120 FPS
    // Проверка всех фич: RGB 320x240 (для feature detection), LiDAR 64x48
    if (!testRunner.initialize(10, 120, "received_data.log", 320, 240, 64, 48)) {
        std::cerr << "Failed to initialize test runner!" << std::endl;
        return 1;
    }
    
    // Запускаем тест
    testRunner.run();
    
    // Получаем статистику
    auto stats = testRunner.getStatistics();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Total frames: " << stats.totalFrames << std::endl;
    std::cout << "Successful frames: " << stats.successfulFrames << std::endl;
    std::cout << "Avg FPS: " << stats.avgFPS << std::endl;
    std::cout << "Avg Position Error: " << stats.avgPositionError * 100 << " cm" << std::endl;
    std::cout << "Max Position Error: " << stats.maxPositionError * 100 << " cm" << std::endl;
    std::cout << "Avg Rotation Error: " << stats.avgRotationError * 57.3 << " deg" << std::endl;
    std::cout << "Max Rotation Error: " << stats.maxRotationError * 57.3 << " deg" << std::endl;
    std::cout << "Avg Confidence: " << stats.avgConfidence << std::endl;
    
    std::cout << "\n=== Log Files ===" << std::endl;
    std::cout << "sensor_data.log - Generated sensor data (ground truth)" << std::endl;
    std::cout << "received_data.log - Received data from LensEngine" << std::endl;
    std::cout << std::endl;
    std::cout << "You can compare these files to verify data integrity." << std::endl;
    
    return 0;
}
