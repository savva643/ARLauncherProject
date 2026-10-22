#!/usr/bin/env bash
# Сборка OpenVINS (VIO, MSCKF) без ROS для LensEngine.
# Результат: third_party/openvins/install/{include/open_vins,lib}
# LensEngineSDK подхватит его автоматически (LENSENGINE_WITH_OPENVINS).
#
# Зависимости (Ubuntu 22.04/24.04):
#   sudo apt install -y libeigen3-dev libopencv-dev libceres-dev \
#        libboost-system-dev libboost-filesystem-dev libboost-thread-dev libboost-date-time-dev
#
# Лицензия OpenVINS — GPL-3.0.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$HERE/openvins/src"
BUILD="$HERE/openvins/build"
PREFIX="$HERE/openvins/install"
# master с поддержкой Ceres 2.2 (Ubuntu 24.04); тег v2.7 с Ceres 2.2 не собирается
COMMIT="6948812"

if [ ! -d "$SRC/.git" ]; then
    git clone https://github.com/rpng/open_vins.git "$SRC"
fi
git -C "$SRC" fetch --quiet origin master || true
git -C "$SRC" checkout --quiet "$COMMIT"

cmake -S "$SRC/ov_msckf" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ROS=OFF \
    -DENABLE_ARUCO_TAGS=OFF \
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build "$BUILD" --target ov_msckf_lib -j"$(nproc)"

# Ставим только библиотеку и заголовки (без debug-символов: OpenVINS собирается с -g3)
mkdir -p "$PREFIX/lib" "$PREFIX/include/open_vins"
cp -a "$BUILD"/libov_msckf_lib.so* "$PREFIX/lib/"
strip --strip-debug "$PREFIX/lib/libov_msckf_lib.so"
for d in ov_core ov_init ov_msckf; do
    (cd "$SRC/$d/src" && find . \( -name '*.h' -o -name '*.hpp' \) -exec cp --parents {} "$PREFIX/include/open_vins/" \;)
done
echo "OpenVINS installed to $PREFIX"
