#include "FastJPEGDecoder.h"
#include <QImage>
#include <QDebug>

FastJPEGDecoder::FastJPEGDecoder(QObject *parent)
    : QObject(parent)
    , m_useHardwareAcceleration(true)
{
}

QImage FastJPEGDecoder::decodeJPEG(const QByteArray &jpegData)
{
    // 🔹 СУПЕР БЫСТРЫЙ МЕТОД ДЕКОДИРОВАНИЯ
    QImage image;

    if (m_useHardwareAcceleration) {
        // 🔹 ПРЯМАЯ ЗАГРУЗКА С ОПТИМИЗАЦИЕЙ ДЛЯ ОТОБРАЖЕНИЯ
        image = QImage::fromData(jpegData, "JPEG");

        if (!image.isNull()) {
            // 🔹 КОНВЕРТИРУЕМ В ФОРМАТ СОВМЕСТИМЫЙ С OPENGL
            if (image.format() != QImage::Format_RGB888 &&
                image.format() != QImage::Format_RGB32) {
                image = image.convertToFormat(QImage::Format_RGB888);
            }
        }
    }

    return image;
}
