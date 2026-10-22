#ifndef MARKERTRACKER_H
#define MARKERTRACKER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <mutex>
#include <vector>

namespace LensEngine {

/**
 * @brief Метки ArUco (аналог ARImageAnchor в ARKit): распечатанная метка становится якорем в мире.
 *
 * Словарь DICT_4X4_50 (метки 0..49; распечатать: https://chev.me/arucogen, 4x4). Размер метки задаётся
 * в метрах — от него зависит расстояние. Поза каждой метки в мире сглаживается по нескольким кадрам,
 * поэтому виртуальный объект на ней стоит неподвижно, даже когда метка ушла из кадра.
 *
 * Работает на кадре яркости (Y), который уже приходит для трекинга. Нужен OpenCV (aruco).
 */
class MarkerTracker {
public:
    struct Anchor {
        int id = -1;
        glm::dvec3 position{0.0};             // центр метки, мир
        glm::dquat orientation{1, 0, 0, 0};   // оси метки: x вправо, y вверх по метке, z из метки
        int64_t lastSeenNs = 0;
        int detections = 0;
    };

    static bool available();

    explicit MarkerTracker(double markerSizeMeters = 0.10);

    /// worldFromCam / camPosition — поза камеры (OpenCV) в мире на момент кадра
    /// Возвращает число найденных в кадре меток.
    int process(int64_t tNs, const uint8_t* y, int width, int height, int stride,
                float fx, float fy, float cx, float cy,
                const glm::dquat& worldFromCam, const glm::dvec3& camPosition);

    std::vector<Anchor> anchors() const;
    void setMarkerSize(double meters) { m_markerSize = meters; }

private:
    double m_markerSize;
    mutable std::mutex m_mutex;
    std::vector<Anchor> m_anchors;
};

} // namespace LensEngine

#endif // MARKERTRACKER_H
