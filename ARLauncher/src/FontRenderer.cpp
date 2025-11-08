#include "FontRenderer.h"
#include <iostream>
#include <fstream>
#include <vector>

#ifdef USE_FREETYPE

FontRenderer::FontRenderer()
    : m_ftLibrary(nullptr)
    , m_ftFace(nullptr)
    , m_fontLoaded(false)
    , m_fontSize(48)
{
}

FontRenderer::~FontRenderer()
{
    cleanup();
}

bool FontRenderer::initialize()
{
#ifdef USE_FREETYPE
    FT_Error error = FT_Init_FreeType(&m_ftLibrary);
    if (error) {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool FontRenderer::loadFont(const std::string& fontPath, unsigned int fontSize)
{
#ifdef USE_FREETYPE
    if (!m_ftLibrary) {
        if (!initialize()) {
            return false;
        }
    }
    
    // Попробуем найти системный шрифт, если путь не указан
    std::string actualPath = fontPath;
    if (actualPath.empty()) {
        // Попробуем стандартные пути для шрифтов
        const char* possiblePaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "C:/Windows/Fonts/arial.ttf",
            nullptr
        };
        
        for (int i = 0; possiblePaths[i]; ++i) {
            std::ifstream test(possiblePaths[i]);
            if (test.good()) {
                actualPath = possiblePaths[i];
                test.close();
                break;
            }
        }
    }
    
    if (actualPath.empty()) {
        std::cerr << "No font file found. Please specify a font path." << std::endl;
        return false;
    }
    
    FT_Error error = FT_New_Face(m_ftLibrary, actualPath.c_str(), 0, &m_ftFace);
    if (error == FT_Err_Unknown_File_Format) {
        std::cerr << "Font file format not supported: " << actualPath << std::endl;
        return false;
    } else if (error) {
        std::cerr << "Failed to load font: " << actualPath << std::endl;
        return false;
    }
    
    m_fontSize = fontSize;
    error = FT_Set_Pixel_Sizes(m_ftFace, 0, fontSize);
    if (error) {
        std::cerr << "Failed to set font size" << std::endl;
        FT_Done_Face(m_ftFace);
        m_ftFace = nullptr;
        return false;
    }
    
    m_fontLoaded = true;
    std::cout << "Font loaded successfully: " << actualPath << " (size: " << fontSize << ")" << std::endl;
    return true;
#else
    (void)fontPath;
    (void)fontSize;
    return false;
#endif
}

void FontRenderer::loadGlyph(char32_t codepoint)
{
#ifdef USE_FREETYPE
    if (!m_ftFace || m_glyphCache.find(codepoint) != m_glyphCache.end()) {
        return; // Уже загружен или шрифт не загружен
    }
    
    FT_Error error = FT_Load_Char(m_ftFace, codepoint, FT_LOAD_RENDER);
    if (error) {
        std::cerr << "Failed to load glyph for codepoint: " << codepoint << std::endl;
        return;
    }
    
    FT_GlyphSlot glyph = m_ftFace->glyph;
    FT_Bitmap bitmap = glyph->bitmap;
    
    // Создаем текстуру для глифа
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    // Конвертируем в RGBA если нужно
    std::vector<unsigned char> rgbaData;
    if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        rgbaData.resize(bitmap.width * bitmap.rows * 4);
        for (unsigned int y = 0; y < bitmap.rows; ++y) {
            for (unsigned int x = 0; x < bitmap.width; ++x) {
                unsigned char gray = bitmap.buffer[y * bitmap.width + x];
                rgbaData[(y * bitmap.width + x) * 4 + 0] = 255; // R
                rgbaData[(y * bitmap.width + x) * 4 + 1] = 255; // G
                rgbaData[(y * bitmap.width + x) * 4 + 2] = 255; // B
                rgbaData[(y * bitmap.width + x) * 4 + 3] = gray; // A
            }
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width, bitmap.rows, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, rgbaData.empty() ? bitmap.buffer : rgbaData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    Glyph glyphData;
    glyphData.textureId = textureId;
    glyphData.width = bitmap.width;
    glyphData.height = bitmap.rows;
    glyphData.bearingX = glyph->bitmap_left;
    glyphData.bearingY = glyph->bitmap_top;
    glyphData.advance = glyph->advance.x;
    
    m_glyphCache[codepoint] = glyphData;
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

std::u32string FontRenderer::utf8ToUtf32(const std::string& utf8)
{
    std::u32string result;
    for (size_t i = 0; i < utf8.length(); ) {
        unsigned char byte = static_cast<unsigned char>(utf8[i]);
        char32_t codepoint = 0;
        
        if (byte < 0x80) {
            // ASCII
            codepoint = byte;
            i += 1;
        } else if ((byte & 0xE0) == 0xC0) {
            // 2-byte sequence
            codepoint = (byte & 0x1F) << 6;
            codepoint |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            // 3-byte sequence
            codepoint = (byte & 0x0F) << 12;
            codepoint |= ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6);
            codepoint |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            // 4-byte sequence
            codepoint = (byte & 0x07) << 18;
            codepoint |= ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12);
            codepoint |= ((static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6);
            codepoint |= (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
            i += 4;
        } else {
            i++; // Пропускаем невалидный байт
        }
        
        if (codepoint > 0) {
            result += codepoint;
        }
    }
    return result;
}

void FontRenderer::renderText(const std::string& text, float x, float y, 
                              const glm::vec4& color, float scale)
{
#ifdef USE_FREETYPE
    if (!m_fontLoaded || !m_ftFace) {
        return;
    }
    
    std::u32string utf32 = utf8ToUtf32(text);
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(color.r, color.g, color.b, color.a);
    
    float startX = x;
    
    for (char32_t codepoint : utf32) {
        // Загружаем глиф если нужно
        if (m_glyphCache.find(codepoint) == m_glyphCache.end()) {
            loadGlyph(codepoint);
        }
        
        auto it = m_glyphCache.find(codepoint);
        if (it == m_glyphCache.end()) {
            continue; // Пропускаем если не удалось загрузить
        }
        
        const Glyph& glyph = it->second;
        
        float xpos = startX + glyph.bearingX * scale;
        float ypos = y - (glyph.height - glyph.bearingY) * scale;
        
        float w = glyph.width * scale;
        float h = glyph.height * scale;
        
        // Рендерим глиф
        glBindTexture(GL_TEXTURE_2D, glyph.textureId);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(xpos, ypos + h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(xpos + w, ypos + h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(xpos + w, ypos);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(xpos, ypos);
        glEnd();
        
        // Перемещаем курсор
        startX += (glyph.advance >> 6) * scale; // advance в 1/64 пикселя
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
#else
    (void)text;
    (void)x;
    (void)y;
    (void)color;
    (void)scale;
#endif
}

glm::vec2 FontRenderer::getTextSize(const std::string& text)
{
#ifdef USE_FREETYPE
    if (!m_fontLoaded || !m_ftFace) {
        return glm::vec2(0.0f);
    }
    
    std::u32string utf32 = utf8ToUtf32(text);
    float width = 0.0f;
    float height = 0.0f;
    
    for (char32_t codepoint : utf32) {
        if (m_glyphCache.find(codepoint) == m_glyphCache.end()) {
            loadGlyph(codepoint);
        }
        
        auto it = m_glyphCache.find(codepoint);
        if (it != m_glyphCache.end()) {
            const Glyph& glyph = it->second;
            width += (glyph.advance >> 6);
            height = std::max(height, static_cast<float>(glyph.height));
        }
    }
    
    return glm::vec2(width, height);
#else
    (void)text;
    return glm::vec2(0.0f);
#endif
}

void FontRenderer::cleanup()
{
#ifdef USE_FREETYPE
    // Удаляем текстуры глифов
    for (auto& pair : m_glyphCache) {
        glDeleteTextures(1, &pair.second.textureId);
    }
    m_glyphCache.clear();
    
    if (m_ftFace) {
        FT_Done_Face(m_ftFace);
        m_ftFace = nullptr;
    }
    
    if (m_ftLibrary) {
        FT_Done_FreeType(m_ftLibrary);
        m_ftLibrary = nullptr;
    }
    
    m_fontLoaded = false;
#endif
}

#else
// Заглушка если FreeType не доступен
FontRenderer::FontRenderer() {}
FontRenderer::~FontRenderer() {}
bool FontRenderer::initialize() { return false; }
bool FontRenderer::loadFont(const std::string&, unsigned int) { return false; }
void FontRenderer::renderText(const std::string&, float, float, const glm::vec4&, float) {}
glm::vec2 FontRenderer::getTextSize(const std::string&) { return glm::vec2(0.0f); }
void FontRenderer::cleanup() {}
#endif

