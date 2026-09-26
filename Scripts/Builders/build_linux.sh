#!/bin/bash

cd ../..

if test \( $# -ne 2 \);
then
    echo "Usage: ./build_linux.sh config qt-path"
    echo ""
    echo "config:"
    echo "  debug   -   build with the debug configuration"
    echo "  release -   build with the release configuration"
    echo ""
    echo "qt-path:"
    echo "  Path to the Qt installation"
    echo ""
    exit 1
fi

QT_PATH="$2"

if command -v ninja &> /dev/null; then
    CMAKE_ARG_BUILD_TOOL_TYPE_CONFIG="-G Ninja"
else
    CMAKE_ARG_BUILD_TOOL_TYPE_CONFIG="-G Unix Makefiles"
fi

if test "$1" = "debug"; then
    CMAKE_ARG_BUILD_TYPE_CONFIG="-DCMAKE_BUILD_TYPE=Debug"
elif test "$1" = "release"; then
    CMAKE_ARG_BUILD_TYPE_CONFIG="-DCMAKE_BUILD_TYPE=Release"
else
    echo "The config \"$1\" is not supported!"
    echo ""
    echo "Configs:"
    echo "  debug   -   build with the debug configuration"
    echo "  release -   build with the release configuration"
    echo ""
    exit 1
fi

MY_DIR="$(cd "$(dirname "$0")" 1>/dev/null 2>/dev/null && pwd)"
cd "${MY_DIR}"

export CC=clang
export CXX=clang++

cmake -S . -B build \
    "${CMAKE_ARG_BUILD_TYPE_CONFIG}" \
    "${CMAKE_ARG_BUILD_TOOL_TYPE_CONFIG}" \
    -DCMAKE_PREFIX_PATH="${QT_PATH}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build "${MY_DIR}/build" -- -j$(nproc)