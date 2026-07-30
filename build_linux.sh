#!/usr/bin/env bash
set -e

echo "========================================================"
echo "  🛡️ Building Blindside Daemon for Linux (C++20)"
echo "========================================================"

# Determine CMake binary
CMAKE_BIN="cmake"
if command -v ~/.local/bin/cmake &> /dev/null; then
    CMAKE_BIN="$HOME/.local/bin/cmake"
fi

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[1/3] Configuring CMake project..."
"$CMAKE_BIN" .. -DCMAKE_BUILD_TYPE=Release

echo "[2/3] Compiling C++20 targets..."
make -j"$(nproc)"

echo "[3/3] Running automated unit test suite..."
ctest --output-on-failure

echo "========================================================"
echo "  SUCCESS: blindside_daemon compiled successfully!"
echo "  Run binary: ./build/blindside_daemon --synthetic"
echo "========================================================"
