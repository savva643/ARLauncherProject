#include "NetworkServerSimplified.h"
#include "FFmpegDecoder.h"
#include <QDebug>
#include <QNetworkDatagram>
#include <QDateTime>
#include <QTimer>

namespace SensorConnector {

NetworkServerSimplified::NetworkServerSimplified(QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_udpSocket(new QUdpSocket(this))
    , m_usbManager(nullptr)
    , m_turboDecoder(nullptr)
    , m_ffmpegDecoder(nullptr)
    , m_serverStatus("Stopped")
    , m_clientsCount(0)
    , m_serversRunning(false)
    , m_framesCount(0)
    , m_totalBytes(0)
{
    // Инициализация декодеров
    m_turboDecoder = new TurboJPEGDecoder(this);
    connect(m_turboDecoder, &TurboJPEGDecoder::imageDecoded,
            this, &NetworkServerSimplified::handleTurboImageDecoded, Qt::QueuedConnection);
    
    m_ffmpegDecoder = new FFmpegDecoder(this);
    m_ffmpegDecoder->initialize();
    
    // Подключение сетевых сигналов
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &NetworkServerSimplified::handleTcpConnection);
    connect(m_udpSocket, &QUdpSocket::readyRead,
            this, &NetworkServerSimplified::processUdpData);
    
    // Инициализация USB менеджера
    m_usbManager = new UsbManager(this);
    connect(m_usbManager, &UsbManager::usbDataReceived,
            this, &NetworkServerSimplified::handleUsbData, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbLidarDataReceived,
            this, &NetworkServerSimplified::handleUsbLidarData, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbSensorDataReceived,
            this, &NetworkServerSimplified::handleUsbSensorData, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbRawLidarPointCloudReceived,
            this, &NetworkServerSimplified::handleUsbRawLidarPointCloud, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbLidarConfidenceMapReceived,
            this, &NetworkServerSimplified::handleUsbLidarConfidenceMap, Qt::QueuedConnection);
    
    m_statsTimer.start();
    
    // Таймер для обновления статистики
    QTimer* statsUpdateTimer = new QTimer(this);
    connect(statsUpdateTimer, &QTimer::timeout, this, [this]() {
        // Обновление статистики каждую секунду
        // Статистика обновляется автоматически при получении данных
    });
    statsUpdateTimer->start(1000); // Каждую секунду
    
    qDebug() << "✅ NetworkServerSimplified initialized";
}

NetworkServerSimplified::~NetworkServerSimplified()
{
    stopServers();
}

void NetworkServerSimplified::startServers(quint16 tcpPort, quint16 udpPort)
{
    qDebug() << "🚀 Starting simplified servers on TCP:" << tcpPort << "UDP:" << udpPort;
    
    if (m_serversRunning) {
        stopServers();
    }
    
    // Запуск TCP сервера
    if (!m_tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "❌ TCP Server failed:" << m_tcpServer->errorString();
        emit statusChanged("TCP Error: " + m_tcpServer->errorString());
        return;
    }
    
    // Запуск UDP сервера
    if (!m_udpSocket->bind(QHostAddress::Any, udpPort)) {
        qWarning() << "❌ UDP Server failed:" << m_udpSocket->errorString();
        emit statusChanged("UDP Error: " + m_udpSocket->errorString());
        m_tcpServer->close();
        return;
    }
    
    // Запуск USB сервера
    if (m_usbManager) {
        m_usbManager->startUsbServer();
    }
    
    m_serversRunning = true;
    m_serverStatus = QString("Running - TCP:%1 UDP:%2").arg(tcpPort).arg(udpPort);
    emit statusChanged(m_serverStatus);
    
    qDebug() << "✅ Servers started successfully";
}

void NetworkServerSimplified::stopServers()
{
    if (!m_serversRunning) {
        return;
    }
    
    qDebug() << "🛑 Stopping servers...";
    
    // Закрытие TCP соединений
    for (QTcpSocket *client : m_tcpClients) {
        client->close();
    }
    m_tcpClients.clear();
    m_tcpBuffers.clear();
    m_tcpServer->close();
    
    // Закрытие UDP
    m_udpSocket->close();
    
    // Остановка USB
    if (m_usbManager) {
        m_usbManager->stopUsbServer();
    }
    
    m_serversRunning = false;
    m_serverStatus = "Stopped";
    m_clientsCount = 0;
    emit statusChanged(m_serverStatus);
    emit clientsCountChanged(m_clientsCount);
}

void NetworkServerSimplified::handleTcpConnection()
{
    QTcpSocket *client = m_tcpServer->nextPendingConnection();
    if (!client) {
        return;
    }
    
    m_tcpClients.append(client);
    m_tcpBuffers[client] = QByteArray();
    m_clientsCount = m_tcpClients.size();
    emit clientsCountChanged(m_clientsCount);
    
    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
        processTcpData();
    });
    
    connect(client, &QTcpSocket::disconnected, this, [this, client]() {
        m_tcpClients.removeAll(client);
        m_tcpBuffers.remove(client);
        client->deleteLater();
        m_clientsCount = m_tcpClients.size();
        emit clientsCountChanged(m_clientsCount);
    });
    
    qDebug() << "📡 New TCP client connected. Total clients:" << m_clientsCount;
}

void NetworkServerSimplified::handleTcpDisconnection()
{
    // Обрабатывается в lambda выше
}

void NetworkServerSimplified::processTcpData()
{
    for (QTcpSocket *client : m_tcpClients) {
        if (!client->bytesAvailable()) {
            continue;
        }
        
        QByteArray &buffer = m_tcpBuffers[client];
        buffer.append(client->readAll());
        
        // Парсинг пакетов: [Header: 1 byte type][Sequence: 8 bytes][Size: 4 bytes][Data: N bytes]
        while (buffer.size() >= 13) { // Минимум: 1 + 8 + 4 = 13 байт
            quint8 dataType = static_cast<quint8>(buffer[0]);
            quint64 sequenceNumber = 0;
            quint32 dataSize = 0;
            
            // Читаем sequence number (8 bytes, big-endian)
            memcpy(&sequenceNumber, buffer.constData() + 1, 8);
            sequenceNumber = qFromBigEndian(sequenceNumber);
            
            // Читаем размер данных (4 bytes, big-endian)
            memcpy(&dataSize, buffer.constData() + 9, 4);
            dataSize = qFromBigEndian(dataSize);
            
            // Проверяем, что весь пакет получен
            if (buffer.size() < 13 + static_cast<int>(dataSize)) {
                break; // Ждем еще данных
            }
            
            // Извлекаем данные
            QByteArray payload = buffer.mid(13, dataSize);
            buffer.remove(0, 13 + dataSize);
            
            // Определяем тип данных
            SensorConnector::DataType type = static_cast<SensorConnector::DataType>(dataType);
            
            // Обрабатываем данные
            processRawData(type, payload, sequenceNumber);
            
            // Статистика
            m_totalBytes += payload.size();
            m_framesCount++;
        }
    }
}

void NetworkServerSimplified::processUdpData()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        
        if (data.size() < 13) {
            continue;
        }
        
        // Парсинг пакета (тот же формат, что и TCP)
        quint8 dataType = static_cast<quint8>(data[0]);
        quint64 sequenceNumber = 0;
        quint32 dataSize = 0;
        
        memcpy(&sequenceNumber, data.constData() + 1, 8);
        sequenceNumber = qFromBigEndian(sequenceNumber);
        
        memcpy(&dataSize, data.constData() + 9, 4);
        dataSize = qFromBigEndian(dataSize);
        
        if (data.size() < 13 + static_cast<int>(dataSize)) {
            continue;
        }
        
        QByteArray payload = data.mid(13, dataSize);
        SensorConnector::DataType type = static_cast<SensorConnector::DataType>(dataType);
        
        processRawData(type, payload, sequenceNumber);
        
        // Статистика
        m_totalBytes += payload.size();
        m_framesCount++;
    }
}

void NetworkServerSimplified::handleUsbData(const QByteArray &data, quint64 sequenceNumber)
{
    processRawData(SensorConnector::RGB_CAMERA, data, sequenceNumber);
}

void NetworkServerSimplified::handleUsbLidarData(const QByteArray &data, quint64 sequenceNumber)
{
    processRawData(SensorConnector::LIDAR_DEPTH, data, sequenceNumber);
}

void NetworkServerSimplified::handleUsbSensorData(const QByteArray &data, quint64 sequenceNumber)
{
    processRawData(SensorConnector::RAW_IMU, data, sequenceNumber);
}

void NetworkServerSimplified::handleUsbRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber)
{
    // Тип 0x08 - Raw LiDAR Point Cloud
    processRawData(SensorConnector::LIDAR_DEPTH, data, sequenceNumber); // Используем LIDAR_DEPTH как базовый тип
}

void NetworkServerSimplified::handleUsbLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber)
{
    // Тип 0x09 - LiDAR Confidence Map
    processRawData(SensorConnector::LIDAR_DEPTH, data, sequenceNumber); // Используем LIDAR_DEPTH как базовый тип
}

void NetworkServerSimplified::handleTurboImageDecoded(const QImage &image, int dataSize, quint64 sequenceNumber)
{
    if (!image.isNull()) {
        emit frameDecoded(image, sequenceNumber);
    }
}

void NetworkServerSimplified::processRawData(SensorConnector::DataType type, const QByteArray &data, quint64 sequenceNumber)
{
    // Отправляем сырые данные для обработки в LensEngineSDK
    emit rawDataReceived(type, data, sequenceNumber);
    
    // Для RGB данных также декодируем для предпросмотра
    if (type == SensorConnector::RGB_CAMERA) {
        // Проверяем, что это JPEG
        if (data.size() >= 2 && static_cast<uchar>(data[0]) == 0xFF && static_cast<uchar>(data[1]) == 0xD8) {
            if (m_turboDecoder) {
                m_turboDecoder->decodeJPEGAsync(data, sequenceNumber);
            }
        }
    }
    
    // Для LiDAR данных также декодируем для предпросмотра
    if (type == SensorConnector::LIDAR_DEPTH) {
        // LiDAR данные уже в формате глубины (float array)
        // Можно создать визуализацию для предпросмотра
        if (data.size() >= 256 * 192 * 4) { // 256x192 float
            // TODO: Создать QImage для визуализации глубины
        }
    }
}

} // namespace SensorConnector

