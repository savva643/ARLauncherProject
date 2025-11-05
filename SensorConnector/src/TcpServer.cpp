#include "TcpServer.h"
#include <QBuffer>
#include <QImageReader>
#include <QDebug>
#include <QtEndian>

TcpServer::TcpServer(QObject *parent)
    : QTcpServer(parent)
{
    timer.start();
    totalBytes = 0;
    framesCount = 0;
}

void TcpServer::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        qWarning() << "⚠️ Не удалось запустить сервер:" << errorString();
    } else {
        qInfo() << "✅ Сервер запущен на порту" << port;
    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor) {
    auto *socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "⚠️ Ошибка соединения:" << socket->errorString();
                                                                         socket->deleteLater();
        return;
    }

    qInfo() << "📡 Новое подключение:" << socketDescriptor;
        clients.append(socket);

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        processData(socket);
    });

    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        qInfo() << "❌ Клиент отключился";
        clients.removeAll(socket);
        socket->deleteLater();
        bufferMap.remove(socket);
    });
}

void TcpServer::processData(QTcpSocket *socket) {
    bufferMap[socket].append(socket->readAll());
    QByteArray &buffer = bufferMap[socket];

    while (buffer.size() >= 4) {
        quint32 size;
        memcpy(&size, buffer.constData(), 4);
        size = qFromBigEndian(size);

        if (buffer.size() < 4 + int(size))
            return;

        QByteArray imgData = buffer.mid(4, size);
        buffer.remove(0, 4 + size);

        totalBytes += imgData.size();

        QImage img;
        img.loadFromData(imgData, "JPEG");

        if (!img.isNull()) {
            emit frameReceived(img);

            // FPS и скорость
            framesCount++;
            qint64 elapsed = timer.elapsed();
            if (elapsed >= 1000) {
                emit debugUpdate(framesCount, (totalBytes/1024.0)/(elapsed/1000.0));
                framesCount = 0;
                totalBytes = 0;
                timer.restart();
            }
        }
    }
}
