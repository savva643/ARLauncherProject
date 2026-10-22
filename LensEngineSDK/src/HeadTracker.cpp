#include "HeadTracker.h"

#include <algorithm>
#include <cmath>

namespace LensEngine {

namespace {
constexpr double kGravity = 9.80665;
constexpr double kPi = 3.14159265358979323846;

inline double wrapPi(double a)
{
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}
} // namespace

HeadTracker::HeadTracker() : HeadTracker(Config{}) {}

HeadTracker::HeadTracker(const Config& cfg) : m_cfg(cfg) {}

void HeadTracker::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
    m_lastT = 0;
    m_q = glm::dquat(1, 0, 0, 0);
    m_bias = glm::dvec3(0.0);
    m_biasInt = glm::dvec3(0.0);
    m_omega = m_omegaPrev = glm::dvec3(0.0);
    m_stillSec = 0;
    m_recenterPending = true;
    m_history.clear();
    m_initAccelSum = glm::dvec3(0.0);
    m_initCount = 0;
    m_vioValid = false;
    m_vioYawOffsetInit = false;
}

glm::dquat HeadTracker::expMap(const glm::dvec3& w, double dt)
{
    const glm::dvec3 phi = w * dt;
    const double angle = glm::length(phi);
    if (angle < 1e-12) return glm::dquat(1, 0.5 * phi.x, 0.5 * phi.y, 0.5 * phi.z);
    const glm::dvec3 axis = phi / angle;
    const double s = std::sin(angle * 0.5);
    return glm::dquat(std::cos(angle * 0.5), axis.x * s, axis.y * s, axis.z * s);
}

// Рысканье направления взгляда камеры (ось -z устройства) в горизонтальной плоскости мира
double HeadTracker::yawOf(const glm::dquat& q)
{
    const glm::dvec3 fwd = q * glm::dvec3(0, 0, -1);
    return std::atan2(fwd.y, fwd.x);
}

void HeadTracker::initFromAccel(const glm::dvec3& accel)
{
    // Ориентация, совмещающая измеренное «вверх» (accel в осях устройства) с +Z мира
    const glm::dvec3 up = glm::normalize(accel);
    const glm::dvec3 z(0, 0, 1);
    const double d = glm::dot(up, z);
    glm::dquat q;
    if (d > 1 - 1e-9) {
        q = glm::dquat(1, 0, 0, 0);
    } else if (d < -1 + 1e-9) {
        q = glm::dquat(0, 1, 0, 0);
    } else {
        const glm::dvec3 axis = glm::normalize(glm::cross(up, z));
        q = glm::angleAxis(std::acos(d), axis);
    }
    m_q = glm::normalize(q);
}

void HeadTracker::applyRecenterLocked()
{
    // Поворот вокруг Z мира, чтобы взгляд камеры смотрел на +Y
    const double yaw = yawOf(m_q);
    const glm::dquat fix = glm::angleAxis(kPi / 2 - yaw, glm::dvec3(0, 0, 1));
    m_q = glm::normalize(fix * m_q);
    for (auto& e : m_history) e.q = glm::normalize(fix * e.q);
    m_vioYawOffsetInit = false;
}

void HeadTracker::recenter()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) applyRecenterLocked();
    else m_recenterPending = true;
}

void HeadTracker::addImu(int64_t tNs, const glm::dvec3& gyro, const glm::dvec3& accel, bool hasAccel)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        // Инициализация наклона по среднему акселерометру (~0.2 с)
        if (!hasAccel) return;
        m_initAccelSum += accel;
        if (++m_initCount < 20) return;
        initFromAccel(m_initAccelSum / double(m_initCount));
        m_initialized = true;
        m_lastT = tNs;
        if (m_recenterPending) {
            applyRecenterLocked();
            m_recenterPending = false;
        }
        m_history.push_back({tNs, m_q, glm::dvec3(0.0)});
        return;
    }

    double dt = double(tNs - m_lastT) * 1e-9;
    if (dt <= 0) return;                    // дубликат / старый сэмпл
    if (dt > 0.1) dt = 0.01;                // разрыв потока — не интегрируем мусор
    m_lastT = tNs;

    glm::dvec3 omega = gyro - m_bias;

    // --- оценка bias в покое (единственный способ поймать bias по оси рысканья без камеры)
    const double accelNorm = hasAccel ? glm::length(accel) : kGravity;
    const bool calm = std::fabs(accelNorm - kGravity) < 0.25;
    if (glm::length(omega) < m_cfg.stillGyroThreshold && calm) {
        m_stillSec += dt;
        if (m_stillSec > m_cfg.stillTimeSec) {
            m_bias += m_cfg.stillBiasAlpha * (gyro - m_bias);
            omega = gyro - m_bias;
        }
    } else {
        m_stillSec = 0;
    }

    // --- Mahony: коррекция наклона по гравитации
    glm::dvec3 correction(0.0);
    if (hasAccel && std::fabs(accelNorm - kGravity) < m_cfg.accelGateMs2 && accelNorm > 1e-3) {
        const glm::dvec3 measuredUp = accel / accelNorm;
        const glm::dvec3 estimatedUp = glm::conjugate(m_q) * glm::dvec3(0, 0, 1);
        const glm::dvec3 err = glm::cross(measuredUp, estimatedUp);
        m_biasInt += m_cfg.accelBiasGain * err * dt;
        correction = m_cfg.accelGain * err + m_biasInt;
    }

    m_q = glm::normalize(m_q * expMap(omega + correction, dt));
    m_omegaPrev = m_omega;
    m_omega = omega;

    m_history.push_back({tNs, m_q, omega});
    while (m_history.size() > m_cfg.historySize) m_history.pop_front();
}

bool HeadTracker::orientationAt(int64_t tNs, glm::dquat& out) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || m_history.empty()) return false;

    const Entry& last = m_history.back();
    if (tNs >= last.tNs) {
        const double dt = std::min(double(tNs - last.tNs) * 1e-9, m_cfg.maxPredictSec);
        out = glm::normalize(last.q * expMap(last.omega, dt));
        return true;
    }
    if (tNs <= m_history.front().tNs) {
        out = m_history.front().q;
        return true;
    }
    auto it = std::lower_bound(m_history.begin(), m_history.end(), tNs,
                               [](const Entry& e, int64_t t) { return e.tNs < t; });
    const Entry& b = *it;
    const Entry& a = *(it - 1);
    const double span = double(b.tNs - a.tNs);
    const double u = span > 0 ? double(tNs - a.tNs) / span : 0.0;
    out = glm::normalize(glm::slerp(a.q, b.q, u));
    return true;
}

HeadTracker::State HeadTracker::predict(int64_t tNs) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    State s;
    if (!m_initialized) return s;
    s.initialized = true;

    // Сглаженная угловая скорость (среднее 2 последних) — меньше дрожания предсказания
    const glm::dvec3 omega = 0.5 * (m_omega + m_omegaPrev);
    const double dt = std::clamp(double(tNs - m_lastT) * 1e-9, 0.0, m_cfg.maxPredictSec);
    s.tNs = tNs;
    s.orientation = glm::normalize(m_q * expMap(omega, dt));
    s.angularVelocity = omega;

    if (m_vioValid) {
        const glm::dquat yawFix = glm::angleAxis(m_vioYawOffset, glm::dvec3(0, 0, 1));
        const double dv = std::clamp(double(tNs - m_vioT) * 1e-9, 0.0, 0.1);
        s.position = yawFix * (m_vioPos + m_vioVel * dv);
        s.velocity = yawFix * m_vioVel;
        s.hasPosition = true;
    }
    return s;
}

HeadTracker::State HeadTracker::latest() const
{
    int64_t t;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        t = m_lastT;
    }
    return predict(t);
}

void HeadTracker::addVioPose(int64_t tNs, const glm::dquat& orientation, const glm::dvec3& position,
                             const glm::dvec3& velocity)
{
    glm::dquat ours;
    if (!orientationAt(tNs, ours)) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    const double offset = wrapPi(yawOf(ours) - yawOf(orientation));
    if (!m_vioYawOffsetInit) {
        m_vioYawOffset = offset;
        m_vioYawOffsetInit = true;
    } else {
        // Медленно подтягиваем смещение рысканья (VIO не дрейфует по рысканью так, как гироскоп)
        const double delta = wrapPi(offset - m_vioYawOffset);
        m_vioYawOffset = wrapPi(m_vioYawOffset + 0.05 * delta);
        // Остаточный дрейф нашего фильтра убираем поворотом всей ориентации по Z мира
        const double drift = wrapPi(yawOf(ours) - (yawOf(orientation) + m_vioYawOffset));
        const glm::dquat fix = glm::angleAxis(-0.02 * drift, glm::dvec3(0, 0, 1));
        m_q = glm::normalize(fix * m_q);
    }
    m_vioPos = position;
    m_vioVel = velocity;
    m_vioT = tNs;
    m_vioValid = true;
}

glm::dvec3 HeadTracker::gyroBias() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bias + m_biasInt;
}

bool HeadTracker::isStill() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stillSec > m_cfg.stillTimeSec;
}

} // namespace LensEngine
