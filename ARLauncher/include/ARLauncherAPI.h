#ifndef ARLAUNCHER_API_H
#define ARLAUNCHER_API_H

#include <glm/glm.hpp>
#include <string>
#include <cstdint>

class Application;
class Renderer;
class Scene;

class ARLauncherAPI {
public:
    explicit ARLauncherAPI(Application* app);

    void initializeDefaultContent();

    uint32_t createWindow(const std::string& title,
                          const std::string& subtitle,
                          const glm::vec3& position,
                          const glm::vec2& size);

    uint32_t createButton(const std::string& label,
                          const glm::vec3& position);

    uint32_t createText(const std::string& text,
                        const glm::vec3& position);

    uint32_t createModel(const std::string& name,
                         const glm::vec3& position,
                         const glm::vec3& rotationEuler,
                         const glm::vec3& scale);

    void setBackgroundVideo();
    void setLighting(const glm::vec3& direction, float intensity);

private:
    Renderer* getRenderer() const;
    Scene* getScene() const;

    Application* m_app;
};

#endif // ARLAUNCHER_API_H
