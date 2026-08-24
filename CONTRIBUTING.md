# 🤝 Contributing to Blindside

Thank you for your interest in contributing to **Blindside**! We welcome contributions from security engineers, computer vision developers, C++ programmers, and open-source privacy advocates.

---

## 📜 Code of Conduct

All contributors are expected to adhere to our [Code of Conduct](CODE_OF_CONDUCT.md). Please read it before participating in our community.

---

## 🛠️ Getting Started

### Prerequisites
- **C++20 Compiler**: GCC 10+, Clang 11+, or MSVC 2022+
- **Build Systems**: CMake 3.20+ and Ninja (recommended) or Make
- **Libraries**: OpenCV 4.x, ONNX Runtime (optional/emulated), X11 (Linux) / Win32 (Windows)

### Fork & Clone
1. Fork the repository on GitHub.
2. Clone your fork locally:
   ```bash
   git clone https://github.com/YOUR-USERNAME/Blindside.git
   cd Blindside
   ```
3. Set up upstream remote:
   ```bash
   git remote add upstream https://github.com/AshcrestHQ/Blindside.git
   ```

---

## 🏗️ Building & Testing

### Linux Build & Test Loop
```bash
# Create build directory
mkdir -p build && cd build

# Configure CMake with Release or Debug flags
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20

# Build library, daemon, and test suite
cmake --build . --parallel $(nproc)

# Execute CTest suite
ctest --output-on-failure
```

### Windows MSVC Build Loop
```cmd
build_windows.bat
```
Or via Developer Command Prompt / PowerShell:
```cmd
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
ctest -C Release --output-on-failure
```

### Running Synthetic Verification
To test system behavior headlessly without requiring a physical camera:
```bash
./build/blindside_daemon --synthetic
```

---

## 🎨 Coding Standards & Guidelines

Blindside follows modern C++20 conventions and strict performance constraints to maintain zero-allocation runtime guarantees:

1. **Modern C++20 Features**: Use `std::span`, `std::jthread`, `std::array`, concepts, and RAII.
2. **Zero In-Loop Allocations**: Real-time video processing pipelines must avoid dynamic heap allocations inside frame loops. Use fixed-capacity constructs like `RingBuffer<T, N>`.
3. **No External Network Dependencies**: Blindside is 100% edge-native. Code submitting telemetry over network sockets will be rejected.
4. **Platform Abstraction**: Platform-dependent code (Win32 API, X11, Wayland) must be guarded with `#if defined(...)` or wrapped in platform abstraction interfaces.
5. **Code Style**:
   - Class names: `PascalCase`
   - Method/variable names: `snake_case`
   - Member variables: `name_` (trailing underscore)
   - Constants/Enums: `UPPER_SNAKE_CASE` or `PascalCase` enum class values
   - Indentation: 4 spaces (no tabs)

---

## 🧪 Benchmark Verification

If your pull request modifies frame processing, spatial pose calculation, or ring buffer logic, you **must** run unit benchmarks and report telemetry:

```bash
# Test RingBuffer latency
./build/test_ring_buffer

# Test 3D gaze math computation speed
./build/test_gaze_math

# Test liveness verification latency
./build/test_liveness
```

Please include timing numbers in your PR description matching the format in [docs/BENCHMARKS.md](docs/BENCHMARKS.md).

---

## 📝 Pull Request Checklist

Before submitting a PR:
- [ ] Code compiles without warnings (`-Wall -Wextra -Wpedantic` on GCC/Clang or `/W4` on MSVC).
- [ ] All CTest unit tests pass (`ctest --output-on-failure`).
- [ ] No regression in synthetic mode runtime (`./build/blindside_daemon --synthetic`).
- [ ] New features include corresponding unit tests under `tests/`.
- [ ] Commit messages follow standard imperative format (e.g., `feat(engine): optimize SolvePnP gaze vector calculations`).

---

## 🐞 Reporting Issues & Bugs

If you discover a bug or security issue:
- For general bugs and feature requests, submit an issue using our [GitHub Issue Templates](.github/ISSUE_TEMPLATE/).
- For security vulnerabilities, refer to [SECURITY.md](SECURITY.md) for private reporting procedures.
