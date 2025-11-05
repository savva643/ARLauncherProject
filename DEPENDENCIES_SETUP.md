# Настройка зависимостей - Детальное руководство

## 📦 Полный список зависимостей

### SensorConnector

| Библиотека | Версия | Назначение | Где взять |
|------------|--------|------------|-----------|
| **Qt** | 5.15+ или 6.x | Фреймворк для сетевых операций | https://www.qt.io/download |
| **libjpeg-turbo** | Latest | JPEG декодирование | https://github.com/libjpeg-turbo/libjpeg-turbo/releases |
| **FFmpeg** | Latest | H.264 декодирование | https://ffmpeg.org/download.html |

### LensEngineSDK

| Библиотека | Версия | Назначение | Где взять |
|------------|--------|------------|-----------|
| **GLM** | Latest | Математика (векторы, матрицы) | https://github.com/g-truc/glm/releases |
| **OpenCV** | 4.x+ | Feature detection (опционально) | https://opencv.org/releases/ |

### ARLauncher

| Библиотека | Версия | Назначение | Где взять |
|------------|--------|------------|-----------|
| **GLFW3** | 3.3+ | Создание окна и обработка ввода | https://www.glfw.org/download.html |
| **GLM** | Latest | Математика (векторы, матрицы) | https://github.com/g-truc/glm/releases |
| **OpenGL** | - | Рендеринг (встроен в Windows/Linux) | - |
| **Vulkan** | Latest | Альтернативный рендерер (опционально) | https://vulkan.lunarg.com/ |

---

## 🪟 Windows - Пошаговая установка

### Способ 1: vcpkg (РЕКОМЕНДУЕТСЯ)

#### Шаг 1: Установка vcpkg

```powershell
# Клонировать vcpkg
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Запустить bootstrap
.\bootstrap-vcpkg.bat

# Интегрировать с Visual Studio (если используете)
.\vcpkg integrate install
```

#### Шаг 2: Установка всех библиотек

```powershell
# Перейти в vcpkg
cd C:\vcpkg

# Установить все необходимые библиотеки
.\vcpkg install glfw3:x64-windows
.\vcpkg install glm:x64-windows
.\vcpkg install libjpeg-turbo:x64-windows
.\vcpkg install ffmpeg:x64-windows
.\vcpkg install opencv:x64-windows
```

**Результат:** Библиотеки будут установлены в `C:\vcpkg\installed\x64-windows\`

#### Шаг 3: Использование в CMake

```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

### Способ 2: Ручная установка

#### Шаг 1: Установка Qt

1. Скачать Qt: https://www.qt.io/download
2. Установить в `C:\Qt\5.15.2\` (или другую версию)
3. Добавить в PATH:
   ```powershell
   $env:PATH += ";C:\Qt\5.15.2\mingw81_64\bin"
   ```

#### Шаг 2: Установка libjpeg-turbo

1. Скачать: https://github.com/libjpeg-turbo/libjpeg-turbo/releases
2. Выбрать версию для Windows (например, `jpeg-turbo-2.1.5-x64.exe`)
3. Установить в `C:\libjpeg-turbo64\`
4. **Проверить структуру:**
   ```
   C:\libjpeg-turbo64\
   ├── include\
   │   └── turbojpeg.h
   ├── lib\
   │   └── turbojpeg.lib
   └── bin\
       └── turbojpeg.dll
   ```

#### Шаг 3: Установка FFmpeg

1. Скачать: https://www.gyan.dev/ffmpeg/builds/ (Windows builds)
2. Распаковать в `C:\ffmpeg\`
3. **Проверить структуру:**
   ```
   C:\ffmpeg\
   ├── include\
   │   ├── libavcodec\
   │   ├── libavformat\
   │   ├── libavutil\
   │   └── libswscale\
   ├── lib\
   │   ├── avcodec.lib
   │   ├── avformat.lib
   │   ├── avutil.lib
   │   └── swscale.lib
   └── bin\
       └── *.dll
   ```

#### Шаг 4: Установка GLM

1. Скачать: https://github.com/g-truc/glm/releases
2. Распаковать в `C:\glm\`
3. **Проверить структуру:**
   ```
   C:\glm\
   └── glm\
       ├── glm.hpp
       ├── vec3.hpp
       └── ...
   ```

#### Шаг 5: Установка GLFW

1. Скачать: https://www.glfw.org/download.html
2. Выбрать "Windows pre-compiled binaries"
3. Распаковать в `C:\glfw\`
4. **Проверить структуру:**
   ```
   C:\glfw\
   ├── include\
   │   └── GLFW\
   │       └── glfw3.h
   └── lib\
       └── libglfw3.a (для MinGW) или glfw3.lib (для MSVC)
   ```

#### Шаг 6: Установка OpenCV (опционально)

1. Скачать: https://opencv.org/releases/
2. Распаковать в `C:\opencv\`
3. Собрать или использовать pre-built версию

---

## 🐧 Linux - Пошаговая установка

### Ubuntu/Debian:

```bash
# Обновить пакеты
sudo apt-get update

# Qt
sudo apt-get install qt5-default qtbase5-dev qtbase5-dev-tools

# CMake
sudo apt-get install cmake build-essential

# SensorConnector зависимости
sudo apt-get install libturbojpeg0-dev libturbojpeg libjpeg-turbo8-dev
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev

# LensEngineSDK зависимости
sudo apt-get install libglm-dev

# ARLauncher зависимости
sudo apt-get install libglfw3-dev libgl1-mesa-dev

# OpenCV (опционально)
sudo apt-get install libopencv-dev libopencv-contrib-dev

# Проверка установки
pkg-config --modversion glfw3
pkg-config --modversion opencv4
```

### Fedora/RHEL:

```bash
sudo dnf install qt5-qtbase-devel cmake gcc-c++
sudo dnf install turbojpeg-devel ffmpeg-devel
sudo dnf install glm-devel glfw-devel
sudo dnf install opencv-devel
```

---

## 🍎 macOS - Пошаговая установка

```bash
# Установить Homebrew (если еще не установлен)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Установить все зависимости
brew install qt@5
brew install cmake
brew install glfw glm libjpeg-turbo ffmpeg opencv

# Проверка
brew list glfw glm
```

---

## 🔧 Настройка путей в проекте

### SensorConnector.pro

**Если библиотеки установлены не в стандартных местах:**

```qmake
win32 {
    # Ваши пути
    LIBJPEG_TURBO_DIR = C:/ваш/путь/к/libjpeg-turbo64
    FFMPEG_DIR = C:/ваш/путь/к/ffmpeg
    
    INCLUDEPATH += $$LIBJPEG_TURBO_DIR/include
    INCLUDEPATH += $$FFMPEG_DIR/include
    
    LIBS += -L$$LIBJPEG_TURBO_DIR/lib
    LIBS += -L$$FFMPEG_DIR/lib
    LIBS += -lturbojpeg
    LIBS += -lavcodec -lavformat -lavutil -lswscale -lswresample
}
```

### CMakeLists.txt (LensEngineSDK)

**Если GLM не найден автоматически:**

```cmake
# Указать путь вручную
set(GLM_INCLUDE_DIR "C:/glm")
include_directories(${GLM_INCLUDE_DIR})
```

**Или при вызове cmake:**

```bash
cmake .. -DGLM_INCLUDE_DIR=C:/glm
```

### CMakeLists.txt (ARLauncher)

**Если GLFW не найден автоматически:**

```bash
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/glfw/include \
    -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a
```

---

## ✅ Проверка установки

### Windows

```powershell
# Проверить Qt
qmake --version

# Проверить CMake
cmake --version

# Проверить библиотеки
Test-Path C:\libjpeg-turbo64\include\turbojpeg.h
Test-Path C:\ffmpeg\include\libavcodec\avcodec.h
Test-Path C:\glm\glm\glm.hpp
Test-Path C:\glfw\include\GLFW\glfw3.h
```

### Linux

```bash
# Проверить Qt
qmake --version

# Проверить CMake
cmake --version

# Проверить библиотеки
pkg-config --exists glfw3 && echo "GLFW OK"
pkg-config --exists libjpeg-turbo && echo "libjpeg-turbo OK"
```

---

## 🚨 Решение проблем с DLL (Windows)

### Проблема: "turbojpeg.dll not found"

**Решение:**
1. Скопировать DLL в папку с .exe:
   ```powershell
   copy C:\libjpeg-turbo64\bin\turbojpeg.dll SensorConnector\lib\
   ```

2. Или добавить в PATH:
   ```powershell
   $env:PATH += ";C:\libjpeg-turbo64\bin"
   ```

### Проблема: FFmpeg DLL не найдены

**Решение:**
```powershell
# Скопировать все FFmpeg DLL
copy C:\ffmpeg\bin\*.dll SensorConnector\lib\

# Или добавить в PATH
$env:PATH += ";C:\ffmpeg\bin"
```

### Автоматическое копирование DLL

Добавить в `SensorConnector.pro`:

```qmake
win32 {
    # Копирование DLL после сборки
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$LIBJPEG_TURBO_DIR/bin/turbojpeg.dll) $$shell_path($$OUT_PWD) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$FFMPEG_DIR/bin/*.dll) $$shell_path($$OUT_PWD) $$escape_expand(\\n\\t)
}
```

---

## 📝 Рекомендуемые пути установки (Windows)

```
C:/
├── Qt/
│   └── 5.15.2/
│       └── mingw81_64/
│           ├── bin/
│           ├── lib/
│           └── include/
│
├── libjpeg-turbo64/
│   ├── include/
│   │   └── turbojpeg.h
│   ├── lib/
│   │   └── turbojpeg.lib
│   └── bin/
│       └── turbojpeg.dll
│
├── ffmpeg/
│   ├── include/
│   │   ├── libavcodec/
│   │   ├── libavformat/
│   │   └── libavutil/
│   ├── lib/
│   │   └── *.lib
│   └── bin/
│       └── *.dll
│
├── glm/
│   └── glm/
│       ├── glm.hpp
│       └── ...
│
├── glfw/
│   ├── include/
│   │   └── GLFW/
│   │       └── glfw3.h
│   └── lib/
│       └── libglfw3.a
│
├── opencv/
│   └── build/
│
└── vcpkg/ (альтернатива)
    └── installed/
        └── x64-windows/
            ├── include/
            └── lib/
```

---

## 🔗 Быстрые ссылки для скачивания

- **Qt**: https://www.qt.io/download
- **libjpeg-turbo**: https://github.com/libjpeg-turbo/libjpeg-turbo/releases
- **FFmpeg**: https://www.gyan.dev/ffmpeg/builds/ (Windows) или https://ffmpeg.org/download.html
- **GLM**: https://github.com/g-truc/glm/releases
- **GLFW**: https://www.glfw.org/download.html
- **OpenCV**: https://opencv.org/releases/
- **Vulkan SDK**: https://vulkan.lunarg.com/
- **vcpkg**: https://github.com/Microsoft/vcpkg

---

## 💡 Рекомендации

1. **Используйте vcpkg** - это самый простой способ для Windows
2. **Проверяйте пути** перед сборкой
3. **Копируйте DLL** в папку с исполняемым файлом
4. **Используйте одну и ту же архитектуру** (x64) для всех библиотек
5. **Добавляйте пути в PATH** для удобства

---

**Примечание:** Если возникают проблемы, проверьте логи сборки и убедитесь, что все пути указаны правильно.

