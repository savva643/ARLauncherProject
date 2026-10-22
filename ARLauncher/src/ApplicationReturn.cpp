// Обратный поток: готовая картинка лаунчера -> экран iPhone.
//
// Телефон шлёт DISPLAY (режим, размер экрана, FOV), ПК рендерит под него, читает кадр с GPU,
// кодирует H.264 с минимальной задержкой и отправляет назад по тому же TCP. В каждом кадре — поза,
// под которую он нарисован (поворот устройства от последнего сэмпла IMU до момента показа),
// поэтому телефон доворачивает кадр своим гироскопом прямо перед показом: поворот головы
// виден с задержкой дисплея телефона, а не всей цепочки ПК.
//
// Режимы: 1 — моно (весь экран), 2 — стерео side-by-side, 3 — стерео + линзы VR-очков
// (искажение линз делает телефон, после доворота). Флаг «своя камера»: ПК рисует только объекты
// с альфой, фон — живая камера телефона (самый быстрый passthrough).

#include "Application.h"
#include "ApplicationReturnState.h"
#include "Renderer.h"
#include "TrackingSession.h"
#include "SensorStream/ReturnEncoder.h"
#include "SensorStream/StreamReceiver.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>



namespace {
const char* returnModeName(int mode)
{
    switch (mode) {
    case 1: return "mono";
    case 2: return "stereo";
    case 3: return "stereo+lens";
    default: return "off";
    }
}

void toFloat4(const glm::dquat& q, float out[4])
{
    out[0] = float(q.x);
    out[1] = float(q.y);
    out[2] = float(q.z);
    out[3] = float(q.w);
}
} // namespace

void Application::initializeReturn()
{
#ifdef USE_VULKAN
    if (!m_stream) return;
    m_return = std::make_unique<ReturnState>();
    if (const char* v = std::getenv("ARL_RETURN_HUD")) m_return->hud = v[0] == '1';
    if (const char* v = std::getenv("ARL_RETURN_LEAD_MS")) m_return->fixedLeadMs = std::atof(v);
    if (const char* v = std::getenv("ARL_RETURN_OVERSCAN")) m_return->overscan = std::clamp(float(std::atof(v)), 1.0f, 1.5f);
    m_returnEncoder = std::make_unique<sensorstream::ReturnEncoder>();
    m_returnEncoder->start(m_stream.get());

    m_stream->setDisplayCallback([this](const sensorstream::DisplayRequest& r) {
        std::lock_guard<std::mutex> lock(m_return->requestMutex);
        m_return->request = r;
        m_return->requestChanged = true;
    });
    m_stream->setAckCallback([this](const sensorstream::ReturnAck& a) { handleReturnAck(a); });

    auto* vk = static_cast<VulkanRenderer*>(m_renderer.get());
    vk->setReadbackCallback([this](const VulkanRenderer::Readback& rb) {
        ReturnState& r = *m_return;
        if (!r.on || !r.metaValid || !m_returnEncoder) return;
        r.metaValid = false;
        // Не чаще, чем просит телефон (ПК может рисовать на 144 Гц)
        const auto now = std::chrono::steady_clock::now();
        const double minPeriod = 0.9 / std::max(1, r.active.fps);
        if (std::chrono::duration<double>(now - r.lastSubmit).count() < minPeriod) return;
        r.lastSubmit = now;
        const uint32_t id = m_returnEncoder->submit(rb.pixels, rb.width, rb.height, rb.stride, rb.bgra,
                                                    r.active.localCamera, r.meta);
        if (id == 0) return;
        std::lock_guard<std::mutex> lock(r.statsMutex);
        r.sent[id % r.sent.size()] = {id, r.latchPhoneNs, r.cameraPhoneNs};
    });
#endif
}

void Application::shutdownReturn()
{
    if (m_returnEncoder) m_returnEncoder->stop();
    m_returnEncoder.reset();
}

bool Application::returnActive() const
{
    return m_return && m_return->on;
}

bool Application::returnHidesHud() const
{
    return m_return && m_return->on && !m_return->hud;
}

bool Application::returnLocalCamera() const
{
    return m_return && m_return->on && m_return->active.localCamera;
}

void Application::updateReturn()
{
#ifdef USE_VULKAN
    if (!m_return) return;
    ReturnState& r = *m_return;
    sensorstream::DisplayRequest req;
    {
        std::lock_guard<std::mutex> lock(r.requestMutex);
        if (!r.requestChanged) return;
        r.requestChanged = false;
        req = r.request;
    }
    auto* vk = static_cast<VulkanRenderer*>(m_renderer.get());
    const bool want = req.mode != 0;

    if (want && !vk->readbackSupported()) {
        std::cerr << "[RETURN] swapchain cannot be read back (no TRANSFER_SRC) — phone display unavailable" << std::endl;
        return;
    }

    if (want && !r.on) {
        r.savedStereo = m_stereo;
        r.savedLens = vk->lensParams().enabled;
        r.savedIpd = m_ipdMeters;
        glfwGetWindowSize(m_window, &r.savedWidth, &r.savedHeight);
    }
    if (want) {
        // Линзы — на телефоне (после доворота), картинка ПК — без искажений
        if (vk->lensParams().enabled) {
            VulkanRenderer::LensParams lens = vk->lensParams();
            lens.enabled = false;
            vk->setLensParams(lens);
        }
        m_stereo = req.mode >= 2;
        m_eyeCount = m_stereo ? 2 : 1;
        m_ipdMeters = req.ipd;
        // Окно = кадр телефона (рендер без масштабирования); в полноэкранном режиме — как есть
        if (!glfwGetWindowMonitor(m_window)) {
            glfwSetWindowSize(m_window, req.width, req.height);
            r.resized = true;
        }
        vk->setClearColor(req.localCamera ? glm::vec4(0.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        vk->setReadbackEnabled(true);
        sensorstream::ReturnEncoder::Config cfg;
        cfg.fps = req.fps;
        cfg.bitrateKbps = req.bitrateKbps;
        cfg.hevc = req.hevc;
        m_returnEncoder->setConfig(cfg);
        m_returnEncoder->requestKeyframe();
        {
            std::lock_guard<std::mutex> lock(r.statsMutex);
            r.leadMs = 45.0;
            r.acks = 0;
        }
        std::cout << "[RETURN] phone display " << returnModeName(req.mode) << " " << req.width << "x" << req.height
                  << (req.localCamera ? ", background = phone camera" : "") << std::endl;
    } else if (r.on) {
        vk->setReadbackEnabled(false);
        vk->setClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        m_stereo = r.savedStereo;
        m_eyeCount = m_stereo ? 2 : 1;
        m_ipdMeters = r.savedIpd;
        if (r.savedLens) {
            VulkanRenderer::LensParams lens = vk->lensParams();
            lens.enabled = true;
            vk->setLensParams(lens);
        }
        if (r.resized && r.savedWidth > 0) glfwSetWindowSize(m_window, r.savedWidth, r.savedHeight);
        r.resized = false;
        std::cout << "[RETURN] phone display off" << std::endl;
    }
    r.active = req;
    r.on = want;
#endif
}

bool Application::returnEyeFov(float& tanX, float& tanY, double& leadMs) const
{
    if (!returnActive()) return false;
    const ReturnState& r = *m_return;
    // Телефон задал FOV (линзы) — его; иначе (0) остаётся FOV, посчитанный по камере
    if (r.active.tanX > 0.05f && r.active.tanY > 0.05f) {
        tanX = r.active.tanX;
        tanY = r.active.tanY;
    }
    // Рисуем шире показа: телефон доворачивает кадр, по краям не должно быть чёрного
    tanX *= r.overscan;
    tanY *= r.overscan;
    {
        std::lock_guard<std::mutex> lock(r.statsMutex);
        leadMs = r.fixedLeadMs >= 0.0 ? r.fixedLeadMs : r.leadMs;
    }
    return true;
}

void Application::returnLatched(int64_t displayPhoneNs, int64_t cameraPhoneNs, float tanX, float tanY)
{
    if (!returnActive() || !m_tracking || !m_stream) return;
    ReturnState& r = *m_return;
    const auto w = m_tracking->displayWarp(displayPhoneNs);
    sensorstream::ReturnFrameMeta& m = r.meta;
    m.layout = m_stereo ? 2 : 1;
    m.mode = uint8_t(r.active.mode);
    m.tRefNs = w.valid ? w.tRefNs : 0;
    m.tTargetNs = displayPhoneNs;
    m.tCameraNs = r.active.localCamera ? 0 : cameraPhoneNs;
    toFloat4(w.predFromRef, m.qPredFromRef);
    toFloat4(w.eyeFromDevice, m.eyeFromDevice);
    toFloat4(w.cameraFromDevice, m.cameraFromDevice);
    m.gyroBias[0] = float(w.gyroBias.x);
    m.gyroBias[1] = float(w.gyroBias.y);
    m.gyroBias[2] = float(w.gyroBias.z);
    m.overscan = r.overscan;
    m.tanX = tanX;
    m.tanY = tanY;
    m.ipd = m_ipdMeters;
    r.metaValid = true;
    r.latchPhoneNs = m_stream->clock().pcToPhone(sensorstream::pcNowNs());
    r.cameraPhoneNs = cameraPhoneNs;
}

void Application::handleReturnAck(const sensorstream::ReturnAck& a)
{
    if (!m_return) return;
    if (a.needKeyframe && m_returnEncoder) m_returnEncoder->requestKeyframe();
    if (a.frameId == 0 || a.displayNs == 0) return;
    ReturnState& r = *m_return;
    std::lock_guard<std::mutex> lock(r.statsMutex);
    const auto& s = r.sent[a.frameId % r.sent.size()];
    if (s.id != a.frameId) return;
    const double renderToPhoton = double(a.displayNs - s.latchPhoneNs) * 1e-6;
    if (renderToPhoton < 0.0 || renderToPhoton > 1000.0) return;
    const double k = r.acks < 10 ? 0.3 : 0.05;
    r.renderToPhotonMs += k * (renderToPhoton - r.renderToPhotonMs);
    if (s.cameraPhoneNs > 0) r.cameraToPhotonMs += k * (double(a.displayNs - s.cameraPhoneNs) * 1e-6 - r.cameraToPhotonMs);
    r.decodeMs += k * (a.decodeUs * 1e-3 - r.decodeMs);
    // Предсказываем позу ровно на момент показа на телефоне; остаток доворачивает телефон
    r.leadMs = std::clamp(r.renderToPhotonMs, 8.0, 150.0);
    ++r.acks;
    r.lastAckPcNs = sensorstream::pcNowNs();
}

std::string Application::returnStatusText() const
{
    if (!returnActive()) return {};
    const ReturnState& r = *m_return;
    const auto es = m_returnEncoder ? m_returnEncoder->stats() : sensorstream::ReturnEncoder::Stats{};
    char buf[256];
    std::lock_guard<std::mutex> lock(r.statsMutex);
    const bool acked = r.acks > 0 && sensorstream::pcNowNs() - r.lastAckPcNs < 2'000'000'000LL;
    if (acked) {
        std::snprintf(buf, sizeof(buf), "Phone display: %s %.0f fps %.1f Mbit/s, %s %.1f ms, render->photon %.0f ms%s",
                      returnModeName(r.active.mode), es.fps, es.megabitsPerSecond, es.name.c_str(), es.encodeMsAvg,
                      r.renderToPhotonMs, r.cameraToPhotonMs > 0 ? (", camera->photon " + std::to_string(int(r.cameraToPhotonMs)) + " ms").c_str() : "");
    } else {
        std::snprintf(buf, sizeof(buf), "Phone display: %s %.0f fps %.1f Mbit/s, %s %.1f ms (waiting for phone)",
                      returnModeName(r.active.mode), es.fps, es.megabitsPerSecond, es.name.c_str(), es.encodeMsAvg);
    }
    return buf;
}
