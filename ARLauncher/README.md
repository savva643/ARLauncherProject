# ARLauncher

## Описание

ARLauncher - это Native C++ лаунчер с 3D сценой для AR приложений.

## Особенности

- Vulkan/OpenGL рендерер
- Custom UI система (не ImGui)
- 3D сцена с синхронизацией с реальной камерой
- Интеграция с LensEngineSDK
- Видео-фон с камеры iPhone
- Виртуальные объекты поверх реального мира

## Зависимости

- **GLFW** - создание окна
- **Vulkan** или **OpenGL** - рендеринг
- **glm** - математика
- **LensEngineSDK** - AR движок

## Сборка

```bash
mkdir build
cd build
cmake .. -DUSE_VULKAN=ON  # или -DUSE_OPENGL=ON
make
```

## Структура

```
ARLauncher/
├── include/          # Заголовочные файлы
│   ├── Application.h
│   ├── Renderer.h
│   ├── Scene.h
│   └── Camera.h
├── src/              # Исходные файлы
│   ├── main.cpp
│   ├── Application.cpp
│   └── ...
├── ui/               # Custom UI система
│   ├── UIRenderer.h
│   ├── UIElement.h
│   ├── Button.h
│   └── ...
└── CMakeLists.txt
```

## Использование

```cpp
#include "Application.h"

int main() {
    Application app;
    if (app.initialize(argc, argv)) {
        app.run();
    }
    return 0;
}
```

## Custom UI

Система UI элементов:
- `UIElement` - базовый класс
- `Button` - кнопки
- `Text` - текст
- `Input` - поля ввода
- `Window` - окна
- `Widget` - виджеты
- `Style` - стилизация

## Демо-функциональность

- 3D куб в пространстве
- Виртуальная камера, синхронизированная с реальной
- Видео поток с камеры как фон
- Простые UI элементы

