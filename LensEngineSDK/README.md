# LensEngineSDK

## Описание

LensEngineSDK - это чистый C++ AR движок для обработки данных от iPhone.

## Особенности

- Чистый C++ без зависимостей от Qt
- Использование glm для математики
- EKF фильтр для сенсорного фьюжна
- Обработка всех типов AR данных (RGB, LiDAR, IMU, Feature Points, etc.)
- Пространственное маппирование
- Опциональная поддержка OpenCV для feature detection

## Зависимости

- **glm** - математическая библиотека (векторы, матрицы, кватернионы)
- **OpenCV** (опционально) - для feature detection и обработки изображений

## Сборка

```bash
mkdir build
cd build
cmake ..
make
```

## Использование

```cpp
#include "LensEngineAPI.h"

using namespace LensEngine;

LensEngineAPI engine;
engine.initialize();

// Установка колбэков
engine.setPoseCallback([](const CameraPose& pose) {
    // Обработка позы камеры
});

// Обработка данных
engine.processRGBData(rgbData, size, width, height, timestamp);
engine.processIMUData(imuData);
engine.processLidarData(depthData, depthSize, confidenceData, confidenceSize, timestamp);

// Получение результатов
CameraPose pose = engine.getCurrentCameraPose();
```

## Структура

```
LensEngineSDK/
├── include/          # Заголовочные файлы
│   ├── LensEngineAPI.h
│   ├── LensEngineTypes.h
│   └── ...
├── src/              # Исходные файлы
│   ├── LensEngine.cpp
│   ├── SensorFusionEKF.cpp
│   └── ...
└── CMakeLists.txt
```

## Типы данных

- `RGB_CAMERA` (0x01) - RGB данные камеры
- `LIDAR_DEPTH` (0x02) - LiDAR данные
- `RAW_IMU` (0x03) - Сырые IMU данные
- `FEATURE_POINTS` (0x04) - Feature Points
- `CAMERA_INTRINSICS` (0x05) - Camera Intrinsics
- `LIGHT_ESTIMATION` (0x06) - Light Estimation

## API

Главный интерфейс - `LensEngineAPI`, который предоставляет:
- Обработку всех типов данных
- Получение результатов (поза камеры, feature points, etc.)
- Колбэки для асинхронной обработки
- Настройки фильтров и параметров

