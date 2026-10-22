#include "SensorStream/VideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace sensorstream {

namespace {

AVPixelFormat pickHwFormat(AVCodecContext* ctx, const AVPixelFormat* fmts)
{
    const int wanted = ctx->opaque ? *static_cast<const int*>(ctx->opaque) : -1;
    for (const AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == wanted) return *p;
    }
    // hw недоступен для этого потока — софт
    for (const AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p) {
        const AVPixFmtDescriptor* d = av_pix_fmt_desc_get(*p);
        if (d && !(d->flags & AV_PIX_FMT_FLAG_HWACCEL)) return *p;
    }
    return fmts[0];
}

std::vector<uint8_t>& paddedBuffer()
{
    thread_local std::vector<uint8_t> buf;
    return buf;
}

} // namespace

VideoDecoder::VideoDecoder() = default;

VideoDecoder::~VideoDecoder()
{
    close();
}

void VideoDecoder::close()
{
    if (m_sws) { sws_freeContext(m_sws); m_sws = nullptr; }
    if (m_frame) av_frame_free(&m_frame);
    if (m_swFrame) av_frame_free(&m_swFrame);
    if (m_packet) av_packet_free(&m_packet);
    if (m_ctx) avcodec_free_context(&m_ctx);
    if (m_hwDevice) av_buffer_unref(&m_hwDevice);
    m_hwPixFmt = -1;
    m_opened = false;
    m_swsW = m_swsH = 0;
    m_swsFmt = -1;
}

void VideoDecoder::reset()
{
    close();
}

bool VideoDecoder::open(VideoCodec codec)
{
    close();
    m_codec = codec;

    AVCodecID id = AV_CODEC_ID_H264;
    if (codec == VideoCodec::HEVC) id = AV_CODEC_ID_HEVC;
    else if (codec == VideoCodec::JPEG) id = AV_CODEC_ID_MJPEG;

    const AVCodec* dec = avcodec_find_decoder(id);
    if (!dec) {
        std::cerr << "[SensorStream] decoder not found for codec " << int(codec) << std::endl;
        return false;
    }

    m_ctx = avcodec_alloc_context3(dec);
    m_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_ctx->flags2 |= AV_CODEC_FLAG2_FAST;
    // Frame-threading добавляет N кадров задержки — только slice-потоки
    m_ctx->thread_type = FF_THREAD_SLICE;
    m_ctx->thread_count = 0;

    // --- аппаратное декодирование
    std::string mode = "auto";
    if (const char* env = std::getenv("ARL_HWDEC")) mode = env;

    std::string hwName;
    if (codec != VideoCodec::JPEG && mode != "none") {
        std::vector<AVHWDeviceType> candidates;
        if (mode == "vaapi") candidates = {AV_HWDEVICE_TYPE_VAAPI};
        else if (mode == "cuda") candidates = {AV_HWDEVICE_TYPE_CUDA};
        else candidates = {AV_HWDEVICE_TYPE_VAAPI, AV_HWDEVICE_TYPE_CUDA};

        for (AVHWDeviceType type : candidates) {
            int pixFmt = -1;
            for (int i = 0;; ++i) {
                const AVCodecHWConfig* cfg = avcodec_get_hw_config(dec, i);
                if (!cfg) break;
                if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) && cfg->device_type == type) {
                    pixFmt = cfg->pix_fmt;
                    break;
                }
            }
            if (pixFmt < 0) continue;
            AVBufferRef* dev = nullptr;
            if (av_hwdevice_ctx_create(&dev, type, nullptr, nullptr, 0) < 0) continue;
            m_hwDevice = dev;
            m_hwPixFmt = pixFmt;
            m_ctx->hw_device_ctx = av_buffer_ref(m_hwDevice);
            m_ctx->opaque = &m_hwPixFmt;
            m_ctx->get_format = pickHwFormat;
            hwName = av_hwdevice_get_type_name(type);
            break;
        }
    }

    if (avcodec_open2(m_ctx, dec, nullptr) < 0) {
        std::cerr << "[SensorStream] avcodec_open2 failed" << std::endl;
        close();
        return false;
    }

    m_frame = av_frame_alloc();
    m_swFrame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_opened = true;
    m_name = std::string(dec->name) + (hwName.empty() ? " (CPU)" : " (" + hwName + ")");
    std::cout << "[SensorStream] video decoder: " << m_name << std::endl;
    return true;
}

bool VideoDecoder::decode(VideoCodec codec, const uint8_t* data, size_t size, int64_t ptsNs, VideoFrame& out)
{
    if (!m_opened || codec != m_codec) {
        if (!open(codec)) return false;
    }

    auto& buf = paddedBuffer();
    buf.resize(size + AV_INPUT_BUFFER_PADDING_SIZE);
    std::memcpy(buf.data(), data, size);
    std::memset(buf.data() + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    m_packet->data = buf.data();
    m_packet->size = static_cast<int>(size);
    m_packet->pts = ptsNs;
    m_packet->dts = ptsNs;

    int ret = avcodec_send_packet(m_ctx, m_packet);
    m_packet->data = nullptr;
    m_packet->size = 0;
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        return false;
    }

    bool got = false;
    while (true) {
        ret = avcodec_receive_frame(m_ctx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            // Аппаратный декодер сломался — переходим на CPU
            if (m_hwDevice) {
                std::cerr << "[SensorStream] hw decode error, falling back to CPU" << std::endl;
                setenv("ARL_HWDEC", "none", 1);
                close();
            }
            return false;
        }
        got = toNV12(m_frame, out);
        if (got) out.ptsNs = m_frame->pts != AV_NOPTS_VALUE ? m_frame->pts : ptsNs;
        av_frame_unref(m_frame);
    }
    return got;
}

bool VideoDecoder::toNV12(const AVFrame* frameIn, VideoFrame& out)
{
    const AVFrame* src = frameIn;
    if (frameIn->hw_frames_ctx) {
        av_frame_unref(m_swFrame);
        if (av_hwframe_transfer_data(m_swFrame, frameIn, 0) < 0) return false;
        m_swFrame->color_range = frameIn->color_range;
        m_swFrame->colorspace = frameIn->colorspace;
        src = m_swFrame;
    }

    const int w = src->width;
    const int h = src->height;
    if (w <= 0 || h <= 0) return false;

    const int uvW = (w + 1) / 2;
    const int uvH = (h + 1) / 2;
    out.width = w;
    out.height = h;
    out.y.resize(size_t(w) * h);
    out.uv.resize(size_t(uvW) * 2 * uvH);

    const AVPixelFormat fmt = static_cast<AVPixelFormat>(src->format);
    const bool jpegFmt = fmt == AV_PIX_FMT_YUVJ420P || fmt == AV_PIX_FMT_YUVJ422P || fmt == AV_PIX_FMT_YUVJ444P;
    // iPhone пишет 420f (full range); если диапазон не указан — считаем full
    out.fullRange = jpegFmt || src->color_range != AVCOL_RANGE_MPEG;
    if (jpegFmt) {
        out.bt709 = (src->colorspace == AVCOL_SPC_BT709);   // JPEG по умолчанию BT.601
    } else {
        out.bt709 = !(src->colorspace == AVCOL_SPC_SMPTE170M || src->colorspace == AVCOL_SPC_BT470BG);
    }

    if (fmt == AV_PIX_FMT_NV12) {
        for (int r = 0; r < h; ++r)
            std::memcpy(out.y.data() + size_t(r) * w, src->data[0] + size_t(r) * src->linesize[0], w);
        for (int r = 0; r < uvH; ++r)
            std::memcpy(out.uv.data() + size_t(r) * uvW * 2, src->data[1] + size_t(r) * src->linesize[1], uvW * 2);
        return true;
    }

    if (fmt == AV_PIX_FMT_YUV420P || fmt == AV_PIX_FMT_YUVJ420P) {
        for (int r = 0; r < h; ++r)
            std::memcpy(out.y.data() + size_t(r) * w, src->data[0] + size_t(r) * src->linesize[0], w);
        for (int r = 0; r < uvH; ++r) {
            const uint8_t* u = src->data[1] + size_t(r) * src->linesize[1];
            const uint8_t* v = src->data[2] + size_t(r) * src->linesize[2];
            uint8_t* d = out.uv.data() + size_t(r) * uvW * 2;
            for (int c = 0; c < uvW; ++c) {
                d[2 * c] = u[c];
                d[2 * c + 1] = v[c];
            }
        }
        return true;
    }

    // Любой другой формат (422 JPEG, 10 бит и т.д.) — через swscale
    if (!m_sws || m_swsW != w || m_swsH != h || m_swsFmt != fmt) {
        if (m_sws) sws_freeContext(m_sws);
        m_sws = sws_getContext(w, h, fmt, w, h, AV_PIX_FMT_NV12, SWS_BILINEAR, nullptr, nullptr, nullptr);
        m_swsW = w;
        m_swsH = h;
        m_swsFmt = fmt;
        if (!m_sws) return false;
    }
    uint8_t* dst[4] = {out.y.data(), out.uv.data(), nullptr, nullptr};
    int dstStride[4] = {w, uvW * 2, 0, 0};
    sws_scale(m_sws, src->data, src->linesize, 0, h, dst, dstStride);
    return true;
}

} // namespace sensorstream
