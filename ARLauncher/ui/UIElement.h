#ifndef UIELEMENT_H
#define UIELEMENT_H

#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <vector>
#include <memory>

class Style;

/**
 * @brief Базовый класс UI элемента
 */
class UIElement {
public:
    UIElement();
    virtual ~UIElement();

    // Позиция и размер
    void setPosition(const glm::vec2& position);
    void setSize(const glm::vec2& size);
    glm::vec2 getPosition() const { return m_position; }
    glm::vec2 getSize() const { return m_size; }
    
    // Видимость
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    // Стиль
    void setStyle(std::shared_ptr<Style> style);
    std::shared_ptr<Style> getStyle() const { return m_style; }
    
    // Рендеринг
    virtual void render() = 0;
    
    // Обработка событий
    virtual bool handleMouseClick(const glm::vec2& position);
    virtual bool handleMouseMove(const glm::vec2& position);
    virtual bool handleKeyPress(int key, int action);
    
    // Проверка попадания
    bool containsPoint(const glm::vec2& point) const;

protected:
    glm::vec2 m_position;
    glm::vec2 m_size;
    bool m_visible;
    std::shared_ptr<Style> m_style;
};

#endif // UIELEMENT_H

