#include "Window.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <algorithm>

Window::Window(const std::string& title)
    : m_title(title)
    , m_minimized(false)
    , m_maximized(false)
    , m_dragging(false)
    , m_dragOffset(0.0f)
{
}

Window::~Window()
{
}

void Window::addChild(std::shared_ptr<UIElement> child)
{
    if (child) {
        m_children.push_back(child);
    }
}

void Window::removeChild(std::shared_ptr<UIElement> child)
{
    m_children.erase(
        std::remove(m_children.begin(), m_children.end(), child),
        m_children.end()
    );
}

void Window::render()
{
    if (!m_visible) {
        return;
    }
    
    renderTitleBar();
    
    if (!m_minimized) {
        renderContent();
    }
}

bool Window::handleMouseClick(const glm::vec2& position)
{
    if (!m_visible) {
        return false;
    }
    
    // Проверка заголовка для перетаскивания
    if (position.y >= m_position.y && position.y <= m_position.y + 30.0f) {
        m_dragging = true;
        m_dragOffset = position - m_position;
        return true;
    }
    
    // Передача события дочерним элементам
    glm::vec2 localPos = position - m_position;
    for (auto& child : m_children) {
        if (child && child->handleMouseClick(localPos)) {
            return true;
        }
    }
    
    return containsPoint(position);
}

bool Window::handleMouseMove(const glm::vec2& position)
{
    if (m_dragging) {
        m_position = position - m_dragOffset;
        return true;
    }
    
    glm::vec2 localPos = position - m_position;
    for (auto& child : m_children) {
        if (child && child->handleMouseMove(localPos)) {
            return true;
        }
    }
    
    return false;
}

void Window::renderTitleBar()
{
#ifdef USE_OPENGL
    // Простой OpenGL рендеринг заголовка
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
    
    // Фон заголовка
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(m_position.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y);
    glVertex2f(m_position.x + m_size.x, m_position.y + 30.0f);
    glVertex2f(m_position.x, m_position.y + 30.0f);
    glEnd();
    
    // TODO: Отрисовка текста заголовка
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
#endif
}

void Window::renderContent()
{
    if (m_minimized) {
        return;
    }
    
    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->render();
        }
    }
}

