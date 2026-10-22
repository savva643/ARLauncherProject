#include "SceneReconstruction.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <istream>
#include <ostream>
#include <unordered_set>

namespace LensEngine {

namespace {

constexpr int kOffset = 1 << 20;   // ключ блока: 3 x 21 бит со сдвигом

struct CellKeyHash {
    size_t operator()(uint64_t k) const { return std::hash<uint64_t>()(k * 0x9E3779B97F4A7C15ULL); }
};

uint64_t cellKey(int x, int y, int z)
{
    return (uint64_t(x + kOffset) << 42) | (uint64_t(y + kOffset) << 21) | uint64_t(z + kOffset);
}

int floorDiv(int a, int b)
{
    return (a >= 0) ? a / b : -((-a + b - 1) / b);
}

} // namespace

const char* SceneReconstruction::kindName(Kind k)
{
    switch (k) {
    case Kind::Floor: return "floor";
    case Kind::Wall: return "wall";
    case Kind::Ceiling: return "ceiling";
    case Kind::Table: return "table";
    case Kind::Other: return "other";
    default: return "none";
    }
}

SceneReconstruction::SceneReconstruction() : m_cfg() {}
SceneReconstruction::SceneReconstruction(const Config& cfg) : m_cfg(cfg) {}

SceneReconstruction::Key SceneReconstruction::key(int bx, int by, int bz)
{
    return cellKey(bx, by, bz);
}

void SceneReconstruction::unkey(Key k, int& bx, int& by, int& bz)
{
    bx = int((k >> 42) & 0x1FFFFF) - kOffset;
    by = int((k >> 21) & 0x1FFFFF) - kOffset;
    bz = int(k & 0x1FFFFF) - kOffset;
}

void SceneReconstruction::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocks.clear();
    m_mesh = Mesh{};
    m_planes.clear();
    m_floorZ = -1e9;
}

size_t SceneReconstruction::blockCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocks.size();
}

double SceneReconstruction::floorZ() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_floorZ;
}

const SceneReconstruction::Voxel* SceneReconstruction::voxelAt(int gx, int gy, int gz) const
{
    const int bx = floorDiv(gx, kBlock), by = floorDiv(gy, kBlock), bz = floorDiv(gz, kBlock);
    auto it = m_blocks.find(key(bx, by, bz));
    if (it == m_blocks.end()) return nullptr;
    const int lx = gx - bx * kBlock, ly = gy - by * kBlock, lz = gz - bz * kBlock;
    return &it->second.v[(lz * kBlock + ly) * kBlock + lx];
}

void SceneReconstruction::integrate(const float* depth, int width, int height, float fx, float fy, float cx, float cy,
                                    const glm::dquat& worldFromCam, const glm::dvec3& camPosition)
{
    if (!depth || width <= 0 || height <= 0 || fx <= 0.0f) return;
    const glm::dmat3 R = glm::mat3_cast(worldFromCam);
    const glm::dmat3 Rt = glm::transpose(R);
    const double vox = m_cfg.voxel, trunc = m_cfg.truncation;
    const double blockSize = vox * kBlock;

    // 1) Какие блоки задеты: точки поверхности ± усечение вдоль луча
    std::unordered_set<Key, CellKeyHash> touched;
    const int step = std::max(1, m_cfg.pixelStep * 2);
    for (int v = 0; v < height; v += step) {
        for (int u = 0; u < width; u += step) {
            const float d = depth[size_t(v) * width + u];
            if (!std::isfinite(d) || d < 0.15f || d > m_cfg.maxDepth) continue;
            const glm::dvec3 ray((u - cx) / fx, (v - cy) / fy, 1.0);
            for (double s : {-trunc, 0.0, trunc}) {
                const glm::dvec3 p = R * (ray * (double(d) + s)) + camPosition;
                touched.insert(key(int(std::floor(p.x / blockSize)), int(std::floor(p.y / blockSize)),
                                   int(std::floor(p.z / blockSize))));
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    // 2) Каждый воксель задетых блоков проецируется в карту глубины
    for (Key k : touched) {
        Block& b = m_blocks[k];
        int bx, by, bz;
        unkey(k, bx, by, bz);
        for (int z = 0; z < kBlock; ++z)
            for (int y = 0; y < kBlock; ++y)
                for (int x = 0; x < kBlock; ++x) {
                    const glm::dvec3 pw(((bx * kBlock + x) + 0.5) * vox, ((by * kBlock + y) + 0.5) * vox,
                                        ((bz * kBlock + z) + 0.5) * vox);
                    const glm::dvec3 pc = Rt * (pw - camPosition);
                    if (pc.z < 0.1) continue;
                    const int u = int(std::lround(fx * pc.x / pc.z + cx));
                    const int v = int(std::lround(fy * pc.y / pc.z + cy));
                    if (u < 0 || v < 0 || u >= width || v >= height) continue;
                    const float d = depth[size_t(v) * width + u];
                    if (!std::isfinite(d) || d < 0.15f || d > m_cfg.maxDepth) continue;
                    const double sdf = double(d) - pc.z;
                    if (sdf < -trunc) continue;   // за поверхностью — не видно
                    const float t = float(std::min(1.0, sdf / trunc));
                    Voxel& vx = b.v[(z * kBlock + y) * kBlock + x];
                    vx.sdf = (vx.sdf * vx.weight + t) / (vx.weight + 1.0f);
                    vx.weight = std::min(vx.weight + 1.0f, m_cfg.maxWeight);
                }
    }
}

SceneReconstruction::Mesh SceneReconstruction::buildMesh(double floorZ) const
{
    Mesh mesh;
    const float vox = m_cfg.voxel;
    auto value = [&](int x, int y, int z, float& out) {
        const Voxel* v = voxelAt(x, y, z);
        if (!v || v->weight < 2.0f) return false;
        out = v->sdf;
        return true;
    };

    // Surface nets: вершина в каждой ячейке со сменой знака (среднее пересечений рёбер)
    std::unordered_map<uint64_t, uint32_t, CellKeyHash> cellVertex;
    static const int corners[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};
    static const int edges[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (const auto& kv : m_blocks) {
        int bx, by, bz;
        unkey(kv.first, bx, by, bz);
        for (int lz = 0; lz < kBlock; ++lz)
            for (int ly = 0; ly < kBlock; ++ly)
                for (int lx = 0; lx < kBlock; ++lx) {
                    const int x = bx * kBlock + lx, y = by * kBlock + ly, z = bz * kBlock + lz;
                    float s[8];
                    bool ok = true;
                    int neg = 0;
                    for (int c = 0; c < 8 && ok; ++c) {
                        ok = value(x + corners[c][0], y + corners[c][1], z + corners[c][2], s[c]);
                        if (s[c] < 0.0f) ++neg;
                    }
                    if (!ok || neg == 0 || neg == 8) continue;
                    // Ложные поверхности на границе «очень далеко/неизвестно» отсекаем: нужен настоящий переход
                    glm::vec3 sum(0.0f);
                    int n = 0;
                    for (const auto& e : edges) {
                        const float a = s[e[0]], b = s[e[1]];
                        if ((a < 0.0f) == (b < 0.0f)) continue;
                        const float t = a / (a - b);
                        const glm::vec3 pa(corners[e[0]][0], corners[e[0]][1], corners[e[0]][2]);
                        const glm::vec3 pb(corners[e[1]][0], corners[e[1]][1], corners[e[1]][2]);
                        sum += pa + (pb - pa) * t;
                        ++n;
                    }
                    const glm::vec3 local = sum / float(n);
                    const glm::vec3 pos = (glm::vec3(x, y, z) + local + glm::vec3(0.5f)) * vox;
                    // нормаль — градиент TSDF (к свободному пространству)
                    glm::vec3 g((s[1] + s[3] + s[5] + s[7]) - (s[0] + s[2] + s[4] + s[6]),
                                (s[2] + s[3] + s[6] + s[7]) - (s[0] + s[1] + s[4] + s[5]),
                                (s[4] + s[5] + s[6] + s[7]) - (s[0] + s[1] + s[2] + s[3]));
                    const float gl = glm::length(g);
                    cellVertex[cellKey(x, y, z)] = uint32_t(mesh.positions.size());
                    mesh.positions.push_back(pos);
                    mesh.normals.push_back(gl > 1e-6f ? g / gl : glm::vec3(0, 0, 1));
                }
    }

    // Грани: каждое ребро сетки со сменой знака — четырёхугольник из 4 ячеек вокруг него
    for (const auto& kv : m_blocks) {
        int bx, by, bz;
        unkey(kv.first, bx, by, bz);
        for (int lz = 0; lz < kBlock; ++lz)
            for (int ly = 0; ly < kBlock; ++ly)
                for (int lx = 0; lx < kBlock; ++lx) {
                    const int x = bx * kBlock + lx, y = by * kBlock + ly, z = bz * kBlock + lz;
                    float s0;
                    if (!value(x, y, z, s0)) continue;
                    for (int axis = 0; axis < 3; ++axis) {
                        const int dx = axis == 0, dy = axis == 1, dz = axis == 2;
                        float s1;
                        if (!value(x + dx, y + dy, z + dz, s1) || ((s0 < 0.0f) == (s1 < 0.0f))) continue;
                        // 4 ячейки, разделяющие ребро (x,y,z)-(x+d)
                        int c[4][3];
                        if (axis == 0) {
                            const int cc[4][3] = {{x, y - 1, z - 1}, {x, y, z - 1}, {x, y, z}, {x, y - 1, z}};
                            std::memcpy(c, cc, sizeof(c));
                        } else if (axis == 1) {
                            const int cc[4][3] = {{x - 1, y, z - 1}, {x - 1, y, z}, {x, y, z}, {x, y, z - 1}};
                            std::memcpy(c, cc, sizeof(c));
                        } else {
                            const int cc[4][3] = {{x - 1, y - 1, z}, {x, y - 1, z}, {x, y, z}, {x - 1, y, z}};
                            std::memcpy(c, cc, sizeof(c));
                        }
                        uint32_t idx[4];
                        bool all = true;
                        for (int i = 0; i < 4 && all; ++i) {
                            auto it = cellVertex.find(cellKey(c[i][0], c[i][1], c[i][2]));
                            all = it != cellVertex.end();
                            if (all) idx[i] = it->second;
                        }
                        if (!all) continue;
                        // Ориентация: лицом к свободному пространству (s > 0)
                        if (s0 > 0.0f) {
                            std::swap(idx[1], idx[3]);
                        }
                        mesh.indices.insert(mesh.indices.end(), {idx[0], idx[1], idx[2], idx[0], idx[2], idx[3]});
                    }
                }
    }

    // Классы поверхностей (как ARMeshClassification)
    double floor = floorZ;
    if (floor < -1e8) {
        // самая низкая горизонтальная поверхность, на которой много вершин
        std::unordered_map<int, int> hist;
        for (size_t i = 0; i < mesh.positions.size(); ++i)
            if (mesh.normals[i].z > 0.85f) ++hist[int(std::floor(mesh.positions[i].z / 0.05f))];
        int best = INT32_MAX;
        for (const auto& h : hist)
            if (h.second >= 150 && h.first < best) best = h.first;
        if (best != INT32_MAX) floor = (best + 0.5) * 0.05;
    }
    float ceiling = -1e9f;
    for (size_t i = 0; i < mesh.positions.size(); ++i)
        if (mesh.normals[i].z < -0.85f) ceiling = std::max(ceiling, mesh.positions[i].z);
    mesh.kinds.resize(mesh.positions.size(), Kind::Other);
    for (size_t i = 0; i < mesh.positions.size(); ++i) {
        const glm::vec3& n = mesh.normals[i];
        const float h = floor > -1e8 ? float(mesh.positions[i].z - floor) : 1e9f;
        Kind k = Kind::Other;
        if (n.z > 0.85f) {
            if (h < 0.12f) k = Kind::Floor;
            else if (h > 0.35f && h < 1.3f) k = Kind::Table;
        } else if (n.z < -0.85f) {
            if (ceiling > -1e8f && mesh.positions[i].z > ceiling - 0.15f && h > 1.8f) k = Kind::Ceiling;
        } else if (std::fabs(n.z) < 0.3f) {
            k = Kind::Wall;
        }
        mesh.kinds[i] = k;
    }
    return mesh;
}

std::vector<SceneReconstruction::Plane> SceneReconstruction::findPlanes(const Mesh& mesh, double floorZ) const
{
    std::vector<Plane> planes;
    const float cell = 0.1f;

    // Компоненты связности на 2D-сетке 10 см -> прямоугольник по PCA
    auto components = [&](const std::vector<glm::vec2>& pts, std::vector<std::vector<int>>& out) {
        std::unordered_map<uint64_t, std::vector<int>, CellKeyHash> grid;
        for (int i = 0; i < int(pts.size()); ++i)
            grid[cellKey(int(std::floor(pts[size_t(i)].x / cell)), int(std::floor(pts[size_t(i)].y / cell)), 0)].push_back(i);
        std::unordered_set<uint64_t, CellKeyHash> visited;
        for (const auto& g : grid) {
            if (visited.count(g.first)) continue;
            std::vector<int> comp;
            std::vector<uint64_t> stack{g.first};
            visited.insert(g.first);
            while (!stack.empty()) {
                const uint64_t k = stack.back();
                stack.pop_back();
                const auto& members = grid[k];
                comp.insert(comp.end(), members.begin(), members.end());
                int cx, cy, cz;
                unkey(k, cx, cy, cz);
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const uint64_t nk = cellKey(cx + dx, cy + dy, 0);
                        if (grid.count(nk) && !visited.count(nk)) {
                            visited.insert(nk);
                            stack.push_back(nk);
                        }
                    }
            }
            out.push_back(std::move(comp));
        }
    };
    auto rectangle = [&](const std::vector<glm::vec2>& pts, const std::vector<int>& idx, glm::vec2& center,
                         glm::vec2& axis, glm::vec2& half) {
        glm::vec2 mean(0.0f);
        for (int i : idx) mean += pts[size_t(i)];
        mean /= float(idx.size());
        // Прямоугольник минимальной площади (перебор углов через 3°): комнаты и столы — прямоугольные
        float bestArea = 1e9f;
        glm::vec2 lo(0.0f), hi(0.0f), perp(0.0f, 1.0f);
        for (int deg = 0; deg < 90; deg += 3) {
            const float a = glm::radians(float(deg));
            const glm::vec2 ax(std::cos(a), std::sin(a)), pp(-ax.y, ax.x);
            glm::vec2 l(1e9f), h(-1e9f);
            for (int i : idx) {
                const glm::vec2 d = pts[size_t(i)] - mean;
                const glm::vec2 q(glm::dot(d, ax), glm::dot(d, pp));
                l = glm::min(l, q);
                h = glm::max(h, q);
            }
            const float area = (h.x - l.x) * (h.y - l.y);
            if (area < bestArea) {
                bestArea = area;
                axis = ax;
                perp = pp;
                lo = l;
                hi = h;
            }
        }
        center = mean + axis * (lo.x + hi.x) * 0.5f + perp * (lo.y + hi.y) * 0.5f;
        half = (hi - lo) * 0.5f + glm::vec2(m_cfg.voxel * 0.5f);
    };

    // --- горизонтальные: пики высоты среди вершин с нормалью вверх/вниз
    for (int dir : {1, -1}) {
        std::unordered_map<int, std::vector<int>> byHeight;
        for (int i = 0; i < int(mesh.positions.size()); ++i)
            if (mesh.normals[size_t(i)].z * dir > 0.9f) byHeight[int(std::floor(mesh.positions[size_t(i)].z / 0.04f))].push_back(i);
        std::vector<std::pair<int, size_t>> peaks;
        for (const auto& b : byHeight) {
            const size_t c = b.second.size() + (byHeight.count(b.first - 1) ? byHeight[b.first - 1].size() : 0) +
                             (byHeight.count(b.first + 1) ? byHeight[b.first + 1].size() : 0);
            const size_t l = byHeight.count(b.first - 1) ? byHeight[b.first - 1].size() : 0;
            const size_t r = byHeight.count(b.first + 1) ? byHeight[b.first + 1].size() : 0;
            if (b.second.size() >= l && b.second.size() > r && c >= 40) peaks.emplace_back(b.first, c);
        }
        for (const auto& pk : peaks) {
            std::vector<glm::vec2> pts;
            std::vector<float> zs;
            for (int d = -1; d <= 1; ++d) {
                auto it = byHeight.find(pk.first + d);
                if (it == byHeight.end()) continue;
                for (int i : it->second) {
                    pts.emplace_back(mesh.positions[size_t(i)].x, mesh.positions[size_t(i)].y);
                    zs.push_back(mesh.positions[size_t(i)].z);
                }
            }
            std::vector<std::vector<int>> comps;
            components(pts, comps);
            for (const auto& comp : comps) {
                const float area = float(comp.size()) * m_cfg.voxel * m_cfg.voxel;
                if (area < 0.12f) continue;
                glm::vec2 c2, axis, half;
                rectangle(pts, comp, c2, axis, half);
                float z = 0.0f;
                for (int i : comp) z += zs[size_t(i)];
                z /= float(comp.size());
                Plane p;
                p.horizontal = true;
                p.center = glm::vec3(c2, z);
                p.normal = glm::vec3(0, 0, float(dir));
                p.axisU = glm::vec3(axis, 0.0f);
                p.axisV = glm::vec3(-axis.y, axis.x, 0.0f);
                p.halfExtent = half;
                p.area = area;
                const double h = floorZ > -1e8 ? z - floorZ : 1e9;
                if (dir < 0) p.kind = h > 1.8 ? Kind::Ceiling : Kind::Other;
                else if (std::fabs(h) < 0.12) p.kind = Kind::Floor;
                else if (h > 0.35 && h < 1.3) p.kind = Kind::Table;
                else p.kind = Kind::Other;
                planes.push_back(p);
            }
        }
    }

    // --- вертикальные (стены): направление нормали + расстояние до начала координат
    std::unordered_map<uint64_t, std::vector<int>> bins;
    for (int i = 0; i < int(mesh.positions.size()); ++i) {
        const glm::vec3& n = mesh.normals[size_t(i)];
        if (std::fabs(n.z) > 0.15f) continue;
        const glm::vec2 h = glm::normalize(glm::vec2(n.x, n.y));
        const float theta = std::atan2(h.y, h.x);
        const float d = glm::dot(h, glm::vec2(mesh.positions[size_t(i)]));
        bins[cellKey(int(std::floor((theta + 3.14159265f) / glm::radians(10.0f))), int(std::floor(d / 0.08f)), 0)].push_back(i);
    }
    std::unordered_set<int> used;
    std::vector<std::pair<size_t, uint64_t>> order;
    for (const auto& b : bins) order.emplace_back(b.second.size(), b.first);
    std::sort(order.rbegin(), order.rend());
    for (const auto& o : order) {
        if (o.first < 40) break;
        const auto& seed = bins[o.second];
        // нормаль и смещение пика
        glm::vec2 nsum(0.0f);
        for (int i : seed) nsum += glm::vec2(mesh.normals[size_t(i)]);
        const glm::vec2 n = glm::normalize(nsum);
        float d = 0.0f;
        for (int i : seed) d += glm::dot(n, glm::vec2(mesh.positions[size_t(i)]));
        d /= float(seed.size());
        // все вершины этой плоскости
        std::vector<int> inl;
        for (int i = 0; i < int(mesh.positions.size()); ++i) {
            if (used.count(i)) continue;
            const glm::vec3& nn = mesh.normals[size_t(i)];
            if (std::fabs(nn.z) > 0.2f || glm::dot(glm::vec2(nn), n) < 0.94f) continue;
            if (std::fabs(glm::dot(n, glm::vec2(mesh.positions[size_t(i)])) - d) > 0.05f) continue;
            inl.push_back(i);
        }
        if (inl.size() < 40) continue;
        const glm::vec2 along(-n.y, n.x);
        std::vector<glm::vec2> pts;   // (вдоль стены, высота)
        for (int i : inl) pts.emplace_back(glm::dot(along, glm::vec2(mesh.positions[size_t(i)])), mesh.positions[size_t(i)].z);
        std::vector<std::vector<int>> comps;
        components(pts, comps);
        for (const auto& comp : comps) {
            const float area = float(comp.size()) * m_cfg.voxel * m_cfg.voxel;
            if (area < 0.25f) continue;
            glm::vec2 lo(1e9f), hi(-1e9f);
            for (int i : comp) {
                lo = glm::min(lo, pts[size_t(i)]);
                hi = glm::max(hi, pts[size_t(i)]);
                used.insert(inl[size_t(i)]);
            }
            Plane p;
            p.horizontal = false;
            p.kind = Kind::Wall;
            p.normal = glm::vec3(n, 0.0f);
            p.axisU = glm::vec3(along, 0.0f);
            p.axisV = glm::vec3(0, 0, 1);
            const float u = (lo.x + hi.x) * 0.5f;
            p.center = glm::vec3(n * d + along * u, (lo.y + hi.y) * 0.5f);
            p.halfExtent = (hi - lo) * 0.5f + glm::vec2(m_cfg.voxel * 0.5f);
            p.area = area;
            planes.push_back(p);
        }
    }
    return planes;
}

void SceneReconstruction::update(double floorZ)
{
    Mesh mesh;
    std::vector<Plane> planes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        mesh = buildMesh(floorZ);
    }
    // пол для плоскостей: переданный или найденный по мешу
    double floor = floorZ;
    if (floor < -1e8) {
        float lowest = 1e9f;
        for (size_t i = 0; i < mesh.positions.size(); ++i)
            if (mesh.kinds[i] == Kind::Floor) lowest = std::min(lowest, mesh.positions[i].z);
        if (lowest < 1e8f) floor = lowest;
    }
    planes = findPlanes(mesh, floor);

    std::lock_guard<std::mutex> lock(m_mutex);
    // Стабильные id: та же плоскость (вид, близкий центр и нормаль) сохраняет номер
    for (auto& p : planes) {
        int best = 0;
        float bestD = 0.5f;
        for (const auto& old : m_planes) {
            if (old.kind != p.kind || glm::dot(old.normal, p.normal) < 0.9f) continue;
            const float d = glm::length(old.center - p.center);
            if (d < bestD) {
                bestD = d;
                best = old.id;
            }
        }
        p.id = best ? best : m_nextPlaneId++;
    }
    mesh.version = m_mesh.version + 1;
    m_mesh = std::move(mesh);
    m_planes = std::move(planes);
    m_floorZ = floor;
}

SceneReconstruction::Mesh SceneReconstruction::mesh() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mesh;
}

std::vector<SceneReconstruction::Plane> SceneReconstruction::planes() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_planes;
}

void SceneReconstruction::save(std::ostream& out) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint32_t magic = 0x46445354;   // TSDF
    const uint32_t count = uint32_t(m_blocks.size());
    out.write(reinterpret_cast<const char*>(&magic), 4);
    out.write(reinterpret_cast<const char*>(&m_cfg.voxel), 4);
    out.write(reinterpret_cast<const char*>(&count), 4);
    for (const auto& kv : m_blocks) {
        out.write(reinterpret_cast<const char*>(&kv.first), 8);
        // 2 байта на воксель: sdf (-1..1 -> int8) и вес
        int8_t buf[kBlock * kBlock * kBlock * 2];
        for (int i = 0; i < kBlock * kBlock * kBlock; ++i) {
            buf[2 * i] = int8_t(std::lround(std::clamp(kv.second.v[i].sdf, -1.0f, 1.0f) * 127.0f));
            buf[2 * i + 1] = int8_t(std::min(127.0f, kv.second.v[i].weight));
        }
        out.write(reinterpret_cast<const char*>(buf), sizeof(buf));
    }
}

bool SceneReconstruction::load(std::istream& in)
{
    uint32_t magic = 0, count = 0;
    float voxel = 0.0f;
    in.read(reinterpret_cast<char*>(&magic), 4);
    in.read(reinterpret_cast<char*>(&voxel), 4);
    in.read(reinterpret_cast<char*>(&count), 4);
    if (!in || magic != 0x46445354 || std::fabs(voxel - m_cfg.voxel) > 1e-6f) return false;
    std::unordered_map<Key, Block> blocks;
    for (uint32_t b = 0; b < count; ++b) {
        Key k = 0;
        int8_t buf[kBlock * kBlock * kBlock * 2];
        in.read(reinterpret_cast<char*>(&k), 8);
        in.read(reinterpret_cast<char*>(buf), sizeof(buf));
        if (!in) return false;
        Block& blk = blocks[k];
        for (int i = 0; i < kBlock * kBlock * kBlock; ++i) {
            blk.v[i].sdf = buf[2 * i] / 127.0f;
            blk.v[i].weight = float(buf[2 * i + 1]);
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocks = std::move(blocks);
    return true;
}

void SceneReconstruction::transform(const glm::dquat& rotation, const glm::dvec3& translation)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unordered_map<Key, Block> moved;
    const double vox = m_cfg.voxel;
    const glm::dmat3 R = glm::mat3_cast(rotation);
    for (const auto& kv : m_blocks) {
        int bx, by, bz;
        unkey(kv.first, bx, by, bz);
        for (int i = 0; i < kBlock * kBlock * kBlock; ++i) {
            const Voxel& v = kv.second.v[i];
            if (v.weight <= 0.0f) continue;
            const int x = i % kBlock, y = (i / kBlock) % kBlock, z = i / (kBlock * kBlock);
            const glm::dvec3 p = R * glm::dvec3((bx * kBlock + x + 0.5) * vox, (by * kBlock + y + 0.5) * vox,
                                                (bz * kBlock + z + 0.5) * vox) + translation;
            const int gx = int(std::floor(p.x / vox)), gy = int(std::floor(p.y / vox)), gz = int(std::floor(p.z / vox));
            const int nbx = floorDiv(gx, kBlock), nby = floorDiv(gy, kBlock), nbz = floorDiv(gz, kBlock);
            Voxel& dst = moved[key(nbx, nby, nbz)].v[((gz - nbz * kBlock) * kBlock + (gy - nby * kBlock)) * kBlock +
                                                      (gx - nbx * kBlock)];
            if (v.weight > dst.weight) dst = v;
        }
    }
    m_blocks = std::move(moved);
}

} // namespace LensEngine
