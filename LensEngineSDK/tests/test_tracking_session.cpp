// Проверка систем координат всей цепочки репроекции:
// матрица cvFromEye = R_world_from_cam(t_capture)^T * R_world_from_eye(t_display)
// должна совпадать с истинным относительным поворотом камеры (момент съёмки) и глаза (момент показа).
// Если перепутана хоть одна ось (устройство/камера/глаз/мир), ошибка будет десятки градусов.

#include "TrackingSession.h"

#include <cmath>
#include <cstdio>

using namespace LensEngine;

namespace {

constexpr double kPi = 3.14159265358979323846;

glm::dquat truthDevice(double t)
{
    const glm::dmat3 r0(glm::dvec3(0, 0, 1), glm::dvec3(-1, 0, 0), glm::dvec3(0, -1, 0));
    const double tm = std::max(0.0, t - 1.0);
    const double yaw = 35.0 * kPi / 180.0 * std::sin(2 * kPi * 0.3 * tm);
    const double pitch = 15.0 * kPi / 180.0 * std::sin(2 * kPi * 0.2 * tm);
    const double roll = 8.0 * kPi / 180.0 * std::sin(2 * kPi * 0.25 * tm);
    return glm::normalize(glm::angleAxis(yaw, glm::dvec3(0, 0, 1)) *
                          glm::angleAxis(pitch, glm::dvec3(1, 0, 0)) *
                          glm::angleAxis(roll, glm::dvec3(0, 1, 0)) * glm::quat_cast(r0));
}

glm::dvec3 bodyRate(double t)
{
    const double dt = 1e-5;
    const glm::dquat d = glm::conjugate(truthDevice(t - dt)) * truthDevice(t + dt);
    return glm::dvec3(d.x, d.y, d.z) * (1.0 / dt) * (d.w < 0 ? -1.0 : 1.0);
}

double angleDeg(const glm::dmat3& a, const glm::dmat3& b)
{
    const glm::dmat3 r = glm::transpose(a) * b;
    const double c = std::clamp((r[0][0] + r[1][1] + r[2][2] - 1.0) * 0.5, -1.0, 1.0);
    return std::acos(c) * 180.0 / kPi;
}

} // namespace

int main()
{
    TrackingSession session;
    TrackingSession::Options opt;
    opt.enableVio = false;
    session.start(opt);

    const int64_t t0 = 1'000'000'000LL;
    // Истинные постоянные оси (канонический монтаж)
    const glm::dmat3 deviceFromCv(glm::dvec3(0, -1, 0), glm::dvec3(-1, 0, 0), glm::dvec3(0, 0, -1));
    const glm::dmat3 deviceFromEye(glm::dvec3(0, -1, 0), glm::dvec3(1, 0, 0), glm::dvec3(0, 0, 1));

    double maxErr = 0;
    double maxForwardErr = 0;
    for (int i = 0; i <= 600; ++i) {
        const double t = i * 0.01;
        const int64_t tNs = t0 + int64_t(t * 1e9);
        const glm::dquat q = truthDevice(t);
        session.addImu(tNs, bodyRate(t), glm::conjugate(q) * glm::dvec3(0, 0, 9.80665), true);

        if (t < 2.0) continue;
        const int64_t tCap = tNs - 60'000'000;   // кадр снят 60 мс назад
        const auto eye = session.predictEye(tNs);
        glm::dquat worldFromCam;
        if (!eye.valid || !session.cameraOrientationAt(tCap, worldFromCam)) continue;

        const glm::dmat3 m = glm::mat3_cast(glm::conjugate(worldFromCam) * eye.orientation);
        const glm::dmat3 truth = glm::transpose(deviceFromCv) * glm::transpose(glm::mat3_cast(truthDevice(t - 0.06))) *
                                 glm::mat3_cast(truthDevice(t)) * deviceFromEye;
        maxErr = std::max(maxErr, angleDeg(m, truth));

        // Глаз смотрит туда же, куда камера (вперёд, без крена): -z глаза == +z камеры в один момент
        glm::dquat camNow;
        session.cameraOrientationAt(tNs, camNow);
        const glm::dvec3 eyeFwd = eye.orientation * glm::dvec3(0, 0, -1);
        const glm::dvec3 camFwd = camNow * glm::dvec3(0, 0, 1);
        maxForwardErr = std::max(maxForwardErr, std::acos(std::clamp(glm::dot(eyeFwd, camFwd), -1.0, 1.0)) * 180.0 / kPi);
    }

    std::printf("reprojection rotation max error: %.3f deg\n", maxErr);
    std::printf("eye/camera forward mismatch: %.3f deg\n", maxForwardErr);
    const auto st = session.status();
    std::printf("mount: %d deg, mode: %s\n", st.mountRollDeg, st.mode.c_str());
    const bool ok = maxErr < 0.5 && maxForwardErr < 0.5 && st.mountRollDeg == 0;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
