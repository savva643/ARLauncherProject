#include "ARLauncherAPI.h"
#include "Application.h"
#include "Renderer.h"
#include "Scene.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

// Forward declaration to access OpenGL specific features
class OpenGLRenderer;

ARLauncherAPI::ARLauncherAPI(Application* app)
    : m_app(app)
{
}

void ARLauncherAPI::initializeDefaultContent()
{
    createWindow("Command Center", "Manage AR applications", glm::vec3(0.0f, 1.2f, -2.0f), glm::vec2(1.2f, 0.6f));
    createButton("Create Window", glm::vec3(-0.7f, 0.6f, -1.8f));
    createButton("Add 3D Model", glm::vec3(0.0f, 0.6f, -1.8f));
    createButton("Quick Menu", glm::vec3(0.7f, 0.6f, -1.8f));
    createText("AR Launcher Ready", glm::vec3(0.0f, 1.7f, -2.5f));
    createModel("demo_cube_left", glm::vec3(-0.85f, 0.25f, -1.6f), glm::vec3(0.0f), glm::vec3(0.22f));
    createModel("demo_cube_right", glm::vec3(0.85f, 0.15f, -1.6f), glm::vec3(0.0f), glm::vec3(0.18f));
}

uint32_t ARLauncherAPI::createWindow(const std::string& title,
                                     const std::string& subtitle,
                                     const glm::vec3& position,
                                     const glm::vec2& size)
{
    Renderer* renderer = getRenderer();
    if (!renderer) {
        std::cerr << "ARLauncherAPI::createWindow: renderer is null" << std::endl;
        return 0;
    }

    auto* glRenderer = dynamic_cast<OpenGLRenderer*>(renderer);
    if (!glRenderer) {
        std::cerr << "ARLauncherAPI::createWindow: OpenGL renderer required" << std::endl;
        return 0;
    }

    return glRenderer->createUIWindow(title, subtitle, position, size, false, std::string());
}

uint32_t ARLauncherAPI::createButton(const std::string& label,
                                     const glm::vec3& position)
{
    Renderer* renderer = getRenderer();
    if (!renderer) {
        std::cerr << "ARLauncherAPI::createButton: renderer is null" << std::endl;
        return 0;
    }

    auto* glRenderer = dynamic_cast<OpenGLRenderer*>(renderer);
    if (!glRenderer) {
        std::cerr << "ARLauncherAPI::createButton: OpenGL renderer required" << std::endl;
        return 0;
    }

    glm::vec2 panelSize(0.6f, 0.35f);
    std::string subtitle = "Interactive control";
    return glRenderer->createUIWindow(label, subtitle, position, panelSize, true, label);
}

uint32_t ARLauncherAPI::createText(const std::string& text,
                                   const glm::vec3& position)
{
    Renderer* renderer = getRenderer();
    if (!renderer) {
        std::cerr << "ARLauncherAPI::createText: renderer is null" << std::endl;
        return 0;
    }

    auto* glRenderer = dynamic_cast<OpenGLRenderer*>(renderer);
    if (!glRenderer) {
        std::cerr << "ARLauncherAPI::createText: OpenGL renderer required" << std::endl;
        return 0;
    }

    glm::vec2 panelSize(0.8f, 0.25f);
    return glRenderer->createUIWindow(text, "", position, panelSize, false, std::string());
}

uint32_t ARLauncherAPI::createModel(const std::string& /*name*/,
                                    const glm::vec3& position,
                                    const glm::vec3& rotationEuler,
                                    const glm::vec3& scale)
{
    Scene* scene = getScene();
    Renderer* renderer = getRenderer();
    if (!scene || !renderer) {
        std::cerr << "ARLauncherAPI::createModel: missing scene or renderer" << std::endl;
        return 0;
    }

    uint32_t objectId = scene->addCube(renderer, position, scale);
    if (objectId == 0) {
        return 0;
    }

    SceneObject* object = scene->getObject(objectId);
    if (object) {
        glm::quat rotationQuat = glm::quat(glm::radians(rotationEuler));
        object->rotation = rotationQuat;
    }
    return objectId;
}

void ARLauncherAPI::setBackgroundVideo()
{
    std::cout << "ARLauncherAPI: background video mode enabled" << std::endl;
}

void ARLauncherAPI::setLighting(const glm::vec3& direction, float intensity)
{
    std::cout << "ARLauncherAPI: lighting direction=" << direction.x << ", " << direction.y << ", " << direction.z
              << " intensity=" << intensity << std::endl;
}

Renderer* ARLauncherAPI::getRenderer() const
{
    return m_app ? m_app->getRenderer() : nullptr;
}

Scene* ARLauncherAPI::getScene() const
{
    return m_app ? m_app->getScene() : nullptr;
}
