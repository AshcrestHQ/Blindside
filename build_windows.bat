@echo off
REM ========================================================
REM   🛡️ Building Blindside Daemon for Windows (MSVC C++20)
REM ========================================================

IF NOT EXIST build mkdir build
cd build

echo [1/3] Configuring CMake project with MSVC...
cmake -G "Visual Studio 17 2022" -A x64 ..

echo [2/3] Compiling Release executable...
cmake --build . --config Release --parallel

echo [3/3] Running automated unit test suite...
ctest -C Release --output-on-failure

echo ========================================================
echo   SUCCESS: blindside_daemon.exe compiled successfully!
echo   Run binary: .\Release\blindside_daemon.exe --synthetic
echo ========================================================
cd ..
