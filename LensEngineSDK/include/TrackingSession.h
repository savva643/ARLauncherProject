#ifndef TRACKINGSESSION_H
#define TRACKINGSESSION_H

#include "HeadTracker.h"
#include "LidarOdometry.h"
#include "SceneReconstruction.h"
#include "VioTracker.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace LensEngine {

/**
 * @brief Сессия трекинга LensEngine (аналог ARSession в ARKit).
 *
 * Принимает сырые IMU и кадры камеры (время — часы телефона), выдаёт позу головы для рендера:
 *  - ориентация с задержкой ~0 (интегрирование гироскопа) + предсказание на момент показа
 *  - позиция из LiDAR (ICP по карте глубины + IMU), иначе из VIO (OpenVINS), иначе — модель шеи
 *  - LiDAR же убирает дрейф рысканья гироскопа и находит пол
 *  - ориентация камеры на момент экспозиции кадра (для репроекции passthrough-видео)
 *
 * Мир рендера как в ARKit: Y вверх, взгляд в начале — по -Z, метры.
 * Оси «глаза» как в OpenGL/Vulkan-камере: x вправо, y вверх, смотрит в -z.
 */
class TrackingSession {
public:
    struct Options {
        bool enableVio = true;
        bool enableLidar = true;
        bool neckModel = true;
        /// Поворот телефона в шлеме (если смотреть на экран): 0 — ландшафт, верх телефона слева; 180 — верх справа;
        /// 90 — портрет, верх вверху; 270 — портрет вверх ногами; -1 — авто по гравитации
        int mountRollDeg = -1;
    };

    struct EyePose {
        bool valid = false;
        int64_t phoneTimeNs = 0;
        glm::dquat orientation{1, 0, 0, 0};   // R_world_from_eye
        glm::dvec3 position{0.0};
        glm::dvec3 angularVelocity{0.0};      // оси устройства
        bool sixDof = false;
    };

    struct TrackingStatus {
        bool imuActive = false;
        bool vioAvailable = false;
        bool vioInitialized = false;
        int mountRollDeg = 0;
        bool mountAuto = true;
        glm::dvec3 gyroBias{0.0};
        std::string mode;
        bool lidarTracking = false;    // поза по LiDAR есть и свежая
        int lidarKeyframes = 0;
        double lidarRmseMm = 0.0;
        bool hasFloor = false;
        double floorY = -1.5;          // высота пола в мире рендера (Y вверх), м
        std::string worldMap;          // "нет", "ищу знакомое место…", "загружена"
        int anchors = 0;
        int roomPlanes = 0;
    };

    /// Якорь в мире (аналог WorldAnchor): живёт в карте комнаты и сохраняется вместе с ней
    struct Anchor {
        int id = 0;
        std::string name;
        glm::dvec3 position{0.0};             // мир рендера
        glm::dquat orientation{1, 0, 0, 0};
    };

    enum class MapState { NoMap, Relocalizing, Relocalized };

    /// Меш комнаты в мире рендера (Y вверх)
    struct RoomMesh {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<SceneReconstruction::Kind> kinds;
        std::vector<uint32_t> indices;
        uint64_t version = 0;
    };

    TrackingSession();
    ~TrackingSession();

    void start(const Options& options);
    void start() { start(Options{}); }
    void stop();

    /// IMU в осях устройства (CoreMotion), gyro rad/s, accel m/s^2 specific force
    void addImu(int64_t phoneTimeNs, const glm::dvec3& gyro, const glm::dvec3& accel, bool hasAccel);

    /// Кадр камеры (яркость) — для VIO. tNs — середина экспозиции.
    void addFrame(int64_t phoneTimeNs, const uint8_t* y, int width, int height, int stride,
                  float fx, float fy, float cx, float cy);

    /// Карта глубины LiDAR (метры), выровнена с кадром камеры. Тяжёлая (ICP ~2-5 мс) — вызывать не из сетевого потока.
    void addDepth(int64_t phoneTimeNs, const float* depth, int width, int height,
                  float fx, float fy, float cx, float cy);

    /// Крепление: -1 авто, 0/90/180/270 вручную
    void setMount(int rollDeg);

    /// Поза глаза, предсказанная на момент phoneTimeNs (обычно — время показа кадра на дисплее)
    EyePose predictEye(int64_t phoneTimeNs) const;

    /// R_world_from_cameraCV на момент phoneTimeNs (камера в осях OpenCV: x вправо, y вниз, z вперёд)
    bool cameraOrientationAt(int64_t phoneTimeNs, glm::dquat& out) const;

    /// Поворот из осей «глаза» (рендер) в оси камеры OpenCV — с учётом крепления телефона
    glm::dquat eyeToCameraRotation() const;

    void recenter();
    TrackingStatus status() const;

    // ---- Карта мира (сохранение между запусками) и якоря
    int addAnchor(const std::string& name, const glm::dvec3& position, const glm::dquat& orientation);
    bool updateAnchor(int id, const glm::dvec3& position, const glm::dquat& orientation);
    bool removeAnchor(int id);
    std::vector<Anchor> anchors() const;
    /// force = записать, даже если сохранённая карта ещё не найдена (новое место)
    bool saveWorldMap(const std::string& path, bool force = false) const;
    /// Загрузить карту: дальше идёт поиск знакомого места; когда найдено — якоря встают на свои места
    bool loadWorldMap(const std::string& path);
    MapState mapState() const { return m_mapState.load(); }

    // ---- Меш комнаты и плоскости (обновляются раз в секунду в своём потоке)
    RoomMesh roomMesh() const;
    std::vector<SceneReconstruction::Plane> roomPlanes() const;   // мир рендера
    uint64_t roomVersion() const { return m_roomVersion.load(); }

private:
    glm::dquat deviceFromEye() const;
    void updateMount(const glm::dvec3& accel);
    static int mountFromGravity(const glm::dvec3& g);

    Options m_options;
    HeadTracker m_head;
    std::unique_ptr<VioTracker> m_vio;
    LidarOdometry m_lidar;
    mutable std::mutex m_lidarMutex;          // m_lidar, карта, якоря
    SceneReconstruction m_recon;
    std::atomic<bool> m_reconDirty{false};
    std::atomic<uint64_t> m_roomVersion{0};
    std::thread m_sceneThread;
    std::atomic<bool> m_sceneRunning{false};
    std::mutex m_sceneWakeMutex;
    std::condition_variable m_sceneWake;
    int m_depthCounter = 0;
    // карта мира
    struct StoredAnchor {
        int id = 0;
        std::string name;
        LidarOdometry::Pose pose;             // мир LiDAR-карты (Z вверх)
    };
    std::vector<StoredAnchor> m_anchors;      // в текущем мире LiDAR
    std::vector<StoredAnchor> m_mapAnchors;   // из файла — ждут релокализации
    int m_nextAnchorId = 1;
    std::unique_ptr<LidarOdometry> m_savedLidar;
    std::string m_savedRecon;
    std::atomic<MapState> m_mapState{MapState::NoMap};
    int m_relocCursor = 0;
    bool m_relocPending = false;
    LidarOdometry::Pose m_relocCandidate;
    glm::dvec3 toRender(const glm::dvec3& lidarWorld) const;
    glm::dquat toRenderRotation(const glm::dquat& lidarWorld) const;
    glm::dvec3 fromRender(const glm::dvec3& render) const;
    glm::dquat fromRenderRotation(const glm::dquat& render) const;
    void applyRelocalization(const LidarOdometry::Pose& savedFromCurrent);
    void sceneLoop();
    std::atomic<int64_t> m_lidarFixT{0};
    int64_t m_lastDepthT = 0;
    std::atomic<int> m_lidarKeyframes{0};
    std::atomic<double> m_lidarRmse{0.0};
    std::atomic<bool> m_floorValid{false};
    std::atomic<double> m_floorZ{0.0};       // мир LiDAR-одометрии
    std::atomic<bool> m_mountAuto{true};
    std::atomic<int> m_mountRoll{0};
    std::atomic<bool> m_mountResolved{false};
    std::atomic<int64_t> m_lastImuT{0};
    glm::dvec3 m_mountAccelSum{0.0};
    int m_mountCount = 0;
    // Слежение за креплением после старта (телефон перевернули / вставили в шлем)
    glm::dvec3 m_gravityLp{0.0};
    int m_mountFlipCount = 0;
};

} // namespace LensEngine

#endif // TRACKINGSESSION_H
