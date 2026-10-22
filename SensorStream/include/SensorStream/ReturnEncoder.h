#pragma once

#include "SensorStream/Protocol.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVBufferRef;
struct SwsContext;

namespace sensorstream {

class StreamReceiver;

/// Что телефону нужно знать о кадре, чтобы показать его без лага (доворот по своему гироскопу)
struct ReturnFrameMeta {
    uint8_t layout = 1;            // 1 моно, 2 SBS
    uint8_t mode = 1;              // ReturnMode
    int64_t tRefNs = 0;            // время IMU, от которого предсказана поза (часы телефона)
    int64_t tTargetNs = 0;         // на какой момент предсказана
    int64_t tCameraNs = 0;         // PTS кадра камеры в фоне (0 — фон не с ПК)
    float qPredFromRef[4] = {0, 0, 0, 1};      // x, y, z, w — поворот устройства tRef -> tTarget
    float eyeFromDevice[4] = {0, 0, 0, 1};
    float cameraFromDevice[4] = {0, 0, 0, 1};
    float gyroBias[3] = {0, 0, 0};
    float overscan = 1.0f;
    float tanX = 1.0f, tanY = 1.0f;
    float ipd = 0.063f;
};

/// Кодер обратного потока: кадр лаунчера (RGBA/BGRA из Vulkan) -> H.264/HEVC с минимальной задержкой -> телефон.
///  * свой поток, берёт только самый свежий кадр (очереди нет — задержка не копится)
///  * NVENC (p1/ull) -> VAAPI -> x264 (ultrafast/zerolatency); ARL_RETURN_ENC=nvenc|vaapi|x264|<имя кодека ffmpeg>
///  * alpha = true: кадр вдвое выше, снизу — альфа в яркости (телефон кладёт объекты на свою камеру)
class ReturnEncoder {
public:
    struct Config {
        int fps = 60;
        int bitrateKbps = 12000;
        bool hevc = false;
        std::string encoder = "auto";
    };
    struct Stats {
        double fps = 0;
        double megabitsPerSecond = 0;
        double encodeMsAvg = 0;        // конвертация + кодирование
        double sendMsAvg = 0;
        uint64_t dropped = 0;          // новый кадр пришёл раньше, чем закодирован прошлый
        std::string name;
    };

    ReturnEncoder();
    ~ReturnEncoder();
    ReturnEncoder(const ReturnEncoder&) = delete;
    ReturnEncoder& operator=(const ReturnEncoder&) = delete;

    void start(StreamReceiver* sink);
    void stop();
    void setConfig(const Config& config);

    /// Кадр (копируется). bgra — порядок байт B,G,R,A (иначе R,G,B,A). Возвращает frameId.
    uint32_t submit(const uint8_t* pixels, int width, int height, int stride, bool bgra, bool alpha,
                    const ReturnFrameMeta& meta);
    void requestKeyframe() { m_forceKey = true; }

    Stats stats() const;

    /// Для тестов: колбэк вместо отправки (payload — готовый RETURN_VIDEO)
    void setPacketCallback(std::function<void(const std::vector<uint8_t>&)> cb) { m_onPacket = std::move(cb); }

private:
    struct Pending {
        std::vector<uint8_t> pixels;
        int width = 0, height = 0, stride = 0;
        bool bgra = true;
        bool alpha = false;
        uint32_t frameId = 0;
        ReturnFrameMeta meta;
        bool valid = false;
    };

    void loop();
    bool open(int width, int height, bool alpha);
    void close();
    bool tryOpen(const std::string& name, int width, int height);
    bool convert(const Pending& p);
    void sendPacket(const AVPacket* pkt, const Pending& p);

    StreamReceiver* m_sink = nullptr;
    std::function<void(const std::vector<uint8_t>&)> m_onPacket;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    Config m_config;
    bool m_configChanged = false;
    Pending m_pending;
    Pending m_work;
    uint32_t m_nextId = 1;
    std::atomic<bool> m_forceKey{true};

    // FFmpeg (только поток кодера)
    AVCodecContext* m_ctx = nullptr;
    AVBufferRef* m_hwDevice = nullptr;
    AVFrame* m_frame = nullptr;
    AVFrame* m_hwFrame = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_sws = nullptr;
    int m_swFormat = -1;
    int m_width = 0, m_height = 0;     // цветная часть
    bool m_alpha = false;
    bool m_hevc = false;
    int64_t m_pts = 0;
    std::string m_name;
    std::vector<uint8_t> m_out;

    // Статистика
    mutable std::mutex m_statsMutex;
    Stats m_stats;
    int64_t m_statsStart = 0;
    uint64_t m_frames = 0, m_bytes = 0;
    int64_t m_encodeNs = 0, m_sendNs = 0;
    std::atomic<uint64_t> m_dropped{0};
};

} // namespace sensorstream
