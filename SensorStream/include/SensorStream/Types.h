#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace sensorstream {

/// Монотонные часы ПК в наносекундах (steady_clock)
inline int64_t pcNowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

/// Синхронизированный сэмпл IMU в осях устройства (CoreMotion: x вправо, y вверх по экрану, z из экрана).
/// Время — часы телефона (ns). Гироскоп сырой (с bias), акселерометр — specific force в m/s^2.
struct ImuSample {
    int64_t tNs = 0;
    float gyro[3] = {0, 0, 0};
    float accel[3] = {0, 0, 0};
    bool hasAccel = false;
};

struct MagSample {
    int64_t tNs = 0;
    float field[3] = {0, 0, 0};
};

/// Декодированный кадр в NV12 (Y + interleaved CbCr), готов к загрузке на GPU как 2 плоскости.
struct VideoFrame {
    uint64_t index = 0;          // порядковый номер декодированного кадра
    int64_t ptsNs = 0;           // время экспозиции (часы телефона)
    int64_t pcPtsNs = 0;         // то же время в часах ПК (через ClockSync)
    int64_t decodedPcNs = 0;     // когда кадр был готов на ПК
    int width = 0;
    int height = 0;
    float fx = 0, fy = 0, cx = 0, cy = 0;  // интринсики в пикселях этого кадра
    uint32_t exposureUs = 0;
    bool fullRange = true;
    bool bt709 = true;
    std::vector<uint8_t> y;      // width*height
    std::vector<uint8_t> uv;     // width*(height/2), пары Cb,Cr
};

using VideoFramePtr = std::shared_ptr<const VideoFrame>;

/// Карта глубины LiDAR в метрах
struct DepthFrame {
    int64_t tNs = 0;
    int width = 0;
    int height = 0;
    float fx = 0, fy = 0, cx = 0, cy = 0;
    std::vector<float> depth;
};

struct StreamStats {
    bool clientConnected = false;
    bool protocolV2 = false;
    double videoFpsReceived = 0;
    double videoFpsDecoded = 0;
    double imuRateHz = 0;
    double megabitsPerSecond = 0;
    double decodeMsAvg = 0;
    double captureToDecodedMsAvg = 0;   // от экспозиции на телефоне до готового кадра на ПК
    double clockOffsetMs = 0;
    uint64_t framesDroppedDecoder = 0;
    std::string decoderName;
};

} // namespace sensorstream
