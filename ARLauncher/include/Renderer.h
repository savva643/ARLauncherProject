#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix4x4.hpp>
#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations
struct GLFWwindow;

/**
 * @brief Базовый класс рендерера (Vulkan или OpenGL)
 */
class Renderer {
public:
    Renderer();
    virtual ~Renderer();

    virtual bool initialize(GLFWwindow* window) = 0;
    virtual void shutdown() = 0;
    
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    virtual void renderVideoBackground(const uint8_t* data, uint32_t width, uint32_t height) = 0;
    virtual void render3DObjects(const std::vector<glm::mat4>& transforms, 
                                 const std::vector<uint32_t>& meshIds) = 0;
    
    virtual void setCameraMatrix(const glm::mat4& view, const glm::mat4& projection) = 0;
    
    // Утилиты
    virtual uint32_t createMesh(const std::vector<float>& vertices, 
                                const std::vector<uint32_t>& indices) = 0;
    virtual void destroyMesh(uint32_t meshId) = 0;
    
    virtual uint32_t createTexture(const uint8_t* data, uint32_t width, uint32_t height) = 0;
    virtual void destroyTexture(uint32_t textureId) = 0;
    
    // Получение размеров окна
    void getWindowSize(int& width, int& height) const;
    
protected:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
};

/**
 * @brief OpenGL рендерер
 */
class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;

    bool initialize(GLFWwindow* window) override;
    void shutdown() override;
    
    void beginFrame() override;
    void endFrame() override;
    
    void renderVideoBackground(const uint8_t* data, uint32_t width, uint32_t height) override;
    void render3DObjects(const std::vector<glm::mat4>& transforms, 
                        const std::vector<uint32_t>& meshIds) override;
    
    void setCameraMatrix(const glm::mat4& view, const glm::mat4& projection) override;
    
    uint32_t createMesh(const std::vector<float>& vertices, 
                       const std::vector<uint32_t>& indices) override;
    void destroyMesh(uint32_t meshId) override;
    
    uint32_t createTexture(const uint8_t* data, uint32_t width, uint32_t height) override;
    void destroyTexture(uint32_t textureId) override;

private:
    uint32_t m_videoTexture;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    
    struct Mesh {
        uint32_t vao;
        uint32_t vbo;
        uint32_t ebo;
        size_t indexCount;
    };
    
    std::vector<Mesh> m_meshes;
    uint32_t m_nextMeshId;
};

// Factory function
std::unique_ptr<Renderer> createRenderer(bool useVulkan = false);

#endif // RENDERER_H

