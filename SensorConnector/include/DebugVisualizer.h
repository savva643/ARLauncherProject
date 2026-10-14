#ifndef DEBUGVISUALIZER_H
#define DEBUGVISUALIZER_H

#include <QString>
#include <QImage>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * @brief Визуализация для отладки ротации, позиции и LiDAR
 */
class DebugVisualizer {
public:
    // Визуализация ориентации (quaternion → 3D cube)
    static QImage visualizeOrientation(const glm::quat &rotation, int size = 256);
    
    // Визуализация позиции в 3D пространстве
    static QImage visualizePosition(const glm::vec3 &position, int size = 256);
    
    // Визуализация LiDAR точек (top-down view)
    static QImage visualizeLiDARPoints(const std::vector<glm::vec3> &points, int size = 512);
    
    // Визуализация IMU данных (gyro, accel)
    static QImage visualizeIMUData(float gyroX, float gyroY, float gyroZ,
                                   float accelX, float accelY, float accelZ,
                                   int size = 256);
    
    // Комбинированная визуализация (все вместе)
    static QImage visualizeDebugPanel(
        const glm::quat &rotation,
        const glm::vec3 &position,
        const std::vector<glm::vec3> &lidarPoints,
        float gyroX, float gyroY, float gyroZ,
        float accelX, float accelY, float accelZ,
        int width = 1024, int height = 768
    );

private:
    static void drawCube(QImage &image, const glm::quat &rotation, int centerX, int centerY, int size);
    static void drawArrow(QImage &image, int x1, int y1, int x2, int y2, const QColor &color);
    static void drawGrid(QImage &image, int cellSize);
};

#endif // DEBUGVISUALIZER_H
