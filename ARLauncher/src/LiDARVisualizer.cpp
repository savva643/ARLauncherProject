#include "LiDARVisualizer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

LiDARVisualizer::LiDARVisualizer()
    : m_pointCount(0)
    , m_visible(true)
    , m_initialized(false)
    , m_rotationX(0.0f)
    , m_rotationY(0.0f)
    , m_zoom(1.0f)
{
}

LiDARVisualizer::~LiDARVisualizer()
{
}

bool LiDARVisualizer::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    m_initialized = true;
    std::cout << "[LiDARVisualizer] Initialized (2D visualization for Vulkan)" << std::endl;
    return true;
}

void LiDARVisualizer::updatePointCloud(const std::vector<glm::vec3>& points)
{
    std::lock_guard<std::mutex> lock(m_pointsMutex);
    m_points = points;
    m_pointCount = points.size();
}

glm::vec2 LiDARVisualizer::project3DTo2D(const glm::vec3& point, 
                                         int centerX, int centerY,
                                         int width, int height)
{
    // Простая перспективная проекция
    float scale = 100.0f * m_zoom;
    
    // Применяем вращение
    glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), m_rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), m_rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec4 rotatedPoint = rotY * rotX * glm::vec4(point, 1.0f);
    
    // Перспективная проекция
    float z = rotatedPoint.z + 2.0f; // Смещение камеры
    if (z < 0.1f) z = 0.1f;
    
    float x = (rotatedPoint.x / z) * scale + centerX;
    float y = (rotatedPoint.y / z) * scale + centerY;
    
    return glm::vec2(x, y);
}

void LiDARVisualizer::getColorForDepth(float depth, float& r, float& g, float& b)
{
    // Цветовая кодировка по глубине (как на iPhone LiDAR)
    // Используем плавный градиент от синего (близко) к красному (далеко)
    
    // Нормализуем глубину в диапазон 0-1
    float normalized = (depth - 0.1f) / (5.0f - 0.1f);
    normalized = glm::clamp(normalized, 0.0f, 1.0f);
    
    // Создаём плавный градиент: синий -> голубой -> зелёный -> жёлтый -> красный
    if (normalized < 0.25f) {
        // Синий -> Голубой
        float t = normalized / 0.25f;
        r = 0.0f;
        g = t;
        b = 1.0f;
    } else if (normalized < 0.5f) {
        // Голубой -> Зелёный
        float t = (normalized - 0.25f) / 0.25f;
        r = 0.0f;
        g = 1.0f;
        b = 1.0f - t;
    } else if (normalized < 0.75f) {
        // Зелёный -> Жёлтый
        float t = (normalized - 0.5f) / 0.25f;
        r = t;
        g = 1.0f;
        b = 0.0f;
    } else {
        // Жёлтый -> Красный
        float t = (normalized - 0.75f) / 0.25f;
        r = 1.0f;
        g = 1.0f - t;
        b = 0.0f;
    }
}

void LiDARVisualizer::render(int windowWidth, int windowHeight,
                             int miniWindowX, int miniWindowY,
                             int miniWindowWidth, int miniWindowHeight)
{
    if (!m_visible || !m_initialized || m_pointCount == 0) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_pointsMutex);
    
    if (m_points.empty()) {
        return;
    }
    
    // Вычисляем центр мини-окна
    int centerX = miniWindowX + miniWindowWidth / 2;
    int centerY = miniWindowY + miniWindowHeight / 2;
    
    // Рисуем облако точек как 2D проекцию
    // Это простая визуализация - каждая точка рисуется как маленький круг
    
    // Примечание: Для полной интеграции с Vulkan нужно использовать
    // UIRenderer или создать отдельный рендер-проход.
    // Здесь мы просто логируем информацию о точках.
    
    static int renderCounter = 0;
    if (renderCounter++ % 30 == 0) {
        std::cout << "[LiDAR Render] Points: " << m_points.size() 
                  << " | Window: " << miniWindowWidth << "x" << miniWindowHeight
                  << " @ (" << miniWindowX << ", " << miniWindowY << ")" << std::endl;
        
        // Вычисляем статистику облака
        float minDepth = 1000.0f, maxDepth = 0.0f, avgDepth = 0.0f;
        for (const auto& p : m_points) {
            float depth = glm::length(p);
            minDepth = std::min(minDepth, depth);
            maxDepth = std::max(maxDepth, depth);
            avgDepth += depth;
        }
        avgDepth /= m_points.size();
        
        std::cout << "  Depth: min=" << minDepth << "m, max=" << maxDepth 
                  << "m, avg=" << avgDepth << "m" << std::endl;
    }
}

void LiDARVisualizer::renderToBuffer(uint8_t* buffer, int bufferWidth, int bufferHeight,
                                      int miniWindowX, int miniWindowY,
                                      int miniWindowWidth, int miniWindowHeight)
{
    if (!m_visible || !m_initialized || m_pointCount == 0 || !buffer) {
        return;
    }
    
    // Validate buffer dimensions
    if (bufferWidth <= 0 || bufferHeight <= 0 || miniWindowWidth <= 0 || miniWindowHeight <= 0) {
        return;
    }
    
    // Validate mini-window bounds
    if (miniWindowX < 0 || miniWindowY < 0 || 
        miniWindowX + miniWindowWidth > bufferWidth ||
        miniWindowY + miniWindowHeight > bufferHeight) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_pointsMutex);
    
    if (m_points.empty()) {
        return;
    }
    
    // Вычисляем центр мини-окна
    int centerX = miniWindowX + miniWindowWidth / 2;
    int centerY = miniWindowY + miniWindowHeight / 2;
    
    // Рисуем облако точек как 2D проекцию в RGBA буфер
    for (const auto& point : m_points) {
        // Проецируем 3D точку на 2D
        glm::vec2 screenPos = project3DTo2D(point, centerX, centerY, miniWindowWidth, miniWindowHeight);
        
        int x = static_cast<int>(screenPos.x);
        int y = static_cast<int>(screenPos.y);
        
        // Проверяем границы мини-окна
        if (x < miniWindowX || x >= miniWindowX + miniWindowWidth ||
            y < miniWindowY || y >= miniWindowY + miniWindowHeight) {
            continue;
        }
        
        // Получаем цвет по глубине
        float depth = glm::length(point);
        float r, g, b;
        getColorForDepth(depth, r, g, b);
        
        // Рисуем точку (маленький круг 3x3 пикселя)
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int px = x + dx;
                int py = y + dy;
                
                // Строгая проверка границ буфера
                if (px >= 0 && px < bufferWidth && py >= 0 && py < bufferHeight) {
                    size_t pixelIdx = (static_cast<size_t>(py) * static_cast<size_t>(bufferWidth) + static_cast<size_t>(px)) * 4;
                    
                    // Проверяем что индекс не выходит за границы буфера
                    if (pixelIdx + 3 < static_cast<size_t>(bufferWidth * bufferHeight * 4)) {
                        // Смешиваем цвета (alpha blending)
                        float alpha = 0.8f;
                        buffer[pixelIdx + 0] = static_cast<uint8_t>(r * 255 * alpha + buffer[pixelIdx + 0] * (1 - alpha));
                        buffer[pixelIdx + 1] = static_cast<uint8_t>(g * 255 * alpha + buffer[pixelIdx + 1] * (1 - alpha));
                        buffer[pixelIdx + 2] = static_cast<uint8_t>(b * 255 * alpha + buffer[pixelIdx + 2] * (1 - alpha));
                        buffer[pixelIdx + 3] = 255;
                    }
                }
            }
        }
    }
    
    static int renderCounter = 0;
    if (renderCounter++ % 30 == 0) {
        std::cout << "[LiDAR Render] Points: " << m_points.size() 
                  << " | Window: " << miniWindowWidth << "x" << miniWindowHeight << std::endl;
    }
}

void LiDARVisualizer::clear()
{
    std::lock_guard<std::mutex> lock(m_pointsMutex);
    m_points.clear();
    m_pointCount = 0;
}
