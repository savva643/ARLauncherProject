#include "UIElement.h"
#include "Style.h"

UIElement::UIElement()
    : m_position(0.0f)
    , m_size(100.0f, 30.0f)
    , m_visible(true)
{
    m_style = std::make_shared<Style>();
}

UIElement::~UIElement()
{
}

void UIElement::setPosition(const glm::vec2& position)
{
    m_position = position;
}

void UIElement::setSize(const glm::vec2& size)
{
    m_size = size;
}

void UIElement::setStyle(std::shared_ptr<Style> style)
{
    m_style = style;
}

bool UIElement::handleMouseClick(const glm::vec2& position)
{
    return containsPoint(position);
}

bool UIElement::handleMouseMove(const glm::vec2& position)
{
    return containsPoint(position);
}

bool UIElement::handleKeyPress(int key, int action)
{
    return false;
}

bool UIElement::containsPoint(const glm::vec2& point) const
{
    return point.x >= m_position.x &&
           point.x <= m_position.x + m_size.x &&
           point.y >= m_position.y &&
           point.y <= m_position.y + m_size.y;
}

