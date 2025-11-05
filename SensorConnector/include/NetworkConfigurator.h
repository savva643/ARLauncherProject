#ifndef NETWORKCONFIGURATOR_H
#define NETWORKCONFIGURATOR_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QNetworkInterface>

class NetworkConfigurator : public QObject
{
    Q_OBJECT

public:
    explicit NetworkConfigurator(QObject *parent = nullptr);

    // Статические настройки
    static const QString usbHostIP;
    static const QString usbSubnetMask;

    // Методы
    bool configureUsbNetwork();
    bool isUsbInterfaceAvailable();
    QString getUsbInterfaceName();
    void startMonitoring();
    void stopMonitoring();

signals:
    void networkStatusChanged(const QString &status);
    void usbInterfaceDetected(bool detected);

private slots:
    void checkUsbInterface();

private:
    QTimer *m_checkTimer;
    bool m_usbDetected;

    // Приватные методы для разных ОС
    bool configureWindowsNetwork();
    bool configureMacNetwork();
    bool configureLinuxNetwork();
    QString executeCommand(const QString &program, const QStringList &arguments);
};

#endif // NETWORKCONFIGURATOR_H
