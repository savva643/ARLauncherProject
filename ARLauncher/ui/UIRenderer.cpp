#include "UIRenderer.h"
#include "UIElement.h"
#include "Renderer.h"
#include <algorithm>

UIRenderer::UIRenderer(Renderer* renderer)
    : m_renderer(renderer)
    , m_uiShaderProgram(0)
    , m_uiTextureAtlas(0)
{
}

UIRenderer::~UIRenderer()
{
    shutdown();
}

bool UIRenderer::initialize()
{
    setupUIResources();
    return true;
}

void UIRenderer::shutdown()
{
    m_elements.clear();
}

void UIRenderer::beginFrame()
{
    // Подготовка к рендерингу UI
}

void UIRenderer::render()
{
    for (auto& element : m_elements) {
        if (element && element->isVisible()) {
            renderElement(element.get());
        }
    }
}

void UIRenderer::endFrame()
{
    // Завершение рендеринга UI
}

void UIRenderer::addElement(std::shared_ptr<UIElement> element)
{
    m_elements.push_back(element);
}

void UIRenderer::removeElement(std::shared_ptr<UIElement> element)
{
    m_elements.erase(
        std::remove(m_elements.begin(), m_elements.end(), element),
        m_elements.end()
    );
}

void UIRenderer::handleMouseClick(float x, float y)
{
    glm::vec2 position(x, y);
    
    // Обработка в обратном порядке (верхние элементы первыми)
    for (auto it = m_elements.rbegin(); it != m_elements.rend(); ++it) {
        if ((*it) && (*it)->isVisible() && (*it)->handleMouseClick(position)) {
            break;
        }
    }
}

void UIRenderer::handleMouseMove(float x, float y)
{
    glm::vec2 position(x, y);
    
    for (auto& element : m_elements) {
        if (element && element->isVisible()) {
            element->handleMouseMove(position);
        }
    }
}

void UIRenderer::handleKeyPress(int key, int action)
{
    for (auto& element : m_elements) {
        if (element && element->isVisible()) {
            element->handleKeyPress(key, action);
        }
    }
}

void UIRenderer::setupUIResources()
{
    // TODO: Инициализация шейдеров и текстур для UI
}

void UIRenderer::renderElement(UIElement* element)
{
    // TODO: Реализовать отрисовку элемента
    element->render();
}

