#ifndef SCENERECONSTRUCTION_H
#define SCENERECONSTRUCTION_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace LensEngine {

/**
 * @brief Меш комнаты и плоскости (аналог SceneReconstructionProvider + PlaneDetectionProvider в visionOS).
 *
 * Карты глубины LiDAR с позой камеры сливаются в TSDF (разреженные блоки 8x8x8 вокселей по 4 см).
 * Из TSDF строится меш (surface nets) с классами поверхностей, как ARMeshClassification:
 * пол, стена, потолок, стол, прочее. По мешу ищутся плоскости: горизонтальные (пол, столы, потолок)
 * и вертикальные (стены) — с центром, нормалью и размерами.
 *
 * Мир — как у LiDAR-одометрии: Z вверх, метры.
 */
class SceneReconstruction {
public:
    enum class Kind : uint8_t { Unclassified = 0, Floor, Wall, Ceiling, Table, Other };
    static const char* kindName(Kind k);

    struct Config {
        float voxel = 0.04f;        // м
        float truncation = 0.12f;   // м
        float maxDepth = 4.0f;
        int pixelStep = 2;          // выборка карты глубины при размещении блоков
        float maxWeight = 40.0f;
    };

    struct Mesh {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<Kind> kinds;            // на вершину
        std::vector<uint32_t> indices;      // треугольники
        uint64_t version = 0;
    };

    struct Plane {
        int id = 0;
        Kind kind = Kind::Unclassified;
        bool horizontal = true;
        glm::vec3 center{0.0f};
        glm::vec3 normal{0.0f, 0.0f, 1.0f};
        glm::vec3 axisU{1.0f, 0.0f, 0.0f};  // направления сторон прямоугольника
        glm::vec3 axisV{0.0f, 1.0f, 0.0f};
        glm::vec2 halfExtent{0.0f};         // полуразмеры по axisU / axisV, м
        float area = 0.0f;
    };

    SceneReconstruction();
    explicit SceneReconstruction(const Config& cfg);

    void reset();

    /// Карта глубины (метры, z камеры) + поза камеры OpenCV в мире
    void integrate(const float* depth, int width, int height, float fx, float fy, float cx, float cy,
                   const glm::dquat& worldFromCam, const glm::dvec3& camPosition);

    /// Пересчитать меш и плоскости (десятки мс — не каждый кадр). floorZ < -1e8 — найти пол самим.
    void update(double floorZ = -1e9);

    Mesh mesh() const;
    std::vector<Plane> planes() const;
    size_t blockCount() const;
    double floorZ() const;

    /// Сохранение/загрузка объёма (для карты мира)
    void save(std::ostream& out) const;
    bool load(std::istream& in);

    /// Перенос всего объёма жёстким преобразованием (после релокализации) — вокселизация заново
    void transform(const glm::dquat& rotation, const glm::dvec3& translation);

private:
    struct Voxel {
        float sdf = 1.0f;
        float weight = 0.0f;
    };
    static constexpr int kBlock = 8;
    struct Block {
        Voxel v[kBlock * kBlock * kBlock];
    };
    using Key = uint64_t;
    static Key key(int bx, int by, int bz);
    static void unkey(Key k, int& bx, int& by, int& bz);
    const Voxel* voxelAt(int gx, int gy, int gz) const;

    Mesh buildMesh(double floorZ) const;
    std::vector<Plane> findPlanes(const Mesh& mesh, double floorZ) const;

    Config m_cfg;
    mutable std::mutex m_mutex;
    std::unordered_map<Key, Block> m_blocks;
    Mesh m_mesh;
    std::vector<Plane> m_planes;
    int m_nextPlaneId = 1;
    double m_floorZ = -1e9;
};

} // namespace LensEngine

#endif // SCENERECONSTRUCTION_H
