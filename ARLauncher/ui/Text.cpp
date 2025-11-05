#include "Text.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif

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
    
    // Простой индикатор текста (квадрат)
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    glVertex2f(m_position.x, m_position.y);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    
    // TODO: Реализовать полноценный рендеринг текста через FreeType
#endif
}

