#include "MarkerTracker.h"

#include <cmath>

#ifdef LENSENGINE_USE_OPENCV
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#if __has_include(<opencv2/objdetect/aruco_detector.hpp>)
#include <opencv2/objdetect/aruco_detector.hpp>   // OpenCV 4.7+
#define LENSENGINE_ARUCO_NEW 1
#elif __has_include(<opencv2/aruco.hpp>)
#include <opencv2/aruco.hpp>                      // OpenCV 4.6 и старше: модуль contrib
#define LENSENGINE_ARUCO_OLD 1
#endif
#endif

namespace LensEngine {

bool MarkerTracker::available()
{
#if defined(LENSENGINE_ARUCO_NEW) || defined(LENSENGINE_ARUCO_OLD)
    return true;
#else
    return false;
#endif
}

MarkerTracker::MarkerTracker(double markerSizeMeters) : m_markerSize(markerSizeMeters) {}

int MarkerTracker::process(int64_t tNs, const uint8_t* y, int width, int height, int stride,
                           float fx, float fy, float cx, float cy,
                           const glm::dquat& worldFromCam, const glm::dvec3& camPosition)
{
#if defined(LENSENGINE_ARUCO_NEW) || defined(LENSENGINE_ARUCO_OLD)
    if (!y || width <= 0 || height <= 0) return 0;
    const cv::Mat gray(height, width, CV_8UC1, const_cast<uint8_t*>(y), size_t(stride));
    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<int> ids;
#ifdef LENSENGINE_ARUCO_NEW
    static const cv::aruco::ArucoDetector detector(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50),
                                                   cv::aruco::DetectorParameters());
    detector.detectMarkers(gray, corners, ids);
#else
    static const cv::Ptr<cv::aruco::Dictionary> dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::detectMarkers(gray, dict, corners, ids);
#endif
    if (ids.empty()) return 0;

    const double s = m_markerSize * 0.5;
    // Углы метки в её осях (порядок OpenCV: левый верхний, правый верхний, правый нижний, левый нижний)
    const std::vector<cv::Point3f> object = {
        {float(-s), float(s), 0.0f}, {float(s), float(s), 0.0f}, {float(s), float(-s), 0.0f}, {float(-s), float(-s), 0.0f}};
    const cv::Matx33d K(fx, 0, cx, 0, fy, cy, 0, 0, 1);
    const glm::dmat3 Rwc = glm::mat3_cast(worldFromCam);

    std::lock_guard<std::mutex> lock(m_mutex);
    int found = 0;
    for (size_t i = 0; i < ids.size(); ++i) {
        cv::Vec3d rvec, tvec;
        if (!cv::solvePnP(object, corners[i], K, cv::noArray(), rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE)) continue;
        if (tvec[2] <= 0.05 || tvec[2] > 5.0) continue;
        cv::Matx33d Rcm;
        cv::Rodrigues(rvec, Rcm);
        glm::dmat3 rcm;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) rcm[c][r] = Rcm(r, c);
        const glm::dquat qWorld = glm::normalize(glm::quat_cast(Rwc * rcm));
        const glm::dvec3 pWorld = Rwc * glm::dvec3(tvec[0], tvec[1], tvec[2]) + camPosition;
        ++found;

        Anchor* a = nullptr;
        for (auto& e : m_anchors) {
            if (e.id == ids[i]) a = &e;
        }
        if (!a) {
            m_anchors.push_back(Anchor{});
            a = &m_anchors.back();
            a->id = ids[i];
        }
        // Метка на месте — сглаживаем (дрожание углов в пикселях); переставили — берём новую позу сразу
        if (a->detections == 0 || glm::length(a->position - pWorld) > 0.3) {
            a->position = pWorld;
            a->orientation = qWorld;
            a->detections = 0;
        } else {
            a->position += 0.3 * (pWorld - a->position);
            a->orientation = glm::normalize(glm::slerp(a->orientation, qWorld, 0.3));
        }
        a->lastSeenNs = tNs;
        ++a->detections;
    }
    return found;
#else
    (void)tNs; (void)y; (void)width; (void)height; (void)stride; (void)fx; (void)fy; (void)cx; (void)cy;
    (void)worldFromCam; (void)camPosition;
    return 0;
#endif
}

std::vector<MarkerTracker::Anchor> MarkerTracker::anchors() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_anchors;
}

} // namespace LensEngine
