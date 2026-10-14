#include "IMUDiagnostics.h"
#include <cmath>

namespace LensEngine {

IMUDiagnostics::IMUDiagnostics()
{
    reset();
}

void IMUDiagnostics::checkIMUData(const RawIMUData& imu)
{
    glm::vec3 accel(imu.accelX, imu.accelY, imu.accelZ);
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);
    
    // Обновляем статистику
    m_stats.accelMin = glm::min(m_stats.accelMin, accel);
    m_stats.accelMax = glm::max(m_stats.accelMax, accel);
    m_stats.accelMean += accel;
    
    m_stats.gyroMin = glm::min(m_stats.gyroMin, gyro);
    m_stats.gyroMax = glm::max(m_stats.gyroMax, gyro);
    m_stats.gyroMean += gyro;
    
    m_stats.sampleCount++;
    
    // Проверяем гравитацию
    checkGravity(accel);
    
    // Проверяем диапазон гироскопа
    checkGyroRange(gyro);
    
    // Выводим каждые 100 кадров
    if (m_printCounter++ % 100 == 0) {
        printStatistics();
    }
}

void IMUDiagnostics::printStatistics()
{
    // Отключено для ускорения теста
    /*
    if (m_stats.sampleCount == 0) {
        return;
    }
    
    glm::vec3 accelMean = m_stats.accelMean / static_cast<float>(m_stats.sampleCount);
    glm::vec3 gyroMean = m_stats.gyroMean / static_cast<float>(m_stats.sampleCount);
    
    std::cout << "\n========== IMU DIAGNOSTICS ==========" << std::endl;
    std::cout << "Samples: " << m_stats.sampleCount << std::endl;
    
    std::cout << "\n📊 ACCELEROMETER (userAcceleration - без гравитации) (m/s²):" << std::endl;
    std::cout << "  Min: [" << std::fixed << std::setprecision(2)
              << m_stats.accelMin.x << ", " << m_stats.accelMin.y << ", " << m_stats.accelMin.z << "]" << std::endl;
    std::cout << "  Max: [" << m_stats.accelMax.x << ", " << m_stats.accelMax.y << ", " << m_stats.accelMax.z << "]" << std::endl;
    std::cout << "  Mean: [" << accelMean.x << ", " << accelMean.y << ", " << accelMean.z << "]" << std::endl;
    
    // Проверяем что это разумные значения
    float accelMag = glm::length(accelMean);
    std::cout << "  Magnitude: " << accelMag << " m/s²" << std::endl;
    
    if (accelMag < 0.5f) {
        std::cout << "  ✅ STATIONARY (телефон неподвижен)" << std::endl;
    } else if (accelMag < 5.0f) {
        std::cout << "  ✅ MOVING (телефон движется нормально)" << std::endl;
    } else {
        std::cout << "  ⚠️  VERY FAST MOVEMENT (очень быстрое движение)" << std::endl;
    }
    
    std::cout << "\n🔄 GYROSCOPE (rad/s):" << std::endl;
    std::cout << "  Min: [" << std::fixed << std::setprecision(3)
              << m_stats.gyroMin.x << ", " << m_stats.gyroMin.y << ", " << m_stats.gyroMin.z << "]" << std::endl;
    std::cout << "  Max: [" << m_stats.gyroMax.x << ", " << m_stats.gyroMax.y << ", " << m_stats.gyroMax.z << "]" << std::endl;
    std::cout << "  Mean: [" << gyroMean.x << ", " << gyroMean.y << ", " << gyroMean.z << "]" << std::endl;
    
    // Проверяем диапазон
    float gyroMag = glm::length(gyroMean);
    if (gyroMag < 0.1f) {
        std::cout << "  ✅ GYRO RANGE OK (stationary)" << std::endl;
    } else {
        std::cout << "  ⚠️  GYRO ACTIVE (magnitude: " << gyroMag << ")" << std::endl;
    }
    
    std::cout << "====================================\n" << std::endl;
    */
}

void IMUDiagnostics::checkGravity(const glm::vec3& accel)
{
    // iPhone отправляет userAcceleration (БЕЗ гравитации!)
    // Поэтому проверяем что это разумные значения для движения
    float mag = glm::length(accel);
    
    // Если телефон неподвижен, ускорение должно быть близко к нулю
    // Если движется, может быть до нескольких m/s²
    // Более 5 m/s² - это очень быстрое движение
    // Отключено для тестового стенда (гравитация ~9.8 всегда считается высокой)
    /*
    if (mag > 5.0f) {
        std::cout << "⚠️  WARNING: High acceleration detected: " << std::fixed << std::setprecision(2) 
                  << mag << " m/s² (phone moving very fast)" << std::endl;
    }
    */
}

void IMUDiagnostics::checkGyroRange(const glm::vec3& gyro)
{
    // Гироскоп должен быть в разумном диапазоне
    float mag = glm::length(gyro);
    
    if (mag > 10.0f) {
        std::cout << "⚠️  WARNING: Gyroscope magnitude very high: " << std::fixed << std::setprecision(2) 
                  << mag << " rad/s" << std::endl;
    }
}

void IMUDiagnostics::reset()
{
    m_stats.accelMin = glm::vec3(999.0f);
    m_stats.accelMax = glm::vec3(-999.0f);
    m_stats.accelMean = glm::vec3(0.0f);
    m_stats.gyroMin = glm::vec3(999.0f);
    m_stats.gyroMax = glm::vec3(-999.0f);
    m_stats.gyroMean = glm::vec3(0.0f);
    m_stats.sampleCount = 0;
    m_printCounter = 0;
}

} // namespace LensEngine
