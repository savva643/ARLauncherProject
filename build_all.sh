#!/bin/bash

# Скрипт сборки ARLauncherProject для Linux (Ubuntu)
# Использование: ./build_all.sh

set -e  # Остановить при ошибке

echo "========================================"
echo "ARLauncherProject Build Script"
echo "========================================"
echo ""

# Проверка зависимостей
echo "📋 Checking dependencies..."

# Qt
if ! command -v qmake &> /dev/null; then
    echo "❌ qmake not found. Installing Qt..."
    sudo apt-get update
    sudo apt-get install -y qtbase5-dev qtbase5-dev-tools
fi

# CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ cmake not found. Installing CMake..."
    sudo apt-get update
    sudo apt-get install -y cmake
fi

# Проверка библиотек
echo "📚 Checking libraries..."

MISSING_LIBS=()

if ! pkg-config --exists libturbojpeg 2>/dev/null && ! ldconfig -p | grep -q libturbojpeg; then
    MISSING_LIBS+=("libturbojpeg0-dev")
fi

if ! pkg-config --exists libavcodec 2>/dev/null && ! ldconfig -p | grep -q libavcodec; then
    MISSING_LIBS+=("libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev")
fi

if ! ldconfig -p | grep -q libglm; then
    MISSING_LIBS+=("libglm-dev")
fi

if ! ldconfig -p | grep -q libglfw; then
    MISSING_LIBS+=("libglfw3-dev")
fi

if [ ${#MISSING_LIBS[@]} -ne 0 ]; then
    echo "⚠️  Missing libraries detected. Installing..."
    sudo apt-get update
    sudo apt-get install -y "${MISSING_LIBS[@]}"
fi

echo "✅ All dependencies installed"
echo ""

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
echo "   Library: $(pwd)/lib/libSensorConnector.a"
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
cmake .. -DLENSENGINE_USE_OPENCV=ON
make -j$(nproc)

if [ ! -f "lib/libLensEngineSDK.a" ]; then
    echo "❌ LensEngineSDK build failed!"
    exit 1
fi

echo "✅ LensEngineSDK built successfully"
echo "   Library: $(pwd)/lib/libLensEngineSDK.a"
echo ""

# 3. Сборка ARLauncher (CMake)
echo "========================================"
echo "Step 3: Building ARLauncher"
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

cmake .. \
    -DUSE_OPENGL=ON \
    -DSENSOR_CONNECTOR_LIB="$SENSOR_CONNECTOR_LIB" \
    -DLENS_ENGINE_SDK_DIR="$LENS_ENGINE_SDK_DIR"

make -j$(nproc)

if [ ! -f "bin/ARLauncher" ]; then
    echo "❌ ARLauncher build failed!"
    exit 1
fi

echo "✅ ARLauncher built successfully"
echo "   Executable: $(pwd)/bin/ARLauncher"
echo ""

# Итог
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
echo "   cd ARLauncher/build/bin && ./ARLauncher"
echo ""
echo "📱 Next steps:"
echo "   1. Start SensorConnector servers (or use example)"
echo "   2. Connect iPhone via USB or WiFi"
echo "   3. Run ARLauncher"
echo ""

