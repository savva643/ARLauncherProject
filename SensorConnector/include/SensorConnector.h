#ifndef SENSORCONNECTOR_H
#define SENSORCONNECTOR_H

#include <QObject>
#include <QString>
#include <QImage>
#include "SensorDataTypes.h"

namespace SensorConnector {

// Forward declaration
class NetworkServerSimplified;

/**
 * @brief Главный класс SensorConnector - эмулятор получения данных с iPhone
 * 
 * Обрабатывает:
 * - USB подключение (Ethernet через USB)
 * - WiFi подключение (TCP/UDP)
 * - Декодирование JPEG (TurboJPEG, FFmpeg, FastJPEG)
 * - Все типы данных (0x01-0x06)
 */
class SensorConnectorCore : public QObject
{
    Q_OBJECT

public:
    explicit SensorConnectorCore(QObject *parent = nullptr);
    ~SensorConnectorCore();

    // Инициализация и управление
    bool initialize();
    void startServers(quint16 tcpPort = 9000, quint16 udpPort = 9000);
    void stopServers();
    
    // Получение статистики
    ConnectionStats getStatistics() const;
    
    // Проверка статуса
    bool isUsbConnected() const;
    bool isWiFiConnected() const;

signals:
    // Данные получены
    void dataReceived(const SensorData &data);
    
    // Декодированные RGB кадры с камеры (для AR рендеринга)
    void frameDecoded(const QImage &frame, quint64 sequenceNumber);
    
    // Статистика обновлена
    void statisticsUpdated(const ConnectionStats &stats);
    
    // Статус соединения
    void connectionStatusChanged(const QString &status);
    void clientsCountChanged(int count);

private:
    NetworkServerSimplified *m_networkServer;
    
    ConnectionStats m_stats;
    
    void updateStatistics();
};

} // namespace SensorConnector

#endif // SENSORCONNECTOR_H

