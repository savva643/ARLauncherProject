# SensorConnector

## Описание

SensorConnector - это Qt компонент для получения данных с iPhone через USB и WiFi.

## Поддерживаемые типы данных

- **0x01**: RGB данные камеры
- **0x02**: LiDAR данные
- **0x03**: Сырые IMU данные
- **0x04**: Feature Points (в разработке)
- **0x05**: Camera Intrinsics (в разработке)
- **0x06**: Light Estimation (в разработке)

## Подключения

- **USB**: Ethernet через USB (Apple Mobile Device Ethernet)
- **WiFi**: TCP/UDP серверы

## Декодеры

- **TurboJPEG**: libjpeg-turbo (быстрое декодирование JPEG)
- **FFmpeg**: H.264 декодирование
- **FastJPEG**: QImage декодирование (fallback)

## Сборка

```bash
qmake SensorConnector.pro
make
```

## Зависимости

- Qt 5/6 (core, network, concurrent)
- libjpeg-turbo
- FFmpeg (libavcodec, libavformat, libavutil, libswscale)

## Использование

```cpp
#include "SensorConnector.h"

using namespace SensorConnector;

SensorConnectorCore connector;
connector.initialize();
connector.startServers(9000, 9000);

connect(&connector, &SensorConnectorCore::dataReceived,
        [](const SensorData &data) {
    // Обработка данных
});
```

## Структура

```
SensorConnector/
├── include/          # Заголовочные файлы
├── src/              # Исходные файлы
└── SensorConnector.pro
```

