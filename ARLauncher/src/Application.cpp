#include "Application.h"
#include "LiDARVisualizer.h"
#include "Renderer.h"
#include "Scene.h"
#include "Camera.h"
#include "UIRenderer.h"
#include "Button.h"
#include "Text.h"
#include "Style.h"
#include "LensEngineAPI.h"
#include "TrackingSession.h"
#include "ARLauncherAPI.h"
#include "FontRenderer.h"
#include "SensorStream/StreamReceiver.h"

#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#define GLFW_INCLUDE_NONE
#include <GL/gl.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

namespace {

double envDouble(const char* name, double fallback)
{
    const char* v = std::getenv(name);
    return v ? std::atof(v) : fallback;
}

bool envFlag(const char* name)
{
    const char* v = std::getenv(name);
    return v && v[0] == '1';
}

} // namespace

struct Application::PassthroughState {
#ifdef USE_VULKAN
    VulkanRenderer::PassthroughParams params[2];
#endif
    bool valid = false;
};

Application::Application()
    : m_window(nullptr)
    , m_running(false)
    , m_initialized(false)
    , m_windowWidth(1920)
    , m_windowHeight(1080)
    , m_pt(std::make_unique<PassthroughState>())
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
    (void)argc;
    (void)argv;
    if (m_initialized) {
        return true;
    }

    m_predictLeadMs = envDouble("ARL_PREDICT_MS", -1.0);
    m_eyeHFovDeg = static_cast<float>(envDouble("ARL_FOV", 0.0));
    m_stereo = envFlag("ARL_STEREO");
    m_depthOverlay = envFlag("ARL_LIDAR_OVERLAY");
    m_ipdMeters = static_cast<float>(envDouble("ARL_IPD_MM", 63.0) / 1000.0);
    m_eyeCount = m_stereo ? 2 : 1;

    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

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

    m_launcherAPI = std::make_unique<ARLauncherAPI>(this);
    m_launcherAPI->initializeDefaultContent();

    if (!initializeLensEngine()) {
        return false;
    }

    if (!initializeSensorStream()) {
        std::cerr << "Warning: sensor stream not started, continuing without iPhone" << std::endl;
    }

    m_statsStart = std::chrono::steady_clock::now();
    m_splashStart = m_statsStart;
    m_splashStarted = true;
    m_initialized = true;
    m_running = true;

    std::cout << "[CONTROLS] R — recenter, P — reprojection on/off, O — prediction on/off, L — LiDAR overlay, ESC — exit" << std::endl;
    return true;
}

void Application::run()
{
    auto lastTime = std::chrono::steady_clock::now();

    while (m_running && !glfwWindowShouldClose(m_window)) {
        auto currentTime = std::chrono::steady_clock::now();
        m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        glfwPollEvents();
        update(m_deltaTime);
        render();
#ifndef USE_VULKAN
        glfwSwapBuffers(m_window);   // legacy OpenGL
#endif
    }
}

void Application::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_stream) {
        m_stream->stop();
    }
    if (m_depthRunning) {
        m_depthRunning = false;
        m_depthCv.notify_all();
        if (m_depthThread.joinable()) m_depthThread.join();
    }
    if (m_tracking) {
        m_tracking->stop();
    }
    m_stream.reset();
    m_tracking.reset();

    m_uiRenderer.reset();
    m_launcherAPI.reset();
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
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
    // LEGACY: OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#endif

    // Полный экран на дисплее шлема: ARL_FULLSCREEN=1 [ARL_MONITOR=<index>]
    // Выбирается режим с максимальной частотой обновления в родном разрешении.
    GLFWmonitor* monitor = nullptr;
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    const int monitorIndex = static_cast<int>(envDouble("ARL_MONITOR", 0));
    GLFWmonitor* target = (monitors && monitorCount > 0)
        ? monitors[std::clamp(monitorIndex, 0, monitorCount - 1)]
        : glfwGetPrimaryMonitor();

    if (target) {
        if (const GLFWvidmode* cur = glfwGetVideoMode(target)) {
            m_refreshHz = cur->refreshRate > 0 ? cur->refreshRate : 60;
            if (envFlag("ARL_FULLSCREEN")) {
                int count = 0;
                const GLFWvidmode* modes = glfwGetVideoModes(target, &count);
                GLFWvidmode best = *cur;
                for (int i = 0; i < count; ++i) {
                    if (modes[i].width == cur->width && modes[i].height == cur->height &&
                        modes[i].refreshRate > best.refreshRate) {
                        best = modes[i];
                    }
                }
                glfwWindowHint(GLFW_REFRESH_RATE, best.refreshRate);
                m_windowWidth = best.width;
                m_windowHeight = best.height;
                m_refreshHz = best.refreshRate;
                monitor = target;
            }
        }
    }

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "GlaskiOS — ARLauncher", monitor, nullptr);

    if (!m_window) {
        const char* description;
        int code = glfwGetError(&description);
        std::cerr << "Failed to create GLFW window (code: " << code << "): " << (description ? description : "unknown error") << std::endl;
        return false;
    }
    std::cout << "[DISPLAY] " << m_windowWidth << "x" << m_windowHeight << " @ " << m_refreshHz << " Hz"
              << (monitor ? " fullscreen" : " windowed") << std::endl;

#ifndef USE_VULKAN
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
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
    m_renderer = createRenderer(true);
#else
    m_renderer = createRenderer(false);   // LEGACY: OpenGL
#endif

    if (!m_renderer) {
        std::cerr << "Failed to create renderer" << std::endl;
        return false;
    }

    if (!m_renderer->initialize(m_window)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

#ifdef USE_VULKAN
    static_cast<VulkanRenderer*>(m_renderer.get())->setPreRenderPassCallback([this]() { latchFrame(); });
#endif
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
    auto startButton = std::make_shared<Button>("Start AR");
    startButton->setPosition(glm::vec2(20.0f, 20.0f));
    startButton->setSize(glm::vec2(180.0f, 50.0f));
    startButton->setStyle(std::make_shared<Style>(Style::createARButtonStyle()));
    startButton->setOnClick([]() {
        std::cout << "AR Started" << std::endl;
    });
    m_uiRenderer->addElement(startButton);
    
    // Виджет для отображения данных камеры
    auto cameraInfoText = std::make_shared<Text>("📷 Camera: Waiting...");
    m_cameraText = cameraInfoText;
    cameraInfoText->setPosition(glm::vec2(20.0f, 80.0f));
    cameraInfoText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(cameraInfoText);
    
    // Виджет для отображения позы
    auto poseText = std::make_shared<Text>("📍 Position: (0, 0, 0)");
    m_poseText = poseText;
    poseText->setPosition(glm::vec2(20.0f, 110.0f));
    poseText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(poseText);
    
    // Виджет для IMU данных
    auto imuText = std::make_shared<Text>("⚡ IMU: No data");
    m_imuText = imuText;
    imuText->setPosition(glm::vec2(20.0f, 140.0f));
    imuText->setStyle(std::make_shared<Style>(Style::createARTextStyle()));
    m_uiRenderer->addElement(imuText);
    
    // Виджет для статуса подключения iPhone
    auto connectionText = std::make_shared<Text>("📡 iPhone: Disconnected");
    m_connectionText = connectionText;
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
    // Классический LensEngine API (фичи, свет, руки, LiDAR/DLIO)
    m_lensEngine = std::make_unique<LensEngine::LensEngineAPI>();
    if (!m_lensEngine->initialize()) {
        std::cerr << "Failed to initialize LensEngine" << std::endl;
        return false;
    }

    // Трекинг головы: IMU 100 Гц + предсказание + VIO (OpenVINS, если собран)
    m_tracking = std::make_unique<LensEngine::TrackingSession>();
    LensEngine::TrackingSession::Options opts;
    opts.enableVio = !envFlag("ARL_NO_VIO");
    if (const char* mount = std::getenv("ARL_MOUNT")) {
        opts.mountRollDeg = std::atoi(mount);
    }
    m_tracking->start(opts);
    return true;
}

bool Application::initializeSensorStream()
{
    m_stream = std::make_unique<sensorstream::StreamReceiver>();

    m_stream->setImuCallback([this](const sensorstream::ImuSample& s) {
        m_tracking->addImu(s.tNs,
                           glm::dvec3(s.gyro[0], s.gyro[1], s.gyro[2]),
                           glm::dvec3(s.accel[0], s.accel[1], s.accel[2]),
                           s.hasAccel);
    });

    // Кадры для VIO (поток декодера; VIO сам берёт только самый свежий)
    m_stream->setVideoCallback([this](const sensorstream::VideoFramePtr& f) {
        const int64_t midExposure = f->ptsNs + int64_t(f->exposureUs) * 500;
        m_tracking->addFrame(midExposure, f->y.data(), f->width, f->height, f->width,
                             f->fx, f->fy, f->cx, f->cy);
    });

    // LiDAR — в отдельный поток (DLIO тяжёлый, сетевой поток задерживать нельзя)
    m_depthRunning = true;
    m_depthThread = std::thread(&Application::depthWorker, this);
    m_stream->setDepthCallback([this](const sensorstream::DepthFrame& d) {
        auto copy = std::make_shared<sensorstream::DepthFrame>(d);
        m_depthW = d.width;
        m_depthH = d.height;
        {
            std::lock_guard<std::mutex> lock(m_depthMutex);
            m_overlayDepth = copy;
            m_pendingDepth = std::move(copy);
        }
        m_depthCv.notify_one();
    });

    if (!m_stream->start()) {
        return false;
    }
    std::cout << "[OK] SensorStream: USB (iPhone hotspot) port 9001, Wi-Fi port 9000. Waiting for iPhone..." << std::endl;
    return true;
}

void Application::depthWorker()
{
    while (m_depthRunning) {
        std::shared_ptr<sensorstream::DepthFrame> d;
        {
            std::unique_lock<std::mutex> lock(m_depthMutex);
            m_depthCv.wait_for(lock, std::chrono::milliseconds(200), [this] { return !m_depthRunning || m_pendingDepth; });
            if (!m_depthRunning) break;
            d = std::move(m_pendingDepth);
        }
        if (!d || !m_lensEngine) continue;
        m_lensEngine->processLidarData(reinterpret_cast<const uint8_t*>(d->depth.data()),
                                       d->depth.size() * sizeof(float), nullptr, 0,
                                       static_cast<uint64_t>(d->tNs));
    }
}

// ---------------------------------------------------------------------------------------------
// Late latching: вызывается рендером сразу после acquire swapchain, до записи render pass.
// Поза предсказывается на момент, когда кадр реально появится на дисплее.
// ---------------------------------------------------------------------------------------------
void Application::latchFrame()
{
#ifdef USE_VULKAN
    auto* vk = static_cast<VulkanRenderer*>(m_renderer.get());
    const VkExtent2D ext = vk->swapchainExtent();
    const float aspect = ext.height > 0 ? float(ext.width) / float(ext.height) : 16.0f / 9.0f;
    const int64_t now = sensorstream::pcNowNs();

    sensorstream::VideoFramePtr frame = m_stream ? m_stream->latestVideoFrame() : nullptr;

    // Поле зрения: как у камеры iPhone (минус ~10% запаса на репроекцию при быстрых поворотах),
    // либо ARL_FOV (горизонтальный FOV одного глаза) под линзы шлема
    float tanX = std::tan(glm::radians(80.0f) * 0.5f);
    if (m_eyeHFovDeg > 1.0f) {
        tanX = std::tan(glm::radians(m_eyeHFovDeg) * 0.5f);
    } else if (frame && frame->fx > 1.0f) {
        tanX = 0.9f * (frame->width * 0.5f) / frame->fx;
    }
    const float eyeAspect = m_stereo ? aspect * 0.5f : aspect;
    const float tanY = tanX / eyeAspect;
    glm::mat4 proj = glm::perspectiveRH_ZO(2.0f * std::atan(tanY), eyeAspect, 0.05f, 200.0f);
    proj[1][1] *= -1.0f;   // Vulkan: Y вниз

    // Сколько пройдёт от этого момента до фотонов: ожидание vblank + развёртка
    const double periodMs = 1000.0 / std::max(30.0, m_refreshHz);
    double leadMs = m_predictLeadMs;
    if (leadMs < 0) {
        leadMs = (vk->presentMode() == VK_PRESENT_MODE_MAILBOX_KHR || vk->presentMode() == VK_PRESENT_MODE_IMMEDIATE_KHR)
            ? periodMs + 3.0
            : 2.0 * periodMs + 3.0;
    }
    if (!m_prediction) leadMs = 0;

    LensEngine::TrackingSession::EyePose eye;
    const bool clockOk = m_stream && m_stream->clock().valid();
    if (clockOk) {
        const int64_t displayPhone = m_stream->clock().pcToPhone(now + int64_t(leadMs * 1e6));
        eye = m_tracking->predictEye(displayPhone);
    }

    m_frameHasPose = eye.valid;
    glm::dmat4 worldFromHead(1.0);
    if (eye.valid) {
        worldFromHead = glm::translate(glm::dmat4(1.0), eye.position) * glm::mat4_cast(eye.orientation);
    }
    for (int i = 0; i < m_eyeCount; ++i) {
        // Стерео: глаза смещены на ±IPD/2 по оси x головы
        const double offset = m_stereo ? (i == 0 ? -0.5 : 0.5) * m_ipdMeters : 0.0;
        const glm::dmat4 worldFromEye = worldFromHead * glm::translate(glm::dmat4(1.0), glm::dvec3(offset, 0.0, 0.0));
        m_viewMatrix[i] = glm::mat4(glm::inverse(worldFromEye));
        m_projMatrix[i] = proj;
    }

    // Видео: загрузка свежего кадра + репроекция под позу показа
    m_pt->valid = false;
    if (frame && !frame->y.empty()) {
        // LiDAR: до видео, чтобы дескриптор глубины уже указывал на настоящую карту
        if (m_depthOverlay) {
            std::shared_ptr<sensorstream::DepthFrame> depth;
            {
                std::lock_guard<std::mutex> lock(m_depthMutex);
                depth = m_overlayDepth;
            }
            if (depth && !depth->depth.empty()) {
                VulkanRenderer::DepthOverlayFrame df;
                df.depth = depth->depth.data();
                df.width = static_cast<uint32_t>(depth->width);
                df.height = static_cast<uint32_t>(depth->height);
                df.id = static_cast<uint64_t>(depth->tNs);
                vk->uploadDepthOverlay(df);
            }
        }

        VulkanRenderer::PassthroughFrame pf;
        pf.y = frame->y.data();
        pf.uv = frame->uv.data();
        pf.width = static_cast<uint32_t>(frame->width);
        pf.height = static_cast<uint32_t>(frame->height);
        pf.id = frame->index;
        vk->uploadPassthroughFrame(pf);

        VulkanRenderer::PassthroughParams p;
        p.camIntrinsicsNorm = glm::vec4(frame->fx / frame->width, frame->fy / frame->height,
                                        frame->cx / frame->width, frame->cy / frame->height);
        p.eyeTanHalfFov = glm::vec2(tanX, tanY);
        p.fullRange = frame->fullRange;
        p.bt709 = frame->bt709;
        p.depthOverlay = m_depthOverlay ? 0.55f : 0.0f;

        // Если поток оборвался — «застывшее» видео плавно гасим (неподвижная картинка укачивает)
        const double ageMs = double(now - frame->decodedPcNs) * 1e-6;
        const float staleFade = float(std::clamp(1.0 - (ageMs - 400.0) / 400.0, 0.0, 1.0));
        p.opacity = m_videoOpacity * staleFade;

        glm::dquat worldFromCam;
        const int64_t midExposure = frame->ptsNs + int64_t(frame->exposureUs) * 500;
        if (eye.valid && m_reprojection && m_tracking->cameraOrientationAt(midExposure, worldFromCam)) {
            p.cvFromEye = glm::mat3(glm::mat3_cast(glm::conjugate(worldFromCam) * eye.orientation));
        } else {
            // Без трекинга/репроекции — видео «приклеено» к голове
            p.cvFromEye = glm::mat3(glm::mat3_cast(m_tracking->eyeToCameraRotation()));
        }
        // Камера одна (моно): обоим глазам одно и то же изображение, направленное «в бесконечность»
        m_pt->params[0] = p;
        m_pt->params[1] = p;
        m_pt->valid = staleFade > 0.0f;
        m_frameHasVideo = true;

        if (ageMs < 400.0) {
            m_videoAgeMsSum += double(now - frame->pcPtsNs) * 1e-6;
            ++m_videoAgeCount;
        }
    }
#endif
}

void Application::update(float deltaTime)
{
    if (m_scene) {
        m_scene->update(deltaTime);
    }
    updateSplash();
    updateHud();
    printStats();
}

void Application::updateHud()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - m_hudUpdate < std::chrono::milliseconds(250)) return;
    m_hudUpdate = now;
    if (!m_cameraText || !m_stream) return;

    char buf[256];
    const auto ss = m_stream->stats();
    const auto frame = m_stream->latestVideoFrame();
    if (ss.clientConnected && frame) {
        std::snprintf(buf, sizeof(buf), "Camera: %dx%d %.0f fps, %s%s", frame->width, frame->height,
                      ss.videoFpsDecoded, ss.decoderName.c_str(), frame->fullRange ? "" : ", limited range");
    } else {
        std::snprintf(buf, sizeof(buf), "Camera: Waiting...");
    }
    m_cameraText->setText(buf);

    if (m_tracking) {
        const auto st = m_tracking->status();
        const glm::mat4 worldFromEye = glm::inverse(m_viewMatrix[0]);
        std::snprintf(buf, sizeof(buf), "%s: (%.2f, %.2f, %.2f) m", st.mode.c_str(),
                      worldFromEye[3].x, worldFromEye[3].y, worldFromEye[3].z);
        m_poseText->setText(buf);
        if (st.imuActive) {
            std::snprintf(buf, sizeof(buf), "IMU: %.0f Hz, mount %d°, gyro bias %.3f°/s", ss.imuRateHz,
                          st.mountRollDeg, glm::degrees(glm::length(st.gyroBias)));
        } else {
            std::snprintf(buf, sizeof(buf), "IMU: No data");
        }
        m_imuText->setText(buf);
    }

    if (ss.clientConnected) {
        const int dw = m_depthW.load();
        if (dw > 0) {
            std::snprintf(buf, sizeof(buf), "iPhone: %.1f Mbit/s, LiDAR %dx%d%s", ss.megabitsPerSecond, dw,
                          m_depthH.load(), m_depthOverlay ? " (overlay L)" : "");
        } else {
            std::snprintf(buf, sizeof(buf), "iPhone: %.1f Mbit/s, LiDAR off", ss.megabitsPerSecond);
        }
    } else {
        std::snprintf(buf, sizeof(buf), "iPhone: Disconnected (USB :9001 / Wi-Fi :9000)");
        m_depthW = 0;
    }
    m_connectionText->setText(buf);
}

void Application::updateSplash()
{
    if (!m_splashActive || !m_splashStarted) {
        return;
    }
    const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_splashStart).count();
    // 3D и UI плавно появляются за 1.5 с; видео — как только пришёл первый кадр
    const float fade = std::clamp((t - 0.5f) / 1.5f, 0.0f, 1.0f);
    m_3dObjectsOpacity = fade;
    m_uiOpacity = fade;
    m_titleOpacity = 1.0f - fade;
    if (m_frameHasVideo) {
        m_videoOpacity = std::min(1.0f, m_videoOpacity + m_deltaTime / 0.8f);
    }
    if (fade >= 1.0f && m_videoOpacity >= 1.0f) {
        m_splashActive = false;
    }
}

void Application::printStats()
{
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - m_statsStart).count();
    if (elapsed < 2.0) {
        return;
    }
    const double fps = double(m_renderFrames) / elapsed;
    std::cout << std::fixed << std::setprecision(1) << "[FRAME] render " << fps << " fps (display "
              << m_refreshHz << " Hz)";
    if (m_tracking) {
        const auto st = m_tracking->status();
        std::cout << " | tracking: " << st.mode;
    }
    if (m_videoAgeCount > 0) {
        std::cout << " | video age at latch " << (m_videoAgeMsSum / double(m_videoAgeCount)) << " ms (hidden by reprojection)";
    }
    if (m_stream) {
        const auto ss = m_stream->stats();
        if (ss.clientConnected) {
            std::cout << " | stream " << ss.videoFpsDecoded << " fps, IMU " << ss.imuRateHz << " Hz, "
                      << ss.megabitsPerSecond << " Mbit/s, " << ss.decoderName;
        } else {
            std::cout << " | iPhone: not connected";
        }
    }
    std::cout << std::endl;
    m_renderFrames = 0;
    m_videoAgeMsSum = 0;
    m_videoAgeCount = 0;
    m_statsStart = now;
}

void Application::render()
{
    if (!m_renderer) {
        return;
    }

    // beginFrame: ждёт свободный слот GPU, берёт образ swapchain и вызывает latchFrame()
    m_renderer->beginFrame();

#ifdef USE_VULKAN
    auto* vk = static_cast<VulkanRenderer*>(m_renderer.get());
    if (!vk->isFrameActive()) {
        return;   // окно свёрнуто / пересоздание swapchain
    }
    const VkExtent2D ext = vk->swapchainExtent();
    m_renderer->set3DObjectsOpacity(m_3dObjectsOpacity);

    std::vector<glm::mat4> transforms;
    std::vector<uint32_t> meshIds;
    if (m_scene) {
        if (auto camera = m_scene->getCamera()) {
            // Камера сцены следует за предсказанной позой (для отсечения по frustum)
            const glm::mat4 worldFromEye = glm::inverse(m_viewMatrix[0]);
            camera->setPosition(glm::vec3(worldFromEye[3]));
            camera->setRotation(glm::quat_cast(glm::mat3(worldFromEye)));
        }
        transforms = m_scene->getObjectTransforms();
        meshIds = m_scene->getVisibleMeshIds();
    }

    for (int i = 0; i < m_eyeCount; ++i) {
        if (m_stereo) {
            const float half = float(ext.width) * 0.5f;
            vk->setEyeViewport(half * float(i), 0.0f, half, float(ext.height));
        } else {
            vk->setEyeViewport(0, 0, 0, 0);
        }
        if (m_pt->valid) {
            vk->renderPassthrough(m_pt->params[i]);
        }
        m_renderer->setCameraMatrix(m_viewMatrix[i], m_projMatrix[i]);
        if (!transforms.empty()) {
            m_renderer->render3DObjects(transforms, meshIds);
        }
        m_renderer->renderUIWindows();   // 3D-окна UI — тоже для каждого глаза
    }
    vk->setEyeViewport(0, 0, 0, 0);
#else
    m_renderer->renderStoredVideoBackground();   // LEGACY
    m_renderer->set3DObjectsOpacity(m_3dObjectsOpacity);
    if (m_scene) {
        if (auto camera = m_scene->getCamera()) {
            m_renderer->setCameraMatrix(camera->getViewMatrix(), camera->getProjectionMatrix());
        }
        m_renderer->render3DObjects(m_scene->getObjectTransforms(), m_scene->getVisibleMeshIds());
    }
    m_renderer->renderUIWindows();
#endif

    if (m_uiRenderer) {
        m_uiRenderer->beginFrame();
        m_uiRenderer->render();
        m_uiRenderer->endFrame();
    }

    m_renderer->endFrame();
    ++m_renderFrames;
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
            app->m_renderer->getWindowSize(app->m_windowWidth, app->m_windowHeight);
        }

        if (app->m_scene && app->m_scene->getCamera() && height > 0) {
            app->m_scene->getCamera()->setAspect(static_cast<float>(width) / static_cast<float>(height));
        }
    }
}

void Application::onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) {
        return;
    }
    if (app->m_uiRenderer) {
        app->m_uiRenderer->handleKeyPress(key, action);
    }
    if (action != GLFW_PRESS) {
        return;
    }

    switch (key) {
    case GLFW_KEY_ESCAPE:
        app->requestExit();
        break;
    case GLFW_KEY_R:
        if (app->m_tracking) {
            app->m_tracking->recenter();
            std::cout << "[TRACKING] recentered" << std::endl;
        }
        break;
    case GLFW_KEY_P:
        app->m_reprojection = !app->m_reprojection;
        std::cout << "[TRACKING] reprojection " << (app->m_reprojection ? "ON" : "OFF") << std::endl;
        break;
    case GLFW_KEY_L:
        app->m_depthOverlay = !app->m_depthOverlay;
        std::cout << "[LIDAR] depth overlay " << (app->m_depthOverlay ? "ON" : "OFF") << std::endl;
        break;
    case GLFW_KEY_O:
        app->m_prediction = !app->m_prediction;
        std::cout << "[TRACKING] prediction " << (app->m_prediction ? "ON" : "OFF") << std::endl;
        break;
    default:
        break;
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

