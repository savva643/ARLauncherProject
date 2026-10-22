// Карта мира между запусками: сеанс 1 ходит по комнате, ставит якорь, сохраняет карту.
// Сеанс 2 стартует в другой точке и с другим поворотом головы (его мир рендера другой), загружает карту,
// узнаёт комнату и показывает якорь в том же физическом месте. Плюс меш комнаты и плоскости в сеансе.

#include "TrackingSession.h"
#include "synthetic_room.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <string>

namespace {

using Traj = std::function<void(double t, glm::dmat3& rot, glm::dvec3& pos)>;

struct Sim {
    LensEngine::TrackingSession session;
    std::mt19937 rng{5};
    std::normal_distribution<double> gN{0.0, 0.002}, aN{0.0, 0.03};
    Traj traj;
    double t = 0.0;

    explicit Sim(Traj tr) : traj(std::move(tr))
    {
        LensEngine::TrackingSession::Options o;
        o.enableVio = false;
        o.mountRollDeg = 0;
        session.start(o);
    }
    void run(double seconds, const std::function<void(double)>& onTick = nullptr)
    {
        const double end = t + seconds;
        for (; t < end; t += 0.01) {
            glm::dmat3 r, ra, rb;
            glm::dvec3 p, pa, pb, pc;
            const double h = 1e-4, e = 1e-3;
            traj(t, r, p);
            traj(t - h, ra, pa);
            traj(t + h, rb, pb);
            const glm::dmat3 m = glm::transpose(ra) * rb;
            const glm::dvec3 w((m[1][2] - m[2][1]) / (4 * h), (m[2][0] - m[0][2]) / (4 * h), (m[0][1] - m[1][0]) / (4 * h));
            glm::dmat3 r1, r2;
            glm::dvec3 p1, p2;
            traj(t - e, r1, p1);
            traj(t + e, r2, p2);
            const glm::dvec3 a = (p1 - 2.0 * p + p2) / (e * e);
            const glm::dvec3 f = glm::transpose(r) * (a + glm::dvec3(0, 0, 9.80665));
            const int64_t tNs = int64_t(t * 1e9) + 1'000'000'000LL;
            session.addImu(tNs, w + glm::dvec3(gN(rng), gN(rng), gN(rng)), f + glm::dvec3(aN(rng), aN(rng), aN(rng)), true);
            const int i = int(std::lround(t * 100));
            if (i % 3 == 0 && t > 0.5) {
                const auto depth = room::renderDepth(r * room::kDeviceFromCam, p, rng);
                session.addDepth(tNs, depth.data(), room::kW, room::kH, room::kF, room::kF, room::kCx, room::kCy);
            }
            if (onTick && i % 10 == 0) onTick(t);
        }
    }
    LensEngine::TrackingSession::EyePose eye() { return session.predictEye(int64_t(t * 1e9) + 1'000'000'000LL); }
};

// Направление взгляда в горизонтали (мир рендера) и по правде (комната, Z вверх)
double bearingRender(const glm::dvec3& from, const glm::dquat& eyeRot, const glm::dvec3& to)
{
    const glm::dvec3 f = eyeRot * glm::dvec3(0, 0, -1), d = to - from;
    return std::atan2(f.x * d.z - f.z * d.x, f.x * d.x + f.z * d.z);   // угол от взгляда до цели вокруг вертикали
}
double bearingRoom(const glm::dmat3& devRot, const glm::dvec3& from, const glm::dvec3& to)
{
    const glm::dvec3 f = devRot * glm::dvec3(0, 0, -1), d = to - from;
    // в Z-up мире положительный поворот вокруг Z — против часовой (влево); в Y-up рендере вокруг Y — тоже
    return -std::atan2(f.x * d.y - f.y * d.x, f.x * d.x + f.y * d.y);
}

} // namespace

int main(int argc, char** argv)
{
    const std::string path = std::string(argc > 1 ? argv[1] : "/tmp") + "/test_world.map";

    // --- Сеанс 1: осмотр комнаты, якорь в 1.2 м перед головой на 15-й секунде
    Sim a([](double t, glm::dmat3& r, glm::dvec3& p) {
        r = room::headOrientation(55.0 * std::sin(2 * M_PI * t / 16.0), -15.0 + 10.0 * std::sin(2 * M_PI * t / 9.0));
        p = glm::dvec3(0.6 * std::sin(2 * M_PI * t / 20.0), -0.4 + 0.3 * std::sin(2 * M_PI * t / 13.0), 1.55);
    });
    a.run(15.0);
    const auto eyeA = a.eye();
    const glm::dvec3 anchorRender = eyeA.position + eyeA.orientation * glm::dvec3(0, 0, -1.2);
    glm::dmat3 rA;
    glm::dvec3 pA;
    a.traj(a.t, rA, pA);
    const glm::dvec3 anchorRoom = pA + rA * glm::dvec3(0, 0, -1.2);   // то же место в координатах комнаты
    const int anchorId = a.session.addAnchor("cube", anchorRender, glm::dquat(1, 0, 0, 0));
    a.run(10.0);
    const auto stA = a.session.status();
    const auto planesA = a.session.roomPlanes();
    const auto meshA = a.session.roomMesh();
    std::printf("session 1: %s, %d keyframes, room mesh %zu vertices, %zu planes, anchor #%d at room (%.2f %.2f %.2f)\n",
                stA.mode.c_str(), stA.lidarKeyframes, meshA.positions.size(), planesA.size(), anchorId, anchorRoom.x,
                anchorRoom.y, anchorRoom.z);
    bool floorPlane = false;
    for (const auto& p : planesA)
        if (p.kind == LensEngine::SceneReconstruction::Kind::Floor && p.area > 2.0f && std::fabs(p.normal.y) > 0.95f &&
            std::fabs(p.center.y - stA.floorY) < 0.08)
            floorPlane = true;
    const bool saved = a.session.saveWorldMap(path);
    a.session.stop();

    // --- Сеанс 2: другая точка старта и поворот на 70°
    Sim b([](double t, glm::dmat3& r, glm::dvec3& p) {
        r = room::headOrientation(70.0 + 40.0 * std::sin(2 * M_PI * t / 14.0), -12.0 + 8.0 * std::sin(2 * M_PI * t / 7.0));
        p = glm::dvec3(0.35 + 0.3 * std::sin(2 * M_PI * t / 18.0), 0.1, 1.5);
    });
    const bool loaded = b.session.loadWorldMap(path);
    double relocAt = -1.0;
    b.run(20.0, [&](double t) {
        if (relocAt < 0 && b.session.mapState() == LensEngine::TrackingSession::MapState::Relocalized) relocAt = t;
    });
    double maxDist = 0.0, maxBearing = 0.0, maxHeight = 0.0;
    int checks = 0;
    for (int k = 0; k < 10; ++k) {
        b.run(0.5);
        const auto anchors = b.session.anchors();
        if (anchors.empty()) break;
        const auto e = b.eye();
        glm::dmat3 r;
        glm::dvec3 p;
        b.traj(b.t, r, p);
        const glm::dvec3 d = anchors[0].position - e.position;
        maxDist = std::max(maxDist, std::fabs(glm::length(d) - glm::length(anchorRoom - p)));
        maxHeight = std::max(maxHeight, std::fabs(d.y - (anchorRoom.z - p.z)));
        double db = bearingRender(e.position, e.orientation, anchors[0].position) - bearingRoom(r, p, anchorRoom);
        db = std::atan2(std::sin(db), std::cos(db));
        maxBearing = std::max(maxBearing, std::fabs(glm::degrees(db)));
        ++checks;
    }
    std::printf("session 2: map %s, familiar place found at t=%.1f s, %zu anchors\n", loaded ? "loaded" : "NOT loaded",
                relocAt, b.session.anchors().size());
    std::printf("anchor in session 2 vs truth: distance error %.1f cm, height error %.1f cm, direction error %.2f deg (%d checks)\n",
                maxDist * 100, maxHeight * 100, maxBearing, checks);
    std::printf("room mesh restored: %zu vertices\n", b.session.roomMesh().positions.size());
    b.session.stop();

    const bool ok = saved && loaded && floorPlane && relocAt > 0 && relocAt < 10.0 && checks == 10 && maxDist < 0.05 &&
                    maxHeight < 0.05 && maxBearing < 2.0;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
