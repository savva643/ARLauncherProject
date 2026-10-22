#include "EyeTracker.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace LensEngine {

namespace {

// Решение n x n (n = 6) методом Гаусса с выбором ведущего элемента
bool solve(std::array<std::array<double, 7>, 6>& m)
{
    const int n = 6;
    for (int c = 0; c < n; ++c) {
        int piv = c;
        for (int r = c + 1; r < n; ++r)
            if (std::fabs(m[size_t(r)][size_t(c)]) > std::fabs(m[size_t(piv)][size_t(c)])) piv = r;
        if (std::fabs(m[size_t(piv)][size_t(c)]) < 1e-12) return false;
        std::swap(m[size_t(c)], m[size_t(piv)]);
        for (int r = 0; r < n; ++r) {
            if (r == c) continue;
            const double f = m[size_t(r)][size_t(c)] / m[size_t(c)][size_t(c)];
            for (int k = c; k <= n; ++k) m[size_t(r)][size_t(k)] -= f * m[size_t(c)][size_t(k)];
        }
    }
    for (int r = 0; r < n; ++r) m[size_t(r)][size_t(n)] /= m[size_t(r)][size_t(r)];
    return true;
}

} // namespace

EyeTracker::EyeTracker() = default;

EyeTracker::Pupil EyeTracker::detectPupil(const uint8_t* gray, int width, int height, int stride)
{
    Pupil out;
    if (!gray || width < 32 || height < 24) return out;

    // 1) Уменьшенная копия (~128 px по ширине) и её интегральное изображение
    const int f = std::max(1, width / 128);
    const int w = width / f, h = height / f;
    std::vector<float> small(size_t(w) * h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int sum = 0;
            for (int dy = 0; dy < f; ++dy)
                for (int dx = 0; dx < f; ++dx) sum += gray[size_t(y * f + dy) * stride + size_t(x * f + dx)];
            small[size_t(y) * w + x] = float(sum) / float(f * f);
        }
    std::vector<double> integral(size_t(w + 1) * (h + 1), 0.0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            integral[size_t(y + 1) * (w + 1) + x + 1] = small[size_t(y) * w + x] + integral[size_t(y) * (w + 1) + x + 1] +
                                                        integral[size_t(y + 1) * (w + 1) + x] - integral[size_t(y) * (w + 1) + x];
    auto boxMean = [&](int x0, int y0, int s) {
        const double sum = integral[size_t(y0 + s) * (w + 1) + x0 + s] - integral[size_t(y0) * (w + 1) + x0 + s] -
                           integral[size_t(y0 + s) * (w + 1) + x0] + integral[size_t(y0) * (w + 1) + x0];
        return float(sum / double(s * s));
    };

    // 2) Самое тёмное окно (зрачок под ИК — самое тёмное место)
    const int s = std::max(4, w / 16);
    float darkest = 1e9f;
    int bx = 0, by = 0;
    for (int y = 0; y + s <= h; ++y)
        for (int x = 0; x + s <= w; ++x) {
            const float m = boxMean(x, y, s);
            if (m < darkest) {
                darkest = m;
                bx = x;
                by = y;
            }
        }
    // фон: медиана уменьшенного кадра
    std::vector<float> sorted = small;
    std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
    const float bright = sorted[sorted.size() / 2];
    if (bright - darkest < 20.0f) return out;   // нет контраста — глаз закрыт или нет подсветки
    const float thr = darkest + 0.35f * (bright - darkest);

    // 3) Заливка от самого тёмного пикселя окна на полном разрешении, в пределах окрестности
    int seedX = (bx + s / 2) * f, seedY = (by + s / 2) * f;
    int bestV = 256;
    for (int y = by * f; y < (by + s) * f && y < height; ++y)
        for (int x = bx * f; x < (bx + s) * f && x < width; ++x)
            if (gray[size_t(y) * stride + x] < bestV) {
                bestV = gray[size_t(y) * stride + x];
                seedX = x;
                seedY = y;
            }
    const int maxR = width / 4;
    const int x0 = std::max(0, seedX - maxR), x1 = std::min(width - 1, seedX + maxR);
    const int y0 = std::max(0, seedY - maxR), y1 = std::min(height - 1, seedY + maxR);
    const int rw = x1 - x0 + 1, rh = y1 - y0 + 1;
    std::vector<uint8_t> mask(size_t(rw) * rh, 0);
    std::vector<int> stack{(seedY - y0) * rw + (seedX - x0)};
    mask[size_t(stack[0])] = 1;
    int area = 0;
    while (!stack.empty()) {
        const int i = stack.back();
        stack.pop_back();
        ++area;
        const int x = i % rw, y = i / rw;
        const int nb[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};
        for (const auto& q : nb) {
            if (q[0] < 0 || q[1] < 0 || q[0] >= rw || q[1] >= rh) continue;
            const size_t j = size_t(q[1]) * rw + q[0];
            if (mask[j] || gray[size_t(q[1] + y0) * stride + q[0] + x0] > thr) continue;
            mask[j] = 1;
            stack.push_back(int(j));
        }
    }
    // 4) Дыры от бликов ИК-диодов: в каждой строке заполняем от крайнего левого до крайнего правого
    double sx = 0, sy = 0;
    int filled = 0, minX = rw, maxX = -1, minY = rh, maxY = -1;
    for (int y = 0; y < rh; ++y) {
        int l = -1, r = -1;
        for (int x = 0; x < rw; ++x)
            if (mask[size_t(y) * rw + x]) {
                if (l < 0) l = x;
                r = x;
            }
        if (l < 0) continue;
        for (int x = l; x <= r; ++x) {
            sx += x;
            sy += y;
            ++filled;
        }
        minX = std::min(minX, l);
        maxX = std::max(maxX, r);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }
    if (filled < 12 || maxX < 0) return out;
    const float bw = float(maxX - minX + 1), bh = float(maxY - minY + 1);
    const float rEst = 0.5f * std::max(bw, bh);
    const float circularity = float(filled) / (3.14159265f * rEst * rEst);
    if (rEst > maxR * 0.95f) return out;   // разлилось на ресницы/тень
    out.center = glm::vec2(float(sx / filled + x0 + 0.5) / float(width), float(sy / filled + y0 + 0.5) / float(height));
    out.radius = rEst / float(width);
    out.confidence = std::clamp(circularity, 0.0f, 1.0f) * std::clamp((bright - darkest) / 60.0f, 0.0f, 1.0f);
    out.found = out.confidence > 0.3f && bw / bh > 0.4f && bw / bh < 2.5f;
    (void)area;
    return out;
}

EyeTracker::Pupil EyeTracker::addImage(int eye, int64_t tNs, const uint8_t* gray, int width, int height, int stride)
{
    const Pupil p = detectPupil(gray, width, height, stride);
    eye = eye ? 1 : 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pupil[eye] = p;
    if (!p.found) return p;
    m_pupilT[eye] = tNs;
    // Сглаживание: фиксации — плавно, саккады (быстрые скачки) — сразу
    if (!m_haveSmoothed[eye] || glm::length(p.center - m_smoothed[eye]) > 0.03f) m_smoothed[eye] = p.center;
    else m_smoothed[eye] = glm::mix(m_smoothed[eye], p.center, 0.5f);
    m_haveSmoothed[eye] = true;
    return p;
}

void EyeTracker::basis(const glm::vec2& p, float out[6])
{
    const float x = p.x - 0.5f, y = p.y - 0.5f;
    out[0] = 1.0f;
    out[1] = x;
    out[2] = y;
    out[3] = x * y;
    out[4] = x * x;
    out[5] = y * y;
}

glm::vec2 EyeTracker::mapToAngles(int eye, const glm::vec2& pupil) const
{
    if (!m_model[eye]) {
        // Без калибровки: центр кадра — прямо, края кадра — ±30° (камера смотрит на глаз: зеркально)
        return glm::vec2(-(pupil.x - 0.5f) * 60.0f, -(pupil.y - 0.5f) * 60.0f);
    }
    float b[6];
    basis(pupil, b);
    glm::vec2 a(0.0f);
    for (int k = 0; k < 6; ++k) {
        a.x += m_yaw[eye][size_t(k)] * b[k];
        a.y += m_pitch[eye][size_t(k)] * b[k];
    }
    return a;
}

EyeTracker::Gaze EyeTracker::gaze() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Gaze g;
    const int64_t latest = std::max(m_pupilT[0], m_pupilT[1]);
    glm::vec2 sum(0.0f);
    int n = 0;
    float conf = 0.0f;
    for (int e = 0; e < 2; ++e) {
        if (!m_haveSmoothed[e] || latest - m_pupilT[e] > 100'000'000LL) continue;
        sum += mapToAngles(e, m_smoothed[e]);
        conf += m_pupil[e].confidence;
        ++n;
    }
    if (n == 0) return g;
    g.valid = true;
    g.tNs = latest;
    g.anglesDeg = sum / float(n);
    g.confidence = conf / float(n);
    g.calibrated = m_model[0] || m_model[1];
    const float yaw = glm::radians(g.anglesDeg.x), pitch = glm::radians(g.anglesDeg.y);
    g.direction = glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
    return g;
}

void EyeTracker::startCalibration()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_samples[0].clear();
    m_samples[1].clear();
    m_calibrating = true;
}

bool EyeTracker::addCalibrationSample(const glm::vec2& target)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    bool added = false;
    for (int e = 0; e < 2; ++e) {
        if (!m_haveSmoothed[e] || !m_pupil[e].found) continue;
        m_samples[e].push_back({m_smoothed[e], target});
        added = true;
    }
    return added;
}

bool EyeTracker::finishCalibration()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_calibrating = false;
    bool any = false;
    for (int e = 0; e < 2; ++e) {
        if (m_samples[e].size() < 6) continue;
        for (int axis = 0; axis < 2; ++axis) {
            std::array<std::array<double, 7>, 6> m{};
            for (const auto& s : m_samples[e]) {
                float b[6];
                basis(s.pupil, b);
                const double t = axis == 0 ? s.target.x : s.target.y;
                for (int r = 0; r < 6; ++r) {
                    for (int c = 0; c < 6; ++c) m[size_t(r)][size_t(c)] += double(b[r]) * b[c];
                    m[size_t(r)][6] += double(b[r]) * t;
                }
            }
            for (int r = 0; r < 6; ++r) m[size_t(r)][size_t(r)] += 1e-6;   // устойчивость
            if (!solve(m)) continue;
            auto& coeff = axis == 0 ? m_yaw[e] : m_pitch[e];
            for (int k = 0; k < 6; ++k) coeff[size_t(k)] = float(m[size_t(k)][6]);
        }
        m_model[e] = true;
        any = true;
    }
    return any;
}

bool EyeTracker::calibrated() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model[0] || m_model[1];
}

bool EyeTracker::saveCalibration(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream f(path);
    if (!f) return false;
    for (int e = 0; e < 2; ++e) {
        f << (m_model[e] ? 1 : 0);
        for (float c : m_yaw[e]) f << ' ' << c;
        for (float c : m_pitch[e]) f << ' ' << c;
        f << '\n';
    }
    return bool(f);
}

bool EyeTracker::loadCalibration(const std::string& path)
{
    std::ifstream f(path);
    if (!f) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int e = 0; e < 2; ++e) {
        int has = 0;
        f >> has;
        for (float& c : m_yaw[e]) f >> c;
        for (float& c : m_pitch[e]) f >> c;
        m_model[e] = has != 0;
    }
    return bool(f);
}

} // namespace LensEngine
