# Инструкции по сборке ARLauncherProject

## 📋 Предварительные требования

### Windows

#### Обязательные:
- **Qt 5.15+ или Qt 6.x**
  - Скачать: https://www.qt.io/download
  - Установить в: `C:/Qt/5.15.2/` или `C:/Qt/6.x.x/`
  - Добавить в PATH: `C:/Qt/5.15.2/mingw81_64/bin` (или `msvc2019_64/bin`)
  
- **CMake 3.15+**
  - Скачать: https://cmake.org/download/
  - Установить и добавить в PATH
  
- **MinGW-w64 или MSVC 2019+**
  - MinGW: входит в Qt или установить отдельно
  - MSVC: Visual Studio 2019/2022

#### Библиотеки для SensorConnector:
- **libjpeg-turbo**
  - Скачать: https://github.com/libjpeg-turbo/libjpeg-turbo/releases
  - **Для Windows:** Выбрать `jpeg-turbo-2.1.5-x64.exe` (или последнюю версию)
  - Распаковать/установить в: `C:/libjpeg-turbo64/`
  - **Структура должна быть:**
    ```
    C:/libjpeg-turbo64/
    ├── include/
    │   └── turbojpeg.h          ← ОБЯЗАТЕЛЬНО
    ├── lib/
    │   ├── turbojpeg.lib        ← Для MSVC
    │   ├── libturbojpeg.a       ← Для MinGW
    │   └── turbojpeg.dll
    └── bin/
        └── turbojpeg.dll        ← Нужно скопировать в папку с .exe
    ```
  - **ВАЖНО:** После установки проверьте путь в `SensorConnector.pro` (строка 16)
  - **ИЛИ** использовать vcpkg:
    ```powershell
    vcpkg install libjpeg-turbo:x64-windows
    ```
    В этом случае путь будет автоматически настроен

- **FFmpeg**
  - Скачать: https://www.gyan.dev/ffmpeg/builds/ (Windows builds)
    - Выбрать "ffmpeg-release-full.7z" или "ffmpeg-release-essentials.zip"
  - Распаковать в: `C:/ffmpeg/`
  - **Структура должна быть:**
    ```
    C:/ffmpeg/
    ├── include/
    │   ├── libavcodec/
    │   │   └── avcodec.h        ← ОБЯЗАТЕЛЬНО
    │   ├── libavformat/
    │   │   └── avformat.h       ← ОБЯЗАТЕЛЬНО
    │   ├── libavutil/
    │   │   └── avutil.h         ← ОБЯЗАТЕЛЬНО
    │   ├── libswscale/
    │   │   └── swscale.h        ← ОБЯЗАТЕЛЬНО
    │   └── libswresample/
    │       └── swresample.h     ← ОБЯЗАТЕЛЬНО
    ├── lib/
    │   ├── avcodec.lib          ← Для MSVC
    │   ├── avcodec.a            ← Для MinGW
    │   ├── avformat.lib
    │   ├── avformat.a
    │   ├── avutil.lib
    │   ├── avutil.a
    │   ├── swscale.lib
    │   ├── swscale.a
    │   ├── swresample.lib
    │   └── swresample.a
    └── bin/
        ├── avcodec-xx.dll       ← Нужно скопировать в папку с .exe
        ├── avformat-xx.dll
        ├── avutil-xx.dll
        ├── swscale-xx.dll
        └── swresample-xx.dll
    ```
  - **ВАЖНО:** После установки проверьте путь в `SensorConnector.pro` (строка 22)
  - **ИЛИ** использовать vcpkg:
    ```powershell
    vcpkg install ffmpeg:x64-windows
    ```

#### Библиотеки для LensEngineSDK:
- **GLM** (header-only библиотека)
  - Скачать: https://github.com/g-truc/glm/releases
  - Распаковать в: `C:/glm/` или `C:/vcpkg/installed/x64-windows/include/`
  - **Структура должна быть:**
    ```
    C:/glm/
    └── glm/                    ← Важно: должна быть папка glm/
        ├── glm.hpp             ← ОБЯЗАТЕЛЬНО
        ├── vec3.hpp
        ├── vec4.hpp
        ├── mat4.hpp
        ├── quat.hpp
        └── ...
    ```
  - **ВАЖНО:** CMake будет искать `glm/glm.hpp`, поэтому путь должен быть `C:/glm/glm/glm.hpp`
  - **ИЛИ** через vcpkg (рекомендуется):
    ```powershell
    vcpkg install glm:x64-windows
    ```
    В этом случае путь будет: `C:/vcpkg/installed/x64-windows/include/glm/glm.hpp`

- **OpenCV** (опционально, для feature detection)
  - Скачать: https://opencv.org/releases/
  - Установить в: `C:/opencv/` или использовать vcpkg
  - **ИЛИ** через vcpkg:
    ```powershell
    vcpkg install opencv:x64-windows
    ```

#### Библиотеки для ARLauncher:
- **GLFW3**
  - Скачать: https://www.glfw.org/download.html
  - Выбрать "Windows pre-compiled binaries"
  - Распаковать в: `C:/glfw/`
  - **Структура должна быть:**
    ```
    C:/glfw/
    ├── include/
    │   └── GLFW/
    │       └── glfw3.h         ← ОБЯЗАТЕЛЬНО
    └── lib/
        ├── libglfw3.a          ← Для MinGW
        ├── glfw3.lib           ← Для MSVC
        └── glfw3.dll           ← Нужно скопировать в папку с .exe
    ```
  - **ВАЖНО:** Для MinGW нужен `.a` файл, для MSVC - `.lib`
  - **ИЛИ** через vcpkg (рекомендуется):
    ```powershell
    vcpkg install glfw3:x64-windows
    ```

- **OpenGL** (входит в Windows)
  - Обычно не требует установки
  - Для MinGW может потребоваться: `vcpkg install opengl:x64-windows`

- **Vulkan** (опционально, альтернатива OpenGL)
  - Скачать: https://vulkan.lunarg.com/
  - Установить Vulkan SDK

### Linux

#### Установка через apt (Ubuntu/Debian):

```bash
# Qt
sudo apt-get install qt5-default qtbase5-dev qtbase5-dev-tools

# CMake
sudo apt-get install cmake

# Библиотеки для SensorConnector
sudo apt-get install libturbojpeg0-dev libturbojpeg libjpeg-turbo8-dev
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev

# Библиотеки для LensEngineSDK
sudo apt-get install libglm-dev

# Библиотеки для ARLauncher
sudo apt-get install libglfw3-dev libgl1-mesa-dev

# OpenCV (опционально)
sudo apt-get install libopencv-dev
```

#### Альтернативно через vcpkg:

```bash
# Установить vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Установить библиотеки
./vcpkg install glfw3 glm opencv libjpeg-turbo ffmpeg
```

### macOS

#### Установка через Homebrew:

```bash
# Qt
brew install qt@5

# CMake
brew install cmake

# Библиотеки
brew install glfw glm libjpeg-turbo ffmpeg opencv
```

---

## 🔧 Настройка путей для Windows

### Вариант 1: Использование vcpkg (рекомендуется)

```powershell
# Установить vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Установить все библиотеки
.\vcpkg install glfw3:x64-windows
.\vcpkg install glm:x64-windows
.\vcpkg install libjpeg-turbo:x64-windows
.\vcpkg install ffmpeg:x64-windows
.\vcpkg install opencv:x64-windows

# Интегрировать с CMake
.\vcpkg integrate install
```

При сборке CMake автоматически найдет библиотеки:
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Вариант 2: Ручная установка

#### Настройка SensorConnector.pro:

Отредактируйте `SensorConnector/SensorConnector.pro`:

```qmake
# Для ваших путей
win32 {
    # Libjpeg-turbo
    LIBJPEG_TURBO_DIR = C:/libjpeg-turbo64
    INCLUDEPATH += $$LIBJPEG_TURBO_DIR/include
    LIBS += -L$$LIBJPEG_TURBO_DIR/lib
    LIBS += -lturbojpeg

    # FFmpeg
    FFMPEG_DIR = C:/ffmpeg
    INCLUDEPATH += $$FFMPEG_DIR/include
    LIBS += -L$$FFMPEG_DIR/lib
    LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample
}
```

#### Настройка CMakeLists.txt для LensEngineSDK:

Если библиотеки не в стандартных местах, укажите пути:

```bash
cmake .. \
    -DGLM_INCLUDE_DIR=C:/glm \
    -DOpenCV_DIR=C:/opencv/build
```

#### Настройка CMakeLists.txt для ARLauncher:

```bash
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/glfw/include \
    -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a \
    -DGLM_INCLUDE_DIR=C:/glm \
    -DUSE_OPENGL=ON
```

---

## 📦 Сборка компонентов

### 1. SensorConnector (Qt)

#### Windows (MinGW):

```powershell
cd SensorConnector
qmake SensorConnector.pro
mingw32-make
```

**Проверка путей:**
- Убедитесь, что `LIBJPEG_TURBO_DIR` и `FFMPEG_DIR` в `.pro` файле указывают на правильные пути
- Если пути другие, отредактируйте `SensorConnector.pro`

#### Windows (MSVC):

```powershell
cd SensorConnector
qmake SensorConnector.pro -spec win32-msvc
nmake
```

#### Linux:

```bash
cd SensorConnector
qmake SensorConnector.pro
make
```

### 2. LensEngineSDK (CMake)

#### Windows:

```powershell
cd LensEngineSDK
mkdir build
cd build

# С vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# ИЛИ без vcpkg (с указанием путей)
cmake .. -DGLM_INCLUDE_DIR=C:/glm

# С OpenCV (опционально)
cmake .. -DLENSENGINE_USE_OPENCV=ON -DOpenCV_DIR=C:/opencv/build

mingw32-make
```

#### Linux:

```bash
cd LensEngineSDK
mkdir build && cd build
cmake .. -DLENSENGINE_USE_OPENCV=ON
make
```

### 3. ARLauncher (CMake)

#### Windows:

```powershell
cd ARLauncher
mkdir build
cd build

# С vcpkg
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DUSE_OPENGL=ON

# ИЛИ без vcpkg (с указанием путей)
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/glfw/include \
    -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a \
    -DGLM_INCLUDE_DIR=C:/glm \
    -DUSE_OPENGL=ON

mingw32-make
```

#### Linux:

```bash
cd ARLauncher
mkdir build && cd build
cmake .. -DUSE_OPENGL=ON
make
```

---

## 🚀 Полная сборка проекта

### Windows (PowerShell) - с vcpkg:

```powershell
# Установить vcpkg зависимости
cd C:\vcpkg
.\vcpkg install glfw3:x64-windows glm:x64-windows libjpeg-turbo:x64-windows ffmpeg:x64-windows opencv:x64-windows

# SensorConnector
cd C:\Users\vboxuser\Documents\GitHub\ARLauncherProject\SensorConnector
qmake SensorConnector.pro
mingw32-make

# LensEngineSDK
cd ..\LensEngineSDK
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
mingw32-make

# ARLauncher
cd ..\..\ARLauncher
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DUSE_OPENGL=ON
mingw32-make
```

### Windows (PowerShell) - без vcpkg (ручная установка):

```powershell
# Установить библиотеки вручную:
# - libjpeg-turbo в C:/libjpeg-turbo64/
# - FFmpeg в C:/ffmpeg/
# - GLM в C:/glm/
# - GLFW в C:/glfw/

# SensorConnector
cd SensorConnector
# Отредактировать SensorConnector.pro с правильными путями
qmake SensorConnector.pro
mingw32-make

# LensEngineSDK
cd ..\LensEngineSDK
mkdir build
cd build
cmake .. -DGLM_INCLUDE_DIR=C:/glm
mingw32-make

# ARLauncher
cd ..\..\ARLauncher
mkdir build
cd build
cmake .. -DGLFW_INCLUDE_DIR=C:/glfw/include -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a -DGLM_INCLUDE_DIR=C:/glm -DUSE_OPENGL=ON
mingw32-make
```

### Linux:

```bash
# Установить все зависимости (см. выше)

# SensorConnector
cd SensorConnector
qmake SensorConnector.pro
make

# LensEngineSDK
cd ../LensEngineSDK
mkdir build && cd build
cmake ..
make

# ARLauncher
cd ../../ARLauncher
mkdir build && cd build
cmake .. -DUSE_OPENGL=ON
make
```

---

## 📋 Проверка зависимостей

### SensorConnector

**Проверить наличие библиотек:**

```powershell
# Windows
dir C:\libjpeg-turbo64\include\turbojpeg.h
dir C:\ffmpeg\include\libavcodec\avcodec.h
```

**Если библиотеки не найдены:**
1. Проверьте пути в `SensorConnector.pro`
2. Убедитесь, что DLL файлы скопированы в папку с исполняемым файлом
3. Для FFmpeg DLL должны быть в PATH или рядом с .exe

### LensEngineSDK

**Проверить GLM:**

```bash
# Windows
dir C:\glm\glm\glm.hpp

# Linux
ls /usr/include/glm/glm.hpp
```

**Проверить OpenCV:**

```bash
# Windows
cmake .. -DOpenCV_DIR=C:/opencv/build

# Linux
pkg-config --modversion opencv4
```

### ARLauncher

**Проверить GLFW:**

```bash
# Windows
dir C:\glfw\include\GLFW\glfw3.h

# Linux
ls /usr/include/GLFW/glfw3.h
```

**Проверить OpenGL:**

```bash
# Windows - обычно уже установлен
# Linux
sudo apt-get install mesa-common-dev libgl1-mesa-dev
```

---

## 🔧 Решение проблем

### Проблема: "libjpeg-turbo not found"

**Решение:**
1. Установите libjpeg-turbo в `C:/libjpeg-turbo64/`
2. Или отредактируйте `SensorConnector.pro`:
   ```qmake
   LIBJPEG_TURBO_DIR = C:/ваш/путь/к/libjpeg-turbo64
   ```

### Проблема: "FFmpeg not found"

**Решение:**
1. Установите FFmpeg в `C:/ffmpeg/`
2. Или отредактируйте `SensorConnector.pro`:
   ```qmake
   FFMPEG_DIR = C:/ваш/путь/к/ffmpeg
   ```
3. Добавьте FFmpeg DLL в PATH:
   ```powershell
   $env:PATH += ";C:\ffmpeg\bin"
   ```

### Проблема: "GLFW not found" в CMake

**Решение:**
```bash
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/glfw/include \
    -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a
```

Или используйте vcpkg:
```powershell
vcpkg install glfw3:x64-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Проблема: "GLM not found"

**Решение:**
```bash
cmake .. -DGLM_INCLUDE_DIR=C:/glm
```

Или используйте vcpkg:
```powershell
vcpkg install glm:x64-windows
```

### Проблема: DLL не найдены при запуске

**Решение:**
1. Скопируйте DLL в папку с .exe:
   ```powershell
   # Для SensorConnector
   copy C:\libjpeg-turbo64\bin\turbojpeg.dll SensorConnector\lib\
   copy C:\ffmpeg\bin\*.dll SensorConnector\lib\
   ```

2. Или добавьте пути к DLL в PATH:
   ```powershell
   $env:PATH += ";C:\libjpeg-turbo64\bin;C:\ffmpeg\bin"
   ```

### Проблема: Qt не найден

**Решение:**
```powershell
# Добавить Qt в PATH
$env:PATH += ";C:\Qt\5.15.2\mingw81_64\bin"

# Или использовать полный путь к qmake
C:\Qt\5.15.2\mingw81_64\bin\qmake.exe SensorConnector.pro
```

---

## 📦 Рекомендуемая структура установки (Windows)

```
C:/
├── Qt/
│   └── 5.15.2/
│       └── mingw81_64/
├── libjpeg-turbo64/
│   ├── include/
│   ├── lib/
│   └── bin/
├── ffmpeg/
│   ├── include/
│   ├── lib/
│   └── bin/
├── glm/
│   └── glm/
├── glfw/
│   ├── include/
│   └── lib/
├── opencv/ (опционально)
│   └── build/
└── vcpkg/ (альтернатива)
    └── installed/
        └── x64-windows/
```

---

## ✅ Проверка сборки

### SensorConnector

```powershell
cd SensorConnector\lib
.\SensorConnector.exe
# Должен запуститься без ошибок
```

### LensEngineSDK

```powershell
cd LensEngineSDK\build\lib
dir LensEngineSDK.a  # или LensEngineSDK.lib для MSVC
```

### ARLauncher

```powershell
cd ARLauncher\build\bin
.\ARLauncher.exe
# Должно открыться окно
```

---

## 📚 Дополнительные ресурсы

- **[DEPENDENCIES_SETUP.md](DEPENDENCIES_SETUP.md)** - **Детальное руководство по установке всех зависимостей**
- **[LIBRARIES_SUMMARY.md](LIBRARIES_SUMMARY.md)** - **Краткая сводка библиотек и их расположение**
- [BUILD_CHECKLIST.md](BUILD_CHECKLIST.md) - Чек-лист для проверки перед сборкой
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) - Руководство по интеграции
- [QUICK_START.md](QUICK_START.md) - Быстрый старт
- [COMPONENTS_API.md](COMPONENTS_API.md) - API компонентов

---

## ⚠️ Важные замечания

1. **Все пути указаны для Windows** - для Linux/macOS используйте стандартные пути пакетного менеджера
2. **Используйте одну архитектуру** (x64) для всех библиотек
3. **Проверяйте версии** - некоторые библиотеки требуют совместимых версий
4. **DLL должны быть доступны** - либо в PATH, либо рядом с .exe файлом
5. **vcpkg - самый простой способ** для Windows - автоматически настраивает все пути

---

**Рекомендация:** Для первого раза используйте vcpkg - это самый надежный способ установки всех зависимостей. См. [DEPENDENCIES_SETUP.md](DEPENDENCIES_SETUP.md) для детальных инструкций.
