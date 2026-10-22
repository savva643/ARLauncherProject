#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <array>
#include <functional>
#include <unordered_map>

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
    virtual void renderStoredVideoBackground() = 0; // Рендерит сохраненную видео текстуру
    virtual void render3DObjects(const std::vector<glm::mat4>& transforms, 
                                 const std::vector<uint32_t>& meshIds) = 0;
    
    // Управление opacity для анимации
    virtual void setVideoOpacity(float opacity) = 0;
    virtual void set3DObjectsOpacity(float opacity) = 0;
    
    virtual void setCameraMatrix(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void setDeviceOrientation(float degrees) {}

    // Утилиты
    virtual uint32_t createMesh(const std::vector<float>& vertices, 
                                const std::vector<uint32_t>& indices) = 0;
    virtual void destroyMesh(uint32_t meshId) = 0;
    
    virtual uint32_t createTexture(const uint8_t* data, uint32_t width, uint32_t height) = 0;
    virtual void destroyTexture(uint32_t textureId) = 0;

    virtual void renderUIWindows() {}

    virtual uint32_t createUIWindow(const std::string& title,
                                      const std::string& subtitle,
                                      const glm::vec3& position,
                                      const glm::vec2& size,
                                      bool hasButton = false,
                                      const std::string& buttonText = "") { return 0; }
    
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
    void renderStoredVideoBackground() override;
    void render3DObjects(const std::vector<glm::mat4>& transforms, 
                        const std::vector<uint32_t>& meshIds) override;
    
    void setCameraMatrix(const glm::mat4& view, const glm::mat4& projection) override;
    void setDeviceOrientation(float degrees) override;

    uint32_t createMesh(const std::vector<float>& vertices,
                       const std::vector<uint32_t>& indices) override;
    void destroyMesh(uint32_t meshId) override;

    uint32_t createTexture(const uint8_t* data, uint32_t width, uint32_t height) override;
    void destroyTexture(uint32_t textureId) override;

    void updateTexture(uint32_t textureId, const uint8_t* data, uint32_t width, uint32_t height);
    
    void setVideoOpacity(float opacity) override;
    void set3DObjectsOpacity(float opacity) override;

    void setUIOverlayTexture(uint32_t textureId, uint32_t width, uint32_t height);
    void setUI3DTexture(uint32_t textureId, uint32_t width, uint32_t height,
                        const glm::vec3& position, const glm::vec2& size, bool billboard);

    void renderUIWindows() override;

    uint32_t createUIWindow(const std::string& title,
                            const std::string& subtitle,
                            const glm::vec3& position,
                            const glm::vec2& size,
                            bool hasButton = false,
                            const std::string& buttonText = "") override;

    // ---------------- Passthrough-видео (NV12) с репроекцией ----------------
    struct PassthroughFrame {
        const uint8_t* y = nullptr;     // width*height
        const uint8_t* uv = nullptr;    // (width/2)*(height/2) пар CbCr
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t id = 0;                // одинаковый id повторно не загружается
    };
    struct PassthroughParams {
        glm::mat3 cvFromEye{1.0f};                            // оси глаза (GL) -> оси камеры (OpenCV)
        glm::vec4 camIntrinsicsNorm{1.0f, 1.0f, 0.5f, 0.5f};  // fx/w, fy/h, cx/w, cy/h
        glm::vec2 eyeTanHalfFov{1.0f, 1.0f};
        float opacity = 1.0f;
        bool fullRange = true;
        bool bt709 = true;
        float depthOverlay = 0.0f;                            // 0..1 — LiDAR цветом поверх видео (отладка)
        float depthMaxMeters = 5.0f;
    };
    struct DepthOverlayFrame {
        const float* depth = nullptr;   // метры, width*height; карта глубины выровнена с кадром камеры
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t id = 0;
    };

    /// Колбэк вызывается каждый кадр ПОСЛЕ ожидания свободного кадра и acquire, но ДО начала render pass.
    /// Здесь приложение берёт самую свежую позу (late latching) и загружает видеокадр.
    void setPreRenderPassCallback(std::function<void()> cb) { m_preRenderPass = std::move(cb); }
    /// Только из pre-render-pass колбэка: копирование кадра в GPU-текстуры (staging, без остановки конвейера)
    void uploadPassthroughFrame(const PassthroughFrame& frame);
    /// Только из pre-render-pass колбэка: карта глубины LiDAR (для наложения в renderPassthrough)
    void uploadDepthOverlay(const DepthOverlayFrame& frame);
    bool hasDepthOverlay() const { return m_ptHasDepth; }
    /// Внутри render pass: полноэкранный проход с репроекцией
    void renderPassthrough(const PassthroughParams& params);
    bool hasPassthroughFrame() const { return m_ptHasFrame; }
    /// Область вывода для passthrough и 3D (стерео side-by-side). width<=0 — весь экран.
    void setEyeViewport(float x, float y, float width, float height);

    // ---- 3D-объекты в реальной комнате: окклюзия по LiDAR, свет и отражения по камере, тени на пол
    struct ObjectMaterial {
        glm::vec4 color{0.2f, 0.7f, 1.0f, 1.0f};   // sRGB + альфа
        float metallic = 0.0f;
        float roughness = 0.5f;
        float occlusionBias = 0.0f;   // м: объект, совпадающий с реальной поверхностью (рука, меш комнаты), не прячется ею
        bool castShadow = true;
    };
    struct SceneParams {
        glm::mat3 cvFromEye{1.0f};                            // как у passthrough: луч глаза -> пиксель видео/LiDAR
        glm::vec4 camIntrinsicsNorm{1.0f, 1.0f, 0.5f, 0.5f};
        bool fullRange = true;
        bool bt709 = true;
        bool occlusion = true;            // реальные предметы закрывают виртуальные (нужна карта LiDAR)
        float occlusionSoftness = 0.03f;  // м — мягкий край окклюзии
        bool reflections = true;
        bool shadows = true;
        bool floorValid = false;
        float floorY = -1.5f;             // пол в мире рендера
        glm::vec3 lightDirWorld{0.3f, 0.9f, 0.3f};   // к источнику
        glm::vec3 lightColor{0.6f};       // линейный RGB (блики)
        float shadowStrength = 0.45f;
        bool alphaLayer = false;          // фон прозрачный (кадр для телефона, фон — его камера): тени — в альфу
        std::array<glm::vec3, 9> sh{};    // SH окружения в мире (линейный RGB); все нули — нейтральный свет
        float opacity = 1.0f;
    };
    void setSceneParams(const SceneParams& params) { m_sceneParams = params; }
    void setObjectMaterials(std::vector<ObjectMaterial> materials) { m_materials = std::move(materials); }
    /// Карта окружения equirect RGBA8 (EnvironmentProbe) — только до render pass (latch)
    void uploadEnvironmentMap(const uint8_t* rgba, uint32_t width, uint32_t height, uint64_t version);
    /// Коррекция линз шлема: сцена рисуется в отдельную картинку, затем раскладывается на экран с
    /// предыскажением «бочкой» и коррекцией хроматической аберрации (для каждого глаза)
    struct LensParams {
        bool enabled = false;
        float k1 = 0.22f;          // радиальные коэффициенты (Brown): f(r) = 1 + k1 r^2 + k2 r^4
        float k2 = 0.24f;
        float chroma = 0.012f;     // разница масштаба красного/синего относительно зелёного
        float scale = 1.0f;        // общий масштаб (меньше 1 — картинка крупнее, меньше чёрных углов)
        float centerShift = 0.0f;  // сдвиг центра линзы к носу (доля полуширины глаза), стерео
        bool stereo = false;
    };
    void setLensParams(const LensParams& params);
    const LensParams& lensParams() const { return m_lens; }

    /// Заменить геометрию меша (меш комнаты меняется раз в секунду); старые буферы удаляются, когда GPU их отпустит
    bool updateMesh(uint32_t meshId, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);

    // ---- Чтение готового кадра (обратный поток на телефон)
    struct Readback {
        const uint8_t* pixels = nullptr;
        int width = 0, height = 0, stride = 0;
        bool bgra = true;
    };
    /// Колбэк зовётся в endFrame, когда GPU дорисовал кадр (рендер-поток ждёт GPU — только пока включено)
    void setReadbackCallback(std::function<void(const Readback&)> cb) { m_readbackCb = std::move(cb); }
    void setReadbackEnabled(bool enabled) { m_readbackEnabled = enabled; }
    bool readbackSupported() const { return m_swapchainTransferSrc; }
    void setClearColor(const glm::vec4& c) { m_clearColor = c; }
    /// Панель статуса (2D поверх кадра) не рисовать — кадр уходит на телефон
    void setOverlayHidden(bool hidden) { m_overlayHidden = hidden; }

    VkExtent2D swapchainExtent() const { return m_swapchainExtent; }
    const char* presentModeName() const;
    VkPresentModeKHR presentMode() const { return m_presentMode; }
    bool isFrameActive() const { return m_frameActive; }

private:
    static constexpr uint32_t kFramesInFlight = 2;
    VkCommandBuffer currentCmd() const { return m_frameCmd[m_frameSlot]; }

    // Кадры в полёте (команды, ожидание swapchain, fence) + семафор «отрисовано» на каждый образ swapchain
    std::array<VkCommandBuffer, kFramesInFlight> m_frameCmd{};
    std::array<VkSemaphore, kFramesInFlight> m_frameImageAvailable{};
    std::array<VkFence, kFramesInFlight> m_frameFences{};
    std::vector<VkSemaphore> m_imageRenderFinished;
    uint32_t m_frameSlot = 0;
    bool m_frameActive = false;
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    std::function<void()> m_preRenderPass;
    bool m_eyeViewportSet = false;
    VkViewport m_eyeViewport{};
    void applyEyeViewport(VkCommandBuffer cmd);
    bool createRenderFinishedSemaphores();
    void destroyRenderFinishedSemaphores();

    // Passthrough-ресурсы
    struct PlaneImage {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    PlaneImage m_ptY;
    PlaneImage m_ptUV;
    PlaneImage m_ptDepth;   // R16_SFLOAT, метры
    uint32_t m_ptDepthW = 0;
    uint32_t m_ptDepthH = 0;
    bool m_ptDepthLayoutReady = false;
    bool m_ptHasDepth = false;
    uint64_t m_ptDepthLastId = 0;
    std::array<VkBuffer, kFramesInFlight> m_ptDepthStaging{};
    std::array<VkDeviceMemory, kFramesInFlight> m_ptDepthStagingMemory{};
    std::array<void*, kFramesInFlight> m_ptDepthStagingPtr{};
    uint32_t m_ptWidth = 0;
    uint32_t m_ptHeight = 0;
    bool m_ptLayoutReady = false;
    bool m_ptHasFrame = false;
    uint64_t m_ptLastId = 0;
    std::array<VkBuffer, kFramesInFlight> m_ptStaging{};
    std::array<VkDeviceMemory, kFramesInFlight> m_ptStagingMemory{};
    std::array<void*, kFramesInFlight> m_ptStagingPtr{};
    VkDeviceSize m_ptStagingSize = 0;
    VkSampler m_ptSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_ptSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_ptPool = VK_NULL_HANDLE;
    VkDescriptorSet m_ptSet = VK_NULL_HANDLE;
    VkPipelineLayout m_ptPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_ptPipeline = VK_NULL_HANDLE;
    bool createPassthroughPipeline();
    void destroyPassthrough();
    bool ensurePassthroughResources(uint32_t width, uint32_t height);
    bool ensureDepthResources(uint32_t width, uint32_t height);
    void destroyDepthResources();
    void uploadDepthHalf(const uint16_t* half, uint32_t width, uint32_t height);
    bool createPlaneImage(PlaneImage& plane, VkFormat format, uint32_t width, uint32_t height);
    void destroyPlaneImage(PlaneImage& plane);

    // Сцена: set 1 = UBO кадра + карта окружения; set 0 общий с passthrough (Y, CbCr, LiDAR)
    SceneParams m_sceneParams;
    std::vector<ObjectMaterial> m_materials;
    VkDescriptorSetLayout m_sceneSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_scenePool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, kFramesInFlight> m_sceneSets{};
    std::array<VkBuffer, kFramesInFlight> m_sceneUbo{};
    std::array<VkDeviceMemory, kFramesInFlight> m_sceneUboMemory{};
    std::array<void*, kFramesInFlight> m_sceneUboPtr{};
    PlaneImage m_env;
    uint32_t m_envW = 0;
    uint32_t m_envH = 0;
    VkSampler m_envSampler = VK_NULL_HANDLE;
    std::array<VkBuffer, kFramesInFlight> m_envStaging{};
    std::array<VkDeviceMemory, kFramesInFlight> m_envStagingMemory{};
    std::array<void*, kFramesInFlight> m_envStagingPtr{};
    bool m_envLayoutReady = false;
    bool m_envReal = false;
    uint64_t m_envVersion = 0;
    VkPipeline m_shadowPipeline = VK_NULL_HANDLE;
    // Коррекция линз
    LensParams m_lens;
    VkRenderPass m_offscreenPass = VK_NULL_HANDLE;
    VkImage m_offscreenImage = VK_NULL_HANDLE;
    VkDeviceMemory m_offscreenMemory = VK_NULL_HANDLE;
    VkImageView m_offscreenView = VK_NULL_HANDLE;
    VkFramebuffer m_offscreenFramebuffer = VK_NULL_HANDLE;
    VkSampler m_lensSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_lensSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_lensPool = VK_NULL_HANDLE;
    VkDescriptorSet m_lensSet = VK_NULL_HANDLE;
    VkPipelineLayout m_lensPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_lensPipeline = VK_NULL_HANDLE;
    bool lensActive() const { return m_lens.enabled && m_lensPipeline != VK_NULL_HANDLE && m_offscreenFramebuffer != VK_NULL_HANDLE; }
    bool createLensPipeline();
    bool createLensTargets();
    void destroyLensTargets();
    void destroyLens();
    void drawLensPass(VkCommandBuffer cmd);
    uint64_t m_frameNumber = 0;
    glm::vec4 m_clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    std::function<void(const Readback&)> m_readbackCb;
    bool m_readbackEnabled = false;
    bool m_overlayHidden = false;
    bool m_swapchainTransferSrc = false;
    std::array<VkBuffer, kFramesInFlight> m_readbackBuf{};
    std::array<VkDeviceMemory, kFramesInFlight> m_readbackMem{};
    std::array<void*, kFramesInFlight> m_readbackPtr{};
    std::array<VkDeviceSize, kFramesInFlight> m_readbackSize{};
    std::array<bool, kFramesInFlight> m_readbackCoherent{};
    bool recordReadback(VkCommandBuffer cmd);
    void destroyReadback();
    uint64_t m_sceneUboFrame = UINT64_MAX;
    bool createSceneResources();
    void destroySceneResources();
    void ensureSceneInputs();       // заглушки видео/LiDAR/карты, чтобы дескрипторы были валидны с первого кадра
    void writeSceneUbo();

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_deviceOrientation = 0.0f;

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

    // Depth resources
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    VkFormat m_depthFormat;

    // Graphics pipeline for meshes
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkCommandPool m_commandPool;
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
    struct RetiredMesh {
        VulkanMesh mesh;
        uint64_t frame;
    };
    std::vector<RetiredMesh> m_retiredMeshes;   // удаляются через kFramesInFlight кадров
    bool fillMesh(VulkanMesh& mesh, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
    void releaseMeshBuffers(VulkanMesh& mesh);
    void collectRetiredMeshes(bool all);
    
    // Видео текстура для RGB камеры (Vulkan textured quad)
    uint32_t m_videoTextureId;
    bool m_videoTextureInitialized;
    float m_videoOpacity;  // Opacity для видео фона (0.0 - 1.0)
    float m_3dObjectsOpacity;  // Opacity для 3D объектов (0.0 - 1.0)
    uint32_t m_videoWidth;  // Ширина видео текстуры
    uint32_t m_videoHeight; // Высота видео текстуры

    struct VulkanTexture {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
        uint32_t width;
        uint32_t height;
    };
    std::vector<VulkanTexture> m_textures;
    uint32_t m_nextTextureId;

    struct UITexturePlacement {
        uint32_t textureId;
        uint32_t width;
        uint32_t height;
        glm::vec3 position;
        glm::vec2 size;
        bool billboard;
        bool enabled;
        float rotation = 0.0f; // Z-rotation in degrees (for video orientation)
    };
    UITexturePlacement m_ui3D;
    UITexturePlacement m_uiOverlay;

    VkDescriptorSetLayout m_uiDescriptorSetLayout;
    VkDescriptorPool m_uiDescriptorPool;
    VkDescriptorSet m_uiDescriptorSet;
    VkPipelineLayout m_uiPipelineLayout;
    VkPipeline m_uiPipeline;
    VkPipeline m_uiPipelineOverlay;
    VkBuffer m_uiVertexBuffer;
    VkDeviceMemory m_uiVertexBufferMemory;
    VkBuffer m_uiIndexBuffer;
    VkDeviceMemory m_uiIndexBufferMemory;
    uint32_t m_uiIndexCount;

    class FontRenderer* m_fontRenderer;
    
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
    bool createDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool createGraphicsPipeline();
    VkShaderModule createShaderModule(const uint32_t* code, size_t sizeBytes);
    void cleanupSwapchain();
    void recreateSwapchain();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool createUIResources();
    void destroyUIResources();
    bool createUIPipeline();
    VkDescriptorSet updateUIDescriptorSet(uint32_t textureId);   // кэш: один set на текстуру
    std::unordered_map<uint32_t, VkDescriptorSet> m_uiTextureSets;
    void renderUITexturedQuad(const UITexturePlacement& placement, bool overlayMode);

    // Image operations
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
};
#endif

/**
 * @brief LEGACY: OpenGL рендерер.
 *
 * Не развивается и не поддерживает новый конвейер (NV12 passthrough, репроекция, late latching).
 * Основной рендерер GlaskiOS — VulkanRenderer. Собирается только с -DUSE_VULKAN=OFF -DUSE_OPENGL=ON.
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
    void renderStoredVideoBackground() override;
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

    void renderUIWindows() override;
    
    // Получение FontRenderer для использования в Text элементах
    class FontRenderer* getFontRenderer() const { return m_fontRenderer; }

    uint32_t createUIWindow(const std::string& title,
                            const std::string& subtitle,
                            const glm::vec3& position,
                            const glm::vec2& size,
                            bool hasButton = false,
                            const std::string& buttonText = "") override;

private:
    struct UIWindow {
        uint32_t id;
        uint32_t fbo;
        uint32_t texture;
        uint32_t rbo;
        glm::vec3 position;
        glm::vec2 size;
        bool billboard;
        std::string title;
        std::string subtitle;
        bool hasButton;
        std::string buttonText;
        int pixelWidth;
        int pixelHeight;
    };
    
    uint32_t loadShader(const char* vertexPath, const char* fragmentPath);
    uint32_t compileShader(const char* source, uint32_t type);
    std::string readShaderFile(const char* filepath);
    void createFullscreenQuad();
    void createUIQuad();
    void renderUIWindowContent(const UIWindow& window);

    uint32_t m_videoTexture;
    uint32_t m_videoTextureWidth;
    uint32_t m_videoTextureHeight;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_videoOpacity;
    float m_3dObjectsOpacity;

    uint32_t m_basicShaderProgram;
    uint32_t m_videoShaderProgram;
    uint32_t m_glassmorphismShaderProgram;
    uint32_t m_uiShaderProgram;

    uint32_t m_fullscreenQuadVAO;
    uint32_t m_fullscreenQuadVBO;

    uint32_t m_uiQuadVAO;
    uint32_t m_uiQuadVBO;
    uint32_t m_uiQuadEBO;

    struct Mesh {
        uint32_t vao;
        uint32_t vbo;
        uint32_t ebo;
        size_t indexCount;
    };

    std::vector<Mesh> m_meshes;
    uint32_t m_nextMeshId;

    std::vector<UIWindow> m_uiWindows;
    uint32_t m_nextUIWindowId;

    void* m_simpleNVG; // NVGcontext* (определен в Renderer.cpp)
    
    // FontRenderer для нормального отображения текста
    class FontRenderer* m_fontRenderer;
};

// Factory function
std::unique_ptr<Renderer> createRenderer(bool useVulkan = false);

#endif // RENDERER_H

