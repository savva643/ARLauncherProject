#!/bin/bash

# 🔨 Скрипт для сборки ARLauncher с LiDAR Visualizer и SensorConnector

set -e  # Выход при ошибке

PROJECT_ROOT="/home/savva/Documents/ARLauncherProject"
ARLAUNCHER_DIR="$PROJECT_ROOT/ARLauncher"
BUILD_DIR="$ARLAUNCHER_DIR/build_vk"
SENSOR_DIR="$PROJECT_ROOT/SensorConnector"

echo "🔨 =========================================="
echo "   ARLauncher Build Script v2.0"
echo "=========================================="
echo ""

# Проверяем, что мы в правильной директории
if [ ! -f "$ARLAUNCHER_DIR/CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found in $ARLAUNCHER_DIR"
    exit 1
fi

echo "📁 Project root: $PROJECT_ROOT"
echo "📁 ARLauncher dir: $ARLAUNCHER_DIR"
echo "📁 Build dir: $BUILD_DIR"
echo ""

# Проверяем наличие LiDARVisualizer файлов
echo "✅ Checking LiDARVisualizer files..."
if [ ! -f "$ARLAUNCHER_DIR/include/LiDARVisualizer.h" ]; then
    echo "❌ Error: LiDARVisualizer.h not found"
    exit 1
fi
if [ ! -f "$ARLAUNCHER_DIR/src/LiDARVisualizer.cpp" ]; then
    echo "❌ Error: LiDARVisualizer.cpp not found"
    exit 1
fi
echo "✅ LiDARVisualizer files found"
echo ""

# Проверяем и собираем SensorConnector если нужно
echo "✅ Checking SensorConnector..."
if [ ! -f "$SENSOR_DIR/lib/libSensorConnector.a" ]; then
    echo "⚠️  libSensorConnector.a not found"
    echo "   Building SensorConnector with qmake6..."
    cd "$SENSOR_DIR"
    
    # Очищаем старые файлы сборки
    rm -f Makefile
    rm -rf build/
    
    # Собираем с qmake6
    qmake6 SensorConnector.pro -o Makefile
    make -j4
    
    echo "✅ SensorConnector built successfully"
    cd "$ARLAUNCHER_DIR"
else
    echo "✅ SensorConnector library found"
fi
echo ""

# Проверяем LensEngineSDK
echo "✅ Checking LensEngineSDK..."
if [ ! -f "$PROJECT_ROOT/LensEngineSDK/CMakeLists.txt" ]; then
    echo "❌ Error: LensEngineSDK/CMakeLists.txt not found"
    exit 1
fi
echo "✅ LensEngineSDK found"
echo ""

# Создаём директорию сборки если её нет
if [ ! -d "$BUILD_DIR" ]; then
    echo "📁 Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
else
    echo "📁 Build directory exists, cleaning..."
    # Не удаляем всё, только очищаем CMake кэш
    rm -f "$BUILD_DIR/CMakeCache.txt"
fi

# Переходим в директорию сборки
cd "$BUILD_DIR"

echo ""
echo "🔨 Running CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_VULKAN=ON \
    -DUSE_OPENGL=ON \
    -DUSE_SENSOR_CONNECTOR=ON \
    -DQt6_DIR="/home/savva/Qt/6.5.3/gcc_64/lib/cmake/Qt6" \
    2>&1 | grep -E "(^--|Error|Warning|Found)" || true

echo ""
echo "🔨 Building ARLauncher (this may take a few minutes)..."
echo ""

# Собираем с выводом прогресса
if make -j4; then
    echo ""
    echo "✅ =========================================="
    echo "   Build Complete!"
    echo "=========================================="
    echo ""
    echo "📦 Output: $BUILD_DIR/bin/ARLauncher"
    echo ""
    echo "🚀 To run:"
    echo "   cd $BUILD_DIR"
    echo "   ./bin/ARLauncher"
    echo ""
else
    echo ""
    echo "❌ =========================================="
    echo "   Build Failed!"
    echo "=========================================="
    echo ""
    echo "Check errors above and fix them"
    exit 1
fi
