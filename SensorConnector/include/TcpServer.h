#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QImage>
#include <QMap>
#include <QElapsedTimer>

class TcpServer : public QTcpServer {
    Q_OBJECT

public:
    explicit TcpServer(QObject *parent = nullptr);
    void startServer(quint16 port);

signals:
    void newMessage(const QByteArray &data);
    void frameReceived(const QImage &frame);       // кадр
    void debugUpdate(int fps, double kbps);        // FPS + скорость

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void processData(QTcpSocket *socket);

private:
    QList<QTcpSocket *> clients;

    QMap<QTcpSocket*, QByteArray> bufferMap; // буфер по клиенту
    QElapsedTimer timer;                      // для FPS
    int framesCount = 0;
    qint64 totalBytes = 0;
};

#endif // TCPSERVER_H
