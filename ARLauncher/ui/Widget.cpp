#include "Widget.h"
#include <algorithm>

Widget::Widget()
    : m_layout(0) // vertical
    , m_spacing(5.0f)
{
}

Widget::~Widget()
{
}

void Widget::addChild(std::shared_ptr<UIElement> child)
{
    if (child) {
        m_children.push_back(child);
        updateLayout();
    }
}

void Widget::removeChild(std::shared_ptr<UIElement> child)
{
    m_children.erase(
        std::remove(m_children.begin(), m_children.end(), child),
        m_children.end()
    );
    updateLayout();
}

void Widget::clear()
{
    m_children.clear();
}

void Widget::render()
{
    if (!m_visible) {
        return;
    }
    
    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->render();
        }
    }
}

bool Widget::handleMouseClick(const glm::vec2& position)
{
    if (!m_visible) {
        return false;
    }
    
    for (auto& child : m_children) {
        if (child && child->isVisible() && child->handleMouseClick(position)) {
            return true;
        }
    }
    
    return containsPoint(position);
}

bool Widget::handleMouseMove(const glm::vec2& position)
{
    if (!m_visible) {
        return false;
    }
    
    for (auto& child : m_children) {
        if (child && child->isVisible() && child->handleMouseMove(position)) {
            return true;
        }
    }
    
    return false;
}

bool Widget::handleKeyPress(int key, int action)
{
    if (!m_visible) {
        return false;
    }
    
    for (auto& child : m_children) {
        if (child && child->isVisible() && child->handleKeyPress(key, action)) {
            return true;
        }
    }
    
    return false;
}

void Widget::updateLayout()
{
    if (m_children.empty()) {
        return;
    }
    
    float currentY = m_position.y;
    float currentX = m_position.x;
    float maxWidth = 0.0f;
    
    for (auto& child : m_children) {
        if (!child) {
            continue;
        }
        
        if (m_layout == 0) { // Vertical
            child->setPosition(glm::vec2(m_position.x, currentY));
            currentY += child->getSize().y + m_spacing;
            maxWidth = std::max(maxWidth, child->getSize().x);
        } else if (m_layout == 1) { // Horizontal
            child->setPosition(glm::vec2(currentX, m_position.y));
            currentX += child->getSize().x + m_spacing;
        }
        // TODO: Grid layout
    }
    
    // Обновляем размер виджета
    if (m_layout == 0) {
        m_size = glm::vec2(maxWidth, currentY - m_position.y - m_spacing);
    } else if (m_layout == 1) {
        m_size = glm::vec2(currentX - m_position.x - m_spacing, m_size.y);
    }
}

