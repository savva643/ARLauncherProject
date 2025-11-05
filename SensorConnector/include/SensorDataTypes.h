#ifndef SENSORDATATYPES_H
#define SENSORDATATYPES_H

#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <QtCore/QString>

namespace SensorConnector {

// Типы данных от iPhone (полный набор)
enum DataType {
    RGB_CAMERA = 0x01,           // RGB данные камеры
    LIDAR_DEPTH = 0x02,          // LiDAR данные
    RAW_IMU = 0x03,              // Сенсорные данные (IMU)
    FEATURE_POINTS = 0x04,       // Feature Points
    CAMERA_INTRINSICS = 0x05,    // Camera Intrinsics
    LIGHT_ESTIMATION = 0x06      // Light Estimation
};

// Структура для передачи данных
struct SensorData {
    DataType type;               // Тип данных
    QByteArray payload;          // Сырые данные
    quint64 sequenceNumber;      // Номер последовательности
    quint64 timestamp;           // Временная метка
    
    SensorData() : type(RGB_CAMERA), sequenceNumber(0), timestamp(0) {}
};

// Структура для статистики
struct ConnectionStats {
    int fps;
    double speedKbps;
    int clientsCount;
    QString connectionType;
    QString status;
    
    ConnectionStats() : fps(0), speedKbps(0.0), clientsCount(0) {}
};

} // namespace SensorConnector

#endif // SENSORDATATYPES_H

