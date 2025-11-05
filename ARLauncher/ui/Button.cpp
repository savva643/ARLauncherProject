#include "Button.h"
#include "Style.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <iostream>
#include <memory>

Button::Button(const std::string& text)
    : m_text(text)
    , m_hovered(false)
    , m_pressed(false)
{
}

Button::~Button()
{
}

void Button::render()
{
#ifdef USE_OPENGL
    if (!m_visible) {
        return;
    }
    
    // Простой OpenGL рендеринг кнопки
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // AR стиль кнопки (полупрозрачный, стеклянный эффект)
    auto style = m_style ? m_style : std::make_shared<Style>(Style::createARButtonStyle());
    
    glm::vec4 bgColor = style->backgroundColor;
    if (m_pressed) {
        bgColor = glm::vec4(bgColor.r * 0.7f, bgColor.g * 0.7f, bgColor.b * 0.7f, bgColor.a);
    } else if (m_hovered) {
        bgColor = glm::vec4(bgColor.r * 1.2f, bgColor.g * 1.2f, bgColor.b * 1.2f, bgColor.a);
    }
    
    glColor4f(bgColor.r, bgColor.g, bgColor.b, bgColor.a * style->opacity);
    
    // Рисуем прямоугольник
    glBegin(GL_QUADS);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + m_size.y);
    glVertex2f(m_position.x, m_position.y + m_size.y);
    glEnd();
    
    // Рамка с AR стилем
    glm::vec4 borderColor = style->borderColor;
    glColor4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a * style->opacity);
    glLineWidth(style->borderWidth);
    glBegin(GL_LINE_LOOP);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + m_size.y);
    glVertex2f(m_position.x, m_position.y + m_size.y);
    glEnd();
    
    // TODO: Отрисовка текста (пока только фон кнопки)
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
#endif
}

bool Button::handleMouseClick(const glm::vec2& position)
{
    if (containsPoint(position)) {
        m_pressed = true;
        if (m_onClick) {
            m_onClick();
        }
        return true;
    }
    m_pressed = false;
    return false;
}

bool Button::handleMouseMove(const glm::vec2& position)
{
    m_hovered = containsPoint(position);
    return m_hovered;
}

