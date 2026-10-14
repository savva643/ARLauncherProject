#ifndef DLO_TRACKER_H
#define DLO_TRACKER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <deque>
#include <mutex>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

namespace LensEngine {

struct RawIMUData;

/**
 * @brief Direct LiDAR Odometry (DLO) Tracker
 * 
 * Использует облако точек LiDAR + IMU для получения точной 6DOF позиции.
 * Алгоритм:
 * - IMU propagation для предсказания начальной позы (initial guess)
 * - Voxel Grid downsampling для ускорения
 * - GICP (Generalized ICP) для выравнивания облаков
 * - Geometric Observer для коррекции state из GICP результата
 * - Temporal filtering для сглаживания
 */

struct DLOPose {
    glm::vec3 position;      // Позиция в world space
    glm::quat rotation;      // Ротация в world space
    float confidence;        // Уверенность (0-1)
    uint64_t timestamp;      // Временная метка
};

struct IMUData {
    glm::vec3 acceleration;  // m/s^2
    glm::vec3 angularVelocity; // rad/s
    float temperature;       // Celsius
    uint64_t timestamp;      // Временная метка
};

class DLOTracker {
public:
    DLOTracker();
    ~DLOTracker();
    
    /**
     * @brief Инициализация трекера
     * @param voxelSize Размер вокселя для downsampling (по умолчанию 0.1m)
     * @param maxIterations Максимум итераций ICP (по умолчанию 50)
     * @return true если успешно инициализировано
     */
    bool initialize(float voxelSize = 0.1f, int maxIterations = 50);
    
    /**
     * @brief Обновить трекер с новым облаком точек и IMU данными
     * @param pointCloud Облако точек LiDAR (в camera space)
     * @param imuData IMU данные (опционально)
     * @return Текущая 6DOF поза
     */
    DLOPose update(const pcl::PointCloud<pcl::PointXYZI>::Ptr& pointCloud, 
                   const IMUData* imuData = nullptr);
    
    /**
     * @brief Обновить трекер с облаком точек (glm::vec3)
     * @param points Облако точек в world space
     * @param imuData IMU данные (опционально)
     * @return Текущая 6DOF поза
     */
    DLOPose update(const std::vector<glm::vec3>& points,
                   const IMUData* imuData = nullptr);
    
    /**
     * @brief Получить текущую позу
     */
    DLOPose getCurrentPose() const { return m_currentPose; }
    
    /**
     * @brief Получить облако точек в world space
     */
    pcl::PointCloud<pcl::PointXYZI>::Ptr getWorldPointCloud() const { return m_worldCloud; }
    
    /**
     * @brief Сбросить трекер (очистить историю)
     */
    void reset();
    
    /**
     * @brief Установить параметры фильтрации
     */
    void setFilterParams(float voxelSize, float maxRange = 100.0f);
    
    /**
     * @brief Установить параметры ICP
     */
    void setICPParams(int maxIterations, float fitnessThreshold = 0.01f);
    
    /**
     * @brief Feed IMU data for prediction between LiDAR frames
     */
    void feedIMU(const RawIMUData& imu, double dt);
    
    /**
     * @brief Получить статистику качества трекинга
     */
    struct TrackingStats {
        float icpFitness;        // Ошибка выравнивания ICP
        int correspondences;     // Количество соответствий
        float transformationError; // Ошибка трансформации
        bool isInitialized;      // Инициализирован ли трекер
    };
    TrackingStats getStats() const { return m_stats; }
    
private:
    // Конвертация между форматами
    pcl::PointCloud<pcl::PointXYZI>::Ptr glmToPCL(const std::vector<glm::vec3>& points);
    glm::vec3 pclToGlm(const pcl::PointXYZI& point);
    
    // Основной алгоритм ICP
    bool performICP(const pcl::PointCloud<pcl::PointXYZI>::Ptr& source,
                    const pcl::PointCloud<pcl::PointXYZI>::Ptr& target,
                    Eigen::Matrix4f& transformation);
    
    // Фильтрация облака точек
    pcl::PointCloud<pcl::PointXYZI>::Ptr filterPointCloud(
        const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud);
    
    // Трансформация облака точек
    pcl::PointCloud<pcl::PointXYZI>::Ptr transformPointCloud(
        const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud,
        const Eigen::Matrix4f& transformation);
    
    // Преобразование Eigen матрицы в glm
    glm::mat4 eigenToGlm(const Eigen::Matrix4f& mat);
    Eigen::Matrix4f glmToEigen(const glm::mat4& mat);
    
    // IMU propagation: предсказание позы между LiDAR кадрами
    Eigen::Matrix4f imuPredict();
    void propagateIMUState(const RawIMUData& imu, double dt);
    
    // Члены класса
    DLOPose m_currentPose;
    pcl::PointCloud<pcl::PointXYZI>::Ptr m_previousCloud;
    pcl::PointCloud<pcl::PointXYZI>::Ptr m_worldCloud;
    
    float m_voxelSize;
    float m_maxRange;
    int m_maxICPIterations;
    float m_icpFitnessThreshold;
    
    bool m_isInitialized;
    TrackingStats m_stats;
    
    // Для сглаживания
    glm::vec3 m_smoothedPosition;
    glm::quat m_smoothedRotation;
    float m_smoothingAlpha = 0.3f;
    
    // IMU propagation state
    glm::vec3 m_imuPosition;
    glm::quat m_imuOrientation;
    glm::vec3 m_imuVelocity;
    glm::vec3 m_imuAccelBias;
    glm::vec3 m_imuGyroBias;
    Eigen::Matrix4f m_lastICPTransform;
    bool m_hasICPResult;
    double m_lastLidarTimestamp;
    
    // Per-frame IMU tracking for relative prediction
    glm::vec3 m_lastFrameIMUPosition;
    glm::quat m_lastFrameIMUOrientation;
};

} // namespace LensEngine

#endif // DLO_TRACKER_H
