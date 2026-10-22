#include "TrackingSession.h"

#include <cmath>
#include <iostream>

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
    m_lidar.reset();
    m_lidarFixT = 0;
    m_lastDepthT = 0;
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
    st.mode = st.lidarTracking ? "6DoF (LiDAR + IMU)"
            : st.vioInitialized ? "6DoF (OpenVINS VIO)"
            : st.vioAvailable ? "3DoF (VIO initializing: hold still, then move)"
            : "3DoF + neck model";
    return st;
}

} // namespace LensEngine
