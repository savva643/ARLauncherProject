// Синтетическая комната для тестов (Z вверх, пол z = 0): стены 5 x 6 x 2.6 м, перегородка, стол, шкаф, комод.
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace room {

struct Box {
    glm::dvec3 lo, hi;
};

inline const Box kRoom{{-2.5, -2.0, 0.0}, {2.5, 4.0, 2.6}};
inline const Box kSolids[] = {
    {{-0.3, 1.0, 0.0}, {0.9, 1.25, 2.0}},     // перегородка
    {{-1.6, 1.5, 0.0}, {-0.8, 2.3, 0.75}},    // стол
    {{1.6, -1.5, 0.0}, {2.5, -0.9, 1.9}},     // шкаф
    {{-2.5, -0.5, 0.0}, {-2.1, 0.7, 1.1}},    // комод
};

inline bool slab(double o, double d, double lo, double hi, double& tmin, double& tmax)
{
    if (std::fabs(d) < 1e-12) return o >= lo && o <= hi;
    double t0 = (lo - o) / d, t1 = (hi - o) / d;
    if (t0 > t1) std::swap(t0, t1);
    tmin = std::max(tmin, t0);
    tmax = std::min(tmax, t1);
    return tmin <= tmax;
}

inline double raycast(const glm::dvec3& o, const glm::dvec3& d)
{
    double best = 1e9;
    {
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

// Расстояние от точки до ближайшей поверхности комнаты
inline double surfaceDistance(const glm::dvec3& p)
{
    auto boxDist = [](const Box& b, const glm::dvec3& q, bool inside) {
        if (inside) {
            return std::min({q.x - b.lo.x, b.hi.x - q.x, q.y - b.lo.y, b.hi.y - q.y, q.z - b.lo.z, b.hi.z - q.z});
        }
        const glm::dvec3 d = glm::max(glm::max(b.lo - q, q - b.hi), glm::dvec3(0.0));
        const double out = glm::length(d);
        if (out > 0) return out;
        return std::min({q.x - b.lo.x, b.hi.x - q.x, q.y - b.lo.y, b.hi.y - q.y, q.z - b.lo.z, b.hi.z - q.z});
    };
    double d = std::fabs(boxDist(kRoom, p, true));
    for (const Box& b : kSolids) d = std::min(d, std::fabs(boxDist(b, p, false)));
    return d;
}

// Камера OpenCV в осях устройства iPhone
inline const glm::dmat3 kDeviceFromCam(glm::dvec3(0, -1, 0), glm::dvec3(-1, 0, 0), glm::dvec3(0, 0, -1));

// Устройство в шлеме: ось x вверх, камера вперёд (+Y мира); yaw/pitch в градусах
inline glm::dmat3 headOrientation(double yawDeg, double pitchDeg)
{
    const glm::dmat3 r0(glm::dvec3(0, 0, 1), glm::dvec3(-1, 0, 0), glm::dvec3(0, -1, 0));
    const glm::dmat3 rz = glm::dmat3(glm::rotate(glm::dmat4(1.0), glm::radians(yawDeg), glm::dvec3(0, 0, 1)));
    const glm::dmat3 rx = glm::dmat3(glm::rotate(glm::dmat4(1.0), glm::radians(pitchDeg), glm::dvec3(1, 0, 0)));
    return rz * rx * r0;
}

constexpr int kW = 256, kH = 192;
constexpr float kF = 190.0f, kCx = 128.0f, kCy = 96.0f;

inline std::vector<float> renderDepth(const glm::dmat3& worldFromCam, const glm::dvec3& pos, std::mt19937& rng,
                                      float noise = 0.004f)
{
    std::normal_distribution<float> n(0.0f, noise);
    std::vector<float> d(size_t(kW) * kH);
    for (int v = 0; v < kH; ++v)
        for (int u = 0; u < kW; ++u) {
            const glm::dvec3 rc((u - kCx) / kF, (v - kCy) / kF, 1.0);
            const double t = raycast(pos, worldFromCam * rc);
            d[size_t(v) * kW + u] = t < 6.0 ? float(t) + n(rng) : 0.0f;
        }
    return d;
}

} // namespace room
