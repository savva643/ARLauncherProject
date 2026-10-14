#include "UIRenderer.h"
#include "UIElement.h"
#include "Renderer.h"
#include "Style.h"
#include "Text.h"
#include "Button.h"
#include "LiDARVisualizer.h"
#include <algorithm>
#include <string>
#include <iostream>

UIRenderer::UIRenderer(Renderer* renderer)
    : m_renderer(renderer)
    , m_uiShaderProgram(0)
    , m_uiTextureAtlas(0)
    , m_uiTextureId(0)
    , m_uiWidth(1024)
    , m_uiHeight(512)
    , m_isVulkan(false)
#ifdef USE_FREETYPE
    , m_ftLibrary(nullptr)
    , m_ftFace(nullptr)
    , m_ftInitialized(false)
#endif
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

#ifdef USE_FREETYPE
    if (m_ftFace) {
        FT_Done_Face(m_ftFace);
        m_ftFace = nullptr;
    }
    if (m_ftInitialized) {
        FT_Done_FreeType(m_ftLibrary);
        m_ftInitialized = false;
        m_ftLibrary = nullptr;
    }
#endif
}

void UIRenderer::beginFrame()
{
    // Подготовка к рендерингу UI
}

void UIRenderer::render()
{
    if (m_isVulkan) {
        if (m_uiRgba.size() != static_cast<size_t>(m_uiWidth) * static_cast<size_t>(m_uiHeight) * 4) {
            m_uiRgba.resize(static_cast<size_t>(m_uiWidth) * static_cast<size_t>(m_uiHeight) * 4);
        }
        std::fill(m_uiRgba.begin(), m_uiRgba.end(), 0);

        auto blendPixel = [&](int x, int y, uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa) {
            if (x < 0 || y < 0 || x >= static_cast<int>(m_uiWidth) || y >= static_cast<int>(m_uiHeight)) return;
            size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(m_uiWidth) + static_cast<size_t>(x)) * 4;
            uint8_t dr = m_uiRgba[idx + 0];
            uint8_t dg = m_uiRgba[idx + 1];
            uint8_t db = m_uiRgba[idx + 2];
            uint8_t da = m_uiRgba[idx + 3];

            float sA = static_cast<float>(sa) / 255.0f;
            float dA = static_cast<float>(da) / 255.0f;
            float outA = sA + dA * (1.0f - sA);
            if (outA <= 0.0f) {
                m_uiRgba[idx + 0] = 0;
                m_uiRgba[idx + 1] = 0;
                m_uiRgba[idx + 2] = 0;
                m_uiRgba[idx + 3] = 0;
                return;
            }

            auto blendChan = [&](uint8_t s, uint8_t d) -> uint8_t {
                float sf = static_cast<float>(s) / 255.0f;
                float df = static_cast<float>(d) / 255.0f;
                float out = (sf * sA + df * dA * (1.0f - sA)) / outA;
                int v = static_cast<int>(out * 255.0f + 0.5f);
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                return static_cast<uint8_t>(v);
            };

            int outAi = static_cast<int>(outA * 255.0f + 0.5f);
            if (outAi < 0) outAi = 0;
            if (outAi > 255) outAi = 255;
            m_uiRgba[idx + 0] = blendChan(sr, dr);
            m_uiRgba[idx + 1] = blendChan(sg, dg);
            m_uiRgba[idx + 2] = blendChan(sb, db);
            m_uiRgba[idx + 3] = static_cast<uint8_t>(outAi);
        };

        auto fillRect = [&](int x, int y, int w, int h, const glm::vec4& c) {
            int x0 = std::max(0, x);
            int y0 = std::max(0, y);
            int x1 = std::min(static_cast<int>(m_uiWidth), x + w);
            int y1 = std::min(static_cast<int>(m_uiHeight), y + h);

            uint8_t r = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
            uint8_t g = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
            uint8_t b = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
            uint8_t a = static_cast<uint8_t>(std::clamp(c.a, 0.0f, 1.0f) * 255.0f);

            for (int yy = y0; yy < y1; yy++) {
                for (int xx = x0; xx < x1; xx++) {
                    blendPixel(xx, yy, r, g, b, a);
                }
            }
        };

        auto drawRectOutline = [&](int x, int y, int w, int h, int thickness, const glm::vec4& c) {
            fillRect(x, y, w, thickness, c);
            fillRect(x, y + h - thickness, w, thickness, c);
            fillRect(x, y, thickness, h, c);
            fillRect(x + w - thickness, y, thickness, h, c);
        };

        auto utf8ToUtf32 = [](const std::string& utf8) {
            std::u32string out;
            for (size_t i = 0; i < utf8.size(); ) {
                unsigned char c = static_cast<unsigned char>(utf8[i]);
                char32_t cp = 0;
                if (c < 0x80) {
                    cp = c;
                    i += 1;
                } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) {
                    cp = (c & 0x1F) << 6;
                    cp |= (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
                    i += 2;
                } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8.size()) {
                    cp = (c & 0x0F) << 12;
                    cp |= ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6);
                    cp |= (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
                    i += 3;
                } else if ((c & 0xF8) == 0xF0 && i + 3 < utf8.size()) {
                    cp = (c & 0x07) << 18;
                    cp |= ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12);
                    cp |= ((static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6);
                    cp |= (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
                    i += 4;
                } else {
                    i += 1;
                }
                if (cp) out.push_back(cp);
            }
            return out;
        };

        auto drawText = [&](const std::string& text, int x, int y, float fontSize, const glm::vec4& color) {
#ifdef USE_FREETYPE
            if (!m_ftInitialized || !m_ftFace) return;
            FT_Set_Pixel_Sizes(m_ftFace, 0, static_cast<FT_UInt>(fontSize));

            uint8_t r = static_cast<uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            uint8_t g = static_cast<uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            uint8_t b = static_cast<uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            uint8_t a = static_cast<uint8_t>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);

            int penX = x;
            int penY = y;
            std::u32string s32 = utf8ToUtf32(text);
            for (char32_t cp : s32) {
                if (FT_Load_Char(m_ftFace, static_cast<FT_ULong>(cp), FT_LOAD_RENDER)) {
                    continue;
                }
                FT_GlyphSlot gslot = m_ftFace->glyph;
                FT_Bitmap& bm = gslot->bitmap;
                int x0 = penX + gslot->bitmap_left;
                int y0 = penY - gslot->bitmap_top;

                for (int row = 0; row < static_cast<int>(bm.rows); row++) {
                    for (int col = 0; col < static_cast<int>(bm.width); col++) {
                        uint8_t cov = bm.buffer[row * bm.pitch + col];
                        uint8_t sa = static_cast<uint8_t>((static_cast<uint32_t>(cov) * static_cast<uint32_t>(a)) / 255u);
                        blendPixel(x0 + col, y0 + row, r, g, b, sa);
                    }
                }
                penX += (gslot->advance.x >> 6);
            }
#else
            (void)text; (void)x; (void)y; (void)fontSize; (void)color;
#endif
        };

        // Background panel
        fillRect(0, 0, static_cast<int>(m_uiWidth), static_cast<int>(m_uiHeight), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        for (auto& element : m_elements) {
            if (!element || !element->isVisible()) continue;

            auto style = element->getStyle();
            if (!style) style = std::make_shared<Style>();

            glm::vec2 pos = element->getPosition();
            glm::vec2 size = element->getSize();
            int x = static_cast<int>(pos.x);
            int y = static_cast<int>(pos.y);
            int w = static_cast<int>(size.x);
            int h = static_cast<int>(size.y);

            if (auto* btn = dynamic_cast<Button*>(element.get())) {
                glm::vec4 bg = style->backgroundColor;
                bg.a *= style->opacity;
                fillRect(x, y, w, h, bg);
                glm::vec4 border = style->borderColor;
                border.a *= style->opacity;
                int t = std::max(1, static_cast<int>(style->borderWidth));
                drawRectOutline(x, y, w, h, t, border);

                glm::vec4 tc = style->textColor;
                tc.a *= style->opacity;
                float fs = (style->fontSize > 0.0f) ? style->fontSize : 18.0f;
                drawText(btn->getText(), x + 12, y + (h / 2) + static_cast<int>(fs * 0.35f), fs, tc);
                continue;
            }

            if (auto* txt = dynamic_cast<Text*>(element.get())) {
                glm::vec4 tc = style->textColor;
                tc.a *= style->opacity;
                float fs = txt->getFontSize();
                if (style->fontSize > 0.0f) fs = style->fontSize;
                if (fs < 10.0f) fs = 18.0f;
                drawText(txt->getText(), x, y + static_cast<int>(fs), fs, tc);
                continue;
            }

            // Unknown element: draw bounding box
            glm::vec4 border = style->borderColor;
            border.a *= style->opacity;
            drawRectOutline(x, y, w, h, 1, border);
        }

        
        // 🔹 LiDAR visualization disabled for performance
        // Render LiDAR visualization
        // if (m_lidarVisualizer && !m_uiRgba.empty() && m_uiWidth > 0 && m_uiHeight > 0) {
        //     // Ensure buffer is large enough
        //     size_t expectedSize = static_cast<size_t>(m_uiWidth) * static_cast<size_t>(m_uiHeight) * 4;
        //     if (m_uiRgba.size() >= expectedSize) {
        //         m_lidarVisualizer->renderToBuffer(m_uiRgba.data(), m_uiWidth, m_uiHeight,
        //                                            m_uiWidth - 320, 20, 300, 300);
        //     }
        // }
#ifdef USE_VULKAN
        VulkanRenderer* vk = dynamic_cast<VulkanRenderer*>(m_renderer);
        if (vk && m_uiTextureId != 0) {
            vk->updateTexture(m_uiTextureId, m_uiRgba.data(), m_uiWidth, m_uiHeight);
        }
#endif
        return;
    }

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
    m_uiRgba.resize(static_cast<size_t>(m_uiWidth) * static_cast<size_t>(m_uiHeight) * 4);
    std::fill(m_uiRgba.begin(), m_uiRgba.end(), 0);

    m_isVulkan = false;
#ifdef USE_VULKAN
    if (dynamic_cast<VulkanRenderer*>(m_renderer) != nullptr) {
        m_isVulkan = true;
    }
#endif

#ifdef USE_FREETYPE
    m_ftInitialized = false;
    if (FT_Init_FreeType(&m_ftLibrary) == 0) {
        m_ftInitialized = true;
        const char* fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        if (FT_New_Face(m_ftLibrary, fontPath, 0, &m_ftFace) != 0) {
            m_ftFace = nullptr;
        }
    }
#endif

#ifdef USE_VULKAN
    if (m_isVulkan) {
        VulkanRenderer* vk = dynamic_cast<VulkanRenderer*>(m_renderer);
        if (vk) {
            m_uiTextureId = vk->createTexture(m_uiRgba.data(), m_uiWidth, m_uiHeight);
            vk->setUIOverlayTexture(m_uiTextureId, m_uiWidth, m_uiHeight);
            vk->setUI3DTexture(m_uiTextureId, m_uiWidth, m_uiHeight,
                               glm::vec3(0.0f, 0.0f, -1.2f), glm::vec2(0.9f, 0.5f), true);
        }
    }
#endif
}

void UIRenderer::renderElement(UIElement* element)
{
    // TODO: Реализовать отрисовку элемента
    element->render();
}

