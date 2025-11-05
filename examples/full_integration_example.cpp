/**
 * @file full_integration_example.cpp
 * @brief Полный пример интеграции всех компонентов
 * 
 * Демонстрирует работу полного цикла:
 * SensorConnector → LensEngineSDK → ARLauncher
 */

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

// SensorConnector
#include "SensorConnector.h"
using namespace SensorConnector;

// LensEngineSDK
#include "LensEngineAPI.h"
using namespace LensEngine;

// ARLauncher (заголовки для демонстрации)
// В реальном приложении:
// #include "Application.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "🚀 ARLauncherProject - Full Integration Example" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // ============================================
    // 1. Инициализация SensorConnector
    // ============================================
    std::cout << "\n📡 Step 1: Initializing SensorConnector..." << std::endl;
    
    SensorConnectorCore connector;
    if (!connector.initialize()) {
        std::cerr << "❌ Failed to initialize SensorConnector" << std::endl;
        return -1;
    }
    
    connector.startServers(9000, 9000);
    std::cout << "✅ SensorConnector initialized on ports 9000 (TCP/UDP)" << std::endl;
    
    // ============================================
    // 2. Инициализация LensEngineSDK
    // ============================================
    std::cout << "\n🎯 Step 2: Initializing LensEngineSDK..." << std::endl;
    
    LensEngineAPI engine;
    if (!engine.initialize()) {
        std::cerr << "❌ Failed to initialize LensEngineSDK" << std::endl;
        return -1;
    }
    
    // Настройка параметров шума
    engine.setNoiseParameters(0.01, 0.1, 0.05, 0.02);
    
    // Настройка параметров камеры (iPhone)
    engine.setCameraParameters(1920.0f, 1080.0f, 960.0f, 540.0f);
    
    std::cout << "✅ LensEngineSDK initialized" << std::endl;
    
    // ============================================
    // 3. Подключение SensorConnector → LensEngineSDK
    // ============================================
    std::cout << "\n🔗 Step 3: Connecting SensorConnector → LensEngineSDK..." << std::endl;
    
    QObject::connect(&connector, &SensorConnectorCore::dataReceived,
                     [&engine](const SensorData &data) {
        switch (data.type) {
            case RGB_CAMERA: {
                // Парсинг JPEG для получения размеров
                // В реальности размеры должны приходить в пакете
                uint32_t width = 1920;
                uint32_t height = 1080;
                
                engine.processRGBData(
                    reinterpret_cast<const uint8_t*>(data.payload.data()),
                    data.payload.size(),
                    width, height,
                    data.timestamp
                );
                break;
            }
            
            case RAW_IMU: {
                // Парсинг IMU данных
                if (data.payload.size() >= sizeof(RawIMUData)) {
                    RawIMUData imu;
                    memcpy(&imu, data.payload.data(), sizeof(RawIMUData));
                    engine.processIMUData(imu);
                }
                break;
            }
            
            case LIDAR_DEPTH: {
                // Парсинг LiDAR данных
                size_t depthSize = 256 * 192 * 4; // float array
                size_t confidenceSize = 256 * 192; // uint8 array
                
                if (data.payload.size() >= depthSize) {
                    const uint8_t* depthData = reinterpret_cast<const uint8_t*>(data.payload.data());
                    const uint8_t* confidenceData = depthData + depthSize;
                    
                    engine.processLidarData(
                        depthData, depthSize,
                        confidenceData, confidenceSize,
                        data.timestamp
                    );
                }
                break;
            }
            
            default:
                // Остальные типы данных обрабатываются аналогично
                break;
        }
    });
    
    std::cout << "✅ SensorConnector connected to LensEngineSDK" << std::endl;
    
    // ============================================
    // 4. Подключение LensEngineSDK → ARLauncher
    // ============================================
    std::cout << "\n🎨 Step 4: Connecting LensEngineSDK → ARLauncher..." << std::endl;
    
    // Установка колбэков для обновления ARLauncher
    engine.setPoseCallback([](const CameraPose& pose) {
        std::cout << "📹 Camera Pose updated: "
                  << "Position(" << pose.position.x << ", "
                  << pose.position.y << ", " << pose.position.z << ") "
                  << "Confidence: " << pose.confidence << std::endl;
        
        // В реальном приложении здесь будет:
        // app.getScene()->updateCameraFromAR(pose.position, pose.rotation);
    });
    
    engine.setFeaturePointsCallback([](const std::vector<FeaturePoint>& points) {
        std::cout << "🔍 Feature Points: " << points.size() << std::endl;
    });
    
    engine.setLidarPointsCallback([](const std::vector<glm::vec3>& points) {
        std::cout << "📊 LiDAR Points: " << points.size() << std::endl;
    });
    
    std::cout << "✅ LensEngineSDK connected to ARLauncher callbacks" << std::endl;
    
    // ============================================
    // 5. Статистика
    // ============================================
    std::cout << "\n📊 Step 5: Setting up statistics..." << std::endl;
    
    QObject::connect(&connector, &SensorConnectorCore::statisticsUpdated,
                     [](const ConnectionStats &stats) {
        std::cout << "📈 Statistics: "
                  << "FPS: " << stats.fps << ", "
                  << "Speed: " << stats.speedKbps << " kbps, "
                  << "Clients: " << stats.clientsCount << std::endl;
    });
    
    // Обновление статистики каждую секунду
    QTimer* statsTimer = new QTimer(&app);
    QObject::connect(statsTimer, &QTimer::timeout, [&connector]() {
        connector.updateStatistics();
    });
    statsTimer->start(1000);
    
    std::cout << "✅ Statistics monitoring enabled" << std::endl;
    
    // ============================================
    // 6. Запуск
    // ============================================
    std::cout << "\n🎉 All components initialized!" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "📱 Waiting for iPhone connection..." << std::endl;
    std::cout << "🔌 Connect iPhone via USB or WiFi to port 9000" << std::endl;
    std::cout << "⏹️  Press Ctrl+C to stop" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // В реальном приложении здесь будет:
    // Application arLauncher;
    // arLauncher.initialize(argc, argv);
    // arLauncher.run();
    
    // Для демонстрации просто запускаем Qt event loop
    return app.exec();
}

