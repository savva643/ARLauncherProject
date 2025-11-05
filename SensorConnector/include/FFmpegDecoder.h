#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QObject>
#include <QImage>
#include <QByteArray>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class FFmpegDecoder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder();

    bool initialize();
    void cleanup();
    QImage decodeH264Frame(const QByteArray &data);

signals:
    void frameDecoded(const QImage &frame);
    void errorOccurred(const QString &error);

private:
    AVCodecContext *m_codecContext = nullptr;
    AVFrame *m_frame = nullptr;
    SwsContext *m_swsContext = nullptr;
    bool m_initialized = false;

    bool initCodec();
    QImage avFrameToQImage(AVFrame *frame);
    QImage decodeDirect(const QByteArray &data);
    QImage receiveFrame();
};

#endif // FFMPEGDECODER_H
