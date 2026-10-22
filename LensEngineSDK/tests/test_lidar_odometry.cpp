// LiDAR-одометрия + IMU на синтетической комнате.
//  1) LidarOdometry: камера ходит по комнате 40 с и возвращается; IMU-ориентация дрейфует по рысканью
//     0.5°/с (20° за тест). Проверяем позицию и то, что рысканье из ICP не дрейфует.
//  2) TrackingSession целиком: сырой IMU (гироскоп с bias по рысканью) + карты глубины 30 Гц —
//     позиция головы отслеживается, дрейф рысканья гироскопа убран, пол найден.

#include "LidarOdometry.h"
#include "TrackingSession.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Box {
    glm::dvec3 lo, hi;
};

// Комната 5 x 6 x 2.6 м (Z вверх, пол z = 0) + колонна, стол и шкаф
const Box kRoom{{-2.5, -2.0, 0.0}, {2.5, 4.0, 2.6}};
const Box kSolids[] = {
    {{-0.3, 1.0, 0.0}, {0.9, 1.25, 2.0}},     // перегородка
    {{-1.6, 1.5, 0.0}, {-0.8, 2.3, 0.75}},    // стол
    {{1.6, -1.5, 0.0}, {2.5, -0.9, 1.9}},     // шкаф
    {{-2.5, -0.5, 0.0}, {-2.1, 0.7, 1.1}},    // комод
};

bool slab(double o, double d, double lo, double hi, double& tmin, double& tmax)
{
    if (std::fabs(d) < 1e-12) return o >= lo && o <= hi;
    double t0 = (lo - o) / d, t1 = (hi - o) / d;
    if (t0 > t1) std::swap(t0, t1);
    tmin = std::max(tmin, t0);
    tmax = std::min(tmax, t1);
    return tmin <= tmax;
}

// Параметр t до первой поверхности вдоль луча o + t*d
double raycast(const glm::dvec3& o, const glm::dvec3& d)
{
    double best = 1e9;
    {
        // Изнутри комнаты: выход через ближайшую стену
        double tmin = -1e9, tmax = 1e9;
        if (slab(o.x, d.x, kRoom.lo.x, kRoom.hi.x, tmin, tmax) && slab(o.y, d.y, kRoom.lo.y, kRoom.hi.y, tmin, tmax) &&
            slab(o.z, d.z, kRoom.lo.z, kRoom.hi.z, tmin, tmax) && tmax > 0)
            best = tmax;
    }
    for (const Box& b : kSolids) {
        double tmin = 0.0, tmax = 1e9;
        if (slab(o.x, d.x, b.lo.x, b.hi.x, tmin, tmax) && slab(o.y, d.y, b.lo.y, b.hi.y, tmin, tmax) &&
            slab(o.z, d.z, b.lo.z, b.hi.z, tmin, tmax) && tmin > 1e-6)
            best = std::min(best, tmin);
    }
    return best;
}

// Устройство в шлеме: ось x устройства вверх, экран к лицу, камера вперёд (+Y мира)
glm::dmat3 deviceOrientation(double t)
{
    const glm::dmat3 r0(glm::dvec3(0, 0, 1), glm::dvec3(-1, 0, 0), glm::dvec3(0, -1, 0));   // столбцы x_d, y_d, z_d
    const double yaw = glm::radians(40.0) * std::sin(2 * kPi * 0.09 * t);
    const double pitch = glm::radians(-12.0) + glm::radians(10.0) * std::sin(2 * kPi * 0.13 * t);
    const glm::dmat3 rz = glm::dmat3(glm::rotate(glm::dmat4(1.0), yaw, glm::dvec3(0, 0, 1)));
    const glm::dmat3 rx = glm::dmat3(glm::rotate(glm::dmat4(1.0), pitch, glm::dvec3(1, 0, 0)));
    return rz * rx * r0;
}

glm::dvec3 devicePosition(double t)
{
    // Шаги по комнате; через 40 с — снова в начале (периоды 20 с и 40 с)
    return glm::dvec3(0.7 * std::sin(2 * kPi * t / 20.0),
                      -0.5 + 0.6 * (1.0 - std::cos(2 * kPi * t / 40.0)) * 0.5,
                      1.55 + 0.03 * std::sin(2 * kPi * 0.5 * t));
}

const glm::dmat3 kDeviceFromCam(glm::dvec3(0, -1, 0), glm::dvec3(-1, 0, 0), glm::dvec3(0, 0, -1));   // столбцы x_c, y_c, z_c

constexpr int kW = 256, kH = 192;
constexpr float kF = 190.0f, kCx = 128.0f, kCy = 96.0f;

std::vector<float> renderDepth(const glm::dmat3& rwc, const glm::dvec3& pos, std::mt19937& rng)
{
    std::normal_distribution<float> noise(0.0f, 0.004f);
    std::vector<float> d(size_t(kW) * kH);
    for (int v = 0; v < kH; ++v)
        for (int u = 0; u < kW; ++u) {
            const glm::dvec3 rc((u - kCx) / kF, (v - kCy) / kF, 1.0);
            const double t = raycast(pos, rwc * rc);   // rc.z = 1 -> t и есть глубина по оси
            d[size_t(v) * kW + u] = t < 6.0 ? float(t) + noise(rng) : 0.0f;
        }
    return d;
}

double yawOfCam(const glm::dquat& q)
{
    const glm::dvec3 f = q * glm::dvec3(0, 0, 1);
    return std::atan2(f.y, f.x);
}

double wrap(double a)
{
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}

bool testOdometry()
{
    LensEngine::LidarOdometry odo;
    std::mt19937 rng(7);
    std::normal_distribution<double> rotNoise(0.0, glm::radians(0.05));
    const glm::dvec3 p0 = devicePosition(0.0);
    double maxPosErr = 0.0, endPosErr = 0.0, maxYawErr = 0.0;
    int ok = 0, total = 0, revisits = 0, keyframes = 0;
    double floorZ = 0.0;
    bool hasFloor = false;
    for (int i = 0; i <= 40 * 30; ++i) {
        const double t = i / 30.0;
        const glm::dmat3 rwc = deviceOrientation(t) * kDeviceFromCam;
        const glm::dvec3 pos = devicePosition(t);
        const auto depth = renderDepth(rwc, pos, rng);
        // IMU: дрейф рысканья 0.5°/с + шум
        const glm::dquat drift = glm::angleAxis(glm::radians(0.5) * t + rotNoise(rng), glm::dvec3(0, 0, 1));
        const glm::dquat imu = glm::normalize(drift * glm::quat_cast(rwc));
        const auto r = odo.process(int64_t(t * 1e9), depth.data(), kW, kH, kF, kF, kCx, kCy, imu);
        ++total;
        if (r.ok) ++ok;
        if (r.revisit) ++revisits;
        keyframes = r.keyframes;
        const double posErr = glm::length(r.pose.t - (pos - p0));
        if (std::getenv("DEBUG_ODO") && (i % 15 == 0 || (t > 22.6 && t < 23.6)))
            std::printf("odo t=%.1f err %.3f d=(%.3f %.3f %.3f) yawErr %.2f\n", t, posErr, r.pose.t.x - (pos - p0).x,
                        r.pose.t.y - (pos - p0).y, r.pose.t.z - (pos - p0).z,
                        glm::degrees(wrap(yawOfCam(r.pose.q) - yawOfCam(glm::quat_cast(rwc)))));
        const double yawErr = std::fabs(glm::degrees(wrap(yawOfCam(r.pose.q) - yawOfCam(glm::quat_cast(rwc)))));
        maxPosErr = std::max(maxPosErr, posErr);
        maxYawErr = std::max(maxYawErr, yawErr);
        endPosErr = posErr;
        hasFloor = r.hasFloor;
        floorZ = r.floorZ;
    }
    std::printf("odometry: %d/%d frames aligned, %d keyframes, %d revisits\n", ok, total, keyframes, revisits);
    std::printf("odometry: max position error %.1f cm, at end %.1f cm; max yaw error %.2f deg (IMU drifted 20 deg)\n",
                maxPosErr * 100, endPosErr * 100, maxYawErr);
    std::printf("odometry: floor %s at %.3f m (truth %.3f)\n", hasFloor ? "found" : "NOT found", floorZ, -p0.z);
    return ok > total * 0.95 && maxPosErr < 0.06 && endPosErr < 0.04 && maxYawErr < 1.5 && hasFloor &&
           std::fabs(floorZ + p0.z) < 0.05;
}

bool testSession()
{
    LensEngine::TrackingSession session;
    LensEngine::TrackingSession::Options opts;
    opts.enableVio = false;
    opts.mountRollDeg = 0;
    session.start(opts);

    std::mt19937 rng(11);
    std::normal_distribution<double> gyroNoise(0.0, 0.002), accNoise(0.0, 0.03);
    const glm::dvec3 gyroBias(0.0, 0.0, 0.0);
    // Bias по оси x устройства (в шлеме это вертикаль = рысканье): 0.4°/с — Mahony его не видит
    const glm::dvec3 yawBias(glm::radians(0.4), 0.0, 0.0);
    const double g = 9.80665;
    const double T = 40.0;
    double maxPosErr = 0.0, maxYawErr = 0.0, endYawErr = 0.0, sumPosErr = 0.0;
    int nPos = 0;
    const glm::dvec3 p0 = devicePosition(0.0);
    double yaw0 = 0.0;
    bool yaw0Set = false;

    for (int i = 0; i <= int(T * 100); ++i) {
        const double t = i / 100.0;
        const int64_t tNs = int64_t(t * 1e9);
        const glm::dmat3 r = deviceOrientation(t);
        // угловая скорость в осях устройства
        const double h = 1e-4;
        const glm::dmat3 m = glm::transpose(deviceOrientation(t - h)) * deviceOrientation(t + h);
        const glm::dvec3 w((m[1][2] - m[2][1]) / (4 * h), (m[2][0] - m[0][2]) / (4 * h), (m[0][1] - m[1][0]) / (4 * h));
        // ускорение: вторая производная позиции
        const double e = 1e-3;
        const glm::dvec3 a = (devicePosition(t + e) - 2.0 * devicePosition(t) + devicePosition(t - e)) / (e * e);
        const glm::dvec3 f = glm::transpose(r) * (a + glm::dvec3(0, 0, g));
        session.addImu(tNs, w + yawBias + gyroBias + glm::dvec3(gyroNoise(rng), gyroNoise(rng), gyroNoise(rng)),
                       f + glm::dvec3(accNoise(rng), accNoise(rng), accNoise(rng)), true);

        if (i % 3 == 0 && t > 0.5) {
            // LiDAR 33 Гц
            const auto depth = renderDepth(r * kDeviceFromCam, devicePosition(t), rng);
            session.addDepth(tNs, depth.data(), kW, kH, kF, kF, kCx, kCy);
        }
        if (i % 10 == 0 && t > 2.0) {
            const auto eye = session.predictEye(tNs);
            if (!eye.valid) continue;
            // Мир рендера Y-up; позиция головы относительно первой позы LiDAR (t ≈ 0.5 с)
            const glm::dvec3 truthZ = devicePosition(t) - devicePosition(0.51);
            const glm::dvec3 truthY(truthZ.x, truthZ.z, -truthZ.y);
            // рысканье: направление взгляда в горизонтали; сравниваем изменение с начала
            const glm::dvec3 fwd = eye.orientation * glm::dvec3(0, 0, -1);
            const glm::dvec3 fwdTrue = r * glm::dvec3(0, 0, -1);
            const double yawEst = std::atan2(-fwd.z, fwd.x);
            const double yawTrue = std::atan2(fwdTrue.y, fwdTrue.x);
            if (!yaw0Set) {
                yaw0 = wrap(yawEst - yawTrue);
                yaw0Set = true;
            }
            const double yawErr = std::fabs(glm::degrees(wrap(yawEst - yawTrue - yaw0)));
            maxYawErr = std::max(maxYawErr, yawErr);
            endYawErr = yawErr;
            // позиция: в мире трекера стартовое рысканье — как у первой позы, сравниваем модуль ошибки
            // после совмещения рысканья миров (поворот на yaw0 вокруг вертикали)
            const glm::dvec3 rotated = glm::dvec3(glm::rotate(glm::dmat4(1.0), yaw0, glm::dvec3(0, 1, 0)) * glm::dvec4(truthY, 1.0));
            const double err = glm::length(eye.position - rotated);
            if (std::getenv("DEBUG_SESSION") && i % 100 == 0) std::printf("t=%.1f err %.3f est (%.3f %.3f %.3f) truth (%.3f %.3f %.3f)\n", t, err, eye.position.x, eye.position.y, eye.position.z, rotated.x, rotated.y, rotated.z);
            maxPosErr = std::max(maxPosErr, err);
            sumPosErr += err;
            ++nPos;
        }
    }
    const auto st = session.status();
    std::printf("session: mode '%s', %d keyframes, ICP rmse %.1f mm\n", st.mode.c_str(), st.lidarKeyframes, st.lidarRmseMm);
    std::printf("session: head position error mean %.1f cm, max %.1f cm\n", 100 * sumPosErr / std::max(1, nPos), 100 * maxPosErr);
    std::printf("session: yaw error max %.2f deg, at end %.2f deg (gyro bias alone would drift %.0f deg)\n",
                maxYawErr, endYawErr, 0.4 * T);
    std::printf("session: floor %s, y = %.2f m (truth %.2f)\n", st.hasFloor ? "found" : "NOT found", st.floorY,
                -devicePosition(0.51).z);
    (void)p0;
    return st.lidarTracking && maxPosErr < 0.08 && sumPosErr / std::max(1, nPos) < 0.03 && endYawErr < 1.0 &&
           maxYawErr < 2.0 && st.hasFloor && std::fabs(st.floorY + devicePosition(0.51).z) < 0.08;
}

} // namespace

int main()
{
    const bool a = testOdometry();
    const bool b = testSession();
    std::printf("%s\n", a && b ? "PASS" : "FAIL");
    return a && b ? 0 : 1;
}
