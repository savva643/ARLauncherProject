#ifndef APPLICATION_H
#define APPLICATION_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

// Forward declarations
struct GLFWwindow;

namespace LensEngine {
    class LensEngineAPI;
    class TrackingSession;
    class EnvironmentProbe;
    class MarkerTracker;
}

namespace sensorstream {
    class StreamReceiver;
    struct DepthFrame;
    struct VideoFrame;
}

class Button;
class Text;
class Renderer;
class Scene;
class UIRenderer;
class ARLauncherAPI;
class FontRenderer;
class LiDARVisualizer;
class AudioEngine;

/**
 * @brief Главный класс приложения ARLauncher (GlaskiOS)
 *
 * Конвейер кадра (Vulkan):
 *   iPhone → SensorStream (сеть/декод в своих потоках) → LensEngine::TrackingSession (IMU 100 Гц)
 *   → beginFrame: ожидание слота GPU + acquire → LATE LATCH: поза предсказывается на момент показа,
 *     свежий видеокадр загружается и репроецируется под эту позу → 3D-сцена → present
 */
class Application {
public:
    Application();
    ~Application();

    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();

    // Получение компонентов
    GLFWwindow* getWindow() const { return m_window; }
    Renderer* getRenderer() const { return m_renderer.get(); }
    Scene* getScene() const { return m_scene.get(); }
    UIRenderer* getUIRenderer() const { return m_uiRenderer.get(); }

    // Статус
    bool isRunning() const { return m_running; }
    void requestExit() { m_running = false; }

private:
    bool initializeWindow();
    bool initializeRenderer();
    bool initializeScene();
    bool initializeUI();
    bool initializeLensEngine();
    bool initializeSensorStream();

    void update(float deltaTime);
    void render();
    void latchFrame();          // вызывается рендером после acquire, до render pass
    void updateSplash();
    void printStats();
    void depthWorker();

    // GLFW callbacks
    static void onWindowClose(GLFWwindow* window);
    static void onWindowResize(GLFWwindow* window, int width, int height);
    static void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onMouseMove(GLFWwindow* window, double x, double y);
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<UIRenderer> m_uiRenderer;
    std::unique_ptr<ARLauncherAPI> m_launcherAPI;
    std::unique_ptr<LensEngine::LensEngineAPI> m_lensEngine;
    std::unique_ptr<FontRenderer> m_fontRenderer;
    std::unique_ptr<LiDARVisualizer> m_lidarVisualizer;

    // Новый конвейер сенсоров
    std::unique_ptr<sensorstream::StreamReceiver> m_stream;
    std::unique_ptr<LensEngine::TrackingSession> m_tracking;
    std::unique_ptr<LensEngine::EnvironmentProbe> m_envProbe;   // свет и отражения по видео
    std::unique_ptr<LensEngine::MarkerTracker> m_markers;      // метки ArUco -> якоря в мире
    std::vector<uint8_t> m_envTexture;
    bool m_markersEnabled = true;     // ARL_MARKERS
    // Зрение (свет комнаты, метки) — отдельный поток, ~10 Гц: декодер видео не ждёт
    std::thread m_visionThread;
    std::mutex m_visionMutex;
    std::condition_variable m_visionCv;
    std::shared_ptr<const sensorstream::VideoFrame> m_visionFrame;
    std::atomic<bool> m_visionRunning{false};
    std::atomic<int> m_markersInView{0};
    void visionWorker();

    // Пространственный звук (OpenAL HRTF): источники на объектах сцены, глушатся реальными стенами (LiDAR)
    std::unique_ptr<AudioEngine> m_audio;
    struct AudioSource {
        int sourceId = -1;
        size_t objectIndex = 0;
        float occlusion = 0.0f;
    };
    std::vector<AudioSource> m_audioSources;
    void initializeAudio();
    void updateAudio();
    float lidarOcclusion(const glm::vec3& worldPos);   // 1 — реальная поверхность ближе точки, 0 — нет, -1 — не видно

    bool m_running;
    bool m_initialized;

    int m_windowWidth;
    int m_windowHeight;

    // Дисплей
    double m_refreshHz = 60.0;
    double m_predictLeadMs = -1.0;     // <0 — авто по частоте и present mode
    float m_eyeHFovDeg = 0.0f;         // 0 — как у камеры iPhone
    bool m_reprojection = true;        // P — сравнить с/без репроекции
    bool m_prediction = true;          // O — сравнить с/без предсказания
    bool m_stereo = false;             // ARL_STEREO=1 — side-by-side для дисплея шлема
    float m_ipdMeters = 0.063f;        // ARL_IPD_MM

    // Результат late latching (заполняется в latchFrame, используется в render)
    int m_eyeCount = 1;
    glm::mat4 m_viewMatrix[2] = {glm::mat4(1.0f), glm::mat4(1.0f)};
    glm::mat4 m_projMatrix[2] = {glm::mat4(1.0f), glm::mat4(1.0f)};
    bool m_frameHasPose = false;
    bool m_frameHasVideo = false;
    struct PassthroughState;
    std::unique_ptr<PassthroughState> m_pt;

    // Статистика
    float m_deltaTime;
    float m_lastFrameTime;
    uint64_t m_renderFrames = 0;
    std::chrono::steady_clock::time_point m_statsStart;
    double m_poseAgeMsSum = 0;
    double m_videoAgeMsSum = 0;
    uint64_t m_videoAgeCount = 0;

    // Splash: 0-3 c надписи, 3-6 c появление видео/3D/UI, 6-9 c надписи гаснут
    bool m_splashActive = true;
    bool m_splashStarted = false;
    std::chrono::steady_clock::time_point m_splashStart;
    float m_videoOpacity = 0.0f;
    float m_3dObjectsOpacity = 0.0f;
    float m_uiOpacity = 0.0f;
    float m_titleOpacity = 0.0f;

    // LiDAR → LensEngine (DLIO, облако точек) в отдельном потоке, чтобы не задерживать IMU
    std::thread m_depthThread;
    std::mutex m_depthMutex;
    std::condition_variable m_depthCv;
    std::shared_ptr<sensorstream::DepthFrame> m_pendingDepth;
    std::shared_ptr<sensorstream::DepthFrame> m_overlayDepth;   // для наложения LiDAR в рендере (клавиша L)
    bool m_depthOverlay = false;
    // Реализм 3D-объектов (клавиши K, H, E; переменные ARL_OCCLUSION / ARL_SHADOWS / ARL_LIGHTING)
    bool m_occlusion = true;          // реальные предметы закрывают виртуальные (LiDAR)
    bool m_shadows = true;            // тени объектов на реальный пол
    bool m_lighting = true;           // свет и отражения по видео комнаты
    int m_mountSetting = -1;          // M: -1 авто, 0/90/180/270
    std::atomic<int> m_depthW{0};
    std::atomic<int> m_depthH{0};

    // Панель статуса (живые данные потока и трекинга)
    std::shared_ptr<Text> m_cameraText;
    std::shared_ptr<Text> m_poseText;
    std::shared_ptr<Text> m_imuText;
    std::shared_ptr<Text> m_connectionText;
    std::chrono::steady_clock::time_point m_hudUpdate{};
    void updateHud();
    std::atomic<bool> m_depthRunning{false};
};

#endif // APPLICATION_H
