# Сводка библиотек и их расположение

## 📋 Полный список всех библиотек

### SensorConnector (Qt)

| Библиотека | Где скачать | Куда установить | Путь в проекте |
|------------|-------------|-----------------|----------------|
| **Qt** | https://www.qt.io/download | `C:/Qt/5.15.2/` | Автоматически через PATH |
| **libjpeg-turbo** | https://github.com/libjpeg-turbo/libjpeg-turbo/releases | `C:/libjpeg-turbo64/` | `SensorConnector.pro` строка 16 |
| **FFmpeg** | https://www.gyan.dev/ffmpeg/builds/ | `C:/ffmpeg/` | `SensorConnector.pro` строка 22 |

### LensEngineSDK (CMake)

| Библиотека | Где скачать | Куда установить | Путь в CMake |
|------------|-------------|-----------------|--------------|
| **GLM** | https://github.com/g-truc/glm/releases | `C:/glm/` | `-DGLM_INCLUDE_DIR=C:/glm` |
| **OpenCV** | https://opencv.org/releases/ | `C:/opencv/` | `-DOpenCV_DIR=C:/opencv/build` |

### ARLauncher (CMake)

| Библиотека | Где скачать | Куда установить | Путь в CMake |
|------------|-------------|-----------------|--------------|
| **GLFW3** | https://www.glfw.org/download.html | `C:/glfw/` | `-DGLFW_INCLUDE_DIR=C:/glfw/include -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a` |
| **GLM** | https://github.com/g-truc/glm/releases | `C:/glm/` | `-DGLM_INCLUDE_DIR=C:/glm` |
| **OpenGL** | Встроен в Windows/Linux | - | Автоматически |

---

## 🔧 Настройка путей

### SensorConnector.pro

**Файл:** `SensorConnector/SensorConnector.pro`

**Строки для редактирования:**

```qmake
# Строка 16 - libjpeg-turbo
LIBJPEG_TURBO_DIR = C:/libjpeg-turbo64

# Строка 22 - FFmpeg
FFMPEG_DIR = C:/ffmpeg
```

**Если библиотеки установлены в других местах, измените эти строки!**

### CMakeLists.txt (LensEngineSDK)

**Файл:** `LensEngineSDK/CMakeLists.txt`

**Автоматический поиск:** CMake пытается найти GLM автоматически

**Если не найдено, укажите путь:**
```bash
cmake .. -DGLM_INCLUDE_DIR=C:/glm
```

### CMakeLists.txt (ARLauncher)

**Файл:** `ARLauncher/CMakeLists.txt`

**Автоматический поиск:** CMake пытается найти GLFW и GLM автоматически

**Если не найдено, укажите пути:**
```bash
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/glfw/include \
    -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a \
    -DGLM_INCLUDE_DIR=C:/glm \
    -DUSE_OPENGL=ON
```

---

## ✅ Проверка установки

### Windows

```powershell
# Проверить libjpeg-turbo
Test-Path C:\libjpeg-turbo64\include\turbojpeg.h
# Должно быть: True

# Проверить FFmpeg
Test-Path C:\ffmpeg\include\libavcodec\avcodec.h
# Должно быть: True

# Проверить GLM
Test-Path C:\glm\glm\glm.hpp
# Должно быть: True

# Проверить GLFW
Test-Path C:\glfw\include\GLFW\glfw3.h
# Должно быть: True
```

### Linux

```bash
# Проверить все библиотеки
pkg-config --exists libjpeg-turbo && echo "libjpeg-turbo OK"
pkg-config --exists libavcodec && echo "FFmpeg OK"
ls /usr/include/glm/glm.hpp && echo "GLM OK"
ls /usr/include/GLFW/glfw3.h && echo "GLFW OK"
```

---

## 🚨 Если библиотеки не найдены

### SensorConnector

**Ошибка:** "Cannot find -lturbojpeg"

**Решение:**
1. Откройте `SensorConnector/SensorConnector.pro`
2. Найдите строку 16: `LIBJPEG_TURBO_DIR = C:/libjpeg-turbo64`
3. Измените на ваш путь
4. Пересоберите: `qmake && make`

### LensEngineSDK / ARLauncher

**Ошибка:** "GLM not found" или "GLFW not found"

**Решение:**
```bash
# Указать пути вручную
cmake .. \
    -DGLM_INCLUDE_DIR=C:/ваш/путь/к/glm \
    -DGLFW_INCLUDE_DIR=C:/ваш/путь/к/glfw/include \
    -DGLFW_LIBRARY=C:/ваш/путь/к/glfw/lib/libglfw3.a
```

---

## 💡 Рекомендации

1. **Используйте vcpkg** - это самый простой способ:
   ```powershell
   vcpkg install glfw3:x64-windows glm:x64-windows libjpeg-turbo:x64-windows ffmpeg:x64-windows
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
   ```

2. **Проверяйте структуру папок** - убедитесь, что файлы в правильных местах

3. **Используйте одну архитектуру** - все библиотеки должны быть x64

4. **Копируйте DLL** в папку с .exe после сборки

---

## 📍 Стандартные пути установки

### Windows (рекомендуется)

```
C:/
├── libjpeg-turbo64/    ← SensorConnector
├── ffmpeg/              ← SensorConnector
├── glm/                 ← LensEngineSDK, ARLauncher
├── glfw/                ← ARLauncher
└── opencv/              ← LensEngineSDK (опционально)
```

### Linux (стандартные пути)

```
/usr/include/
├── turbojpeg.h          ← libjpeg-turbo
├── libavcodec/          ← FFmpeg
├── glm/                 ← GLM
└── GLFW/                ← GLFW
```

---

**См. [DEPENDENCIES_SETUP.md](DEPENDENCIES_SETUP.md) для детальных инструкций по установке каждой библиотеки.**

