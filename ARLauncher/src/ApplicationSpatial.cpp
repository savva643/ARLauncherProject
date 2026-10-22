// Пространство GlaskiOS: руки и ввод щипком, меш комнаты и плоскости, карта мира с якорями.
#include "Application.h"
#include "Renderer.h"
#include "Scene.h"
#include "HandTrackingProvider.h"
#include "TrackingSession.h"
#include "SensorStream/StreamReceiver.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

using Kind = LensEngine::SceneReconstruction::Kind;

glm::vec4 kindColor(Kind k)
{
    switch (k) {
    case Kind::Floor: return {0.30f, 0.85f, 0.45f, 0.30f};
    case Kind::Wall: return {0.35f, 0.60f, 1.00f, 0.25f};
    case Kind::Ceiling: return {0.90f, 0.90f, 0.95f, 0.20f};
    case Kind::Table: return {1.00f, 0.75f, 0.25f, 0.40f};
    default: return {0.80f, 0.40f, 0.90f, 0.25f};
    }
}

// Поворот, переводящий ось z в направление dir
glm::mat4 alignZ(const glm::vec3& dir)
{
    const glm::vec3 z = glm::normalize(dir);
    const glm::vec3 up = std::fabs(z.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    const glm::vec3 x = glm::normalize(glm::cross(up, z));
    const glm::vec3 y = glm::cross(z, x);
    return glm::mat4(glm::vec4(x, 0), glm::vec4(y, 0), glm::vec4(z, 0), glm::vec4(0, 0, 0, 1));
}

// Луч vs сфера: расстояние до входа или -1
float raySphere(const glm::vec3& o, const glm::vec3& d, const glm::vec3& c, float r)
{
    const glm::vec3 oc = o - c;
    const float b = glm::dot(oc, d);
    const float disc = b * b - (glm::dot(oc, oc) - r * r);
    if (disc < 0.0f) return -1.0f;
    const float t = -b - std::sqrt(disc);
    return t > 0.0f ? t : -1.0f;
}

std::string defaultWorldMapPath()
{
    const char* env = std::getenv("ARL_WORLD_MAP");
    if (env && std::strcmp(env, "0") == 0) return {};
    if (env && env[0]) return env;
    const char* home = std::getenv("HOME");
    if (!home) return {};
    const std::string dir = std::string(home) + "/.local/share/glaski";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir + "/world.map";
}

} // namespace

void Application::initializeSpatial()
{
    m_handsEnabled = !std::getenv("ARL_HANDS") || std::getenv("ARL_HANDS")[0] != '0';
    m_showRoom = std::getenv("ARL_SHOW_ROOM") && std::getenv("ARL_SHOW_ROOM")[0] == '1';
    if (m_handsEnabled && LensEngine::HandTrackingProvider::available()) {
        m_handsProvider = std::make_unique<LensEngine::HandTrackingProvider>();
        if (!m_handsProvider->initialize("")) m_handsProvider.reset();
        else std::cout << "[HANDS] MediaPipe hand tracking ready (pinch = select/grab)" << std::endl;
    }

    // Пустые меши для комнаты (заполняются раз в секунду)
    const std::vector<float> dummyV(24, 0.0f);
    const std::vector<uint32_t> dummyI = {0, 1, 2};
    for (auto& id : m_roomMeshIds) {
        id = m_renderer->createMesh(dummyV, dummyI);
        static_cast<VulkanRenderer*>(m_renderer.get())->updateMesh(id, {}, {});
    }

    m_worldMapPath = defaultWorldMapPath();
    if (!m_worldMapPath.empty() && m_tracking && std::filesystem::exists(m_worldMapPath)) {
        m_tracking->loadWorldMap(m_worldMapPath);
    }
}

void Application::shutdownSpatial()
{
    if (m_saveThread.joinable()) m_saveThread.join();
    if (m_tracking && !m_worldMapPath.empty()) {
        m_tracking->saveWorldMap(m_worldMapPath);
    }
}

void Application::updateSpatial()
{
    if (!m_tracking || !m_scene) return;
    const auto st = m_tracking->status();

    // Карта мира нашлась — объекты, которые пользователь расставил в прошлый раз, встают на свои места
    const int state = int(m_tracking->mapState());
    if (state != m_lastMapState) {
        m_lastMapState = state;
        if (m_tracking->mapState() == LensEngine::TrackingSession::MapState::Relocalized) {
            for (const auto& a : m_tracking->anchors()) {
                if (a.name.rfind("obj:", 0) != 0) continue;
                SceneObject* o = m_scene->objectAt(size_t(std::atoi(a.name.c_str() + 4)));
                if (!o) continue;
                o->anchorId = a.id;
                o->onFloor = o->onTable = false;
                o->visible = true;
            }
        }
    }
    // Якоря держат объекты (позиции уточняются вместе с картой)
    for (const auto& a : m_tracking->anchors()) {
        if (a.name.rfind("obj:", 0) != 0) continue;
        SceneObject* o = m_scene->objectAt(size_t(std::atoi(a.name.c_str() + 4)));
        if (o && o->anchorId == a.id && !(m_grab.active && m_grab.object == size_t(std::atoi(a.name.c_str() + 4)))) {
            o->position = glm::vec3(a.position);
        }
    }

    // Стол найден — ставим на него объекты onTable (на самый большой стол)
    if (st.roomPlanes > 0) {
        const auto planes = m_tracking->roomPlanes();
        const LensEngine::SceneReconstruction::Plane* best = nullptr;
        for (const auto& p : planes)
            if (p.kind == Kind::Table && (!best || p.area > best->area)) best = &p;
        if (best) {
            for (size_t i = 0; i < m_scene->objects().size(); ++i) {
                SceneObject* o = m_scene->objectAt(i);
                if (!o->onTable || o->anchorId != 0 || (m_grab.active && m_grab.object == i)) continue;
                o->position = best->center + glm::vec3(0.0f, 0.5f * o->scale.y + 0.01f, 0.0f);
                o->visible = true;
            }
        }
    }

    // Автосохранение карты раз в минуту (в фоне: несколько МБ на диск)
    const auto now = std::chrono::steady_clock::now();
    if (!m_worldMapPath.empty() && st.lidarTracking && now - m_lastAutosave > std::chrono::seconds(60)) {
        m_lastAutosave = now;
        if (m_saveThread.joinable()) m_saveThread.join();
        m_saveThread = std::thread([this] { m_tracking->saveWorldMap(m_worldMapPath); });
    }

    updateRoomMesh();
    updateInteraction();
}

void Application::updateRoomMesh()
{
    if (!m_showRoom || !m_tracking) return;
    const uint64_t version = m_tracking->roomVersion();
    if (version == m_roomVersionShown) return;
    m_roomVersionShown = version;
    auto* vk = static_cast<VulkanRenderer*>(m_renderer.get());

    const auto mesh = m_tracking->roomMesh();
    // Делим меш по классам поверхности (у каждого свой цвет)
    std::vector<float> verts[6];
    std::vector<uint32_t> idx[6];
    std::vector<int32_t> remap(mesh.positions.size(), -1);
    for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        const uint32_t a = mesh.indices[t], b = mesh.indices[t + 1], c = mesh.indices[t + 2];
        const int k = int(mesh.kinds[a]);
        if (k < 0 || k > 5) continue;
        for (uint32_t v : {a, b, c}) {
            // вершина может принадлежать нескольким классам — копируем в каждый
            auto& vv = verts[k];
            const glm::vec3& p = mesh.positions[v];
            const glm::vec3& n = mesh.normals[v];
            idx[k].push_back(uint32_t(vv.size() / 8));
            vv.insert(vv.end(), {p.x, p.y, p.z, n.x, n.y, n.z, 0.0f, 0.0f});
        }
    }
    for (int k = 0; k < 6; ++k) vk->updateMesh(m_roomMeshIds[k], verts[k], idx[k]);

    m_planeViews.clear();
    for (const auto& p : m_tracking->roomPlanes()) {
        const glm::mat4 basis(glm::vec4(p.axisU, 0), glm::vec4(p.normal, 0), glm::vec4(p.axisV, 0), glm::vec4(0, 0, 0, 1));
        const glm::mat4 t = glm::translate(glm::mat4(1.0f), p.center + p.normal * 0.01f) * basis *
                            glm::scale(glm::mat4(1.0f), glm::vec3(2.0f * p.halfExtent.x, 0.004f, 2.0f * p.halfExtent.y));
        glm::vec4 c = kindColor(p.kind);
        c.a = 0.45f;
        m_planeViews.push_back({t, c});
    }
}

void Application::updateInteraction()
{
    using Hand = LensEngine::HandTrackingProvider::Hand;
    std::vector<Hand> hands;
    if (m_handsProvider && sensorstream::pcNowNs() - m_handsPcNs.load() < 400'000'000LL) hands = m_handsProvider->hands();

    // Луч выбора: взгляд (если есть айтрекер) -> иначе рука, которая указывает -> иначе центр поля зрения
    const glm::mat4 worldFromEye = glm::inverse(m_viewMatrix[0]);
    const glm::vec3 head(worldFromEye[3]);
    glm::vec3 rayO = head, rayD = glm::normalize(glm::vec3(worldFromEye * glm::vec4(0, 0, -1, 0)));
    if (m_gazeValid && m_gazeFromEyes) {
        rayO = m_gazeOrigin;
        rayD = m_gazeDirection;
    } else if (!hands.empty()) {
        rayO = hands[0].aimOrigin;
        rayD = hands[0].aimDirection;
    }
    if (!m_gazeFromEyes) {
        m_gazeValid = true;
        m_gazeOrigin = rayO;
        m_gazeDirection = rayD;
    }

    // Наведение: ближайший объект на луче
    m_hoverObject = -1;
    float bestT = 1e9f;
    const auto& objects = m_scene->objects();
    for (size_t i = 0; i < objects.size(); ++i) {
        const SceneObject& o = objects[i];
        if (!o.visible || !o.grabbable) continue;
        const float r = 0.6f * std::max({o.scale.x, o.scale.y, o.scale.z}) + 0.03f;
        const float t = raySphere(rayO, rayD, o.position, r);
        if (t > 0.0f && t < bestT) {
            bestT = t;
            m_hoverObject = int(i);
        }
    }

    // Щипок: начало — взять объект под лучом, удержание — двигать, отпустил — оставить (и запомнить в карте)
    bool pinchNow[2] = {false, false};
    const Hand* byHand[2] = {nullptr, nullptr};
    for (const auto& h : hands) {
        byHand[h.right ? 1 : 0] = &h;
        pinchNow[h.right ? 1 : 0] = h.pinching;
    }
    for (int s = 0; s < 2; ++s) {
        if (pinchNow[s] && !m_prevPinch[s] && !m_grab.active && m_hoverObject >= 0 && byHand[s]) {
            m_grab.active = true;
            m_grab.rightHand = s == 1;
            m_grab.object = size_t(m_hoverObject);
            m_grab.objectStart = objects[m_grab.object].position;
            m_grab.handStart = byHand[s]->pinchPoint;
            // Далёкий объект двигается сильнее руки (как в visionOS): во столько, во сколько он дальше кисти
            const float dObj = glm::length(m_grab.objectStart - head);
            const float dHand = std::max(0.2f, glm::length(m_grab.handStart - head));
            m_grab.gain = std::max(1.0f, dObj / dHand);
            std::cout << "[INPUT] grab object " << m_grab.object << std::endl;
        }
        m_prevPinch[s] = pinchNow[s];
    }
    if (m_grab.active) {
        const Hand* h = byHand[m_grab.rightHand ? 1 : 0];
        SceneObject* o = m_scene->objectAt(m_grab.object);
        if (h && h->pinching && o) {
            o->position = m_grab.objectStart + (h->pinchPoint - m_grab.handStart) * m_grab.gain;
            o->onFloor = o->onTable = false;
        } else {
            m_grab.active = false;
            if (o && m_tracking) {
                // Объект остаётся тут и в следующий запуск (якорь в карте мира)
                if (o->anchorId != 0 && m_tracking->updateAnchor(o->anchorId, glm::dvec3(o->position), glm::dquat(1, 0, 0, 0))) {
                } else {
                    o->anchorId = m_tracking->addAnchor("obj:" + std::to_string(m_grab.object), glm::dvec3(o->position),
                                                        glm::dquat(1, 0, 0, 0));
                }
                std::cout << "[INPUT] placed object " << m_grab.object << " (anchor " << o->anchorId << ")" << std::endl;
            }
        }
    }
}

void Application::appendSpatialDraws(std::vector<glm::mat4>& transforms, std::vector<uint32_t>& meshIds,
                                     std::vector<glm::vec4>& colors, std::vector<glm::vec4>& params)
{
    if (!m_scene) return;
    auto add = [&](const glm::mat4& t, uint32_t mesh, const glm::vec4& c, float metallic, float rough, float bias, bool shadow) {
        transforms.push_back(t);
        meshIds.push_back(mesh);
        colors.push_back(c);
        params.push_back(glm::vec4(metallic, rough, bias, shadow ? 1.0f : 0.0f));
    };

    // Подсветка объекта под лучом/взглядом
    if (m_hoverObject >= 0 && size_t(m_hoverObject) < m_scene->objects().size()) {
        const SceneObject& o = m_scene->objects()[size_t(m_hoverObject)];
        const float r = std::max({o.scale.x, o.scale.y, o.scale.z}) * 1.25f;
        add(glm::translate(glm::mat4(1.0f), o.position) * glm::scale(glm::mat4(1.0f), glm::vec3(r)), m_scene->sphereMeshId(),
            m_grab.active ? glm::vec4(1.0f, 0.7f, 0.2f, 0.25f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.15f), 0.0f, 1.0f, 0.0f, false);
    }

    // Руки: суставы (кончики крупнее), луч указания
    if (m_handsProvider && sensorstream::pcNowNs() - m_handsPcNs.load() < 400'000'000LL) {
        for (const auto& h : m_handsProvider->hands()) {
            for (int j = 0; j < LensEngine::HandTrackingProvider::kJoints; ++j) {
                const bool tip = j == 4 || j == 8 || j == 12 || j == 16 || j == 20;
                const bool pinchJoint = h.pinching && (j == 4 || j == 8);
                const float size = tip ? 0.014f : 0.010f;
                const glm::vec4 c = pinchJoint ? glm::vec4(1.0f, 0.6f, 0.15f, 0.95f)
                                               : (h.right ? glm::vec4(0.3f, 0.9f, 1.0f, 0.85f) : glm::vec4(0.6f, 1.0f, 0.5f, 0.85f));
                add(glm::translate(glm::mat4(1.0f), h.world[size_t(j)]) * glm::scale(glm::mat4(1.0f), glm::vec3(size)),
                    m_scene->sphereMeshId(), c, 0.0f, 0.6f, 0.08f, false);
            }
            if (!m_gazeFromEyes) {
                const float len = (m_hoverObject >= 0) ? glm::length(m_scene->objects()[size_t(m_hoverObject)].position - h.aimOrigin) : 2.5f;
                const glm::mat4 t = glm::translate(glm::mat4(1.0f), h.aimOrigin + h.aimDirection * (len * 0.5f)) *
                                    alignZ(h.aimDirection) * glm::scale(glm::mat4(1.0f), glm::vec3(0.003f, 0.003f, len));
                add(t, m_scene->cubeMeshId(), glm::vec4(1.0f, 1.0f, 1.0f, h.pinching ? 0.8f : 0.35f), 0.0f, 1.0f, 0.0f, false);
            }
        }
    }

    // Точка взгляда (айтрекер): маленький круг на поверхности объекта или в 2 м
    if (m_gazeValid && m_gazeFromEyes) {
        const glm::vec3 p = m_gazeOrigin + m_gazeDirection * 2.0f;
        add(glm::translate(glm::mat4(1.0f), p) * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)), m_scene->sphereMeshId(),
            glm::vec4(1.0f, 1.0f, 1.0f, 0.6f), 0.0f, 1.0f, 0.0f, false);
    }

    // Меш комнаты и плоскости (V)
    if (m_showRoom) {
        for (int k = 0; k < 6; ++k)
            add(glm::mat4(1.0f), m_roomMeshIds[k], kindColor(Kind(k)), 0.0f, 1.0f, 0.15f, false);
        for (const auto& p : m_planeViews) add(p.transform, m_scene->cubeMeshId(), p.color, 0.0f, 1.0f, 0.15f, false);
    }
}
