#ifndef USBMANAGER_H
#define USBMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QTimer>
#include <QtEndian>
// Forward declaration
class NetworkConfigurator;

class UsbManager : public QObject
{
    Q_OBJECT

public:
    explicit UsbManager(QObject *parent = nullptr);
    ~UsbManager();

    void startUsbServer();
    void stopUsbServer();
    bool isUsbConnected() const;

    // 🔹 ДОБАВЛЕННЫЕ МЕТОДЫ ДЛЯ АВТОМАТИЧЕСКОГО ОПРЕДЕЛЕНИЯ IP
    QString findAppleUSBInterface();
    QString getLocalPCIP();
    void refreshNetworkInterface();

    // Статические настройки USB Ethernet
    static const QString usbHostIP;
    static const quint16 usbPort;

    // 🔹 УДАЛИТЬ ЭТИ МЕТОДЫ - они больше не нужны
    // void startLidarUsbServer();
    // void stopLidarUsbServer();
    // void sendLidarUsbData(const QByteArray &data);

signals:
    void usbClientConnected();
    void usbClientDisconnected();
    void usbStatusChanged(const QString &status);
    // 🔹 СУЩЕСТВУЮЩИЕ СИГНАЛЫ
    void usbDataReceived(const QByteArray &data, quint64 sequenceNumber);           // 0x01 - RGB
    void usbLidarDataReceived(const QByteArray &data, quint64 sequenceNumber);      // 0x02 - LiDAR Depth
    void usbSensorDataReceived(const QByteArray &data, quint64 sequenceNumber);     // 0x03 - Raw IMU

    // 🔹 НОВЫЕ СИГНАЛЫ ДЛЯ ДОПОЛНИТЕЛЬНЫХ ТИПОВ ДАННЫХ
    void usbRawLidarPointCloudReceived(const QByteArray &data, quint64 sequenceNumber);  // 0x08 - Raw LiDAR Point Cloud
    void usbLidarConfidenceMapReceived(const QByteArray &data, quint64 sequenceNumber);  // 0x09 - LiDAR Confidence Map

public slots:
    void sendUsbData(const QByteArray &data);

private slots:
    void handleUsbConnection();
    void handleUsbDisconnection();
    void processUsbData();
    void checkUsbConnection();
    void handleNetworkStatus(const QString &status);
    void handleUsbInterfaceDetected(bool detected);

private:
    QTcpServer *m_usbServer;
    QTcpSocket *m_usbClient;
    QTimer *m_connectionTimer;
    bool m_usbConnected;
    NetworkConfigurator *m_networkConfigurator;

    void setupUsbNetwork();

    // 🔹 УДАЛИТЬ ЭТИ ПЕРЕМЕННЫЕ - они больше не нужны
    // QTcpServer *m_lidarUsbServer;
    // QTcpSocket *m_lidarUsbClient;
    // bool m_lidarUsbConnected;
    // static const quint16 LIDAR_USB_PORT = 9004; // 🔹 УДАЛИТЬ
};

#endif // USBMANAGER_H
