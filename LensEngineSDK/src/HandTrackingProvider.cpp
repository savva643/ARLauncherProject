#include "HandTrackingProvider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#ifdef LENSENGINE_USE_OPENCV
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace LensEngine {

namespace {

bool fileExists(const std::string& p)
{
    std::ifstream f(p);
    return f.good();
}

} // namespace

std::string HandTrackingProvider::findModelDir()
{
    std::vector<std::string> dirs;
    if (const char* env = std::getenv("ARL_MODELS")) dirs.push_back(env);
    dirs.insert(dirs.end(), {"models", "../models", "../../models", "../../../models"});
#ifdef LENSENGINE_SOURCE_DIR
    dirs.push_back(std::string(LENSENGINE_SOURCE_DIR) + "/../models");
#endif
    for (const auto& d : dirs) {
        if (fileExists(d + "/palm_detection_mediapipe_2023feb.onnx")) return d;
    }
    return {};
}

#ifdef LENSENGINE_USE_OPENCV

struct HandTrackingProvider::Impl {
    cv::dnn::Net palm;
    cv::dnn::Net pose;
    std::vector<std::string> palmOut, poseOut;
    std::vector<cv::Point2f> anchors;
    // Сглаживание и гистерезис щипка по каждой руке (0 — левая, 1 — правая)
    bool havePrev[2] = {false, false};
    std::array<glm::vec3, kJoints> prevWorld[2];
    bool pinchState[2] = {false, false};

    static constexpr int kPalmSize = 192;
    static constexpr int kPoseSize = 224;

    Impl()
    {
        // Якоря SSD детектора ладоней MediaPipe: шаг 8 (24x24, 2 якоря) + шаг 16 (12x12, 6 якорей) = 2016
        const int strides[2] = {8, 16};
        const int perCell[2] = {2, 6};
        for (int l = 0; l < 2; ++l) {
            const int fm = kPalmSize / strides[l];
            for (int y = 0; y < fm; ++y)
                for (int x = 0; x < fm; ++x)
                    for (int a = 0; a < perCell[l]; ++a)
                        anchors.emplace_back((x + 0.5f) / fm, (y + 0.5f) / fm);
        }
    }

    static cv::dnn::Net load(const std::string& dir, const std::string& name)
    {
        for (const std::string& file : {dir + "/" + name + ".onnx", dir + "/" + name + "_compat.onnx"}) {
            if (!fileExists(file)) continue;
            try {
                cv::dnn::Net net = cv::dnn::readNetFromONNX(file);
                if (!net.empty()) {
                    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
                    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
                    std::cout << "[Hands] model " << file << std::endl;
                    return net;
                }
            } catch (const cv::Exception&) {
                std::cout << "[Hands] " << file << " needs newer OpenCV, trying compat model" << std::endl;
            }
        }
        return {};
    }

    struct Palm {
        cv::Point2f tl, br;
        std::array<cv::Point2f, 7> kp;
        float score;
    };

    std::vector<Palm> detectPalms(const cv::Mat& bgr)
    {
        // Вписываем в 192x192 с полями (пропорции сохраняются)
        const float ratio = std::min(float(kPalmSize) / bgr.rows, float(kPalmSize) / bgr.cols);
        const cv::Size rs(int(bgr.cols * ratio), int(bgr.rows * ratio));
        cv::Mat resized, padded, rgb, input;
        cv::resize(bgr, resized, rs);
        const int padW = kPalmSize - rs.width, padH = kPalmSize - rs.height;
        cv::copyMakeBorder(resized, padded, padH / 2, padH - padH / 2, padW / 2, padW - padW / 2, cv::BORDER_CONSTANT,
                           cv::Scalar(0, 0, 0));
        cv::cvtColor(padded, rgb, cv::COLOR_BGR2RGB);
        rgb.convertTo(input, CV_32FC3, 1.0 / 255.0);
        const int shape[4] = {1, kPalmSize, kPalmSize, 3};
        cv::Mat blob(4, shape, CV_32F, input.ptr<float>());   // NHWC
        palm.setInput(blob);
        std::vector<cv::Mat> outs;
        palm.forward(outs, palmOut);
        const cv::Mat* reg = nullptr;
        const cv::Mat* cls = nullptr;
        for (const auto& o : outs) {
            if (o.dims == 3 && o.size[2] == 18) reg = &o;
            if (o.dims == 3 && o.size[2] == 1) cls = &o;
        }
        std::vector<Palm> result;
        if (!reg || !cls) return result;

        const float scale = float(std::max(bgr.rows, bgr.cols));
        const cv::Point2f padBias(float(padW / 2) / ratio, float(padH / 2) / ratio);
        std::vector<cv::Rect2d> boxes;
        std::vector<float> scores;
        std::vector<Palm> candidates;
        const float* r = reg->ptr<float>();
        const float* c = cls->ptr<float>();
        for (int i = 0; i < int(anchors.size()); ++i) {
            const float score = 1.0f / (1.0f + std::exp(-c[i]));
            if (std::getenv("HANDS_DEBUG") && score > 0.3f) std::printf("palm cand %d score %.2f\n", i, score);
            if (score < 0.6f) continue;
            const float* d = r + i * 18;
            const cv::Point2f ctr(d[0] / kPalmSize + anchors[i].x, d[1] / kPalmSize + anchors[i].y);
            const cv::Point2f half(d[2] / kPalmSize * 0.5f, d[3] / kPalmSize * 0.5f);
            Palm p;
            p.tl = (ctr - half) * scale - padBias;
            p.br = (ctr + half) * scale - padBias;
            for (int k = 0; k < 7; ++k)
                p.kp[size_t(k)] = cv::Point2f(d[4 + 2 * k] / kPalmSize + anchors[i].x,
                                              d[5 + 2 * k] / kPalmSize + anchors[i].y) * scale - padBias;
            p.score = score;
            candidates.push_back(p);
            boxes.emplace_back(p.tl.x, p.tl.y, p.br.x - p.tl.x, p.br.y - p.tl.y);
            scores.push_back(score);
        }
        std::vector<int> keep;
        cv::dnn::NMSBoxes(boxes, scores, 0.6f, 0.3f, keep, 1.0f, 2);
        for (int k : keep) result.push_back(candidates[size_t(k)]);
        return result;
    }

    // Вырезка вокруг ладони: сдвиг, увеличение, квадрат с полями (как в MediaPipe / opencv_zoo)
    static cv::Mat cropAndPad(const cv::Mat& image, cv::Point2f& tl, cv::Point2f& br, bool forRotation, cv::Point2f& bias)
    {
        const cv::Point2f wh = br - tl;
        const cv::Point2f shift = forRotation ? cv::Point2f(0, 0) : cv::Point2f(0, -0.4f * wh.y);
        tl += shift;
        br += shift;
        const cv::Point2f center = (tl + br) * 0.5f;
        const float enlarge = forRotation ? 4.0f : 3.0f;
        const cv::Point2f half = (br - tl) * (enlarge * 0.5f);
        cv::Point2i a(int(center.x - half.x), int(center.y - half.y));
        cv::Point2i b(int(center.x + half.x), int(center.y + half.y));
        a.x = std::clamp(a.x, 0, image.cols);
        a.y = std::clamp(a.y, 0, image.rows);
        b.x = std::clamp(b.x, 0, image.cols);
        b.y = std::clamp(b.y, 0, image.rows);
        tl = cv::Point2f(float(a.x), float(a.y));
        br = cv::Point2f(float(b.x), float(b.y));
        if (b.x - a.x < 2 || b.y - a.y < 2) return {};
        cv::Mat crop = image(cv::Rect(a, b));
        const int side = forRotation ? int(std::hypot(crop.rows, crop.cols)) : std::max(crop.rows, crop.cols);
        const int padH = side - crop.rows, padW = side - crop.cols;
        cv::Mat padded;
        cv::copyMakeBorder(crop, padded, padH / 2, padH - padH / 2, padW / 2, padW - padW / 2, cv::BORDER_CONSTANT,
                           cv::Scalar(0, 0, 0));
        bias = cv::Point2f(float(a.x - padW / 2), float(a.y - padH / 2));
        return padded;
    }

    bool estimate(const cv::Mat& bgr, const Palm& palmIn, Hand& out)
    {
        // 1) Широкая вырезка вокруг ладони, поворот так, чтобы рука смотрела вверх
        cv::Point2f tl = palmIn.tl, br = palmIn.br, padBias;
        cv::Mat image = cropAndPad(bgr, tl, br, true, padBias);
        if (image.empty()) return false;
        tl -= padBias;
        br -= padBias;
        std::array<cv::Point2f, 7> kp;
        for (int k = 0; k < 7; ++k) kp[size_t(k)] = palmIn.kp[size_t(k)] - padBias;
        const cv::Point2f p1 = kp[0], p2 = kp[2];   // основание ладони -> основание среднего пальца
        double radians = M_PI / 2 - std::atan2(-(p2.y - p1.y), p2.x - p1.x);
        radians -= 2 * M_PI * std::floor((radians + M_PI) / (2 * M_PI));
        const double angle = radians * 180.0 / M_PI;
        const cv::Point2f center = (tl + br) * 0.5f;
        const cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
        cv::Mat rotated;
        cv::warpAffine(image, rotated, rot, image.size());
        cv::Point2f lo(1e9f, 1e9f), hi(-1e9f, -1e9f);
        for (const auto& p : kp) {
            const float x = float(rot.at<double>(0, 0) * p.x + rot.at<double>(0, 1) * p.y + rot.at<double>(0, 2));
            const float y = float(rot.at<double>(1, 0) * p.x + rot.at<double>(1, 1) * p.y + rot.at<double>(1, 2));
            lo = cv::Point2f(std::min(lo.x, x), std::min(lo.y, y));
            hi = cv::Point2f(std::max(hi.x, x), std::max(hi.y, y));
        }
        // 2) Вырезка руки из повёрнутого кадра -> 224x224
        cv::Point2f unused;
        cv::Mat crop = cropAndPad(rotated, lo, hi, false, unused);
        if (crop.empty()) return false;
        cv::Mat resized, rgb, input;
        cv::resize(crop, resized, cv::Size(kPoseSize, kPoseSize), 0, 0, cv::INTER_AREA);
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
        rgb.convertTo(input, CV_32FC3, 1.0 / 255.0);
        const int shape[4] = {1, kPoseSize, kPoseSize, 3};
        cv::Mat blob(4, shape, CV_32F, input.ptr<float>());
        pose.setInput(blob);
        std::vector<cv::Mat> outs;
        pose.forward(outs, poseOut);
        // Выходы: суставы (пиксели 224), уверенность, рука (правая/левая), суставы в метрах
        const cv::Mat *lm = nullptr, *world = nullptr;
        std::vector<const cv::Mat*> scalars;
        for (const auto& o : outs) {
            if (o.total() == 63) {
                if (!lm) lm = &o;
                else world = &o;
            } else if (o.total() == 1) {
                scalars.push_back(&o);
            }
        }
        if (!lm || !world || scalars.size() < 2) return false;
        // Пиксели — значения ~0..224, метры — ~0.1: различаем по величине
        if (std::fabs(lm->ptr<float>()[0]) < std::fabs(world->ptr<float>()[0])) std::swap(lm, world);
        const float conf = scalars[0]->ptr<float>()[0];
        const float handedness = scalars[1]->ptr<float>()[0];
        if (conf < 0.5f) return false;

        // 3) Обратно в координаты исходного кадра
        const cv::Point2f whRot = hi - lo;
        const float s = std::max(whRot.x, whRot.y) / kPoseSize;
        const cv::Mat crot = cv::getRotationMatrix2D(cv::Point2f(0, 0), angle, 1.0);
        // обратное аффинное преобразование поворота кадра
        cv::Mat inv;
        cv::invertAffineTransform(rot, inv);
        const cv::Point2f rotCenter = (lo + hi) * 0.5f;
        const cv::Point2f origCenter(
            float(inv.at<double>(0, 0) * rotCenter.x + inv.at<double>(0, 1) * rotCenter.y + inv.at<double>(0, 2)),
            float(inv.at<double>(1, 0) * rotCenter.x + inv.at<double>(1, 1) * rotCenter.y + inv.at<double>(1, 2)));
        const float* L = lm->ptr<float>();
        const float* W = world->ptr<float>();
        for (int j = 0; j < kJoints; ++j) {
            const float x = (L[3 * j] - kPoseSize * 0.5f) * s;
            const float y = (L[3 * j + 1] - kPoseSize * 0.5f) * s;
            // поворот обратно: [x y] * R(angle)[:2,:2]
            const float rx = float(x * crot.at<double>(0, 0) + y * crot.at<double>(1, 0));
            const float ry = float(x * crot.at<double>(0, 1) + y * crot.at<double>(1, 1));
            out.image[size_t(j)] = glm::vec2(rx + origCenter.x + padBias.x, ry + origCenter.y + padBias.y);
            const float wx = W[3 * j], wy = W[3 * j + 1];
            out.local[size_t(j)] = glm::vec3(float(wx * crot.at<double>(0, 0) + wy * crot.at<double>(1, 0)),
                                             float(wx * crot.at<double>(0, 1) + wy * crot.at<double>(1, 1)), W[3 * j + 2]);
        }
        out.confidence = conf;
        // Модель считает кадр зеркальным (селфи); задняя камера/камера шлема не зеркалит — меняем сторону
        out.right = handedness < 0.5f;
        return true;
    }
};

bool HandTrackingProvider::available()
{
    return true;
}

HandTrackingProvider::HandTrackingProvider() : m_impl(std::make_unique<Impl>()) {}
HandTrackingProvider::~HandTrackingProvider() = default;

bool HandTrackingProvider::initialize(const std::string& modelDir)
{
    const std::string dir = modelDir.empty() ? findModelDir() : modelDir;
    if (dir.empty()) {
        std::cout << "[Hands] models not found (set ARL_MODELS=<dir with palm_detection/handpose onnx>)" << std::endl;
        return false;
    }
    m_impl->palm = Impl::load(dir, "palm_detection_mediapipe_2023feb");
    m_impl->pose = Impl::load(dir, "handpose_estimation_mediapipe_2023feb");
    if (m_impl->palm.empty() || m_impl->pose.empty()) {
        std::cout << "[Hands] cannot load models from " << dir
                  << " (OpenCV < 4.7: python3 tools/onnx_clip_compat.py models/handpose_estimation_mediapipe_2023feb.onnx)"
                  << std::endl;
        return false;
    }
    m_impl->palmOut = m_impl->palm.getUnconnectedOutLayersNames();
    m_impl->poseOut = m_impl->pose.getUnconnectedOutLayersNames();
    m_ready = true;
    return true;
}

std::vector<HandTrackingProvider::Hand> HandTrackingProvider::process(int64_t tNs, const uint8_t* y, int yStride,
                                                                     const uint8_t* uv, int uvStride, int width,
                                                                     int height, const Camera& camera, const Depth* depth)
{
    if (!m_ready || !y || width < 16 || height < 16) return {};
    // Уменьшаем до ~640 по ширине (детектору хватает, а 4K в DNN — долго), NV12 -> BGR
    const int step = std::max(1, width / 640);
    const int w = (width / step) & ~1, h = (height / step) & ~1;
    cv::Mat nv12(h * 3 / 2, w, CV_8UC1);
    for (int r = 0; r < h; ++r) {
        const uint8_t* src = y + size_t(r * step) * yStride;
        uint8_t* dst = nv12.ptr<uint8_t>(r);
        for (int c = 0; c < w; ++c) dst[c] = src[c * step];
    }
    for (int r = 0; r < h / 2; ++r) {
        uint8_t* dst = nv12.ptr<uint8_t>(h + r);
        if (!uv) {
            std::fill(dst, dst + w, uint8_t(128));
            continue;
        }
        const uint8_t* src = uv + size_t(r * step) * uvStride;
        for (int c = 0; c < w / 2; ++c) {
            dst[2 * c] = src[2 * c * step];
            dst[2 * c + 1] = src[2 * c * step + 1];
        }
    }
    cv::Mat bgr;
    cv::cvtColor(nv12, bgr, cv::COLOR_YUV2BGR_NV12);
    Camera cam = camera;
    cam.fx /= step;
    cam.fy /= step;
    cam.cx /= step;
    cam.cy /= step;
    auto hands = processBGR(tNs, bgr.data, bgr.cols, bgr.rows, int(bgr.step), cam, depth);
    for (auto& hd : hands)
        for (auto& p : hd.image) p *= float(step);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hands = hands;
    return hands;
}

std::vector<HandTrackingProvider::Hand> HandTrackingProvider::processBGR(int64_t tNs, const uint8_t* bgrData, int width,
                                                                        int height, int stride, const Camera& cam,
                                                                        const Depth* depth)
{
    std::vector<Hand> result;
    if (!m_ready) return result;
    const cv::Mat bgr(height, width, CV_8UC3, const_cast<uint8_t*>(bgrData), size_t(stride));
    std::vector<Impl::Palm> palms;
    try {
        palms = m_impl->detectPalms(bgr);
    } catch (const cv::Exception& e) {
        std::cerr << "[Hands] palm detection failed: " << e.what() << std::endl;
        return result;
    }
    const glm::mat3 R = glm::mat3_cast(cam.worldFromCam);
    bool seen[2] = {false, false};
    for (const auto& palm : palms) {
        Hand hand;
        hand.tNs = tNs;
        try {
            if (!m_impl->estimate(bgr, palm, hand)) continue;
        } catch (const cv::Exception& e) {
            std::cerr << "[Hands] landmarks failed: " << e.what() << std::endl;
            continue;
        }
        // Глубина руки: LiDAR по ладони; иначе — по размеру ладони (модель даёт метры)
        float zRef = 0.0f;
        const int palmIds[5] = {0, 5, 9, 13, 17};
        if (depth && depth->data && depth->width > 0) {
            std::vector<float> zs;
            for (int id : palmIds) {
                const glm::vec2 p = hand.image[size_t(id)];
                const float nx = (p.x - cam.cx) / cam.fx, ny = (p.y - cam.cy) / cam.fy;
                const int du = int(std::lround(nx * depth->fx + depth->cx));
                const int dv = int(std::lround(ny * depth->fy + depth->cy));
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int u = du + dx, v = dv + dy;
                        if (u < 0 || v < 0 || u >= depth->width || v >= depth->height) continue;
                        const float z = depth->data[size_t(v) * depth->width + u];
                        if (std::isfinite(z) && z > 0.1f && z < 1.5f) zs.push_back(z);
                    }
            }
            if (zs.size() >= 6) {
                std::nth_element(zs.begin(), zs.begin() + zs.size() / 2, zs.end());
                zRef = zs[zs.size() / 2];
                hand.depthFromLidar = true;
            }
        }
        if (zRef <= 0.0f && cam.fx > 0.0f) {
            float metric = 0.0f, pixels = 0.0f;
            const int pairs[3][2] = {{0, 5}, {0, 17}, {5, 17}};
            for (const auto& pr : pairs) {
                metric += glm::length(glm::vec2(hand.local[size_t(pr[0])] - hand.local[size_t(pr[1])]));
                pixels += glm::length(hand.image[size_t(pr[0])] - hand.image[size_t(pr[1])]);
            }
            if (pixels > 1.0f) zRef = cam.fx * metric / pixels;
        }
        if (zRef <= 0.0f) continue;
        glm::vec3 localRef(0.0f);
        for (int id : palmIds) localRef += hand.local[size_t(id)];
        localRef /= 5.0f;
        for (int j = 0; j < kJoints; ++j) {
            const float z = std::max(0.05f, zRef + (hand.local[size_t(j)].z - localRef.z));
            const glm::vec3 pc((hand.image[size_t(j)].x - cam.cx) / cam.fx * z, (hand.image[size_t(j)].y - cam.cy) / cam.fy * z, z);
            hand.world[size_t(j)] = R * pc + cam.position;
        }
        // Сглаживание (дрожание детектора) — тем слабее, чем быстрее рука
        // Две руки «одной стороны» — модель ошиблась со стороной у второй (у человека их по одной)
        if (seen[hand.right ? 1 : 0]) hand.right = !hand.right;
        const int side = hand.right ? 1 : 0;
        if (seen[side]) continue;
        seen[side] = true;
        if (m_impl->havePrev[side]) {
            const float move = glm::length(hand.world[0] - m_impl->prevWorld[side][0]);
            const float a = std::clamp(0.35f + move * 8.0f, 0.35f, 1.0f);
            for (int j = 0; j < kJoints; ++j)
                hand.world[size_t(j)] = glm::mix(m_impl->prevWorld[side][size_t(j)], hand.world[size_t(j)], a);
        }
        m_impl->prevWorld[side] = hand.world;
        m_impl->havePrev[side] = true;

        // Жесты (по метрическим координатам модели — не зависят от расстояния до камеры)
        hand.pinchDistance = glm::length(hand.local[4] - hand.local[8]);
        bool& pinch = m_impl->pinchState[side];
        pinch = pinch ? hand.pinchDistance < 0.04f : hand.pinchDistance < 0.025f;   // гистерезис
        hand.pinching = pinch;
        float tipToPalm = 0.0f;
        for (int tip : {8, 12, 16, 20}) tipToPalm += glm::length(hand.local[size_t(tip)] - localRef);
        hand.fist = tipToPalm / 4.0f < 0.065f;
        hand.pinchPoint = (hand.world[4] + hand.world[8]) * 0.5f;
        hand.palmCenter = (hand.world[0] + hand.world[5] + hand.world[9] + hand.world[13] + hand.world[17]) / 5.0f;
        glm::vec3 n = glm::cross(hand.world[5] - hand.world[0], hand.world[17] - hand.world[0]);
        if (glm::length(n) > 1e-6f) hand.palmNormal = glm::normalize(n) * (hand.right ? 1.0f : -1.0f);
        // Луч указания: от плеча через точку между большим и указательным (как в visionOS/Quest)
        const glm::vec3 camRight = R * glm::vec3(1, 0, 0);
        const glm::vec3 camDown = R * glm::vec3(0, 1, 0);
        const glm::vec3 shoulder = cam.headPosition + camDown * 0.18f + camRight * (hand.right ? 0.17f : -0.17f);
        hand.aimOrigin = (hand.world[4] + hand.world[5]) * 0.5f;
        const glm::vec3 dir = hand.aimOrigin - shoulder;
        if (glm::length(dir) > 1e-4f) hand.aimDirection = glm::normalize(dir);
        result.push_back(hand);
    }
    for (int s = 0; s < 2; ++s)
        if (!seen[s]) {
            m_impl->havePrev[s] = false;
            m_impl->pinchState[s] = false;
        }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hands = result;
    return result;
}

std::vector<HandTrackingProvider::Hand> HandTrackingProvider::hands() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hands;
}

#else // без OpenCV

struct HandTrackingProvider::Impl {};
bool HandTrackingProvider::available() { return false; }
HandTrackingProvider::HandTrackingProvider() : m_impl(std::make_unique<Impl>()) {}
HandTrackingProvider::~HandTrackingProvider() = default;
bool HandTrackingProvider::initialize(const std::string&)
{
    std::cout << "[Hands] built without OpenCV — hand tracking off" << std::endl;
    return false;
}
std::vector<HandTrackingProvider::Hand> HandTrackingProvider::process(int64_t, const uint8_t*, int, const uint8_t*, int, int,
                                                                     int, const Camera&, const Depth*)
{
    return {};
}
std::vector<HandTrackingProvider::Hand> HandTrackingProvider::processBGR(int64_t, const uint8_t*, int, int, int,
                                                                        const Camera&, const Depth*)
{
    return {};
}
std::vector<HandTrackingProvider::Hand> HandTrackingProvider::hands() const
{
    return {};
}

#endif

} // namespace LensEngine
