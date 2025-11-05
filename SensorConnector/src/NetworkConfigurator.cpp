#include "NetworkConfigurator.h"
#include <QDebug>
#include <QRegularExpression>

const QString NetworkConfigurator::usbHostIP = "172.20.10.3"; // Тот же IP
const QString NetworkConfigurator::usbSubnetMask = "255.255.255.240"; // Маска для 172.20.10.x

NetworkConfigurator::NetworkConfigurator(QObject *parent)
    : QObject(parent)
    , m_usbDetected(false)
{
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &NetworkConfigurator::checkUsbInterface);
}

bool NetworkConfigurator::configureUsbNetwork()
{
    qDebug() << "🔧 Configuring USB network...";

#ifdef Q_OS_WIN
    return configureWindowsNetwork();
#elif defined(Q_OS_MAC)
    return configureMacNetwork();
#elif defined(Q_OS_LINUX)
    return configureLinuxNetwork();
#else
    qWarning() << "Unsupported operating system";
    return false;
#endif
}

bool NetworkConfigurator::isUsbInterfaceAvailable()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        QString name = interface.humanReadableName().toLower();

        // Проверяем различные названия USB Ethernet интерфейсов
        if (name.contains("usb") ||
            name.contains("rndis") ||
            name.contains("apple mobile device ethernet") ||
            name.contains("iphone") ||
            name.contains("ipad") ||
            (name.contains("ethernet") && (name.contains("mobile") || name.contains("portable")))) {

            qDebug() << "✅ Found USB interface:" << interface.humanReadableName();
            return true;
        }
    }

    return false;
}

void NetworkConfigurator::startMonitoring()
{
    m_checkTimer->start(5000); // Проверка каждые 5 секунд
    checkUsbInterface(); // Немедленная проверка
}

void NetworkConfigurator::stopMonitoring()
{
    m_checkTimer->stop();
}

QString NetworkConfigurator::getUsbInterfaceName()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        QString name = interface.humanReadableName().toLower();

        if (name.contains("usb") ||
            name.contains("rndis") ||
            name.contains("apple mobile device ethernet") ||
            name.contains("iphone") ||
            name.contains("ipad")) {

            return interface.humanReadableName();
        }
    }

    return QString();
}

void NetworkConfigurator::checkUsbInterface()
{
    bool detected = isUsbInterfaceAvailable();
    if (detected != m_usbDetected) {
        m_usbDetected = detected;
        emit usbInterfaceDetected(detected);

        if (detected) {
            emit networkStatusChanged("USB Ethernet interface detected");
            configureUsbNetwork();
        } else {
            emit networkStatusChanged("Waiting for USB Ethernet...");
        }
    }
}

// 🔹 WINDOWS КОНФИГУРАЦИЯ
bool NetworkConfigurator::configureWindowsNetwork()
{
    QString interfaceName = getUsbInterfaceName();
    if (interfaceName.isEmpty()) {
        emit networkStatusChanged("No USB interface found");
        return false;
    }

    qDebug() << "Configuring Windows network for interface:" << interfaceName;

    // Используем netsh для настройки сети
    QStringList args;

    // Сначала удаляем существующие IP
    QStringList removeArgs;
    removeArgs << "interface" << "ip" << "delete" << "address"
               << QString("name=\"%1\"").arg(interfaceName)
               << "169.254.2.1"; // 🔹 ИЗМЕНИТЕ IP
    executeCommand("netsh", removeArgs);

    // Устанавливаем статический IP
    args << "interface" << "ip" << "set" << "address"
         << QString("name=\"%1\"").arg(interfaceName)
         << "static" << usbHostIP << "255.255.0.0"; // 🔹 ИЗМЕНИТЕ МАСКУ

    QString result = executeCommand("netsh", args);
    if (result.contains("ok", Qt::CaseInsensitive) || result.isEmpty()) {
        qDebug() << "✅ Successfully configured USB network on Windows";
        emit networkStatusChanged("USB network configured: " + usbHostIP);
        return true;
    } else {
        qWarning() << "❌ Failed to configure USB network on Windows:" << result;

                    // 🔹 ПРОБУЕМ АВТОМАТИЧЕСКУЮ НАСТРОЙКУ (DHCP)
                    QStringList dhcpArgs;
        dhcpArgs << "interface" << "ip" << "set" << "address"
                 << QString("name=\"%1\"").arg(interfaceName) << "dhcp";
        executeCommand("netsh", dhcpArgs);

        emit networkStatusChanged("Using automatic USB network configuration");
        return true;
    }
}

// 🔹 macOS КОНФИГУРАЦИЯ
bool NetworkConfigurator::configureMacNetwork()
{
    QString interfaceName = getUsbInterfaceName();
    if (interfaceName.isEmpty()) return false;

    // Для macOS используем networksetup
    QStringList args;
    args << "-setmanual" << interfaceName << usbHostIP << usbSubnetMask;

    QString result = executeCommand("networksetup", args);
    return !result.contains("error", Qt::CaseInsensitive);
}

// 🔹 LINUX КОНФИГУРАЦИЯ
bool NetworkConfigurator::configureLinuxNetwork()
{
    QString interfaceName = getUsbInterfaceName();
    if (interfaceName.isEmpty()) return false;

    // Для Linux используем ip command
    QStringList args;
    args << "addr" << "add" << QString("%1/24").arg(usbHostIP) << "dev" << interfaceName;

    QString result = executeCommand("sudo", QStringList() << "ip" << args);
    return result.isEmpty();
}

QString NetworkConfigurator::executeCommand(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.start(program, arguments);

    if (!process.waitForFinished(10000)) { // 10 секунд таймаут
        qWarning() << "Command timeout:" << program << arguments;
        return "timeout";
    }

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (process.exitCode() != 0) {
        qWarning() << "Command failed:" << program << arguments << "Error:" << error;
    }

    return output + error;
}
