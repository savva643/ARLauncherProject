// Тест HeadTracker на синтетическом движении головы с известной истиной:
// гироскоп с bias и шумом, акселерометр с шумом, 100 Гц.
// Проверяем ошибку ориентации, оценку bias и точность предсказания на 30 мс вперёд.

#include "HeadTracker.h"

#include <cmath>
#include <cstdio>
#include <random>

using namespace LensEngine;

namespace {

constexpr double kPi = 3.14159265358979323846;

// Ориентация телефона в шлеме (камера вперёд по +Y, x устройства вверх), как в stream_simulator
glm::dquat truth(double t)
{
    const glm::dmat3 r0(glm::dvec3(0, 0, 1), glm::dvec3(-1, 0, 0), glm::dvec3(0, -1, 0)); // столбцы
    const double still = 3.0;  // первые 3 с голова неподвижна — bias должен успеть оцениться
    const double tm = std::max(0.0, t - still);
    const double yaw = 40.0 * kPi / 180.0 * std::sin(2 * kPi * 0.25 * tm);
    const double pitch = 15.0 * kPi / 180.0 * std::sin(2 * kPi * 0.17 * tm);
    const glm::dquat qz = glm::angleAxis(yaw, glm::dvec3(0, 0, 1));
    const glm::dquat qx = glm::angleAxis(pitch, glm::dvec3(1, 0, 0));
    return glm::normalize(qz * qx * glm::quat_cast(r0));
}

glm::dvec3 bodyRate(double t)
{
    const double dt = 1e-5;
    const glm::dquat a = truth(t - dt), b = truth(t + dt);
    const glm::dquat d = glm::conjugate(a) * b;
    return glm::dvec3(d.x, d.y, d.z) * (2.0 / (2 * dt)) * (d.w < 0 ? -1.0 : 1.0);
}

double angleDeg(const glm::dquat& a, const glm::dquat& b)
{
    const double d = std::fabs(glm::dot(glm::normalize(a), glm::normalize(b)));
    return 2.0 * std::acos(std::min(1.0, d)) * 180.0 / kPi;
}

} // namespace

int main()
{
    HeadTracker tracker;
    std::mt19937 rng(7);
    std::normal_distribution<double> gN(0, 0.002), aN(0, 0.03);
    const glm::dvec3 bias(0.012, -0.007, 0.009);

    const int64_t t0 = 5'000'000'000LL;
    bool haveOffset = false;
    glm::dquat offset;
    double maxErrMoving = 0, sumErr = 0, maxPredErr = 0;
    int nErr = 0;

    for (int i = 0; i < 30 * 100; ++i) {
        const double t = i * 0.01;
        const int64_t tNs = t0 + int64_t(t * 1e9);
        const glm::dquat q = truth(t);
        const glm::dvec3 gyro = bodyRate(t) + bias + glm::dvec3(gN(rng), gN(rng), gN(rng));
        const glm::dvec3 accel = glm::conjugate(q) * glm::dvec3(0, 0, 9.80665) + glm::dvec3(aN(rng), aN(rng), aN(rng));
        tracker.addImu(tNs, gyro, accel, true);

        auto st = tracker.latest();
        if (!st.initialized) continue;
        if (!haveOffset) {
            offset = st.orientation * glm::conjugate(q);   // рысканье трекера задаётся recenter
            haveOffset = true;
        }
        if (t > 4.0) {
            const double err = angleDeg(st.orientation, offset * q);
            maxErrMoving = std::max(maxErrMoving, err);
            sumErr += err;
            ++nErr;
            // предсказание на 30 мс
            const auto pred = tracker.predict(tNs + 30'000'000);
            maxPredErr = std::max(maxPredErr, angleDeg(pred.orientation, offset * truth(t + 0.03)));
        }
    }

    const glm::dvec3 b = tracker.gyroBias();
    const double biasErr = glm::length(b - bias);
    std::printf("orientation error: mean %.3f deg, max %.3f deg\n", sumErr / std::max(1, nErr), maxErrMoving);
    std::printf("prediction +30ms max error: %.3f deg\n", maxPredErr);
    std::printf("gyro bias estimate: (%.4f %.4f %.4f), error %.4f rad/s\n", b.x, b.y, b.z, biasErr);

    const bool ok = maxErrMoving < 1.5 && maxPredErr < 2.0 && biasErr < 0.004;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
