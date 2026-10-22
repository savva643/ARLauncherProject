#!/bin/bash

# LEGACY: старая сборка с Qt SensorConnector и OpenGL.
# Актуальная сборка (Vulkan + SensorStream + LensEngine): ./build_arlauncher.sh
#
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

# Очистка предыдущей сборки
echo "🧹 Cleaning previous build..."
if [ -f "Makefile" ]; then
    make clean 2>/dev/null || true
fi
rm -f Makefile* *.o moc_* 2>/dev/null || true
rm -rf build/obj build/moc 2>/dev/null || true

# Поиск qmake
QMAKE_CMD="qmake"
if [ -f "/home/savva/Qt/6.5.3/gcc_64/bin/qmake6" ]; then
    QMAKE_CMD="/home/savva/Qt/6.5.3/gcc_64/bin/qmake6"
    echo "📦 Using Qt qmake: $QMAKE_CMD"
elif command -v qmake6 &> /dev/null; then
    QMAKE_CMD="qmake6"
elif command -v qmake &> /dev/null; then
    QMAKE_CMD="qmake"
else
    echo "❌ qmake not found! Please install Qt or set QMAKE path"
    exit 1
fi

# Генерация Makefile
echo "📦 Generating Makefile..."
$QMAKE_CMD SensorConnector.pro

# Сборка
echo "🔨 Building SensorConnector..."
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

# Очистка предыдущей сборки (опционально)
echo "🧹 Cleaning previous build..."
rm -rf CMakeCache.txt CMakeFiles/ 2>/dev/null || true

# Конфигурация и сборка
echo "📦 Configuring CMake..."
cmake ..
echo "🔨 Building LensEngineSDK..."
make -j$(nproc)

if [ ! -f "lib/libLensEngineSDK.a" ]; then
    echo "❌ LensEngineSDK build failed!"
    exit 1
fi

echo "✅ LensEngineSDK built successfully"
echo ""

# 3. Сборка ARLauncher (CMake) с OpenGL
echo "========================================"
echo "Step 3: Building ARLauncher (OpenGL)"
echo "========================================"
cd ../../ARLauncher

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Очистка предыдущей сборки (опционально)
echo "🧹 Cleaning previous build..."
rm -rf CMakeCache.txt CMakeFiles/ 2>/dev/null || true

# Настраиваем пути к библиотекам
SENSOR_CONNECTOR_LIB="../../SensorConnector/lib/libSensorConnector.a"
LENS_ENGINE_SDK_DIR="../../LensEngineSDK"

if [ ! -f "$SENSOR_CONNECTOR_LIB" ]; then
    echo "❌ SensorConnector library not found at $SENSOR_CONNECTOR_LIB"
    exit 1
fi

# Проверка наличия FreeType
if ! pkg-config --exists freetype2 2>/dev/null; then
    echo "⚠️  FreeType not found. Installing libfreetype6-dev..."
    echo "   Run: sudo apt-get install libfreetype6-dev"
    echo "   Text rendering will be limited without FreeType"
fi

# Конфигурация CMake с OpenGL и SensorConnector
echo "📦 Configuring CMake..."
cmake .. \
    -DUSE_OPENGL=ON \
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
