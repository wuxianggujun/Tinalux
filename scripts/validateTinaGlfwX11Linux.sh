#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${ROOT_DIR}/3rdparty/tina_glfw"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/tina_glfw-linux-x11}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

if [[ ! -f "${SOURCE_DIR}/CMakeLists.txt" ]]; then
    echo "error: missing tina_glfw checkout at ${SOURCE_DIR}" >&2
    exit 1
fi

CMAKE_GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR_ARGS=(-G Ninja)
fi

cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" "${CMAKE_GENERATOR_ARGS[@]}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DGLFW_BUILD_DOCS=OFF \
    -DGLFW_BUILD_EXAMPLES=OFF \
    -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_WAYLAND=OFF \
    -DGLFW_BUILD_X11=ON \
    -DGLFW_INSTALL=OFF

cmake --build "${BUILD_DIR}" --target glfw --parallel
