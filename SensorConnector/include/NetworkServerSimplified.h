#ifndef NETWORKSERVERSIMPLIFIED_H
#define NETWORKSERVERSIMPLIFIED_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QImage>
#include <QMap>
#include <QHash>
#include <QtEndian>
#include "SensorDataTypes.h"
#include "UsbManager.h"
#include "TurboJPEGDecoder.h"
#include "FFmpegDecoder.h"

/**
 * @brief Упрощенный NetworkServer для SensorConnector
 * 
 * Только получает данные и передает их через сигналы.
 * AR обработка должна быть в LensEngineSDK.
 */
class NetworkServerSimplified : public QObject
{
    Q_OBJECT

public:
    explicit NetworkServerSimplified(QObject *parent = nullptr);
    ~NetworkServerSimplified();

    void startServers(quint16 tcpPort, quint16 udpPort);
    void stopServers();

    int clientsCount() const { return m_clientsCount; }
    QString serverStatus() const { return m_serverStatus; }

signals:
    // Статус и статистика
    void statusChanged(const QString &status);
    void clientsCountChanged(int count);
    
    // Сырые данные получены (для передачи в LensEngineSDK)
    void rawDataReceived(SensorConnector::DataType type, const QByteArray &data, quint64 sequenceNumber);
    
    // Декодированные изображения (для предпросмотра)
    void frameDecoded(const QImage &frame, quint64 sequenceNumber);
    void lidarFrameDecoded(const QImage &frame, quint64 sequenceNumber);

private slots:
    // Сетевые слоты
    void handleTcpConnection();
    void handleTcpDisconnection();
    void processTcpData();
    void processUdpData();
    
    // USB слоты
    void handleUsbData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbLidarData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbSensorData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber);
    
    // Декодеры
    void handleTurboImageDecoded(const QImage &image, int dataSize, quint64 sequenceNumber);

private:
    // Протокол обработки данных
    void processRawData(SensorConnector::DataType type, const QByteArray &data, quint64 sequenceNumber);
    void parseDataPacket(const QByteArray &packet, SensorConnector::DataType &type, QByteArray &payload, quint64 &sequenceNumber);
    
    // TCP/UDP серверы
    QTcpServer *m_tcpServer;
    QUdpSocket *m_udpSocket;
    QList<QTcpSocket*> m_tcpClients;
    QMap<QTcpSocket*, QByteArray> m_tcpBuffers;
    
    // USB менеджер
    UsbManager *m_usbManager;
    
    // Декодеры
    TurboJPEGDecoder *m_turboDecoder;
    FFmpegDecoder *m_ffmpegDecoder;
    
    // Состояние
    QString m_serverStatus;
    int m_clientsCount;
    bool m_serversRunning;
    
    // Статистика
    QElapsedTimer m_statsTimer;
    int m_framesCount;
    qint64 m_totalBytes;
    
public:
    int getFramesCount() const { return m_framesCount; }
    qint64 getTotalBytes() const { return m_totalBytes; }
};

#endif // NETWORKSERVERSIMPLIFIED_H

