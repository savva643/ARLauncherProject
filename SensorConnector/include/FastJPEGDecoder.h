#ifndef FASTJPEGDECODER_H
#define FASTJPEGDECODER_H


#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class FastJPEGDecoder : public QObject
{
    Q_OBJECT
public:
    explicit FastJPEGDecoder(QObject *parent = nullptr);
    QImage decodeJPEG(const QByteArray &jpegData);

private:
    bool m_useHardwareAcceleration;
};
#endif // FASTJPEGDECODER_H
