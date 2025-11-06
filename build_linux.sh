#!/bin/bash

# Скрипт сборки ARLauncher для Linux Ubuntu
# Использование: ./build_linux.sh

set -e  # Остановка при ошибке

echo "🔧 Building ARLauncher for Linux Ubuntu..."

# Определяем корневую директорию
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# 1. Сборка SensorConnector (Qt)
echo "========================================"
echo "Step 1: Building SensorConnector"
echo "========================================"
cd SensorConnector

if [ ! -f "SensorConnector.pro" ]; then
    echo "❌ SensorConnector.pro not found!"
    exit 1
fi

qmake SensorConnector.pro
make -j$(nproc)

if [ ! -f "lib/libSensorConnector.a" ]; then
    echo "❌ SensorConnector build failed!"
    exit 1
fi

echo "✅ SensorConnector built successfully"
echo ""

# 2. Сборка LensEngineSDK (CMake)
echo "========================================"
echo "Step 2: Building LensEngineSDK"
echo "========================================"
cd ../LensEngineSDK

if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ..
make -j$(nproc)

if [ ! -f "lib/libLensEngineSDK.a" ]; then
    echo "❌ LensEngineSDK build failed!"
    exit 1
fi

echo "✅ LensEngineSDK built successfully"
echo ""

# 3. Сборка ARLauncher (CMake) с Vulkan
echo "========================================"
echo "Step 3: Building ARLauncher (Vulkan)"
echo "========================================"
cd ../../ARLauncher

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Настраиваем пути к библиотекам
SENSOR_CONNECTOR_LIB="../../SensorConnector/lib/libSensorConnector.a"
LENS_ENGINE_SDK_DIR="../../LensEngineSDK"

if [ ! -f "$SENSOR_CONNECTOR_LIB" ]; then
    echo "❌ SensorConnector library not found at $SENSOR_CONNECTOR_LIB"
    exit 1
fi

# Конфигурация CMake с Vulkan и SensorConnector
cmake .. \
    -DUSE_VULKAN=ON \
    -DUSE_SENSOR_CONNECTOR=ON \
    -DSENSOR_CONNECTOR_LIB="$SENSOR_CONNECTOR_LIB" \
    -DLENS_ENGINE_SDK_DIR="$LENS_ENGINE_SDK_DIR"

# Сборка
echo "🔨 Building ARLauncher..."
make -j$(nproc)

if [ ! -f "bin/ARLauncher" ]; then
    echo "❌ ARLauncher build failed!"
    exit 1
fi

echo ""
echo "========================================"
echo "✅ Build Complete!"
echo "========================================"
echo ""
echo "📦 Built components:"
echo "   1. SensorConnector: $SCRIPT_DIR/SensorConnector/lib/libSensorConnector.a"
echo "   2. LensEngineSDK: $SCRIPT_DIR/LensEngineSDK/build/lib/libLensEngineSDK.a"
echo "   3. ARLauncher: $SCRIPT_DIR/ARLauncher/build/bin/ARLauncher"
echo ""
echo "🚀 To run ARLauncher:"
echo "   cd ARLauncher/build/bin"
echo "   ./ARLauncher"
echo ""

