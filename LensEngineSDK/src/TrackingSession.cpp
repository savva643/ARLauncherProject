#include "TrackingSession.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

namespace LensEngine {

namespace {

// Мир трекера Z-up (вперёд +Y) -> мир рендера Y-up (вперёд -Z), как в ARKit
const glm::dquat& zUpToYUp()
{
    static const glm::dquat q = glm::quat_cast(glm::dmat3(glm::dvec3(1, 0, 0),    // x -> x
                                                          glm::dvec3(0, 0, -1),   // y(вперёд) -> -z
                                                          glm::dvec3(0, 1, 0)));  // z(вверх) -> y
    return q;
}

// Камера OpenCV в осях устройства iPhone: x_c = -y_d, y_c = -x_d, z_c = -z_d
const glm::dquat& deviceFromCameraCV()
{
    static const glm::dquat q = glm::quat_cast(glm::dmat3(glm::dvec3(0, -1, 0),
                                                          glm::dvec3(-1, 0, 0),
                                                          glm::dvec3(0, 0, -1)));
    return q;
}

// Смещение глаз относительно оси вращения шеи (в осях глаза): 7.5 см вверх, 8 см вперёд
const glm::dvec3 kNeckToEye(0.0, 0.075, -0.08);

} // namespace

TrackingSession::TrackingSession() = default;

TrackingSession::~TrackingSession()
{
    stop();
}

void TrackingSession::start(const Options& options)
{
    m_options = options;
    m_head.reset();
    m_mountAccelSum = glm::dvec3(0.0);
    m_mountCount = 0;
    m_mountFlipCount = 0;
    {
        std::lock_guard<std::mutex> lock(m_lidarMutex);
        m_lidar.reset();
        m_anchors.clear();
    }
    m_recon.reset();
    m_lidarFixT = 0;
    m_lastDepthT = 0;
    if (!m_sceneRunning && options.enableLidar) {
        m_sceneRunning = true;
        m_sceneThread = std::thread(&TrackingSession::sceneLoop, this);
    }
    m_floorValid = false;
    setMount(options.mountRollDeg);

    if (options.enableVio && VioTracker::available()) {
        m_vio = std::make_unique<VioTracker>();
        m_vio->setPoseCallback([this](const VioTracker::Pose& p) {
            // LiDAR точнее и не теряет масштаб — VIO только когда LiDAR нет
            if (p.tNs - m_lidarFixT.load() < 500'000'000LL) return;
            m_head.addVioPose(p.tNs, p.orientation, p.position, p.velocity);
        });
        m_vio->start();
    }
    std::cout << "[LensEngine] tracking session started (LiDAR odometry " << (options.enableLidar ? "on" : "off")
              << ", VIO " << (m_vio ? "OpenVINS" : "off") << ")" << std::endl;
}

void TrackingSession::stop()
{
    if (m_vio) {
        m_vio->stop();
        m_vio.reset();
    }
    if (m_sceneRunning) {
        m_sceneRunning = false;
        m_sceneWake.notify_all();
        if (m_sceneThread.joinable()) m_sceneThread.join();
    }
}

void TrackingSession::addImu(int64_t phoneTimeNs, const glm::dvec3& gyro, const glm::dvec3& accel, bool hasAccel)
{
    // Крепление телефона: по гравитации определяем, куда смотрит его верх
    if (!m_mountResolved && hasAccel) {
        m_mountAccelSum += accel;
        if (++m_mountCount >= 20) {
            // Канонично ось x устройства смотрит вверх (specific force +x); смотрим вниз/вверх — пока ландшафт
            m_gravityLp = m_mountAccelSum / double(m_mountCount);
            const int m = mountFromGravity(m_gravityLp);
            m_mountRoll = m >= 0 ? m : 0;
            m_mountResolved = true;
            std::cout << "[LensEngine] phone mount detected: roll " << m_mountRoll.load() << " deg" << std::endl;
        }
    } else if (hasAccel && m_mountAuto.load()) {
        updateMount(accel);
    }

    m_head.addImu(phoneTimeNs, gyro, accel, hasAccel);
    if (m_vio && hasAccel) m_vio->addImu(phoneTimeNs, gyro, accel);
    m_lastImuT = phoneTimeNs;
}

void TrackingSession::addFrame(int64_t phoneTimeNs, const uint8_t* y, int width, int height, int stride,
                               float fx, float fy, float cx, float cy)
{
    if (m_vio) m_vio->addFrame(phoneTimeNs, y, width, height, stride, fx, fy, cx, cy);
}

int TrackingSession::mountFromGravity(const glm::dvec3& g)
{
    const double n = glm::length(g);
    if (n < 5.0) return -1;
    if (std::abs(g.x) > 0.6 * n) return g.x >= 0 ? 0 : 180;    // ландшафт
    if (std::abs(g.y) > 0.8 * n) return g.y >= 0 ? 90 : 270;   // портрет
    return -1;                                                   // смотрим вверх/вниз — не понять
}

void TrackingSession::setMount(int rollDeg)
{
    if (rollDeg < 0) {
        m_mountAuto = true;
        m_mountResolved = false;
        m_mountAccelSum = glm::dvec3(0.0);
        m_mountCount = 0;
        m_mountFlipCount = 0;
        return;
    }
    rollDeg = ((rollDeg % 360) + 360) % 360;
    rollDeg = ((rollDeg + 45) / 90 * 90) % 360;
    m_mountAuto = false;
    m_mountRoll = rollDeg;
    m_mountResolved = true;
    std::cout << "[LensEngine] phone mount set manually: roll " << rollDeg << " deg" << std::endl;
}

void TrackingSession::updateMount(const glm::dvec3& accel)
{
    // Авто-режим: телефон могли перевернуть после подключения (подключились с телефоном в руке,
    // потом вставили в шлем другой стороной). Гравитация усредняется ~0.5 с, переключение —
    // только если новое положение держится 1 с и однозначно (гистерезис: взгляд вверх/вниз
    // и наклоны головы крепление не переключают).
    m_gravityLp += (accel - m_gravityLp) * 0.02;
    const int desired = mountFromGravity(m_gravityLp);
    if (desired >= 0 && desired != m_mountRoll.load()) {
        // В портрет — только при уверенном удержании 2 с (наклон головы на 90° сам по себе редкость)
        const int need = (desired == 90 || desired == 270) ? 200 : 100;
        if (++m_mountFlipCount >= need) {
            m_mountRoll = desired;
            m_mountFlipCount = 0;
            std::cout << "[LensEngine] phone mount changed: roll " << desired << " deg" << std::endl;
        }
    } else {
        m_mountFlipCount = 0;
    }
}

void TrackingSession::addDepth(int64_t phoneTimeNs, const float* depth, int width, int height,
                               float fx, float fy, float cx, float cy)
{
    if (!m_options.enableLidar || !depth) return;
    glm::dquat dev;
    if (!m_head.orientationAt(phoneTimeNs, dev)) return;   // IMU ещё не инициализирован
    const glm::dquat imuWorldFromCam = glm::normalize(dev * deviceFromCameraCV());

    // Смещение с прошлой карты по IMU (акселерометр в фильтре позиции) — подсказка ICP там,
    // где геометрии мало (стена во весь кадр)
    glm::dvec3 p0, p1, imuDelta;
    const bool haveDelta = m_lastDepthT != 0 && m_head.positionAt(m_lastDepthT, p0) && m_head.positionAt(phoneTimeNs, p1);
    if (haveDelta) imuDelta = m_head.worldToFixDirection(p1 - p0);
    m_lastDepthT = phoneTimeNs;

    std::lock_guard<std::mutex> lock(m_lidarMutex);
    const LidarOdometry::Result r = m_lidar.process(phoneTimeNs, depth, width, height, fx, fy, cx, cy, imuWorldFromCam,
                                                    haveDelta ? &imuDelta : nullptr);
    m_lidarKeyframes = r.keyframes;
    if (r.hasFloor) {
        m_floorZ = r.floorZ;
        m_floorValid = true;
    }
    if (!r.ok) return;
    m_lidarRmse = r.rmse;
    m_lidarFixT = phoneTimeNs;
    if (r.revisit) {
        std::cout << "[LensEngine] LiDAR: familiar place, drift reset (" << r.keyframes << " keyframes)" << std::endl;
    }
    // Поза камеры -> поза устройства
    const glm::dquat worldFromDevice = glm::normalize(r.pose.q * glm::conjugate(deviceFromCameraCV()));
    m_head.addPoseFix(phoneTimeNs, worldFromDevice, r.pose.t);

    // Меш комнаты: каждая третья карта (10 Гц хватает, объём копится)
    if (++m_depthCounter % 3 == 0) {
        m_recon.integrate(depth, width, height, fx, fy, cx, cy, r.pose.q, r.pose.t);
        m_reconDirty = true;
    }

    // Сохранённая карта: ищем знакомое место (каждая вторая карта, ~36 совмещений)
    if (m_mapState == MapState::Relocalizing && m_savedLidar && m_depthCounter % 2 == 0) {
        LidarOdometry::Pose saved;
        double quality = 0.0;
        if (m_savedLidar->relocalize(depth, width, height, fx, fy, cx, cy, r.pose.q, saved, quality, 36, m_relocCursor)) {
            // T: текущий мир -> мир карты; оба с гравитацией по Z, поэтому T — поворот вокруг Z + сдвиг
            LidarOdometry::Pose T;
            T.q = glm::normalize(saved.q * glm::conjugate(r.pose.q));
            T.t = saved.t - T.q * r.pose.t;
            const glm::dvec3 z = T.q * glm::dvec3(0, 0, 1);
            if (z.z > std::cos(glm::radians(3.0))) {
                const glm::dvec3 x = T.q * glm::dvec3(1, 0, 0);
                const double yaw = std::atan2(x.y, x.x);
                T.q = glm::angleAxis(yaw, glm::dvec3(0, 0, 1));
                T.t = saved.t - T.q * r.pose.t;
                // Подтверждение вторым совпадением (защита от похожих мест)
                if (m_relocPending && glm::length(T.t - m_relocCandidate.t) < 0.08 &&
                    std::fabs(glm::degrees(2.0 * std::acos(std::min(1.0, std::fabs(glm::dot(T.q, m_relocCandidate.q)))))) < 3.0) {
                    applyRelocalization(T);
                } else {
                    m_relocPending = true;
                    m_relocCandidate = T;
                    m_relocCursor = std::max(0, m_relocCursor - 36);   // ещё раз эти же кандидаты на следующей карте
                }
            }
        }
    }
}

void TrackingSession::applyRelocalization(const LidarOdometry::Pose& T)
{
    // Вызывается под m_lidarMutex
    const glm::dvec3 x = T.q * glm::dvec3(1, 0, 0);
    const double yaw = std::atan2(x.y, x.x);
    m_lidar.transform(T);
    m_lidar.appendKeyframes(*m_savedLidar);
    m_head.rebaseFix(yaw, T.t);
    for (auto& a : m_anchors) {
        a.pose.q = glm::normalize(T.q * a.pose.q);
        a.pose.t = T.q * a.pose.t + T.t;
    }
    for (auto& a : m_mapAnchors) {
        a.id = m_nextAnchorId++;
        m_anchors.push_back(a);
    }
    m_mapAnchors.clear();
    if (m_floorValid) m_floorZ = m_floorZ.load() + T.t.z;
    // Меш комнаты — сохранённый (текущий объём доберётся заново)
    std::istringstream in(m_savedRecon);
    m_recon.reset();
    m_recon.load(in);
    m_reconDirty = true;
    m_savedRecon.clear();
    m_savedLidar.reset();
    m_relocPending = false;
    m_mapState = MapState::Relocalized;
    std::cout << "[LensEngine] world map: familiar place found (yaw " << glm::degrees(yaw) << " deg, shift "
              << glm::length(T.t) << " m), " << m_anchors.size() << " anchors restored" << std::endl;
}

void TrackingSession::sceneLoop()
{
    while (m_sceneRunning) {
        {
            std::unique_lock<std::mutex> lock(m_sceneWakeMutex);
            m_sceneWake.wait_for(lock, std::chrono::milliseconds(1000), [this] { return !m_sceneRunning.load(); });
        }
        if (!m_sceneRunning) break;
        if (!m_reconDirty.exchange(false)) continue;
        m_recon.update(m_floorValid ? m_floorZ.load() : -1e9);
        ++m_roomVersion;
    }
}

// ---------------- мир LiDAR (Z вверх) <-> мир рендера (Y вверх) ----------------

glm::dvec3 TrackingSession::toRender(const glm::dvec3& p) const
{
    return zUpToYUp() * m_head.fixToWorld(p);
}

glm::dquat TrackingSession::toRenderRotation(const glm::dquat& q) const
{
    const glm::dquat yaw = glm::angleAxis(m_head.fixYawOffset(), glm::dvec3(0, 0, 1));
    return glm::normalize(zUpToYUp() * yaw * q * glm::conjugate(zUpToYUp()));
}

glm::dvec3 TrackingSession::fromRender(const glm::dvec3& p) const
{
    return m_head.worldToFix(glm::conjugate(zUpToYUp()) * p);
}

glm::dquat TrackingSession::fromRenderRotation(const glm::dquat& q) const
{
    const glm::dquat yaw = glm::angleAxis(-m_head.fixYawOffset(), glm::dvec3(0, 0, 1));
    return glm::normalize(yaw * glm::conjugate(zUpToYUp()) * q * zUpToYUp());
}

int TrackingSession::addAnchor(const std::string& name, const glm::dvec3& position, const glm::dquat& orientation)
{
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    StoredAnchor a;
    a.id = m_nextAnchorId++;
    a.name = name;
    a.pose.t = fromRender(position);
    a.pose.q = fromRenderRotation(orientation);
    m_anchors.push_back(a);
    return a.id;
}

bool TrackingSession::updateAnchor(int id, const glm::dvec3& position, const glm::dquat& orientation)
{
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    for (auto& a : m_anchors) {
        if (a.id != id) continue;
        a.pose.t = fromRender(position);
        a.pose.q = fromRenderRotation(orientation);
        return true;
    }
    return false;
}

bool TrackingSession::removeAnchor(int id)
{
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    const auto it = std::remove_if(m_anchors.begin(), m_anchors.end(), [id](const StoredAnchor& a) { return a.id == id; });
    const bool found = it != m_anchors.end();
    m_anchors.erase(it, m_anchors.end());
    return found;
}

std::vector<TrackingSession::Anchor> TrackingSession::anchors() const
{
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    std::vector<Anchor> out;
    for (const auto& a : m_anchors) out.push_back({a.id, a.name, toRender(a.pose.t), toRenderRotation(a.pose.q)});
    return out;
}

namespace {
constexpr char kMapMagic[8] = {'G', 'L', 'S', 'K', 'M', 'A', 'P', '1'};
template <typename T>
void put(std::ostream& o, const T& v) { o.write(reinterpret_cast<const char*>(&v), sizeof(T)); }
template <typename T>
bool get(std::istream& i, T& v) { i.read(reinterpret_cast<char*>(&v), sizeof(T)); return bool(i); }
} // namespace

bool TrackingSession::saveWorldMap(const std::string& path, bool force) const
{
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    if (m_mapState == MapState::Relocalizing && !force) {
        std::cout << "[LensEngine] world map: saved place not found yet — not overwriting " << path << std::endl;
        return false;
    }
    if (!m_lidar.initialized()) return false;
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary);
        if (!out) return false;
        out.write(kMapMagic, 8);
        m_lidar.save(out);
        m_recon.save(out);
        put(out, uint32_t(m_anchors.size()));
        for (const auto& a : m_anchors) {
            put(out, uint32_t(a.name.size()));
            out.write(a.name.data(), std::streamsize(a.name.size()));
            put(out, a.pose.q.w); put(out, a.pose.q.x); put(out, a.pose.q.y); put(out, a.pose.q.z);
            put(out, a.pose.t.x); put(out, a.pose.t.y); put(out, a.pose.t.z);
        }
        if (!out) return false;
    }
    std::rename(tmp.c_str(), path.c_str());
    std::cout << "[LensEngine] world map saved: " << path << " (" << m_lidar.keyframeCount() << " keyframes, "
              << m_recon.blockCount() << " room blocks, " << m_anchors.size() << " anchors)" << std::endl;
    return true;
}

bool TrackingSession::loadWorldMap(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char magic[8] = {};
    in.read(magic, 8);
    if (!in || std::string(magic, 8) != std::string(kMapMagic, 8)) return false;
    auto saved = std::make_unique<LidarOdometry>();
    if (!saved->load(in)) return false;
    // объём комнаты — сырыми байтами до релокализации
    const std::streampos reconStart = in.tellg();
    SceneReconstruction probe;
    if (!probe.load(in)) return false;
    const std::streampos reconEnd = in.tellg();
    std::string recon(size_t(reconEnd - reconStart), '\0');
    in.seekg(reconStart);
    in.read(recon.data(), std::streamsize(recon.size()));
    uint32_t count = 0;
    std::vector<StoredAnchor> anchors;
    if (get(in, count)) {
        for (uint32_t i = 0; i < count && i < 100000; ++i) {
            uint32_t len = 0;
            if (!get(in, len) || len > 4096) break;
            StoredAnchor a;
            a.name.resize(len);
            in.read(a.name.data(), len);
            get(in, a.pose.q.w); get(in, a.pose.q.x); get(in, a.pose.q.y); get(in, a.pose.q.z);
            get(in, a.pose.t.x); get(in, a.pose.t.y); get(in, a.pose.t.z);
            if (!in) break;
            anchors.push_back(a);
        }
    }
    std::lock_guard<std::mutex> lock(m_lidarMutex);
    std::cout << "[LensEngine] world map loaded: " << path << " (" << saved->keyframeCount() << " keyframes, "
              << anchors.size() << " anchors) — looking for the familiar place..." << std::endl;
    m_savedLidar = std::move(saved);
    m_savedRecon = std::move(recon);
    m_mapAnchors = std::move(anchors);
    m_relocCursor = 0;
    m_relocPending = false;
    m_mapState = MapState::Relocalizing;
    return true;
}

TrackingSession::RoomMesh TrackingSession::roomMesh() const
{
    const SceneReconstruction::Mesh m = m_recon.mesh();
    RoomMesh out;
    out.version = m_roomVersion.load();
    out.positions.reserve(m.positions.size());
    out.normals.reserve(m.normals.size());
    const glm::dquat rot = zUpToYUp() * glm::angleAxis(m_head.fixYawOffset(), glm::dvec3(0, 0, 1));
    for (size_t i = 0; i < m.positions.size(); ++i) {
        out.positions.push_back(glm::vec3(toRender(glm::dvec3(m.positions[i]))));
        out.normals.push_back(glm::vec3(rot * glm::dvec3(m.normals[i])));
    }
    out.kinds = m.kinds;
    out.indices = m.indices;
    return out;
}

std::vector<SceneReconstruction::Plane> TrackingSession::roomPlanes() const
{
    std::vector<SceneReconstruction::Plane> planes = m_recon.planes();
    const glm::dquat rot = zUpToYUp() * glm::angleAxis(m_head.fixYawOffset(), glm::dvec3(0, 0, 1));
    for (auto& p : planes) {
        p.center = glm::vec3(toRender(glm::dvec3(p.center)));
        p.normal = glm::vec3(rot * glm::dvec3(p.normal));
        p.axisU = glm::vec3(rot * glm::dvec3(p.axisU));
        p.axisV = glm::vec3(rot * glm::dvec3(p.axisV));
    }
    return planes;
}

glm::dquat TrackingSession::deviceFromEye() const
{
    // Глаз (GL): x вправо, y вверх, z назад (к лицу, как ось z устройства).
    // Крепление — поворот вокруг оси z устройства: 0° (ландшафт, верх слева): x_eye = -y_d, y_eye = x_d;
    // 90° (портрет): оси совпадают; 180°: x_eye = y_d, y_eye = -x_d; 270°: x_eye = -x_d, y_eye = -y_d.
    const double theta = glm::radians(double(m_mountRoll.load()) - 90.0);
    return glm::angleAxis(theta, glm::dvec3(0, 0, 1));
}

glm::dquat TrackingSession::eyeToCameraRotation() const
{
    // R_cv_from_eye = R_cv_from_device * R_device_from_eye
    return glm::normalize(glm::conjugate(deviceFromCameraCV()) * deviceFromEye());
}

TrackingSession::EyePose TrackingSession::predictEye(int64_t phoneTimeNs) const
{
    EyePose out;
    const HeadTracker::State s = m_head.predict(phoneTimeNs);
    if (!s.initialized) return out;

    out.valid = true;
    out.phoneTimeNs = phoneTimeNs;
    out.orientation = glm::normalize(zUpToYUp() * s.orientation * deviceFromEye());
    out.angularVelocity = s.angularVelocity;

    if (s.hasPosition) {
        out.position = zUpToYUp() * s.position;
        out.sixDof = true;
    } else if (m_options.neckModel) {
        // Модель шеи: глаза вращаются вокруг точки за ними → естественный параллакс при поворотах
        const glm::dvec3 neutral = kNeckToEye;
        out.position = out.orientation * kNeckToEye - neutral;
    }
    return out;
}

bool TrackingSession::cameraOrientationAt(int64_t phoneTimeNs, glm::dquat& out) const
{
    glm::dquat dev;
    if (!m_head.orientationAt(phoneTimeNs, dev)) return false;
    out = glm::normalize(zUpToYUp() * dev * deviceFromCameraCV());
    return true;
}

void TrackingSession::recenter()
{
    m_head.recenter();
}

TrackingSession::TrackingStatus TrackingSession::status() const
{
    TrackingStatus st;
    st.imuActive = m_lastImuT.load() != 0;
    st.vioAvailable = m_vio != nullptr;
    st.vioInitialized = m_vio && m_vio->initialized();
    st.mountRollDeg = m_mountRoll.load();
    st.mountAuto = m_mountAuto.load();
    st.gyroBias = m_head.gyroBias();
    st.lidarTracking = m_lidarFixT.load() != 0 && m_lastImuT.load() - m_lidarFixT.load() < 500'000'000LL;
    st.lidarKeyframes = m_lidarKeyframes.load();
    st.lidarRmseMm = m_lidarRmse.load() * 1000.0;
    if (m_floorValid.load() && m_head.hasFixFrame()) {
        // Z-up мир LiDAR -> мир трекера -> Y-up мир рендера
        st.hasFloor = true;
        st.floorY = m_head.fixToWorld(glm::dvec3(0.0, 0.0, m_floorZ.load())).z;
    }
    const MapState ms = m_mapState.load();
    st.worldMap = ms == MapState::Relocalizing ? "searching familiar place..."
                : ms == MapState::Relocalized ? "loaded" : "new";
    {
        std::lock_guard<std::mutex> lock(m_lidarMutex);
        st.anchors = int(m_anchors.size());
    }
    st.roomPlanes = int(m_recon.planes().size());
    st.mode = st.lidarTracking ? "6DoF (LiDAR + IMU)"
            : st.vioInitialized ? "6DoF (OpenVINS VIO)"
            : st.vioAvailable ? "3DoF (VIO initializing: hold still, then move)"
            : "3DoF + neck model";
    return st;
}

} // namespace LensEngine
