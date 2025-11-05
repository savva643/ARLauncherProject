#ifndef TEXT_H
#define TEXT_H

#include "UIElement.h"
#include <string>

/**
 * @brief Текстовый элемент UI
 */
class Text : public UIElement {
public:
    Text(const std::string& text = "");
    ~Text() override;

    void setText(const std::string& text) { m_text = text; }
    std::string getText() const { return m_text; }
    
    void setFontSize(float size);
    float getFontSize() const { return m_fontSize; }
    
    void render() override;

private:
    std::string m_text;
    float m_fontSize;
};

#endif // TEXT_H

