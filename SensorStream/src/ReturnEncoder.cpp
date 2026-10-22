#include "SensorStream/ReturnEncoder.h"
#include "SensorStream/StreamReceiver.h"
#include "SensorStream/Types.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace sensorstream {

ReturnEncoder::ReturnEncoder() = default;

ReturnEncoder::~ReturnEncoder()
{
    stop();
}

void ReturnEncoder::start(StreamReceiver* sink)
{
    if (m_running) return;
    m_sink = sink;
    m_running = true;
    m_statsStart = pcNowNs();
    m_thread = std::thread(&ReturnEncoder::loop, this);
}

void ReturnEncoder::stop()
{
    if (!m_running) return;
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
    close();
}

void ReturnEncoder::setConfig(const Config& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (config.fps == m_config.fps && config.bitrateKbps == m_config.bitrateKbps && config.hevc == m_config.hevc &&
        config.encoder == m_config.encoder)
        return;
    m_config = config;
    m_configChanged = true;
}

uint32_t ReturnEncoder::submit(const uint8_t* pixels, int width, int height, int stride, bool bgra, bool alpha,
                               const ReturnFrameMeta& meta)
{
    width &= ~1;
    height &= ~1;
    if (!pixels || width < 2 || height < 2) return 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pending.valid) ++m_dropped;   // прошлый ещё не взят кодером — заменяем свежим
    Pending& p = m_pending;
    p.width = width;
    p.height = height;
    p.stride = width * 4;
    p.pixels.resize(size_t(p.stride) * size_t(height));
    for (int y = 0; y < height; ++y)
        std::memcpy(p.pixels.data() + size_t(y) * size_t(p.stride), pixels + size_t(y) * size_t(stride), size_t(p.stride));
    p.bgra = bgra;
    p.alpha = alpha;
    p.meta = meta;
    p.frameId = m_nextId++;
    p.valid = true;
    m_cv.notify_one();
    return p.frameId;
}

ReturnEncoder::Stats ReturnEncoder::stats() const
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

// ---------------------------------------------------------------- FFmpeg

namespace {

std::vector<std::string> candidates(const std::string& choice, bool hevc)
{
    const std::string c = hevc ? "hevc" : "h264";
    if (choice == "nvenc") return {c + "_nvenc"};
    if (choice == "vaapi") return {c + "_vaapi"};
    if (choice == "x264" || choice == "x265" || choice == "cpu") return {hevc ? "libx265" : "libx264"};
    if (choice == "rkmpp") return {c + "_rkmpp"};    // Orange Pi 5 (RK3588), ffmpeg-rockchip
    if (choice == "v4l2") return {c + "_v4l2m2m"};   // Raspberry Pi и др.
    if (!choice.empty() && choice != "auto") return {choice};
    return {c + "_nvenc", c + "_vaapi", c + "_rkmpp", hevc ? "libx265" : "libx264"};
}

bool supports(const AVCodec* codec, AVPixelFormat fmt)
{
    if (!codec->pix_fmts) return fmt == AV_PIX_FMT_YUV420P;
    for (const AVPixelFormat* p = codec->pix_fmts; *p != AV_PIX_FMT_NONE; ++p)
        if (*p == fmt) return true;
    return false;
}

} // namespace

bool ReturnEncoder::tryOpen(const std::string& name, int width, int height)
{
    const AVCodec* codec = avcodec_find_encoder_by_name(name.c_str());
    if (!codec) return false;
    const bool vaapi = name.find("_vaapi") != std::string::npos;

    Config cfg;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cfg = m_config;
    }
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return false;
    const int fps = std::clamp(cfg.fps, 10, 240);
    const int64_t bitrate = int64_t(std::clamp(cfg.bitrateKbps, 500, 200000)) * 1000;
    ctx->width = width;
    ctx->height = height;
    ctx->time_base = AVRational{1, fps};
    ctx->framerate = AVRational{fps, 1};
    ctx->gop_size = fps * 2;
    ctx->max_b_frames = 0;
    ctx->bit_rate = bitrate;
    ctx->rc_max_rate = bitrate;
    ctx->rc_buffer_size = int(bitrate / fps * 2);   // ~2 кадра: размер кадра ровный, сеть не забивается
    ctx->thread_count = 0;
    ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    ctx->color_range = AVCOL_RANGE_JPEG;
    ctx->colorspace = AVCOL_SPC_BT709;
    ctx->color_primaries = AVCOL_PRI_BT709;
    ctx->color_trc = AVCOL_TRC_BT709;
    // SPS/PPS (VPS) в потоке перед каждым keyframe: телефон может подключиться в любой момент

    AVPixelFormat sw = supports(codec, AV_PIX_FMT_NV12) ? AV_PIX_FMT_NV12 : AV_PIX_FMT_YUV420P;
    if (name.rfind("libx26", 0) == 0) sw = AV_PIX_FMT_YUV420P;

    if (name.rfind("libx264", 0) == 0) {
        av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
        av_opt_set(ctx->priv_data, "x264-params", "aud=0:intra-refresh=0", 0);
    } else if (name.rfind("libx265", 0) == 0) {
        av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
        av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
    } else if (name.find("_nvenc") != std::string::npos) {
        av_opt_set(ctx->priv_data, "preset", "p1", 0);
        av_opt_set(ctx->priv_data, "tune", "ull", 0);
        av_opt_set(ctx->priv_data, "zerolatency", "1", 0);
        av_opt_set(ctx->priv_data, "delay", "0", 0);
        av_opt_set(ctx->priv_data, "rc", "cbr", 0);
        av_opt_set(ctx->priv_data, "forced-idr", "1", 0);
    } else if (vaapi) {
        av_opt_set(ctx->priv_data, "rc_mode", "CBR", 0);
        av_opt_set_int(ctx->priv_data, "async_depth", 1, 0);
    }

    if (vaapi) {
        const char* dev = std::getenv("ARL_VAAPI_DEVICE");
        if (av_hwdevice_ctx_create(&m_hwDevice, AV_HWDEVICE_TYPE_VAAPI, dev, nullptr, 0) < 0) {
            avcodec_free_context(&ctx);
            return false;
        }
        AVBufferRef* frames = av_hwframe_ctx_alloc(m_hwDevice);
        auto* fc = reinterpret_cast<AVHWFramesContext*>(frames->data);
        fc->format = AV_PIX_FMT_VAAPI;
        fc->sw_format = AV_PIX_FMT_NV12;
        fc->width = width;
        fc->height = height;
        fc->initial_pool_size = 4;
        if (av_hwframe_ctx_init(frames) < 0) {
            av_buffer_unref(&frames);
            av_buffer_unref(&m_hwDevice);
            avcodec_free_context(&ctx);
            return false;
        }
        ctx->hw_frames_ctx = av_buffer_ref(frames);
        ctx->pix_fmt = AV_PIX_FMT_VAAPI;
        av_buffer_unref(&frames);
        sw = AV_PIX_FMT_NV12;
    } else {
        ctx->pix_fmt = sw;
    }

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        if (m_hwDevice) av_buffer_unref(&m_hwDevice);
        return false;
    }

    m_ctx = ctx;
    m_swFormat = sw;
    m_frame = av_frame_alloc();
    m_frame->format = sw;
    m_frame->width = width;
    m_frame->height = height;
    m_frame->color_range = AVCOL_RANGE_JPEG;
    m_frame->colorspace = AVCOL_SPC_BT709;
    if (av_frame_get_buffer(m_frame, 32) < 0) {
        close();
        return false;
    }
    if (vaapi) m_hwFrame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_name = name;
    return true;
}

bool ReturnEncoder::open(int width, int height, bool alpha)
{
    close();
    Config cfg;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cfg = m_config;
        m_configChanged = false;
    }
    const char* env = std::getenv("ARL_RETURN_ENC");
    const std::string choice = env && env[0] ? env : cfg.encoder;
    const int codedHeight = alpha ? height * 2 : height;
    for (const std::string& name : candidates(choice, cfg.hevc)) {
        if (tryOpen(name, width, codedHeight)) {
            m_width = width;
            m_height = height;
            m_alpha = alpha;
            m_hevc = cfg.hevc;
            m_forceKey = true;
            std::cout << "[RETURN] encoder " << m_name << " " << width << "x" << codedHeight << (alpha ? " (color + alpha)" : "")
                      << " @" << cfg.fps << " fps, " << cfg.bitrateKbps / 1000.0 << " Mbit/s" << std::endl;
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_stats.name = m_name;
            return true;
        }
    }
    std::cerr << "[RETURN] no H.264/HEVC encoder available (tried " << choice << ")" << std::endl;
    return false;
}

void ReturnEncoder::close()
{
    if (m_ctx) avcodec_free_context(&m_ctx);
    if (m_frame) av_frame_free(&m_frame);
    if (m_hwFrame) av_frame_free(&m_hwFrame);
    if (m_packet) av_packet_free(&m_packet);
    if (m_hwDevice) av_buffer_unref(&m_hwDevice);
    if (m_sws) sws_freeContext(m_sws);
    m_sws = nullptr;
    m_width = m_height = 0;
}

bool ReturnEncoder::convert(const Pending& p)
{
    const AVPixelFormat src = p.bgra ? AV_PIX_FMT_BGRA : AV_PIX_FMT_RGBA;
    m_sws = sws_getCachedContext(m_sws, p.width, p.height, src, p.width, p.height, AVPixelFormat(m_swFormat),
                                 SWS_POINT, nullptr, nullptr, nullptr);
    if (!m_sws) return false;
    // Полный диапазон BT.709 — как написано в VUI потока и во флагах заголовка
    const int* coef = sws_getCoefficients(SWS_CS_ITU709);
    sws_setColorspaceDetails(m_sws, coef, 1, coef, 1, 0, 1 << 16, 1 << 16);
    if (av_frame_make_writable(m_frame) < 0) return false;

    const uint8_t* srcData[1] = {p.pixels.data()};
    const int srcStride[1] = {p.stride};
    // Цвет — верхняя половина кадра (плоскости кадра выше на высоту альфы, указатели те же)
    sws_scale(m_sws, srcData, srcStride, 0, p.height, m_frame->data, m_frame->linesize);

    if (m_alpha) {
        // Альфа — яркость нижней половины, цветность нейтральная
        const int a = 3;
        for (int y = 0; y < p.height; ++y) {
            const uint8_t* s = p.pixels.data() + size_t(y) * size_t(p.stride) + a;
            uint8_t* d = m_frame->data[0] + size_t(p.height + y) * size_t(m_frame->linesize[0]);
            for (int x = 0; x < p.width; ++x) d[x] = s[size_t(x) * 4];
        }
        const int ch = p.height / 2;
        if (m_swFormat == AV_PIX_FMT_NV12) {
            for (int y = 0; y < ch; ++y)
                std::memset(m_frame->data[1] + size_t(ch + y) * size_t(m_frame->linesize[1]), 128, size_t(p.width));
        } else {
            for (int plane = 1; plane <= 2; ++plane)
                for (int y = 0; y < ch; ++y)
                    std::memset(m_frame->data[plane] + size_t(ch + y) * size_t(m_frame->linesize[plane]), 128, size_t(p.width / 2));
        }
    }
    return true;
}

void ReturnEncoder::sendPacket(const AVPacket* pkt, const Pending& p)
{
    const ReturnFrameMeta& m = p.meta;
    m_out.resize(kReturnVideoHeaderSize + size_t(pkt->size));
    uint8_t* h = m_out.data();
    std::memset(h, 0, kReturnVideoHeaderSize);
    h[0] = uint8_t(m_hevc ? VideoCodec::HEVC : VideoCodec::H264);
    uint8_t flags = kReturnFlagFullRange | kReturnFlagBT709;
    if (pkt->flags & AV_PKT_FLAG_KEY) flags |= kReturnFlagKeyframe;
    if (m_alpha) flags |= kReturnFlagAlpha;
    h[1] = flags;
    h[2] = m.layout;
    h[3] = m.mode;
    wrU16(h + 4, uint16_t(m_width));
    wrU16(h + 6, uint16_t(m_alpha ? m_height * 2 : m_height));
    wrU32(h + 8, p.frameId);
    wrI64(h + 16, m.tRefNs);
    wrI64(h + 24, m.tTargetNs);
    wrI64(h + 32, m.tCameraNs);
    const bool clockOk = m_sink && m_sink->clock().valid();
    wrI64(h + 40, clockOk ? m_sink->clock().pcToPhone(pcNowNs()) : 0);
    for (int i = 0; i < 4; ++i) {
        wrF32(h + 48 + 4 * i, m.qPredFromRef[i]);
        wrF32(h + 64 + 4 * i, m.eyeFromDevice[i]);
        wrF32(h + 80 + 4 * i, m.cameraFromDevice[i]);
    }
    for (int i = 0; i < 3; ++i) wrF32(h + 96 + 4 * i, m.gyroBias[i]);
    wrF32(h + 108, m.overscan);
    wrF32(h + 112, m.tanX);
    wrF32(h + 116, m.tanY);
    wrF32(h + 120, m.ipd);
    std::memcpy(h + kReturnVideoHeaderSize, pkt->data, size_t(pkt->size));

    const int64_t t0 = pcNowNs();
    if (m_onPacket) m_onPacket(m_out);
    if (m_sink) m_sink->sendToPhone(PacketType::ReturnVideo, m_out.data(), m_out.size());
    m_sendNs += pcNowNs() - t0;
    m_bytes += m_out.size();
}

void ReturnEncoder::loop()
{
    while (m_running) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(100), [this] { return !m_running || m_pending.valid; });
            if (!m_running) break;
            if (!m_pending.valid) continue;
            std::swap(m_work, m_pending);
            m_pending.valid = false;
        }
        const Pending& p = m_work;
        bool reconfigure;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            reconfigure = m_configChanged;
        }
        if (!m_ctx || reconfigure || p.width != m_width || p.height != m_height || p.alpha != m_alpha) {
            if (!open(p.width, p.height, p.alpha)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        const int64_t t0 = pcNowNs();
        if (!convert(p)) continue;
        m_frame->pts = m_pts++;
        m_frame->pict_type = m_forceKey.exchange(false) ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
        AVFrame* input = m_frame;
        if (m_hwFrame) {
            av_frame_unref(m_hwFrame);
            if (av_hwframe_get_buffer(m_ctx->hw_frames_ctx, m_hwFrame, 0) < 0 ||
                av_hwframe_transfer_data(m_hwFrame, m_frame, 0) < 0)
                continue;
            m_hwFrame->pts = m_frame->pts;
            m_hwFrame->pict_type = m_frame->pict_type;
            input = m_hwFrame;
        }
        if (avcodec_send_frame(m_ctx, input) < 0) {
            std::cerr << "[RETURN] encode error, reopening" << std::endl;
            close();
            continue;
        }
        bool any = false;
        while (avcodec_receive_packet(m_ctx, m_packet) == 0) {
            if (!any) m_encodeNs += pcNowNs() - t0;
            any = true;
            sendPacket(m_packet, p);
            av_packet_unref(m_packet);
        }
        if (any) ++m_frames;

        const int64_t now = pcNowNs();
        if (now - m_statsStart >= 1'000'000'000LL) {
            const double sec = double(now - m_statsStart) * 1e-9;
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_stats.fps = double(m_frames) / sec;
            m_stats.megabitsPerSecond = double(m_bytes) * 8e-6 / sec;
            m_stats.encodeMsAvg = m_frames ? double(m_encodeNs) * 1e-6 / double(m_frames) : 0.0;
            m_stats.sendMsAvg = m_frames ? double(m_sendNs) * 1e-6 / double(m_frames) : 0.0;
            m_stats.dropped = m_dropped.load();
            m_frames = m_bytes = 0;
            m_encodeNs = m_sendNs = 0;
            m_statsStart = now;
        }
    }
}

} // namespace sensorstream
