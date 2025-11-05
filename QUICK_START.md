# Быстрый старт

## 🚀 Запуск проекта

### 1. Сборка компонентов

```bash
# SensorConnector
cd SensorConnector
qmake && make

# LensEngineSDK  
cd ../LensEngineSDK
mkdir build && cd build
cmake .. && make

# ARLauncher
cd ../../ARLauncher
mkdir build && cd build
cmake .. -DUSE_OPENGL=ON && make
```

### 2. Запуск

```bash
# Запуск ARLauncher
cd ARLauncher/build/bin
./ARLauncher
```

## 📝 Пример использования

### SensorConnector

```cpp
#include "SensorConnector.h"
using namespace SensorConnector;

SensorConnectorCore connector;
connector.initialize();
connector.startServers(9000, 9000);

// Подключение к iPhone через USB или WiFi
// Данные будут поступать через сигнал dataReceived
```

### LensEngineSDK

```cpp
#include "LensEngineAPI.h"
using namespace LensEngine;

LensEngineAPI engine;
engine.initialize();

// Обработка данных
engine.processRGBData(rgbData, size, width, height, timestamp);
engine.processIMUData(imuData);

// Получение позы камеры
CameraPose pose = engine.getCurrentCameraPose();
```

### ARLauncher

```cpp
#include "Application.h"

Application app;
app.initialize(argc, argv);
app.run();
```

## 🔗 Интеграция

См. [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) для полной инструкции по интеграции всех компонентов.

## 📚 Документация

- [REFACTORING_REPORT.md](REFACTORING_REPORT.md) - Детальный отчет о рефакторинге
- [SUMMARY.md](SUMMARY.md) - Итоговый отчет
- [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) - Инструкции по сборке
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) - Руководство по интеграции

