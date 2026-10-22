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
    m_gyroMean = glm::dvec3(0.0);
    m_gyroJitter2 = 1e-4;
    m_recenterPending = true;
    m_history.clear();
    m_initAccelSum = glm::dvec3(0.0);
    m_initCount = 0;
    m_posValid = false;
    m_fixT = 0;
    m_pos = m_vel = m_posOrigin = glm::dvec3(0.0);
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
    // Мир повернулся: смещение рысканья внешнего трекера поворачиваем вместе с ним,
    // а текущую позицию делаем нулём (объекты снова перед глазами)
    m_vioYawOffset = wrapPi(m_vioYawOffset + (kPi / 2 - yaw));
    m_vel = fix * m_vel;
    if (m_posValid) {
        m_posOrigin = fix * (m_pos + m_posOrigin);
        for (auto& e : m_history) e.p = glm::dvec3(0.0);
        m_pos = glm::dvec3(0.0);
    }
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
        m_gyroMean = gyro;
        m_lastT = tNs;
        if (m_recenterPending) {
            applyRecenterLocked();
            m_recenterPending = false;
        }
        m_history.push_back({tNs, m_q, glm::dvec3(0.0), m_pos});
        return;
    }

    double dt = double(tNs - m_lastT) * 1e-9;
    if (dt <= 0) return;                    // дубликат / старый сэмпл
    if (dt > 0.1) dt = 0.01;                // разрыв потока — не интегрируем мусор
    m_lastT = tNs;

    glm::dvec3 omega = gyro - m_bias;

    // --- оценка bias в покое (единственный способ поймать bias по оси рысканья без камеры)
    const double accelNorm = hasAccel ? glm::length(accel) : kGravity;
    // Покой — когда гироскоп показывает только шум: скорость почти постоянна (разброс мал) и невелика.
    // Медленный плавный поворот головы (1-2°/с) тоже «мал», но его скорость меняется — разброс выдаёт.
    // Идём (позиция от LiDAR меняется) — тоже не покой.
    m_gyroMean += 0.1 * (gyro - m_gyroMean);
    const glm::dvec3 dev = gyro - m_gyroMean;
    m_gyroJitter2 += 0.1 * (glm::dot(dev, dev) - m_gyroJitter2);
    const bool calm = std::fabs(accelNorm - kGravity) < 0.25 && (!m_posValid || glm::length(m_vel) < 0.08) &&
                      tNs > m_fixMovingUntil;
    if (glm::length(omega) < m_cfg.stillGyroThreshold && std::sqrt(m_gyroJitter2) < m_cfg.stillGyroJitter && calm) {
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

    // Позиция между позами LiDAR/VIO: интегрируем ускорение в мире (без гравитации).
    // Без поз дольше 0.4 с — держим позицию (двойной интеграл акселерометра быстро «уплывает»).
    if (m_posValid) {
        if (tNs - m_fixT < 400'000'000LL) {
            if (hasAccel) {
                const glm::dvec3 aWorld = m_q * accel - glm::dvec3(0.0, 0.0, kGravity);
                m_pos += m_vel * dt + 0.5 * aWorld * dt * dt;
                m_vel += aWorld * dt;
            } else {
                m_pos += m_vel * dt;
            }
        } else {
            m_vel = glm::dvec3(0.0);
        }
    }

    m_history.push_back({tNs, m_q, omega, m_pos});
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

    if (m_posValid) {
        const double dp = std::clamp(double(tNs - m_lastT) * 1e-9, 0.0, 0.05);
        s.position = m_pos + m_vel * dp;
        s.velocity = m_vel;
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

void HeadTracker::addPoseFix(int64_t tNs, const glm::dquat& orientation, const glm::dvec3& position)
{
    glm::dquat ours;
    if (!orientationAt(tNs, ours)) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    const double offset = wrapPi(yawOf(ours) - yawOf(orientation));
    if (!m_vioYawOffsetInit) {
        m_vioYawOffset = offset;
        m_vioYawOffsetInit = true;
    } else {
        // Внешний трекер не дрейфует по рысканью (LiDAR совмещается со стенами комнаты),
        // а гироскоп — дрейфует. Плавно (τ ≈ 0.3 с) возвращаем наше рысканье к его.
        const double drift = wrapPi(offset - m_vioYawOffset);
        if (std::fabs(drift) < 0.5) {
            const glm::dquat fix = glm::angleAxis(-0.1 * drift, glm::dvec3(0, 0, 1));
            m_q = glm::normalize(fix * m_q);
        } else {
            m_vioYawOffset = offset;   // большое расхождение — трекер перезапустился, берём его заново
        }
    }

    // Внешний трекер видит поворот (даже медленный, который гироскоп не отличит от bias) — это не покой
    if (m_fixT != 0 && tNs > m_fixT) {
        const double dt = double(tNs - m_fixT) * 1e-9;
        const double angle = 2.0 * std::acos(std::min(1.0, std::fabs(glm::dot(m_lastFixQ, orientation))));
        if (dt < 0.5 && angle / dt > 0.01) m_fixMovingUntil = tNs + 300'000'000LL;
    }
    m_lastFixQ = orientation;

    const glm::dvec3 measured = glm::angleAxis(m_vioYawOffset, glm::dvec3(0, 0, 1)) * position - m_posOrigin;
    if (!m_posValid || tNs - m_fixT > 1'000'000'000LL) {
        m_pos = measured;
        m_vel = glm::dvec3(0.0);
        m_posValid = true;
    } else {
        // Поза пришла с задержкой: сравниваем с нашей позицией на её момент, поправку — в текущее состояние
        glm::dvec3 then = m_pos;
        if (!m_history.empty() && tNs >= m_history.front().tNs && tNs <= m_history.back().tNs) {
            auto it = std::lower_bound(m_history.begin(), m_history.end(), tNs,
                                       [](const Entry& e, int64_t t) { return e.tNs < t; });
            then = it->p;
        }
        const glm::dvec3 e = measured - then;
        if (glm::length(e) > 0.5) {
            m_pos += e;                 // скачок (вернулись в знакомое место после потери) — без «пролёта»
            m_vel = glm::dvec3(0.0);
            for (auto& h : m_history) h.p += e;
        } else {
            m_pos += 0.35 * e;
            m_vel += 2.5 * e;
            for (auto& h : m_history) h.p += 0.35 * e;
        }
    }
    m_fixT = tNs;
}

glm::dvec3 HeadTracker::fixToWorld(const glm::dvec3& p) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return glm::angleAxis(m_vioYawOffset, glm::dvec3(0, 0, 1)) * p - m_posOrigin;
}

bool HeadTracker::positionAt(int64_t tNs, glm::dvec3& out) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_posValid || m_history.empty() || tNs < m_history.front().tNs) return false;
    if (tNs >= m_history.back().tNs) {
        out = m_pos + m_vel * std::min(double(tNs - m_history.back().tNs) * 1e-9, 0.05);
        return true;
    }
    auto it = std::lower_bound(m_history.begin(), m_history.end(), tNs,
                               [](const Entry& e, int64_t t) { return e.tNs < t; });
    out = it->p;
    return true;
}

glm::dvec3 HeadTracker::worldToFixDirection(const glm::dvec3& v) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return glm::angleAxis(-m_vioYawOffset, glm::dvec3(0, 0, 1)) * v;
}

bool HeadTracker::hasFixFrame() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_vioYawOffsetInit;
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
