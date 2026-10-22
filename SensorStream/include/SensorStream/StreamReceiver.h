#pragma once

#include "SensorStream/ClockSync.h"
#include "SensorStream/Protocol.h"
#include "SensorStream/Types.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sensorstream {

/// Приёмник потока с iPhone (USB-модем / Wi-Fi) — без Qt, в своих потоках:
///  * сетевой поток: TCP-сервер на 9000 (Wi-Fi) и 9001 (USB), разбор пакетов, IMU сразу в колбэк
///  * поток декодера: H.264/HEVC/JPEG → NV12, публикуется только последний кадр (без очереди → без лага)
/// Понимает протокол v2 и старые пакеты (0x01 JPEG, 0x03 IMU) для совместимости.
class StreamReceiver {
public:
    struct Config {
        std::vector<uint16_t> ports{9000, 9001};
        bool verbose = true;
    };

    using ImuCallback = std::function<void(const ImuSample&)>;
    using MagCallback = std::function<void(const MagSample&)>;
    using DepthCallback = std::function<void(const DepthFrame&)>;
    using VideoCallback = std::function<void(const VideoFramePtr&)>;
    using AudioCallback = std::function<void(const AudioChunk&)>;
    using EyeCallback = std::function<void(const EyeImage&)>;
    using DisplayCallback = std::function<void(const DisplayRequest&)>;
    using AckCallback = std::function<void(const ReturnAck&)>;

    StreamReceiver();
    ~StreamReceiver();

    StreamReceiver(const StreamReceiver&) = delete;
    StreamReceiver& operator=(const StreamReceiver&) = delete;

    bool start(const Config& config);
    bool start() { return start(Config{}); }
    void stop();

    // Колбэки задаются до start(). IMU/Mag/Depth вызываются из сетевого потока, Video — из потока декодера.
    void setImuCallback(ImuCallback cb) { m_onImu = std::move(cb); }
    void setMagCallback(MagCallback cb) { m_onMag = std::move(cb); }
    void setDepthCallback(DepthCallback cb) { m_onDepth = std::move(cb); }
    void setVideoCallback(VideoCallback cb) { m_onVideo = std::move(cb); }
    void setAudioCallback(AudioCallback cb) { m_onAudio = std::move(cb); }
    void setEyeCallback(EyeCallback cb) { m_onEye = std::move(cb); }
    void setDisplayCallback(DisplayCallback cb) { m_onDisplay = std::move(cb); }
    void setAckCallback(AckCallback cb) { m_onAck = std::move(cb); }

    /// Пакет на телефон (тот клиент, что прислал DISPLAY). Из любого потока; false — некому/ошибка.
    /// Блокирует не дольше ~250 мс (если телефон перестал читать — соединение рвётся).
    bool sendToPhone(PacketType type, const uint8_t* payload, size_t size);
    bool hasReturnClient() const { return m_returnFd.load() >= 0; }
    /// Последний запрос показа (mode = 0 — телефон картинку не ждёт)
    DisplayRequest displayRequest() const;

    /// Последний декодированный кадр (nullptr если ещё нет). Потокобезопасно, без копирования пикселей.
    VideoFramePtr latestVideoFrame() const;

    const ClockSync& clock() const { return m_clock; }
    StreamStats stats() const;

private:
    struct Client {
        int fd = -1;
        std::vector<uint8_t> buffer;
        size_t readPos = 0;
        bool streaming = false;
    };

    struct EncodedFrame {
        VideoCodec codec = VideoCodec::H264;
        bool keyframe = false;
        int64_t ptsNs = 0;
        int width = 0, height = 0;
        float fx = 0, fy = 0, cx = 0, cy = 0;
        uint32_t exposureUs = 0;
        uint8_t flags = 0;          // биты VIDEO-заголовка (kVideoFlag*)
        std::vector<uint8_t> data;
    };

    void networkLoop();
    void decodeLoop();
    bool openListener(uint16_t port);
    void closeClient(Client& c);
    void processBuffer(Client& c);
    void handlePacket(Client& c, uint8_t type, const uint8_t* p, size_t n);

    void handleHello(const uint8_t* p, size_t n);
    void handleVideo(const uint8_t* p, size_t n);
    void handleImu(const uint8_t* p, size_t n);
    void handleAudio(const uint8_t* p, size_t n);
    void handleEye(const uint8_t* p, size_t n);
    void handleDisplay(Client& c, const uint8_t* p, size_t n);
    void handleAck(const uint8_t* p, size_t n);
    void handleDepth(const uint8_t* p, size_t n);
    void handleLegacyJpeg(const uint8_t* p, size_t n);
    void handleLegacyImu(const uint8_t* p, size_t n);
    void emitGyro(int64_t tNs, const float g[3]);

    void queueEncoded(EncodedFrame&& f);
    std::shared_ptr<VideoFrame> acquirePoolFrame();
    void updateStats();

    Config m_config;
    std::atomic<bool> m_running{false};
    std::thread m_netThread;
    std::thread m_decodeThread;
    std::vector<int> m_listeners;
    std::vector<Client> m_clients;

    ImuCallback m_onImu;
    MagCallback m_onMag;
    DepthCallback m_onDepth;
    VideoCallback m_onVideo;
    AudioCallback m_onAudio;
    EyeCallback m_onEye;
    DisplayCallback m_onDisplay;
    AckCallback m_onAck;

    // Обратный поток на телефон
    std::atomic<int> m_returnFd{-1};
    std::mutex m_sendMutex;          // запись в m_returnFd и его закрытие
    uint64_t m_sendSequence = 0;
    mutable std::mutex m_displayMutex;
    DisplayRequest m_display;

    ClockSync m_clock;

    // IMU sync (сетевой поток)
    float m_lastAccel[3] = {0, 0, 0};
    bool m_haveAccel = false;

    // Очередь на декодирование
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::deque<EncodedFrame> m_queue;
    bool m_waitKeyframe = true;
    std::atomic<bool> m_resetDecoder{false};

    // Публикация кадров
    mutable std::mutex m_latestMutex;
    VideoFramePtr m_latest;
    std::vector<std::shared_ptr<VideoFrame>> m_pool;
    uint64_t m_decodedIndex = 0;

    // Статистика
    std::atomic<uint64_t> m_bytesRx{0};
    std::atomic<uint64_t> m_videoRx{0};
    std::atomic<uint64_t> m_videoDecoded{0};
    std::atomic<uint64_t> m_imuRx{0};
    std::atomic<uint64_t> m_audioSamplesRx{0};
    std::atomic<uint64_t> m_eyeRx{0};
    std::atomic<uint64_t> m_droppedDecoder{0};
    std::atomic<int64_t> m_decodeNsSum{0};
    std::atomic<int64_t> m_latencyNsSum{0};
    std::atomic<bool> m_protocolV2{false};
    mutable std::mutex m_statsMutex;
    StreamStats m_stats;
    int64_t m_statsWindowStart = 0;
    std::string m_decoderName;
};

} // namespace sensorstream
