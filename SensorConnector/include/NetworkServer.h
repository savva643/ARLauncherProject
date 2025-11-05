#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QImage>
#include <QMap>
#include <QHash>
#include <QtEndian>
#include <QThread>
#include "UsbManager.h"
#include "TurboJPEGDecoder.h"
#include "ffmpegdecoder.h"
#include "Lidar3DProcessor.h"
#include <QVector3D>
#include "ARCameraController.h"
#include "ardataprocessor.h"
#include "lensenginetypes.h"

class NetworkServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY statusChanged)
    Q_PROPERTY(int clientsCount READ clientsCount NOTIFY clientsCountChanged)
    Q_PROPERTY(int fps READ fps NOTIFY debugUpdate)
    Q_PROPERTY(double speed READ speed NOTIFY debugUpdate)
    Q_PROPERTY(QString connectionType READ connectionType NOTIFY debugUpdate)
    Q_PROPERTY(int lidarFps READ lidarFps NOTIFY lidarDebugUpdate)
    Q_PROPERTY(double lidarSpeed READ lidarSpeed NOTIFY lidarDebugUpdate)

public:
    enum ConnectionType { TCP, UDP, USB };
    Q_ENUM(ConnectionType)

    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    Q_INVOKABLE void startServers(quint16 tcpPort, quint16 udpPort);
    Q_INVOKABLE void stopServers();
    Q_INVOKABLE void setConnectionType(int type);

    QString serverStatus() const { return m_serverStatus; }
    int clientsCount() const { return m_clientsCount; }
    int fps() const { return m_fps; }
    double speed() const { return m_speed; }
    QString connectionType() const { return m_currentConnectionTypeStr; }
    int lidarFps() const { return m_lidarFps; }
    double lidarSpeed() const { return m_lidarSpeed; }

    // 🔹 ДОСТУП К ПОСЛЕДНИМ ДАННЫМ ДЛЯ ИНИЦИАЛИЗАЦИИ В 3D РЕЖИМЕ
    Q_INVOKABLE QVariantMap getLastCameraPose() const {
        QVariantMap pose;
        pose["position"] = QVariant::fromValue(m_lastCameraPosition);
        QVariantMap rot;
        rot["w"] = m_lastCameraRotation.scalar();
        rot["x"] = m_lastCameraRotation.x();
        rot["y"] = m_lastCameraRotation.y();
        rot["z"] = m_lastCameraRotation.z();
        pose["rotation"] = rot;
        return pose;
    }
    Q_INVOKABLE QVariantList getLastFeaturePoints() const { return m_lastFeaturePoints; }
    Q_INVOKABLE QVariantList getLastLidarPoints() const { return m_lastLidarPoints; }

signals:
    void statusChanged(const QString &status);
    void clientsCountChanged(int count);
    void frameReceived(const QImage &frame);
    void lidarFrameReceived(const QImage &frame);
    void debugUpdate(int fps, double kbps, const QString &type);
    void lidarDebugUpdate(int fps, double kbps);

    void lidarPointsUpdated(const QVariantList &points);
    void objectsDetected(const QVariantList &objectPositions);
    void lidarStatisticsUpdated(float avgDepth, float maxDepth, int pointCount, int objectCount);

    void sensorDataUpdated(float pitch, float yaw, float roll,
                           float accelX, float accelY, float accelZ);
    void cameraPoseUpdated(const QVector3D &position, const QQuaternion &rotation);

    void featurePointsUpdated(const QVariantList &points);
    void cameraIntrinsicsUpdated(const QVariantMap &intrinsics);
    void lightEstimationUpdated(const QVariantMap &light);
    void fusionStabilityUpdated(float stability);
    void cameraProjectionUpdated(const QMatrix4x4 &projection);

    // 🔹 НОВЫЕ СИГНАЛЫ
    void arFrameProcessed(const LensEngine::ARFrame &frame);
    void lidarDataUpdated(const QVector<QVector3D> &points);
    void imuDataUpdated(double accelX, double accelY, double accelZ,
                        double gyroX, double gyroY, double gyroZ);

private slots:
    // 🔹 ОСНОВНЫЕ СЛОТЫ СЕТИ
    void handleTcpConnection();
    void handleTcpDisconnection();
    void processTcpData();
    void processUdpData();

    // 🔹 USB СЛОТЫ
    void handleUsbClientConnected();
    void handleUsbClientDisconnected();
    void handleUsbStatusChanged(const QString &status);
    void handleUsbData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbLidarData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbSensorData(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber);
    void handleUsbLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber);

    // 🔹 ОБРАБОТКА ДАННЫХ
    void handleTurboImageDecoded(const QImage &image, int dataSize, quint64 sequenceNumber);
    void updateDisplay();
    void cleanupBuffers();

    // 🔹 ОБРАБОТКА РАЗНЫХ ТИПОВ ДАННЫХ
    void processRGBData(const QByteArray &data, quint64 sequenceNumber);
    void processLidarDepthData(const QByteArray &data, quint64 sequenceNumber);
    void processRawIMUData(const QByteArray &data, quint64 sequenceNumber);
    void processRawLidarPointCloud(const QByteArray &data, quint64 sequenceNumber);
    void processLidarConfidenceMap(const QByteArray &data, quint64 sequenceNumber);

    // 🔹 ОБРАБОТЧИКИ ОТ ARDataProcessor
    void onFrameProcessed(const LensEngine::ARFrame &processedFrame);
    void onFeaturePointsUpdated(const QVector<LensEngine::FeaturePoint> &features);
    void onCameraIntrinsicsUpdated(const LensEngine::CameraIntrinsics &intrinsics);
    void onLightEstimationUpdated(const LensEngine::LightEstimation &light);
    void onCameraPoseUpdated(const LensEngine::CameraPose &pose);
    void onSensorDataUpdated(float pitch, float yaw, float roll,
                             float accelX, float accelY, float accelZ);

    // 🔹 СЛОТЫ ДЛЯ LiDAR
    void onLidarSpatialAnalysisCompleted(const Lidar3DProcessor::SpatialAnalysisResult &analysis);
    void onLidarPointsProcessed(const QVector<QVector3D> &points);
    void onFloorDetected(const QVector3D &normal, float height);
    void onObstaclesDetected(const QVector<QVector3D> &obstacles);

private:
    // LiDAR константы
    static constexpr int LIDAR_WIDTH = 256;
    static constexpr int LIDAR_HEIGHT = 192;
    static constexpr int LIDAR_TARGET_WIDTH = 512;
    static constexpr int LIDAR_TARGET_HEIGHT = 384;
    static constexpr int LIDAR_DATA_SIZE = LIDAR_WIDTH * LIDAR_HEIGHT * sizeof(float);

    // 🔹 ОСНОВНЫЕ МЕТОДЫ ОБРАБОТКИ
    void processLidarFrame(const QByteArray &data, quint64 sequenceNumber);
    void deliverFrame(const QImage &img, int dataSize);

    // 🔹 СТАТИСТИКА
    void updateStatistics(int dataSize);
    void updateStatisticsFast(int dataSize);
    void updateLidarStatistics(int dataSize);
    void cleanupExcessFrames(QMap<quint64, QImage> &frameBuffer,
                             QHash<quint64, qint64> &timestamps, // Меняем на QHash
                             int maxFrames);
    // 🔹 LiDAR МЕТОДЫ
    float findMaxDepthFast(const float* depthData);
    void convertDepthToImageFast(const float* depthData, QImage &image, float scale);
    void initLookupTables();

    // 🔹 ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
    bool isLikelyJpegData(const QByteArray &data);
    void createErrorFrame(const QByteArray &data, const QString &error);
    quint64 findBestLidarMatch(quint64 rgbSequence);

    // Серверы и сокеты
    QTcpServer *tcpServer;
    QUdpSocket *udpSocket;

    // Клиенты и буферы
    QList<QTcpSocket*> tcpClients;
    QMap<QTcpSocket*, QByteArray> tcpBuffers;

    // Менеджеры и декодеры
    UsbManager *m_usbManager;
    TurboJPEGDecoder *m_turboDecoder;
    FFmpegDecoder *m_ffmpegDecoder;

    // Состояние
    QString m_serverStatus;
    int m_clientsCount;
    int m_fps;
    double m_speed;
    int m_lidarFps;
    double m_lidarSpeed;
    ConnectionType currentConnectionType;
    QString m_currentConnectionTypeStr;
    bool serversRunning;

    // Буферы и статистика
    QByteArray h264Buffer;
    qint64 totalBytes;
    qint64 lidarTotalBytes;
    int framesCount;
    int lidarFramesCount;
    QElapsedTimer fpsTimer;

    // 🔹 БУФЕРЫ ДЛЯ ИЗОБРАЖЕНИЙ
    QImage lidarDepthImage;
    QImage lidarFallbackImage;
    QMap<quint64, QImage> rgbFrameBuffer;
    QMap<quint64, QImage> lidarFrameBuffer;
    QHash<quint64, qint64> rgbFrameTimestamps; // Меняем на QHash
    QHash<quint64, qint64> lidarFrameTimestamps; // Меняем на QHash

    quint64 lastRgbSequence = 0;
    quint64 lastLidarSequence = 0;

    // 🔹 ТАЙМЕРЫ
    QTimer *displayTimer;
    QTimer *cleanupTimer;

    // 🔹 ПРЕДВАРИТЕЛЬНО ВЫЧИСЛЕННЫЕ ТАБЛИЦЫ ДЛЯ LiDAR
    QVector<int> srcXTable;
    QVector<int> srcYTable;
    QVector<int> srcIndexTable;

    // 🔹 ОСНОВНЫЕ КОМПОНЕНТЫ AR
    Lidar3DProcessor* m_lidar3DProcessor;
    ARCameraController* m_arCameraController;
    ARDataProcessor *m_arDataProcessor;

    // 🔹 ПОТОКИ - ДОБАВЛЯЕМ ОБЪЯВЛЕНИЯ
    QThread *m_imuThread;
    QThread *m_lidarThread;
    QThread *m_arProcessingThread;

    // 🔹 ТЕКУЩИЙ AR КАДР ДЛЯ ОБРАБОТКИ
    LensEngine::ARFrame m_currentARFrame;

    // 🔹 КЭШ ПОСЛЕДНИХ ДАННЫХ
    QVector3D m_lastCameraPosition;
    QQuaternion m_lastCameraRotation;
    QVariantList m_lastFeaturePoints;
    QVariantList m_lastLidarPoints;


};

#endif // NETWORKSERVER_H
