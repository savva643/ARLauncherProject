// Айтрекинг: синтетические ИК-кадры глаз (веки, радужка, зрачок, блики диодов, шум).
// Калибровка по 9 точкам, затем случайные взгляды — ошибка направления.

#include "EyeTracker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace {

// Глаз смотрит на (yaw, pitch); камера снизу-спереди. Возвращает GRAY8 width x height.
std::vector<uint8_t> renderEye(int w, int h, float yawDeg, float pitchDeg, int eye, std::mt19937& rng)
{
    std::normal_distribution<float> noise(0.0f, 4.0f);
    std::vector<uint8_t> img(size_t(w) * h);
    const float R = 0.33f * w;                                  // радиус глазного яблока в пикселях
    const float yaw = yawDeg * 3.14159265f / 180.0f, pitch = pitchDeg * 3.14159265f / 180.0f;
    const float ex = 0.5f * w + (eye ? 3.0f : -3.0f);           // глаза чуть по-разному сидят в кадре
    const float ey = 0.52f * h;
    const float px = ex - R * std::sin(yaw) * std::cos(pitch);  // камера смотрит на глаз — зеркально
    const float py = ey - R * std::sin(pitch) * 0.9f;
    const float iris = 0.12f * w, pupil = 0.048f * w;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const float dx = (x - ex) / (0.42f * w), dy = (y - ey) / (0.30f * h);
            float v = (dx * dx + dy * dy < 1.0f) ? 205.0f : 125.0f + 10.0f * std::sin(x * 0.3f) * std::sin(y * 0.2f);
            const float r = std::hypot(x - px, (y - py) * 1.05f);
            if (dx * dx + dy * dy < 1.0f && r < iris) v = 85.0f + 10.0f * std::sin(std::atan2(y - py, x - px) * 12.0f);
            if (r < pupil) v = 18.0f;
            // блики ИК-диодов на роговице
            for (float gx : {-0.03f * w, 0.03f * w})
                if (std::hypot(x - (ex + gx + (px - ex) * 0.5f), y - (ey + 0.02f * h + (py - ey) * 0.5f)) < 2.2f) v = 250.0f;
            img[size_t(y) * w + x] = uint8_t(std::clamp(v + noise(rng), 0.0f, 255.0f));
        }
    return img;
}

} // namespace

int main()
{
    const int w = 200, h = 150;
    std::mt19937 rng(9);
    LensEngine::EyeTracker tracker;

    // Детектор: зрачок найден там, где нарисован (центральный взгляд)
    const auto img = renderEye(w, h, 0, 0, 0, rng);
    const auto p = LensEngine::EyeTracker::detectPupil(img.data(), w, h, w);
    std::printf("pupil: found %d at (%.3f, %.3f), radius %.3f, confidence %.2f\n", p.found, p.center.x, p.center.y,
                p.radius, p.confidence);

    // Калибровка по 9 точкам (±20°)
    tracker.startCalibration();
    int64_t t = 0;
    for (float pitch : {-20.0f, 0.0f, 20.0f})
        for (float yaw : {-20.0f, 0.0f, 20.0f})
            for (int k = 0; k < 5; ++k) {
                for (int eye = 0; eye < 2; ++eye) {
                    const auto im = renderEye(w, h, yaw, pitch, eye, rng);
                    tracker.addImage(eye, t, im.data(), w, h, w);
                }
                t += 10'000'000;
                if (k >= 2) tracker.addCalibrationSample({yaw, pitch});
            }
    const bool calibrated = tracker.finishCalibration();

    // Проверка на случайных взглядах
    std::uniform_real_distribution<float> angle(-18.0f, 18.0f);
    double maxErr = 0.0, sumErr = 0.0;
    const int trials = 40;
    for (int i = 0; i < trials; ++i) {
        const float yaw = angle(rng), pitch = angle(rng);
        for (int k = 0; k < 2; ++k) {   // два кадра: саккада, потом фиксация
            for (int eye = 0; eye < 2; ++eye) {
                const auto im = renderEye(w, h, yaw, pitch, eye, rng);
                tracker.addImage(eye, t, im.data(), w, h, w);
            }
            t += 10'000'000;
        }
        const auto g = tracker.gaze();
        const float ty = glm::radians(yaw), tp = glm::radians(pitch);
        const glm::vec3 truth(std::sin(ty) * std::cos(tp), std::sin(tp), -std::cos(ty) * std::cos(tp));
        const double err = glm::degrees(std::acos(std::clamp(double(glm::dot(g.direction, truth)), -1.0, 1.0)));
        maxErr = std::max(maxErr, err);
        sumErr += err;
    }
    std::printf("calibration %s; gaze error mean %.2f deg, max %.2f deg (%d random gazes within ±18°)\n",
                calibrated ? "ok" : "FAILED", sumErr / trials, maxErr, trials);
    const bool ok = p.found && std::fabs(p.center.x - 0.485f) < 0.02f && calibrated && sumErr / trials < 0.8 && maxErr < 2.0;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
