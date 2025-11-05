#include "Application.h"
#include "Renderer.h"
#include "Scene.h"
#include "UIRenderer.h"
#include "ui/Button.h"
#include "ui/Text.h"
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "ARLauncher", nullptr, nullptr);
    
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
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
    m_renderer = createRenderer(false); // Используем OpenGL по умолчанию
    
    if (!m_renderer || !m_renderer->initialize(m_window)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
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
    
    m_scene->createDemoScene();
    
    return true;
}

bool Application::initializeUI()
{
    m_uiRenderer = std::make_unique<UIRenderer>(m_renderer.get());
    
    if (!m_uiRenderer->initialize()) {
        std::cerr << "Failed to initialize UI renderer" << std::endl;
        return false;
    }
    
    // Создаем демо UI элементы
    auto startButton = std::make_shared<Button>("Start AR");
    startButton->setPosition(glm::vec2(10.0f, 10.0f));
    startButton->setSize(glm::vec2(150.0f, 40.0f));
    startButton->setOnClick([]() {
        std::cout << "AR Started" << std::endl;
    });
    m_uiRenderer->addElement(startButton);
    
    auto infoText = std::make_shared<Text>("Camera Position: (0, 0, 0)");
    infoText->setPosition(glm::vec2(10.0f, 60.0f));
    m_uiRenderer->addElement(infoText);
    
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
    
    // Рендеринг сцены
    if (m_scene) {
        auto camera = m_scene->getCamera();
        if (camera) {
            m_renderer->setCameraMatrix(camera->getViewMatrix(), camera->getProjectionMatrix());
        }
        
        auto transforms = m_scene->getObjectTransforms();
        auto meshIds = m_scene->getVisibleMeshIds();
        m_renderer->render3DObjects(transforms, meshIds);
    }
    
    // Рендеринг UI
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

