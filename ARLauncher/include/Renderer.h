#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations
struct GLFWwindow;
#ifdef USE_VULKAN
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#endif

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
    
    // Управление opacity для анимации
    virtual void setVideoOpacity(float opacity) = 0;
    virtual void set3DObjectsOpacity(float opacity) = 0;
    
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
 * @brief Vulkan рендерер (заглушка)
 */
#ifdef USE_VULKAN
class VulkanRenderer : public Renderer {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

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
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    
    // Vulkan объекты
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    uint32_t m_graphicsQueueFamily;
    uint32_t m_presentQueueFamily;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    VkRenderPass m_renderPass;
    VkRenderPass m_renderPassWithLoad; // Render pass с LOAD_OP_LOAD для видео фона
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_currentFrame;
    uint32_t m_currentImageIndex;
    bool m_initialized;
    
    // Структура для хранения мешей
    struct VulkanMesh {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };
    std::vector<VulkanMesh> m_meshes;
    uint32_t m_nextMeshId;
    
    // Видео текстура для RGB камеры
    VkImage m_videoImage;
    VkDeviceMemory m_videoImageMemory;
    VkImageView m_videoImageView;
    VkSampler m_videoSampler;
    bool m_videoTextureInitialized;
    
    // Вспомогательные методы для создания буферов
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    bool createVideoTexture(uint32_t width, uint32_t height);
    void updateVideoTexture(const uint8_t* data, uint32_t width, uint32_t height);
    
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createRenderPassWithLoad();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    void cleanupSwapchain();
    void recreateSwapchain();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Image operations for background blit path (no graphics pipeline required)
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyVideoToSwapchain(VkCommandBuffer cmd, VkImage dstSwapchainImage, uint32_t width, uint32_t height);
};
#endif

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

    void setVideoOpacity(float opacity) override;
    void set3DObjectsOpacity(float opacity) override;

private:
    uint32_t m_videoTexture;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_videoOpacity;
    float m_3dObjectsOpacity;
    
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

