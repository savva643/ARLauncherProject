#include "Text.h"
#include "Style.h"
#include "FontRenderer.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <memory>

// Глобальный FontRenderer (можно сделать синглтоном или передавать через контекст)
static FontRenderer* g_fontRenderer = nullptr;

void Text::setGlobalFontRenderer(FontRenderer* renderer)
{
    g_fontRenderer = renderer;
}

Text::Text(const std::string& text)
    : m_text(text)
    , m_fontSize(14.0f)
{
}

Text::~Text()
{
}

void Text::setFontSize(float size)
{
    m_fontSize = size;
}

void Text::render()
{
#ifdef USE_OPENGL
    if (!m_visible || m_text.empty()) {
        return;
    }
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
    // OpenGL координаты: (0,0) внизу слева, но мы хотим (0,0) вверху слева для UI
    // Поэтому используем glOrtho с инвертированными Y координатами
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // AR стиль текста
    auto style = m_style ? m_style : std::make_shared<Style>(Style::createARTextStyle());
    glm::vec4 textColor = style->textColor;
    
#ifdef USE_FREETYPE
    // Используем FreeType для рендеринга текста
    if (g_fontRenderer) {
        float scale = m_fontSize / 48.0f; // Нормализуем размер
        g_fontRenderer->renderText(m_text, m_position.x, m_position.y, 
                                   textColor * style->opacity, scale);
    } else {
        // Fallback на простые точки если FreeType не доступен
        glColor4f(textColor.r, textColor.g, textColor.b, textColor.a * style->opacity);
        glPointSize(style->fontSize * 0.5f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < m_text.length() && i < 20; i++) {
            glVertex2f(m_position.x + i * style->fontSize * 0.6f, m_position.y);
        }
        glEnd();
    }
#else
    // Fallback на простые точки если FreeType не доступен
    glColor4f(textColor.r, textColor.g, textColor.b, textColor.a * style->opacity);
    glPointSize(style->fontSize * 0.5f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < m_text.length() && i < 20; i++) {
        glVertex2f(m_position.x + i * style->fontSize * 0.6f, m_position.y);
    }
    glEnd();
#endif
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
#endif
}

