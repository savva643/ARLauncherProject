# Руководство по интеграции компонентов

## Обзор архитектуры

```
┌─────────────────┐
│  SensorConnector│  (Qt)
│  USB + WiFi     │
└────────┬────────┘
         │ Сырые данные
         ▼
┌─────────────────┐
│  LensEngineSDK  │  (Чистый C++)
│  AR обработка   │
└────────┬────────┘
         │ 6DoF поза камеры
         ▼
┌─────────────────┐
│   ARLauncher    │  (Native C++)
│  Рендеринг + UI │
└─────────────────┘
```

## Интеграция SensorConnector → LensEngineSDK

### Шаг 1: Подключение SensorConnector

```cpp
#include "SensorConnector.h"

using namespace SensorConnector;

SensorConnectorCore connector;
connector.initialize();
connector.startServers(9000, 9000);

// Обработка данных
connect(&connector, &SensorConnectorCore::dataReceived,
        [](const SensorData &data) {
    // Передача данных в LensEngineSDK
});
```

### Шаг 2: Передача данных в LensEngineSDK

```cpp
#include "LensEngineAPI.h"

using namespace LensEngine;

LensEngineAPI engine;
engine.initialize();

// Обработка данных от SensorConnector
connector.dataReceived.connect([&engine](const SensorData &data) {
    switch (data.type) {
        case SensorConnector::RGB_CAMERA:
            engine.processRGBData(data.payload.data(), 
                                 data.payload.size(),
                                 width, height, data.timestamp);
            break;
        case SensorConnector::RAW_IMU:
            // Парсинг IMU данных
            RawIMUData imu;
            // ... заполнение из data.payload
            engine.processIMUData(imu);
            break;
        case SensorConnector::LIDAR_DEPTH:
            // Парсинг LiDAR данных
            engine.processLidarData(depthData, depthSize,
                                   confidenceData, confidenceSize,
                                   data.timestamp);
            break;
    }
});
```

## Интеграция LensEngineSDK → ARLauncher

### Шаг 1: Получение позы камеры

```cpp
// В Application.cpp
m_lensEngine->setPoseCallback([this](const CameraPose& pose) {
    if (m_scene) {
        m_scene->updateCameraFromAR(pose.position, pose.rotation);
    }
});
```

### Шаг 2: Обновление виртуальной камеры

```cpp
// В Scene.cpp
void Scene::updateCameraFromAR(const glm::vec3& position, const glm::quat& rotation)
{
    if (m_camera) {
        m_camera->updateFromAR(position, rotation);
    }
}
```

### Шаг 3: Рендеринг с синхронизированной камерой

```cpp
// В Application::render()
auto camera = m_scene->getCamera();
if (camera) {
    m_renderer->setCameraMatrix(
        camera->getViewMatrix(),
        camera->getProjectionMatrix()
    );
}
```

## Полный цикл интеграции

1. **SensorConnector** получает данные от iPhone (USB/WiFi)
2. **SensorConnector** декодирует JPEG/H.264
3. **SensorConnector** отправляет сырые данные через сигналы
4. **LensEngineSDK** обрабатывает данные:
   - RGB → Feature Points
   - IMU → EKF фильтр → 6DoF поза
   - LiDAR → 3D точки → Пространственное маппирование
5. **LensEngineSDK** выдает позу камеры через колбэки
6. **ARLauncher** обновляет виртуальную камеру
7. **ARLauncher** рендерит:
   - Видео-фон (RGB кадр)
   - 3D объекты (синхронизированные с реальным миром)
   - Custom UI элементы

## Формат данных

### SensorConnector → LensEngineSDK

```cpp
struct SensorData {
    DataType type;          // 0x01-0x06
    QByteArray payload;     // Сырые данные
    quint64 sequenceNumber;
    quint64 timestamp;
};
```

### LensEngineSDK → ARLauncher

```cpp
struct CameraPose {
    glm::vec3 position;     // Мировые координаты
    glm::quat rotation;    // Ориентация
    glm::mat4 viewMatrix;  // Матрица вида
    glm::mat4 projectionMatrix; // Матрица проекции
    float confidence;      // Уверенность (0-1)
};
```

## Пример полной интеграции

```cpp
// main.cpp
#include "Application.h"
#include "SensorConnector.h"
#include "LensEngineAPI.h"

int main() {
    // 1. Инициализация SensorConnector
    SensorConnector::SensorConnectorCore connector;
    connector.initialize();
    connector.startServers(9000, 9000);
    
    // 2. Инициализация LensEngineSDK
    LensEngine::LensEngineAPI engine;
    engine.initialize();
    
    // 3. Инициализация ARLauncher
    Application app;
    app.initialize(argc, argv);
    
    // 4. Подключение SensorConnector → LensEngineSDK
    QObject::connect(&connector, &SensorConnector::SensorConnectorCore::dataReceived,
                     [&engine](const SensorConnector::SensorData &data) {
        // Обработка и передача данных
    });
    
    // 5. Подключение LensEngineSDK → ARLauncher
    engine.setPoseCallback([&app](const LensEngine::CameraPose& pose) {
        app.getScene()->updateCameraFromAR(pose.position, pose.rotation);
    });
    
    // 6. Запуск приложения
    app.run();
    
    return 0;
}
```

## Тестирование

1. **Тест SensorConnector**: Запустить серверы, проверить получение данных
2. **Тест LensEngineSDK**: Передать тестовые данные, проверить позу камеры
3. **Тест ARLauncher**: Создать демо-сцену, проверить рендеринг
4. **Интеграционный тест**: Полный цикл от iPhone до экрана

## Отладка

- Используйте логи в каждом компоненте
- Проверяйте последовательность данных (sequenceNumber)
- Валидируйте позу камеры (confidence)
- Проверяйте синхронизацию временных меток

