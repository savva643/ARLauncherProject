#ifndef WINDOW_H
#define WINDOW_H

#include "UIElement.h"
#include <vector>
#include <memory>
#include <string>

/**
 * @brief 3D окно приложения в AR сцене
 */
class Window : public UIElement {
public:
    Window(const std::string& title);
    ~Window() override;

    void setTitle(const std::string& title) { m_title = title; }
    std::string getTitle() const { return m_title; }
    
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(std::shared_ptr<UIElement> child);
    
    void setMinimized(bool minimized) { m_minimized = minimized; }
    bool isMinimized() const { return m_minimized; }
    
    void setMaximized(bool maximized) { m_maximized = maximized; }
    bool isMaximized() const { return m_maximized; }
    
    void render() override;
    bool handleMouseClick(const glm::vec2& position) override;
    bool handleMouseMove(const glm::vec2& position) override;

private:
    std::string m_title;
    std::vector<std::shared_ptr<UIElement>> m_children;
    bool m_minimized;
    bool m_maximized;
    bool m_dragging;
    glm::vec2 m_dragOffset;
    
    void renderTitleBar();
    void renderContent();
};

#endif // WINDOW_H

