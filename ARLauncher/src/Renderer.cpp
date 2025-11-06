#include "Renderer.h"
#include <GLFW/glfw3.h>
#ifdef USE_OPENGL
#include <GL/gl.h>
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
    , m_nextMeshId(1)
{
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
    // Убеждаемся, что контекст активен
    glfwMakeContextCurrent(window);
    
    // Проверяем, что контекст действительно активен
    if (glfwGetCurrentContext() != window) {
        std::cerr << "OpenGLRenderer::initialize: failed to make context current" << std::endl;
        return false;
    }
    
    // Инициализация OpenGL
    glfwGetWindowSize(window, &m_width, &m_height);
    
    // Очищаем возможные предыдущие ошибки
    while (glGetError() != GL_NO_ERROR) {}
    
    // Устанавливаем viewport
    glViewport(0, 0, m_width, m_height);
    
    // Проверяем наличие ошибок после viewport
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glViewport: " << error << std::endl;
        // Не возвращаем false, так как это может быть просто предупреждение
    }
    
    // Настраиваем OpenGL состояние
    glEnable(GL_DEPTH_TEST);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glEnable(GL_DEPTH_TEST): " << error << std::endl;
    }
    
    glEnable(GL_BLEND);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glEnable(GL_BLEND): " << error << std::endl;
    }
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glBlendFunc: " << error << std::endl;
    }
    
    // Проверяем версию OpenGL (может быть NULL если функции не загружены)
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version) {
        std::cout << "OpenGL version: " << version << std::endl;
    } else {
        std::cerr << "Warning: glGetString(GL_VERSION) returned NULL - OpenGL functions may not be loaded" << std::endl;
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
    std::cout << "✅ VulkanRenderer initialized successfully" << std::endl;
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

