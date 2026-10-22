#include "LidarOdometry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>

namespace LensEngine {

namespace {

using Pose = LidarOdometry::Pose;

Pose compose(const Pose& a, const Pose& b)
{
    Pose r;
    r.q = glm::normalize(a.q * b.q);
    r.t = a.q * b.t + a.t;
    return r;
}

Pose inverse(const Pose& a)
{
    Pose r;
    r.q = glm::conjugate(a.q);
    r.t = -(r.q * a.t);
    return r;
}

double angleOf(const glm::dquat& q)
{
    return 2.0 * std::acos(std::clamp(std::fabs(q.w), 0.0, 1.0));
}

glm::dquat expMap(const glm::dvec3& w)
{
    const double a = glm::length(w);
    if (a < 1e-12) return glm::normalize(glm::dquat(1, 0.5 * w.x, 0.5 * w.y, 0.5 * w.z));
    const glm::dvec3 axis = w / a;
    const double s = std::sin(0.5 * a);
    return glm::dquat(std::cos(0.5 * a), axis.x * s, axis.y * s, axis.z * s);
}

// Решение 6x6 H x = b (H симметричная положительно определённая) через Холецкого
bool solve6(double H[6][6], const double b[6], double x[6])
{
    double L[6][6] = {};
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j <= i; ++j) {
            double s = H[i][j];
            for (int k = 0; k < j; ++k) s -= L[i][k] * L[j][k];
            if (i == j) {
                if (s <= 1e-12) return false;
                L[i][i] = std::sqrt(s);
            } else {
                L[i][j] = s / L[j][j];
            }
        }
    }
    double y[6];
    for (int i = 0; i < 6; ++i) {
        double s = b[i];
        for (int k = 0; k < i; ++k) s -= L[i][k] * y[k];
        y[i] = s / L[i][i];
    }
    for (int i = 5; i >= 0; --i) {
        double s = y[i];
        for (int k = i + 1; k < 6; ++k) s -= L[k][i] * x[k];
        x[i] = s / L[i][i];
    }
    return true;
}

// Минимальное собственное значение симметричной 3x3 (аналитически)
double minEigen3(const double A[3][3])
{
    const double p1 = A[0][1] * A[0][1] + A[0][2] * A[0][2] + A[1][2] * A[1][2];
    if (p1 < 1e-18) return std::min({A[0][0], A[1][1], A[2][2]});
    const double q = (A[0][0] + A[1][1] + A[2][2]) / 3.0;
    const double p2 = (A[0][0] - q) * (A[0][0] - q) + (A[1][1] - q) * (A[1][1] - q) + (A[2][2] - q) * (A[2][2] - q) + 2.0 * p1;
    const double p = std::sqrt(p2 / 6.0);
    double B[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) B[i][j] = (A[i][j] - (i == j ? q : 0.0)) / p;
    const double detB = B[0][0] * (B[1][1] * B[2][2] - B[1][2] * B[2][1]) -
                        B[0][1] * (B[1][0] * B[2][2] - B[1][2] * B[2][0]) +
                        B[0][2] * (B[1][0] * B[2][1] - B[1][1] * B[2][0]);
    const double r = std::clamp(detB / 2.0, -1.0, 1.0);
    const double phi = std::acos(r) / 3.0;
    return q + 2.0 * p * std::cos(phi + 2.0 * 3.14159265358979323846 / 3.0);
}

} // namespace

LidarOdometry::LidarOdometry() : m_cfg() {}

LidarOdometry::LidarOdometry(const Config& cfg) : m_cfg(cfg) {}

void LidarOdometry::reset()
{
    m_keyframes.clear();
    m_current = -1;
    m_nextId = 0;
    m_havePose = false;
    m_pose = Pose{};
    m_velocity = glm::dvec3(0.0);
    m_lastT = 0;
    m_failures = 0;
    m_floorValid = false;
}

void LidarOdometry::buildFrame(const float* depth, int width, int height, float fx, float fy, float cx, float cy,
                               Frame& out) const
{
    const int step = std::max(1, m_cfg.step);
    out.w = width / step;
    out.h = height / step;
    out.fx = fx / step;
    out.fy = fy / step;
    out.cx = cx / step;
    out.cy = cy / step;
    out.v.assign(size_t(out.w) * out.h, glm::vec3(0.0f));
    out.n.assign(out.v.size(), glm::vec3(0.0f));
    out.valid = 0;

    for (int v = 0; v < out.h; ++v) {
        for (int u = 0; u < out.w; ++u) {
            const int su = u * step, sv = v * step;
            const float d = depth[size_t(sv) * width + su];
            if (!std::isfinite(d) || d < m_cfg.minDepth || d > m_cfg.maxDepth) continue;
            out.v[size_t(v) * out.w + u] = glm::vec3((su - cx) / fx * d, (sv - cy) / fy * d, d);
        }
    }
    computeNormals(out);
}

void LidarOdometry::computeNormals(Frame& out)
{
    out.n.assign(out.v.size(), glm::vec3(0.0f));
    out.valid = 0;
    for (int v = 1; v < out.h - 1; ++v) {
        for (int u = 1; u < out.w - 1; ++u) {
            const size_t i = size_t(v) * out.w + u;
            const glm::vec3& p = out.v[i];
            if (p.z <= 0.0f) continue;
            const glm::vec3& l = out.v[i - 1];
            const glm::vec3& r = out.v[i + 1];
            const glm::vec3& t = out.v[i - out.w];
            const glm::vec3& b = out.v[i + out.w];
            if (l.z <= 0.0f || r.z <= 0.0f || t.z <= 0.0f || b.z <= 0.0f) continue;
            // На краях предметов глубина прыгает — там нормали нет
            const float jump = 0.04f + 0.04f * p.z;
            if (std::fabs(r.z - l.z) > jump || std::fabs(b.z - t.z) > jump) continue;
            glm::vec3 n = glm::cross(r - l, b - t);
            const float len = glm::length(n);
            if (len < 1e-9f) continue;
            n /= len;
            if (glm::dot(n, p) > 0.0f) n = -n;   // к камере
            out.n[i] = n;
            ++out.valid;
        }
    }
}

LidarOdometry::IcpResult LidarOdometry::align(const Frame& src, const Keyframe& key, const Pose& keyFromCamInit,
                                              const AlignParams* params) const
{
    const AlignParams defaults{m_cfg.maxPointDist, m_cfg.iterations, 1.0, 1.0, 10.0, m_cfg.minInlierRatio};
    const AlignParams& P = params ? *params : defaults;
    IcpResult res;
    Pose T = keyFromCamInit;
    glm::dvec3 omegaAcc(0.0);
    const Frame& kf = key.frame;
    const int iters = std::max(2, P.iterations);
    int inliers = 0;
    double sse = 0.0;
    bool degenerate = false;

    for (int it = 0; it < iters; ++it) {
        const double maxD = P.startDist + (0.05 - P.startDist) * double(it) / double(iters - 1);
        const double maxD2 = maxD * maxD;
        const glm::dmat3 R = glm::mat3_cast(T.q);
        double H[6][6] = {};
        double g[6] = {};
        inliers = 0;
        sse = 0.0;

        for (size_t i = 0; i < src.v.size(); ++i) {
            const glm::vec3& ns = src.n[i];
            if (ns.x == 0.0f && ns.y == 0.0f && ns.z == 0.0f) continue;
            const glm::dvec3 q = R * glm::dvec3(src.v[i]) + T.t;
            if (q.z < m_cfg.minDepth) continue;
            const int u = int(std::lround(kf.fx * q.x / q.z + kf.cx));
            const int v = int(std::lround(kf.fy * q.y / q.z + kf.cy));
            if (u < 1 || v < 1 || u >= kf.w - 1 || v >= kf.h - 1) continue;
            const size_t k = size_t(v) * kf.w + u;
            const glm::vec3& nkf = kf.n[k];
            if (nkf.x == 0.0f && nkf.y == 0.0f && nkf.z == 0.0f) continue;
            const glm::dvec3 vk(kf.v[k]);
            const glm::dvec3 nk(nkf);
            const glm::dvec3 d = q - vk;
            if (glm::dot(d, d) > maxD2) continue;
            if (glm::dot(R * glm::dvec3(ns), nk) < m_cfg.minNormalDot) continue;

            const double r = glm::dot(nk, d);
            const glm::dvec3 jr = glm::cross(q, nk);
            const double J[6] = {jr.x, jr.y, jr.z, nk.x, nk.y, nk.z};
            const double huber = 0.02;
            const double w = std::fabs(r) < huber ? 1.0 : huber / std::fabs(r);
            for (int a = 0; a < 6; ++a) {
                g[a] += w * J[a] * r;
                for (int b = 0; b <= a; ++b) H[a][b] += w * J[a] * J[b];
            }
            ++inliers;
            sse += r * r;
        }
        if (inliers < 100) return res;
        for (int a = 0; a < 6; ++a)
            for (int b = a + 1; b < 6; ++b) H[a][b] = H[b][a];

        // Мало геометрии (стена во весь кадр): смещение вдоль неё почти не наблюдается
        const double Htt[3][3] = {{H[3][3], H[3][4], H[3][5]}, {H[4][3], H[4][4], H[4][5]}, {H[5][3], H[5][4], H[5][5]}};
        degenerate = minEigen3(Htt) / inliers < 0.02;

        // Приоры: поворот — к IMU, смещение — к предсказанию (IMU/скорость). В хорошо видимых направлениях
        // решает геометрия (её вес в 10-30 раз больше), в слабых — предсказание, и шум ICP туда не копится.
        const double lr = m_cfg.rotPrior * P.rotPriorScale * inliers;
        const double lt = m_cfg.transPrior * P.transPriorScale * inliers;
        const glm::dvec3 dt = T.t - keyFromCamInit.t;
        for (int a = 0; a < 3; ++a) {
            H[a][a] += lr;
            g[a] += lr * omegaAcc[a];
            H[a + 3][a + 3] += lt;
            g[a + 3] += lt * dt[a];
        }
        double b[6], x[6];
        for (int a = 0; a < 6; ++a) b[a] = -g[a];
        if (!solve6(H, b, x)) return res;

        const glm::dvec3 w(x[0], x[1], x[2]);
        const glm::dvec3 t(x[3], x[4], x[5]);
        const glm::dquat dq = expMap(w);
        T.q = glm::normalize(dq * T.q);
        T.t = dq * T.t + t;
        omegaAcc += w;
        if (glm::length(w) < 1e-5 && glm::length(t) < 1e-5) break;
    }

    res.keyFromCam = T;
    res.rmse = std::sqrt(sse / std::max(1, inliers));
    res.inlierRatio = src.valid > 0 ? double(inliers) / src.valid : 0.0;
    res.degenerate = degenerate;
    res.ok = res.inlierRatio >= P.minInlierRatio && res.rmse <= m_cfg.maxRmse &&
             angleOf(glm::conjugate(keyFromCamInit.q) * T.q) < glm::radians(P.maxAngleChangeDeg);
    return res;
}

int LidarOdometry::findKeyframeNear(const Pose& pose, int exclude) const
{
    const glm::dvec3 axis = pose.q * glm::dvec3(0, 0, 1);
    int best = -1;
    double bestScore = 1e9;
    for (int i = 0; i < int(m_keyframes.size()); ++i) {
        if (i == exclude) continue;
        const Pose& kp = m_keyframes[size_t(i)].pose;
        const double dist = glm::length(kp.t - pose.t);
        const double ang = std::acos(std::clamp(glm::dot(axis, kp.q * glm::dvec3(0, 0, 1)), -1.0, 1.0));
        if (dist > m_cfg.keyframeDist * 1.2 || ang > glm::radians(double(m_cfg.keyframeAngleDeg)) * 1.2) continue;
        const double score = dist + 0.3 * ang;
        if (score < bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

void LidarOdometry::updateFloor(const Frame& f, const Pose& pose)
{
    // Пол: самая низкая горизонтальная поверхность хотя бы в 30 см под камерой
    constexpr double kBin = 0.03;
    constexpr int kBins = 100;   // 3 м вниз
    int hist[kBins] = {};
    double sum[kBins] = {};
    const glm::dmat3 R = glm::mat3_cast(pose.q);
    const double top = pose.t.z - 0.3;
    int horizontal = 0;
    for (size_t i = 0; i < f.v.size(); ++i) {
        const glm::vec3& n = f.n[i];
        if (n.x == 0.0f && n.y == 0.0f && n.z == 0.0f) continue;
        if ((R * glm::dvec3(n)).z < 0.9) continue;
        const double z = (R * glm::dvec3(f.v[i]) + pose.t).z;
        const int bin = int((top - z) / kBin);
        if (bin < 0 || bin >= kBins) continue;
        ++hist[bin];
        sum[bin] += z;
        ++horizontal;
    }
    const int need = std::max(40, f.valid / 50);
    for (int b = kBins - 1; b >= 0; --b) {
        const int c = hist[b] + (b > 0 ? hist[b - 1] : 0) + (b + 1 < kBins ? hist[b + 1] : 0);
        if (c < need) continue;
        const double s = sum[b] + (b > 0 ? sum[b - 1] : 0) + (b + 1 < kBins ? sum[b + 1] : 0);
        const double z = s / c;
        if (!m_floorValid || std::fabs(z - m_floorZ) > 0.3) m_floorZ = z;
        else m_floorZ += 0.2 * (z - m_floorZ);
        m_floorValid = true;
        return;
    }
    (void)horizontal;
}

LidarOdometry::Result LidarOdometry::process(int64_t tNs, const float* depth, int width, int height,
                                            float fx, float fy, float cx, float cy, const glm::dquat& imuWorldFromCam,
                                            const glm::dvec3* imuDelta)
{
    Result out;
    out.tNs = tNs;
    if (!depth || width < 16 || height < 16 || fx <= 0.0f || fy <= 0.0f) return out;

    Frame cur;
    buildFrame(depth, width, height, fx, fy, cx, cy, cur);

    if (!m_havePose) {
        if (cur.valid < 500) return out;
        // Первая карта: мир задаёт IMU (гравитация — Z, рысканье — как у трекера головы), позиция 0
        m_pose.q = imuWorldFromCam;
        m_pose.t = glm::dvec3(0.0);
        m_keyframes.push_back({m_nextId++, m_pose, std::move(cur)});
        m_current = 0;
        m_havePose = true;
        m_lastImu = imuWorldFromCam;
        m_lastT = tNs;
        out.ok = true;
        out.pose = m_pose;
        out.newKeyframe = true;
        out.keyframes = 1;
        updateFloor(m_keyframes.back().frame, m_pose);
        out.hasFloor = m_floorValid;
        out.floorZ = m_floorZ;
        return out;
    }

    const double dt = std::clamp(double(tNs - m_lastT) * 1e-9, 0.0, 0.2);
    // Предсказание: поворот — гироскоп (относительный, без накопленного дрейфа), смещение — постоянная скорость
    Pose pred;
    pred.q = glm::normalize(m_pose.q * (glm::conjugate(m_lastImu) * imuWorldFromCam));
    pred.t = m_pose.t + (imuDelta ? *imuDelta : m_velocity * dt);
    m_lastImu = imuWorldFromCam;
    m_lastT = tNs;

    IcpResult icp;
    int keyIndex = m_current;
    if (cur.valid >= 300) {
        const Keyframe& key = m_keyframes[size_t(m_current)];
        icp = align(cur, key, compose(inverse(key.pose), pred));
        if (!icp.ok) {
            // Потеряли текущий ключевой кадр (резкое движение, закрыли датчик) — пробуем соседний по карте
            const int j = findKeyframeNear(pred, m_current);
            if (j >= 0) {
                const Keyframe& alt = m_keyframes[size_t(j)];
                IcpResult r = align(cur, alt, compose(inverse(alt.pose), pred));
                if (r.ok) {
                    icp = r;
                    keyIndex = j;
                    out.revisit = true;
                }
            }
        }
    }

    if (icp.ok) {
        const Pose pose = compose(m_keyframes[size_t(keyIndex)].pose, icp.keyFromCam);
        if (dt > 1e-3) {
            glm::dvec3 v = (pose.t - m_pose.t) / dt;
            const double speed = glm::length(v);
            if (speed > 3.0) v *= 3.0 / speed;
            m_velocity = glm::mix(m_velocity, v, 0.5);
        }
        m_pose = pose;
        m_current = keyIndex;
        m_failures = 0;
        out.ok = true;
        out.rmse = icp.rmse;
        out.inlierRatio = icp.inlierRatio;
        out.degenerate = icp.degenerate;

        // Ушли от ключевого кадра: сначала ищем знакомое место на карте, иначе — новый ключевой кадр
        const Pose rel = compose(inverse(m_keyframes[size_t(m_current)].pose), m_pose);
        if (glm::length(rel.t) > m_cfg.keyframeDist || angleOf(rel.q) > glm::radians(double(m_cfg.keyframeAngleDeg)) ||
            icp.inlierRatio < 0.5) {
            bool switched = false;
            const int j = findKeyframeNear(m_pose, m_current);
            if (j >= 0) {
                const Keyframe& alt = m_keyframes[size_t(j)];
                IcpResult r = align(cur, alt, compose(inverse(alt.pose), m_pose));
                if (r.ok && r.inlierRatio >= 0.5) {
                    out.revisit = alt.id < m_keyframes[size_t(m_current)].id - 1;
                    m_current = j;
                    m_pose = compose(alt.pose, r.keyFromCam);
                    switched = true;
                }
            }
            if (!switched) {
                m_keyframes.push_back({m_nextId++, m_pose, cur});
                m_current = int(m_keyframes.size()) - 1;
                out.newKeyframe = true;
                if (m_keyframes.size() > m_cfg.maxKeyframes) {
                    m_keyframes.pop_front();
                    --m_current;
                }
            }
        }
        updateFloor(cur, m_pose);
    } else {
        // Нет совмещения: поворот по IMU, позиция почти стоит (скорость гасится)
        m_pose = pred;
        m_velocity *= 0.5;
        if (++m_failures > 45) {
            // ~1.5 с без совмещения — начинаем новый участок карты с последней позы
            m_keyframes.push_back({m_nextId++, m_pose, cur});
            m_current = int(m_keyframes.size()) - 1;
            m_failures = 0;
            out.newKeyframe = true;
            if (m_keyframes.size() > m_cfg.maxKeyframes) {
                m_keyframes.pop_front();
                --m_current;
            }
        }
    }

    if (std::getenv("LIDAR_DEBUG") && (std::getenv("LIDAR_DEBUG")[0] == '2' || out.revisit || out.newKeyframe || !out.ok || out.degenerate))
        std::printf("[lidar %.2f] ok=%d rev=%d new=%d deg=%d rmse=%.4f inl=%.2f kf=%d t=(%.3f %.3f %.3f)\n", tNs * 1e-9, out.ok,
                    out.revisit, out.newKeyframe, out.degenerate, out.rmse, out.inlierRatio, m_keyframes[size_t(m_current)].id,
                    m_pose.t.x, m_pose.t.y, m_pose.t.z);
    out.pose = m_pose;
    out.keyframes = int(m_keyframes.size());
    out.hasFloor = m_floorValid;
    out.floorZ = m_floorZ;
    return out;
}

// ---------------- Карта мира ----------------

namespace {
template <typename T>
void put(std::ostream& o, const T& v) { o.write(reinterpret_cast<const char*>(&v), sizeof(T)); }
template <typename T>
bool get(std::istream& i, T& v) { i.read(reinterpret_cast<char*>(&v), sizeof(T)); return bool(i); }
} // namespace

void LidarOdometry::save(std::ostream& out) const
{
    put(out, uint32_t(0x4F44494C));   // LIDO
    put(out, uint32_t(m_keyframes.size()));
    for (const auto& kf : m_keyframes) {
        put(out, int32_t(kf.id));
        put(out, kf.pose.q.w); put(out, kf.pose.q.x); put(out, kf.pose.q.y); put(out, kf.pose.q.z);
        put(out, kf.pose.t.x); put(out, kf.pose.t.y); put(out, kf.pose.t.z);
        const Frame& f = kf.frame;
        put(out, int32_t(f.w)); put(out, int32_t(f.h));
        put(out, f.fx); put(out, f.fy); put(out, f.cx); put(out, f.cy);
        // глубина в мм (точки восстанавливаются по интринсикам)
        std::vector<uint16_t> mm(f.v.size());
        for (size_t i = 0; i < f.v.size(); ++i) mm[i] = uint16_t(std::clamp(f.v[i].z * 1000.0f + 0.5f, 0.0f, 65535.0f));
        out.write(reinterpret_cast<const char*>(mm.data()), std::streamsize(mm.size() * 2));
    }
    put(out, uint8_t(m_floorValid));
    put(out, m_floorZ);
}

bool LidarOdometry::load(std::istream& in)
{
    uint32_t magic = 0, count = 0;
    if (!get(in, magic) || magic != 0x4F44494C || !get(in, count) || count > 100000) return false;
    std::deque<Keyframe> kfs;
    int maxId = 0;
    for (uint32_t k = 0; k < count; ++k) {
        Keyframe kf;
        int32_t id = 0, w = 0, h = 0;
        if (!get(in, id)) return false;
        get(in, kf.pose.q.w); get(in, kf.pose.q.x); get(in, kf.pose.q.y); get(in, kf.pose.q.z);
        get(in, kf.pose.t.x); get(in, kf.pose.t.y); get(in, kf.pose.t.z);
        get(in, w); get(in, h);
        Frame& f = kf.frame;
        get(in, f.fx); get(in, f.fy); get(in, f.cx); get(in, f.cy);
        if (!in || w <= 0 || h <= 0 || w * h > 1 << 22) return false;
        f.w = w;
        f.h = h;
        std::vector<uint16_t> mm(size_t(w) * h);
        in.read(reinterpret_cast<char*>(mm.data()), std::streamsize(mm.size() * 2));
        if (!in) return false;
        f.v.assign(mm.size(), glm::vec3(0.0f));
        for (int v = 0; v < h; ++v)
            for (int u = 0; u < w; ++u) {
                const float d = mm[size_t(v) * w + u] * 0.001f;
                if (d <= 0.0f) continue;
                f.v[size_t(v) * w + u] = glm::vec3((u - f.cx) / f.fx * d, (v - f.cy) / f.fy * d, d);
            }
        computeNormals(f);
        kf.id = id;
        maxId = std::max(maxId, int(id));
        kfs.push_back(std::move(kf));
    }
    uint8_t floorValid = 0;
    double floorZ = 0.0;
    get(in, floorValid);
    get(in, floorZ);
    reset();
    m_keyframes = std::move(kfs);
    m_nextId = maxId + 1;
    m_current = m_keyframes.empty() ? -1 : 0;
    m_havePose = false;
    m_floorValid = floorValid != 0;
    m_floorZ = floorZ;
    return true;
}

bool LidarOdometry::relocalize(const float* depth, int width, int height, float fx, float fy, float cx, float cy,
                               const glm::dquat& imuWorldFromCam, Pose& worldFromCam, double& quality, int budget,
                               int& cursor) const
{
    if (m_keyframes.empty() || !depth) return false;
    Frame cur;
    buildFrame(depth, width, height, fx, fy, cx, cy, cur);
    if (cur.valid < 500) return false;

    constexpr int kYaws = 18;   // через 20°
    // Широкое окно ассоциации, слабый приор поворота (рысканье неизвестно), строже по качеству
    const AlignParams P{0.6f, 30, 0.05, 0.1, 25.0, 0.55};
    const int total = int(m_keyframes.size()) * kYaws;
    double best = 0.0;
    for (int n = 0; n < budget && n < total; ++n) {
        const int c = cursor++ % total;
        const Keyframe& kf = m_keyframes[size_t(c / kYaws)];
        const double yaw = (c % kYaws) * (2.0 * 3.14159265358979323846 / kYaws);
        Pose init;
        init.q = glm::normalize(glm::angleAxis(yaw, glm::dvec3(0, 0, 1)) * imuWorldFromCam);
        init.t = kf.pose.t;
        const IcpResult r = align(cur, kf, compose(inverse(kf.pose), init), &P);
        if (!r.ok || r.rmse > 0.02) continue;
        const double q = r.inlierRatio - 5.0 * r.rmse;
        if (q > best) {
            best = q;
            worldFromCam = compose(kf.pose, r.keyFromCam);
        }
    }
    quality = best;
    return best > 0.0;
}

void LidarOdometry::transform(const Pose& T)
{
    for (auto& kf : m_keyframes) kf.pose = compose(T, kf.pose);
    m_pose = compose(T, m_pose);
    m_velocity = T.q * m_velocity;
    // пол остаётся горизонтальным: T — поворот вокруг вертикали + сдвиг
    m_floorZ += T.t.z;
}

void LidarOdometry::appendKeyframes(const LidarOdometry& other)
{
    for (const auto& kf : other.m_keyframes) {
        Keyframe copy = kf;
        copy.id = m_nextId++;
        m_keyframes.push_front(std::move(copy));   // старые — в начало (вытесняются первыми при переполнении)
        if (m_current >= 0) ++m_current;
    }
    while (m_keyframes.size() > m_cfg.maxKeyframes) {
        m_keyframes.pop_front();
        if (m_current > 0) --m_current;
    }
    if (m_current < 0 && !m_keyframes.empty()) m_current = int(m_keyframes.size()) - 1;
    if (!m_floorValid && other.m_floorValid) {
        m_floorValid = true;
        m_floorZ = other.m_floorZ;
    }
}

} // namespace LensEngine
