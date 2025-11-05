#include "NetworkServerSimplified.h"
#include "SensorConnector.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QDateTime>

namespace SensorConnector {

SensorConnectorCore::SensorConnectorCore(QObject *parent)
    : QObject(parent)
    , m_networkServer(nullptr)
{
}

SensorConnectorCore::~SensorConnectorCore()
{
    stopServers();
}

bool SensorConnectorCore::initialize()
{
    qDebug() << "🔧 Initializing SensorConnector...";
    
    // Инициализация упрощенного сетевого сервера
    m_networkServer = new NetworkServerSimplified(this);
    
    // Подключаем сигналы от NetworkServer
    connect(m_networkServer, &NetworkServerSimplified::rawDataReceived,
            this, [this](SensorConnector::DataType type, const QByteArray &data, quint64 sequenceNumber) {
                SensorData sensorData;
                sensorData.type = type;
                sensorData.payload = data;
                sensorData.sequenceNumber = sequenceNumber;
                sensorData.timestamp = QDateTime::currentMSecsSinceEpoch();
                emit dataReceived(sensorData);
            });
    
    // Подключаем сигнал декодированных RGB кадров для AR рендеринга
    connect(m_networkServer, &NetworkServerSimplified::frameDecoded,
            this, &SensorConnectorCore::frameDecoded);
    
    connect(m_networkServer, &NetworkServerSimplified::statusChanged,
            this, &SensorConnectorCore::connectionStatusChanged);
    connect(m_networkServer, &NetworkServerSimplified::clientsCountChanged,
            this, &SensorConnectorCore::clientsCountChanged);
    
    qDebug() << "✅ SensorConnector initialized";
    return true;
}

void SensorConnectorCore::startServers(quint16 tcpPort, quint16 udpPort)
{
    qDebug() << "🚀 Starting SensorConnector servers...";
    
    if (m_networkServer) {
        m_networkServer->startServers(tcpPort, udpPort);
    }
    
    emit connectionStatusChanged("Servers started");
}

void SensorConnectorCore::stopServers()
{
    qDebug() << "🛑 Stopping SensorConnector servers...";
    
    if (m_networkServer) {
        m_networkServer->stopServers();
    }
    
    emit connectionStatusChanged("Servers stopped");
}

ConnectionStats SensorConnectorCore::getStatistics() const
{
    // Создаем локальную копию для изменения (метод const)
    ConnectionStats stats = m_stats;
    
    // Обновляем статистику из NetworkServer
    stats.clientsCount = m_networkServer ? m_networkServer->clientsCount() : 0;
    stats.status = m_networkServer ? m_networkServer->serverStatus() : QString("Stopped");
    
    return stats;
}

bool SensorConnectorCore::isUsbConnected() const
{
    // USB управляется внутри NetworkServerSimplified
    return m_networkServer ? (m_networkServer->clientsCount() > 0) : false;
}

bool SensorConnectorCore::isWiFiConnected() const
{
    return m_networkServer ? (m_networkServer->clientsCount() > 0) : false;
}

void SensorConnectorCore::updateStatistics()
{
    // Обновляем статистику из NetworkServer
    if (m_networkServer) {
        m_stats.clientsCount = m_networkServer->clientsCount();
        m_stats.status = m_networkServer->serverStatus();
        
        // Вычисляем FPS и скорость (если есть данные)
        static QElapsedTimer statsTimer;
        static int lastFramesCount = 0;
        static qint64 lastTotalBytes = 0;
        
        if (!statsTimer.isValid()) {
            statsTimer.start();
        }
        
        qint64 elapsed = statsTimer.elapsed();
        if (elapsed > 1000) { // Раз в секунду
            int currentFrames = m_networkServer->getFramesCount();
            qint64 currentBytes = m_networkServer->getTotalBytes();
            
            m_stats.fps = (currentFrames - lastFramesCount) * 1000 / elapsed;
            m_stats.speedKbps = (currentBytes - lastTotalBytes) * 8.0 / elapsed / 1000.0; // kbps
            
            lastFramesCount = currentFrames;
            lastTotalBytes = currentBytes;
            statsTimer.restart();
        }
        
        // Определяем тип соединения
        if (m_stats.clientsCount > 0) {
            m_stats.connectionType = "USB/WiFi";
        } else {
            m_stats.connectionType = "None";
        }
    }
    emit statisticsUpdated(m_stats);
}

} // namespace SensorConnector

