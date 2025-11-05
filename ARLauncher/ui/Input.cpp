#include "Input.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <iostream>

Input::Input()
    : m_focused(false)
{
}

Input::~Input()
{
}

void Input::render()
{
#ifdef USE_OPENGL
    if (!m_visible) {
        return;
    }
    
    // Простой OpenGL рендеринг поля ввода
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
    
    // Фон поля ввода
    if (m_focused) {
        glColor3f(0.2f, 0.2f, 0.3f);
    } else {
        glColor3f(0.15f, 0.15f, 0.15f);
    }
    
    glBegin(GL_QUADS);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + m_size.y);
    glVertex2f(m_position.x, m_position.y + m_size.y);
    glEnd();
    
    // Рамка
    glColor3f(0.6f, 0.6f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + m_size.y);
    glVertex2f(m_position.x, m_position.y + m_size.y);
    glEnd();
    
    // TODO: Отрисовка текста/плейсхолдера
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
#endif
}

bool Input::handleKeyPress(int key, int action)
{
    if (!m_focused) {
        return false;
    }
    
    // TODO: Обработка ввода текста
    return true;
}

bool Input::handleMouseClick(const glm::vec2& position)
{
    bool clicked = containsPoint(position);
    m_focused = clicked;
    return clicked;
}

