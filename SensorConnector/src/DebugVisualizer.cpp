#include "DebugVisualizer.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

QImage DebugVisualizer::visualizeOrientation(const glm::quat &rotation, int size)
{
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(QColor(20, 20, 30));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int centerX = size / 2;
    int centerY = size / 2;
    
    // Рисуем куб с ориентацией
    drawCube(image, rotation, centerX, centerY, size / 3);
    
    // Оси координат
    painter.setPen(QPen(Qt::red, 2));
    painter.drawLine(centerX, centerY, centerX + 50, centerY); // X (red)
    
    painter.setPen(QPen(Qt::green, 2));
    painter.drawLine(centerX, centerY, centerX, centerY - 50); // Y (green)
    
    painter.setPen(QPen(Qt::blue, 2));
    painter.drawLine(centerX, centerY, centerX + 30, centerY + 30); // Z (blue)
    
    // Текст
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10));
    painter.drawText(10, 20, QString("Rotation: %1°").arg(
        static_cast<int>(glm::degrees(glm::angle(rotation)))
    ));
    
    return image;
}

QImage DebugVisualizer::visualizePosition(const glm::vec3 &position, int size)
{
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(QColor(20, 20, 30));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int centerX = size / 2;
    int centerY = size / 2;
    
    // Сетка
    drawGrid(image, 50);
    
    // Масштаб: 1 метр = 50 пикселей
    int scale = 50;
    int posX = centerX + static_cast<int>(position.x * scale);
    int posY = centerY - static_cast<int>(position.z * scale); // Z вверх
    
    // Рисуем позицию
    painter.setPen(QPen(Qt::cyan, 2));
    painter.setBrush(QBrush(Qt::cyan));
    painter.drawEllipse(posX - 5, posY - 5, 10, 10);
    
    // Стрелка от центра
    painter.setPen(QPen(Qt::yellow, 1));
    painter.drawLine(centerX, centerY, posX, posY);
    
    // Текст
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10));
    painter.drawText(10, 20, QString("Pos: (%.2f, %.2f, %.2f)m")
        .arg(position.x).arg(position.y).arg(position.z));
    painter.drawText(10, 35, QString("Distance: %.2f m").arg(glm::length(position)));
    
    return image;
}

QImage DebugVisualizer::visualizeLiDARPoints(const std::vector<glm::vec3> &points, int size)
{
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(QColor(20, 20, 30));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int centerX = size / 2;
    int centerY = size / 2;
    
    // Сетка
    drawGrid(image, 100);
    
    if (points.empty()) {
        painter.setPen(Qt::red);
        painter.setFont(QFont("Arial", 12));
        painter.drawText(10, size / 2, "No LiDAR data");
        return image;
    }
    
    // Масштаб: 1 метр = 100 пикселей
    int scale = 100;
    
    // Найти min/max для нормализации цвета по глубине
    float minZ = points[0].z, maxZ = points[0].z;
    for (const auto &p : points) {
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }
    float zRange = maxZ - minZ + 0.001f;
    
    // Рисуем точки (top-down view: X-Z)
    for (const auto &p : points) {
        int x = centerX + static_cast<int>(p.x * scale);
        int y = centerY + static_cast<int>(p.z * scale);
        
        // Цвет по глубине (красный близко, синий далеко)
        float depth = (p.z - minZ) / zRange;
        int r = static_cast<int>(255 * (1.0f - depth));
        int b = static_cast<int>(255 * depth);
        
        painter.setPen(QPen(QColor(r, 100, b), 2));
        painter.drawPoint(x, y);
    }
    
    // Текст
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10));
    painter.drawText(10, 20, QString("LiDAR Points: %1").arg(points.size()));
    painter.drawText(10, 35, QString("Depth: %.2f - %.2f m").arg(minZ).arg(maxZ));
    
    return image;
}

QImage DebugVisualizer::visualizeIMUData(float gyroX, float gyroY, float gyroZ,
                                         float accelX, float accelY, float accelZ,
                                         int size)
{
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(QColor(20, 20, 30));
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int centerX = size / 2;
    int centerY = size / 2;
    
    // Масштаб для визуализации
    float gyroScale = 50.0f;  // rad/s
    float accelScale = 30.0f; // m/s²
    
    // Рисуем гироскоп (красный)
    painter.setPen(QPen(Qt::red, 2));
    drawArrow(image, centerX, centerY,
              centerX + static_cast<int>(gyroX * gyroScale),
              centerY - static_cast<int>(gyroY * gyroScale),
              Qt::red);
    
    // Рисуем акселерометр (зелёный)
    painter.setPen(QPen(Qt::green, 2));
    drawArrow(image, centerX, centerY,
              centerX + static_cast<int>(accelX * accelScale),
              centerY - static_cast<int>(accelY * accelScale),
              Qt::green);
    
    // Центр
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QBrush(Qt::white));
    painter.drawEllipse(centerX - 3, centerY - 3, 6, 6);
    
    // Текст
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, 20, QString("Gyro: (%.2f, %.2f, %.2f) rad/s")
        .arg(gyroX).arg(gyroY).arg(gyroZ));
    painter.drawText(10, 35, QString("Accel: (%.2f, %.2f, %.2f) m/s²")
        .arg(accelX).arg(accelY).arg(accelZ));
    
    return image;
}

QImage DebugVisualizer::visualizeDebugPanel(
    const glm::quat &rotation,
    const glm::vec3 &position,
    const std::vector<glm::vec3> &lidarPoints,
    float gyroX, float gyroY, float gyroZ,
    float accelX, float accelY, float accelZ,
    int width, int height)
{
    QImage panel(width, height, QImage::Format_RGB32);
    panel.fill(QColor(10, 10, 20));
    
    QPainter painter(&panel);
    
    int panelW = width / 2;
    int panelH = height / 2;
    
    // Верхний левый: Ориентация
    QImage rotImage = visualizeOrientation(rotation, panelW);
    painter.drawImage(0, 0, rotImage);
    
    // Верхний правый: Позиция
    QImage posImage = visualizePosition(position, panelW);
    painter.drawImage(panelW, 0, posImage);
    
    // Нижний левый: LiDAR
    QImage lidarImage = visualizeLiDARPoints(lidarPoints, panelW);
    painter.drawImage(0, panelH, lidarImage);
    
    // Нижний правый: IMU
    QImage imuImage = visualizeIMUData(gyroX, gyroY, gyroZ, accelX, accelY, accelZ, panelW);
    painter.drawImage(panelW, panelH, imuImage);
    
    return panel;
}

void DebugVisualizer::drawCube(QImage &image, const glm::quat &rotation, int centerX, int centerY, int size)
{
    // Простой куб (8 вершин)
    glm::vec3 vertices[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
    };
    
    // Применяем ротацию
    for (int i = 0; i < 8; ++i) {
        vertices[i] = rotation * vertices[i];
        vertices[i] *= size;
    }
    
    // Рисуем рёбра
    QPainter painter(&image);
    painter.setPen(QPen(Qt::cyan, 1));
    
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // front
        {4,5}, {5,6}, {6,7}, {7,4}, // back
        {0,4}, {1,5}, {2,6}, {3,7}  // sides
    };
    
    for (int i = 0; i < 12; ++i) {
        int v1 = edges[i][0];
        int v2 = edges[i][1];
        
        int x1 = centerX + static_cast<int>(vertices[v1].x);
        int y1 = centerY - static_cast<int>(vertices[v1].y);
        int x2 = centerX + static_cast<int>(vertices[v2].x);
        int y2 = centerY - static_cast<int>(vertices[v2].y);
        
        painter.drawLine(x1, y1, x2, y2);
    }
}

void DebugVisualizer::drawArrow(QImage &image, int x1, int y1, int x2, int y2, const QColor &color)
{
    QPainter painter(&image);
    painter.setPen(QPen(color, 2));
    painter.drawLine(x1, y1, x2, y2);
    
    // Стрелка
    float angle = std::atan2(y2 - y1, x2 - x1);
    int arrowSize = 8;
    
    int x3 = x2 - static_cast<int>(arrowSize * std::cos(angle - 0.4f));
    int y3 = y2 - static_cast<int>(arrowSize * std::sin(angle - 0.4f));
    int x4 = x2 - static_cast<int>(arrowSize * std::cos(angle + 0.4f));
    int y4 = y2 - static_cast<int>(arrowSize * std::sin(angle + 0.4f));
    
    painter.drawLine(x2, y2, x3, y3);
    painter.drawLine(x2, y2, x4, y4);
}

void DebugVisualizer::drawGrid(QImage &image, int cellSize)
{
    QPainter painter(&image);
    painter.setPen(QPen(QColor(60, 60, 80), 1));
    
    for (int x = 0; x < image.width(); x += cellSize) {
        painter.drawLine(x, 0, x, image.height());
    }
    for (int y = 0; y < image.height(); y += cellSize) {
        painter.drawLine(0, y, image.width(), y);
    }
}
