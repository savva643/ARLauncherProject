#include "Button.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <iostream>

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
    
    // Цвет кнопки
    if (m_pressed) {
        glColor3f(0.3f, 0.3f, 0.3f);
    } else if (m_hovered) {
        glColor3f(0.5f, 0.5f, 0.5f);
    } else {
        glColor3f(0.4f, 0.4f, 0.4f);
    }
    
    // Рисуем прямоугольник
    glBegin(GL_QUADS);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + m_size.y);
    glVertex2f(m_position.x, m_position.y + m_size.y);
    glEnd();
    
    // Рамка
    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(2.0f);
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

