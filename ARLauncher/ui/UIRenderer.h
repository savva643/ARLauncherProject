#ifndef UIRENDERER_H
#define UIRENDERER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

// Forward declarations
class Renderer;
class UIElement;

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

private:
    Renderer* m_renderer;
    std::vector<std::shared_ptr<UIElement>> m_elements;
    
    // Шейдеры и текстуры для UI
    uint32_t m_uiShaderProgram;
    uint32_t m_uiTextureAtlas;
    
    void setupUIResources();
    void renderElement(UIElement* element);
};

#endif // UIRENDERER_H

