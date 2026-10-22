#ifndef LIDARODOMETRY_H
#define LIDARODOMETRY_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <deque>
#include <iosfwd>
#include <vector>

namespace LensEngine {

/**
 * @brief Позиция по LiDAR iPhone + IMU (6DoF без дрейфа в знакомом месте).
 *
 * Каждая карта глубины (256x192, метры) совмещается с ключевым кадром point-to-plane ICP
 * (проективная ассоциация, как в KinectFusion). Поворот между кадрами берётся из IMU — ICP только
 * уточняет его и находит смещение. Ключевые кадры образуют карту комнаты: вернувшись в знакомое место,
 * трекер совмещается со старым ключевым кадром, и накопленный дрейф пропадает.
 *
 * Мир — как у HeadTracker (Z вверх, метры); поза камеры в осях OpenCV (x вправо, y вниз, z вперёд).
 * Попутно по горизонтальным точкам ищется пол (для теней).
 */
class LidarOdometry {
public:
    struct Config {
        int step = 2;                    // 256x192 -> 128x96 точек
        float minDepth = 0.15f;
        float maxDepth = 5.0f;
        int iterations = 12;
        float maxPointDist = 0.20f;      // ассоциация в первой итерации, м (дальше сужается до 0.05)
        float minNormalDot = 0.7f;
        float keyframeDist = 0.25f;      // новый ключевой кадр через 25 см ...
        float keyframeAngleDeg = 20.0f;  // ... или 20°
        float minInlierRatio = 0.30f;
        float maxRmse = 0.04f;
        size_t maxKeyframes = 300;
        double rotPrior = 0.05;          // вес поворота от IMU, м^2 на радиан^2 на точку
        double transPrior = 0.02;        // вес предсказанного смещения на точку
    };

    struct Pose {
        glm::dquat q{1, 0, 0, 0};   // R_world_from_camCV
        glm::dvec3 t{0.0};          // положение камеры в мире
    };

    struct Result {
        bool ok = false;            // поза из ICP (иначе — предсказание по IMU)
        Pose pose;
        int64_t tNs = 0;
        double rmse = 0.0;
        double inlierRatio = 0.0;
        bool degenerate = false;    // геометрии мало (одна стена): смещение вдоль неё — по предсказанию
        bool newKeyframe = false;
        bool revisit = false;       // совместились со старым ключевым кадром (дрейф сброшен)
        int keyframes = 0;
        bool hasFloor = false;
        double floorZ = 0.0;        // высота пола в мире
    };

    LidarOdometry();
    explicit LidarOdometry(const Config& cfg);

    void reset();

    /// depth — метры (0/NaN — нет данных); интринсики в пикселях карты глубины;
    /// imuWorldFromCam — ориентация камеры по IMU на момент карты (тот же мир);
    /// imuDelta — смещение камеры с прошлой карты по IMU (если есть), иначе — постоянная скорость
    Result process(int64_t tNs, const float* depth, int width, int height,
                   float fx, float fy, float cx, float cy, const glm::dquat& imuWorldFromCam,
                   const glm::dvec3* imuDelta = nullptr);

    bool initialized() const { return !m_keyframes.empty(); }
    size_t keyframeCount() const { return m_keyframes.size(); }
    bool hasPose() const { return m_havePose; }
    Pose pose() const { return m_pose; }

    // ---- Карта мира: сохранение и «где я?» после перезапуска
    void save(std::ostream& out) const;
    bool load(std::istream& in);            // только ключевые кадры; текущей позы нет

    /// Найти позу камеры в мире этой карты без приближения — известна только гравитация
    /// (imuWorldFromCam — ориентация в любом мире с Z вверх). Перебирает ключевые кадры и рысканье,
    /// за вызов — не больше budget совмещений (cursor продолжает перебор между вызовами).
    bool relocalize(const float* depth, int width, int height, float fx, float fy, float cx, float cy,
                    const glm::dquat& imuWorldFromCam, Pose& worldFromCam, double& quality, int budget, int& cursor) const;

    /// Перевести всю карту и текущую позу в другой мир: p' = T * p
    void transform(const Pose& T);
    /// Добавить ключевые кадры другой карты (уже в том же мире)
    void appendKeyframes(const LidarOdometry& other);

private:
    struct Frame {
        int w = 0, h = 0;
        float fx = 0, fy = 0, cx = 0, cy = 0;     // проекция в сетку w x h
        std::vector<glm::vec3> v;                 // точки в осях камеры (z = 0 — нет)
        std::vector<glm::vec3> n;                 // нормали (0 — нет)
        int valid = 0;
    };
    struct Keyframe {
        int id = 0;
        Pose pose;
        Frame frame;
    };
    struct IcpResult {
        bool ok = false;
        Pose keyFromCam;
        double rmse = 0.0;
        double inlierRatio = 0.0;
        bool degenerate = false;
    };

    struct AlignParams {
        float startDist;            // окно ассоциации в первой итерации, м
        int iterations;
        double rotPriorScale;       // доля обычного IMU-приора на поворот
        double transPriorScale;
        double maxAngleChangeDeg;   // насколько ICP может повернуть начальное приближение
        double minInlierRatio;
    };
    void buildFrame(const float* depth, int width, int height, float fx, float fy, float cx, float cy, Frame& out) const;
    static void computeNormals(Frame& f);
    IcpResult align(const Frame& src, const Keyframe& key, const Pose& keyFromCamInit,
                    const AlignParams* params = nullptr) const;
    void updateFloor(const Frame& f, const Pose& pose);
    int findKeyframeNear(const Pose& pose, int exclude) const;

    Config m_cfg;
    std::deque<Keyframe> m_keyframes;
    int m_current = -1;              // индекс в m_keyframes
    int m_nextId = 0;
    bool m_havePose = false;
    Pose m_pose;
    glm::dquat m_lastImu{1, 0, 0, 0};
    glm::dvec3 m_velocity{0.0};
    int64_t m_lastT = 0;
    int m_failures = 0;
    bool m_floorValid = false;
    double m_floorZ = 0.0;
};

} // namespace LensEngine

#endif // LIDARODOMETRY_H
