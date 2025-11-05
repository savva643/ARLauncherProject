#include "Renderer.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
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
    , m_nextMeshId(1)
{
}

OpenGLRenderer::~OpenGLRenderer()
{
    shutdown();
}

bool OpenGLRenderer::initialize(GLFWwindow* window)
{
    m_window = window;
    
#ifdef USE_OPENGL
    // Инициализация OpenGL
    glfwGetWindowSize(window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
#else
    return false;
#endif
}

void OpenGLRenderer::shutdown()
{
#ifdef USE_OPENGL
    // Очистка ресурсов
    for (auto& mesh : m_meshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
    }
    m_meshes.clear();
    
    if (m_videoTexture) {
        glDeleteTextures(1, &m_videoTexture);
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
    
    // Обновляем или создаем текстуру для видео
    if (m_videoTexture == 0) {
        glGenTextures(1, &m_videoTexture);
    }
    
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Рендерим полноэкранный квад
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0); // Используем fixed-function pipeline для простоты
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    
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
    
    // Устанавливаем матрицы камеры
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_projectionMatrix));
    
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(m_viewMatrix));
    
    // Рендерим каждый объект
    for (size_t i = 0; i < transforms.size(); ++i) {
        uint32_t meshId = meshIds[i];
        if (meshId == 0 || meshId > m_meshes.size()) {
            continue;
        }
        
        const Mesh& mesh = m_meshes[meshId - 1];
        
        // Применяем трансформацию объекта
        glPushMatrix();
        glMultMatrixf(glm::value_ptr(transforms[i]));
        
        // Рендерим меш
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glPopMatrix();
    }
#endif
}

void OpenGLRenderer::setCameraMatrix(const glm::mat4& view, const glm::mat4& projection)
{
    m_viewMatrix = view;
    m_projectionMatrix = projection;
}

uint32_t OpenGLRenderer::createMesh(const std::vector<float>& vertices, 
                                    const std::vector<uint32_t>& indices)
{
#ifdef USE_OPENGL
    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    
    glBindVertexArray(mesh.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    
    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    mesh.indexCount = indices.size();
    m_meshes.push_back(mesh);
    
    return m_nextMeshId++;
#else
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
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
    
    // Помечаем как пустой
    mesh.vao = 0;
    mesh.vbo = 0;
    mesh.ebo = 0;
    mesh.indexCount = 0;
#endif
}

uint32_t OpenGLRenderer::createTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
#ifdef USE_OPENGL
    uint32_t texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return texture;
#else
    return 0;
#endif
}

void OpenGLRenderer::destroyTexture(uint32_t textureId)
{
#ifdef USE_OPENGL
    glDeleteTextures(1, &textureId);
#endif
}

std::unique_ptr<Renderer> createRenderer(bool useVulkan)
{
    if (useVulkan) {
        // TODO: Создать VulkanRenderer
        return nullptr;
    } else {
        return std::make_unique<OpenGLRenderer>();
    }
}

