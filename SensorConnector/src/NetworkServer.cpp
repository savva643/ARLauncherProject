#include "NetworkServer.h"
#include "ffmpegdecoder.h"
#include <cmath>
#include <QBuffer>
#include <QImageReader>
#include <QDebug>
#include <QNetworkInterface>
#include <QNetworkDatagram>
#include <QPainter>
#include <QThreadPool>
#include <QtConcurrent>

// 🔹 ДОБАВЛЯЕМ ГЛОБАЛЬНЫЙ THREAD POOL ДЛЯ ОБРАБОТКИ
static QThreadPool* arThreadPool = nullptr;

NetworkServer::NetworkServer(QObject *parent)
    : QObject(parent)
    , tcpServer(new QTcpServer(this))
    , udpSocket(new QUdpSocket(this))
    , serversRunning(false)
    , m_clientsCount(0)
    , m_fps(0)
    , m_speed(0.0)
    , m_lidarFps(0)
    , m_lidarSpeed(0.0)
    , currentConnectionType(TCP)
    , m_currentConnectionTypeStr("TCP (JPEG)")
    , totalBytes(0)
    , lidarTotalBytes(0)
    , framesCount(0)
    , lidarFramesCount(0)
    , lastRgbSequence(0)
    , lastLidarSequence(0)
{
    // 🔹 ИНИЦИАЛИЗАЦИЯ THREAD POOL
    if (!arThreadPool) {
        arThreadPool = new QThreadPool(this);
        arThreadPool->setMaxThreadCount(8); // Оптимально для AR обработки
        qDebug() << "🎯 AR Thread Pool initialized with" << arThreadPool->maxThreadCount() << "threads";
    }

    // 🔹 ИНИЦИАЛИЗАЦИЯ ИЗОБРАЖЕНИЙ ДЛЯ LiDAR
    lidarDepthImage = QImage(LIDAR_TARGET_WIDTH, LIDAR_TARGET_HEIGHT, QImage::Format_Grayscale8);
    lidarFallbackImage = QImage(LIDAR_TARGET_WIDTH, LIDAR_TARGET_HEIGHT, QImage::Format_RGB888);
    lidarFallbackImage.fill(QColor(0, 50, 100));

    // 🔹 СОЗДАЕМ ОТДЕЛЬНЫЕ ПОТОКИ ДЛЯ КРИТИЧЕСКИХ КОМПОНЕНТОВ
    m_imuThread = new QThread(this);
    m_lidarThread = new QThread(this);
    m_arProcessingThread = new QThread(this);

    // 🔹 ИНИЦИАЛИЗАЦИЯ AR КОМПОНЕНТОВ В ПРАВИЛЬНОМ ПОРЯДКЕ
    m_arCameraController = new ARCameraController(this);

    // 🔹 LiDAR ПРОЦЕССОР ПЕРЕМЕЩАЕМ В ОТДЕЛЬНЫЙ ПОТОК
    m_lidar3DProcessor = new Lidar3DProcessor(this);
    m_lidar3DProcessor->moveToThread(m_lidarThread);

    // 🔹 AR DATA PROCESSOR ПЕРЕМЕЩАЕМ В ОТДЕЛЬНЫЙ ПОТОК
    m_arDataProcessor = new ARDataProcessor();
    m_arDataProcessor->setCameraController(m_arCameraController);
    m_arDataProcessor->moveToThread(m_arProcessingThread);

    // 🔹 ЗАПУСКАЕМ ПОТОКИ
    m_imuThread->start();
    m_lidarThread->start();
    m_arProcessingThread->start();

    // 🔹 ПОДКЛЮЧАЕМ СИГНАЛЫ ПОСЛЕ ПЕРЕМЕЩЕНИЯ В ПОТОКИ
    connect(m_arCameraController, &ARCameraController::cameraUpdated,
            this, &NetworkServer::cameraPoseUpdated);

    // 🔹 ПОДКЛЮЧАЕМ СИГНАЛЫ LiDAR
    connect(m_lidar3DProcessor, &Lidar3DProcessor::spatialAnalysisCompleted,
            this, &NetworkServer::onLidarSpatialAnalysisCompleted, Qt::QueuedConnection);
    connect(m_lidar3DProcessor, &Lidar3DProcessor::lidarPointsProcessed,
            this, &NetworkServer::onLidarPointsProcessed, Qt::QueuedConnection);
    connect(m_lidar3DProcessor, &Lidar3DProcessor::floorDetected,
            this, &NetworkServer::onFloorDetected, Qt::QueuedConnection);
    connect(m_lidar3DProcessor, &Lidar3DProcessor::obstaclesDetected,
            this, &NetworkServer::onObstaclesDetected, Qt::QueuedConnection);

    // 🔹 ПОДКЛЮЧАЕМ СИГНАЛЫ ОТ ARDataProcessor
    connect(m_arDataProcessor, &ARDataProcessor::frameProcessed,
            this, &NetworkServer::onFrameProcessed, Qt::QueuedConnection);
    connect(m_arDataProcessor, &ARDataProcessor::featurePointsUpdated,
            this, &NetworkServer::onFeaturePointsUpdated, Qt::QueuedConnection);
    connect(m_arDataProcessor, &ARDataProcessor::cameraIntrinsicsUpdated,
            this, &NetworkServer::onCameraIntrinsicsUpdated, Qt::QueuedConnection);
    connect(m_arDataProcessor, &ARDataProcessor::lightEstimationUpdated,
            this, &NetworkServer::onLightEstimationUpdated, Qt::QueuedConnection);
    connect(m_arDataProcessor, &ARDataProcessor::cameraPoseUpdated,
            this, &NetworkServer::onCameraPoseUpdated, Qt::QueuedConnection);
    connect(m_arDataProcessor, &ARDataProcessor::sensorDataUpdated,
            this, &NetworkServer::onSensorDataUpdated, Qt::QueuedConnection);

    // 🔹 ИНИЦИАЛИЗАЦИЯ ДЕКОДЕРОВ
    m_turboDecoder = new TurboJPEGDecoder(this);
    connect(m_turboDecoder, &TurboJPEGDecoder::imageDecoded,
            this, &NetworkServer::handleTurboImageDecoded, Qt::QueuedConnection);

    m_ffmpegDecoder = new FFmpegDecoder(this);
    connect(m_ffmpegDecoder, &FFmpegDecoder::frameDecoded,
            this, &NetworkServer::frameReceived, Qt::QueuedConnection);

    // 🔹 ИНИЦИАЛИЗАЦИЯ USB МЕНЕДЖЕРА
    m_usbManager = new UsbManager(this);
    connect(m_usbManager, &UsbManager::usbDataReceived,
            this, &NetworkServer::handleUsbData, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbLidarDataReceived,
            this, &NetworkServer::handleUsbLidarData, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbSensorDataReceived,
            this, &NetworkServer::handleUsbSensorData, Qt::QueuedConnection);

    // 🔹 ПОДКЛЮЧАЕМ НОВЫЕ USB СИГНАЛЫ
    connect(m_usbManager, &UsbManager::usbRawLidarPointCloudReceived,
            this, &NetworkServer::handleUsbRawLidarPointCloud, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbLidarConfidenceMapReceived,
            this, &NetworkServer::handleUsbLidarConfidenceMap, Qt::QueuedConnection);

    connect(m_usbManager, &UsbManager::usbClientConnected,
            this, &NetworkServer::handleUsbClientConnected, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbClientDisconnected,
            this, &NetworkServer::handleUsbClientDisconnected, Qt::QueuedConnection);
    connect(m_usbManager, &UsbManager::usbStatusChanged,
            this, &NetworkServer::handleUsbStatusChanged, Qt::QueuedConnection);

    // 🔹 ПОДКЛЮЧЕНИЕ СЕТЕВЫХ СИГНАЛОВ
    connect(tcpServer, &QTcpServer::newConnection,
            this, &NetworkServer::handleTcpConnection);
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &NetworkServer::processUdpData);

    // 🔹 ИНИЦИАЛИЗАЦИЯ ТАЙМЕРОВ
    displayTimer = new QTimer(this);
    connect(displayTimer, &QTimer::timeout, this, &NetworkServer::updateDisplay);
    displayTimer->start(16); // ~60 FPS

    cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &NetworkServer::cleanupBuffers);
    cleanupTimer->start(1000);

    // 🔹 ИНИЦИАЛИЗАЦИЯ ТАБЛИЦ ПРЕОБРАЗОВАНИЯ ДЛЯ LiDAR
    initLookupTables();

    qDebug() << "🎯 NetworkServer initialized with multi-threaded architecture";
}


NetworkServer::~NetworkServer()
{
    // 🔹 КОРРЕКТНОЕ ЗАВЕРШЕНИЕ ПОТОКОВ
    if (m_imuThread) {
        m_imuThread->quit();
        m_imuThread->wait();
    }
    if (m_lidarThread) {
        m_lidarThread->quit();
        m_lidarThread->wait();
    }
    if (m_arProcessingThread) {
        m_arProcessingThread->quit();
        m_arProcessingThread->wait();
    }

    stopServers();
}



void NetworkServer::startServers(quint16 tcpPort, quint16 udpPort) {
    qDebug() << "🔄 Starting servers on TCP port:" << tcpPort << "UDP port:" << udpPort;

        if (serversRunning) {
        stopServers();
    }

    // Запуск TCP сервера для видео
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "⚠️ TCP Server error:" << tcpServer->errorString();
                                                        emit statusChanged("TCP Error: " + tcpServer->errorString());
        serversRunning = false;
        return;
    } else {
        qInfo() << "✅ TCP Server started on port" << tcpPort;
                emit statusChanged("TCP Server ready on port " + QString::number(tcpPort));
    }

    // Запуск UDP сервера для видео
    if (!udpSocket->bind(QHostAddress::Any, udpPort)) {
        qWarning() << "⚠️ UDP Server error:" << udpSocket->errorString();
                                                        emit statusChanged("UDP Error: " + udpSocket->errorString());
        tcpServer->close();
        serversRunning = false;
        return;
    } else {
        qInfo() << "✅ UDP Server started on port" << udpPort;
                emit statusChanged("UDP Server ready on port " + QString::number(udpPort));
    }

    m_usbManager->startUsbServer();
    serversRunning = true;

    // Получаем локальный IP
    QString ipAddress;
    const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : ipAddressesList) {
        if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
            if (address.toString().startsWith("169.254")) {
                ipAddress = address.toString();
                break;
            }
            ipAddress = address.toString();
        }
    }
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    emit statusChanged("Servers started. Local IP: " + ipAddress + " | USB: " + UsbManager::usbHostIP);
}

void NetworkServer::stopServers() {
    qDebug() << "🛑 Stopping servers...";

    tcpServer->close();
    udpSocket->close();

    if (m_usbManager) {
        m_usbManager->stopUsbServer();
    }

    // Очищаем TCP клиенты видео
    for (auto *client : tcpClients) {
        client->close();
        client->deleteLater();
    }
    tcpClients.clear();
    tcpBuffers.clear();

    serversRunning = false;

    framesCount = 0;
    totalBytes = 0;

    // 🔹 ОЧИСТКА БУФЕРОВ ПРИ ОСТАНОВКЕ
    rgbFrameBuffer.clear();
    lidarFrameBuffer.clear();
    rgbFrameTimestamps.clear();
    lidarFrameTimestamps.clear();

    QImage emptyImage;
    emit frameReceived(emptyImage);

    emit statusChanged("Servers stopped");
    emit clientsCountChanged(0);
}




// 🔹 ОПТИМИЗИРОВАННЫЙ МЕТОД ОБНОВЛЕНИЯ ДИСПЛЕЯ
void NetworkServer::updateDisplay()
{
    static quint64 lastProcessedRgb = 0;
    static quint64 lastProcessedLidar = 0;

    // 🔹 ОБРАБОТКА RGB - ПРИОРИТЕТ ВЫСОКИЙ
    if (!rgbFrameBuffer.isEmpty()) {
        quint64 latestRgb = rgbFrameBuffer.keys().last();
        if (latestRgb > lastProcessedRgb) {
            QImage rgbFrame = rgbFrameBuffer[latestRgb];
            if (!rgbFrame.isNull()) {
                emit frameReceived(rgbFrame);
                lastRgbSequence = latestRgb;
                lastProcessedRgb = latestRgb;
            }
        }
    }

    // 🔹 ОБРАБОТКА LiDAR - ПРИОРИТЕТ НИЖЕ
    if (!lidarFrameBuffer.isEmpty()) {
        quint64 latestLidar = lidarFrameBuffer.keys().last();
        if (latestLidar > lastProcessedLidar) {
            QImage lidarFrame = lidarFrameBuffer[latestLidar];
            if (!lidarFrame.isNull()) {
                emit lidarFrameReceived(lidarFrame);
                lastLidarSequence = latestLidar;
                lastProcessedLidar = latestLidar;
            }
        }
    }

    // 🔹 ОЧИСТКА БУФЕРОВ (сохраняем только последние 3 кадра для экономии памяти)
    cleanupExcessFrames(rgbFrameBuffer, rgbFrameTimestamps, 3);
    cleanupExcessFrames(lidarFrameBuffer, lidarFrameTimestamps, 3);
}
// 🔹 ВСПОМОГАТЕЛЬНЫЙ МЕТОД ДЛЯ ОЧИСТКИ БУФЕРОВ (ИСПРАВЛЕННАЯ СИГНАТУРА)
void NetworkServer::cleanupExcessFrames(QMap<quint64, QImage> &frameBuffer,
                                        QHash<quint64, qint64> &timestamps,
                                        int maxFrames)
{
    if (frameBuffer.size() > maxFrames) {
        auto keys = frameBuffer.keys();
        while (keys.size() > maxFrames) {
            frameBuffer.remove(keys.first());
            timestamps.remove(keys.first());
            keys.removeFirst();
        }
    }
}

// 🔹 ПОИСК БЛИЖАЙШЕГО LiDAR КАДРА К КАДРУ КАМЕРЫ
quint64 NetworkServer::findBestLidarMatch(quint64 rgbSequence)
{
    if (lidarFrameBuffer.isEmpty()) return 0;

    QList<quint64> lidarKeys = lidarFrameBuffer.keys();
    quint64 bestMatch = 0;
    quint64 minDiff = ULLONG_MAX;

    for (quint64 lidarSeq : lidarKeys) {
        quint64 diff = (lidarSeq > rgbSequence) ? (lidarSeq - rgbSequence) : (rgbSequence - lidarSeq);
        if (diff < minDiff && diff <= 5) { // 🔹 ДОПУСТИМАЯ РАЗНИЦА В 5 КАДРОВ
            minDiff = diff;
            bestMatch = lidarSeq;
        }
    }

    return bestMatch;
}

// 🔹 ОЧИСТКА БУФЕРОВ ОТ СТАРЫХ КАДРОВ
void NetworkServer::cleanupBuffers()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 🔹 ОЧИСТКА RGB БУФЕРА (кадры старше 3 секунд)
    QList<quint64> rgbKeys = rgbFrameBuffer.keys();
    for (quint64 key : rgbKeys) {
        if (currentTime - rgbFrameTimestamps[key] > 3000) {
            rgbFrameBuffer.remove(key);
            rgbFrameTimestamps.remove(key);
        }
    }

    // 🔹 ОЧИСТКА LiDAR БУФЕРА (кадры старше 3 секунд)
    QList<quint64> lidarKeys = lidarFrameBuffer.keys();
    for (quint64 key : lidarKeys) {
        if (currentTime - lidarFrameTimestamps[key] > 3000) {
            lidarFrameBuffer.remove(key);
            lidarFrameTimestamps.remove(key);
        }
    }

}



void NetworkServer::handleTurboImageDecoded(const QImage &image, int dataSize, quint64 sequenceNumber)
{
    if (!image.isNull()) {
        // 🔹 СОХРАНЯЕМ В БУФЕР КАМЕРЫ
        rgbFrameBuffer[sequenceNumber] = image;
        rgbFrameTimestamps[sequenceNumber] = QDateTime::currentMSecsSinceEpoch();

        // 🔹 СОЗДАЕМ ПОЛНЫЙ ARFrame С RGB ИЗОБРАЖЕНИЕМ
        LensEngine::ARFrame rgbFrame;
        rgbFrame.rgbImage = image;
        rgbFrame.sequenceNumber = sequenceNumber;
        rgbFrame.timestamp = QDateTime::currentMSecsSinceEpoch();

        // 🔹 ЗАПУСКАЕМ AR ОБРАБОТКУ ДЛЯ RGB КАДРА
        QtConcurrent::run(arThreadPool, [this, rgbFrame]() {
            m_arDataProcessor->processFrameAsync(rgbFrame);
        });

        // 🔹 ЭМИТИМ СИГНАЛ ДЛЯ НЕМЕДЛЕННОГО ОБНОВЛЕНИЯ
        emit frameReceived(image);
        updateStatisticsFast(dataSize);

        static int rgbDecodedCounter = 0;
        if (rgbDecodedCounter++ % 60 == 0) {
            qDebug() << "🎯 [RGB] Frame decoded and sent to AR processor - Seq:" << sequenceNumber;
        }
    }
}

void NetworkServer::deliverFrame(const QImage &img, int dataSize)
{
    emit frameReceived(img);
    updateStatisticsFast(dataSize);
}

// 🔹 ОПТИМИЗИРОВАННАЯ СТАТИСТИКА
void NetworkServer::updateStatisticsFast(int dataSize)
{
    totalBytes += dataSize;
    framesCount++;

    static QElapsedTimer statTimer;
    static bool timerStarted = false;

    if (!timerStarted) {
        statTimer.start();
        timerStarted = true;
    }

    if (statTimer.elapsed() >= 1000) {
        m_speed = (totalBytes / 1024.0) / (statTimer.elapsed() / 1000.0);
        m_fps = framesCount;

        emit debugUpdate(framesCount, m_speed, m_currentConnectionTypeStr);

        framesCount = 0;
        totalBytes = 0;
        statTimer.restart();
    }
}

// 🔹 ОБРАБОТКА TCP ПОДКЛЮЧЕНИЙ
void NetworkServer::handleTcpConnection() {
    QTcpSocket *socket = tcpServer->nextPendingConnection();
    tcpClients.append(socket);
    tcpBuffers[socket] = QByteArray();

    qInfo() << "📡 New TCP connection from:" << socket->peerAddress().toString();
    emit statusChanged("TCP client connected: " + socket->peerAddress().toString());
    m_clientsCount = tcpClients.size();
    emit clientsCountChanged(m_clientsCount);

    connect(socket, &QTcpSocket::readyRead, this, &NetworkServer::processTcpData);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkServer::handleTcpDisconnection);
}

void NetworkServer::handleTcpDisconnection() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        tcpClients.removeAll(socket);
        tcpBuffers.remove(socket);
        qInfo() << "❌ TCP client disconnected";
        emit statusChanged("TCP client disconnected");
        m_clientsCount = tcpClients.size();
        emit clientsCountChanged(m_clientsCount);
        socket->deleteLater();
    }
}

void NetworkServer::processTcpData() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray &buffer = tcpBuffers[socket];
    buffer.append(socket->readAll());

    while (buffer.size() >= 13) { // 🔹 1 (type) + 8 (sequence) + 4 (size)
        uchar dataType = static_cast<uchar>(buffer[0]);
        quint64 sequenceNumber = qFromBigEndian(*reinterpret_cast<const quint64*>(buffer.constData() + 1));
        quint32 dataSize = qFromBigEndian(*reinterpret_cast<const quint32*>(buffer.constData() + 9));

        if (dataSize > 1000000) {
            qWarning() << "❌ Suspicious data size:" << dataSize;
                                                            buffer.clear();
            return;
        }

        if (buffer.size() < 13 + (int)dataSize) {
            return;
        }

        QByteArray payload = QByteArray::fromRawData(buffer.constData() + 13, dataSize);

        // 🔹 ОБРАБОТКА РАЗНЫХ ТИПОВ ДАННЫХ
        switch (dataType) {
        case 0x01: // 🔹 Основной RGB
            processRGBData(payload, sequenceNumber);
            break;

        case 0x02: // 🔹 LiDAR Depth
            processLidarDepthData(payload, sequenceNumber);
            break;

        case 0x03: // 🔹 Raw IMU
            processRawIMUData(payload, sequenceNumber);
            break;

        case 0x08: // 🔹 Raw LiDAR Point Cloud
            processRawLidarPointCloud(payload, sequenceNumber);
            break;

        case 0x09: // 🔹 LiDAR Confidence Map
            processLidarConfidenceMap(payload, sequenceNumber);
            break;

        case 0x0A: // 🔹 Device Orientation
            processDeviceOrientation(payload, sequenceNumber);
            break;

        case 0x10: // 🔹 Camera Intrinsics
            processCameraIntrinsics(payload, sequenceNumber);
            break;

        default:
            qWarning() << "❌ Unknown data type:" << dataType;
                break;
        }

        buffer.remove(0, 13 + dataSize);
    }
}

// 🔹 ОБРАБОТКА UDP ДАННЫХ (ВСЕ ТИПЫ ДАННЫХ ЧЕРЕЗ ОДИН ПОРТ)
void NetworkServer::processUdpData() {
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();

        if (data.size() < 13) continue; // 🔹 1 (type) + 8 (sequence) + 4 (size)

        uchar dataType = static_cast<uchar>(data[0]);
        quint64 sequenceNumber = qFromBigEndian(*reinterpret_cast<const quint64*>(data.constData() + 1));
        quint32 dataSize = qFromBigEndian(*reinterpret_cast<const quint32*>(data.constData() + 9));

        if (data.size() >= 13 + (int)dataSize) {
            QByteArray payload = QByteArray::fromRawData(data.constData() + 13, dataSize);

            // 🔹 ОБРАБОТКА РАЗНЫХ ТИПОВ ДАННЫХ
            switch (dataType) {
            case 0x01: // 🔹 Основной RGB
                    processRGBData(payload, sequenceNumber);
                    break;

            case 0x02: // 🔹 LiDAR Depth
                    processLidarDepthData(payload, sequenceNumber);
                    break;

            case 0x03: // 🔹 Raw IMU
                    processRawIMUData(payload, sequenceNumber);
                    break;

            case 0x08: // 🔹 Raw LiDAR Point Cloud
                    processRawLidarPointCloud(payload, sequenceNumber);
                    break;

            case 0x09: // 🔹 LiDAR Confidence Map
                    processLidarConfidenceMap(payload, sequenceNumber);
                    break;

            case 0x0A: // 🔹 Device Orientation
                    processDeviceOrientation(payload, sequenceNumber);
                    break;

            default:
                    qWarning() << "❌ Unknown data type:" << dataType;
                        break;
            }
        }
    }
}




void NetworkServer::updateStatistics(int dataSize)
{
    totalBytes += dataSize;
    framesCount++;

    qint64 elapsed = fpsTimer.elapsed(); // 🔹 ИСПРАВЛЕНО
    if (elapsed >= 1000) {
        double kbps = (totalBytes / 1024.0) / (elapsed / 1000.0);
        emit debugUpdate(framesCount, kbps, m_currentConnectionTypeStr);
        framesCount = 0;
        totalBytes = 0;
        fpsTimer.restart(); // 🔹 ИСПРАВЛЕНО
    }
}




// 🔹 ОПТИМИЗИРОВАННЫЙ ПОИСК МАКСИМАЛЬНОЙ ГЛУБИНЫ
float NetworkServer::findMaxDepthFast(const float* depthData)
{
    float maxDepth = 0.0f;
    const int totalPixels = LIDAR_WIDTH * LIDAR_HEIGHT;

    // 🔹 ИСПОЛЬЗУЕМ SIMD-ПОДОБНЫЙ ПОДХОД
    const float* end = depthData + totalPixels;
    const float* ptr = depthData;

    while (ptr < end - 8) {
            // Обрабатываем 8 значений за раз
            float max1 = qMax(qMax(ptr[0], ptr[1]), qMax(ptr[2], ptr[3]));
            float max2 = qMax(qMax(ptr[4], ptr[5]), qMax(ptr[6], ptr[7]));
            float batchMax = qMax(max1, max2);

            if (batchMax > maxDepth && batchMax < 10.0f) {
            maxDepth = batchMax;
            }
            ptr += 8;
    }

    // Обрабатываем оставшиеся значения
    while (ptr < end) {
            if (*ptr > maxDepth && *ptr < 10.0f) {
            maxDepth = *ptr;
            }
            ptr++;
    }

    return maxDepth;
}

// 🔹 СУПЕР-ОПТИМИЗИРОВАННОЕ ПРЕОБРАЗОВАНИЕ В ИЗОБРАЖЕНИЕ
void NetworkServer::convertDepthToImageFast(const float* depthData, QImage &image, float scale)
{
    uchar* bits = image.bits();
    const int totalPixels = LIDAR_TARGET_WIDTH * LIDAR_TARGET_HEIGHT;

    // 🔹 ИСПОЛЬЗУЕМ ПРЕДВАРИТЕЛЬНО ВЫЧИСЛЕННЫЕ ИНДЕКСЫ
    for (int i = 0; i < totalPixels; i++) {
            const int srcIndex = srcIndexTable[i];
            const float depth = depthData[srcIndex];

            bits[i] = (depth > 0 && depth < 10.0f)
                          ? (255 - static_cast<uchar>(depth * scale))
                          : 0;
    }
}


void NetworkServer::updateLidarStatistics(int dataSize)
{
    lidarTotalBytes += dataSize;
    lidarFramesCount++;

    static QElapsedTimer lidarStatTimer;
    static bool lidarTimerStarted = false;

    if (!lidarTimerStarted) {
            lidarStatTimer.start();
            lidarTimerStarted = true;
    }

    if (lidarStatTimer.elapsed() >= 1000) {
            m_lidarSpeed = (lidarTotalBytes / 1024.0) / (lidarStatTimer.elapsed() / 1000.0);
            m_lidarFps = lidarFramesCount;

            emit lidarDebugUpdate(m_lidarFps, m_lidarSpeed);

            lidarFramesCount = 0;
            lidarTotalBytes = 0;
            lidarStatTimer.restart();
    }
}

// 🔹 ОСТАЛЬНЫЕ МЕТОДЫ (без изменений)
void NetworkServer::setConnectionType(int type) {
    qDebug() << "🔧 Setting connection type:" << type;
    currentConnectionType = static_cast<ConnectionType>(type);

    switch (currentConnectionType) {
    case TCP:
        m_currentConnectionTypeStr = "TCP (JPEG)";
        break;
    case UDP:
        m_currentConnectionTypeStr = "UDP (H.264)";
        break;
    case USB:
        m_currentConnectionTypeStr = "USB";
        break;
    }

    qDebug() << "Connection type set to:" << m_currentConnectionTypeStr;
    emit statusChanged("Connection type set to: " + m_currentConnectionTypeStr);
    emit debugUpdate(framesCount, 0.0, m_currentConnectionTypeStr);
}







// 🔹 МЕТОД ДЛЯ СОЗДАНИЯ КАДРА С ОШИБКОЙ
void NetworkServer::createErrorFrame(const QByteArray &data, const QString &error)
{
    QImage errorImg(640, 480, QImage::Format_RGB888);
    errorImg.fill(QColor(60, 60, 60));

    QPainter painter(&errorImg);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 14));

    QString infoText = QString(
        "%1\n"
        "Data size: %2 bytes\n"
        "Connection: USB\n"
        "Time: %3"
    ).arg(error).arg(data.size()).arg(QTime::currentTime().toString("hh:mm:ss.zzz"));

    painter.drawText(errorImg.rect().adjusted(20, 20, -20, -20), Qt::AlignTop, infoText);

    // Показываем первые байты для отладки
    if (!data.isEmpty()) {
        painter.setFont(QFont("Arial", 10));
        QString hexPreview;
        int bytesToShow = qMin(16, data.size());
        for (int i = 0; i < bytesToShow; i++) {
            hexPreview += QString("%1 ").arg(static_cast<uchar>(data[i]), 2, 16, QChar('0'));
        }
        painter.drawText(errorImg.rect().adjusted(20, 150, -20, -20),
                        "Hex: " + hexPreview + "...");
    }

    emit frameReceived(errorImg);
}

void NetworkServer::handleUsbClientConnected()
{
    qInfo() << "🔌 USB client connected";
    emit statusChanged("USB client connected");

    // Обновляем счетчик клиентов для USB режима
    if (currentConnectionType == USB) {
        m_clientsCount = 1;
        emit clientsCountChanged(m_clientsCount);
    }
}

void NetworkServer::handleUsbClientDisconnected()
{
    qInfo() << "🔌 USB client disconnected";
    emit statusChanged("USB client disconnected");

    // Обновляем счетчик клиентов для USB режима
    if (currentConnectionType == USB) {
        m_clientsCount = 0;
        emit clientsCountChanged(m_clientsCount);
    }
}

void NetworkServer::handleUsbStatusChanged(const QString &status)
{
    qDebug() << "🔌 USB status:" << status;
            emit statusChanged("USB: " + status);
}

// 🔹 ДОПОЛНИТЕЛЬНО: МЕТОД ДЛЯ ПРОВЕРКИ JPEG
bool NetworkServer::isLikelyJpegData(const QByteArray &data)
{
    if (data.size() < 4) return false;

    uchar b0 = static_cast<uchar>(data[0]);
    uchar b1 = static_cast<uchar>(data[1]);
    uchar b2 = static_cast<uchar>(data[2]);

    // Стандартная JPEG сигнатура: FF D8 FF
    bool hasJpegSignature = (b0 == 0xFF && b1 == 0xD8 && b2 == 0xFF);

    // Дополнительные проверки для надежности
    bool reasonableSize = (data.size() > 1000 && data.size() < 20 * 1024 * 1024);

    // Проверяем конечный маркер JPEG (FF D9)
    bool hasJpegEnd = false;
    if (data.size() >= 2) {
        uchar end0 = static_cast<uchar>(data[data.size() - 2]);
        uchar end1 = static_cast<uchar>(data[data.size() - 1]);
        hasJpegEnd = (end0 == 0xFF && end1 == 0xD9);
    }

    qDebug() << "🔍 JPEG Check - Signature:" << hasJpegSignature
             << "Size:" << data.size() << "End:" << hasJpegEnd
             << "First bytes:" << QString("%1 %2 %3 %4")
                                      .arg(b0, 2, 16, QChar('0'))
                                      .arg(b1, 2, 16, QChar('0'))
                                      .arg(b2, 2, 16, QChar('0'))
                                      .arg(static_cast<uchar>(data[3]), 2, 16, QChar('0'));

    return hasJpegSignature && reasonableSize;
}


// 🔹 ИНИЦИАЛИЗАЦИЯ ТАБЛИЦ ПРЕОБРАЗОВАНИЯ ДЛЯ УСКОРЕНИЯ
void NetworkServer::initLookupTables()
{
    srcXTable.resize(LIDAR_TARGET_WIDTH);
    srcYTable.resize(LIDAR_TARGET_HEIGHT);
    srcIndexTable.resize(LIDAR_TARGET_WIDTH * LIDAR_TARGET_HEIGHT);

    // Предварительно вычисляем координаты для масштабирования
    for (int y = 0; y < LIDAR_TARGET_HEIGHT; y++) {
        srcYTable[y] = (y * LIDAR_HEIGHT) / LIDAR_TARGET_HEIGHT;
    }

    for (int x = 0; x < LIDAR_TARGET_WIDTH; x++) {
        srcXTable[x] = (x * LIDAR_WIDTH) / LIDAR_TARGET_WIDTH;
    }

    // Предварительно вычисляем индексы
    for (int y = 0; y < LIDAR_TARGET_HEIGHT; y++) {
        const int srcY = srcYTable[y];
        const int srcRowStart = srcY * LIDAR_WIDTH;

        for (int x = 0; x < LIDAR_TARGET_WIDTH; x++) {
            const int srcX = srcXTable[x];
            srcIndexTable[y * LIDAR_TARGET_WIDTH + x] = srcRowStart + srcX;
        }
    }
}







// 🔹 БЫСТРАЯ ОБРАБОТКА RGB (0x01) - основной поток
void NetworkServer::processRGBData(const QByteArray &data, quint64 sequenceNumber)
{
    static int rgbLogCounter = 0;
    if (rgbLogCounter++ % 60 == 0) {
        qDebug() << "🎯 [RGB] Processing RGB data - Seq:" << sequenceNumber;
    }

                                                                   // 🔹 ДЕКОДИРУЕМ JPEG АСИНХРОННО
                                                                   m_turboDecoder->decodeJPEGAsync(data, sequenceNumber);

    // 🔹 СОЗДАЕМ ARFrame ДЛЯ RGB ДАННЫХ
    // (изображение будет добавлено в handleTurboImageDecoded)
    LensEngine::ARFrame rgbFrame;
    rgbFrame.sequenceNumber = sequenceNumber;
    rgbFrame.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 🔹 ЗАПУСКАЕМ AR ОБРАБОТКУ ДЛЯ RGB (когда изображение будет готово)
    // Обработка продолжится в handleTurboImageDecoded
}


// 🔹 ПЕРЕРАБОТАННАЯ ОБРАБОТКА LIDAR - БЕЗ БЛОКИРОВКИ
void NetworkServer::processLidarDepthData(const QByteArray &data, quint64 sequenceNumber)
{
    static int lidarLogCounter = 0;
    if (lidarLogCounter++ % 60 == 0) {
        qDebug() << "🎯 [LIDAR] processLidarDepthData START - Size:" << data.size() << "Seq:" << sequenceNumber;
    }

    // 🔹 ПРОВЕРКА ДАННЫХ
    if (data.isEmpty() || data.size() < 8) {
        qWarning() << "❌ [LIDAR] Invalid data size:" << data.size();
        return;
    }

    // 🔹 1. МГНОВЕННАЯ 2D ВИЗУАЛИЗАЦИЯ
    processLidarFrame(data, sequenceNumber);

    // 🔹 2. ТЯЖЕЛАЯ 3D ОБРАБОТКА - В ОТДЕЛЬНОМ ПОТОКЕ LIDAR
    if (!m_lidar3DProcessor) {
        qWarning() << "❌ [LIDAR] m_lidar3DProcessor is null";
        return;
    }

    QMetaObject::invokeMethod(m_lidar3DProcessor, [this, data, sequenceNumber]() {
        if (!m_lidar3DProcessor) {
            qWarning() << "❌ [LIDAR] m_lidar3DProcessor became null in thread";
            return;
        }

        auto points3D = m_lidar3DProcessor->processDepthDataFast(data);

        // 🔹 ОБНОВЛЕНИЕ РЕЗУЛЬТАТОВ В ОСНОВНОМ ПОТОКЕ
        QMetaObject::invokeMethod(this, [this, points3D, data, sequenceNumber]() {
            if (!m_arDataProcessor) {
                qWarning() << "❌ [LIDAR] m_arDataProcessor is null";
                return;
            }

            // 🔹 СОЗДАЕМ ОТДЕЛЬНЫЙ ARFrame ДЛЯ LiDAR ДАННЫХ
            LensEngine::ARFrame lidarFrame;
            lidarFrame.lidar.points3D = points3D;
            lidarFrame.lidar.depthMap = data;
            lidarFrame.lidar.sequenceNumber = sequenceNumber;
            lidarFrame.lidar.timestamp = QDateTime::currentMSecsSinceEpoch();
            lidarFrame.sequenceNumber = sequenceNumber;
            lidarFrame.timestamp = QDateTime::currentMSecsSinceEpoch();

            // 🔹 ЗАПУСКАЕМ AR ОБРАБОТКУ ДЛЯ LiDAR ДАННЫХ
            if (arThreadPool) {
                QtConcurrent::run(arThreadPool, [this, lidarFrame]() {
                    if (m_arDataProcessor) {
                        m_arDataProcessor->processFrameAsync(lidarFrame);
                    }
                });
            }

            // 🔹 СИГНАЛ ДЛЯ QML
            emit lidarDataUpdated(points3D);
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}


// 🔹 КРИТИЧЕСКИЙ МЕТОД: ОБРАБОТКА IMU ДАННЫХ В ОТДЕЛЬНОМ ПОТОКЕ
void NetworkServer::processRawIMUData(const QByteArray &data, quint64 sequenceNumber)
{
    // 🔹 БЫСТРАЯ ПРОВЕРКА РАЗМЕРА ДАННЫХ
    if (data.size() < 104) {
        return;
    }

    // 🔹 СУПЕР БЫСТРЫЙ ПАРСИНГ В ОДИН ПРОХОД
    LensEngine::RawIMUData imuData;
    const char* rawData = data.constData();

    // Используем memcpy для более быстрого копирования
    memcpy(&imuData.timestamp, rawData, 8);
    imuData.timestamp = qFromLittleEndian<quint64>(&imuData.timestamp);

    memcpy(&imuData.accelX, rawData + 8, 8);
    imuData.accelX = qFromLittleEndian<double>(&imuData.accelX);
    memcpy(&imuData.accelY, rawData + 16, 8);
    imuData.accelY = qFromLittleEndian<double>(&imuData.accelY);
    memcpy(&imuData.accelZ, rawData + 24, 8);
    imuData.accelZ = qFromLittleEndian<double>(&imuData.accelZ);

    memcpy(&imuData.gyroX, rawData + 32, 8);
    imuData.gyroX = qFromLittleEndian<double>(&imuData.gyroX);
    memcpy(&imuData.gyroY, rawData + 40, 8);
    imuData.gyroY = qFromLittleEndian<double>(&imuData.gyroY);
    memcpy(&imuData.gyroZ, rawData + 48, 8);
    imuData.gyroZ = qFromLittleEndian<double>(&imuData.gyroZ);

    memcpy(&imuData.gravityX, rawData + 56, 8);
    imuData.gravityX = qFromLittleEndian<double>(&imuData.gravityX);
    memcpy(&imuData.gravityY, rawData + 64, 8);
    imuData.gravityY = qFromLittleEndian<double>(&imuData.gravityY);
    memcpy(&imuData.gravityZ, rawData + 72, 8);
    imuData.gravityZ = qFromLittleEndian<double>(&imuData.gravityZ);

    memcpy(&imuData.magX, rawData + 80, 8);
    imuData.magX = qFromLittleEndian<double>(&imuData.magX);
    memcpy(&imuData.magY, rawData + 88, 8);
    imuData.magY = qFromLittleEndian<double>(&imuData.magY);
    memcpy(&imuData.magZ, rawData + 96, 8);
    imuData.magZ = qFromLittleEndian<double>(&imuData.magZ);

    // 🔹 ЛОГ IMU ДАННЫХ КАЖДЫЕ 10 КАДРОВ
    static int imuCounter = 0;
    if (imuCounter++ % 10 == 0) {
        qDebug() << "📱 [IMU] Data Received #" << sequenceNumber << "- "
                 << "Accel:(" << QString::number(imuData.accelX, 'f', 3) << ","
                 << QString::number(imuData.accelY, 'f', 3) << ","
                 << QString::number(imuData.accelZ, 'f', 3) << ")"
                 << "Gyro:(" << QString::number(imuData.gyroX, 'f', 3) << ","
                 << QString::number(imuData.gyroY, 'f', 3) << ","
                 << QString::number(imuData.gyroZ, 'f', 3) << ")";
    }

    // 🔹 СОХРАНЯЕМ ДЛЯ DEBUG ПАНЕЛИ
    m_lastGyroX = imuData.gyroX;
    m_lastGyroY = imuData.gyroY;
    m_lastGyroZ = imuData.gyroZ;
    m_lastAccelX = imuData.accelX;
    m_lastAccelY = imuData.accelY;
    m_lastAccelZ = imuData.accelZ;

    // 🔹 СРАЗУ В СЕНСОРНЫЙ ФЬЮЖН (самое важное!)
    if (m_arDataProcessor) {
        QMetaObject::invokeMethod(m_arDataProcessor, [this, imuData]() {
                m_arDataProcessor->processIMUData(imuData);
            }, Qt::QueuedConnection);
    }

    // 🔹 СИГНАЛ ДЛЯ UI
    QMetaObject::invokeMethod(this, [this, imuData]() {
            emit imuDataUpdated(imuData.accelX, imuData.accelY, imuData.accelZ,
                                imuData.gyroX, imuData.gyroY, imuData.gyroZ);
        }, Qt::QueuedConnection);
}




// 🔹 ОБРАБОТКА RAW LIDAR POINT CLOUD (0x08)
void NetworkServer::processRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber)
{
    qDebug() << "🔦 [LiDAR PC] Processing raw point cloud - Seq:" << sequenceNumber;

                // 🔹 СОЗДАЕМ ARFrame ДЛЯ POINT CLOUD
                LensEngine::ARFrame pointCloudFrame;
    pointCloudFrame.lidar.pointCloud = data;
    pointCloudFrame.lidar.sequenceNumber = sequenceNumber;
    pointCloudFrame.lidar.timestamp = QDateTime::currentMSecsSinceEpoch();
    pointCloudFrame.sequenceNumber = sequenceNumber;
    pointCloudFrame.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 🔹 ЗАПУСКАЕМ AR ОБРАБОТКУ
    QtConcurrent::run(arThreadPool, [this, pointCloudFrame]() {
        m_arDataProcessor->processFrameAsync(pointCloudFrame);
    });
}

void NetworkServer::processLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber)
{
    qDebug() << "🎯 [LiDAR CONF] Processing confidence map - Seq:" << sequenceNumber;

                // 🔹 СОЗДАЕМ ARFrame ДЛЯ CONFIDENCE MAP
                LensEngine::ARFrame confidenceFrame;
    confidenceFrame.lidar.confidenceMap = data;
    confidenceFrame.lidar.sequenceNumber = sequenceNumber;
    confidenceFrame.lidar.timestamp = QDateTime::currentMSecsSinceEpoch();
    confidenceFrame.sequenceNumber = sequenceNumber;
    confidenceFrame.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 🔹 ЗАПУСКАЕМ AR ОБРАБОТКУ
    QtConcurrent::run(arThreadPool, [this, confidenceFrame]() {
        m_arDataProcessor->processFrameAsync(confidenceFrame);
    });
}

void NetworkServer::processDeviceOrientation(const QByteArray &data, quint64 sequenceNumber)
{
    emit rawDataReceived(SensorConnector::DEVICE_ORIENTATION, data, sequenceNumber);
}

// 🔹 ОБРАБОТЧИКИ ОТ ARDataProcessor
void NetworkServer::onFrameProcessed(const LensEngine::ARFrame &processedFrame)
{
    qDebug() << "🎯 Full AR frame processed - Features:" << processedFrame.featurePoints.size()
             << "Pose confidence:" << processedFrame.cameraPose.confidence;

    // 🔹 СОХРАНЯЕМ ДЛЯ DEBUG ПАНЕЛИ
    m_lastRotation = processedFrame.cameraPose.rotation;
    m_lastPosition = processedFrame.cameraPose.position;
    
    // Сохраняем LiDAR точки если есть
    if (!processedFrame.lidar.pointCloud.isEmpty()) {
        // Парсим point cloud (каждая точка 3 float = 12 bytes)
        const float* points = reinterpret_cast<const float*>(processedFrame.lidar.pointCloud.constData());
        int numPoints = processedFrame.lidar.pointCloud.size() / 12;
        m_lastLidarPoints.clear();
        for (int i = 0; i < numPoints; ++i) {
            m_lastLidarPoints.push_back(glm::vec3(points[i*3], points[i*3+1], points[i*3+2]));
        }
    }
    
    // Генерируем debug панель каждые 30 кадров
    static int frameCounter = 0;
    if (frameCounter++ % 30 == 0) {
        generateAndDisplayDebugPanel();
    }

    // 🔹 СИГНАЛ ДЛЯ ВНЕШНИХ КОМПОНЕНТОВ
    emit arFrameProcessed(processedFrame);
}

void NetworkServer::onFeaturePointsUpdated(const QVector<LensEngine::FeaturePoint> &features)
{
    QVariantList featuresList;
    for (const auto &feature : features) {
        QVariantMap featureMap;
        featureMap["x"] = feature.position.x();
        featureMap["y"] = feature.position.y();
        featureMap["z"] = feature.position.z();
        featureMap["confidence"] = feature.confidence;
        featuresList.append(featureMap);
    }

    // 🔹 КЭШИРУЕМ
    m_lastFeaturePoints = featuresList;

    emit featurePointsUpdated(featuresList);
}

void NetworkServer::onCameraIntrinsicsUpdated(const LensEngine::CameraIntrinsics &intrinsics)
{
    if (intrinsics.isValid) {
        QVariantMap intrinsicsMap;
        intrinsicsMap["focalLengthX"] = intrinsics.focalLengthX;
        intrinsicsMap["focalLengthY"] = intrinsics.focalLengthY;
        intrinsicsMap["principalPointX"] = intrinsics.principalPointX;
        intrinsicsMap["principalPointY"] = intrinsics.principalPointY;
        intrinsicsMap["width"] = intrinsics.resolution.width();
        intrinsicsMap["height"] = intrinsics.resolution.height();

        emit cameraIntrinsicsUpdated(intrinsicsMap);
    }
}

void NetworkServer::onLightEstimationUpdated(const LensEngine::LightEstimation &light)
{
    if (light.isValid) {
        QVariantMap lightMap;
        lightMap["ambientIntensity"] = light.ambientIntensity;
        lightMap["colorTemperature"] = light.colorTemperature;
        lightMap["ambientColor"] = QVariant::fromValue(light.ambientColor);

        emit lightEstimationUpdated(lightMap);
    }
}

void NetworkServer::onCameraPoseUpdated(const LensEngine::CameraPose &pose)
{
    if (m_arCameraController) {
        m_arCameraController->updateFromCameraPose(pose);
    }

    // 🔹 КЭШИРУЕМ
    m_lastCameraPosition = pose.position;
    m_lastCameraRotation = pose.rotation;

    emit cameraPoseUpdated(pose.position, pose.rotation);
    emit fusionStabilityUpdated(pose.confidence);
}

void NetworkServer::onSensorDataUpdated(float pitch, float yaw, float roll,
                                       float accelX, float accelY, float accelZ)
{
    emit sensorDataUpdated(pitch, yaw, roll, accelX, accelY, accelZ);
}

// 🔹 ОБРАБОТКА USB RGB ДАННЫХ (0x01)
void NetworkServer::handleUsbData(const QByteArray &data, quint64 sequenceNumber)
{
    processRGBData(data, sequenceNumber);
}

// 🔹 ОБРАБОТКА USB LIDAR DEPTH (0x02)
void NetworkServer::handleUsbLidarData(const QByteArray &data, quint64 sequenceNumber)
{
    processLidarDepthData(data, sequenceNumber);
}

// 🔹 ОБРАБОТКА USB RAW IMU (0x03)
void NetworkServer::handleUsbSensorData(const QByteArray &data, quint64 sequenceNumber)
{
    processRawIMUData(data, sequenceNumber);
}

// 🔹 ОБРАБОТКА USB RAW LIDAR POINT CLOUD (0x08)
void NetworkServer::handleUsbRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber)
{
    processRawLidarPointCloud(data, sequenceNumber);
}

// 🔹 ОБРАБОТКА USB LIDAR CONFIDENCE MAP (0x09)
void NetworkServer::handleUsbLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber)
{
    processLidarConfidenceMap(data, sequenceNumber);
}

void NetworkServer::processLidarFrame(const QByteArray &data, quint64 sequenceNumber)
{
    if (data.isEmpty() || data.size() < static_cast<int>(sizeof(float))) {
        qWarning() << "❌ Invalid LiDAR data size:" << data.size();
        return;
    }

    const float* depthData = reinterpret_cast<const float*>(data.constData());
    int dataSize = data.size() / sizeof(float);

    // Определяем разрешение из размера (типичные: 256x192=49152, 320x240=76800)
    int width = 256, height = 192;
    if (dataSize == 256 * 192) { width = 256; height = 192; }
    else if (dataSize == 320 * 240) { width = 320; height = 240; }
    else if (dataSize == 240 * 180) { width = 240; height = 180; }
    else {
        height = static_cast<int>(std::sqrt(dataSize));
        width = dataSize / height;
        if (width * height != dataSize) { width = 256; height = 192; }
    }

    QImage depthImage(width, height, QImage::Format_Grayscale8);
    depthImage.fill(0);

    float maxDepth = 0.0f;
    for (int i = 0; i < dataSize; ++i) {
        if (depthData[i] > maxDepth && depthData[i] < 10.0f)
            maxDepth = depthData[i];
    }
    if (maxDepth < 0.1f) maxDepth = 1.0f;
    float scale = 255.0f / maxDepth;

    for (int y = 0; y < height; ++y) {
        uchar* scanline = depthImage.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (idx < dataSize) {
                float depth = depthData[idx];
                scanline[x] = (depth > 0 && depth < 10.0f)
                                  ? static_cast<uchar>(qMin(depth * scale, 255.0f))
                                  : 0;
            }
        }
    }

    lidarFrameBuffer[sequenceNumber] = depthImage;
    lidarFrameTimestamps[sequenceNumber] = QDateTime::currentMSecsSinceEpoch();

    emit lidarFrameReceived(depthImage);
    updateLidarStatistics(data.size());
}



// 🔹 РЕАЛИЗАЦИЯ НЕДОСТАЮЩИХ СЛОТОВ ДЛЯ LiDAR

void NetworkServer::onLidarSpatialAnalysisCompleted(const Lidar3DProcessor::SpatialAnalysisResult &analysis)
{
    qDebug() << "🔦 LiDAR spatial analysis - Floor:" << analysis.hasFloor
             << "Walls:" << analysis.planes.size()
             << "Obstacles:" << analysis.obstacles.size();

    // 🔹 ПЕРЕДАЕМ ДАННЫЕ В SPATIAL MAPPING
    if (m_arDataProcessor) {
        QVector<QVector3D> points = m_lidar3DProcessor->getLastProcessedPoints();
        m_arDataProcessor->updateSpatialMapping(analysis, points);
    }

    // 🔹 ОТПРАВЛЯЕМ ДАННЫЕ В QML ДЛЯ ВИЗУАЛИЗАЦИИ
    QVariantList obstaclesList;
    for (const QVector3D &obstacle : analysis.obstacles) {
        QVariantMap obstacleMap;
        obstacleMap["x"] = obstacle.x();
        obstacleMap["y"] = obstacle.y();
        obstacleMap["z"] = obstacle.z();
        obstaclesList.append(obstacleMap);
    }
    emit objectsDetected(obstaclesList);
}

void NetworkServer::onLidarPointsProcessed(const QVector<QVector3D> &points)
{
    qDebug() << "📊 LiDAR points processed:" << points.size() << "points";

    // 🔹 КОНВЕРТИРУЕМ ДЛЯ QML
    QVariantList pointsList;
    for (const QVector3D &point : points) {
        QVariantMap pointMap;
        pointMap["x"] = point.x();
        pointMap["y"] = point.y();
        pointMap["z"] = point.z();
        pointsList.append(pointMap);
    }

    // 🔹 КЭШИРУЕМ
    m_lastLidarPoints = pointsList;

    emit lidarPointsUpdated(pointsList);
}

void NetworkServer::onFloorDetected(const QVector3D &normal, float height)
{
    qDebug() << "📐 Floor detected - Height:" << height << "Normal:" << normal;

    // Можно добавить логику обработки пола
}

// 🔹 ОБРАБОТКА CAMERA INTRINSICS (0x10)
void NetworkServer::processCameraIntrinsics(const QByteArray &data, quint64 sequenceNumber)
{
    if (data.size() < 64) {
        qWarning() << "❌ Invalid intrinsics data size:" << data.size();
        return;
    }

    // Parse 8 doubles (64 bytes)
    const double* values = reinterpret_cast<const double*>(data.constData());
    
    m_cameraIntrinsics.fx_rgb = values[0];
    m_cameraIntrinsics.fy_rgb = values[1];
    m_cameraIntrinsics.cx_rgb = values[2];
    m_cameraIntrinsics.cy_rgb = values[3];
    m_cameraIntrinsics.fx_lidar = values[4];
    m_cameraIntrinsics.fy_lidar = values[5];
    m_cameraIntrinsics.cx_lidar = values[6];
    m_cameraIntrinsics.cy_lidar = values[7];

    qDebug() << "📷 Camera Intrinsics Received:";
    qDebug() << "   RGB: fx=" << m_cameraIntrinsics.fx_rgb << "fy=" << m_cameraIntrinsics.fy_rgb
             << "cx=" << m_cameraIntrinsics.cx_rgb << "cy=" << m_cameraIntrinsics.cy_rgb;
    qDebug() << "   LiDAR: fx=" << m_cameraIntrinsics.fx_lidar << "fy=" << m_cameraIntrinsics.fy_lidar
             << "cx=" << m_cameraIntrinsics.cx_lidar << "cy=" << m_cameraIntrinsics.cy_lidar;

    // Pass to ARDataProcessor
    if (m_arDataProcessor) {
        m_arDataProcessor->setCameraIntrinsics(
            m_cameraIntrinsics.fx_rgb, m_cameraIntrinsics.fy_rgb,
            m_cameraIntrinsics.cx_rgb, m_cameraIntrinsics.cy_rgb
        );
    }
}

// 🔹 GENERATE DEBUG PANEL
void NetworkServer::generateAndDisplayDebugPanel()
{
    m_debugPanel = DebugVisualizer::visualizeDebugPanel(
        m_lastRotation,
        m_lastPosition,
        m_lastLidarPoints,
        m_lastGyroX, m_lastGyroY, m_lastGyroZ,
        m_lastAccelX, m_lastAccelY, m_lastAccelZ,
        1024, 768
    );
    
    // Сохраняем для отладки
    static int debugCounter = 0;
    if (debugCounter++ % 30 == 0) {
        QString debugPath = QString("debug_panel_%1.png").arg(debugCounter);
        m_debugPanel.save(debugPath);
        qDebug() << "💾 Debug panel saved:" << debugPath;
    }
}

void NetworkServer::onObstaclesDetected(const QVector<QVector3D> &obstacles)
{
    qDebug() << "🚧 Obstacles detected:" << obstacles.size() << "obstacles";

    // 🔹 ОТПРАВЛЯЕМ ДАННЫЕ В QML
    QVariantList obstaclesList;
    for (const QVector3D &obstacle : obstacles) {
        QVariantMap obstacleMap;
        obstacleMap["x"] = obstacle.x();
        obstacleMap["y"] = obstacle.y();
        obstacleMap["z"] = obstacle.z();
        obstaclesList.append(obstacleMap);
    }
    emit objectsDetected(obstaclesList);
}
