# Руководство по сборке ARLauncherProject на Linux (Ubuntu)

## 📋 Быстрая сборка

### Автоматическая сборка (рекомендуется)

```bash
cd /home/savva/Documents/GitHub/ARLauncherProject
./build_all.sh
```

Скрипт автоматически:
1. Проверит и установит все зависимости
2. Соберет SensorConnector (Qt)
3. Соберет LensEngineSDK (CMake)
4. Соберет ARLauncher (CMake)

### Ручная сборка

#### Шаг 1: Установка зависимостей

```bash
sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev qtbase5-dev-tools \
    cmake build-essential \
    libturbojpeg0-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev \
    libglm-dev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libopencv-dev  # Опционально, для OpenCV
```

#### Шаг 2: Сборка SensorConnector

```bash
cd SensorConnector
qmake SensorConnector.pro
make -j$(nproc)
```

Результат: `lib/libSensorConnector.a`

#### Шаг 3: Сборка LensEngineSDK

```bash
cd ../LensEngineSDK
mkdir -p build && cd build
cmake .. -DLENSENGINE_USE_OPENCV=ON
make -j$(nproc)
```

Результат: `build/lib/libLensEngineSDK.a`

#### Шаг 4: Сборка ARLauncher

```bash
cd ../../ARLauncher
mkdir -p build && cd build
cmake .. -DUSE_OPENGL=ON
make -j$(nproc)
```

Результат: `build/bin/ARLauncher`

## 🚀 Запуск

### Вариант 1: Только ARLauncher (без интеграции с SensorConnector)

```bash
cd ARLauncher/build/bin
./ARLauncher
```

### Вариант 2: Полная интеграция (SensorConnector + LensEngineSDK + ARLauncher)

Для полной интеграции нужно создать главное приложение, которое:
1. Инициализирует SensorConnector
2. Подключает данные к LensEngineSDK
3. Передает позу камеры в ARLauncher

См. файл `examples/full_integration_example.cpp` для примера.

## 📦 Структура сборки

После сборки структура будет такой:

```
ARLauncherProject/
├── SensorConnector/
│   └── lib/
│       └── libSensorConnector.a          ✅
├── LensEngineSDK/
│   └── build/
│       └── lib/
│           └── libLensEngineSDK.a        ✅
└── ARLauncher/
    └── build/
        └── bin/
            └── ARLauncher                ✅
```

## 🔧 Интеграция компонентов

### Архитектура

```
iPhone (данные)
    ↓ USB/WiFi
SensorConnector (Qt библиотека)
    ↓ Сырые данные (SensorData)
LensEngineSDK (C++ библиотека)
    ↓ 6DoF поза камеры (CameraPose)
ARLauncher (C++ приложение)
    ↓ Рендеринг
Экран
```

### Как подключить SensorConnector к ARLauncher

1. **Собрать SensorConnector с Qt** (уже сделано выше)

2. **Обновить CMakeLists.txt ARLauncher:**

```cmake
# В ARLauncher/CMakeLists.txt
option(USE_SENSOR_CONNECTOR "Use SensorConnector library" ON)

if(USE_SENSOR_CONNECTOR)
    find_package(Qt5 COMPONENTS Core Network REQUIRED)
    set(SENSOR_CONNECTOR_DIR "${CMAKE_SOURCE_DIR}/../SensorConnector")
    
    target_link_libraries(ARLauncher PRIVATE 
        ${SENSOR_CONNECTOR_DIR}/lib/libSensorConnector.a
        Qt5::Core
        Qt5::Network
    )
    target_include_directories(ARLauncher PRIVATE ${SENSOR_CONNECTOR_DIR}/include)
endif()
```

3. **Использовать в коде:**

```cpp
// В Application.cpp
#include "SensorConnector.h"
#include "LensEngineAPI.h"

using namespace SensorConnector;
using namespace LensEngine;

// Инициализация
SensorConnectorCore connector;
connector.initialize();
connector.startServers(9000, 9000);

LensEngineAPI engine;
engine.initialize();

// Подключение данных
QObject::connect(&connector, &SensorConnectorCore::dataReceived,
    [&engine](const SensorData &data) {
        // Передача данных в LensEngineSDK
        switch(data.type) {
            case RGB_CAMERA:
                engine.processRGBData(...);
                break;
            case RAW_IMU:
                engine.processIMUData(...);
                break;
            // ...
        }
    });

// Получение позы камеры
engine.setPoseCallback([this](const CameraPose& pose) {
    // Обновление виртуальной камеры в ARLauncher
    m_scene->updateCameraFromAR(pose.position, pose.rotation);
});
```

## ⚠️ Решение проблем

### Проблема: "qmake not found"

```bash
sudo apt-get install qtbase5-dev qtbase5-dev-tools
```

### Проблема: "libturbojpeg not found"

```bash
sudo apt-get install libturbojpeg0-dev
```

### Проблема: "FFmpeg libraries not found"

```bash
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

### Проблема: "GLFW not found"

```bash
sudo apt-get install libglfw3-dev
```

### Проблема: "GLM not found"

```bash
sudo apt-get install libglm-dev
```

### Проблема: "OpenGL not found"

```bash
sudo apt-get install libgl1-mesa-dev
```

## 📚 Дополнительная информация

- **Архитектура проекта**: см. `ARCHITECTURE.md`
- **Интеграция компонентов**: см. `INTEGRATION_GUIDE.md`
- **API компонентов**: см. `COMPONENTS_API.md`
- **Детальные инструкции**: см. `BUILD_INSTRUCTIONS.md`

## ✅ Проверка сборки

После сборки проверьте:

```bash
# SensorConnector
ls -lh SensorConnector/lib/libSensorConnector.a

# LensEngineSDK
ls -lh LensEngineSDK/build/lib/libLensEngineSDK.a

# ARLauncher
ls -lh ARLauncher/build/bin/ARLauncher
./ARLauncher/build/bin/ARLauncher
```

Все должно быть успешно собрано! 🎉

