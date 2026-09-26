#!/bin/bash

cd ../..

if test \( $# -ne 2 \);
then
    echo "Usage: ./build_macos.sh config qt_path"
    echo ""
    echo "config:"
    echo "  debug   -   build with the debug configuration"
    echo "  release -   build with the release configuration"
    echo ""
    echo "qt_path:"
    echo "  Path to the Qt installation"
    echo ""
    exit 1
fi


if test \( \( -n "$1" \) -a \( "$1" = "debug" \) \);then
    CONFIG="Debug"
elif test \( \( -n "$1" \) -a \( "$1" = "release" \) \);then
    CONFIG="Release"
else
    echo "The config \"$1\" is not supported!"
    echo ""
    echo "Configs:"
    echo "  debug   -   build with the debug configuration"
    echo "  release -   build with the release configuration"
    echo ""
    exit 1
fi

QT_PATH="$2"

cmake -S . -B build \
    -G "Xcode" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_PREFIX_PATH="$QT_PATH"

echo cmake --build build --config "${CONFIG}" \
    -DCMAKE_C_COMPILER="clang" \
    -DCMAKE_CXX_COMPILER="clang++"

cmake --build build --config "${CONFIG}" \
    -DCMAKE_C_COMPILER="clang" \
    -DCMAKE_CXX_COMPILER="clang++"