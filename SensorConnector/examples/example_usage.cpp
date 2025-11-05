#include "SensorConnector.h"
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

/**
 * @brief Пример использования SensorConnector
 */
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    using namespace SensorConnector;
    
    // Создание и инициализация SensorConnector
    SensorConnectorCore connector;
    if (!connector.initialize()) {
        qCritical() << "Failed to initialize SensorConnector";
        return -1;
    }
    
    // Запуск серверов
    connector.startServers(9000, 9000);
    qDebug() << "✅ SensorConnector servers started on TCP:9000, UDP:9000";
    
    // Подключение обработчика данных
    QObject::connect(&connector, &SensorConnectorCore::dataReceived,
                     [](const SensorData &data) {
        qDebug() << "📦 Data received:"
                 << "Type:" << static_cast<int>(data.type)
                 << "Size:" << data.payload.size()
                 << "Sequence:" << data.sequenceNumber;
    });
    
    // Подключение обработчика статистики
    QObject::connect(&connector, &SensorConnectorCore::statisticsUpdated,
                     [](const ConnectionStats &stats) {
        qDebug() << "📊 Statistics:"
                 << "FPS:" << stats.fps
                 << "Speed:" << stats.speedKbps << "kbps"
                 << "Clients:" << stats.clientsCount
                 << "Status:" << stats.status;
    });
    
    qDebug() << "🚀 SensorConnector ready. Waiting for iPhone connection...";
    qDebug() << "📱 Connect iPhone via USB or WiFi to port 9000";
    
    return app.exec();
}

