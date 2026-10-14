#ifndef IMU_DIAGNOSTICS_H
#define IMU_DIAGNOSTICS_H

#include "LensEngineTypes.h"
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

namespace LensEngine {

/**
 * @brief Диагностика IMU - проверяет правильность работы датчиков
 */
class IMUDiagnostics {
public:
    IMUDiagnostics();
    
    // Проверить IMU данные
    void checkIMUData(const RawIMUData& imu);
    
    // Вывести статистику
    void printStatistics();
    
    // Сброс
    void reset();

private:
    struct IMUStats {
        glm::vec3 accelMin = glm::vec3(999.0f);
        glm::vec3 accelMax = glm::vec3(-999.0f);
        glm::vec3 accelMean = glm::vec3(0.0f);
        
        glm::vec3 gyroMin = glm::vec3(999.0f);
        glm::vec3 gyroMax = glm::vec3(-999.0f);
        glm::vec3 gyroMean = glm::vec3(0.0f);
        
        int sampleCount = 0;
    };
    
    IMUStats m_stats;
    int m_printCounter = 0;
    
    // Проверки
    void checkGravity(const glm::vec3& accel);
    void checkGyroRange(const glm::vec3& gyro);
};

} // namespace LensEngine

#endif // IMU_DIAGNOSTICS_H
