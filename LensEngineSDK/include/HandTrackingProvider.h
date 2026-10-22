#ifndef HANDTRACKINGPROVIDER_H
#define HANDTRACKINGPROVIDER_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace LensEngine {

/**
 * @brief Отслеживание рук (аналог HandTrackingProvider в visionOS).
 *
 * MediaPipe (ONNX через OpenCV DNN): детектор ладоней 192x192 -> поворот кадра по ладони ->
 * 21 сустав 224x224 (+ метрические координаты суставов относительно руки). Глубина руки —
 * LiDAR по ладони (если есть карта глубины), иначе по размеру ладони. Итог — суставы в мире.
 *
 * Жесты как в visionOS: щипок (большой + указательный), кулак, луч указания (от плеча через щипок).
 * Работает с любой камерой — в настоящем шлеме сюда же подаются кадры с его камер.
 *
 * Суставы (как в MediaPipe): 0 запястье; 1-4 большой; 5-8 указательный; 9-12 средний; 13-16 безымянный;
 * 17-20 мизинец (последний в каждом пальце — кончик).
 */
class HandTrackingProvider {
public:
    static constexpr int kJoints = 21;

    struct Hand {
        bool right = false;
        float confidence = 0.0f;
        std::array<glm::vec2, kJoints> image{};   // пиксели входного кадра
        std::array<glm::vec3, kJoints> world{};   // мир (если задана поза камеры), метры
        std::array<glm::vec3, kJoints> local{};   // метрические координаты суставов относительно руки (модель)
        bool depthFromLidar = false;
        // Жесты
        float pinchDistance = 1.0f;   // м, между кончиками большого и указательного
        bool pinching = false;
        bool fist = false;
        glm::vec3 pinchPoint{0.0f};   // мир
        glm::vec3 palmCenter{0.0f};
        glm::vec3 palmNormal{0.0f, 0.0f, 1.0f};
        glm::vec3 aimOrigin{0.0f};    // луч указания (мир)
        glm::vec3 aimDirection{0.0f, 0.0f, -1.0f};
        int64_t tNs = 0;
    };

    struct Camera {
        float fx = 0, fy = 0, cx = 0, cy = 0;    // интринсики в пикселях кадра
        glm::quat worldFromCam{1, 0, 0, 0};      // OpenCV-оси камеры в мире
        glm::vec3 position{0.0f};                // камера в мире
        glm::vec3 headPosition{0.0f};            // голова (для луча от плеча)
    };

    struct Depth {
        const float* data = nullptr;             // метры, выровнена с кадром камеры
        int width = 0, height = 0;
        float fx = 0, fy = 0, cx = 0, cy = 0;
    };

    static bool available();

    HandTrackingProvider();
    ~HandTrackingProvider();

    /// modelDir — папка с palm_detection_mediapipe_2023feb.onnx и handpose_estimation_mediapipe_2023feb.onnx
    /// (для OpenCV < 4.7 — *_compat.onnx, см. tools/onnx_clip_compat.py)
    bool initialize(const std::string& modelDir);
    bool ready() const { return m_ready; }

    /// Кадр NV12 (или только яркость: uv = nullptr) -> найденные руки
    std::vector<Hand> process(int64_t tNs, const uint8_t* y, int yStride, const uint8_t* uv, int uvStride,
                              int width, int height, const Camera& camera, const Depth* depth = nullptr);
    /// Кадр BGR 8 бит (тесты, другие камеры)
    std::vector<Hand> processBGR(int64_t tNs, const uint8_t* bgr, int width, int height, int stride,
                                 const Camera& camera, const Depth* depth = nullptr);

    std::vector<Hand> hands() const;

    /// Поиск моделей: ARL_MODELS, ./models, ../models, ../../models, рядом с исходниками
    static std::string findModelDir();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_ready = false;
    mutable std::mutex m_mutex;
    std::vector<Hand> m_hands;
};

} // namespace LensEngine

#endif // HANDTRACKINGPROVIDER_H
