#ifndef SIMPLE_NVG_H
#define SIMPLE_NVG_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

struct NVGcolor {
    float r;
    float g;
    float b;
    float a;
};

inline NVGcolor nvgRGBAf(float r, float g, float b, float a) {
    NVGcolor col{r, g, b, a};
    return col;
}

inline NVGcolor nvgRGBf(float r, float g, float b) {
    return nvgRGBAf(r, g, b, 1.0f);
}

inline NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return nvgRGBAf(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

enum {
    NVG_ALIGN_LEFT   = 1 << 0,
    NVG_ALIGN_CENTER = 1 << 1,
    NVG_ALIGN_RIGHT  = 1 << 2,
    NVG_ALIGN_TOP    = 1 << 3,
    NVG_ALIGN_MIDDLE = 1 << 4,
    NVG_ALIGN_BOTTOM = 1 << 5
};

struct _NVGPath {
    float x{0.0f};
    float y{0.0f};
    float w{0.0f};
    float h{0.0f};
    float radius{0.0f};
    bool valid{false};
};

struct NVGcontext {
    NVGcolor fillColor{nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f)};
    float fontSize{18.0f};
    int textAlign{NVG_ALIGN_LEFT | NVG_ALIGN_TOP};
    _NVGPath path{};
    int frameWidth{0};
    int frameHeight{0};
    float pixelRatio{1.0f};
};

inline NVGcontext* nvgCreateSimple() {
    return new NVGcontext();
}

inline void nvgDeleteSimple(NVGcontext* ctx) {
    delete ctx;
}

inline void nvgBeginFrame(NVGcontext* ctx, int width, int height, float devicePixelRatio) {
    if (!ctx) return;
    ctx->frameWidth = width;
    ctx->frameHeight = height;
    ctx->pixelRatio = devicePixelRatio;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

inline void nvgEndFrame(NVGcontext* /*ctx*/) {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

inline void nvgBeginPath(NVGcontext* ctx) {
    if (!ctx) return;
    ctx->path = _NVGPath{};
}

inline void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r) {
    if (!ctx) return;
    ctx->path.valid = true;
    ctx->path.x = x;
    ctx->path.y = y;
    ctx->path.w = w;
    ctx->path.h = h;
    ctx->path.radius = r;
}

inline void nvgFillColor(NVGcontext* ctx, NVGcolor color) {
    if (!ctx) return;
    ctx->fillColor = color;
}

inline void nvgFontSize(NVGcontext* ctx, float size) {
    if (!ctx) return;
    ctx->fontSize = size;
}

inline void nvgFontFace(NVGcontext* /*ctx*/, const char* /*name*/) {
    // Not used in the minimal implementation
}

inline void nvgTextAlign(NVGcontext* ctx, int align) {
    if (!ctx) return;
    ctx->textAlign = align;
}

inline void _drawFilledRect(float x, float y, float w, float h, const NVGcolor& color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

inline void nvgFill(NVGcontext* ctx) {
    if (!ctx || !ctx->path.valid) return;
    _drawFilledRect(ctx->path.x, ctx->path.y, ctx->path.w, ctx->path.h, ctx->fillColor);
    ctx->path.valid = false;
}

struct SimpleGlyph {
    char character;
    const char* rows[7];
};

inline const SimpleGlyph* _findGlyph(char c);

inline void nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
    if (!ctx || !string) return;
    std::string text;
    if (end) {
        text.assign(string, end);
    } else {
        text.assign(string);
    }

    // Uppercase conversion for simplicity
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    const float cellWidth = ctx->fontSize * 0.6f;
    const float cellHeight = ctx->fontSize;
    const float spacing = ctx->fontSize * 0.15f;
    float totalWidth = 0.0f;
    for (char ch : text) {
        if (ch == ' ') {
            totalWidth += cellWidth + spacing;
        } else {
            totalWidth += cellWidth + spacing;
        }
    }

    float startX = x;
    float startY = y;

    if (ctx->textAlign & NVG_ALIGN_CENTER) {
        startX -= totalWidth * 0.5f;
    } else if (ctx->textAlign & NVG_ALIGN_RIGHT) {
        startX -= totalWidth;
    }

    if (ctx->textAlign & NVG_ALIGN_MIDDLE) {
        startY -= cellHeight * 0.5f;
    } else if (ctx->textAlign & NVG_ALIGN_BOTTOM) {
        startY -= cellHeight;
    }

    glColor4f(ctx->fillColor.r, ctx->fillColor.g, ctx->fillColor.b, ctx->fillColor.a);

    float penX = startX;
    for (char ch : text) {
        if (ch == ' ') {
            penX += cellWidth + spacing;
            continue;
        }
        const SimpleGlyph* glyph = _findGlyph(ch);
        if (!glyph) {
            penX += cellWidth + spacing;
            continue;
        }

        const float blockSizeX = cellWidth / 5.0f;
        const float blockSizeY = cellHeight / 7.0f;

        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph->rows[row][col] == '#') {
                    float rx = penX + col * blockSizeX;
                    float ry = startY + row * blockSizeY;
                    _drawFilledRect(rx, ry, blockSizeX * 0.9f, blockSizeY * 0.9f, ctx->fillColor);
                }
            }
        }
        penX += cellWidth + spacing;
    }
}

inline const SimpleGlyph* _findGlyph(char c) {
    static const SimpleGlyph glyphs[] = {
        {'A', {"  #  "," # # ","#   #","#####","#   #","#   #","#   #"}},
        {'B', {"#### ","#   #","#   #","#### ","#   #","#   #","#### "}},
        {'C', {" ### ","#   #","#    ","#    ","#    ","#   #"," ### "}},
        {'D', {"#### ","#   #","#   #","#   #","#   #","#   #","#### "}},
        {'E', {"#####","#    ","#    ","#####","#    ","#    ","#####"}},
        {'G', {" ### ","#   #","#    ","# ###","#   #","#   #"," ### "}},
        {'H', {"#   #","#   #","#   #","#####","#   #","#   #","#   #"}},
        {'L', {"#    ","#    ","#    ","#    ","#    ","#    ","#####"}},
        {'M', {"#   #","## ##","# # #","#   #","#   #","#   #","#   #"}},
        {'N', {"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"}},
        {'O', {" ### ","#   #","#   #","#   #","#   #","#   #"," ### "}},
        {'P', {"#### ","#   #","#   #","#### ","#    ","#    ","#    "}},
        {'R', {"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"}},
        {'S', {" ####","#    ","#    "," ### ","    #","    #","#### "}},
        {'T', {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "}},
        {'U', {"#   #","#   #","#   #","#   #","#   #","#   #"," ### "}},
        {'V', {"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "}},
        {'W', {"#   #","#   #","#   #","# # #","# # #","## ##","#   #"}},
        {'Y', {"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "}},
        {'Z', {"#####","    #","   # ","  #  "," #   ","#    ","#####"}},
        {'0', {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}},
        {'1', {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  "," ### "}},
        {'2', {" ### ","#   #","    #","   # ","  #  "," #   ","#####"}},
        {'3', {" ### ","#   #","    #"," ### ","    #","#   #"," ### "}},
        {'4', {"   # ","  ## "," # # ","#  # ","#####","   # ","   # "}},
        {'5', {"#####","#    ","#    ","#### ","    #","#   #"," ### "}},
        {'6', {" ### ","#   #","#    ","#### ","#   #","#   #"," ### "}},
        {'7', {"#####","    #","   # ","  #  ","  #  ","  #  ","  #  "}},
        {'8', {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}},
        {'9', {" ### ","#   #","#   #"," ####","    #","#   #"," ### "}},
        {' ', {"     ","     ","     ","     ","     ","     ","     "}},
        {'-', {"     ","     ","     ","#####","     ","     ","     "}},
        {':', {"     ","  #  ","     ","     ","     ","  #  ","     "}}
    };

    for (const auto& glyph : glyphs) {
        if (glyph.character == c) {
            return &glyph;
        }
    }
    return nullptr;
}

#endif // SIMPLE_NVG_H

