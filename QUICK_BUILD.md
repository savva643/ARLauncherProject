# 🚀 Быстрая сборка ARLauncherProject

## Linux (Ubuntu) - Один скрипт

```bash
cd /home/savva/Documents/GitHub/ARLauncherProject
./build_all.sh
```

## Ручная сборка (3 шага)

### 1. SensorConnector (Qt библиотека)
```bash
cd SensorConnector
qmake SensorConnector.pro && make -j$(nproc)
```

### 2. LensEngineSDK (CMake библиотека)
```bash
cd ../LensEngineSDK
mkdir -p build && cd build
cmake .. && make -j$(nproc)
```

### 3. ARLauncher (CMake приложение)
```bash
cd ../../ARLauncher
mkdir -p build && cd build
cmake .. -DUSE_OPENGL=ON && make -j$(nproc)
```

## Запуск

```bash
cd ARLauncher/build/bin
./ARLauncher
```

## Что дальше?

1. ✅ **SensorConnector собран** → Библиотека готова для использования
2. ✅ **LensEngineSDK собран** → AR движок готов
3. ✅ **ARLauncher собран** → Приложение готово к запуску

### Для полной интеграции:

1. **Создайте главное приложение**, которое:
   - Инициализирует `SensorConnectorCore`
   - Подключает данные к `LensEngineAPI`
   - Передает позу камеры в `ARLauncher`

2. **Или используйте пример** из `examples/full_integration_example.cpp`

## 📖 Подробная документация

- `BUILD_GUIDE_LINUX.md` - Детальное руководство для Linux
- `ARCHITECTURE.md` - Архитектура проекта
- `INTEGRATION_GUIDE.md` - Интеграция компонентов
- `BUILD_INSTRUCTIONS.md` - Полные инструкции по сборке

