#ifndef WIDGET_H
#define WIDGET_H

#include "UIElement.h"
#include <vector>
#include <memory>

/**
 * @brief Базовый виджет (контейнер для других элементов)
 */
class Widget : public UIElement {
public:
    Widget();
    ~Widget() override;

    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(std::shared_ptr<UIElement> child);
    void clear();
    
    void setLayout(int layout); // 0 = vertical, 1 = horizontal, 2 = grid
    int getLayout() const { return m_layout; }
    
    void setSpacing(float spacing) { m_spacing = spacing; }
    float getSpacing() const { return m_spacing; }
    
    void render() override;
    bool handleMouseClick(const glm::vec2& position) override;
    bool handleMouseMove(const glm::vec2& position) override;
    bool handleKeyPress(int key, int action) override;

protected:
    void updateLayout();

private:
    std::vector<std::shared_ptr<UIElement>> m_children;
    int m_layout; // 0 = vertical, 1 = horizontal, 2 = grid
    float m_spacing;
};

#endif // WIDGET_H

