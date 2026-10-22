#!/usr/bin/env bash
# Сборка ARLauncher (GlaskiOS): Vulkan + SensorStream + LensEngine (+ OpenVINS VIO, если собран)
#
#   ./build_arlauncher.sh            — обычная сборка
#   ./build_arlauncher.sh --vio      — сначала собрать OpenVINS (6DoF), потом лаунчер
#
# Зависимости (Ubuntu 22.04/24.04):
#   sudo apt install -y cmake build-essential pkg-config libglfw3-dev libglm-dev \
#        libvulkan-dev vulkan-tools glslang-tools mesa-vulkan-drivers \
#        libavcodec-dev libavutil-dev libswscale-dev libva-dev \
#        libfreetype6-dev libeigen3-dev libpcl-dev libopencv-dev libbullet-dev
#   Для --vio дополнительно: libceres-dev libboost-system-dev libboost-filesystem-dev \
#        libboost-thread-dev libboost-date-time-dev
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARLAUNCHER_DIR="$PROJECT_ROOT/ARLauncher"
BUILD_DIR="$ARLAUNCHER_DIR/build_vk"

if [[ "${1:-}" == "--vio" ]]; then
    echo "🔨 Building OpenVINS (VIO)..."
    "$PROJECT_ROOT/third_party/build_openvins.sh"
fi

mkdir -p "$BUILD_DIR"
# Кэш мог остаться от старой конфигурации (OpenGL/Qt) — сбрасываем
rm -f "$BUILD_DIR/CMakeCache.txt"

echo "🔨 CMake (Vulkan)..."
cmake -S "$ARLAUNCHER_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_VULKAN=ON \
    -DUSE_OPENGL=OFF \
    2>&1 | grep -E "(^--.*(OpenVINS|Vulkan|SensorStream|nvision|OpenCV)|Error|error)" || true

echo "🔨 Building..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo ""
echo "✅ Build complete: $BUILD_DIR/bin/ARLauncher"
echo ""
echo "🚀 Запуск (из папки сборки, там лежат shaders_vk):"
echo "   cd $BUILD_DIR && ./bin/ARLauncher"
echo "   ARL_FULLSCREEN=1 ARL_MONITOR=1 ./bin/ARLauncher      # на дисплей шлема, макс. частота"
echo "   ARL_STEREO=1 ARL_FOV=90 ./bin/ARLauncher             # стерео side-by-side"
echo ""
echo "🧪 Без iPhone: ./SensorStream/stream_simulator 127.0.0.1 9001"
