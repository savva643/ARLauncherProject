// Меш комнаты и плоскости: осматриваем синтетическую комнату с двух точек, проверяем точность меша,
// классы поверхностей и найденные плоскости (пол, стол, потолок, стены), сохранение/загрузку объёма.

#include "SceneReconstruction.h"
#include "synthetic_room.h"

#include <chrono>
#include <cstdio>
#include <map>
#include <sstream>

using LensEngine::SceneReconstruction;

int main()
{
    SceneReconstruction recon;
    std::mt19937 rng(3);
    int frames = 0;
    double integrateMs = 0.0;
    for (const glm::dvec3 pos : {glm::dvec3(0.0, -0.5, 1.55), glm::dvec3(-0.8, 0.6, 1.55), glm::dvec3(0.8, 0.3, 1.5)}) {
        for (double yaw = -180; yaw < 180; yaw += 20) {
            for (double pitch : {-35.0, 0.0, 35.0}) {
                const glm::dmat3 rwc = room::headOrientation(yaw, pitch) * room::kDeviceFromCam;
                const auto depth = room::renderDepth(rwc, pos, rng);
                const auto t0 = std::chrono::steady_clock::now();
                recon.integrate(depth.data(), room::kW, room::kH, room::kF, room::kF, room::kCx, room::kCy,
                                glm::quat_cast(rwc), pos);
                integrateMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                ++frames;
            }
        }
    }
    const auto t0 = std::chrono::steady_clock::now();
    recon.update();
    const double updateMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    const auto mesh = recon.mesh();
    const auto planes = recon.planes();

    // Точность меша: расстояние вершин до настоящих поверхностей
    double sum = 0.0;
    size_t far = 0;
    std::map<SceneReconstruction::Kind, size_t> kinds;
    for (size_t i = 0; i < mesh.positions.size(); ++i) {
        const double d = room::surfaceDistance(glm::dvec3(mesh.positions[i]));
        sum += d;
        if (d > 0.05) ++far;
        ++kinds[mesh.kinds[i]];
    }
    const double meanErr = sum / std::max<size_t>(1, mesh.positions.size());
    std::printf("%d frames, integrate %.1f ms/frame, mesh+planes %.0f ms; %zu blocks\n", frames, integrateMs / frames,
                updateMs, recon.blockCount());
    std::printf("mesh: %zu vertices, %zu triangles, mean error %.1f mm, %.1f%% vertices > 5 cm off\n",
                mesh.positions.size(), mesh.indices.size() / 3, meanErr * 1000, 100.0 * far / std::max<size_t>(1, mesh.positions.size()));
    for (const auto& k : kinds) std::printf("  %-8s %zu\n", SceneReconstruction::kindName(k.first), k.second);

    bool floor = false, table = false, ceiling = false;
    int walls = 0;
    for (const auto& p : planes) {
        std::printf("plane #%d %-7s center (%.2f %.2f %.2f) size %.2f x %.2f m, area %.2f m2\n", p.id,
                    SceneReconstruction::kindName(p.kind), p.center.x, p.center.y, p.center.z, 2 * p.halfExtent.x,
                    2 * p.halfExtent.y, p.area);
        if (p.kind == SceneReconstruction::Kind::Floor && std::fabs(p.center.z) < 0.04 && p.area > 4.0f) floor = true;
        if (p.kind == SceneReconstruction::Kind::Table && std::fabs(p.center.z - 0.75) < 0.04 &&
            std::fabs(p.center.x + 1.2) < 0.15 && std::fabs(p.center.y - 1.9) < 0.15) table = true;
        if (p.kind == SceneReconstruction::Kind::Ceiling && std::fabs(p.center.z - 2.6) < 0.05) ceiling = true;
        if (p.kind == SceneReconstruction::Kind::Wall && p.area > 1.0f) ++walls;
    }

    // Сохранение и загрузка объёма дают тот же меш
    std::stringstream ss;
    recon.save(ss);
    SceneReconstruction loaded;
    const bool loadedOk = loaded.load(ss);
    loaded.update();
    const size_t loadedVerts = loaded.mesh().positions.size();
    std::printf("save/load: %zu bytes, %s, %zu vertices after load\n", ss.str().size(), loadedOk ? "ok" : "FAILED", loadedVerts);

    const bool ok = meanErr < 0.015 && far * 100 < mesh.positions.size() * 3 && floor && table && ceiling && walls >= 4 &&
                    loadedOk && loadedVerts > mesh.positions.size() * 9 / 10;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
