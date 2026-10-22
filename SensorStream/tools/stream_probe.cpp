// Проверка приёма без лаунчера: слушает 9000/9001, печатает статистику,
// опционально сохраняет последний кадр (--dump out.pgm, яркость Y).
//
//   stream_probe [--seconds N] [--dump frame.pgm]

#include "SensorStream/StreamReceiver.h"

#include <atomic>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

using namespace sensorstream;

int main(int argc, char** argv)
{
    double seconds = 0;   // 0 = бесконечно
    std::string dumpPath;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seconds" && i + 1 < argc) seconds = std::stod(argv[++i]);
        else if (a == "--dump" && i + 1 < argc) dumpPath = argv[++i];
    }

    StreamReceiver rx;
    std::atomic<uint64_t> imuCount{0}, depthCount{0};
    std::atomic<int64_t> lastImuT{0};
    rx.setImuCallback([&](const ImuSample& s) { ++imuCount; lastImuT = s.tNs; });
    std::atomic<float> centerDepth{0};
    rx.setDepthCallback([&](const DepthFrame& d) {
        ++depthCount;
        if (!d.depth.empty()) centerDepth = d.depth[size_t(d.height / 2) * d.width + d.width / 2];
    });

    if (!rx.start()) {
        std::cerr << "cannot start receiver" << std::endl;
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    while (seconds <= 0 || std::chrono::steady_clock::now() - start < std::chrono::duration<double>(seconds)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto st = rx.stats();
    auto frame = rx.latestVideoFrame();
    std::cout << "IMU samples: " << imuCount << ", depth frames: " << depthCount
              << " (center " << centerDepth.load() << " m)"
              << ", decoder: " << st.decoderName << std::endl;
    if (frame) {
        std::cout << "last frame #" << frame->index << " " << frame->width << "x" << frame->height
                  << " fx=" << frame->fx << " cx=" << frame->cx << " fullRange=" << frame->fullRange
                  << " bt709=" << frame->bt709 << std::endl;
        if (!dumpPath.empty()) {
            if (FILE* f = std::fopen(dumpPath.c_str(), "wb")) {
                std::fprintf(f, "P5\n%d %d\n255\n", frame->width, frame->height);
                std::fwrite(frame->y.data(), 1, frame->y.size(), f);
                std::fclose(f);
                std::cout << "saved " << dumpPath << std::endl;
            }
        }
    } else {
        std::cout << "no video decoded" << std::endl;
    }
    rx.stop();
    return frame ? 0 : 2;
}
