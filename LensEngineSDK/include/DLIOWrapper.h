#ifndef DLIO_WRAPPER_H
#define DLIO_WRAPPER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <deque>
#include <Eigen/Dense>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

namespace LensEngine {

struct RawIMUData;

/**
 * @brief Standalone DLIO (Direct LiDAR Inertial Odometry) wrapper
 *
 * Ports core DLIO algorithms without ROS dependency:
 * - GICP scan-to-submap registration (PCL)
 * - Geometric observer for IMU-LiDAR fusion
 * - Keyframe/submap management
 * - IMU propagation between LiDAR frames
 */

struct DLIOPose {
    glm::vec3 position;
    glm::quat rotation;
    float confidence;
    uint64_t timestamp;
};

struct IMUData {
    glm::vec3 acceleration;
    glm::vec3 angularVelocity;
    float temperature;
    uint64_t timestamp;
};

class DLIOWrapper {
public:
    DLIOWrapper();
    ~DLIOWrapper();

    bool initialize(float voxelSize = 0.05f, int maxIterations = 64);
    /**
     * @brief Сбросить DLIO (очистить историю)
     */
    void reset();
    
    /**
     * @brief Передать IMU данные для предсказания между LiDAR кадрами
     */
    void feedIMU(const RawIMUData& imuData, double dt);
    
    DLIOPose update(const pcl::PointCloud<pcl::PointXYZI>::Ptr& pointCloud,
                    const IMUData* imuData = nullptr);
    DLIOPose update(const std::vector<glm::vec3>& points,
                    const IMUData* imuData = nullptr);

    DLIOPose getCurrentPose() const { return m_currentPose; }

    struct TrackingStats {
        float icpFitness;
        int correspondences;
        bool isInitialized;
        int keyframeCount;
    };
    TrackingStats getStats() const { return m_stats; }

private:
    struct State {
        Eigen::Vector3f p = Eigen::Vector3f::Zero();
        Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
        Eigen::Vector3f velocity = Eigen::Vector3f::Zero();
        Eigen::Vector3f accelBias = Eigen::Vector3f::Zero();
        Eigen::Vector3f gyroBias = Eigen::Vector3f::Zero();
    };

    struct ImuSample {
        double stamp;
        double dt;
        Eigen::Vector3f angVel;
        Eigen::Vector3f linAccel;
    };

    struct Keyframe {
        Eigen::Vector3f position;
        Eigen::Quaternionf rotation;
        pcl::PointCloud<pcl::PointXYZI>::Ptr cloud;
    };

    pcl::PointCloud<pcl::PointXYZI>::Ptr glmToPCL(const std::vector<glm::vec3>& points);
    pcl::PointCloud<pcl::PointXYZI>::Ptr filterPointCloud(
        const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud);
    pcl::PointCloud<pcl::PointXYZI>::Ptr transformCloud(
        const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud,
        const Eigen::Matrix4f& T);

    void propagateState(double dt);
    void updateState(const Eigen::Vector3f& lidarPos, const Eigen::Quaternionf& lidarQuat, double dt);
    Eigen::Matrix4f imuPredict();
    void propagateGICP(const Eigen::Matrix4f& T);
    bool performGICP(const pcl::PointCloud<pcl::PointXYZI>::Ptr& source,
                     const pcl::PointCloud<pcl::PointXYZI>::Ptr& target,
                     const Eigen::Matrix4f& initialGuess,
                     Eigen::Matrix4f& transformation,
                     float& fitness);
    void updateKeyframes(const Eigen::Vector3f& lidarPos, const Eigen::Quaternionf& lidarQuat,
                         const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud);
    void buildSubmap();
    glm::vec3 eigenToGlm(const Eigen::Vector3f& v) const;
    glm::quat eigenToGlm(const Eigen::Quaternionf& q) const;
    Eigen::Matrix4f glmToEigen(const glm::mat4& m) const;
    Eigen::Matrix4f stateToMatrix() const;
    void syncPoseFromState();

    DLIOPose m_currentPose;
    State m_state;
    Eigen::Vector3f m_lidarPose;
    Eigen::Quaternionf m_lidarQuat;

    pcl::PointCloud<pcl::PointXYZI>::Ptr m_submapCloud;
    std::vector<Keyframe> m_keyframes;
    std::mutex m_keyframeMutex;

    std::deque<ImuSample> m_imuBuffer;
    ImuSample m_lastImu;
    std::mutex m_imuMutex;
    bool m_hasImu;

    float m_voxelSize;
    float m_maxRange;
    int m_maxGICPIterations;
    float m_gicpMaxCorrDist;

    double m_keyframeDistThresh;
    double m_keyframeRotThresh;
    int m_submapKnn;

    double m_geoKp, m_geoKv, m_geoKq, m_geoKab, m_geoKgb;
    double m_gravity;

    bool m_isInitialized;
    bool m_firstScan;
    double m_prevScanTime;
    double m_lastScanTime;

    Eigen::Matrix4f m_T;
    Eigen::Matrix4f m_Tprior;

    TrackingStats m_stats;
};

} // namespace LensEngine

#endif // DLIO_WRAPPER_H
