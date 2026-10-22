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
    m_portraitCount = 0;
    m_portraitWarned = false;
    if (options.mountRollDeg >= 0) {
        m_mountRoll = options.mountRollDeg == 180 ? 180 : 0;
        m_mountResolved = true;
    } else {
        m_mountResolved = false;
    }

    if (options.enableVio && VioTracker::available()) {
        m_vio = std::make_unique<VioTracker>();
        m_vio->setPoseCallback([this](const VioTracker::Pose& p) {
            m_head.addVioPose(p.tNs, p.orientation, p.position, p.velocity);
        });
        m_vio->start();
    }
    std::cout << "[LensEngine] tracking session started (VIO "
              << (m_vio ? "OpenVINS" : "off — 3DoF + neck model") << ")" << std::endl;
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
            // Канонично ось x устройства смотрит вверх (specific force +x); иначе телефон перевёрнут
            m_mountRoll = (m_mountAccelSum.x >= 0) ? 0 : 180;
            m_mountResolved = true;
            m_gravityLp = m_mountAccelSum / double(m_mountCount);
            std::cout << "[LensEngine] phone mount detected: roll " << m_mountRoll.load() << " deg" << std::endl;
        }
    } else if (hasAccel && m_options.mountRollDeg < 0) {
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

void TrackingSession::updateMount(const glm::dvec3& accel)
{
    // Авто-режим: телефон могли перевернуть после подключения (подключились с телефоном в руке,
    // потом вставили в шлем другой стороной). Гравитация усредняется ~0.5 с, переключение —
    // только если новое положение держится 1 с и телефон явно горизонтален (гистерезис:
    // взгляд вверх/вниз крепление не переключает).
    m_gravityLp += (accel - m_gravityLp) * 0.02;
    const double g = glm::length(m_gravityLp);
    if (g < 5.0) return;   // свободное падение / тряска

    const int desired = m_gravityLp.x >= 0 ? 0 : 180;
    if (std::abs(m_gravityLp.x) > 0.6 * g && desired != m_mountRoll.load()) {
        if (++m_mountFlipCount >= 100) {
            m_mountRoll = desired;
            m_mountFlipCount = 0;
            std::cout << "[LensEngine] phone mount changed: roll " << desired << " deg" << std::endl;
        }
    } else {
        m_mountFlipCount = 0;
    }

    // Портрет: ось y устройства вдоль гравитации. Камера тогда повёрнута на 90° — шлем рассчитан на ландшафт
    if (std::abs(m_gravityLp.y) > 0.8 * g) {
        if (++m_portraitCount >= 200 && !m_portraitWarned) {
            m_portraitWarned = true;
            std::cout << "[LensEngine] WARNING: phone is in portrait — mount it in landscape for correct passthrough" << std::endl;
        }
    } else {
        m_portraitCount = 0;
        m_portraitWarned = false;
    }
}

glm::dquat TrackingSession::deviceFromEye() const
{
    // Глаз (GL): x вправо, y вверх, z назад. Канонично: x_eye = -y_d, y_eye = x_d, z_eye = z_d
    if (m_mountRoll.load() == 180) {
        static const glm::dquat q = glm::quat_cast(glm::dmat3(glm::dvec3(0, 1, 0),
                                                              glm::dvec3(-1, 0, 0),
                                                              glm::dvec3(0, 0, 1)));
        return q;
    }
    static const glm::dquat q = glm::quat_cast(glm::dmat3(glm::dvec3(0, -1, 0),
                                                          glm::dvec3(1, 0, 0),
                                                          glm::dvec3(0, 0, 1)));
    return q;
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
    st.gyroBias = m_head.gyroBias();
    st.mode = st.vioInitialized ? "6DoF (OpenVINS VIO)"
            : st.vioAvailable ? "3DoF (VIO initializing: hold still, then move)"
            : "3DoF + neck model";
    return st;
}

} // namespace LensEngine
