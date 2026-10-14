#include "Text.h"
#include "Style.h"
#include "FontRenderer.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
#endif
#include <memory>
#include <iostream>

// Тип для glUseProgram
#ifdef USE_OPENGL
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
extern PFNGLUSEPROGRAMPROC glUseProgram;
#endif

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
    
    // КРИТИЧЕСКИ ВАЖНО: Отключаем активный shader program перед fixed-function рендерингом
    // Fixed-function OpenGL (glBegin/glEnd) не работает когда активен shader
    extern PFNGLUSEPROGRAMPROC glUseProgram;
    if (glUseProgram) {
        glUseProgram(0);
    }
    
    // Сохраняем текущий viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Сохраняем текущие состояния OpenGL
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean texture2DWasEnabled = glIsEnabled(GL_TEXTURE_2D);
    
    // Настраиваем OpenGL для 2D рендеринга текста
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    
    // Сохраняем текущие матрицы
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    int width, height;
    GLFWwindow* window = glfwGetCurrentContext();
    if (window) {
        glfwGetWindowSize(window, &width, &height);
    } else {
        // Если окно недоступно, используем сохраненный viewport
        width = viewport[2];
        height = viewport[3];
    }
    
    // OpenGL координаты: (0,0) внизу слева, но мы хотим (0,0) вверху слева для UI
    // Поэтому используем glOrtho с инвертированными Y координатами
    glOrtho(0, width, height, 0, -1, 1);
    
    // Устанавливаем viewport на весь экран
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // AR стиль текста
    auto style = m_style ? m_style : std::make_shared<Style>(Style::createARTextStyle());
    glm::vec4 textColor = style->textColor;
    
    // Используем fontSize из стиля если он задан, иначе используем m_fontSize
    float actualFontSize = (style && style->fontSize > 0.0f) ? style->fontSize : m_fontSize;
    if (actualFontSize < 12.0f) actualFontSize = 20.0f; // Минимум 20 пикселей для видимости
    
#ifdef USE_FREETYPE
    // Используем FreeType для рендеринга текста
    if (g_fontRenderer && g_fontRenderer->isFontLoaded()) {
        float scale = actualFontSize / 48.0f; // Нормализуем размер (базовый размер шрифта 48)
        // Отладочный вывод (только первый раз)
        static bool debugPrinted = false;
        if (!debugPrinted) {
            std::cout << "[DEBUG] Text::render() - рендерим текст: \"" << m_text << "\" в позиции (" 
                      << m_position.x << ", " << m_position.y << ") fontSize=" << actualFontSize 
                      << " scale=" << scale << std::endl;
            debugPrinted = true;
        }
        g_fontRenderer->renderText(m_text, m_position.x, m_position.y, 
                                   textColor * style->opacity, scale);
    } else {
        // Fallback на простые точки если FreeType не доступен
        if (g_fontRenderer) {
            std::cout << "[WARN] FontRenderer существует, но шрифт не загружен!" << std::endl;
        } else {
            std::cout << "[WARN] FontRenderer не установлен!" << std::endl;
        }
        glColor4f(textColor.r, textColor.g, textColor.b, textColor.a * style->opacity);
        glPointSize(actualFontSize * 0.5f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < m_text.length() && i < 20; i++) {
            glVertex2f(m_position.x + i * actualFontSize * 0.6f, m_position.y);
        }
        glEnd();
    }
#else
    // Fallback на простые точки если FreeType не доступен
    glColor4f(textColor.r, textColor.g, textColor.b, textColor.a * style->opacity);
    glPointSize(actualFontSize * 0.5f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < m_text.length() && i < 20; i++) {
        glVertex2f(m_position.x + i * actualFontSize * 0.6f, m_position.y);
    }
    glEnd();
#endif
    
    // Восстанавливаем матрицы
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // Восстанавливаем viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    // Восстанавливаем OpenGL состояние
    if (depthTestWasEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (!blendWasEnabled) {
        glDisable(GL_BLEND);
    }
    
    if (!texture2DWasEnabled) {
        glDisable(GL_TEXTURE_2D);
    }
#endif
}

