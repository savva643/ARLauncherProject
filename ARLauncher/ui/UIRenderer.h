#ifndef UIRENDERER_H
#define UIRENDERER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

#ifdef USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

// Forward declarations
class Renderer;
class UIElement;
class LiDARVisualizer;

/**
 * @brief Рендерер UI элементов
 * 
 * Отрисовывает кастомные UI элементы поверх 3D сцены
 */
class UIRenderer {
public:
    UIRenderer(Renderer* renderer);
    ~UIRenderer();

    bool initialize();
    void shutdown();
    
    // Рендеринг
    void beginFrame();
    void render();
    void endFrame();
    
    // Управление элементами
    void addElement(std::shared_ptr<UIElement> element);
    void removeElement(std::shared_ptr<UIElement> element);
    
    // Обработка ввода
    void handleMouseClick(float x, float y);
    void handleMouseMove(float x, float y);
    void handleKeyPress(int key, int action);
    
    // LiDAR визуализация
    void setLiDARVisualizer(LiDARVisualizer* lidar) { m_lidarVisualizer = lidar; }

private:
    Renderer* m_renderer;
    std::vector<std::shared_ptr<UIElement>> m_elements;
    
    // Шейдеры и текстуры для UI
    uint32_t m_uiShaderProgram;
    uint32_t m_uiTextureAtlas;

    std::vector<uint8_t> m_uiRgba;
    uint32_t m_uiTextureId;
    uint32_t m_uiWidth;
    uint32_t m_uiHeight;
    bool m_isVulkan;
    
    LiDARVisualizer* m_lidarVisualizer = nullptr;

#ifdef USE_FREETYPE
    FT_Library m_ftLibrary;
    FT_Face m_ftFace;
    bool m_ftInitialized;
#endif
    
    void setupUIResources();
    void renderElement(UIElement* element);
};

#endif // UIRENDERER_H

