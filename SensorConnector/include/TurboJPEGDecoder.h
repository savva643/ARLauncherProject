#ifndef TURBOJPEGDECODER_H
#define TURBOJPEGDECODER_H

#include <QImage>
#include <QByteArray>
#include <QObject>
#include <QThreadPool>
#include <QRunnable>

class TurboJPEGDecoder : public QObject
{
    Q_OBJECT
public:
    explicit TurboJPEGDecoder(QObject *parent = nullptr);
    ~TurboJPEGDecoder();

    bool isAvailable() const { return m_initialized; }

    void decodeJPEGAsync(const QByteArray &jpegData, quint64 sequenceNumber);

signals:
    void imageDecoded(const QImage &image, int dataSize, quint64 sequenceNumber); // 🔹 ИЗМЕНИЛОСЬ: добавлен sequenceNumber

private slots:
    void handleDecodeResult(const QImage &image, int dataSize, quint64 sequenceNumber); // 🔹 ИСПРАВЛЕНО: правильное имя

private:
    bool m_initialized = false;
    QThreadPool m_decodePool;
};

#endif // TURBOJPEGDECODER_H
