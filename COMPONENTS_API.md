# API компонентов

## 📡 SensorConnector API

### Основной класс

```cpp
namespace SensorConnector {
    class SensorConnectorCore : public QObject {
    public:
        bool initialize();
        void startServers(quint16 tcpPort, quint16 udpPort);
        void stopServers();
        ConnectionStats getStatistics() const;
        bool isUsbConnected() const;
        bool isWiFiConnected() const;
        
    signals:
        void dataReceived(const SensorData &data);
        void statisticsUpdated(const ConnectionStats &stats);
        void connectionStatusChanged(const QString &status);
        void clientsCountChanged(int count);
    };
}
```

### Типы данных

```cpp
enum DataType {
    RGB_CAMERA = 0x01,
    LIDAR_DEPTH = 0x02,
    RAW_IMU = 0x03,
    FEATURE_POINTS = 0x04,
    CAMERA_INTRINSICS = 0x05,
    LIGHT_ESTIMATION = 0x06
};

struct SensorData {
    DataType type;
    QByteArray payload;
    quint64 sequenceNumber;
    quint64 timestamp;
};
```

### Использование

```cpp
SensorConnectorCore connector;
connector.initialize();
connector.startServers(9000, 9000);

connect(&connector, &SensorConnectorCore::dataReceived,
        [](const SensorData &data) {
    // Обработка данных
});
```

---

## 🎯 LensEngineSDK API

### Основной класс

```cpp
namespace LensEngine {
    class LensEngineAPI {
    public:
        bool initialize();
        void shutdown();
        
        void processRGBData(const uint8_t* data, size_t size, 
                            uint32_t width, uint32_t height, 
                            uint64_t timestamp);
        void processLidarData(const uint8_t* depthData, size_t depthSize,
                             const uint8_t* confidenceData, size_t confidenceSize,
                             uint64_t timestamp);
        void processIMUData(const RawIMUData& imuData);
        
        CameraPose getCurrentCameraPose() const;
        std::vector<FeaturePoint> getFeaturePoints() const;
        std::vector<glm::vec3> getLidarPoints() const;
        CameraIntrinsics getCameraIntrinsics() const;
        LightEstimation getLightEstimation() const;
        
        void setPoseCallback(PoseCallback callback);
        void setFeaturePointsCallback(FeaturePointsCallback callback);
        void setLidarPointsCallback(LidarPointsCallback callback);
        
        void setNoiseParameters(double gyroNoise, double accelNoise, 
                               double visualNoise, double lidarNoise);
        void setCameraParameters(float focalLengthX, float focalLengthY,
                                float principalPointX, float principalPointY);
    };
}
```

### Типы данных

```cpp
struct CameraPose {
    glm::vec3 position;
    glm::quat rotation;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    float confidence;
    uint64_t timestamp;
};

struct RawIMUData {
    uint64_t timestamp;
    double accelX, accelY, accelZ;
    double gyroX, gyroY, gyroZ;
    double gravityX, gravityY, gravityZ;
    double magX, magY, magZ;
};

struct FeaturePoint {
    glm::vec3 position;
    glm::vec2 screenPosition;
    float confidence;
    uint64_t trackId;
};
```

### Использование

```cpp
LensEngineAPI engine;
engine.initialize();

engine.setPoseCallback([](const CameraPose& pose) {
    // Обработка позы камеры
});

engine.processIMUData(imuData);
CameraPose pose = engine.getCurrentCameraPose();
```

---

## 🎨 ARLauncher API

### Основной класс

```cpp
class Application {
public:
    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();
    
    GLFWwindow* getWindow() const;
    Renderer* getRenderer() const;
    Scene* getScene() const;
    UIRenderer* getUIRenderer() const;
    
    bool isRunning() const;
    void requestExit();
};
```

### Рендерер

```cpp
class Renderer {
public:
    virtual bool initialize(GLFWwindow* window) = 0;
    virtual void shutdown() = 0;
    
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    virtual void renderVideoBackground(const uint8_t* data, 
                                      uint32_t width, uint32_t height) = 0;
    virtual void render3DObjects(const std::vector<glm::mat4>& transforms,
                                const std::vector<uint32_t>& meshIds) = 0;
    
    virtual void setCameraMatrix(const glm::mat4& view, 
                                const glm::mat4& projection) = 0;
    
    virtual uint32_t createMesh(const std::vector<float>& vertices,
                                const std::vector<uint32_t>& indices) = 0;
    virtual uint32_t createTexture(const uint8_t* data,
                                  uint32_t width, uint32_t height) = 0;
};

std::unique_ptr<Renderer> createRenderer(bool useVulkan = false);
```

### Сцена

```cpp
class Scene {
public:
    bool initialize();
    void shutdown();
    
    uint32_t addObject(const SceneObject& object);
    void removeObject(uint32_t objectId);
    SceneObject* getObject(uint32_t objectId);
    
    Camera* getCamera() const;
    void update(float deltaTime);
    
    std::vector<glm::mat4> getObjectTransforms() const;
    std::vector<uint32_t> getVisibleMeshIds() const;
    
    void createDemoScene();
    void updateCameraFromAR(const glm::vec3& position, 
                            const glm::quat& rotation);
};
```

### Камера

```cpp
class Camera {
public:
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void updateFromAR(const glm::vec3& position, const glm::quat& rotation);
    
    glm::vec3 getPosition() const;
    glm::quat getRotation() const;
    
    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    void setProjection(float fov, float aspect, 
                      float nearPlane, float farPlane);
};
```

### UI система

```cpp
class UIRenderer {
public:
    void addElement(std::shared_ptr<UIElement> element);
    void removeElement(std::shared_ptr<UIElement> element);
    
    void render();
    void handleMouseClick(float x, float y);
    void handleMouseMove(float x, float y);
    void handleKeyPress(int key, int action);
};

// UI элементы
class Button : public UIElement {
    void setText(const std::string& text);
    void setOnClick(std::function<void()> callback);
};

class Text : public UIElement {
    void setText(const std::string& text);
    void setFontSize(float size);
};

class Input : public UIElement {
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    void setOnChange(std::function<void(const std::string&)> callback);
};

class Window : public UIElement {
    void setTitle(const std::string& title);
    void addChild(std::shared_ptr<UIElement> child);
    void setMinimized(bool minimized);
    void setMaximized(bool maximized);
};

class Widget : public UIElement {
    void addChild(std::shared_ptr<UIElement> child);
    void setLayout(int layout); // 0=vertical, 1=horizontal, 2=grid
    void setSpacing(float spacing);
};
```

### Использование

```cpp
Application app;
app.initialize(argc, argv);

// Получение компонентов
Scene* scene = app.getScene();
Renderer* renderer = app.getRenderer();
UIRenderer* ui = app.getUIRenderer();

// Создание UI
auto button = std::make_shared<Button>("Start");
button->setOnClick([]() { /* ... */ });
ui->addElement(button);

// Запуск
app.run();
```

---

## 🔗 Интеграция компонентов

### SensorConnector → LensEngineSDK

```cpp
SensorConnectorCore connector;
LensEngineAPI engine;

connector.dataReceived.connect([&engine](const SensorData &data) {
    switch (data.type) {
        case RGB_CAMERA:
            engine.processRGBData(data.payload.data(), data.payload.size(),
                                 width, height, data.timestamp);
            break;
        case RAW_IMU:
            // Парсинг и передача IMU данных
            engine.processIMUData(imuData);
            break;
        // ...
    }
});
```

### LensEngineSDK → ARLauncher

```cpp
LensEngineAPI engine;
Application app;

engine.setPoseCallback([&app](const CameraPose& pose) {
    app.getScene()->updateCameraFromAR(pose.position, pose.rotation);
});

app.run();
```

---

## 📝 Примечания

- Все API используют стандартные C++ типы (кроме SensorConnector с Qt)
- Колбэки через `std::function` для асинхронной обработки
- Типы данных определены в заголовочных файлах каждого компонента
- Документация в заголовочных файлах (Doxygen-стиль)

