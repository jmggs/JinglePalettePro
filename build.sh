#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# Jingle Palette Pro - Qt/C++ Build Script
# Requires: Qt 6.x, CMake 3.16+, C++17 compiler
# ─────────────────────────────────────────────────────────────────────────────

set -e

echo "=== Jingle Palette Pro Build ==="

# Create build directory
mkdir -p build
cd build

# Configure
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(qt-cmake -query QT_HOST_PREFIX 2>/dev/null || echo $QTDIR)"

# Build
cmake --build . --parallel

echo ""
echo "=== Build complete! ==="
echo "Executable: build/Jingle Palette Pro"
