#!/usr/bin/env bash
set -e

echo "=== Building Master Duel Switcher (Linux) ==="
mkdir -p build
cd build

if command -v cmake &> /dev/null; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j$(nproc)
    echo "✓ Build complete: build/MasterDuelSwitcher"
else
    g++ -O2 -std=c++17 -fPIC ../src/main.cpp $(pkg-config --cflags --libs Qt6Widgets) -o MasterDuelSwitcher
    echo "✓ Built using g++: build/MasterDuelSwitcher"
fi
