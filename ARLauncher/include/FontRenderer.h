#ifndef FONTRENDERER_H
#define FONTRENDERER_H

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

#ifdef USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#ifdef USE_OPENGL
#include <GL/gl.h>
#endif

/**
 * @brief Класс для рендеринга текста с использованием FreeType
 */
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();
    
    /**
     * @brief Инициализация рендерера
     * @return true если успешно
     */
    bool initialize();
    
    /**
     * @brief Загрузка шрифта из файла
     * @param fontPath путь к файлу шрифта (.ttf, .otf)
     * @param fontSize размер шрифта в пикселях
     * @return true если успешно
     */
    bool loadFont(const std::string& fontPath, unsigned int fontSize = 48);
    
    /**
     * @brief Рендеринг текста
     * @param text текст для отображения
     * @param x позиция X
     * @param y позиция Y
     * @param color цвет текста (RGBA)
     * @param scale масштаб (по умолчанию 1.0)
     */
    void renderText(const std::string& text, float x, float y, 
                    const glm::vec4& color = glm::vec4(1.0f), float scale = 1.0f);
    
    /**
     * @brief Получить размер текста
     * @param text текст
     * @return размер (ширина, высота)
     */
    glm::vec2 getTextSize(const std::string& text);
    
    /**
     * @brief Очистка ресурсов
     */
    void cleanup();

private:
#ifdef USE_FREETYPE
    FT_Library m_ftLibrary;
    FT_Face m_ftFace;
    bool m_fontLoaded;
    unsigned int m_fontSize;
    
    // Кэш глифов (текстуры для каждого символа)
    struct Glyph {
        GLuint textureId;
        unsigned int width;
        unsigned int height;
        int bearingX;
        int bearingY;
        long advance;
    };
    std::unordered_map<char32_t, Glyph> m_glyphCache;
    
    /**
     * @brief Загрузка глифа в текстуру
     * @param codepoint код символа (UTF-32)
     */
    void loadGlyph(char32_t codepoint);
    
    /**
     * @brief Конвертация UTF-8 в UTF-32
     */
    std::u32string utf8ToUtf32(const std::string& utf8);
#endif
};

#endif // FONTRENDERER_H

