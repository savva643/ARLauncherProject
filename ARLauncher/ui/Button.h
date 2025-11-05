#ifndef BUTTON_H
#define BUTTON_H

#include "UIElement.h"
#include <string>
#include <functional>

/**
 * @brief Кнопка UI
 */
class Button : public UIElement {
public:
    Button(const std::string& text);
    ~Button() override;

    void setText(const std::string& text) { m_text = text; }
    std::string getText() const { return m_text; }
    
    void setOnClick(std::function<void()> callback) { m_onClick = callback; }
    
    void render() override;
    bool handleMouseClick(const glm::vec2& position) override;
    bool handleMouseMove(const glm::vec2& position) override;

private:
    std::string m_text;
    std::function<void()> m_onClick;
    bool m_hovered;
    bool m_pressed;
};

#endif // BUTTON_H

