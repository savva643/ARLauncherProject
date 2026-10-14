#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>

/**
 * @brief Визуализатор облака точек LiDAR для Vulkan
 * Отображает мини-окно с облаком точек поверх RGB видео
 * Использует простую 2D визуализацию (проекция на экран)
 */
class LiDARVisualizer {
public:
    LiDARVisualizer();
    ~LiDARVisualizer();
    
    /**
     * @brief Инициализация визуализатора
     */
    bool initialize();
    
    /**
     * @brief Обновить облако точек LiDAR
     * @param points Вектор 3D точек (x, y, z)
     */
    void updatePointCloud(const std::vector<glm::vec3>& points);
    
    /**
     * @brief Отрисовать облако точек в мини-окне (2D визуализация)
     * @param windowWidth Ширина окна
     * @param windowHeight Высота окна
     * @param miniWindowX X координата мини-окна (в пикселях)
     * @param miniWindowY Y координата мини-окна (в пикселях)
     * @param miniWindowWidth Ширина мини-окна
     * @param miniWindowHeight Высота мини-окна
     */
    void render(int windowWidth, int windowHeight, 
                int miniWindowX, int miniWindowY,
                int miniWindowWidth, int miniWindowHeight);
    
    /**
     * @brief Отрисовать облако точек в RGBA буфер (для Vulkan)
     * @param buffer RGBA буфер (4 байта на пиксель)
     * @param bufferWidth Ширина буфера
     * @param bufferHeight Высота буфера
     * @param miniWindowX X координата мини-окна
     * @param miniWindowY Y координата мини-окна
     * @param miniWindowWidth Ширина мини-окна
     * @param miniWindowHeight Высота мини-окна
     */
    void renderToBuffer(uint8_t* buffer, int bufferWidth, int bufferHeight,
                       int miniWindowX, int miniWindowY,
                       int miniWindowWidth, int miniWindowHeight);
    
    /**
     * @brief Получить количество точек в облаке
     */
    size_t getPointCount() const { return m_pointCount; }
    
    /**
     * @brief Очистить облако точек
     */
    void clear();
    
    /**
     * @brief Установить видимость
     */
    void setVisible(bool visible) { m_visible = visible; }
    
    /**
     * @brief Получить видимость
     */
    bool isVisible() const { return m_visible; }
    
private:
    std::vector<glm::vec3> m_points;
    size_t m_pointCount = 0;
    
    std::mutex m_pointsMutex;
    bool m_visible = true;
    bool m_initialized = false;
    
    // Параметры для 2D визуализации
    float m_rotationX = 0.0f;
    float m_rotationY = 0.0f;
    float m_zoom = 1.0f;
    
    /**
     * @brief Проецировать 3D точку на 2D экран
     */
    glm::vec2 project3DTo2D(const glm::vec3& point, 
                           int centerX, int centerY,
                           int width, int height);
    
    /**
     * @brief Получить цвет точки по глубине
     */
    void getColorForDepth(float depth, float& r, float& g, float& b);
};
