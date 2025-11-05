#include "Text.h"
#include "Style.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <memory>

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
    
    // Простой OpenGL рендеринг текста (заглушка)
    // В реальности нужна библиотека для рендеринга текста (FreeType, stb_truetype)
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
    
    // AR стиль текста
    auto style = m_style ? m_style : std::make_shared<Style>(Style::createARTextStyle());
    glm::vec4 textColor = style->textColor;
    glColor4f(textColor.r, textColor.g, textColor.b, textColor.a * style->opacity);
    
    // Простой индикатор текста (квадрат)
    // TODO: Реализовать полноценный рендеринг текста через FreeType
    glPointSize(style->fontSize * 0.5f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < m_text.length() && i < 20; i++) {
        glVertex2f(m_position.x + i * style->fontSize * 0.6f, m_position.y);
    }
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    
    // TODO: Реализовать полноценный рендеринг текста через FreeType
#endif
}

