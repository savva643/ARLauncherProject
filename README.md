# ARLauncher Project

AR лаунчер для операционной системы AR - проект для получения и обработки данных с iPhone камеры и сенсоров, с отображением 3D сцены поверх реального мира.

## ⚡ Новый конвейер (GlaskiOS streaming v2)

- **Рендер — только Vulkan.** OpenGL — LEGACY, не развивается.
- **SensorStream** — приём iPhone без Qt, H.264/HEVC → NV12 → GPU. Старый Qt SensorConnector — LEGACY.
- **LensEngine::TrackingSession** — трекинг головы по сырому IMU с предсказанием; 6DoF через OpenVINS.
- **Late latching + репроекция видео** — мир стоит на месте при поворотах головы.
- **LiDAR** — позиция 6DoF без дрейфа, окклюзия (стены закрывают объекты), пол и тени; свет и отражения по комнате; метки ArUco; пространственный звук.
- **GlaskiOS Control** — окно сборки и запуска со всеми переключателями: `python3 tools/glaski_control.py`.

Как собрать, запустить и проверить без iPhone: [docs/GLASKI_STREAMING.md](docs/GLASKI_STREAMING.md).
Протокол: [docs/PROTOCOL_V2.md](docs/PROTOCOL_V2.md).
Что работает, а чего ещё нет (видео, IMU, LiDAR, тени, свет, звук, метки): [docs/FEATURE_STATUS.md](docs/FEATURE_STATUS.md).

```bash
./build_arlauncher.sh --vio && cd ARLauncher/build_vk && ./bin/ARLauncher
```

## 📋 Описание

ARLauncher - это нативный C++ лаунчер с 3D сценой для AR приложений. Проект состоит из трех независимых компонентов:

1. **SensorConnector** - получение данных с iPhone через USB/WiFi
2. **LensEngineSDK** - AR движок для обработки данных и вычисления позы камеры
3. **ARLauncher** - лаунчер с 3D рендерингом и UI

## 🏗️ Структура проекта

```
ARLauncherProject/
│
├── SensorConnector/              # Компонент 1: Qt эмулятор датчиков
│   ├── include/                   # Заголовочные файлы
│   │   ├── SensorConnector.h
│   │   ├── SensorDataTypes.h
│   │   ├── NetworkServer.h
│   │   ├── UsbManager.h
│   │   ├── TurboJPEGDecoder.h
│   │   └── FFmpegDecoder.h
│   ├── src/                      # Исходные файлы
│   ├── examples/                  # Примеры использования
│   ├── SensorConnector.pro        # QMake файл
│   └── README.md
│
├── LensEngineSDK/                # Компонент 2: Чистый C++ AR движок
│   ├── include/                   # Заголовочные файлы
│   │   ├── LensEngineAPI.h
│   │   ├── LensEngineTypes.h
│   │   ├── SensorFusionEKF.h
│   │   ├── Lidar3DProcessor.h
│   │   └── CameraController.h
│   ├── src/                      # Исходные файлы
│   ├── examples/                  # Примеры использования
│   ├── CMakeLists.txt
│   └── README.md
│
└── ARLauncher/                    # Компонент 3: Native C++ лаунчер
    ├── include/                   # Заголовочные файлы
    │   ├── Application.h
    │   ├── Renderer.h
    │   ├── Scene.h
    │   ├── Camera.h
    │   └── FontRenderer.h
    ├── src/                       # Исходные файлы
    │   ├── main.cpp
    │   ├── Application.cpp
    │   ├── Renderer.cpp
    │   └── Scene.cpp
    ├── ui/                        # Custom UI система
    │   ├── UIRenderer.h
    │   ├── UIElement.h
    │   ├── Button.h
    │   ├── Text.h
    │   └── Style.h
    ├── shaders/                   # GLSL шейдеры
    │   ├── basic.vert/frag
    │   ├── video.vert/frag
    │   └── ui.vert/frag
    ├── CMakeLists.txt
    └── README.md
```

## 🔧 Установка зависимостей (Ubuntu 22.04/24.04)

```bash
sudo apt update
sudo apt install -y cmake build-essential pkg-config \
  libglfw3-dev libglm-dev \
  libvulkan-dev vulkan-tools vulkan-validationlayers vulkan-utility-libraries-dev \
  libturbojpeg-dev libjpeg-turbo8-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev \
  libfreetype6-dev \
  ipheth-utils usbmuxd libimobiledevice6 libimobiledevice-utils
# optional shader tools
sudo apt install -y glslang-tools spirv-tools libshaderc-dev
```

**Важно:** `libfreetype6-dev` необходим для нормального отображения текста в UI (без него текст будет отображаться как квадратики).

### Qt 6

Установите Qt 6 через Qt Maintenance Tool (пример пути): `/home/savva/Qt/6.5.3/gcc_64`

## 🚀 Сборка проекта

### Автоматическая сборка (рекомендуется)

Используйте скрипт `build_linux.sh` для автоматической сборки всех компонентов:

```bash
./build_linux.sh
```

Скрипт автоматически:
1. Соберет SensorConnector (Qt6)
2. Соберет LensEngineSDK (CMake)
3. Соберет ARLauncher (CMake с OpenGL и SensorConnector)

### Ручная сборка (по шагам)

Если нужно собрать компоненты вручную:

#### 1. Сборка SensorConnector (Qt6)

```bash
cd SensorConnector
/home/savva/Qt/6.5.3/gcc_64/bin/qmake6 SensorConnector.pro CONFIG+=release
make -j$(nproc)
```

#### 2. Сборка LensEngineSDK (CMake)

```bash
cd LensEngineSDK
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

#### 3. Сборка ARLauncher (CMake)

```bash
cd ARLauncher
rm -rf build && mkdir build && cd build
cmake -S .. \
      -B . \
      -DUSE_OPENGL=ON \
      -DUSE_SENSOR_CONNECTOR=ON \
      -DSENSOR_CONNECTOR_LIB=../../SensorConnector/lib/libSensorConnector.a \
      -DLENS_ENGINE_SDK_DIR=../../LensEngineSDK \
      -DQt6_DIR=/home/savva/Qt/6.5.3/gcc_64/lib/cmake/Qt6
cmake --build . -j$(nproc)
```

### Быстрая проверка Vulkan (без SensorConnector)

Только чтобы проверить:
- Vulkan рендер (видео можно игнорировать)
- 3D сцена (фигуры)
- UI (текст/кнопки) через FreeType

```bash
cd ARLauncher
rm -rf build_vk && mkdir build_vk
cmake -S . -B build_vk \
      -DUSE_VULKAN=ON \
      -DUSE_OPENGL=OFF \
      -DUSE_SENSOR_CONNECTOR=OFF \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build build_vk -j$(nproc)
```

## 📱 Настройка iPhone USB Tethering

1. **В VM (если используется):** VM → Removable Devices → Apple iPhone → Connect (Disconnect from Host). USB Controller установлен на USB 3.0/3.1.

2. **На iPhone:** Settings → Personal Hotspot → включить (держать экран открытым) и Trust this Computer.

3. **Проверка и настройка интерфейса:**

```bash
sudo systemctl status usbmuxd
lsmod | grep ipheth || sudo modprobe ipheth
lsusb | grep -i apple
nmcli device status            # найти iPhone интерфейс (enx...)
IF=<IPHONE_IFACE>
nmcli device set $IF managed yes
nmcli con add type ethernet ifname $IF con-name iPhoneUSB ipv4.method auto ipv6.method ignore connection.autoconnect yes
nmcli con up iPhoneUSB
ip addr show $IF
ping -c1 172.20.10.1           # обычно iPhone на 172.20.10.1
```

**Примечание:** Хост обычно получает 172.20.10.2. Не хардкодить 172.20.10.3.

**Альтернативы:** если USB–Ethernet не появляется, используйте `iproxy 9000 9000` (usbmuxd port forwarding) или Wi‑Fi (слушать на 0.0.0.0:9000).

## ▶️ Запуск

```bash
./ARLauncher/build/bin/ARLauncher
```

Если собирал Vulkan-only в `build_vk`, важно запускать из build-директории, чтобы подхватились `shaders_vk/*.spv`:

```bash
cd ARLauncher/build_vk
./bin/ARLauncher
```

Опциональная валидация Vulkan:
```bash
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./ARLauncher/build/bin/ARLauncher
```

Для Vulkan-only сборки:

```bash
cd ARLauncher/build_vk
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./bin/ARLauncher
```

## ✨ Ожидаемое поведение

- OpenGL/Vulkan рендерер инициализирован; размер swapchain выводится; демо сцена создана
- SensorConnector серверы на TCP/UDP 9000; когда кадры приходят с iPhone, они появляются как фон
- Поза камеры синхронизирована через LensEngine
- 3D объекты отображаются поверх видео фона
- UI элементы (кнопки, текст) отображаются корректно

## 🏛️ Архитектура

```
iPhone (Источник данных)
    ↓ USB / WiFi (TCP/UDP)
SensorConnector (Qt)
    ↓ Сырые данные (RGB, IMU, LiDAR)
LensEngineSDK (Pure C++)
    ↓ CameraPose (6DoF поза камеры)
ARLauncher (Native C++)
    ↓ Рендеринг на экран
```

## 📚 Компоненты

### SensorConnector
- Получение данных от iPhone через USB/WiFi
- Декодирование JPEG/H.264
- Передача сырых данных дальше
- **Технологии:** Qt, libjpeg-turbo, FFmpeg

### LensEngineSDK
- Обработка RGB кадров и IMU данных
- Вычисление 6DoF позы камеры (Sensor Fusion EKF)
- Обработка LiDAR данных
- Управление виртуальной камерой
- **Технологии:** Pure C++, GLM

### ARLauncher
- OpenGL/Vulkan рендеринг
- 3D сцена с виртуальными объектами
- Custom UI система (не ImGui)
- Синхронизация виртуальной камеры с реальной
- Видео фон с камеры iPhone
- **Технологии:** GLFW, OpenGL/Vulkan, GLM, FreeType

## 🎨 Особенности

- **3D рендеринг:** OpenGL или Vulkan
- **Custom UI:** собственная система UI элементов (Button, Text, Input, Window)
- **AR синхронизация:** виртуальная камера синхронизирована с реальной через LensEngine
- **Видео фон:** поток с камеры iPhone отображается как фон
- **Текст:** поддержка FreeType для нормального отображения текста
- **Шейдеры:** GLSL шейдеры для рендеринга (basic, video, ui, glassmorphism)

## 🔍 Отладка

Если текст отображается как квадратики:
- Убедитесь, что установлен `libfreetype6-dev`
- Пересоберите проект после установки FreeType

Если черный экран при запуске:
- Проверьте, что OpenGL контекст создан правильно
- Проверьте логи в консоли на наличие ошибок инициализации

## 📝 Лицензия

[Указать лицензию проекта]

## 👤 Автор

[Указать автора проекта]
