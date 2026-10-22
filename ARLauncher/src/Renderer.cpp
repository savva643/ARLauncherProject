#include "Renderer.h"
#include "FontRenderer.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/packing.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fstream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <iostream>
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
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
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
// Примечание: glDrawArrays, glDrawElements, glActiveTexture уже объявлены в GL/gl.h, не переопределяем

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
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
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
    LOAD_GL_FUNC(glUniform2f);
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
    // glDrawArrays, glDrawElements, glActiveTexture уже доступны из GL/gl.h
    
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

// Удалены глифы - теперь используется FontRenderer через Text класс
// Простой fallback для nvgText (используется только если FontRenderer недоступен)
inline void nvgText(NVGcontext* ctx, float x, float y, const char* string, const char* end) {
    if (!ctx || !string) return;
    // Простой fallback - просто выводим текст как точки (для отладки)
    // В реальном использовании должен использоваться FontRenderer через Text класс
    std::string text(end ? std::string(string, end) : std::string(string));
    glColor4f(ctx->fillColor.r, ctx->fillColor.g, ctx->fillColor.b, ctx->fillColor.a);
    glPointSize(ctx->fontSize * 0.3f);
    glBegin(GL_POINTS);
    float penX = x;
    for (size_t i = 0; i < text.length() && i < 100; ++i) {
        if (text[i] != ' ') {
            glVertex2f(penX, y);
        }
        penX += ctx->fontSize * 0.5f;
    }
    glEnd();
}
} // namespace

#ifdef USE_VULKAN
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#include <set>
#include <cstring>
#include <array>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>
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
    , m_videoTextureWidth(0)
    , m_videoTextureHeight(0)
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
    , m_fontRenderer(nullptr)
{
    m_viewMatrix = glm::mat4(1.0f);
    m_projectionMatrix = glm::mat4(1.0f);
    
    // Инициализируем FontRenderer
    m_fontRenderer = new FontRenderer();
    if (m_fontRenderer) {
        m_fontRenderer->initialize();
        // Загружаем системный шрифт (если доступен)
        // В Linux обычно: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
        // В Windows: C:/Windows/Fonts/arial.ttf
        #ifdef _WIN32
        m_fontRenderer->loadFont("C:/Windows/Fonts/arial.ttf", 48);
        #else
        m_fontRenderer->loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
        #endif
    }
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
    
    if (m_fontRenderer) {
        m_fontRenderer->cleanup();
        delete m_fontRenderer;
        m_fontRenderer = nullptr;
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

    // Создаем или обновляем текстуру для видео (только обновление данных, без рендеринга)
    if (m_videoTexture == 0) {
        glGenTextures(1, &m_videoTexture);
        glBindTexture(GL_TEXTURE_2D, m_videoTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_videoTextureWidth = width;
        m_videoTextureHeight = height;
        // Создаем текстуру с начальными данными
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_videoTexture);
        // Обновляем текстуру только если размер изменился
        if (m_videoTextureWidth != width || m_videoTextureHeight != height) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            m_videoTextureWidth = width;
            m_videoTextureHeight = height;
        } else {
            // Используем glTexSubImage2D для обновления существующей текстуры (быстрее, стрим)
            // Оптимизация для достижения 30 FPS:
            // 1. Используем GL_UNPACK_ALIGNMENT=1 для RGB (3 байта на пиксель)
            // 2. Отключаем синхронизацию для максимальной скорости
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Используем полную ширину
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Возвращаем обратно
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        }
    }
    
    // НЕ рендерим здесь - рендеринг будет в renderStoredVideoBackground()
#endif
}

void OpenGLRenderer::renderStoredVideoBackground()
{
#ifdef USE_OPENGL
    if (m_videoTexture == 0) {
        return; // Нет видео текстуры
    }

    glDisable(GL_DEPTH_TEST);

    if (m_videoShaderProgram != 0 && m_fullscreenQuadVAO != 0) {
        glUseProgram(m_videoShaderProgram);
        GLint opacityLoc = glGetUniformLocation(m_videoShaderProgram, "opacity");
        GLint texLoc = glGetUniformLocation(m_videoShaderProgram, "videoTexture");
        GLint videoAspectLoc = glGetUniformLocation(m_videoShaderProgram, "videoAspect");
        GLint videoOffsetLoc = glGetUniformLocation(m_videoShaderProgram, "videoOffset");
        
        if (opacityLoc >= 0) glUniform1f(opacityLoc, m_videoOpacity);
        if (texLoc >= 0) glUniform1i(texLoc, 0);
        
        // Вычисляем соотношение сторон для правильного отображения
        float windowAspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        float videoAspect = (m_videoTextureWidth > 0 && m_videoTextureHeight > 0) 
            ? static_cast<float>(m_videoTextureWidth) / static_cast<float>(m_videoTextureHeight) 
            : windowAspect;
        
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        
        if (videoAspect > windowAspect) {
            // Видео шире - подгоняем по ширине
            offsetY = (1.0f - windowAspect / videoAspect) * 0.5f;
        } else {
            // Видео выше - подгоняем по высоте
            offsetX = (1.0f - videoAspect / windowAspect) * 0.5f;
        }
        
        if (videoAspectLoc >= 0) glUniform2f(videoAspectLoc, videoAspect, windowAspect);
        if (videoOffsetLoc >= 0) glUniform2f(videoOffsetLoc, offsetX, offsetY);

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
        
        // Вычисляем соотношение сторон для правильного отображения
        float windowAspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        float videoAspect = (m_videoTextureWidth > 0 && m_videoTextureHeight > 0) 
            ? static_cast<float>(m_videoTextureWidth) / static_cast<float>(m_videoTextureHeight) 
            : windowAspect;
        
        float displayWidth = 1.0f;
        float displayHeight = 1.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        
        if (videoAspect > windowAspect) {
            // Видео шире - подгоняем по ширине
            displayWidth = 1.0f;
            displayHeight = windowAspect / videoAspect;
            offsetY = (1.0f - displayHeight) * 0.5f;
        } else {
            // Видео выше - подгоняем по высоте
            displayWidth = videoAspect / windowAspect;
            displayHeight = 1.0f;
            offsetX = (1.0f - displayWidth) * 0.5f;
        }
        
        glBegin(GL_QUADS);
        // QImage имеет координаты сверху вниз (0,0 вверху), OpenGL текстуры снизу вверх (0,0 внизу)
        // Переворачиваем координаты текстуры по Y для правильного отображения
        // Верхний левый угол экрана -> верх текстуры (Y=0)
        glTexCoord2f(0.0f, 0.0f); glVertex2f(offsetX, offsetY);
        // Верхний правый угол экрана -> верх текстуры (Y=0)
        glTexCoord2f(1.0f, 0.0f); glVertex2f(offsetX + displayWidth, offsetY);
        // Нижний правый угол экрана -> низ текстуры (Y=1)
        glTexCoord2f(1.0f, 1.0f); glVertex2f(offsetX + displayWidth, offsetY + displayHeight);
        // Нижний левый угол экрана -> низ текстуры (Y=1)
        glTexCoord2f(0.0f, 1.0f); glVertex2f(offsetX, offsetY + displayHeight);
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

    // Включаем depth test и отключаем culling для отображения обеих сторон
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Отключаем culling чтобы видеть обе стороны кубаа

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

#ifdef USE_VULKAN
uint32_t VulkanRenderer::createUIWindow(const std::string& title,
                                        const std::string& subtitle,
                                        const glm::vec3& position,
                                        const glm::vec2& size,
                                        bool hasButton,
                                        const std::string& buttonText)
{
#ifdef USE_VULKAN
    const int baseResolution = 512;
    int pixelWidth = static_cast<int>(std::max(256.0f, size.x * baseResolution));
    int pixelHeight = static_cast<int>(std::max(256.0f, size.y * baseResolution));

    std::vector<uint8_t> bitmap(pixelWidth * pixelHeight * 4, 0);

    // Background panel
    uint8_t r = static_cast<uint8_t>(0.15f * 255);
    uint8_t g = static_cast<uint8_t>(0.18f * 255);
    uint8_t b = static_cast<uint8_t>(0.22f * 255);
    uint8_t a = static_cast<uint8_t>(0.65f * 255);
    for (int y = 0; y < pixelHeight; ++y) {
        for (int x = 0; x < pixelWidth; ++x) {
            int idx = (y * pixelWidth + x) * 4;
            bitmap[idx + 0] = r;
            bitmap[idx + 1] = g;
            bitmap[idx + 2] = b;
            bitmap[idx + 3] = a;
        }
    }

    if (m_fontRenderer && m_fontRenderer->isFontLoaded()) {
        m_fontRenderer->renderTextToBitmap(title, 40.0f, 60.0f,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.95f), 42.0f / 48.0f,
            bitmap.data(), pixelWidth, pixelHeight);
        m_fontRenderer->renderTextToBitmap(subtitle, 40.0f, 120.0f,
            glm::vec4(0.85f, 0.88f, 0.95f, 0.85f), 26.0f / 48.0f,
            bitmap.data(), pixelWidth, pixelHeight);
        if (hasButton && !buttonText.empty()) {
            int btnX = 40, btnY = pixelHeight - 100;
            int btnW = pixelWidth - 80, btnH = 60;
            for (int by = btnY; by < btnY + btnH && by < pixelHeight; ++by) {
                for (int bx = btnX; bx < btnX + btnW && bx < pixelWidth; ++bx) {
                    int idx = (by * pixelWidth + bx) * 4;
                    bitmap[idx + 0] = static_cast<uint8_t>(0.25f * 255);
                    bitmap[idx + 1] = static_cast<uint8_t>(0.55f * 255);
                    bitmap[idx + 2] = static_cast<uint8_t>(0.95f * 255);
                    bitmap[idx + 3] = static_cast<uint8_t>(0.9f * 255);
                }
            }
            glm::vec2 textSize = m_fontRenderer->getTextSize(buttonText);
            float textX = btnX + (btnW - textSize.x * (30.0f / 48.0f)) * 0.5f;
            float textY = btnY + (btnH - textSize.y * (30.0f / 48.0f)) * 0.5f + 40.0f;
            m_fontRenderer->renderTextToBitmap(buttonText, textX, textY,
                glm::vec4(1.0f, 1.0f, 1.0f, 0.95f), 30.0f / 48.0f,
                bitmap.data(), pixelWidth, pixelHeight);
        }
    }

    uint32_t textureId = createTexture(bitmap.data(), pixelWidth, pixelHeight);
    if (textureId == 0) {
        std::cerr << "[ERROR] VulkanRenderer::createUIWindow: failed to create texture" << std::endl;
        return 0;
    }

    setUI3DTexture(textureId, pixelWidth, pixelHeight, position, size, false);
    return textureId;
#else
    (void)title; (void)subtitle; (void)position; (void)size; (void)hasButton; (void)buttonText;
    return 0;
#endif
}
#endif // USE_VULKAN

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

void OpenGLRenderer::setVideoOpacity(float opacity)
{
#ifdef USE_OPENGL
    m_videoOpacity = opacity;
#else
    (void)opacity;
#endif
}

void OpenGLRenderer::set3DObjectsOpacity(float opacity)
{
#ifdef USE_OPENGL
    m_3dObjectsOpacity = opacity;
#else
    (void)opacity;
#endif
}

uint32_t OpenGLRenderer::createMesh(const std::vector<float>& vertices, 
                                    const std::vector<uint32_t>& indices)
{
#ifdef USE_OPENGL
    if (vertices.empty() || indices.empty()) {
        return 0;
    }

    Mesh mesh;
    mesh.indexCount = indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Позиция (location 0) - формат: pos(3) + normal(3) + uv(2) = 8 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Нормаль (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    uint32_t meshId = m_nextMeshId++;
    m_meshes.push_back(mesh);
    return meshId;
#else
    (void)vertices;
    (void)indices;
    return 0;
#endif
}

void OpenGLRenderer::destroyMesh(uint32_t meshId)
{
#ifdef USE_OPENGL
    if (meshId == 0 || meshId > m_meshes.size()) {
        return;
    }

    Mesh& mesh = m_meshes[meshId - 1];
    if (mesh.vao != 0) {
        glDeleteVertexArrays(1, &mesh.vao);
    }
    if (mesh.vbo != 0) {
        glDeleteBuffers(1, &mesh.vbo);
    }
    if (mesh.ebo != 0) {
        glDeleteBuffers(1, &mesh.ebo);
    }
    mesh = Mesh{};
#else
    (void)meshId;
#endif
}

uint32_t OpenGLRenderer::createTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
#ifdef USE_OPENGL
    if (!data || width == 0 || height == 0) {
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
#else
    (void)data;
    (void)width;
    (void)height;
    return 0;
#endif
}

void OpenGLRenderer::destroyTexture(uint32_t textureId)
{
#ifdef USE_OPENGL
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
    }
#else
    (void)textureId;
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

    // Используем FontRenderer для нормального отображения текста вместо глифов
    if (m_fontRenderer) {
        // Настраиваем OpenGL для рендеринга текста
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, window.pixelWidth, window.pixelHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        // Рендерим заголовок
        m_fontRenderer->renderText(window.title, 40.0f, 40.0f, 
                                   glm::vec4(1.0f, 1.0f, 1.0f, 0.95f), 42.0f / 48.0f);
        
        // Рендерим подзаголовок
        m_fontRenderer->renderText(window.subtitle, 40.0f, 100.0f, 
                                   glm::vec4(0.85f, 0.88f, 0.95f, 0.85f), 26.0f / 48.0f);
        
        if (window.hasButton) {
            float buttonWidth = static_cast<float>(window.pixelWidth) - 80.0f;
            float buttonHeight = 70.0f;
            float buttonX = 40.0f;
            float buttonY = static_cast<float>(window.pixelHeight) - buttonHeight - 40.0f;

            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, buttonX, buttonY, buttonWidth, buttonHeight, 18.0f);
            nvgFillColor(nvg, nvgRGBAf(0.35f, 0.55f, 0.95f, 0.65f));
            nvgFill(nvg);

            // Рендерим текст кнопки по центру
            glm::vec2 textSize = m_fontRenderer->getTextSize(window.buttonText);
            float textX = buttonX + (buttonWidth - textSize.x * (30.0f / 48.0f)) * 0.5f;
            float textY = buttonY + (buttonHeight - textSize.y * (30.0f / 48.0f)) * 0.5f;
            m_fontRenderer->renderText(window.buttonText, textX, textY, 
                                       glm::vec4(1.0f, 1.0f, 1.0f, 0.95f), 30.0f / 48.0f);
        }
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
    } else {
        // Fallback на старый способ если FontRenderer недоступен
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

    // QImage хранит данные сверху вниз (0,0 вверху), OpenGL текстуры снизу вверх (0,0 внизу)
    // Переворачиваем координаты текстуры по Y для правильного отображения
    const float vertices[] = {
        // positions   // texCoords (перевернуты по Y)
        -1.0f,  1.0f, 0.0f, 0.0f,  // Верхний левый: texCoord Y = 0 (верх текстуры)
        -1.0f, -1.0f, 0.0f, 1.0f,  // Нижний левый: texCoord Y = 1 (низ текстуры)
         1.0f, -1.0f, 1.0f, 1.0f,  // Нижний правый: texCoord Y = 1

        -1.0f,  1.0f, 0.0f, 0.0f,  // Верхний левый
         1.0f, -1.0f, 1.0f, 1.0f,  // Нижний правый
         1.0f,  1.0f, 1.0f, 0.0f,  // Верхний правый: texCoord Y = 0
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
namespace {
std::vector<char> ReadBinaryFileWithFallback(const char* filepath)
{
    // Сначала папка сборки (shaders_vk/ рядом с бинарником): иначе может подхватиться устаревший .spv
    std::vector<std::string> fallbackPaths = {
        std::string("shaders_vk/") + filepath,
        filepath,
        std::string("../") + filepath,
        std::string("../../") + filepath,
        std::string("../../../") + filepath,
        std::string("ARLauncher/shaders_vk/") + filepath,
        std::string("build/shaders_vk/") + filepath,
        std::string("ARLauncher/build/shaders_vk/") + filepath,
        std::string("build_vk/shaders_vk/") + filepath,
        std::string("../build_vk/shaders_vk/") + filepath,
        std::string("../../ARLauncher/build_vk/shaders_vk/") + filepath,
        std::string("/home/savva/Documents/ARLauncherProject/ARLauncher/build_vk/shaders_vk/") + filepath,
    };

    for (const auto& path : fallbackPaths) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            continue;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();
        return buffer;
    }

    std::cerr << "[ERROR] Unable to open SPIR-V file: " << filepath << std::endl;
    return {};
}
} // namespace

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
    , m_renderPassWithLoad(VK_NULL_HANDLE)
    , m_depthImage(VK_NULL_HANDLE)
    , m_depthImageMemory(VK_NULL_HANDLE)
    , m_depthImageView(VK_NULL_HANDLE)
    , m_depthFormat(VK_FORMAT_UNDEFINED)
    , m_pipelineLayout(VK_NULL_HANDLE)
    , m_graphicsPipeline(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
    , m_currentImageIndex(0)
    , m_initialized(false)
    , m_nextMeshId(1)
    , m_videoTextureId(0)
    , m_videoTextureInitialized(false)
    , m_videoOpacity(1.0f)
    , m_3dObjectsOpacity(1.0f)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_nextTextureId(1)
    , m_uiDescriptorSetLayout(VK_NULL_HANDLE)
    , m_uiDescriptorPool(VK_NULL_HANDLE)
    , m_uiDescriptorSet(VK_NULL_HANDLE)
    , m_uiPipelineLayout(VK_NULL_HANDLE)
    , m_uiPipeline(VK_NULL_HANDLE)
    , m_uiPipelineOverlay(VK_NULL_HANDLE)
    , m_uiVertexBuffer(VK_NULL_HANDLE)
    , m_uiVertexBufferMemory(VK_NULL_HANDLE)
    , m_uiIndexBuffer(VK_NULL_HANDLE)
    , m_uiIndexBufferMemory(VK_NULL_HANDLE)
    , m_uiIndexCount(0)
    , m_fontRenderer(nullptr)
{
    m_swapchainExtent = {0, 0};

    m_ui3D = {};
    m_ui3D.enabled = false;
    m_uiOverlay = {};
    m_uiOverlay.enabled = false;

    m_fontRenderer = new FontRenderer();
    if (m_fontRenderer) {
        m_fontRenderer->initialize();
#ifdef _WIN32
        m_fontRenderer->loadFont("C:/Windows/Fonts/arial.ttf", 48);
#else
        m_fontRenderer->loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
#endif
    }
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

    if (!createDepthResources()) {
        std::cerr << "Failed to create depth resources" << std::endl;
        return false;
    }
    
    if (!createFramebuffers()) {
        std::cerr << "Failed to create framebuffers" << std::endl;
        return false;
    }

    if (!createUIResources()) {
        std::cerr << "Failed to create Vulkan UI resources" << std::endl;
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

    if (!createPassthroughPipeline()) {
        std::cerr << "Failed to create passthrough pipeline (shaders_vk/passthrough.*.spv?)" << std::endl;
        return false;
    }

    // 3D-сцена использует набор дескрипторов passthrough (видео + LiDAR) — создаём после него
    if (!createSceneResources() || !createGraphicsPipeline()) {
        std::cerr << "Failed to create 3D scene pipeline (shaders_vk/mesh.*.spv?)" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "[OK] VulkanRenderer initialized successfully" << std::endl;
    std::cout << "   Swapchain: " << m_swapchainExtent.width << "x" << m_swapchainExtent.height
              << ", present mode " << presentModeName() << ", frames in flight " << kFramesInFlight << std::endl;
    
    return true;
}

void VulkanRenderer::shutdown()
{
    if (!m_initialized) return;
    
    vkDeviceWaitIdle(m_device);
    
    // Очистка видео текстуры
    if (m_videoTextureId != 0) {
        destroyTexture(m_videoTextureId);
        m_videoTextureId = 0;
    }

    destroyUIResources();
    destroySceneResources();
    destroyPassthrough();

    for (auto& tex : m_textures) {
        if (tex.sampler != VK_NULL_HANDLE) vkDestroySampler(m_device, tex.sampler, nullptr);
        if (tex.view != VK_NULL_HANDLE) vkDestroyImageView(m_device, tex.view, nullptr);
        if (tex.image != VK_NULL_HANDLE) vkDestroyImage(m_device, tex.image, nullptr);
        if (tex.memory != VK_NULL_HANDLE) vkFreeMemory(m_device, tex.memory, nullptr);
        tex = {};
    }
    m_textures.clear();
    
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    if (m_shadowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_shadowPipeline, nullptr);
        m_shadowPipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        vkFreeMemory(m_device, m_depthImageMemory, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        m_depthImageMemory = VK_NULL_HANDLE;
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
    
    for (uint32_t i = 0; i < kFramesInFlight; i++) {
        if (m_frameFences[i] != VK_NULL_HANDLE) vkDestroyFence(m_device, m_frameFences[i], nullptr);
        if (m_frameImageAvailable[i] != VK_NULL_HANDLE) vkDestroySemaphore(m_device, m_frameImageAvailable[i], nullptr);
        m_frameFences[i] = VK_NULL_HANDLE;
        m_frameImageAvailable[i] = VK_NULL_HANDLE;
    }
    destroyRenderFinishedSemaphores();
    
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

    if (m_fontRenderer) {
        m_fontRenderer->cleanup();
        delete m_fontRenderer;
        m_fontRenderer = nullptr;
    }
    
    m_initialized = false;
}

void VulkanRenderer::beginFrame()
{
    m_frameActive = false;
    if (!m_initialized) return;

    // Окно свёрнуто — не рендерим
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    if (fbw == 0 || fbh == 0) return;

    // Ждём, пока GPU освободит этот слот (не дольше kFramesInFlight кадров вперёд)
    vkWaitForFences(m_device, 1, &m_frameFences[m_frameSlot], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                            m_frameImageAvailable[m_frameSlot],
                                            VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire swapchain image" << std::endl;
        return;
    }

    // Fence сбрасываем только когда точно будем сабмитить (иначе следующий wait зависнет)
    vkResetFences(m_device, 1, &m_frameFences[m_frameSlot]);
    m_currentImageIndex = imageIndex;

    VkCommandBuffer cmd = currentCmd();
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin recording command buffer" << std::endl;
        return;
    }
    m_frameActive = true;

    ++m_frameNumber;
    // Late latching: поза и видеокадр берутся максимально близко к сабмиту
    if (m_preRenderPass) m_preRenderPass();
    ensureSceneInputs();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::endFrame()
{
    if (!m_initialized || !m_frameActive) return;
    m_frameActive = false;

    VkCommandBuffer cmd = currentCmd();
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        std::cerr << "Failed to record command buffer" << std::endl;
        return;
    }

    VkSemaphore waitSemaphores[] = {m_frameImageAvailable[m_frameSlot]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {m_imageRenderFinished[m_currentImageIndex]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_frameFences[m_frameSlot]) != VK_SUCCESS) {
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
    presentInfo.pImageIndices = &m_currentImageIndex;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    m_frameSlot = (m_frameSlot + 1) % kFramesInFlight;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present swapchain image" << std::endl;
    }
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
    
}

void VulkanRenderer::renderStoredVideoBackground()
{
    // Legacy RGB-путь (старый SensorConnector). Новый путь — renderPassthrough().
    if (!m_frameActive || m_ptHasFrame) return;
    if (!m_videoTextureInitialized || m_videoTextureId == 0) return;

    // Aspect-ratio-preserving fullscreen quad (letterbox / pillarbox)
    float videoAspect = (m_videoWidth > 0 && m_videoHeight > 0)
        ? static_cast<float>(m_videoWidth) / static_cast<float>(m_videoHeight)
        : 1.0f;
    float windowAspect = static_cast<float>(m_swapchainExtent.width) / static_cast<float>(m_swapchainExtent.height);

    float dstW = static_cast<float>(m_swapchainExtent.width);
    float dstH = static_cast<float>(m_swapchainExtent.height);
    float dstX = 0.0f;
    float dstY = 0.0f;

    if (videoAspect > windowAspect) {
        dstH = dstW / videoAspect;
        dstY = (m_swapchainExtent.height - dstH) / 2.0f;
    } else {
        dstW = dstH * videoAspect;
        dstX = (m_swapchainExtent.width - dstW) / 2.0f;
    }

    UITexturePlacement placement{};
    placement.textureId = m_videoTextureId;
    placement.width = m_videoWidth;
    placement.height = m_videoHeight;
    placement.position = glm::vec3(dstX, dstY, 0.0f);
    placement.size = glm::vec2(dstW, dstH);
    placement.billboard = false;
    placement.enabled = true;
    // Камера в программе перевернута на 180°
    placement.rotation = 180.0f;

    // Use overlay pipeline (no depth test) so 3D objects rendered later draw on top
    renderUITexturedQuad(placement, true);
}

void VulkanRenderer::render3DObjects(const std::vector<glm::mat4>& transforms,
                                     const std::vector<uint32_t>& meshIds)
{
    if (!m_initialized || transforms.empty() || meshIds.empty()) return;
    if (transforms.size() != meshIds.size()) return;
    if (!m_frameActive) return;
    if (m_graphicsPipeline == VK_NULL_HANDLE || m_pipelineLayout == VK_NULL_HANDLE) return;
    if (!m_ptLayoutReady || !m_ptDepthLayoutReady || !m_envLayoutReady) return;   // дескрипторы ещё не готовы
    VkCommandBuffer cmd = currentCmd();

    writeSceneUbo();
    applyEyeViewport(cmd);
    const std::array<VkDescriptorSet, 2> sets = {m_ptSet, m_sceneSets[m_frameSlot]};

    struct Push {
        glm::mat4 modelView;
        glm::vec4 color;
        glm::vec4 material;   // metallic, roughness, режим (0 объект, 1 тень), запас окклюзии (м)
    };
    static_assert(sizeof(Push) == 96, "push constants");
    const ObjectMaterial defaultMaterial{};

    // Тени на пол — до объектов: темним видео под объектом (blend MIN — перекрытия не темнеют дважды)
    const SceneParams& sp = m_sceneParams;
    if (sp.shadows && sp.floorValid && m_shadowPipeline != VK_NULL_HANDLE) {
        glm::vec3 L = glm::normalize(sp.lightDirWorld);
        if (L.y < 0.25f) {
            L.y = 0.25f;
            L = glm::normalize(L);
        }
        const float h = sp.floorY + 0.003f;
        // Проекция на плоскость y = h вдоль L: P' = P - L * (P.y - h) / L.y
        glm::mat4 S(1.0f);
        S[1][0] = -L.x / L.y;
        S[1][1] = 0.0f;
        S[1][2] = -L.z / L.y;
        S[3][0] = L.x / L.y * h;
        S[3][1] = h;
        S[3][2] = L.z / L.y * h;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
                                static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
        for (size_t i = 0; i < transforms.size(); ++i) {
            const uint32_t meshId = meshIds[i];
            if (meshId == 0 || meshId > m_meshes.size()) continue;
            const float height = transforms[i][3].y - sp.floorY;
            if (height < 0.0f) continue;   // под полом
            const VulkanMesh& mesh = m_meshes[meshId - 1];
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, offsets);
            vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            Push pc;
            pc.modelView = m_viewMatrix * S * transforms[i];
            // Чем выше объект над полом, тем тень слабее (свет в комнате не точечный)
            pc.color = glm::vec4(0.0f, 0.0f, 0.0f, glm::clamp(1.0f - height / 3.0f, 0.35f, 1.0f) * sp.opacity * m_3dObjectsOpacity);
            pc.material = glm::vec4(0.0f, 1.0f, 1.0f, 0.15f);   // на самом полу LiDAR видит тот же пол — запас 15 см
            vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(Push), &pc);
            vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
        }
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0,
                            static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    for (size_t i = 0; i < transforms.size(); i++) {
        const uint32_t meshId = meshIds[i];
        if (meshId == 0 || meshId > m_meshes.size()) continue;
        const VulkanMesh& mesh = m_meshes[meshId - 1];
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        const ObjectMaterial& m = i < m_materials.size() ? m_materials[i] : defaultMaterial;
        Push pc;
        pc.modelView = m_viewMatrix * transforms[i];
        pc.color = glm::vec4(glm::vec3(m.color), m.color.a * sp.opacity * m_3dObjectsOpacity);
        pc.material = glm::vec4(m.metallic, m.roughness, 0.0f, 0.0f);
        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(Push), &pc);
        vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
    }
}

bool VulkanRenderer::createDepthResources()
{
    m_depthFormat = findDepthFormat();
    if (m_depthFormat == VK_FORMAT_UNDEFINED) {
        return false;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_swapchainExtent.width;
    imageInfo.extent.height = m_swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthImageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        vkFreeMemory(m_device, m_depthImageMemory, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        m_depthImageMemory = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanRenderer::findDepthFormat()
{
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkShaderModule VulkanRenderer::createShaderModule(const uint32_t* code, size_t sizeBytes)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = sizeBytes;
    createInfo.pCode = code;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool VulkanRenderer::createGraphicsPipeline()
{
    auto vertCode = ReadBinaryFileWithFallback("mesh.vert.spv");
    auto fragCode = ReadBinaryFileWithFallback("mesh.frag.spv");
    if (vertCode.empty() || fragCode.empty()) {
        return false;
    }

    VkShaderModule vertShaderModule = createShaderModule(reinterpret_cast<const uint32_t*>(vertCode.data()), vertCode.size());
    VkShaderModule fragShaderModule = createShaderModule(reinterpret_cast<const uint32_t*>(fragCode.data()), fragCode.size());
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        if (vertShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        if (fragShaderModule != VK_NULL_HANDLE) vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = 8 * sizeof(float);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 3 * sizeof(float);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 6 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Альфа: мягкий край окклюзии (реальный предмет перед виртуальным) и прозрачность сцены
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // modelView + цвет + материал = 96 байт (гарантированный минимум — 128)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) + 2 * sizeof(glm::vec4);

    const std::array<VkDescriptorSetLayout, 2> setLayouts = {m_ptSetLayout, m_sceneSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    bool ok = (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) == VK_SUCCESS);

    // Тени: тот же шейдер в режиме «тень»; глубину не пишут, перекрытия — MIN (не темнеют дважды)
    VkPipelineDepthStencilStateCreateInfo shadowDepth = depthStencil;
    shadowDepth.depthWriteEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState shadowBlend = colorBlendAttachment;
    shadowBlend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    shadowBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    shadowBlend.colorBlendOp = VK_BLEND_OP_MIN;
    shadowBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    shadowBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    shadowBlend.alphaBlendOp = VK_BLEND_OP_MAX;
    VkPipelineColorBlendStateCreateInfo shadowBlending = colorBlending;
    shadowBlending.pAttachments = &shadowBlend;
    VkGraphicsPipelineCreateInfo shadowInfo = pipelineInfo;
    shadowInfo.pDepthStencilState = &shadowDepth;
    shadowInfo.pColorBlendState = &shadowBlending;
    ok = ok && vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &shadowInfo, nullptr, &m_shadowPipeline) == VK_SUCCESS;

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return ok;
}

void VulkanRenderer::setCameraMatrix(const glm::mat4& view, const glm::mat4& projection)
{
    m_viewMatrix = view;
    m_projectionMatrix = projection;
}

void VulkanRenderer::setDeviceOrientation(float degrees)
{
    m_deviceOrientation = degrees;
}

void VulkanRenderer::setVideoOpacity(float opacity)
{
    m_videoOpacity = opacity;
}

void VulkanRenderer::set3DObjectsOpacity(float opacity)
{
    m_3dObjectsOpacity = opacity;
}

void VulkanRenderer::setUIOverlayTexture(uint32_t textureId, uint32_t width, uint32_t height)
{
    m_uiOverlay.textureId = textureId;
    m_uiOverlay.width = width;
    m_uiOverlay.height = height;
    m_uiOverlay.position = glm::vec3(0.0f);
    m_uiOverlay.size = glm::vec2(static_cast<float>(width), static_cast<float>(height));
    m_uiOverlay.billboard = false;
    m_uiOverlay.enabled = (textureId != 0);
}

void VulkanRenderer::setUI3DTexture(uint32_t textureId, uint32_t width, uint32_t height,
                                   const glm::vec3& position, const glm::vec2& size, bool billboard)
{
    m_ui3D.textureId = textureId;
    m_ui3D.width = width;
    m_ui3D.height = height;
    m_ui3D.position = position;
    m_ui3D.size = size;
    m_ui3D.billboard = billboard;
    m_ui3D.enabled = (textureId != 0);
}

void VulkanRenderer::renderUIWindows()
{
    if (!m_initialized) return;
    if (m_uiPipelineLayout == VK_NULL_HANDLE || m_uiPipeline == VK_NULL_HANDLE) return;
    if ((!m_ui3D.enabled) && (!m_uiOverlay.enabled)) return;

    // Called while render pass is active (beginFrame() starts it; endFrame() ends it)
    if (!m_frameActive) return;

    if (m_uiOverlay.enabled) {
        renderUITexturedQuad(m_uiOverlay, true);
    }
    if (m_ui3D.enabled) {
        renderUITexturedQuad(m_ui3D, false);
    }
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
    if (!m_initialized || !data || width == 0 || height == 0) return 0;

    VulkanTexture tex{};
    tex.width = width;
    tex.height = height;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &tex.image) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, tex.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &tex.memory) != VK_SUCCESS) {
        vkDestroyImage(m_device, tex.image, nullptr);
        return 0;
    }
    vkBindImageMemory(m_device, tex.image, tex.memory, 0);

    transitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_GENERAL);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = tex.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &tex.view) != VK_SUCCESS) {
        vkDestroyImage(m_device, tex.image, nullptr);
        vkFreeMemory(m_device, tex.memory, nullptr);
        return 0;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &tex.sampler) != VK_SUCCESS) {
        vkDestroyImageView(m_device, tex.view, nullptr);
        vkDestroyImage(m_device, tex.image, nullptr);
        vkFreeMemory(m_device, tex.memory, nullptr);
        return 0;
    }

    m_textures.push_back(tex);
    uint32_t id = m_nextTextureId++;
    updateTexture(id, data, width, height);
    return id;
}

void VulkanRenderer::destroyTexture(uint32_t textureId)
{
    if (textureId == 0) return;
    m_uiTextureSets.erase(textureId);
    uint32_t index = textureId - 1;
    if (index >= m_textures.size()) return;

    VulkanTexture& tex = m_textures[index];
    if (tex.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, tex.sampler, nullptr);
        tex.sampler = VK_NULL_HANDLE;
    }
    if (tex.view != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, tex.view, nullptr);
        tex.view = VK_NULL_HANDLE;
    }
    if (tex.image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, tex.image, nullptr);
        tex.image = VK_NULL_HANDLE;
    }
    if (tex.memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, tex.memory, nullptr);
        tex.memory = VK_NULL_HANDLE;
    }
    tex.width = 0;
    tex.height = 0;
}

void VulkanRenderer::updateTexture(uint32_t textureId, const uint8_t* data, uint32_t width, uint32_t height)
{
    if (textureId == 0 || !data || width == 0 || height == 0) return;
    uint32_t index = textureId - 1;
    if (index >= m_textures.size()) return;

    VulkanTexture& tex = m_textures[index];
    if (tex.image == VK_NULL_HANDLE || tex.memory == VK_NULL_HANDLE) return;
    if (tex.width != width || tex.height != height) {
        // For now, require fixed size per texture id
        return;
    }

    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResource.mipLevel = 0;
    subResource.arrayLayer = 0;

    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_device, tex.image, &subResource, &subResourceLayout);

    void* mapped = nullptr;
    vkMapMemory(m_device, tex.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    uint8_t* dst = static_cast<uint8_t*>(mapped) + subResourceLayout.offset;

    const uint32_t srcRowPitch = width * 4;
    for (uint32_t y = 0; y < height; y++) {
        memcpy(dst + y * subResourceLayout.rowPitch, data + y * srcRowPitch, srcRowPitch);
    }

    vkUnmapMemory(m_device, tex.memory);
}

bool VulkanRenderer::createUIResources()
{
    // Descriptor layout
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_uiDescriptorSetLayout) != VK_SUCCESS) {
        return false;
    }

    // Descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 257;

    // Свой descriptor set на каждую UI-текстуру: обновлять один общий set, пока GPU
    // ещё рисует предыдущий кадр, нельзя (кадры в полёте)
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 257;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_uiDescriptorPool) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_uiDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_uiDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_uiDescriptorSet) != VK_SUCCESS) {
        return false;
    }

    // Quad geometry (unit quad, centered)
    struct UIVertex { glm::vec3 pos; glm::vec2 uv; };
    const std::array<UIVertex, 4> vertices = {
        UIVertex{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
        UIVertex{{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}},
        UIVertex{{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f}},
        UIVertex{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f}},
    };
    const std::array<uint32_t, 6> indices = {0, 1, 2, 2, 3, 0};
    m_uiIndexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vbSize = sizeof(vertices);
    if (!createBuffer(vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_uiVertexBuffer, m_uiVertexBufferMemory)) {
        return false;
    }
    void* mapped = nullptr;
    vkMapMemory(m_device, m_uiVertexBufferMemory, 0, vbSize, 0, &mapped);
    memcpy(mapped, vertices.data(), static_cast<size_t>(vbSize));
    vkUnmapMemory(m_device, m_uiVertexBufferMemory);

    VkDeviceSize ibSize = sizeof(indices);
    if (!createBuffer(ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_uiIndexBuffer, m_uiIndexBufferMemory)) {
        return false;
    }
    vkMapMemory(m_device, m_uiIndexBufferMemory, 0, ibSize, 0, &mapped);
    memcpy(mapped, indices.data(), static_cast<size_t>(ibSize));
    vkUnmapMemory(m_device, m_uiIndexBufferMemory);

    return createUIPipeline();
}

void VulkanRenderer::destroyUIResources()
{
    if (m_uiPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipeline, nullptr);
        m_uiPipeline = VK_NULL_HANDLE;
    }
    if (m_uiPipelineOverlay != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipelineOverlay, nullptr);
        m_uiPipelineOverlay = VK_NULL_HANDLE;
    }
    if (m_uiPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_uiPipelineLayout, nullptr);
        m_uiPipelineLayout = VK_NULL_HANDLE;
    }
    m_uiTextureSets.clear();
    if (m_uiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_uiDescriptorPool, nullptr);
        m_uiDescriptorPool = VK_NULL_HANDLE;
    }
    if (m_uiDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_uiDescriptorSetLayout, nullptr);
        m_uiDescriptorSetLayout = VK_NULL_HANDLE;
    }
    if (m_uiVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_uiVertexBuffer, nullptr);
        vkFreeMemory(m_device, m_uiVertexBufferMemory, nullptr);
        m_uiVertexBuffer = VK_NULL_HANDLE;
        m_uiVertexBufferMemory = VK_NULL_HANDLE;
    }
    if (m_uiIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_uiIndexBuffer, nullptr);
        vkFreeMemory(m_device, m_uiIndexBufferMemory, nullptr);
        m_uiIndexBuffer = VK_NULL_HANDLE;
        m_uiIndexBufferMemory = VK_NULL_HANDLE;
    }
    m_uiIndexCount = 0;
}

static bool ReadBinaryFileWithFallbackUI(const std::string& primaryPath, const std::string& fallbackPath, std::vector<uint32_t>& outWords)
{
    auto read = [&](const std::string& p) -> bool {
        std::ifstream file(p, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return false;
        size_t sizeBytes = static_cast<size_t>(file.tellg());
        if (sizeBytes == 0 || (sizeBytes % 4) != 0) return false;
        outWords.resize(sizeBytes / 4);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outWords.data()), sizeBytes);
        return true;
    };
    if (read(primaryPath)) return true;
    return read(fallbackPath);
}

bool VulkanRenderer::createUIPipeline()
{
    // Load shaders from build/shaders_vk/*.spv (with fallback to local path)
    std::vector<uint32_t> vertCode;
    std::vector<uint32_t> fragCode;

    bool okV = ReadBinaryFileWithFallbackUI("shaders_vk/ui.vert.spv", "/home/savva/Documents/ARLauncherProject/ARLauncher/build_vk/shaders_vk/ui.vert.spv", vertCode);
    bool okF = ReadBinaryFileWithFallbackUI("shaders_vk/ui.frag.spv", "/home/savva/Documents/ARLauncherProject/ARLauncher/build_vk/shaders_vk/ui.frag.spv", fragCode);
    if (!okV || !okF) {
        std::cerr << "[VulkanUI] Failed to load ui shaders (.spv)" << std::endl;
        return false;
    }

    VkShaderModule vertModule = createShaderModule(vertCode.data(), vertCode.size() * sizeof(uint32_t));
    VkShaderModule fragModule = createShaderModule(fragCode.data(), fragCode.size() * sizeof(uint32_t));
    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // Vertex input
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(glm::vec3) + sizeof(glm::vec2);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attrs{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = sizeof(glm::vec3);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.logicOpEnable = VK_FALSE;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    std::array<VkDynamicState, 2> dynStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dyn.pDynamicStates = dynStates.data();

    // Push constants
    struct Push { glm::mat4 mvp; float opacity; glm::vec3 pad; };
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(Push);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_uiDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pcRange;

    if (m_uiPipelineLayout == VK_NULL_HANDLE) {
        if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_uiPipelineLayout) != VK_SUCCESS) {
            vkDestroyShaderModule(m_device, vertModule, nullptr);
            vkDestroyShaderModule(m_device, fragModule, nullptr);
            return false;
        }
    }

    auto makePipeline = [&](bool overlayMode, VkPipeline* outPipe) -> bool {
        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable = overlayMode ? VK_FALSE : VK_TRUE;
        depth.depthWriteEnable = VK_FALSE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth.depthBoundsTestEnable = VK_FALSE;
        depth.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipe{};
        pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipe.stageCount = 2;
        pipe.pStages = stages;
        pipe.pVertexInputState = &vertexInput;
        pipe.pInputAssemblyState = &inputAssembly;
        pipe.pViewportState = &viewportState;
        pipe.pRasterizationState = &raster;
        pipe.pMultisampleState = &ms;
        pipe.pDepthStencilState = &depth;
        pipe.pColorBlendState = &blend;
        pipe.pDynamicState = &dyn;
        pipe.layout = m_uiPipelineLayout;
        pipe.renderPass = m_renderPass;
        pipe.subpass = 0;

        return vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipe, nullptr, outPipe) == VK_SUCCESS;
    };

    bool ok = true;
    if (m_uiPipeline == VK_NULL_HANDLE) {
        ok = ok && makePipeline(false, &m_uiPipeline);
    }
    if (m_uiPipelineOverlay == VK_NULL_HANDLE) {
        ok = ok && makePipeline(true, &m_uiPipelineOverlay);
    }

    vkDestroyShaderModule(m_device, vertModule, nullptr);
    vkDestroyShaderModule(m_device, fragModule, nullptr);
    return ok;
}

VkDescriptorSet VulkanRenderer::updateUIDescriptorSet(uint32_t textureId)
{
    if (textureId == 0) return VK_NULL_HANDLE;
    auto it = m_uiTextureSets.find(textureId);
    if (it != m_uiTextureSets.end()) return it->second;

    uint32_t index = textureId - 1;
    if (index >= m_textures.size()) return VK_NULL_HANDLE;
    const VulkanTexture& tex = m_textures[index];
    if (tex.view == VK_NULL_HANDLE || tex.sampler == VK_NULL_HANDLE) return VK_NULL_HANDLE;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_uiDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_uiDescriptorSetLayout;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &set) != VK_SUCCESS) return VK_NULL_HANDLE;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = tex.view;
    imageInfo.sampler = tex.sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

    m_uiTextureSets[textureId] = set;
    return set;
}

void VulkanRenderer::renderUITexturedQuad(const UITexturePlacement& placement, bool overlayMode)
{
    if (placement.textureId == 0 || !m_frameActive) return;

    VkCommandBuffer cmd = currentCmd();

    if (overlayMode) {
        // 2D-оверлей — всегда на весь экран
        VkViewport viewport{0.0f, 0.0f, float(m_swapchainExtent.width), float(m_swapchainExtent.height), 0.0f, 1.0f};
        VkRect2D scissor{{0, 0}, m_swapchainExtent};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    } else {
        applyEyeViewport(cmd);   // 3D-окно — в области текущего глаза
    }

    VkPipeline pipe = overlayMode ? m_uiPipelineOverlay : m_uiPipeline;
    if (pipe == VK_NULL_HANDLE) return;

    VkDescriptorSet set = updateUIDescriptorSet(placement.textureId);
    if (set == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipelineLayout, 0, 1, &set, 0, nullptr);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_uiVertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, m_uiIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    glm::mat4 mvp(1.0f);
    if (overlayMode) {
        // Pixel-space quad pinned to screen (top-left origin) using an ortho projection.
        float w = static_cast<float>(m_swapchainExtent.width);
        float h = static_cast<float>(m_swapchainExtent.height);
        glm::mat4 proj = glm::ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(placement.position.x + placement.size.x * 0.5f, placement.position.y + placement.size.y * 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(placement.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(placement.size.x, placement.size.y, 1.0f));
        mvp = proj * model;
    } else {
        glm::mat4 viewInverse = glm::inverse(m_viewMatrix);
        glm::mat4 rotation(1.0f);
        if (placement.billboard) {
            rotation = glm::mat4(glm::mat3(viewInverse));
        }
        glm::mat4 model = glm::translate(glm::mat4(1.0f), placement.position) * rotation;
        model = glm::scale(model, glm::vec3(placement.size.x, placement.size.y, 1.0f));
        mvp = m_projectionMatrix * m_viewMatrix * model;
    }

    struct Push { glm::mat4 mvp; float opacity; glm::vec3 pad; } pc;
    pc.mvp = mvp;
    pc.opacity = 1.0f;
    pc.pad = glm::vec3(0.0f);
    vkCmdPushConstants(cmd, m_uiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push), &pc);

    vkCmdDrawIndexed(cmd, m_uiIndexCount, 1, 0, 0, 0);
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
    // ARL_VK_VALIDATION=1 — включить слой валидации (отладка)
    static const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    if (const char* env = std::getenv("ARL_VK_VALIDATION")) {
        if (env[0] == '1') {
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = &validationLayer;
        }
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSurface()
{
    if (!glfwVulkanSupported()) {
        std::cerr << "GLFW reports Vulkan is not supported on this system" << std::endl;
        return false;
    }

    VkResult res = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    if (res != VK_SUCCESS) {
        std::cerr << "glfwCreateWindowSurface failed with VkResult=" << static_cast<int>(res) << std::endl;
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

    // Предпочитаем дискретную видеокарту (ARL_GPU=<index> — выбрать вручную)
    int forced = -1;
    if (const char* env = std::getenv("ARL_GPU")) forced = std::atoi(env);

    int bestScore = -1;
    for (uint32_t d = 0; d < deviceCount; ++d) {
        VkPhysicalDevice device = devices[d];
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int graphics = -1, present = -1;
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
            const bool g = (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
            // Одна очередь на графику и показ — лучше всего
            if (g && presentSupport) { graphics = present = int(i); break; }
            if (g && graphics < 0) graphics = int(i);
            if (presentSupport && present < 0) present = int(i);
        }
        if (graphics < 0 || present < 0) continue;

        int score = 1;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score = 100;
        else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score = 50;
        else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) score = 0;
        if (forced == int(d)) score = 1000;

        std::cout << "   GPU[" << d << "]: " << props.deviceName << " (score " << score << ")" << std::endl;
        if (score > bestScore) {
            bestScore = score;
            m_physicalDevice = device;
            m_graphicsQueueFamily = uint32_t(graphics);
            m_presentQueueFamily = uint32_t(present);
        }
    }
    if (m_physicalDevice != VK_NULL_HANDLE) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
        std::cout << "[OK] Vulkan GPU: " << props.deviceName << std::endl;
    }
    return m_physicalDevice != VK_NULL_HANDLE;
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
    
    // MAILBOX: показывается самый свежий кадр без ожидания очереди — минимальная задержка.
    // FIFO: строгий vsync. Переопределение: ARL_PRESENT_MODE=mailbox|fifo|immediate
    auto supported = [&](VkPresentModeKHR m) {
        return std::find(presentModes.begin(), presentModes.end(), m) != presentModes.end();
    };
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::string wanted = "mailbox";
    if (const char* env = std::getenv("ARL_PRESENT_MODE")) wanted = env;
    if (wanted == "mailbox" && supported(VK_PRESENT_MODE_MAILBOX_KHR)) presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (wanted == "immediate" && supported(VK_PRESENT_MODE_IMMEDIATE_KHR)) presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    m_presentMode = presentMode;
    
    if (capabilities.currentExtent.width != UINT32_MAX) {
        m_swapchainExtent = capabilities.currentExtent;
    } else {
        m_swapchainExtent.width = static_cast<uint32_t>(m_width);
        m_swapchainExtent.height = static_cast<uint32_t>(m_height);
    }
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) imageCount = std::max(imageCount, 3u);
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
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
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
    m_depthFormat = findDepthFormat();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // 🔹 CLEAR: видео будет нарисовано как textured quad внутри render pass
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 1> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        return false;
    }

    return true;
}

bool VulkanRenderer::createFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());
    
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {m_swapchainImageViews[i], m_depthImageView};
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 2;
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
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = kFramesInFlight;
    return vkAllocateCommandBuffers(m_device, &allocInfo, m_frameCmd.data()) == VK_SUCCESS;
}

bool VulkanRenderer::createRenderFinishedSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_imageRenderFinished.assign(m_swapchainImages.size(), VK_NULL_HANDLE);
    for (auto& sem : m_imageRenderFinished) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &sem) != VK_SUCCESS) return false;
    }
    return true;
}

void VulkanRenderer::destroyRenderFinishedSemaphores()
{
    for (auto sem : m_imageRenderFinished) {
        if (sem != VK_NULL_HANDLE) vkDestroySemaphore(m_device, sem, nullptr);
    }
    m_imageRenderFinished.clear();
}

bool VulkanRenderer::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < kFramesInFlight; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frameImageAvailable[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameFences[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return createRenderFinishedSemaphores();
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
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    if (fbw == 0 || fbh == 0) return;   // свёрнуто — пересоздадим, когда окно вернётся

    vkDeviceWaitIdle(m_device);
    
    cleanupSwapchain();
    destroyRenderFinishedSemaphores();

    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        vkFreeMemory(m_device, m_depthImageMemory, nullptr);
        m_depthImage = VK_NULL_HANDLE;
        m_depthImageMemory = VK_NULL_HANDLE;
    }
    
    glfwGetWindowSize(m_window, &m_width, &m_height);
    
    createSwapchain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
    createRenderFinishedSemaphores();

    destroyUIResources();

    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    if (m_shadowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_shadowPipeline, nullptr);
        m_shadowPipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    createGraphicsPipeline();

    createUIResources();
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
    std::cout << "[VIDEO] Creating video texture: " << width << "x" << height << std::endl;

    // createTexture expects RGBA data
    std::vector<uint8_t> emptyRgba(static_cast<size_t>(width) * height * 4, 0);
    m_videoTextureId = createTexture(emptyRgba.data(), width, height);
    if (m_videoTextureId == 0) {
        std::cerr << "[VIDEO] Failed to create video texture via createTexture" << std::endl;
        return false;
    }

    m_videoWidth = width;
    m_videoHeight = height;
    m_videoTextureInitialized = true;

    std::cout << "[VIDEO] Video texture created with ID: " << m_videoTextureId << std::endl;
    return true;
}

void VulkanRenderer::updateVideoTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (m_videoTextureId == 0 || !data) return;

    if (width != m_videoWidth || height != m_videoHeight) {
        // Recreate texture if dimensions changed
        destroyTexture(m_videoTextureId);
        m_videoTextureId = 0;
        m_videoTextureInitialized = false;
        if (!createVideoTexture(width, height)) return;
    }

    // Convert RGB → RGBA (createTexture/updateTexture expect RGBA)
    std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
    for (uint32_t i = 0; i < width * height; i++) {
        rgba[i * 4 + 0] = data[i * 3 + 0];
        rgba[i * 4 + 1] = data[i * 3 + 1];
        rgba[i * 4 + 2] = data[i * 3 + 2];
        rgba[i * 4 + 3] = 255;
    }

    updateTexture(m_videoTextureId, rgba.data(), width, height);
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
    } else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

// copyVideoToSwapchain removed - video is now rendered as a textured quad inside the render pass
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



#ifdef USE_VULKAN
// ============================================================================
// Passthrough-видео: NV12 (Y + CbCr) -> GPU без конвертации на CPU, репроекция в шейдере
// ============================================================================

const char* VulkanRenderer::presentModeName() const
{
    switch (m_presentMode) {
    case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX";
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
    default: return "FIFO";
    }
}

bool VulkanRenderer::createPlaneImage(PlaneImage& plane, VkFormat format, uint32_t width, uint32_t height)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(m_device, &imageInfo, nullptr, &plane.image) != VK_SUCCESS) return false;

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(m_device, plane.image, &req);
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(m_device, &alloc, nullptr, &plane.memory) != VK_SUCCESS) return false;
    vkBindImageMemory(m_device, plane.image, plane.memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = plane.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    return vkCreateImageView(m_device, &viewInfo, nullptr, &plane.view) == VK_SUCCESS;
}

void VulkanRenderer::destroyPlaneImage(PlaneImage& plane)
{
    if (plane.view != VK_NULL_HANDLE) vkDestroyImageView(m_device, plane.view, nullptr);
    if (plane.image != VK_NULL_HANDLE) vkDestroyImage(m_device, plane.image, nullptr);
    if (plane.memory != VK_NULL_HANDLE) vkFreeMemory(m_device, plane.memory, nullptr);
    plane = PlaneImage{};
}

bool VulkanRenderer::createPassthroughPipeline()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_ptSampler) != VK_SUCCESS) return false;

    // 0 = Y, 1 = CbCr, 2 = глубина LiDAR
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    for (uint32_t i = 0; i < 3; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_ptSetLayout) != VK_SUCCESS) return false;

    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_ptPool) != VK_SUCCESS) return false;

    VkDescriptorSetAllocateInfo setAlloc{};
    setAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAlloc.descriptorPool = m_ptPool;
    setAlloc.descriptorSetCount = 1;
    setAlloc.pSetLayouts = &m_ptSetLayout;
    if (vkAllocateDescriptorSets(m_device, &setAlloc, &m_ptSet) != VK_SUCCESS) return false;

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset = 0;
    push.size = sizeof(glm::mat4) + 3 * sizeof(glm::vec4);
    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &m_ptSetLayout;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &push;
    if (vkCreatePipelineLayout(m_device, &plInfo, nullptr, &m_ptPipelineLayout) != VK_SUCCESS) return false;

    auto vertCode = ReadBinaryFileWithFallback("passthrough.vert.spv");
    auto fragCode = ReadBinaryFileWithFallback("passthrough.frag.spv");
    if (vertCode.empty() || fragCode.empty()) return false;
    VkShaderModule vert = createShaderModule(reinterpret_cast<const uint32_t*>(vertCode.data()), vertCode.size());
    VkShaderModule frag = createShaderModule(reinterpret_cast<const uint32_t*>(fragCode.data()), fragCode.size());
    if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE) {
        if (vert != VK_NULL_HANDLE) vkDestroyShaderModule(m_device, vert, nullptr);
        if (frag != VK_NULL_HANDLE) vkDestroyShaderModule(m_device, frag, nullptr);
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;    // фон: 3D-объекты рисуются поверх
    depth.depthWriteEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;
    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<uint32_t>(stages.size());
    info.pStages = stages.data();
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &ms;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = m_ptPipelineLayout;
    info.renderPass = m_renderPass;
    info.subpass = 0;
    const bool ok = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &m_ptPipeline) == VK_SUCCESS;
    vkDestroyShaderModule(m_device, vert, nullptr);
    vkDestroyShaderModule(m_device, frag, nullptr);
    return ok;
}

bool VulkanRenderer::ensurePassthroughResources(uint32_t width, uint32_t height)
{
    if (m_ptY.image != VK_NULL_HANDLE && width == m_ptWidth && height == m_ptHeight) return true;

    // Размер кадра поменялся (редко): ждём GPU и пересоздаём
    vkDeviceWaitIdle(m_device);
    destroyPlaneImage(m_ptY);
    destroyPlaneImage(m_ptUV);
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (m_ptStagingPtr[i]) vkUnmapMemory(m_device, m_ptStagingMemory[i]);
        if (m_ptStaging[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_ptStaging[i], nullptr);
        if (m_ptStagingMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_ptStagingMemory[i], nullptr);
        m_ptStaging[i] = VK_NULL_HANDLE;
        m_ptStagingMemory[i] = VK_NULL_HANDLE;
        m_ptStagingPtr[i] = nullptr;
    }

    const uint32_t uvW = (width + 1) / 2;
    const uint32_t uvH = (height + 1) / 2;
    if (!createPlaneImage(m_ptY, VK_FORMAT_R8_UNORM, width, height) ||
        !createPlaneImage(m_ptUV, VK_FORMAT_R8G8_UNORM, uvW, uvH)) {
        std::cerr << "[VIDEO] failed to create NV12 plane images" << std::endl;
        return false;
    }

    m_ptStagingSize = VkDeviceSize(width) * height + VkDeviceSize(uvW) * uvH * 2;
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (!createBuffer(m_ptStagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_ptStaging[i], m_ptStagingMemory[i])) {
            return false;
        }
        vkMapMemory(m_device, m_ptStagingMemory[i], 0, m_ptStagingSize, 0, &m_ptStagingPtr[i]);
    }

    std::array<VkDescriptorImageInfo, 2> images{};
    images[0] = {m_ptSampler, m_ptY.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    images[1] = {m_ptSampler, m_ptUV.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::array<VkWriteDescriptorSet, 2> writes{};
    for (uint32_t i = 0; i < 2; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_ptSet;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(m_device, 2, writes.data(), 0, nullptr);

    m_ptWidth = width;
    m_ptHeight = height;
    m_ptLayoutReady = false;
    m_ptHasFrame = false;
    std::cout << "[VIDEO] passthrough NV12 textures " << width << "x" << height << std::endl;
    return true;
}

void VulkanRenderer::uploadPassthroughFrame(const PassthroughFrame& frame)
{
    if (!m_frameActive || !frame.y || !frame.uv || frame.width == 0 || frame.height == 0) return;
    if (frame.id != 0 && frame.id == m_ptLastId && m_ptHasFrame) return;
    if (!ensurePassthroughResources(frame.width, frame.height)) return;
    if (!m_ptDepthLayoutReady) {
        // Дескриптор глубины должен быть валиден, даже если LiDAR выключен: пустая карта 1x1
        const uint16_t zero = 0;
        uploadDepthHalf(&zero, 1, 1);
        m_ptHasDepth = false;
    }

    // Слот m_frameSlot свободен (fence дождались в beginFrame) — пишем в его staging
    const size_t ySize = size_t(frame.width) * frame.height;
    const uint32_t uvW = (frame.width + 1) / 2;
    const uint32_t uvH = (frame.height + 1) / 2;
    const size_t uvSize = size_t(uvW) * uvH * 2;
    uint8_t* dst = static_cast<uint8_t*>(m_ptStagingPtr[m_frameSlot]);
    std::memcpy(dst, frame.y, ySize);
    std::memcpy(dst + ySize, frame.uv, uvSize);

    VkCommandBuffer cmd = currentCmd();
    std::array<VkImageMemoryBarrier, 2> toTransfer{};
    for (int i = 0; i < 2; ++i) {
        toTransfer[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransfer[i].oldLayout = m_ptLayoutReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer[i].image = (i == 0) ? m_ptY.image : m_ptUV.image;
        toTransfer[i].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        toTransfer[i].srcAccessMask = 0;
        toTransfer[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    // Чтение предыдущими кадрами (фрагментный шейдер) должно закончиться до перезаписи
    vkCmdPipelineBarrier(cmd,
                         m_ptLayoutReady ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         static_cast<uint32_t>(toTransfer.size()), toTransfer.data());

    std::array<VkBufferImageCopy, 2> regions{};
    regions[0].bufferOffset = 0;
    regions[0].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    regions[0].imageExtent = {frame.width, frame.height, 1};
    regions[1].bufferOffset = ySize;
    regions[1].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    regions[1].imageExtent = {uvW, uvH, 1};
    vkCmdCopyBufferToImage(cmd, m_ptStaging[m_frameSlot], m_ptY.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions[0]);
    vkCmdCopyBufferToImage(cmd, m_ptStaging[m_frameSlot], m_ptUV.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions[1]);

    std::array<VkImageMemoryBarrier, 2> toShader = toTransfer;
    for (auto& b : toShader) {
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(toShader.size()), toShader.data());

    m_ptLayoutReady = true;
    m_ptHasFrame = true;
    m_ptLastId = frame.id;
}

void VulkanRenderer::renderPassthrough(const PassthroughParams& params)
{
    if (!m_frameActive || !m_ptHasFrame || m_ptPipeline == VK_NULL_HANDLE) return;
    VkCommandBuffer cmd = currentCmd();

    applyEyeViewport(cmd);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ptPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ptPipelineLayout, 0, 1, &m_ptSet, 0, nullptr);

    struct Push {
        glm::mat4 cvFromEye;
        glm::vec4 cam;
        glm::vec4 eye;
        glm::vec4 depth;
    } pc;
    pc.cvFromEye = glm::mat4(params.cvFromEye);
    pc.cam = params.camIntrinsicsNorm;
    const float flags = (params.fullRange ? 1.0f : 0.0f) + (params.bt709 ? 2.0f : 0.0f);
    pc.eye = glm::vec4(params.eyeTanHalfFov.x, params.eyeTanHalfFov.y, params.opacity, flags);
    pc.depth = glm::vec4(m_ptHasDepth ? params.depthOverlay : 0.0f, std::max(0.5f, params.depthMaxMeters), 0.0f, 0.0f);
    vkCmdPushConstants(cmd, m_ptPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push), &pc);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

// ---------------- LiDAR: карта глубины для наложения поверх видео ----------------

void VulkanRenderer::destroyDepthResources()
{
    destroyPlaneImage(m_ptDepth);
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (m_ptDepthStagingPtr[i]) vkUnmapMemory(m_device, m_ptDepthStagingMemory[i]);
        if (m_ptDepthStaging[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_ptDepthStaging[i], nullptr);
        if (m_ptDepthStagingMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_ptDepthStagingMemory[i], nullptr);
        m_ptDepthStaging[i] = VK_NULL_HANDLE;
        m_ptDepthStagingMemory[i] = VK_NULL_HANDLE;
        m_ptDepthStagingPtr[i] = nullptr;
    }
    m_ptDepthW = m_ptDepthH = 0;
    m_ptDepthLayoutReady = false;
    m_ptHasDepth = false;
}

bool VulkanRenderer::ensureDepthResources(uint32_t width, uint32_t height)
{
    if (m_ptDepth.image != VK_NULL_HANDLE && width == m_ptDepthW && height == m_ptDepthH) return true;

    // Размер карты поменялся (1x1 заглушка -> 256x192 и т.п.): ждём GPU и пересоздаём
    vkDeviceWaitIdle(m_device);
    destroyDepthResources();
    if (!createPlaneImage(m_ptDepth, VK_FORMAT_R16_SFLOAT, width, height)) {
        std::cerr << "[LIDAR] failed to create depth texture" << std::endl;
        return false;
    }
    const VkDeviceSize size = VkDeviceSize(width) * height * sizeof(uint16_t);
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_ptDepthStaging[i], m_ptDepthStagingMemory[i])) {
            return false;
        }
        vkMapMemory(m_device, m_ptDepthStagingMemory[i], 0, size, 0, &m_ptDepthStagingPtr[i]);
    }

    VkDescriptorImageInfo image{m_ptSampler, m_ptDepth.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_ptSet;
    write.dstBinding = 2;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

    m_ptDepthW = width;
    m_ptDepthH = height;
    if (width > 1) std::cout << "[LIDAR] depth texture " << width << "x" << height << std::endl;
    return true;
}

void VulkanRenderer::uploadDepthHalf(const uint16_t* half, uint32_t width, uint32_t height)
{
    if (!ensureDepthResources(width, height)) return;
    std::memcpy(m_ptDepthStagingPtr[m_frameSlot], half, size_t(width) * height * sizeof(uint16_t));

    VkCommandBuffer cmd = currentCmd();
    VkImageMemoryBarrier b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout = m_ptDepthLayoutReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = m_ptDepth.image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
                         m_ptDepthLayoutReady ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, m_ptDepthStaging[m_frameSlot], m_ptDepth.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &b);
    m_ptDepthLayoutReady = true;
}

void VulkanRenderer::uploadDepthOverlay(const DepthOverlayFrame& frame)
{
    if (!m_frameActive || m_ptSet == VK_NULL_HANDLE || !frame.depth || frame.width == 0 || frame.height == 0) return;
    if (frame.id != 0 && frame.id == m_ptDepthLastId && m_ptHasDepth) return;

    std::vector<uint16_t> half(size_t(frame.width) * frame.height);
    for (size_t i = 0; i < half.size(); ++i) {
        const float d = frame.depth[i];
        half[i] = glm::packHalf1x16(std::isfinite(d) ? d : 0.0f);
    }
    uploadDepthHalf(half.data(), frame.width, frame.height);
    m_ptHasDepth = true;
    m_ptDepthLastId = frame.id;
}

void VulkanRenderer::setEyeViewport(float x, float y, float width, float height)
{
    m_eyeViewportSet = width > 0.0f && height > 0.0f;
    m_eyeViewport = VkViewport{x, y, width, height, 0.0f, 1.0f};
}

void VulkanRenderer::applyEyeViewport(VkCommandBuffer cmd)
{
    VkViewport viewport{0.0f, 0.0f, float(m_swapchainExtent.width), float(m_swapchainExtent.height), 0.0f, 1.0f};
    if (m_eyeViewportSet) viewport = m_eyeViewport;
    VkRect2D scissor{};
    scissor.offset = {int32_t(viewport.x), int32_t(viewport.y)};
    scissor.extent = {uint32_t(viewport.width), uint32_t(viewport.height)};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void VulkanRenderer::destroyPassthrough()
{
    if (m_device == VK_NULL_HANDLE) return;
    destroyPlaneImage(m_ptY);
    destroyPlaneImage(m_ptUV);
    destroyDepthResources();
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (m_ptStagingPtr[i]) vkUnmapMemory(m_device, m_ptStagingMemory[i]);
        if (m_ptStaging[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_ptStaging[i], nullptr);
        if (m_ptStagingMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_ptStagingMemory[i], nullptr);
        m_ptStaging[i] = VK_NULL_HANDLE;
        m_ptStagingMemory[i] = VK_NULL_HANDLE;
        m_ptStagingPtr[i] = nullptr;
    }
    if (m_ptPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device, m_ptPipeline, nullptr);
    if (m_ptPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device, m_ptPipelineLayout, nullptr);
    if (m_ptPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(m_device, m_ptPool, nullptr);
    if (m_ptSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_device, m_ptSetLayout, nullptr);
    if (m_ptSampler != VK_NULL_HANDLE) vkDestroySampler(m_device, m_ptSampler, nullptr);
    m_ptPipeline = VK_NULL_HANDLE;
    m_ptPipelineLayout = VK_NULL_HANDLE;
    m_ptPool = VK_NULL_HANDLE;
    m_ptSetLayout = VK_NULL_HANDLE;
    m_ptSampler = VK_NULL_HANDLE;
    m_ptSet = VK_NULL_HANDLE;
    m_ptHasFrame = false;
    m_ptWidth = m_ptHeight = 0;
}

// ---------------- 3D-сцена: UBO кадра, карта окружения, заглушки ----------------

namespace {

struct SceneUbo {
    glm::mat4 proj;
    glm::mat4 cvFromEye;
    glm::mat4 worldFromEye;
    glm::vec4 cam;
    glm::vec4 lightDir;      // оси глаза; w = 1
    glm::vec4 lightColor;    // w = сила тени
    glm::vec4 sh[9];
    glm::vec4 params;        // x флаги, y флаги видео, z мягкость окклюзии, w непрозрачность
};

constexpr uint32_t kEnvW = 128;
constexpr uint32_t kEnvH = 64;

} // namespace

bool VulkanRenderer::createSceneResources()
{
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_sceneSetLayout) != VK_SUCCESS) return false;

    std::array<VkDescriptorPoolSize, 2> poolSizes = {
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kFramesInFlight},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kFramesInFlight}};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = kFramesInFlight;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_scenePool) != VK_SUCCESS) return false;

    std::array<VkDescriptorSetLayout, kFramesInFlight> layouts;
    layouts.fill(m_sceneSetLayout);
    VkDescriptorSetAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc.descriptorPool = m_scenePool;
    alloc.descriptorSetCount = kFramesInFlight;
    alloc.pSetLayouts = layouts.data();
    if (vkAllocateDescriptorSets(m_device, &alloc, m_sceneSets.data()) != VK_SUCCESS) return false;

    // Карта окружения: повтор по долготе, край по широте
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_envSampler) != VK_SUCCESS) return false;
    if (!createPlaneImage(m_env, VK_FORMAT_R8G8B8A8_UNORM, kEnvW, kEnvH)) return false;
    m_envW = kEnvW;
    m_envH = kEnvH;

    const VkDeviceSize envBytes = VkDeviceSize(kEnvW) * kEnvH * 4;
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (!createBuffer(sizeof(SceneUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_sceneUbo[i], m_sceneUboMemory[i]) ||
            !createBuffer(envBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_envStaging[i], m_envStagingMemory[i])) {
            return false;
        }
        vkMapMemory(m_device, m_sceneUboMemory[i], 0, sizeof(SceneUbo), 0, &m_sceneUboPtr[i]);
        vkMapMemory(m_device, m_envStagingMemory[i], 0, envBytes, 0, &m_envStagingPtr[i]);

        VkDescriptorBufferInfo bufferInfo{m_sceneUbo[i], 0, sizeof(SceneUbo)};
        VkDescriptorImageInfo imageInfo{m_envSampler, m_env.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_sceneSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &bufferInfo;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_sceneSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    return true;
}

void VulkanRenderer::destroySceneResources()
{
    if (m_device == VK_NULL_HANDLE) return;
    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        if (m_sceneUboPtr[i]) vkUnmapMemory(m_device, m_sceneUboMemory[i]);
        if (m_sceneUbo[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_sceneUbo[i], nullptr);
        if (m_sceneUboMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_sceneUboMemory[i], nullptr);
        if (m_envStagingPtr[i]) vkUnmapMemory(m_device, m_envStagingMemory[i]);
        if (m_envStaging[i] != VK_NULL_HANDLE) vkDestroyBuffer(m_device, m_envStaging[i], nullptr);
        if (m_envStagingMemory[i] != VK_NULL_HANDLE) vkFreeMemory(m_device, m_envStagingMemory[i], nullptr);
        m_sceneUbo[i] = VK_NULL_HANDLE;
        m_sceneUboMemory[i] = VK_NULL_HANDLE;
        m_sceneUboPtr[i] = nullptr;
        m_envStaging[i] = VK_NULL_HANDLE;
        m_envStagingMemory[i] = VK_NULL_HANDLE;
        m_envStagingPtr[i] = nullptr;
    }
    destroyPlaneImage(m_env);
    if (m_envSampler != VK_NULL_HANDLE) vkDestroySampler(m_device, m_envSampler, nullptr);
    if (m_scenePool != VK_NULL_HANDLE) vkDestroyDescriptorPool(m_device, m_scenePool, nullptr);
    if (m_sceneSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_device, m_sceneSetLayout, nullptr);
    m_envSampler = VK_NULL_HANDLE;
    m_scenePool = VK_NULL_HANDLE;
    m_sceneSetLayout = VK_NULL_HANDLE;
    m_sceneSets.fill(VK_NULL_HANDLE);
    m_envLayoutReady = false;
}

void VulkanRenderer::uploadEnvironmentMap(const uint8_t* rgba, uint32_t width, uint32_t height, uint64_t version)
{
    if (!m_frameActive || !rgba || width != m_envW || height != m_envH || m_env.image == VK_NULL_HANDLE) return;
    if (m_envReal && version == m_envVersion) return;
    std::memcpy(m_envStagingPtr[m_frameSlot], rgba, size_t(width) * height * 4);

    VkCommandBuffer cmd = currentCmd();
    VkImageMemoryBarrier b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout = m_envLayoutReady ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = m_env.image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, m_envLayoutReady ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, m_envStaging[m_frameSlot], m_env.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &b);
    m_envLayoutReady = true;
    m_envReal = version != 0;
    m_envVersion = version;
}

void VulkanRenderer::ensureSceneInputs()
{
    if (!m_frameActive) return;
    // До первого кадра с iPhone: серое видео 2x2 (passthrough его не рисует) и пустая карта LiDAR
    if (!m_ptLayoutReady) {
        const uint8_t y[4] = {128, 128, 128, 128};
        const uint8_t uv[2] = {128, 128};
        PassthroughFrame f;
        f.y = y;
        f.uv = uv;
        f.width = 2;
        f.height = 2;
        f.id = UINT64_MAX;
        uploadPassthroughFrame(f);
        m_ptHasFrame = false;
        m_ptLastId = 0;
    }
    // Нейтральная «комната» до первой оценки: светлый верх, тёмный низ
    if (!m_envLayoutReady && m_env.image != VK_NULL_HANDLE) {
        std::vector<uint8_t> neutral(size_t(kEnvW) * kEnvH * 4);
        for (uint32_t v = 0; v < kEnvH; ++v) {
            const float t = float(v) / float(kEnvH - 1);
            const uint8_t c = uint8_t(150.0f - 70.0f * t);
            for (uint32_t u = 0; u < kEnvW; ++u) {
                uint8_t* p = neutral.data() + (size_t(v) * kEnvW + u) * 4;
                p[0] = p[1] = p[2] = c;
                p[3] = 0;
            }
        }
        uploadEnvironmentMap(neutral.data(), kEnvW, kEnvH, 0);
    }
}

void VulkanRenderer::writeSceneUbo()
{
    if (m_sceneUboFrame == m_frameNumber || !m_sceneUboPtr[m_frameSlot]) return;
    m_sceneUboFrame = m_frameNumber;
    const SceneParams& sp = m_sceneParams;

    SceneUbo u{};
    u.proj = m_projectionMatrix;
    u.cvFromEye = glm::mat4(sp.cvFromEye);
    // Поворот вида одинаков для обоих глаз (стерео — только сдвиг)
    const glm::mat3 eyeFromWorld = glm::mat3(m_viewMatrix);
    u.worldFromEye = glm::mat4(glm::transpose(eyeFromWorld));
    u.cam = sp.camIntrinsicsNorm;
    u.lightDir = glm::vec4(glm::normalize(eyeFromWorld * glm::normalize(sp.lightDirWorld)), 1.0f);
    u.lightColor = glm::vec4(sp.lightColor, sp.shadowStrength);
    bool shValid = false;
    for (const auto& c : sp.sh) shValid = shValid || glm::length(c) > 1e-6f;
    for (int k = 0; k < 9; ++k) u.sh[k] = glm::vec4(sp.sh[size_t(k)], 0.0f);
    if (!shValid) {
        // Нейтральный свет: равномерный фон 0.45 + чуть ярче сверху
        u.sh[0] = glm::vec4(glm::vec3(0.45f / 0.282095f), 0.0f);
        u.sh[1] = glm::vec4(glm::vec3(0.25f), 0.0f);
    }
    int flags = 0;
    if (sp.occlusion) flags |= 1;
    if (m_ptHasDepth) flags |= 2;
    if (m_ptHasFrame) flags |= 4;
    if (sp.reflections) flags |= 8;
    if (m_envReal) flags |= 16;
    const int videoFlags = (sp.fullRange ? 1 : 0) | (sp.bt709 ? 2 : 0);
    u.params = glm::vec4(float(flags), float(videoFlags), std::max(0.005f, sp.occlusionSoftness), 1.0f);
    std::memcpy(m_sceneUboPtr[m_frameSlot], &u, sizeof(u));
}

#endif // USE_VULKAN
