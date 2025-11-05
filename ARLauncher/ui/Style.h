#ifndef STYLE_H
#define STYLE_H

#include <glm/glm.hpp>

/**
 * @brief Стиль UI элемента
 */
struct Style {
    glm::vec4 backgroundColor;
    glm::vec4 textColor;
    glm::vec4 borderColor;
    float borderWidth;
    float borderRadius;
    float fontSize;
    std::string fontFamily;
    
    Style() : backgroundColor(0.2f, 0.2f, 0.2f, 0.8f),
              textColor(1.0f, 1.0f, 1.0f, 1.0f),
              borderColor(0.5f, 0.5f, 0.5f, 1.0f),
              borderWidth(1.0f),
              borderRadius(4.0f),
              fontSize(14.0f),
              fontFamily("Arial") {}
};

#endif // STYLE_H

