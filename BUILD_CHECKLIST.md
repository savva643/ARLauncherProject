# Чек-лист сборки проекта

## ✅ Проверка перед сборкой

### Windows

#### 1. Проверка инструментов
- [ ] Qt установлен и в PATH
  ```powershell
  qmake --version
  ```
- [ ] CMake установлен и в PATH
  ```powershell
  cmake --version
  ```
- [ ] Компилятор доступен (MinGW или MSVC)
  ```powershell
  g++ --version  # или cl для MSVC
  ```

#### 2. Проверка библиотек для SensorConnector
- [ ] libjpeg-turbo установлен
  ```powershell
  Test-Path C:\libjpeg-turbo64\include\turbojpeg.h
  ```
- [ ] FFmpeg установлен
  ```powershell
  Test-Path C:\ffmpeg\include\libavcodec\avcodec.h
  ```
- [ ] Пути в SensorConnector.pro правильные

#### 3. Проверка библиотек для LensEngineSDK
- [ ] GLM установлен
  ```powershell
  Test-Path C:\glm\glm\glm.hpp
  ```
- [ ] OpenCV установлен (опционально)
  ```powershell
  Test-Path C:\opencv\build\include\opencv2\opencv.hpp
  ```

#### 4. Проверка библиотек для ARLauncher
- [ ] GLFW установлен
  ```powershell
  Test-Path C:\glfw\include\GLFW\glfw3.h
  ```
- [ ] OpenGL доступен (обычно встроен)

### Linux

#### 1. Проверка инструментов
- [ ] Qt установлен
  ```bash
  qmake --version
  ```
- [ ] CMake установлен
  ```bash
  cmake --version
  ```
- [ ] Компилятор установлен
  ```bash
  g++ --version
  ```

#### 2. Проверка библиотек
```bash
# Проверить все библиотеки
pkg-config --exists libjpeg-turbo && echo "libjpeg-turbo OK"
pkg-config --exists libavcodec && echo "FFmpeg OK"
pkg-config --exists glfw3 && echo "GLFW OK"
pkg-config --exists opencv4 && echo "OpenCV OK"
```

---

## 🔧 Настройка путей

### SensorConnector.pro

**Если библиотеки не в стандартных местах, отредактируйте:**

```qmake
win32 {
    # Изменить на ваши пути
    LIBJPEG_TURBO_DIR = C:/ваш/путь/к/libjpeg-turbo64
    FFMPEG_DIR = C:/ваш/путь/к/ffmpeg
}
```

### CMakeLists.txt

**Для LensEngineSDK:**
```bash
cmake .. -DGLM_INCLUDE_DIR=C:/ваш/путь/к/glm
```

**Для ARLauncher:**
```bash
cmake .. \
    -DGLFW_INCLUDE_DIR=C:/ваш/путь/к/glfw/include \
    -DGLFW_LIBRARY=C:/ваш/путь/к/glfw/lib/libglfw3.a \
    -DGLM_INCLUDE_DIR=C:/ваш/путь/к/glm
```

---

## 📦 Проверка после сборки

### SensorConnector
- [ ] Файл создан: `SensorConnector/lib/SensorConnector.lib` (или `.a`)
- [ ] DLL скопированы в папку lib
- [ ] Запуск без ошибок

### LensEngineSDK
- [ ] Библиотека создана: `LensEngineSDK/build/lib/LensEngineSDK.a`
- [ ] Заголовочные файлы доступны

### ARLauncher
- [ ] Исполняемый файл создан: `ARLauncher/build/bin/ARLauncher.exe`
- [ ] Запуск открывает окно
- [ ] Нет ошибок линковки

---

## 🚨 Частые проблемы

### "Cannot find -lturbojpeg"
**Решение:** Проверьте путь к libjpeg-turbo в SensorConnector.pro

### "FFmpeg libraries not found"
**Решение:** Проверьте путь к FFmpeg и добавьте DLL в PATH

### "GLFW not found"
**Решение:** 
```bash
cmake .. -DGLFW_INCLUDE_DIR=C:/glfw/include -DGLFW_LIBRARY=C:/glfw/lib/libglfw3.a
```

### "GLM not found"
**Решение:**
```bash
cmake .. -DGLM_INCLUDE_DIR=C:/glm
```

### "DLL not found" при запуске
**Решение:** Скопируйте DLL в папку с .exe или добавьте пути в PATH

---

## ✅ Итоговая проверка

После успешной сборки всех компонентов:

1. ✅ SensorConnector собирается без ошибок
2. ✅ LensEngineSDK собирается без ошибок
3. ✅ ARLauncher собирается без ошибок
4. ✅ Все DLL доступны
5. ✅ Проект готов к использованию

---

**Если всё проверено - можно начинать интеграцию!**

