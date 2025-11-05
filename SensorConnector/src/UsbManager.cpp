#include "usbmanager.h"
#include "networkconfigurator.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QBuffer>

const QString UsbManager::usbHostIP = "172.20.10.3"; // 🔹 Резервный IP
const quint16 UsbManager::usbPort = 9001;

UsbManager::UsbManager(QObject *parent)
    : QObject(parent)
    , m_usbServer(nullptr)
    , m_usbClient(nullptr)
    , m_usbConnected(false)
    , m_networkConfigurator(new NetworkConfigurator(this))
{
    m_connectionTimer = new QTimer(this);
    connect(m_connectionTimer, &QTimer::timeout, this, &UsbManager::checkUsbConnection);

    // 🔹 АВТОМАТИЧЕСКАЯ НАСТРОЙКА СЕТИ
    connect(m_networkConfigurator, &NetworkConfigurator::networkStatusChanged,
            this, &UsbManager::handleNetworkStatus);
    connect(m_networkConfigurator, &NetworkConfigurator::usbInterfaceDetected,
            this, &UsbManager::handleUsbInterfaceDetected);
}

UsbManager::~UsbManager()
{
    stopUsbServer();
}

// 🔹 АВТОМАТИЧЕСКОЕ ОПРЕДЕЛЕНИЕ APPLE MOBILE DEVICE ETHERNET
QString UsbManager::findAppleUSBInterface()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        QString name = interface.name();
        QString description = interface.humanReadableName();

        qDebug() << "🔍 Проверка интерфейса:" << name << "(" << description << ")";

        // 🔹 ИЩЕМ APPLE MOBILE DEVICE ETHERNET ПО НАЗВАНИЮ И ОПИСАНИЮ
        if (description.contains("Apple Mobile Device Ethernet", Qt::CaseInsensitive) ||
            description.contains("Apple Mobile Device", Qt::CaseInsensitive) ||
            name.contains("Ethernet", Qt::CaseInsensitive)) {

            qDebug() << "🎯 Найден Apple USB интерфейс:" << name << "(" << description << ")";

            // 🔹 ПОЛУЧАЕМ IP АДРЕСА ЭТОГО ИНТЕРФЕЙСА
            QList<QNetworkAddressEntry> addresses = interface.addressEntries();
            for (const QNetworkAddressEntry &entry : addresses) {
                QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                    ip != QHostAddress::LocalHost) {

                    qDebug() << "✅ IP адрес Apple USB интерфейса:" << ip.toString();
                    return ip.toString();
                }
            }
        }
    }

    qWarning() << "❌ Apple Mobile Device Ethernet интерфейс не найден";
    return QString();
}

// 🔹 АВТОМАТИЧЕСКОЕ ПОЛУЧЕНИЕ IP ПК В USB СЕТИ
QString UsbManager::getLocalPCIP()
{
    // Сначала пытаемся найти Apple USB интерфейс
    QString appleInterfaceIP = findAppleUSBInterface();
    if (!appleInterfaceIP.isEmpty()) {
        return appleInterfaceIP;
    }

    // 🔹 ЕСЛИ APPLE ИНТЕРФЕЙС НЕ НАЙДЕН, ИЩЕМ ЛЮБОЙ USB/ETHERNET
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &interface : interfaces) {
        // Пропускаем виртуальные и беспроводные интерфейсы
        if (interface.name().startsWith("vEthernet") ||
            interface.name().startsWith("Wireless") ||
            interface.name().startsWith("Wi-Fi") ||
            interface.name().contains("Virtual") ||
            !interface.isValid() ||
            interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }

        // 🔹 ИЩЕМ ПОДКЛЮЧЕННЫЕ ETHERNET ИНТЕРФЕЙСЫ
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.addressEntries().isEmpty()) {

            QList<QNetworkAddressEntry> addresses = interface.addressEntries();
            for (const QNetworkAddressEntry &entry : addresses) {
                QHostAddress ip = entry.ip();
                // 🔹 ПРИОРИТЕТ IP ИЗ ПОДСЕТИ 172.20.10.x
                if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                    ip != QHostAddress::LocalHost &&
                    ip.toString().startsWith("172.20.10.")) {

                    qDebug() << "✅ Найден USB Ethernet IP:" << ip.toString() << "на интерфейсе" << interface.name();
                    return ip.toString();
                }
            }
        }
    }

    // 🔹 ЕСЛИ НИЧЕГО НЕ НАЙДЕНО, ВОЗВРАЩАЕМ РЕЗЕРВНЫЙ IP
    qWarning() << "⚠️ USB интерфейс не найден, использую резервный IP:" << usbHostIP;
    return usbHostIP;
}

// 🔹 ОБНОВИТЕ МЕТОД startUsbServer
void UsbManager::startUsbServer()
{
    if (m_usbServer) {
        stopUsbServer();
    }

    // 🔹 ЗАПУСК ЕДИНОГО USB СЕРВЕРА НА ПОРТУ 9001
    QString localIP = getLocalPCIP();
    qDebug() << "🎯 Запуск USB сервера на IP:" << localIP << "порт:" << usbPort;

    m_usbServer = new QTcpServer(this);
    connect(m_usbServer, &QTcpServer::newConnection, this, &UsbManager::handleUsbConnection);

    if (m_usbServer->listen(QHostAddress(localIP), usbPort)) {
        qDebug() << "✅ USB Server started on" << localIP << "port" << usbPort;
        emit usbStatusChanged("USB Server ready on " + localIP + ":" + QString::number(usbPort));
    } else {
        qWarning() << "❌ Failed to start USB server:" << m_usbServer->errorString();

        // 🔹 РЕЗЕРВНЫЙ ВАРИАНТ
        if (m_usbServer->listen(QHostAddress::Any, usbPort)) {
            qDebug() << "✅ USB Server started on all interfaces";
            emit usbStatusChanged("USB Server started on all interfaces");
        } else {
            emit usbStatusChanged("USB Error: " + m_usbServer->errorString());
        }
    }

    m_connectionTimer->start(3000);
}

void UsbManager::stopUsbServer()
{
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    if (m_usbClient) {
        m_usbClient->close();
        m_usbClient->deleteLater();
        m_usbClient = nullptr;
    }

    if (m_usbServer) {
        m_usbServer->close();
        m_usbServer->deleteLater();
        m_usbServer = nullptr;
    }

    m_usbConnected = false;
    emit usbStatusChanged("USB Server stopped");
}

void UsbManager::handleUsbConnection()
{
    if (m_usbClient) {
        QTcpSocket *newClient = m_usbServer->nextPendingConnection();
        qDebug() << "🔦 Отклоняем повторное подключение USB от:" << newClient->peerAddress().toString();
        newClient->close();
        newClient->deleteLater();
        return;
    }

    m_usbClient = m_usbServer->nextPendingConnection();

    // 🔹 ОПТИМИЗАЦИЯ ДЛЯ ВЫСОКОСКОРОСТНОЙ ПЕРЕДАЧИ
    m_usbClient->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 2 * 1024 * 1024);
    m_usbClient->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 2 * 1024 * 1024);
    m_usbClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    QString clientIP = m_usbClient->peerAddress().toString();
    QString serverIP = m_usbServer->serverAddress().toString();

    qInfo() << "🔌 USB Client connected from:" << clientIP;
    qDebug() << "🔌 USB Server listening on:" << serverIP << "port:" << m_usbServer->serverPort();

    connect(m_usbClient, &QTcpSocket::readyRead, this, &UsbManager::processUsbData);
    connect(m_usbClient, &QTcpSocket::disconnected, this, &UsbManager::handleUsbDisconnection);
    connect(m_usbClient, &QTcpSocket::stateChanged, this, [](QAbstractSocket::SocketState state) {
        qDebug() << "🔌 USB Socket state changed:" << state;
    });

    m_usbConnected = true;
    emit usbClientConnected();
    emit usbStatusChanged("USB Client connected from " + clientIP);

    // 🔹 ОТПРАВЛЯЕМ ТЕСТОВОЕ СООБЩЕНИЕ ДЛЯ ПРОВЕРКИ СВЯЗИ
    QByteArray testMessage = "USB_SERVER_READY";
    sendUsbData(testMessage);
}

void UsbManager::handleUsbDisconnection()
{
    if (m_usbClient) {
        qInfo() << "🔌 USB Client disconnected";
        m_usbClient->deleteLater();
        m_usbClient = nullptr;
    }

    m_usbConnected = false;
    emit usbClientDisconnected();
    emit usbStatusChanged("USB Client disconnected");
}

// 🔹 ОБРАБОТКА ДАННЫХ USB С РАЗДЕЛЕНИЕМ ПО ТИПАМ
void UsbManager::processUsbData()
{
    if (!m_usbClient || !m_usbConnected) {
        return;
    }

    QByteArray data = m_usbClient->readAll();
    if (data.isEmpty()) {
        return;
    }

    static QByteArray usbBuffer;
    usbBuffer.append(data);

    while (usbBuffer.size() >= 13) {
        uchar dataType = static_cast<uchar>(usbBuffer[0]);
        quint64 sequenceNumber = qFromBigEndian(*reinterpret_cast<const quint64*>(usbBuffer.constData() + 1));
        quint32 frameSize = qFromBigEndian(*reinterpret_cast<const quint32*>(usbBuffer.constData() + 9));

        if (usbBuffer.size() < 13 + (int)frameSize) {
            return;
        }

        QByteArray payload = usbBuffer.mid(13, frameSize);
        usbBuffer.remove(0, 13 + frameSize);

        switch (dataType) {
        case 0x01: // RGB данные камеры
            emit usbDataReceived(payload, sequenceNumber);
            break;

        case 0x02: // LiDAR Depth
            qDebug() << "🎯 usbLidarDataReceived";
            emit usbLidarDataReceived(payload, sequenceNumber);
            break;

        case 0x03: // Raw IMU
            emit usbSensorDataReceived(payload, sequenceNumber);
            break;

        case 0x08: // Raw LiDAR Point Cloud
            // Можно добавить отдельный сигнал или использовать существующий
            emit usbRawLidarPointCloudReceived(payload, sequenceNumber);
            break;

        case 0x09: // LiDAR Confidence Map
            // Можно добавить отдельный сигнал или использовать существующий
            emit usbLidarConfidenceMapReceived(payload, sequenceNumber);
            break;

        default:
            qWarning() << "⚠️ Unknown USB data type:" << dataType;
                break;
        }
    }
}

// 🔹 ОТПРАВКА ДАННЫХ ЧЕРЕЗ USB
void UsbManager::sendUsbData(const QByteArray &data)
{
    if (m_usbClient && m_usbConnected &&
        m_usbClient->state() == QAbstractSocket::ConnectedState) {

        qint64 bytesWritten = m_usbClient->write(data);
        if (bytesWritten == -1) {
            qWarning() << "❌ Ошибка отправки USB данных:" << m_usbClient->errorString();
        } else {
            qDebug() << "📤 USB отправлены данные:" << bytesWritten << "байт";
        }
    } else {
        qWarning() << "⚠️ USB клиент не подключен для отправки данных";
    }
}

bool UsbManager::isUsbConnected() const
{
    return m_usbConnected;
}

void UsbManager::checkUsbConnection()
{
    if (m_usbClient && m_usbClient->state() != QAbstractSocket::ConnectedState) {
        handleUsbDisconnection();
    }
}

void UsbManager::setupUsbNetwork()
{
    // Пустая реализация - теперь настройка делается через NetworkConfigurator
}

// 🔹 ОБРАБОТЧИКИ СТАТУСА СЕТИ
void UsbManager::handleNetworkStatus(const QString &status)
{
    qDebug() << "🔧 Network status:" << status;
    emit usbStatusChanged(status);
}

void UsbManager::handleUsbInterfaceDetected(bool detected)
{
    if (detected) {
        qDebug() << "✅ USB Ethernet interface detected";
        QString localIP = getLocalPCIP();
        emit usbStatusChanged("USB Ethernet interface ready - IP: " + localIP);

        // 🔹 ПЕРЕЗАПУСКАЕМ СЕРВЕР НА ПРАВИЛЬНОМ ИНТЕРФЕЙСЕ
        if (m_usbServer && m_usbServer->isListening()) {
            m_usbServer->close();
            QString newIP = getLocalPCIP();
            if (m_usbServer->listen(QHostAddress(newIP), usbPort)) {
                qDebug() << "✅ USB Server restarted on USB interface:" << newIP;
                emit usbStatusChanged("USB Server restarted on " + newIP);
            }
        }
    } else {
        qDebug() << "⏳ USB Ethernet interface not detected";
        emit usbStatusChanged("USB Ethernet interface not detected");
    }
}

// 🔹 МЕТОД ДЛЯ РУЧНОГО ОБНОВЛЕНИЯ IP
void UsbManager::refreshNetworkInterface()
{
    QString currentIP = getLocalPCIP();
    qDebug() << "🔄 Обновление сетевого интерфейса. Текущий IP:" << currentIP;
    emit usbStatusChanged("Network refreshed - IP: " + currentIP);

    // Перезапускаем сервер с новым IP
    if (m_usbServer && m_usbServer->isListening()) {
        stopUsbServer();
        startUsbServer();
    }
}
