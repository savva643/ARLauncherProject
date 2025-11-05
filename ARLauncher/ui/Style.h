#ifndef STYLE_H
#define STYLE_H

#include <glm/glm.hpp>
#include <string>

/**
 * @brief AR стиль для UI элементов
 * Полупрозрачные, стеклянные эффекты для AR лаунчера
 */
struct Style {
    glm::vec4 backgroundColor;      // Цвет фона (RGBA)
    glm::vec4 textColor;            // Цвет текста
    glm::vec4 borderColor;          // Цвет рамки
    float borderWidth;               // Толщина рамки
    float borderRadius;             // Скругление углов
    float fontSize;                  // Размер шрифта
    std::string fontFamily;          // Семейство шрифта
    float opacity;                  // Прозрачность (0.0-1.0)
    float blurAmount;               // Размытие для glass эффекта
    bool glassEffect;               // Включить glass эффект
    
    // AR стили по умолчанию
    static Style createARButtonStyle() {
        Style style;
        style.backgroundColor = glm::vec4(0.15f, 0.15f, 0.25f, 0.7f); // Полупрозрачный синий
        style.textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.borderColor = glm::vec4(0.3f, 0.5f, 0.8f, 0.9f); // Светло-синий
        style.borderWidth = 2.0f;
        style.borderRadius = 8.0f;
        style.fontSize = 16.0f;
        style.fontFamily = "Arial";
        style.opacity = 0.85f;
        style.blurAmount = 0.5f;
        style.glassEffect = true;
        return style;
    }
    
    static Style createARTextStyle() {
        Style style;
        style.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Прозрачный
        style.textColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.95f); // Почти непрозрачный белый
        style.borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.borderWidth = 0.0f;
        style.borderRadius = 0.0f;
        style.fontSize = 14.0f;
        style.fontFamily = "Arial";
        style.opacity = 0.95f;
        style.blurAmount = 0.0f;
        style.glassEffect = false;
        return style;
    }
    
    static Style createARWindowStyle() {
        Style style;
        style.backgroundColor = glm::vec4(0.1f, 0.1f, 0.15f, 0.6f); // Полупрозрачный темный
        style.textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.borderColor = glm::vec4(0.4f, 0.6f, 0.9f, 0.8f); // Синий
        style.borderWidth = 1.5f;
        style.borderRadius = 12.0f;
        style.fontSize = 14.0f;
        style.fontFamily = "Arial";
        style.opacity = 0.75f;
        style.blurAmount = 0.8f;
        style.glassEffect = true;
        return style;
    }
    
    Style() : backgroundColor(0.2f, 0.2f, 0.2f, 0.8f),
              textColor(1.0f, 1.0f, 1.0f, 1.0f),
              borderColor(0.5f, 0.5f, 0.5f, 1.0f),
              borderWidth(1.0f),
              borderRadius(4.0f),
              fontSize(14.0f),
              fontFamily("Arial"),
              opacity(0.8f),
              blurAmount(0.0f),
              glassEffect(false) {}
};

#endif // STYLE_H

