#include "Renderer.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

// Объявления типов функций OpenGL 3.3+
#ifndef APIENTRY
#define APIENTRY
#endif

typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void (APIENTRY *PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const void*);
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

// Глобальные указатели на функции OpenGL 3.3+
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM3FPROC glUniform3f = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
PFNGLDRAWELEMENTSPROC glDrawElements = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

// Загрузка указателей на функции OpenGL через GLFW
static bool LoadOpenGLFunctions() {
    static bool loaded = false;
    if (loaded) return true;
    
    #define LOAD_GL_FUNC(name) \
        name = (decltype(name))glfwGetProcAddress(#name); \
        if (!name) { \
            std::cerr << "[WARN] Failed to load OpenGL function: " #name << std::endl; \
        }
    
    LOAD_GL_FUNC(glGenVertexArrays);
    LOAD_GL_FUNC(glBindVertexArray);
    LOAD_GL_FUNC(glDeleteVertexArrays);
    LOAD_GL_FUNC(glGenBuffers);
    LOAD_GL_FUNC(glBindBuffer);
    LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glDeleteBuffers);
    LOAD_GL_FUNC(glEnableVertexAttribArray);
    LOAD_GL_FUNC(glVertexAttribPointer);
    LOAD_GL_FUNC(glCreateShader);
    LOAD_GL_FUNC(glShaderSource);
    LOAD_GL_FUNC(glCompileShader);
    LOAD_GL_FUNC(glGetShaderiv);
    LOAD_GL_FUNC(glGetShaderInfoLog);
    LOAD_GL_FUNC(glDeleteShader);
    LOAD_GL_FUNC(glCreateProgram);
    LOAD_GL_FUNC(glAttachShader);
    LOAD_GL_FUNC(glLinkProgram);
    LOAD_GL_FUNC(glGetProgramiv);
    LOAD_GL_FUNC(glGetProgramInfoLog);
    LOAD_GL_FUNC(glDeleteProgram);
    LOAD_GL_FUNC(glUseProgram);
    LOAD_GL_FUNC(glGetUniformLocation);
    LOAD_GL_FUNC(glUniform1f);
    LOAD_GL_FUNC(glUniform1i);
    LOAD_GL_FUNC(glUniform3f);
    LOAD_GL_FUNC(glUniformMatrix4fv);
    LOAD_GL_FUNC(glGenFramebuffers);
    LOAD_GL_FUNC(glBindFramebuffer);
    LOAD_GL_FUNC(glFramebufferTexture2D);
    LOAD_GL_FUNC(glCheckFramebufferStatus);
    LOAD_GL_FUNC(glGenRenderbuffers);
    LOAD_GL_FUNC(glBindRenderbuffer);
    LOAD_GL_FUNC(glRenderbufferStorage);
    LOAD_GL_FUNC(glFramebufferRenderbuffer);
    LOAD_GL_FUNC(glDeleteFramebuffers);
    LOAD_GL_FUNC(glDeleteRenderbuffers);
    LOAD_GL_FUNC(glDrawArrays);
    LOAD_GL_FUNC(glDrawElements);
    LOAD_GL_FUNC(glActiveTexture);
    
    #undef LOAD_GL_FUNC
    
    loaded = true;
    return true;
}
#endif

// Встроенная реализация simple_nvg для избежания проблем с путями
namespace {
struct NVGcolor {
    float r, g, b, a;
};

inline NVGcolor nvgRGBAf(float r, float g, float b, float a) {
    return {r, g, b, a};
}

inline NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

enum {
    NVG_ALIGN_LEFT = 1 << 0,
    NVG_ALIGN_CENTER = 1 << 1,
    NVG_ALIGN_RIGHT = 1 << 2,
    NVG_ALIGN_TOP = 1 << 3,
    NVG_ALIGN_MIDDLE = 1 << 4,
    NVG_ALIGN_BOTTOM = 1 << 5
};

struct _NVGPath {
    float x{0.0f}, y{0.0f}, w{0.0f}, h{0.0f}, radius{0.0f};
    bool valid{false};
};

struct NVGcontext {
    NVGcolor fillColor{nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f)};
    float fontSize{18.0f};
    int textAlign{NVG_ALIGN_LEFT | NVG_ALIGN_TOP};
    _NVGPath path{};
    int frameWidth{0}, frameHeight{0};
    float pixelRatio{1.0f};
};

inline NVGcontext* nvgCreateSimple() { return new NVGcontext(); }
inline void nvgDeleteSimple(NVGcontext* ctx) { delete ctx; }

inline void nvgBeginFrame(NVGcontext* ctx, int width, int height, float) {
    if (!ctx) return;
    ctx->frameWidth = width;
    ctx->frameHeight = height;
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

inline void nvgEndFrame(NVGcontext*) {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

inline void nvgBeginPath(NVGcontext* ctx) {
    if (ctx) ctx->path = _NVGPath{};
}

inline void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float) {
    if (!ctx) return;
    ctx->path = {x, y, w, h, 0.0f, true};
}

inline void nvgFillColor(NVGcontext* ctx, NVGcolor color) {
    if (ctx) ctx->fillColor = color;
}

inline void nvgFontSize(NVGcontext* ctx, float size) {
    if (ctx) ctx->fontSize = size;
}

inline void nvgTextAlign(NVGcontext* ctx, int align) {
    if (ctx) ctx->textAlign = align;
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
        if (glyph.character == c) return &glyph;
    }
    return nullptr;
}

inline void nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
    if (!ctx || !string) return;
    std::string text(end ? std::string(string, end) : std::string(string));
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    const float cellWidth = ctx->fontSize * 0.6f;
    const float cellHeight = ctx->fontSize;
    const float spacing = ctx->fontSize * 0.15f;
    float totalWidth = 0.0f;
    for (char ch : text) totalWidth += cellWidth + spacing;
    float startX = x;
    float startY = y;
    if (ctx->textAlign & NVG_ALIGN_CENTER) startX -= totalWidth * 0.5f;
    else if (ctx->textAlign & NVG_ALIGN_RIGHT) startX -= totalWidth;
    if (ctx->textAlign & NVG_ALIGN_MIDDLE) startY -= cellHeight * 0.5f;
    else if (ctx->textAlign & NVG_ALIGN_BOTTOM) startY -= cellHeight;
    glColor4f(ctx->fillColor.r, ctx->fillColor.g, ctx->fillColor.b, ctx->fillColor.a);
    float penX = startX;
    for (char ch : text) {
        if (ch == ' ') { penX += cellWidth + spacing; continue; }
        const SimpleGlyph* glyph = _findGlyph(ch);
        if (!glyph) { penX += cellWidth + spacing; continue; }
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
} // namespace
#endif
#ifdef USE_VULKAN
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#include <set>
#include <cstring>
#endif
#include <iostream>

Renderer::Renderer()
    : m_window(nullptr)
    , m_width(0)
    , m_height(0)
{
}

Renderer::~Renderer()
{
}

void Renderer::getWindowSize(int& width, int& height) const
{
    if (m_window) {
        glfwGetWindowSize(m_window, &width, &height);
    } else {
        width = m_width;
        height = m_height;
    }
}

// OpenGL Renderer Implementation
OpenGLRenderer::OpenGLRenderer()
    : m_videoTexture(0)
    , m_videoOpacity(1.0f)
    , m_3dObjectsOpacity(1.0f)
    , m_basicShaderProgram(0)
    , m_videoShaderProgram(0)
    , m_glassmorphismShaderProgram(0)
    , m_uiShaderProgram(0)
    , m_fullscreenQuadVAO(0)
    , m_fullscreenQuadVBO(0)
    , m_uiQuadVAO(0)
    , m_uiQuadVBO(0)
    , m_uiQuadEBO(0)
    , m_nextMeshId(1)
    , m_nextUIWindowId(1)
    , m_simpleNVG(nullptr)
{
    m_viewMatrix = glm::mat4(1.0f);
    m_projectionMatrix = glm::mat4(1.0f);
}

OpenGLRenderer::~OpenGLRenderer()
{
    shutdown();
}

bool OpenGLRenderer::initialize(GLFWwindow* window)
{
    if (!window) {
        std::cerr << "OpenGLRenderer::initialize: window is null" << std::endl;
        return false;
    }
    
    m_window = window;
    
#ifdef USE_OPENGL
    glfwMakeContextCurrent(window);
    if (glfwGetCurrentContext() != window) {
        std::cerr << "OpenGLRenderer::initialize: failed to make context current" << std::endl;
        return false;
    }
    
    // Загружаем функции OpenGL 3.3+
    if (!LoadOpenGLFunctions()) {
        std::cerr << "[ERROR] Failed to load OpenGL 3.3+ functions" << std::endl;
        return false;
    }
    
    glfwGetWindowSize(window, &m_width, &m_height);
    while (glGetError() != GL_NO_ERROR) {}
    glViewport(0, 0, m_width, m_height);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glViewport: " << error << std::endl;
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version) {
        std::cout << "OpenGL version: " << version << std::endl;
    } else {
        std::cerr << "Warning: glGetString(GL_VERSION) returned NULL - OpenGL functions may not be loaded" << std::endl;
    }

    m_basicShaderProgram = loadShader("shaders/basic.vert", "shaders/basic.frag");
    m_videoShaderProgram = loadShader("shaders/video.vert", "shaders/video.frag");
    m_glassmorphismShaderProgram = loadShader("shaders/glassmorphism.vert", "shaders/glassmorphism.frag");
    m_uiShaderProgram = loadShader("shaders/ui.vert", "shaders/ui.frag");

    if (m_basicShaderProgram == 0 || m_videoShaderProgram == 0 || m_uiShaderProgram == 0) {
        std::cerr << "[WARN] Some shader programs failed to load. Fallback paths enabled." << std::endl;
    }

    createFullscreenQuad();
    createUIQuad();

    if (!m_simpleNVG) {
        m_simpleNVG = reinterpret_cast<void*>(nvgCreateSimple());
    }

    return true;
#else
    std::cerr << "OpenGLRenderer::initialize: USE_OPENGL not defined" << std::endl;
    return false;
#endif
}

void OpenGLRenderer::shutdown()
{
#ifdef USE_OPENGL
    for (auto& mesh : m_meshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
    }
    m_meshes.clear();

    for (auto& window : m_uiWindows) {
        if (window.fbo != 0) {
            glDeleteFramebuffers(1, &window.fbo);
        }
        if (window.texture != 0) {
            glDeleteTextures(1, &window.texture);
        }
        if (window.rbo != 0) {
            glDeleteRenderbuffers(1, &window.rbo);
        }
    }
    m_uiWindows.clear();

    if (m_videoTexture != 0) {
        glDeleteTextures(1, &m_videoTexture);
        m_videoTexture = 0;
    }

    if (m_fullscreenQuadVAO != 0) {
        glDeleteVertexArrays(1, &m_fullscreenQuadVAO);
        glDeleteBuffers(1, &m_fullscreenQuadVBO);
        m_fullscreenQuadVAO = 0;
        m_fullscreenQuadVBO = 0;
    }

    if (m_uiQuadVAO != 0) {
        glDeleteVertexArrays(1, &m_uiQuadVAO);
        glDeleteBuffers(1, &m_uiQuadVBO);
        glDeleteBuffers(1, &m_uiQuadEBO);
        m_uiQuadVAO = 0;
        m_uiQuadVBO = 0;
        m_uiQuadEBO = 0;
    }

    if (m_basicShaderProgram != 0) {
        glDeleteProgram(m_basicShaderProgram);
        m_basicShaderProgram = 0;
    }
    if (m_videoShaderProgram != 0) {
        glDeleteProgram(m_videoShaderProgram);
        m_videoShaderProgram = 0;
    }
    if (m_glassmorphismShaderProgram != 0) {
        glDeleteProgram(m_glassmorphismShaderProgram);
        m_glassmorphismShaderProgram = 0;
    }
    if (m_uiShaderProgram != 0) {
        glDeleteProgram(m_uiShaderProgram);
        m_uiShaderProgram = 0;
    }

    if (m_simpleNVG) {
        nvgDeleteSimple(reinterpret_cast<NVGcontext*>(m_simpleNVG));
        m_simpleNVG = nullptr;
    }
#endif
}

void OpenGLRenderer::beginFrame()
{
#ifdef USE_OPENGL
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void OpenGLRenderer::endFrame()
{
    // OpenGL автоматически обновляет буфер через glfwSwapBuffers
}

void OpenGLRenderer::renderVideoBackground(const uint8_t* data, uint32_t width, uint32_t height)
{
#ifdef USE_OPENGL
    if (!data || width == 0 || height == 0) {
        return;
    }

    if (m_videoTexture == 0) {
        glGenTextures(1, &m_videoTexture);
        glBindTexture(GL_TEXTURE_2D, m_videoTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glDisable(GL_DEPTH_TEST);

    if (m_videoShaderProgram != 0 && m_fullscreenQuadVAO != 0) {
        glUseProgram(m_videoShaderProgram);
        GLint opacityLoc = glGetUniformLocation(m_videoShaderProgram, "opacity");
        GLint texLoc = glGetUniformLocation(m_videoShaderProgram, "videoTexture");
        if (opacityLoc >= 0) glUniform1f(opacityLoc, m_videoOpacity);
        if (texLoc >= 0) glUniform1i(texLoc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_videoTexture);

        glBindVertexArray(m_fullscreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glUseProgram(0);
    } else {
        glUseProgram(0);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_videoTexture);
        glColor4f(1.0f, 1.0f, 1.0f, m_videoOpacity);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 1.0f);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    glEnable(GL_DEPTH_TEST);
#endif
}

void OpenGLRenderer::render3DObjects(const std::vector<glm::mat4>& transforms, 
                                     const std::vector<uint32_t>& meshIds)
{
#ifdef USE_OPENGL
    if (transforms.size() != meshIds.size() || transforms.empty()) {
        return;
    }

    if (m_basicShaderProgram != 0) {
        glUseProgram(m_basicShaderProgram);
        GLint modelLoc = glGetUniformLocation(m_basicShaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(m_basicShaderProgram, "view");
        GLint projLoc = glGetUniformLocation(m_basicShaderProgram, "projection");
        GLint colorLoc = glGetUniformLocation(m_basicShaderProgram, "objectColor");
        GLint opacityLoc = glGetUniformLocation(m_basicShaderProgram, "opacity");
        GLint lightingLoc = glGetUniformLocation(m_basicShaderProgram, "useLighting");

        if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
        if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
        if (colorLoc >= 0) glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        if (opacityLoc >= 0) glUniform1f(opacityLoc, m_3dObjectsOpacity);
        if (lightingLoc >= 0) glUniform1i(lightingLoc, 1);

        for (size_t i = 0; i < transforms.size(); ++i) {
            uint32_t meshId = meshIds[i];
            if (meshId == 0 || meshId > m_meshes.size()) {
                continue;
            }
            const Mesh& mesh = m_meshes[meshId - 1];
            if (mesh.vao == 0) continue;

            if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transforms[i]));
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glUseProgram(0);
    } else {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(m_projectionMatrix));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(m_viewMatrix));
        for (size_t i = 0; i < transforms.size(); ++i) {
            uint32_t meshId = meshIds[i];
            if (meshId == 0 || meshId > m_meshes.size()) {
                continue;
            }
            const Mesh& mesh = m_meshes[meshId - 1];
            if (mesh.vao == 0) continue;
            glPushMatrix();
            glMultMatrixf(glm::value_ptr(transforms[i]));
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
            glPopMatrix();
        }
    }
#endif
}

void OpenGLRenderer::setCameraMatrix(const glm::mat4& view, const glm::mat4& projection)
{
    m_viewMatrix = view;
    m_projectionMatrix = projection;
}

void OpenGLRenderer::renderUIWindows()
{
#ifdef USE_OPENGL
    if (m_uiWindows.empty()) {
        return;
    }

    if (m_uiShaderProgram == 0 || m_uiQuadVAO == 0) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_uiShaderProgram);
    GLint modelLoc = glGetUniformLocation(m_uiShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_uiShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_uiShaderProgram, "projection");
    GLint opacityLoc = glGetUniformLocation(m_uiShaderProgram, "opacity");
    GLint texLoc = glGetUniformLocation(m_uiShaderProgram, "uiTexture");

    if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
    if (texLoc >= 0) glUniform1i(texLoc, 0);

    glm::mat4 viewInverse = glm::inverse(m_viewMatrix);

    for (const auto& window : m_uiWindows) {
        if (window.texture == 0) continue;
        renderUIWindowContent(window);

        glm::mat4 rotation = glm::mat4(1.0f);
        if (window.billboard) {
            rotation = glm::mat4(glm::mat3(viewInverse));
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0f), window.position) * rotation;
        model = glm::scale(model, glm::vec3(window.size.x, window.size.y, 1.0f));

        if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        if (opacityLoc >= 0) glUniform1f(opacityLoc, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, window.texture);

        glBindVertexArray(m_uiQuadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    glUseProgram(0);
    glDisable(GL_BLEND);
#endif
}

uint32_t OpenGLRenderer::createUIWindow(const std::string& title,
                                        const std::string& subtitle,
                                        const glm::vec3& position,
                                        const glm::vec2& size,
                                        bool hasButton,
                                        const std::string& buttonText)
{
#ifdef USE_OPENGL
    const int baseResolution = 512;
    int pixelWidth = static_cast<int>(std::max(256.0f, size.x * baseResolution));
    int pixelHeight = static_cast<int>(std::max(256.0f, size.y * baseResolution));

    GLuint fbo = 0;
    GLuint texture = 0;
    GLuint rbo = 0;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelWidth, pixelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, pixelWidth, pixelHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ERROR] Failed to create UI framebuffer" << std::endl;
    }

    glViewport(0, 0, pixelWidth, pixelHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_simpleNVG) {
        m_simpleNVG = reinterpret_cast<void*>(nvgCreateSimple());
    }
    
    NVGcontext* nvg = reinterpret_cast<NVGcontext*>(m_simpleNVG);

    nvgBeginFrame(nvg, pixelWidth, pixelHeight, 1.0f);

    // Background panel
    nvgBeginPath(nvg);
    nvgRoundedRect(nvg, 0.0f, 0.0f, static_cast<float>(pixelWidth), static_cast<float>(pixelHeight), 20.0f);
    nvgFillColor(nvg, nvgRGBAf(0.15f, 0.18f, 0.22f, 0.65f));
    nvgFill(nvg);

    // Header accent
    nvgBeginPath(nvg);
    nvgRoundedRect(nvg, 20.0f, 20.0f, static_cast<float>(pixelWidth - 40), 60.0f, 12.0f);
    nvgFillColor(nvg, nvgRGBAf(0.45f, 0.55f, 0.95f, 0.35f));
    nvgFill(nvg);

    // Title
    nvgFontSize(nvg, 42.0f);
    nvgFillColor(nvg, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.95f));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(nvg, 40.0f, 40.0f, title.c_str(), nullptr);

    // Subtitle
    nvgFontSize(nvg, 26.0f);
    nvgFillColor(nvg, nvgRGBAf(0.85f, 0.88f, 0.95f, 0.85f));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(nvg, 40.0f, 100.0f, subtitle.c_str(), nullptr);

    if (hasButton) {
        float buttonWidth = static_cast<float>(pixelWidth) - 80.0f;
        float buttonHeight = 70.0f;
        float buttonX = 40.0f;
        float buttonY = static_cast<float>(pixelHeight) - buttonHeight - 40.0f;

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, buttonX, buttonY, buttonWidth, buttonHeight, 18.0f);
        nvgFillColor(nvg, nvgRGBAf(0.35f, 0.55f, 0.95f, 0.65f));
        nvgFill(nvg);

        nvgFontSize(nvg, 30.0f);
        nvgFillColor(nvg, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.95f));
        nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(nvg, buttonX + buttonWidth * 0.5f, buttonY + buttonHeight * 0.5f, buttonText.c_str(), nullptr);
    }

    nvgEndFrame(nvg);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);

    UIWindow window{};
    window.id = m_nextUIWindowId++;
    window.fbo = fbo;
    window.texture = texture;
    window.rbo = rbo;
    window.position = position;
    window.size = size;
    window.billboard = true;
    window.title = title;
    window.subtitle = subtitle;
    window.hasButton = hasButton;
    window.buttonText = buttonText;
    window.pixelWidth = pixelWidth;
    window.pixelHeight = pixelHeight;

    m_uiWindows.push_back(window);
    return window.id;
#else
    (void)title;
    (void)subtitle;
    (void)position;
    (void)size;
    (void)hasButton;
    (void)buttonText;
    return 0;
#endif
}

void OpenGLRenderer::renderUIWindowContent(const UIWindow& window)
{
#ifdef USE_OPENGL
    NVGcontext* nvg = reinterpret_cast<NVGcontext*>(m_simpleNVG);
    if (!nvg || window.fbo == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, window.fbo);
    glViewport(0, 0, window.pixelWidth, window.pixelHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    nvgBeginFrame(nvg, window.pixelWidth, window.pixelHeight, 1.0f);

    nvgBeginPath(nvg);
    nvgRoundedRect(nvg, 0.0f, 0.0f, static_cast<float>(window.pixelWidth), static_cast<float>(window.pixelHeight), 20.0f);
    nvgFillColor(nvg, nvgRGBAf(0.15f, 0.18f, 0.22f, 0.65f));
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgRoundedRect(nvg, 20.0f, 20.0f, static_cast<float>(window.pixelWidth - 40), 60.0f, 12.0f);
    nvgFillColor(nvg, nvgRGBAf(0.45f, 0.55f, 0.95f, 0.35f));
    nvgFill(nvg);

    nvgFontSize(nvg, 42.0f);
    nvgFillColor(nvg, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.95f));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(nvg, 40.0f, 40.0f, window.title.c_str(), nullptr);

    nvgFontSize(nvg, 26.0f);
    nvgFillColor(nvg, nvgRGBAf(0.85f, 0.88f, 0.95f, 0.85f));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(nvg, 40.0f, 100.0f, window.subtitle.c_str(), nullptr);

    if (window.hasButton) {
        float buttonWidth = static_cast<float>(window.pixelWidth) - 80.0f;
        float buttonHeight = 70.0f;
        float buttonX = 40.0f;
        float buttonY = static_cast<float>(window.pixelHeight) - buttonHeight - 40.0f;

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, buttonX, buttonY, buttonWidth, buttonHeight, 18.0f);
        nvgFillColor(nvg, nvgRGBAf(0.35f, 0.55f, 0.95f, 0.65f));
        nvgFill(nvg);

        nvgFontSize(nvg, 30.0f);
        nvgFillColor(nvg, nvgRGBAf(1.0f, 1.0f, 1.0f, 0.95f));
        nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(nvg, buttonX + buttonWidth * 0.5f, buttonY + buttonHeight * 0.5f, window.buttonText.c_str(), nullptr);
    }

    nvgEndFrame(nvg);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
#endif
}

uint32_t OpenGLRenderer::loadShader(const char* vertexPath, const char* fragmentPath)
{
#ifdef USE_OPENGL
    std::string vertexCode = readShaderFile(vertexPath);
    std::string fragmentCode = readShaderFile(fragmentPath);
    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;
    }

    uint32_t vertexShader = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    uint32_t fragmentShader = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return 0;
    }

    uint32_t program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "[ERROR] Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
#else
    (void)vertexPath;
    (void)fragmentPath;
    return 0;
#endif
}

uint32_t OpenGLRenderer::compileShader(const char* source, uint32_t type)
{
#ifdef USE_OPENGL
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[ERROR] Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
#else
    (void)source;
    (void)type;
    return 0;
#endif
}

std::string OpenGLRenderer::readShaderFile(const char* filepath)
{
#ifdef USE_OPENGL
    std::string content;
    std::ifstream file(filepath, std::ios::in);
    std::vector<std::string> fallbackPaths = {
        filepath,
        std::string("../") + filepath,
        std::string("../../") + filepath,
        std::string("../../../") + filepath,
        std::string("shaders/") + filepath,
        std::string("ARLauncher/shaders/") + filepath
    };

    for (const auto& path : fallbackPaths) {
        file.open(path, std::ios::in);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            content = ss.str();
            file.close();
            return content;
        }
    }

    std::cerr << "[ERROR] Unable to open shader file: " << filepath << std::endl;
    return content;
#else
    (void)filepath;
    return std::string();
#endif
}

void OpenGLRenderer::createFullscreenQuad()
{
#ifdef USE_OPENGL
    if (m_fullscreenQuadVAO != 0) return;

    const float vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_fullscreenQuadVAO);
    glGenBuffers(1, &m_fullscreenQuadVBO);

    glBindVertexArray(m_fullscreenQuadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

void OpenGLRenderer::createUIQuad()
{
#ifdef USE_OPENGL
    if (m_uiQuadVAO != 0) return;

    const float vertices[] = {
        // positions          // texCoords
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f,
    };

    const unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &m_uiQuadVAO);
    glGenBuffers(1, &m_uiQuadVBO);
    glGenBuffers(1, &m_uiQuadEBO);

    glBindVertexArray(m_uiQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiQuadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

#ifdef USE_VULKAN
// VulkanRenderer Implementation
VulkanRenderer::VulkanRenderer()
    : m_instance(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
    , m_device(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_presentQueue(VK_NULL_HANDLE)
    , m_graphicsQueueFamily(0)
    , m_presentQueueFamily(0)
    , m_surface(VK_NULL_HANDLE)
    , m_swapchain(VK_NULL_HANDLE)
    , m_swapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM)
    , m_renderPass(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
    , m_currentFrame(0)
    , m_currentImageIndex(0)
    , m_initialized(false)
    , m_nextMeshId(1)
    , m_videoImage(VK_NULL_HANDLE)
    , m_videoImageMemory(VK_NULL_HANDLE)
    , m_videoImageView(VK_NULL_HANDLE)
    , m_videoSampler(VK_NULL_HANDLE)
    , m_videoTextureInitialized(false)
{
    m_swapchainExtent = {0, 0};
}

VulkanRenderer::~VulkanRenderer()
{
    shutdown();
}

bool VulkanRenderer::initialize(GLFWwindow* window)
{
    if (!window) {
        std::cerr << "VulkanRenderer::initialize: window is null" << std::endl;
        return false;
    }
    
    m_window = window;
    glfwGetWindowSize(window, &m_width, &m_height);
    
    if (!createInstance()) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return false;
    }
    
    if (!createSurface()) {
        std::cerr << "Failed to create Vulkan surface" << std::endl;
        return false;
    }
    
    if (!pickPhysicalDevice()) {
        std::cerr << "Failed to pick physical device" << std::endl;
        return false;
    }
    
    if (!createLogicalDevice()) {
        std::cerr << "Failed to create logical device" << std::endl;
        return false;
    }
    
    if (!createSwapchain()) {
        std::cerr << "Failed to create swapchain" << std::endl;
        return false;
    }
    
    if (!createImageViews()) {
        std::cerr << "Failed to create image views" << std::endl;
        return false;
    }
    
    if (!createRenderPass()) {
        std::cerr << "Failed to create render pass" << std::endl;
        return false;
    }
    
    if (!createFramebuffers()) {
        std::cerr << "Failed to create framebuffers" << std::endl;
        return false;
    }
    
    if (!createCommandPool()) {
        std::cerr << "Failed to create command pool" << std::endl;
        return false;
    }
    
    if (!createCommandBuffers()) {
        std::cerr << "Failed to create command buffers" << std::endl;
        return false;
    }
    
    if (!createSyncObjects()) {
        std::cerr << "Failed to create sync objects" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "[OK] VulkanRenderer initialized successfully" << std::endl;
    std::cout << "   Swapchain: " << m_swapchainExtent.width << "x" << m_swapchainExtent.height << std::endl;
    
    return true;
}

void VulkanRenderer::shutdown()
{
    if (!m_initialized) return;
    
    vkDeviceWaitIdle(m_device);
    
    // Очистка видео текстуры
    if (m_videoSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_videoSampler, nullptr);
        m_videoSampler = VK_NULL_HANDLE;
    }
    if (m_videoImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_videoImageView, nullptr);
        m_videoImageView = VK_NULL_HANDLE;
    }
    if (m_videoImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_videoImage, nullptr);
        vkFreeMemory(m_device, m_videoImageMemory, nullptr);
        m_videoImage = VK_NULL_HANDLE;
        m_videoImageMemory = VK_NULL_HANDLE;
    }
    
    // Очистка мешей
    for (auto& mesh : m_meshes) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, mesh.vertexBuffer, nullptr);
            vkFreeMemory(m_device, mesh.vertexBufferMemory, nullptr);
        }
        if (mesh.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, mesh.indexBuffer, nullptr);
            vkFreeMemory(m_device, mesh.indexBufferMemory, nullptr);
        }
    }
    m_meshes.clear();
    
    cleanupSwapchain();
    
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
    
    for (size_t i = 0; i < m_inFlightFences.size(); i++) {
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
    }
    
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

void VulkanRenderer::beginFrame()
{
    if (!m_initialized) return;
    
    uint32_t frameIndex = m_currentFrame % m_swapchainImages.size();
    
    vkWaitForFences(m_device, 1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFences[frameIndex]);
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, 
                                            m_imageAvailableSemaphores[frameIndex], 
                                            VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to acquire swapchain image" << std::endl;
        return;
    }
    
    // Сохраняем индекс изображения для endFrame
    m_currentImageIndex = imageIndex;
    
    vkResetCommandBuffer(m_commandBuffers[imageIndex], 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin recording command buffer" << std::endl;
        return;
    }
    
    // Если есть видеоизображение, копируем его в swapchain до начала render pass
    if (m_videoTextureInitialized && m_videoImage != VK_NULL_HANDLE) {
        // Гарантируем, что источник находится в правильном layout
        // (updateVideoTexture переводит его в TRANSFER_SRC_OPTIMAL)
        copyVideoToSwapchain(m_commandBuffers[imageIndex], m_swapchainImages[imageIndex], m_swapchainExtent.width, m_swapchainExtent.height);
    }
    
    // Начинаем render pass (ничего не рисуем, но оставляем для будущих оверлеев)
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::endFrame()
{
    if (!m_initialized) return;
    
    uint32_t imageIndex = m_currentImageIndex;
    uint32_t frameIndex = m_currentFrame % m_swapchainImages.size();
    
    vkCmdEndRenderPass(m_commandBuffers[imageIndex]);
    
    if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS) {
        std::cerr << "Failed to record command buffer" << std::endl;
        return;
    }
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[frameIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[frameIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[frameIndex]) != VK_SUCCESS) {
        std::cerr << "Failed to submit draw command buffer" << std::endl;
        return;
    }
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapchains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    
    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present swapchain image" << std::endl;
    }
    
    m_currentFrame = (m_currentFrame + 1) % m_swapchainImages.size();
}

void VulkanRenderer::renderVideoBackground(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (!m_initialized || !data || width == 0 || height == 0) {
        return;
    }
    
    // Создаем или обновляем текстуру для видео
    if (!m_videoTextureInitialized) {
        if (!createVideoTexture(width, height)) {
            std::cerr << "Failed to create video texture" << std::endl;
            return;
        }
        m_videoTextureInitialized = true;
    }
    
    // Обновляем текстуру данными с камеры
    updateVideoTexture(data, width, height);
    
    // TODO: Рендеринг полноэкранного квада с видео текстурой
    // Для этого нужен graphics pipeline с шейдерами
    // Пока текстура обновляется, но не отображается
}

void VulkanRenderer::render3DObjects(const std::vector<glm::mat4>& transforms, 
                                     const std::vector<uint32_t>& meshIds)
{
    if (!m_initialized || transforms.empty() || meshIds.empty()) return;
    if (transforms.size() != meshIds.size()) return;
    
    uint32_t imageIndex = m_currentImageIndex;
    VkCommandBuffer commandBuffer = m_commandBuffers[imageIndex];
    
    // Рендерим каждый объект
    for (size_t i = 0; i < transforms.size(); i++) {
        uint32_t meshId = meshIds[i];
        if (meshId == 0 || meshId > m_meshes.size()) continue;
        
        const VulkanMesh& mesh = m_meshes[meshId - 1];
        
        // Применяем трансформацию (упрощенная версия - без pipeline)
        // В полноценной реализации нужен graphics pipeline с шейдерами
        
        // Пока просто очищаем экран с цветом для демонстрации
        // В реальной реализации здесь будет vkCmdDrawIndexed
        (void)mesh;
        (void)transforms[i];
    }
    
    // Для демонстрации просто рисуем цветной фон
    // В полноценной реализации здесь будет рендеринг через pipeline
}

void VulkanRenderer::setCameraMatrix(const glm::mat4& view, const glm::mat4& projection)
{
    m_viewMatrix = view;
    m_projectionMatrix = projection;
}

uint32_t VulkanRenderer::createMesh(const std::vector<float>& vertices, 
                                     const std::vector<uint32_t>& indices)
{
    if (!m_initialized || vertices.empty() || indices.empty()) {
        return 0;
    }
    
    VulkanMesh mesh{};
    
    // Создаем vertex buffer
    VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    if (!createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      mesh.vertexBuffer, mesh.vertexBufferMemory)) {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return 0;
    }
    
    // Заполняем vertex buffer
    void* data;
    vkMapMemory(m_device, mesh.vertexBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(vertexBufferSize));
    vkUnmapMemory(m_device, mesh.vertexBufferMemory);
    
    // Создаем index buffer
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    if (!createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      mesh.indexBuffer, mesh.indexBufferMemory)) {
        std::cerr << "Failed to create index buffer" << std::endl;
        vkDestroyBuffer(m_device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(m_device, mesh.vertexBufferMemory, nullptr);
        return 0;
    }
    
    // Заполняем index buffer
    vkMapMemory(m_device, mesh.indexBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(indexBufferSize));
    vkUnmapMemory(m_device, mesh.indexBufferMemory);
    
    mesh.indexCount = static_cast<uint32_t>(indices.size());
    m_meshes.push_back(mesh);
    
    return m_nextMeshId++;
}

void VulkanRenderer::destroyMesh(uint32_t meshId)
{
    if (meshId == 0 || meshId > m_meshes.size()) return;
    
    VulkanMesh& mesh = m_meshes[meshId - 1];
    
    if (mesh.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(m_device, mesh.vertexBufferMemory, nullptr);
        mesh.vertexBuffer = VK_NULL_HANDLE;
    }
    
    if (mesh.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, mesh.indexBuffer, nullptr);
        vkFreeMemory(m_device, mesh.indexBufferMemory, nullptr);
        mesh.indexBuffer = VK_NULL_HANDLE;
    }
    
    mesh.indexCount = 0;
}

uint32_t VulkanRenderer::createTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
    // TODO: Реализовать создание текстуры
    (void)data;
    (void)width;
    (void)height;
    return 0;
}

void VulkanRenderer::destroyTexture(uint32_t textureId)
{
    // TODO: Реализовать удаление текстуры
    (void)textureId;
}

// Вспомогательные методы Vulkan
bool VulkanRenderer::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ARLauncher";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ARLauncher Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;
    
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        return false;
    }
    return true;
}

bool VulkanRenderer::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    // Выбираем первое подходящее устройство
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        // Проверяем поддержку очередей
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        
        bool hasGraphicsQueue = false;
        bool hasPresentQueue = false;
        
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsQueueFamily = i;
                hasGraphicsQueue = true;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
            if (presentSupport) {
                m_presentQueueFamily = i;
                hasPresentQueue = true;
            }
        }
        
        if (hasGraphicsQueue && hasPresentQueue) {
            m_physicalDevice = device;
            return true;
        }
    }
    
    return false;
}

bool VulkanRenderer::createLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {m_graphicsQueueFamily, m_presentQueueFamily};
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        return false;
    }
    
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentQueueFamily, 0, &m_presentQueue);
    
    return true;
}

bool VulkanRenderer::createSwapchain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());
    
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    
    if (capabilities.currentExtent.width != UINT32_MAX) {
        m_swapchainExtent = capabilities.currentExtent;
    } else {
        m_swapchainExtent.width = static_cast<uint32_t>(m_width);
        m_swapchainExtent.height = static_cast<uint32_t>(m_height);
    }
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queueFamilyIndices[] = {m_graphicsQueueFamily, m_presentQueueFamily};
    if (m_graphicsQueueFamily != m_presentQueueFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        return false;
    }
    
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
    
    m_swapchainImageFormat = surfaceFormat.format;
    
    return true;
}

bool VulkanRenderer::createImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());
    
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());
    
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {m_swapchainImageViews[i]};
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createCommandBuffers()
{
    m_commandBuffers.resize(m_swapchainFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(m_swapchainImages.size());
    m_renderFinishedSemaphores.resize(m_swapchainImages.size());
    m_inFlightFences.resize(m_swapchainImages.size());
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

void VulkanRenderer::cleanupSwapchain()
{
    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    
    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanRenderer::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);
    
    cleanupSwapchain();
    
    glfwGetWindowSize(m_window, &m_width, &m_height);
    
    createSwapchain();
    createImageViews();
    createFramebuffers();
}

bool VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                                  VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return false;
    }
    
    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
    
    return true;
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return 0;
}

bool VulkanRenderer::createVideoTexture(uint32_t width, uint32_t height)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_videoImage) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_videoImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_videoImageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_videoImage, nullptr);
        return false;
    }
    
    vkBindImageMemory(m_device, m_videoImage, m_videoImageMemory, 0);
    
    // Создаем image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_videoImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_videoImageView) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_videoImage, nullptr);
        vkFreeMemory(m_device, m_videoImageMemory, nullptr);
        return false;
    }
    
    // Создаем sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_videoSampler) != VK_SUCCESS) {
        vkDestroyImageView(m_device, m_videoImageView, nullptr);
        vkDestroyImage(m_device, m_videoImage, nullptr);
        vkFreeMemory(m_device, m_videoImageMemory, nullptr);
        return false;
    }
    // Переводим видеоизображение в layout источника копирования (для дальнейших копий)
    transitionImageLayout(m_videoImage, VK_FORMAT_R8G8B8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    return true;
}

void VulkanRenderer::updateVideoTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (!m_videoImage || !data) return;
    
    // Получаем требования к памяти для изображения
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.mipLevel = 0;
    subResource.arrayLayer = 0;
    
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_device, m_videoImage, &subResource, &subResourceLayout);
    
    // Копируем данные в память изображения
    void* mapped;
    vkMapMemory(m_device, m_videoImageMemory, 0, VK_WHOLE_SIZE, 0, &mapped);
    
    uint8_t* dst = static_cast<uint8_t*>(mapped) + subResourceLayout.offset;
    const uint32_t rowPitch = width * 3; // RGB
    
    for (uint32_t y = 0; y < height; y++) {
        memcpy(dst + y * subResourceLayout.rowPitch, 
               data + y * rowPitch, 
               rowPitch);
    }
    
    vkUnmapMemory(m_device, m_videoImageMemory);
    // Layout уже установлен в TRANSFER_SRC_OPTIMAL при создании
}
#endif

#ifdef USE_VULKAN
void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat /*format*/, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanRenderer::copyVideoToSwapchain(VkCommandBuffer cmd, VkImage dstSwapchainImage, uint32_t width, uint32_t height)
{
    // Переводим swapchain в layout для назначения копирования
    VkImageMemoryBarrier toDst{};
    toDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDst.image = dstSwapchainImage;
    toDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toDst.subresourceRange.baseMipLevel = 0;
    toDst.subresourceRange.levelCount = 1;
    toDst.subresourceRange.baseArrayLayer = 0;
    toDst.subresourceRange.layerCount = 1;
    toDst.srcAccessMask = 0;
    toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &toDst
    );

    // Регион копирования (matching the smaller side, simple stretch to fit)
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};

    copyRegion.extent.width = std::min(width, m_swapchainExtent.width);
    copyRegion.extent.height = std::min(height, m_swapchainExtent.height);
    copyRegion.extent.depth = 1;

    vkCmdCopyImage(
        cmd,
        m_videoImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion
    );

    // Возвращаем swapchain изображение в layout для color attachment
    VkImageMemoryBarrier toColor{};
    toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toColor.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.image = dstSwapchainImage;
    toColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toColor.subresourceRange.baseMipLevel = 0;
    toColor.subresourceRange.levelCount = 1;
    toColor.subresourceRange.baseArrayLayer = 0;
    toColor.subresourceRange.layerCount = 1;
    toColor.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &toColor
    );
}
#endif

std::unique_ptr<Renderer> createRenderer(bool useVulkan)
{
#ifdef USE_VULKAN
    if (useVulkan) {
        return std::make_unique<VulkanRenderer>();
    }
#endif
    return std::make_unique<OpenGLRenderer>();
}

