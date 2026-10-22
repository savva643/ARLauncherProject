#include "EnvironmentProbe.h"

#include <algorithm>
#include <cmath>

namespace LensEngine {

namespace {

constexpr float kPi = 3.14159265358979323846f;

inline float srgbToLinear(float c) { return std::pow(std::clamp(c, 0.0f, 1.0f), 2.2f); }
inline float linearToSrgb(float c) { return std::pow(std::clamp(c, 0.0f, 1.0f), 1.0f / 2.2f); }
inline float luminance(const glm::vec3& c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }

void shBasis(const glm::vec3& d, float Y[9])
{
    Y[0] = 0.282095f;
    Y[1] = 0.488603f * d.y;
    Y[2] = 0.488603f * d.z;
    Y[3] = 0.488603f * d.x;
    Y[4] = 1.092548f * d.x * d.y;
    Y[5] = 1.092548f * d.y * d.z;
    Y[6] = 0.315392f * (3.0f * d.z * d.z - 1.0f);
    Y[7] = 1.092548f * d.x * d.z;
    Y[8] = 0.546274f * (d.x * d.x - d.y * d.y);
}

} // namespace

EnvironmentProbe::EnvironmentProbe()
{
    const size_t n = size_t(kWidth) * kHeight;
    m_dirs.resize(n);
    m_solidAngle.resize(n);
    for (int v = 0; v < kHeight; ++v) {
        const float theta = (v + 0.5f) / kHeight * kPi;
        const float sa = (2.0f * kPi / kWidth) * (kPi / kHeight) * std::sin(theta);
        for (int u = 0; u < kWidth; ++u) {
            m_dirs[size_t(v) * kWidth + u] = texelDirection((u + 0.5f) / kWidth, (v + 0.5f) / kHeight);
            m_solidAngle[size_t(v) * kWidth + u] = sa;
        }
    }
    reset();
}

glm::vec3 EnvironmentProbe::texelDirection(float u, float v)
{
    const float phi = (u - 0.5f) * 2.0f * kPi;
    const float theta = v * kPi;
    const float s = std::sin(theta);
    return glm::vec3(s * std::sin(phi), std::cos(theta), -s * std::cos(phi));
}

void EnvironmentProbe::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const size_t n = size_t(kWidth) * kHeight;
    m_radiance.assign(n, glm::vec3(0.0f));
    m_weight.assign(n, 0.0f);
    m_texture.assign(n * 4, 0);
    m_lighting = Lighting{};
    m_version = 0;
}

void EnvironmentProbe::addFrame(const uint8_t* y, int yStride, const uint8_t* uv, int uvStride, int width, int height,
                                float fx, float fy, float cx, float cy, const glm::quat& worldFromCamCV,
                                bool fullRange, bool bt709)
{
    if (!y || !uv || width < 8 || height < 8 || fx <= 0.0f || fy <= 0.0f) return;

    const glm::mat3 camFromWorld = glm::transpose(glm::mat3_cast(worldFromCamCV));
    const float margin = 2.0f;

    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_dirs.size(); ++i) {
        const glm::vec3 c = camFromWorld * m_dirs[i];
        if (c.z < 0.1f) continue;
        const float px = fx * c.x / c.z + cx;
        const float py = fy * c.y / c.z + cy;
        if (px < margin || py < margin || px > width - 1 - margin || py > height - 1 - margin) continue;

        // 3x3 отсчёта на тексель — карта мелкая (128x64), один пиксель видео даёт шум
        glm::vec3 sum(0.0f);
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int sx = std::clamp(int(px) + dx * 2, 0, width - 1);
                const int sy = std::clamp(int(py) + dy * 2, 0, height - 1);
                float Y = y[size_t(sy) * yStride + sx] / 255.0f;
                const uint8_t* p = uv + size_t(sy / 2) * uvStride + size_t(sx / 2) * 2;
                float cb = p[0] / 255.0f - 0.5f;
                float cr = p[1] / 255.0f - 0.5f;
                if (!fullRange) {
                    Y = (Y - 16.0f / 255.0f) * (255.0f / 219.0f);
                    cb *= 255.0f / 224.0f;
                    cr *= 255.0f / 224.0f;
                }
                glm::vec3 rgb = bt709 ? glm::vec3(Y + 1.5748f * cr, Y - 0.1873f * cb - 0.4681f * cr, Y + 1.8556f * cb)
                                      : glm::vec3(Y + 1.402f * cr, Y - 0.344136f * cb - 0.714136f * cr, Y + 1.772f * cb);
                sum += glm::vec3(srgbToLinear(rgb.r), srgbToLinear(rgb.g), srgbToLinear(rgb.b));
            }
        }
        const glm::vec3 sample = sum / 9.0f;
        // Свежие кадры важнее старых (свет в комнате меняется), но один кадр карту не перекрашивает
        const float alpha = m_weight[i] > 0.0f ? 0.35f : 1.0f;
        m_radiance[i] = glm::mix(m_radiance[i], sample, alpha);
        m_weight[i] = std::min(1.0f, m_weight[i] + 0.25f);
    }
    rebuildLocked();
    ++m_version;
}

void EnvironmentProbe::rebuildLocked()
{
    // 1) Средний цвет каждой широты по увиденному — им заполняем то, что камера ещё не видела
    std::vector<glm::vec3> rowMean(kHeight, glm::vec3(0.0f));
    std::vector<bool> rowHas(kHeight, false);
    glm::vec3 total(0.0f);
    float totalW = 0.0f, seenSolid = 0.0f, allSolid = 0.0f;
    for (int v = 0; v < kHeight; ++v) {
        glm::vec3 s(0.0f);
        float w = 0.0f;
        for (int u = 0; u < kWidth; ++u) {
            const size_t i = size_t(v) * kWidth + u;
            allSolid += m_solidAngle[i];
            if (m_weight[i] <= 0.0f) continue;
            s += m_radiance[i];
            w += 1.0f;
            total += m_radiance[i] * m_solidAngle[i];
            totalW += m_solidAngle[i];
            seenSolid += m_solidAngle[i];
        }
        if (w > 0.0f) {
            rowMean[v] = s / w;
            rowHas[v] = true;
        }
    }
    if (totalW <= 0.0f) return;
    const glm::vec3 mean = total / totalW;
    for (int v = 0; v < kHeight; ++v) {
        if (rowHas[v]) continue;
        // ближайшая широта с данными; над головой и под ногами без данных — чуть темнее среднего
        int best = -1;
        for (int d = 1; d < kHeight && best < 0; ++d) {
            if (v - d >= 0 && rowHas[v - d]) best = v - d;
            else if (v + d < kHeight && rowHas[v + d]) best = v + d;
        }
        rowMean[v] = best >= 0 ? glm::mix(rowMean[best], mean, 0.5f) : mean;
    }

    // 2) Итоговая карта + SH + самый яркий источник
    Lighting L;
    L.valid = true;
    L.coverage = allSolid > 0.0f ? seenSolid / allSolid : 0.0f;
    L.ambient = mean;
    std::array<glm::vec3, 9> sh{};
    float maxLum = 0.0f;
    for (size_t i = 0; i < m_dirs.size(); ++i) {
        const int v = int(i / kWidth);
        const glm::vec3 c = m_weight[i] > 0.0f ? glm::mix(rowMean[v], m_radiance[i], m_weight[i]) : rowMean[v];
        m_texture[i * 4 + 0] = uint8_t(std::lround(linearToSrgb(c.r) * 255.0f));
        m_texture[i * 4 + 1] = uint8_t(std::lround(linearToSrgb(c.g) * 255.0f));
        m_texture[i * 4 + 2] = uint8_t(std::lround(linearToSrgb(c.b) * 255.0f));
        m_texture[i * 4 + 3] = uint8_t(std::lround(std::clamp(m_weight[i], 0.0f, 1.0f) * 255.0f));
        float Y[9];
        shBasis(m_dirs[i], Y);
        for (int k = 0; k < 9; ++k) sh[k] += c * (Y[k] * m_solidAngle[i]);
        if (m_weight[i] > 0.0f) maxLum = std::max(maxLum, luminance(m_radiance[i]));
    }
    L.sh = sh;

    // Источник: самые яркие увиденные тексели (лампа, окно). Направление — центр масс по яркости.
    glm::vec3 dirSum(0.0f), colSum(0.0f);
    float wSum = 0.0f;
    const float threshold = maxLum * 0.75f;
    for (size_t i = 0; i < m_dirs.size(); ++i) {
        if (m_weight[i] <= 0.0f) continue;
        const float lum = luminance(m_radiance[i]);
        if (lum < threshold) continue;
        const float w = (lum - threshold + 1e-4f) * m_solidAngle[i];
        dirSum += m_dirs[i] * w;
        colSum += m_radiance[i] * m_solidAngle[i];
        wSum += m_solidAngle[i];
    }
    const float meanLum = luminance(mean);
    if (wSum > 0.0f && glm::length(dirSum) > 1e-6f && maxLum > meanLum * 1.3f) {
        glm::vec3 dir = glm::normalize(dirSum);
        // Свет снизу почти не бывает (это светлый пол) — тени тогда падали бы вверх
        if (dir.y < 0.25f) {
            dir.y = 0.25f;
            dir = glm::normalize(dir);
        }
        L.lightDir = dir;
        L.lightColor = glm::max(colSum / wSum - mean, glm::vec3(0.0f));
    } else {
        L.lightDir = glm::normalize(glm::vec3(0.3f, 0.9f, 0.3f));
        L.lightColor = mean * 0.5f;    // рассеянный свет: мягкая тень сверху
    }
    m_lighting = L;
}

EnvironmentProbe::Lighting EnvironmentProbe::lighting() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lighting;
}

uint64_t EnvironmentProbe::copyTexture(std::vector<uint8_t>& rgba) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_version == 0) return 0;
    rgba = m_texture;
    return m_version;
}

uint64_t EnvironmentProbe::version() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_version;
}

} // namespace LensEngine
