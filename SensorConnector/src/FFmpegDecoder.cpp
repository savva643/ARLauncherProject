#include "FFmpegDecoder.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

FFmpegDecoder::FFmpegDecoder(QObject *parent)
    : QObject(parent)
    , m_codecContext(nullptr)
    , m_frame(nullptr)
    , m_swsContext(nullptr)
    , m_initialized(false)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
    cleanup();
}

bool FFmpegDecoder::initialize()
{
    if (m_initialized) {
        return true;
    }

    return initCodec();
}

bool FFmpegDecoder::initCodec()
{
    // Находим H.264 декодер
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qWarning() << "H.264 decoder not found";
        emit errorOccurred("H.264 decoder not found");
        return false;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        qWarning() << "Could not allocate codec context";
        emit errorOccurred("Could not allocate codec context");
        return false;
    }

    // 🔹 ПРОСТЫЕ НАСТРОЙКИ ДЛЯ НИЗКОЙ ЗАДЕРЖКИ
    m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecContext->thread_count = 1;

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "flags", "low_delay", 0);
    av_dict_set(&opts, "threads", "1", 0);

    if (avcodec_open2(m_codecContext, codec, &opts) < 0) {
        qWarning() << "Could not open H.264 codec";
        emit errorOccurred("Could not open H.264 codec");
        av_dict_free(&opts);
        return false;
    }
    av_dict_free(&opts);

    m_frame = av_frame_alloc();
    if (!m_frame) {
        qWarning() << "Could not allocate frame";
        emit errorOccurred("Could not allocate frame");
        return false;
    }

    m_initialized = true;
    qDebug() << "FFmpeg H.264 decoder initialized successfully";
    return true;
}

void FFmpegDecoder::cleanup()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    m_initialized = false;
}

QImage FFmpegDecoder::decodeH264Frame(const QByteArray &data)
{
    if (!m_initialized && !initialize()) {
        qWarning() << "Decoder not initialized";
        return QImage();
    }

    if (data.isEmpty()) {
        return QImage();
    }

    // 🔹 ПОДГОТОВКА ПАКЕТА
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        qWarning() << "Could not allocate packet";
        return QImage();
    }

    // Копируем данные в пакет
    packet->data = (uint8_t*)av_malloc(data.size() + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!packet->data) {
        qWarning() << "Could not allocate packet data";
        av_packet_free(&packet);
        return QImage();
    }

    memcpy(packet->data, data.constData(), data.size());
    packet->size = data.size();

    // 🔹 ЗАПОЛНЯЕМ PADDING БУФЕР НУЛЯМИ
    memset(packet->data + packet->size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    // 🔹 ОТПРАВКА ПАКЕТА В ДЕКОДЕР
    int ret = avcodec_send_packet(m_codecContext, packet);
    av_packet_free(&packet);

    if (ret < 0) {
        QString errorMsg;
        switch (ret) {
        case AVERROR(EAGAIN):
            errorMsg = "Decoder needs more data";
            break;
        case AVERROR_EOF:
            errorMsg = "End of stream";
            break;
        case AVERROR(EINVAL):
            errorMsg = "Invalid data";
            break;
        case AVERROR(ENOMEM):
            errorMsg = "Out of memory";
            break;
        default:
            errorMsg = QString("Error %1").arg(ret);
            break;
        }
        qDebug() << "Error sending packet:" << errorMsg;
        return QImage();
    }

    // 🔹 ПОЛУЧЕНИЕ ДЕКОДИРОВАННОГО КАДРА
    ret = avcodec_receive_frame(m_codecContext, m_frame);
    if (ret == AVERROR(EAGAIN)) {
        // Нужно больше данных
        return QImage();
    } else if (ret == AVERROR_EOF) {
        qDebug() << "End of stream";
        return QImage();
    } else if (ret < 0) {
        qDebug() << "Error receiving frame:" << ret;
        return QImage();
    }

    // 🔹 КОНВЕРТАЦИЯ В QImage
    QImage result = avFrameToQImage(m_frame);
    if (!result.isNull()) {
        qDebug() << "✅ Frame decoded:" << result.size();
    }

    return result;
}

QImage FFmpegDecoder::decodeDirect(const QByteArray &data)
{
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        qWarning() << "Could not allocate packet";
        return QImage();
    }

    // Создаем копию данных
    packet->data = (uint8_t*)av_malloc(data.size() + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!packet->data) {
        qWarning() << "Could not allocate packet data";
        av_packet_free(&packet);
        return QImage();
    }

    memcpy(packet->data, data.constData(), data.size());
    packet->size = data.size();

    // 🔹 ОЧИЩАЕМ БУФЕР ДЕКОДЕРА ПЕРЕД НОВЫМИ ДАННЫМИ
    avcodec_flush_buffers(m_codecContext);

    int ret = avcodec_send_packet(m_codecContext, packet);
    av_packet_free(&packet);

    if (ret < 0) {
        QString errorMsg;
        switch (ret) {
        case AVERROR(EAGAIN):
            errorMsg = "Decoder needs more data";
            break;
        case AVERROR_EOF:
            errorMsg = "End of stream";
            break;
        case AVERROR(EINVAL):
            errorMsg = "Invalid data";
            break;
        case AVERROR(ENOMEM):
            errorMsg = "Out of memory";
            break;
        default:
            errorMsg = QString("Unknown error: %1").arg(ret);
            break;
        }
        qDebug() << "Error sending packet:" << errorMsg;
        return QImage();
    }

    return receiveFrame();
}

QImage FFmpegDecoder::receiveFrame()
{
    int ret = avcodec_receive_frame(m_codecContext, m_frame);

    if (ret == AVERROR(EAGAIN)) {
        // Нужно больше данных
        return QImage();
    } else if (ret == AVERROR_EOF) {
        qDebug() << "End of stream";
        return QImage();
    } else if (ret < 0) {
        qDebug() << "Error receiving frame:" << ret;
        return QImage();
    }

    // Успешно декодировали кадр
    QImage result = avFrameToQImage(m_frame);

    if (!result.isNull()) {
        qDebug() << "Frame decoded:" << m_frame->width << "x" << m_frame->height
                 << "format:" << m_frame->format;
    }

    return result;
}

QImage FFmpegDecoder::avFrameToQImage(AVFrame *frame)
{
    if (!frame || !frame->data[0]) {
        qWarning() << "Invalid frame in avFrameToQImage";
        return QImage();
    }

    AVPixelFormat sourceFormat = static_cast<AVPixelFormat>(frame->format);
    AVPixelFormat targetFormat = AV_PIX_FMT_RGB32;

    qDebug() << "Converting frame:" << frame->width << "x" << frame->height
             << "format:" << sourceFormat;

    // Создаем контекст для преобразования цветового пространства
    m_swsContext = sws_getCachedContext(m_swsContext,
                                        frame->width, frame->height, sourceFormat,
                                        frame->width, frame->height, targetFormat,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_swsContext) {
        qWarning() << "Cannot create conversion context";
        return QImage();
    }

    // Создаем QImage для результата
    QImage image(frame->width, frame->height, QImage::Format_RGB32);

    // Подготавливаем данные для преобразования
    uint8_t *destData[1] = { image.bits() };
    int destLinesize[1] = { static_cast<int>(image.bytesPerLine()) };

    // Выполняем преобразование
    int result = sws_scale(m_swsContext,
                           frame->data, frame->linesize, 0,
                           frame->height,
                           destData, destLinesize);

    if (result <= 0) {
        qWarning() << "sws_scale failed:" << result;
        return QImage();
    }

    qDebug() << "Frame converted successfully to QImage";
    return image;
}
