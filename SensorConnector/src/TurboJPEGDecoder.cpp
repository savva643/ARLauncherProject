#include "TurboJPEGDecoder.h"
#include <QDebug>
#include <turbojpeg.h>

// 🔹 ОБНОВЛЕННЫЙ КЛАСС ЗАДАЧИ С ПОДДЕРЖКОЙ sequenceNumber
class TurboDecodeTask : public QRunnable
{
public:
    TurboDecodeTask(const QByteArray &data, TurboJPEGDecoder *decoder, quint64 sequenceNumber)
        : m_data(data), m_decoder(decoder), m_sequenceNumber(sequenceNumber) // 🔹 ДОБАВЛЕНО: sequenceNumber
    {
        setAutoDelete(true);
    }

    void run() override {
        // 🔹 СОЗДАЕМ ОТДЕЛЬНЫЙ ДЕКОДЕР ДЛЯ КАЖДОГО ПОТОКА
        tjhandle turboHandle = tjInitDecompress();
        if (!turboHandle) {
            qWarning() << "❌ Failed to create TurboJPEG decoder in thread";
            return;
        }

        const unsigned char* jpegBuf = reinterpret_cast<const unsigned char*>(m_data.constData());
        unsigned long jpegSize = m_data.size();

        int width, height, jpegSubsamp, jpegColorspace;

        // 🔹 ПОЛУЧАЕМ РАЗМЕРЫ ИЗОБРАЖЕНИЯ
        if (tjDecompressHeader3(turboHandle, jpegBuf, jpegSize,
                               &width, &height, &jpegSubsamp, &jpegColorspace) != 0) {
            qWarning() << "❌ JPEG header error:" << tjGetErrorStr2(turboHandle);
            tjDestroy(turboHandle);
            return;
        }

        // 🔹 СОЗДАЕМ ИЗОБРАЖЕНИЕ
        QImage image(width, height, QImage::Format_RGB888);
        if (image.isNull()) {
            qWarning() << "❌ Failed to create image buffer" << width << "x" << height;
            tjDestroy(turboHandle);
            return;
        }

        // 🔹 ДЕКОДИРУЕМ
        int result = tjDecompress2(turboHandle, jpegBuf, jpegSize,
                                  image.bits(), width, 0, height, TJPF_RGB,
                                  TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);

        tjDestroy(turboHandle); // 🔹 ВАЖНО: уничтожаем декодер

        if (result == 0 && !image.isNull()) {
            // 🔹 ПЕРЕДАЕМ sequenceNumber В КОЛБЭК
            QMetaObject::invokeMethod(m_decoder, "handleDecodeResult",
                Qt::QueuedConnection,
                Q_ARG(QImage, image),
                Q_ARG(int, m_data.size()),
                Q_ARG(quint64, m_sequenceNumber)); // 🔹 ИСПРАВЛЕНО: передаем sequenceNumber
        } else {
            qWarning() << "❌ TurboJPEG decode failed for frame #" << m_sequenceNumber;
        }
    }

private:
    QByteArray m_data;
    TurboJPEGDecoder *m_decoder;
    quint64 m_sequenceNumber; // 🔹 ДОБАВЛЕНО: хранение номера кадра
};

TurboJPEGDecoder::TurboJPEGDecoder(QObject *parent)
    : QObject(parent)
{
    // 🔹 ПРОВЕРЯЕМ ЧТО TURBOJPEG ДОСТУПЕН
    tjhandle testHandle = tjInitDecompress();
    if (testHandle) {
        m_initialized = true;
        tjDestroy(testHandle);
        qDebug() << "✅ TurboJPEG initialized successfully";
    } else {
        qWarning() << "❌ TurboJPEG init failed";
    }

    // 🔹 НАСТРАИВАЕМ ПУЛ ИЗ 2-4 ПОТОКОВ
    m_decodePool.setMaxThreadCount(4);
}

TurboJPEGDecoder::~TurboJPEGDecoder()
{
    m_decodePool.waitForDone();
}

void TurboJPEGDecoder::decodeJPEGAsync(const QByteArray &jpegData, quint64 sequenceNumber)
{
    if (!m_initialized || jpegData.isEmpty()) {
        qWarning() << "❌ TurboJPEG not initialized or empty data for frame #" << sequenceNumber;
        return;
    }

    // 🔹 ПРОВЕРЯЕМ JPEG СИГНАТУРУ
    if (jpegData.size() < 2 ||
        static_cast<uchar>(jpegData[0]) != 0xFF ||
        static_cast<uchar>(jpegData[1]) != 0xD8) {
        qWarning() << "❌ Invalid JPEG signature for frame #" << sequenceNumber;
        return;
    }

    TurboDecodeTask *task = new TurboDecodeTask(jpegData, this, sequenceNumber);
    m_decodePool.start(task);

}

// 🔹 РЕАЛИЗАЦИЯ МЕТОДА ОБРАБОТКИ РЕЗУЛЬТАТА
void TurboJPEGDecoder::handleDecodeResult(const QImage &image, int dataSize, quint64 sequenceNumber)
{
    if (image.isNull()) {
        qWarning() << "❌ TurboJPEG decode failed for frame #" << sequenceNumber;
        return;
    }

    emit imageDecoded(image, dataSize, sequenceNumber); // 🔹 УБЕДИТЕСЬ ЧТО ПЕРЕДАЕТЕ sequenceNumber
}
