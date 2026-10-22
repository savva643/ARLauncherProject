// Метки ArUco: метка 10 см в 0.8 м от камеры под углом — находим её позу в мире.

#include "MarkerTracker.h"

#include <glm/gtc/matrix_transform.hpp>
#include <opencv2/core.hpp>
#if __has_include(<opencv2/objdetect/aruco_detector.hpp>)
#include <opencv2/objdetect/aruco_detector.hpp>
#else
#include <opencv2/aruco.hpp>
#endif

#include <cmath>
#include <cstdio>
#include <vector>

int main()
{
    if (!LensEngine::MarkerTracker::available()) {
        std::printf("OpenCV aruco not available — SKIP\n");
        return 0;
    }
    const int id = 7;
    cv::Mat marker;
#if __has_include(<opencv2/objdetect/aruco_detector.hpp>)
    cv::aruco::generateImageMarker(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50), id, 240, marker, 1);
#else
    cv::aruco::drawMarker(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50), id, 240, marker, 1);
#endif

    // Метка 10 см: центр в 0.8 м перед камерой, чуть правее, повёрнута на 25° вокруг вертикали
    const double size = 0.10;
    const glm::dvec3 center(0.05, -0.02, 0.8);
    const glm::dmat3 Rcm = glm::dmat3(glm::rotate(glm::dmat4(1.0), glm::radians(25.0), glm::dvec3(0, 1, 0))) *
                           glm::dmat3(glm::dvec3(1, 0, 0), glm::dvec3(0, -1, 0), glm::dvec3(0, 0, -1));   // y метки вверх, z к камере
    const int w = 640, h = 480;
    const double f = 520, cx = 320, cy = 240;
    std::vector<uint8_t> img(size_t(w) * h, 128);
    const glm::dvec3 n = Rcm * glm::dvec3(0, 0, 1);
    for (int v = 0; v < h; ++v)
        for (int u = 0; u < w; ++u) {
            const glm::dvec3 ray((u - cx) / f, (v - cy) / f, 1.0);
            const double t = glm::dot(center, n) / glm::dot(ray, n);
            const glm::dvec3 local = glm::transpose(Rcm) * (ray * t - center);   // оси метки
            // метка + белая рамка 20% вокруг
            const double mx = local.x / size + 0.5, my = 0.5 - local.y / size;
            if (mx < -0.2 || mx > 1.2 || my < -0.2 || my > 1.2) continue;
            uint8_t c = 255;
            if (mx >= 0 && mx < 1 && my >= 0 && my < 1) c = marker.at<uint8_t>(int(my * 240), int(mx * 240));
            img[size_t(v) * w + u] = c;
        }

    // Камера в мире: повёрнута и сдвинута
    const glm::dquat worldFromCam = glm::angleAxis(glm::radians(30.0), glm::normalize(glm::dvec3(0.2, 1, 0.1)));
    const glm::dvec3 camPos(1.0, 0.5, -2.0);
    LensEngine::MarkerTracker tracker(size);
    int found = 0;
    for (int i = 0; i < 5; ++i)
        found = tracker.process(i * 33'000'000LL, img.data(), w, h, w, float(f), float(f), float(cx), float(cy), worldFromCam, camPos);

    const auto anchors = tracker.anchors();
    const glm::dvec3 truePos = worldFromCam * center + camPos;
    const glm::dquat trueRot = glm::normalize(worldFromCam * glm::quat_cast(Rcm));
    bool ok = found == 1 && anchors.size() == 1 && anchors[0].id == id;
    if (ok) {
        const double posErr = glm::length(anchors[0].position - truePos);
        const double angErr = glm::degrees(2.0 * std::acos(std::min(1.0, std::fabs(glm::dot(anchors[0].orientation, trueRot)))));
        std::printf("marker %d: position error %.1f mm, orientation error %.2f deg, %d detections\n", anchors[0].id,
                    posErr * 1000, angErr, anchors[0].detections);
        ok = posErr < 0.01 && angErr < 3.0;
    } else {
        std::printf("marker not found (found=%d, anchors=%zu)\n", found, anchors.size());
    }
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
