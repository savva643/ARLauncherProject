// Руки: настоящее фото (tests/data/hands.jpg, из tfjs-models, Apache-2.0) — две руки, правая перевёрнута.
// Проверяем: обе найдены, суставы на своих руках, ориентация пальцев, 3D по размеру ладони, жесты считаются.

#include "HandTrackingProvider.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cstdio>
#include <string>

int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : LENSENGINE_TEST_DATA;
    cv::Mat img = cv::imread(dir + "/hands.jpg");
    if (img.empty()) {
        std::printf("no test image\nFAIL\n");
        return 1;
    }
    LensEngine::HandTrackingProvider hands;
    if (!hands.initialize("")) {
        std::printf("models not found\nFAIL\n");
        return 1;
    }
    LensEngine::HandTrackingProvider::Camera cam;
    cam.fx = cam.fy = 600.0f;
    cam.cx = img.cols * 0.5f;
    cam.cy = img.rows * 0.5f;
    auto t0 = std::chrono::steady_clock::now();
    auto found = hands.processBGR(0, img.data, img.cols, img.rows, int(img.step), cam);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("found %zu hands in %.1f ms\n", found.size(), ms);

    bool leftUp = false, rightDown = false;
    for (const auto& h : found) {
        const auto& w = h.image[0];
        const auto& tip = h.image[12];
        std::printf("  %s hand conf %.2f: wrist (%.0f, %.0f) middle tip (%.0f, %.0f), depth %.2f m, pinch %.3f m%s\n",
                    h.right ? "right" : "left", h.confidence, w.x, w.y, tip.x, tip.y, h.world[0].z, h.pinchDistance,
                    h.fist ? ", fist" : "");
        // Все суставы — на одной половине кадра
        bool sameSide = true;
        for (const auto& p : h.image) sameSide = sameSide && ((p.x < img.cols * 0.5f) == (w.x < img.cols * 0.5f));
        if (w.x < img.cols * 0.5f && tip.y < w.y - 100 && sameSide) leftUp = true;      // рука слева, пальцы вверх
        if (w.x > img.cols * 0.5f && tip.y > w.y + 100 && sameSide) rightDown = true;   // рука справа, пальцы вниз
        // раскрытая ладонь: не щипок и не кулак; глубина правдоподобна
        if (h.pinching || h.fist || h.world[0].z < 0.1f || h.world[0].z > 3.0f) leftUp = rightDown = false;
    }
    if (argc > 2) {   // картинка с суставами для глаз
        const int bones[20][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {0, 5}, {5, 6}, {6, 7}, {7, 8}, {5, 9}, {9, 10},
                                  {10, 11}, {11, 12}, {9, 13}, {13, 14}, {14, 15}, {15, 16}, {13, 17}, {0, 17}, {17, 18}, {18, 19}};
        for (const auto& h : found) {
            for (const auto& b : bones)
                cv::line(img, {int(h.image[b[0]].x), int(h.image[b[0]].y)}, {int(h.image[b[1]].x), int(h.image[b[1]].y)},
                         {0, 200, 0}, 2);
            cv::line(img, {int(h.image[19].x), int(h.image[19].y)}, {int(h.image[20].x), int(h.image[20].y)}, {0, 200, 0}, 2);
            for (const auto& p : h.image) cv::circle(img, {int(p.x), int(p.y)}, 4, {0, 0, 255}, -1);
        }
        cv::imwrite(argv[2], img);
    }
    const bool ok = found.size() == 2 && leftUp && rightDown;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
