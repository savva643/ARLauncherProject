#pragma once

#include "SensorStream/Protocol.h"
#include "SensorStream/Types.h"

#include <functional>
#include <string>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVBufferRef;
struct SwsContext;

namespace sensorstream {

/// FFmpeg-декодер с низкой задержкой (без frame-threading, без буферизации).
/// Аппаратное ускорение: env ARL_HWDEC=auto|vaapi|cuda|none (по умолчанию auto → vaapi → cuda → CPU).
/// На выходе всегда NV12 в VideoFrame (буферы переиспользуются).
class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    /// Декодирует один access unit. При успехе заполняет out (размер/данные) и возвращает true.
    bool decode(VideoCodec codec, const uint8_t* data, size_t size, int64_t ptsNs, VideoFrame& out);

    void reset();
    const std::string& name() const { return m_name; }

private:
    bool open(VideoCodec codec);
    void close();
    bool toNV12(const AVFrame* src, VideoFrame& out);

    VideoCodec m_codec = VideoCodec::H264;
    bool m_opened = false;
    AVCodecContext* m_ctx = nullptr;
    AVBufferRef* m_hwDevice = nullptr;
    int m_hwPixFmt = -1;
    AVFrame* m_frame = nullptr;
    AVFrame* m_swFrame = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_sws = nullptr;
    int m_swsW = 0, m_swsH = 0, m_swsFmt = -1;
    std::string m_name;
};

} // namespace sensorstream
