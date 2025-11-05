#ifndef INPUT_H
#define INPUT_H

#include "UIElement.h"
#include <string>
#include <functional>

/**
 * @brief Поле ввода UI
 */
class Input : public UIElement {
public:
    Input();
    ~Input() override;

    void setText(const std::string& text) { m_text = text; }
    std::string getText() const { return m_text; }
    
    void setPlaceholder(const std::string& placeholder) { m_placeholder = placeholder; }
    std::string getPlaceholder() const { return m_placeholder; }
    
    void setOnChange(std::function<void(const std::string&)> callback) { m_onChange = callback; }
    
    void render() override;
    bool handleKeyPress(int key, int action) override;
    bool handleMouseClick(const glm::vec2& position) override;

private:
    std::string m_text;
    std::string m_placeholder;
    std::function<void(const std::string&)> m_onChange;
    bool m_focused;
};

#endif // INPUT_H

