# Карта файлов - Детальное описание переноса

## 📋 ФАЗА 1: SensorConnector

### Перенесенные файлы из ARReceiverQt → SensorConnector

| Исходный файл | Новый файл | Назначение | Статус |
|---------------|-----------|------------|--------|
| `usbmanager.h/cpp` | `include/UsbManager.h`<br>`src/UsbManager.cpp` | USB Ethernet подключение | ✅ |
| `NetworkServer.h/cpp` | `include/NetworkServerSimplified.h`<br>`src/NetworkServerSimplified.cpp` | TCP/UDP серверы (упрощенная версия) | ✅ |
| `networkconfigurator.h/cpp` | `include/NetworkConfigurator.h`<br>`src/NetworkConfigurator.cpp` | Конфигурация USB сети | ✅ |
| `tcpserver.h/cpp` | `include/TcpServer.h`<br>`src/TcpServer.cpp` | Базовый TCP сервер | ✅ |
| `turbojpegdecoder.h/cpp` | `include/TurboJPEGDecoder.h`<br>`src/TurboJPEGDecoder.cpp` | JPEG декодер (libjpeg-turbo) | ✅ |
| `ffmpegdecoder.h/cpp` | `include/FFmpegDecoder.h`<br>`src/FFmpegDecoder.cpp` | H.264 декодер (FFmpeg) | ✅ |
| `fastjpegdecoder.h/cpp` | `include/FastJPEGDecoder.h`<br>`src/FastJPEGDecoder.cpp` | Быстрый JPEG декодер | ✅ |

### Новые файлы

| Файл | Назначение | Статус |
|------|------------|--------|
| `include/SensorConnector.h` | Главный класс компонента | ✅ |
| `include/SensorDataTypes.h` | Типы данных для SensorConnector | ✅ |
| `src/SensorConnector.cpp` | Реализация главного класса | ✅ |
| `SensorConnector.pro` | QMake файл для сборки | ✅ |
| `README.md` | Документация | ✅ |

### Удаленные зависимости

- ❌ `ARCameraController` - перенесен в LensEngineSDK
- ❌ `ARDataProcessor` - перенесен в LensEngineSDK
- ❌ `Lidar3DProcessor` - перенесен в LensEngineSDK
- ❌ `lensenginetypes.h` (Qt версия) - заменен на чистый C++ в LensEngineSDK

---

## 📋 ФАЗА 2: LensEngineSDK

### Созданные файлы (новые, без Qt)

| Файл | Назначение | Замены Qt → C++ | Статус |
|------|------------|----------------|--------|
| `include/LensEngineTypes.h` | Типы данных | QVector3D→glm::vec3<br>QQuaternion→glm::quat<br>QMatrix4x4→glm::mat4<br>QVector→std::vector<br>QImage→RGBImage | ✅ |
| `include/LensEngineAPI.h` | Чистый C++ API | Qt сигналы→std::function | ✅ |
| `include/LensEngine.h` | Главный класс движка | QObject→чистый C++ | ✅ |
| `include/SensorFusionEKF.h` | EKF фильтр | QVector→std::vector<br>QMutex→std::mutex<br>QElapsedTimer→std::chrono | ✅ |
| `include/Lidar3DProcessor.h` | Обработчик LiDAR | QVector→std::vector<br>QObject→чистый C++ | ✅ |
| `include/CameraController.h` | Контроллер камеры | QVector3D→glm::vec3<br>QQuaternion→glm::quat | ✅ |
| `include/ARDataProcessor.h` | Процессор AR данных | QVector→std::vector<br>Qt сигналы→std::function | ✅ |
| `include/SpatialMappingSystem.h` | Пространственное маппирование | QVector→std::vector<br>QMap→std::map | ✅ |
| `src/LensEngineAPI.cpp` | Реализация API | ✅ |
| `src/LensEngine.cpp` | Реализация движка | ✅ |
| `src/SensorFusionEKF.cpp` | Реализация EKF | ✅ |
| `src/Lidar3DProcessor.cpp` | Реализация LiDAR | ✅ |
| `src/CameraController.cpp` | Реализация камеры | ✅ |
| `src/ARDataProcessor.cpp` | Реализация процессора | ✅ |
| `src/SpatialMappingSystem.cpp` | Реализация маппинга | ✅ |
| `CMakeLists.txt` | Система сборки CMake | ✅ |
| `README.md` | Документация | ✅ |
| `PROGRESS.md` | Прогресс разработки | ✅ |

### Оригинальные файлы (для справки)

| Оригинальный файл | Статус переноса | Заметки |
|-------------------|----------------|---------|
| `sensorfusion_ekf.h/cpp` | ✅ Перенесен | Полная замена Qt на std/glm |
| `lidar3dprocessor.h/cpp` | ✅ Перенесен | Полная замена Qt на std/glm |
| `arcameracontroller.h/cpp` | ✅ Перенесен | Полная замена Qt на glm |
| `ardataprocessor.h/cpp` | ✅ Перенесен | Полная замена Qt на std/glm |
| `spatialmappingsystem.h/cpp` | ✅ Перенесен | Полная замена Qt на std/glm |
| `lensenginetypes.h` | ✅ Пересоздан | Полная замена Qt типов |
| `ARDataTypes.h` | ⏳ Не требуется | Заменен на LensEngineTypes.h |

---

## 📋 ФАЗА 3: ARLauncher

### Созданные файлы (новые)

| Файл | Назначение | Статус |
|------|------------|--------|
| `include/Application.h` | Главный класс приложения | ✅ |
| `include/Renderer.h` | Базовый класс рендерера | ✅ |
| `include/Scene.h` | 3D сцена | ✅ |
| `include/Camera.h` | Виртуальная камера | ✅ |
| `include/ARLauncher.h` | Главный заголовочный файл | ✅ |
| `ui/UIRenderer.h` | Рендерер UI | ✅ |
| `ui/UIElement.h` | Базовый элемент UI | ✅ |
| `ui/Button.h` | Кнопка | ✅ |
| `ui/Text.h` | Текст | ✅ |
| `ui/Input.h` | Поле ввода | ✅ |
| `ui/Window.h` | 3D окно | ✅ |
| `ui/Widget.h` | Виджет-контейнер | ✅ |
| `ui/Style.h` | Стилизация | ✅ |
| `src/main.cpp` | Точка входа | ✅ |
| `src/Application.cpp` | Реализация приложения | ✅ |
| `src/Renderer.cpp` | Реализация рендерера | ✅ |
| `src/Scene.cpp` | Реализация сцены | ✅ |
| `src/Camera.cpp` | Реализация камеры | ✅ |
| `ui/UIRenderer.cpp` | Реализация UI рендерера | ✅ |
| `ui/UIElement.cpp` | Реализация элемента | ✅ |
| `ui/Button.cpp` | Реализация кнопки | ✅ |
| `ui/Text.cpp` | Реализация текста | ✅ |
| `ui/Input.cpp` | Реализация ввода | ✅ |
| `ui/Window.cpp` | Реализация окна | ✅ |
| `ui/Widget.cpp` | Реализация виджета | ✅ |
| `CMakeLists.txt` | Система сборки | ✅ |
| `README.md` | Документация | ✅ |

### Не используются (из оригинального проекта)

| Файл | Причина |
|------|---------|
| `mainwindow.h/cpp` | Qt QML - заменен на Application |
| `qml/*.qml` | QML файлы - заменены на Custom UI |
| `widget3d*.h/cpp` | Qt3D - заменены на Renderer |
| `videotexturemanager.h/cpp` | Qt3D - заменен на Renderer |
| `pointcloudgeometry.h/cpp` | Qt3D - заменен на Renderer |
| `frameprovider.h/cpp` | QML ImageProvider - не нужен |
| `lidarframeprovider.h/cpp` | QML ImageProvider - не нужен |

---

## 📊 Статистика

### Файлы созданы
- SensorConnector: 18 файлов
- LensEngineSDK: 17 файлов
- ARLauncher: 24 файла
- Документация: 5 файлов
- **Всего: 64+ файла**

### Строки кода (приблизительно)
- SensorConnector: ~3000 строк
- LensEngineSDK: ~4000 строк
- ARLauncher: ~2500 строк
- **Всего: ~9500 строк**

### Удалено Qt зависимостей
- LensEngineSDK: 100% (полностью чистый C++)
- SensorConnector: Минимум (только для сетевых операций)
- ARLauncher: 100% (Native C++)

---

## 🔄 Схема переноса

```
ARReceiverQt (монолитный)
│
├─→ SensorConnector/ (Qt)
│   ├─ USB/WiFi код
│   └─ Декодеры
│
├─→ LensEngineSDK/ (Чистый C++)
│   ├─ AR обработка (без Qt)
│   ├─ EKF фильтр (без Qt)
│   └─ Пространственное маппирование (без Qt)
│
└─→ ARLauncher/ (Native C++)
    ├─ Vulkan/OpenGL рендерер
    ├─ Custom UI (не ImGui)
    └─ 3D сцена
```

---

**Последнее обновление:** $(date)

