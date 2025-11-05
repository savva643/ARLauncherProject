# Структура проекта ARLauncherProject

## 📁 Полная структура файлов

```
ARLauncherProject/
│
├── 📄 Документация (корень)
│   ├── README.md                    # Главный README
│   ├── SUMMARY.md                   # Итоговый отчет
│   ├── REFACTORING_REPORT.md        # Детальный отчет о рефакторинге
│   ├── FILES_MAP.md                 # Карта переноса файлов
│   ├── ARCHITECTURE.md              # Архитектура проекта
│   ├── COMPONENTS_API.md            # API компонентов
│   ├── INTEGRATION_GUIDE.md         # Руководство по интеграции
│   ├── BUILD_INSTRUCTIONS.md        # Инструкции по сборке
│   ├── QUICK_START.md               # Быстрый старт
│   ├── CHANGELOG.md                 # История изменений
│   └── PROJECT_STRUCTURE.md         # Этот файл
│
├── 📦 SensorConnector/              # Компонент 1: Qt эмулятор датчиков
│   ├── include/
│   │   ├── SensorConnector.h
│   │   ├── SensorDataTypes.h
│   │   ├── NetworkServerSimplified.h
│   │   ├── UsbManager.h
│   │   ├── NetworkConfigurator.h
│   │   ├── TcpServer.h
│   │   ├── TurboJPEGDecoder.h
│   │   ├── FFmpegDecoder.h
│   │   └── FastJPEGDecoder.h
│   ├── src/
│   │   ├── SensorConnector.cpp
│   │   ├── NetworkServerSimplified.cpp
│   │   ├── UsbManager.cpp
│   │   ├── NetworkConfigurator.cpp
│   │   ├── TcpServer.cpp
│   │   ├── TurboJPEGDecoder.cpp
│   │   ├── FFmpegDecoder.cpp
│   │   └── FastJPEGDecoder.cpp
│   ├── examples/
│   │   └── example_usage.cpp
│   ├── SensorConnector.pro          # QMake файл
│   └── README.md
│
├── 📦 LensEngineSDK/                # Компонент 2: Чистый C++ AR движок
│   ├── include/
│   │   ├── LensEngineAPI.h
│   │   ├── LensEngineTypes.h
│   │   ├── LensEngine.h
│   │   ├── SensorFusionEKF.h
│   │   ├── Lidar3DProcessor.h
│   │   ├── CameraController.h
│   │   ├── ARDataProcessor.h
│   │   └── SpatialMappingSystem.h
│   ├── src/
│   │   ├── LensEngineAPI.cpp
│   │   ├── LensEngine.cpp
│   │   ├── SensorFusionEKF.cpp
│   │   ├── Lidar3DProcessor.cpp
│   │   ├── CameraController.cpp
│   │   ├── ARDataProcessor.cpp
│   │   └── SpatialMappingSystem.cpp
│   ├── examples/
│   │   └── example_usage.cpp
│   ├── CMakeLists.txt
│   ├── README.md
│   └── PROGRESS.md
│
└── 📦 ARLauncher/                   # Компонент 3: Native C++ лаунчер
    ├── include/
    │   ├── Application.h
    │   ├── Renderer.h
    │   ├── Scene.h
    │   ├── Camera.h
    │   └── ARLauncher.h
    ├── src/
    │   ├── main.cpp
    │   ├── Application.cpp
    │   ├── Renderer.cpp
    │   ├── Scene.cpp
    │   └── Camera.cpp
    ├── ui/
    │   ├── UIRenderer.h
    │   ├── UIRenderer.cpp
    │   ├── UIElement.h
    │   ├── UIElement.cpp
    │   ├── Button.h
    │   ├── Button.cpp
    │   ├── Text.h
    │   ├── Text.cpp
    │   ├── Input.h
    │   ├── Input.cpp
    │   ├── Window.h
    │   ├── Window.cpp
    │   ├── Widget.h
    │   ├── Widget.cpp
    │   └── Style.h
    ├── examples/
    │   └── demo_scene.cpp
    ├── CMakeLists.txt
    └── README.md
```

## 📊 Статистика файлов

### По типам:
- **Заголовочные файлы (.h)**: 35+
- **Исходные файлы (.cpp)**: 30+
- **CMake/QMake файлы**: 3
- **Документация (.md)**: 11
- **Примеры**: 3
- **Итого: 80+ файлов**

### По компонентам:
- **SensorConnector**: 18 файлов
- **LensEngineSDK**: 17 файлов
- **ARLauncher**: 24 файла
- **Документация**: 11 файлов
- **Примеры**: 3 файла

## 🔗 Зависимости между компонентами

```
SensorConnector (Qt)
    ↓ (сырые данные)
LensEngineSDK (Pure C++)
    ↓ (CameraPose)
ARLauncher (Native C++)
```

## 📝 Примечания

- Все компоненты могут собираться независимо
- Каждый компонент имеет свой README
- Примеры использования в папке examples/
- Полная документация в корне проекта

