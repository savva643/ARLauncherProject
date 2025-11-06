#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <string>
#include <functional>

// Forward declarations
struct GLFWwindow;
namespace LensEngine {
    class LensEngineAPI;
}
#ifdef USE_SENSOR_CONNECTOR
namespace SensorConnector {
    class SensorConnectorCore;
}
#include <QtCore/QtGlobal>
#endif
class Button;
class Text;

class Renderer;
class Scene;
class UIRenderer;

/**
 * @brief Главный класс приложения ARLauncher
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
#ifdef USE_SENSOR_CONNECTOR
    bool initializeSensorConnector();
#endif
    
    void update(float deltaTime);
    void render();
    
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
    std::unique_ptr<LensEngine::LensEngineAPI> m_lensEngine;
#ifdef USE_SENSOR_CONNECTOR
    std::unique_ptr<SensorConnector::SensorConnectorCore> m_sensorConnector;
#endif
    
    bool m_running;
    bool m_initialized;
    
    int m_windowWidth;
    int m_windowHeight;
    
    // Статистика
    float m_deltaTime;
    float m_lastFrameTime;

#ifdef USE_SENSOR_CONNECTOR
    // Splash overlay state
    bool m_splashActive = true;
    qint64 m_splashStartMs = 0;
    float m_videoOpacity = 0.0f;      // Opacity видео фона (0.0 - 1.0)
    float m_3dObjectsOpacity = 0.0f;  // Opacity 3D объектов (0.0 - 1.0)
    float m_uiOpacity = 0.0f;          // Opacity UI элементов (0.0 - 1.0)
    float m_titleOpacity = 0.0f;       // Opacity надписей "Spatial Home" и "GlaskiOS" (0.0 - 1.0)
#endif
};

#endif // APPLICATION_H

