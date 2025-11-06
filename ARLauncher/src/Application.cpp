#ifdef USE_SENSOR_CONNECTOR
#include <QtCore/qdatastream.h>
#include <QtCore/qmetatype.h>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <QGuiApplication>
#include <QTimer>
#include "SensorConnector.h"
#endif

#include "Application.h"
#include "Renderer.h"
#include "Scene.h"
#include "Camera.h"
#include "UIRenderer.h"
#include "Button.h"
#include "Text.h"
#include "Style.h"
#include "LensEngineAPI.h"

#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#define GLFW_INCLUDE_NONE
#include <GL/gl.h>
#endif
#include <iostream>
#include <chrono>
#include <memory>
#include <glm/glm.hpp>

Application::Application()
    : m_window(nullptr)
    , m_running(false)
    , m_initialized(false)
    , m_windowWidth(1920)
    , m_windowHeight(1080)
    , m_deltaTime(0.0f)
    , m_lastFrameTime(0.0f)
{
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize(int argc, char* argv[])
{
    if (m_initialized) {
        return true;
    }
    
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Инициализация компонентов
    if (!initializeWindow()) {
        return false;
    }
    
    if (!initializeRenderer()) {
        return false;
    }
    
    if (!initializeScene()) {
        return false;
    }
    
    if (!initializeUI()) {
        return false;
    }
    
    if (!initializeLensEngine()) {
        return false;
    }
    
#ifdef USE_SENSOR_CONNECTOR
    if (!initializeSensorConnector()) {
        std::cerr << "Warning: SensorConnector initialization failed, continuing without it" << std::endl;
    }
#endif
    
    m_initialized = true;
    m_running = true;
    
    return true;
}

void Application::run()
{
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (m_running && !glfwWindowShouldClose(m_window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
        m_deltaTime = duration.count() / 1000000.0f;
        lastTime = currentTime;
        
        glfwPollEvents();
        
#ifdef USE_SENSOR_CONNECTOR
        // Обрабатываем Qt события (для SensorConnector)
        if (QGuiApplication::instance()) {
            QGuiApplication::processEvents();
        }
#endif
        
        update(m_deltaTime);
        render();
        
        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
#ifdef USE_SENSOR_CONNECTOR
    if (m_sensorConnector) {
        m_sensorConnector->stopServers();
        m_sensorConnector.reset();
    }
#endif
    
    m_uiRenderer.reset();
    m_scene.reset();
    m_renderer.reset();
    m_lensEngine.reset();
    
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    
    glfwTerminate();
    m_initialized = false;
}

bool Application::initializeWindow()
{
#ifdef USE_VULKAN
    // Для Vulkan не нужны OpenGL hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
    // Для OpenGL настраиваем контекст
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // Используем Compatibility Profile для legacy OpenGL
#endif
    
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "ARLauncher", nullptr, nullptr);
    
    if (!m_window) {
        const char* description;
        int code = glfwGetError(&description);
        std::cerr << "Failed to create GLFW window (code: " << code << "): " << (description ? description : "unknown error") << std::endl;
        return false;
    }
    
#ifndef USE_VULKAN
    // Только для OpenGL нужен контекст
    glfwMakeContextCurrent(m_window);
    if (!glfwGetCurrentContext()) {
        std::cerr << "Failed to make OpenGL context current" << std::endl;
        return false;
    }
#endif
    
    glfwSetWindowUserPointer(m_window, this);
    
    // Callbacks
    glfwSetWindowCloseCallback(m_window, onWindowClose);
    glfwSetWindowSizeCallback(m_window, onWindowResize);
    glfwSetKeyCallback(m_window, onKeyPress);
    glfwSetCursorPosCallback(m_window, onMouseMove);
    glfwSetMouseButtonCallback(m_window, onMouseButton);
    
    return true;
}

bool Application::initializeRenderer()
{
#ifdef USE_VULKAN
    m_renderer = createRenderer(true); // Используем Vulkan если доступен
#else
    m_renderer = createRenderer(false); // Используем OpenGL
#endif
    
    if (!m_renderer) {
        std::cerr << "Failed to create renderer" << std::endl;
        return false;
    }
    
    if (!m_renderer->initialize(m_window)) {
        std::cerr << "Failed to initialize renderer (OpenGL context issue?)" << std::endl;
        return false;
    }
    
    return true;
}

bool Application::initializeScene()
{
    m_scene = std::make_unique<Scene>();
    
    if (!m_scene->initialize()) {
        std::cerr << "Failed to initialize scene" << std::endl;
        return false;
    }
    
    m_scene->createDemoScene(m_renderer.get());
    
    return true;
}

bool Application::initializeUI()
{
    m_uiRenderer = std::make_unique<UIRenderer>(m_renderer.get());
    
    if (!m_uiRenderer->initialize()) {
        std::cerr << "Failed to initialize UI renderer" << std::endl;
        return false;
    }
    
    // Создаем AR UI элементы с красивым стилем
    auto startButton = std::make_shared<Button>("🚀 Start AR");
    startButton->setPosition(glm::vec2(20.0f, 20.0f));
    startButton->setSize(glm::vec2(180.0f, 50.0f));
    startButton->setStyle(std::make_shared<Style>(Style::createARButtonStyle()));
    startButton->setOnClick([]() {
        std::cout << "AR Started" << std::endl;
    });
    m_uiRenderer->addElement(startButton);
    
    // Виджет для отображения данных камеры
    auto cameraInfoText = std::make_shared<Text>("📷 Camera: Waiting...");
    cameraInfoText->setPosition(glm::vec2(20.0f, 80.0f));
    cameraInfoText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(cameraInfoText);
    
    // Виджет для отображения позы
    auto poseText = std::make_shared<Text>("📍 Position: (0, 0, 0)");
    poseText->setPosition(glm::vec2(20.0f, 110.0f));
    poseText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(poseText);
    
    // Виджет для IMU данных
    auto imuText = std::make_shared<Text>("⚡ IMU: No data");
    imuText->setPosition(glm::vec2(20.0f, 140.0f));
    imuText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(imuText);
    
    // Виджет для статуса подключения iPhone
    auto connectionText = std::make_shared<Text>("📡 iPhone: Disconnected");
    connectionText->setPosition(glm::vec2(20.0f, 170.0f));
    connectionText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(connectionText);
    
    // Кнопка подключения
    auto connectButton = std::make_shared<Button>("🔌 Connect iPhone");
    connectButton->setPosition(glm::vec2(20.0f, 200.0f));
    connectButton->setSize(glm::vec2(180.0f, 50.0f));
    connectButton->setStyle(std::make_shared<Style>(Style::createARButtonStyle()));
    connectButton->setOnClick([]() {
        std::cout << "Connecting to iPhone..." << std::endl;
    });
    m_uiRenderer->addElement(connectButton);
    
    return true;
}

bool Application::initializeLensEngine()
{
    m_lensEngine = std::make_unique<LensEngine::LensEngineAPI>();
    
    if (!m_lensEngine->initialize()) {
        std::cerr << "Failed to initialize LensEngine" << std::endl;
        return false;
    }
    
    // Установка колбэков
    m_lensEngine->setPoseCallback([this](const LensEngine::CameraPose& pose) {
        if (m_scene) {
            m_scene->updateCameraFromAR(pose.position, pose.rotation);
        }
    });
    
    return true;
}

#ifdef USE_SENSOR_CONNECTOR
bool Application::initializeSensorConnector()
{
    // Создаем QGuiApplication если его нет (для Qt event loop + QFont/QPainter)
    static int argc = 1;
    static char* argv[] = {(char*)"ARLauncher"};
    if (!QGuiApplication::instance()) {
        new QGuiApplication(argc, argv);
    }
    
    m_sensorConnector = std::make_unique<SensorConnector::SensorConnectorCore>();
    qRegisterMetaType<SensorConnector::SensorData>("SensorConnector::SensorData");
    
    if (!m_sensorConnector->initialize()) {
        std::cerr << "Failed to initialize SensorConnector" << std::endl;
        return false;
    }
    
    // Подключаем сигналы для получения декодированных RGB кадров с камеры iPhone
    QObject::connect(m_sensorConnector.get(), &SensorConnector::SensorConnectorCore::frameDecoded,
                     [this](const QImage& frame, quint64 sequenceNumber) {
                         if (m_renderer && !frame.isNull()) {
                            // Преобразуем QImage в RGB и наложим splash-оверлей (fade)
                            QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);

                            // Splash анимация:
                            // 0-2s: черный экран
                            // 2-3s: fade in "Spatial Home" и "GlaskiOS" + камера/мир/UI
                            // 3-6s: все видно
                            // 6-9s: fade out названия, камера/мир/UI остаются
                            if (m_splashActive) {
                                qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                                if (m_splashStartMs == 0) m_splashStartMs = nowMs;
                                qreal elapsed = qreal(nowMs - m_splashStartMs) / 1000.0; // секунды
                                if (elapsed >= 9.0) {
                                    m_splashActive = false;
                                    elapsed = 9.0;
                                }

                                qreal cameraOpacity = 0.0;
                                qreal titleOpacity = 0.0;

                                if (elapsed < 2.0) {
                                    // 0-2s: черный экран
                                    cameraOpacity = 0.0;
                                    titleOpacity = 0.0;
                                } else if (elapsed < 3.0) {
                                    // 2-3s: fade in
                                    qreal fadeIn = (elapsed - 2.0) / 1.0;
                                    cameraOpacity = fadeIn;
                                    titleOpacity = fadeIn;
                                } else if (elapsed < 6.0) {
                                    // 3-6s: все видно
                                    cameraOpacity = 1.0;
                                    titleOpacity = 1.0;
                                } else {
                                    // 6-9s: fade out названия
                                    qreal fadeOut = (9.0 - elapsed) / 3.0;
                                    cameraOpacity = 1.0;
                                    titleOpacity = fadeOut;
                                }

                                QPainter p(&rgbFrame);
                                p.setRenderHint(QPainter::Antialiasing, true);

                                if (elapsed < 2.0) {
                                    // Полностью черный экран
                                    p.fillRect(rgbFrame.rect(), QColor(0, 0, 0, 255));
                                } else {
                                    // Применяем opacity к кадру камеры
                                    if (cameraOpacity < 1.0) {
                                        QImage cameraFrame = rgbFrame.copy();
                                        p.fillRect(rgbFrame.rect(), QColor(0, 0, 0, 255));
                                        p.setOpacity(cameraOpacity);
                                        p.drawImage(0, 0, cameraFrame);
                                        p.setOpacity(1.0);
                                    }
                                    // Затемнение для названия
                                    if (titleOpacity > 0.0) {
                                        QColor overlay(0, 0, 0, int(180 * titleOpacity));
                                        p.fillRect(rgbFrame.rect(), overlay);
                                    }
                                }

                                // Титры (только если titleOpacity > 0)
                                if (titleOpacity > 0.0 && elapsed >= 2.0) {
                                    QFont titleFont;
                                    titleFont.setFamily("Sans Serif");
                                    titleFont.setBold(true);
                                    titleFont.setPointSizeF(std::max(24.0, rgbFrame.width() * 0.045));
                                    p.setFont(titleFont);
                                    p.setPen(QColor(255, 255, 255, int(255 * titleOpacity)));
                                    QString title = QString::fromUtf8("Spatial Home");
                                    QFontMetrics fmTitle(titleFont);
                                    int xTitle = (rgbFrame.width() - fmTitle.horizontalAdvance(title)) / 2;
                                    int yTitle = int(rgbFrame.height() * 0.42);
                                    p.drawText(xTitle, yTitle, title);

                                    QFont subFont;
                                    subFont.setFamily("Sans Serif");
                                    subFont.setBold(false);
                                    subFont.setPointSizeF(std::max(16.0, rgbFrame.width() * 0.018));
                                    p.setFont(subFont);
                                    QString sub = QString::fromUtf8("GlaskiOS");
                                    QFontMetrics fmSub(subFont);
                                    int xSub = (rgbFrame.width() - fmSub.horizontalAdvance(sub)) / 2;
                                    int ySub = int(rgbFrame.height() * 0.42 + fmTitle.height() * 1.8);
                                    p.setPen(QColor(255, 255, 255, int(255 * titleOpacity)));
                                    p.drawText(xSub, ySub, sub);
                                }
                            }
                             
                             if (!rgbFrame.isNull()) {
                                 uint32_t width = static_cast<uint32_t>(rgbFrame.width());
                                 uint32_t height = static_cast<uint32_t>(rgbFrame.height());
                                 const uint8_t* rgbData = rgbFrame.constBits();
                                 
                                 // Рендерим видео фон для AR
                                 m_renderer->renderVideoBackground(rgbData, width, height);
                                 
                                 static int frameCount = 0;
                                 if (frameCount++ % 60 == 0) {
                                     std::cout << "📹 RGB Frame received: " << width << "x" << height 
                                               << " (Seq: " << sequenceNumber << ")" << std::endl;
                                 }
                             }
                         }
                     });
    
    // Подключаем сигналы для получения других данных (IMU, LiDAR и т.д.)
    QObject::connect(m_sensorConnector.get(), &SensorConnector::SensorConnectorCore::dataReceived,
                     [this](const SensorConnector::SensorData& data) {
                         // Передаем данные в LensEngine
                         if (m_lensEngine) {
                             // TODO: Преобразовать SensorData в формат LensEngine
                             // m_lensEngine->processSensorData(data);
                         }
                     });
    
    // Запускаем серверы на порту 9000 (TCP и UDP)
    m_sensorConnector->startServers(9000, 9000);
    
    std::cout << "✅ SensorConnector initialized" << std::endl;
    std::cout << "   TCP Server: port 9000" << std::endl;
    std::cout << "   UDP Server: port 9000" << std::endl;
    std::cout << "   USB Server: port 9001" << std::endl;
    std::cout << "   Waiting for iPhone connection..." << std::endl;
    
    return true;
}
#endif

void Application::update(float deltaTime)
{
    if (m_scene) {
        m_scene->update(deltaTime);
    }
}

void Application::render()
{
    if (!m_renderer) {
        return;
    }
    
    m_renderer->beginFrame();
    
    // ВАЖНО: Порядок рендеринга для AR:
    // 1. Сначала рендерим видео фон с камеры iPhone (renderVideoBackground вызывается асинхронно через сигнал)
    // 2. Затем рендерим 3D объекты поверх видео (AR наложение)
    // 3. Наконец рендерим UI поверх всего
    
    // Рендеринг 3D объектов поверх видео фона (AR наложение)
    if (m_scene) {
        auto camera = m_scene->getCamera();
        if (camera) {
            m_renderer->setCameraMatrix(camera->getViewMatrix(), camera->getProjectionMatrix());
        }
        
        auto transforms = m_scene->getObjectTransforms();
        auto meshIds = m_scene->getVisibleMeshIds();
        m_renderer->render3DObjects(transforms, meshIds);
    }
    
    // Рендеринг UI поверх всего
    if (m_uiRenderer) {
        m_uiRenderer->beginFrame();
        m_uiRenderer->render();
        m_uiRenderer->endFrame();
    }
    
    m_renderer->endFrame();
}

void Application::onWindowClose(GLFWwindow* window)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->requestExit();
    }
}

void Application::onWindowResize(GLFWwindow* window, int width, int height)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_windowWidth = width;
        app->m_windowHeight = height;
        
        if (app->m_renderer) {
            // Обновление viewport в рендерере
            app->m_renderer->getWindowSize(app->m_windowWidth, app->m_windowHeight);
        }
        
        if (app->m_scene && app->m_scene->getCamera()) {
            // Обновление aspect ratio камеры
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            app->m_scene->getCamera()->setAspect(aspect);
        }
    }
}

void Application::onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->m_uiRenderer) {
        app->m_uiRenderer->handleKeyPress(key, action);
    }
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        app->requestExit();
    }
}

void Application::onMouseMove(GLFWwindow* window, double x, double y)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->m_uiRenderer) {
        app->m_uiRenderer->handleMouseMove(static_cast<float>(x), static_cast<float>(y));
    }
}

void Application::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->m_uiRenderer) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        app->m_uiRenderer->handleMouseClick(static_cast<float>(x), static_cast<float>(y));
    }
}

