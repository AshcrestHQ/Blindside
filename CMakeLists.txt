cmake_minimum_required(VERSION 3.20)
project(blindside VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform Compiler Optimization Flags
if(MSVC)
    add_compile_options(/O2 /fp:fast /W4 /permissive-)
else()
    add_compile_options(-O3 -ffast-math -Wall -Wextra -Wpedantic)
endif()

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/include
)

# Find Dependencies
find_package(Threads REQUIRED)

# OpenCV Search
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found version: ${OpenCV_VERSION}")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(STATUS "OpenCV not found via CMake find_package. Using native/emulated fallback headers.")
endif()

# ONNX Runtime Search
find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h PATHS /usr/local/include /usr/include)
find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu)

if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
    message(STATUS "ONNX Runtime found: ${ONNXRUNTIME_LIBRARY}")
    include_directories(${ONNXRUNTIME_INCLUDE_DIR})
endif()

# Platform specific UI & OS libraries
set(PLATFORM_LIBS Threads::Threads)

if(UNIX AND NOT APPLE)
    find_package(X11 QUIET)
    if(X11_FOUND)
        include_directories(${X11_INCLUDE_DIR})
        list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
    endif()
endif()

if(WIN32)
    list(APPEND PLATFORM_LIBS User32.lib Gdi32.lib Dwmapi.lib Comctl32.lib)
endif()

if(OpenCV_FOUND)
    list(APPEND PLATFORM_LIBS ${OpenCV_LIBS})
endif()

if(ONNXRUNTIME_LIBRARY)
    list(APPEND PLATFORM_LIBS ${ONNXRUNTIME_LIBRARY})
endif()

# Core Blindside Library
add_library(blindside_lib
    src/core/camera.cpp
    src/engine/face_detector.cpp
    src/engine/pose_estimator.cpp
    src/engine/eavesdropper_detector.cpp
    src/trigger/privacy_trigger.cpp
    src/daemon/daemon.cpp
)

target_link_libraries(blindside_lib PUBLIC ${PLATFORM_LIBS})

# Main Executable
add_executable(blindside_daemon src/main.cpp)
target_link_libraries(blindside_daemon PRIVATE blindside_lib)

# Enable CTest Testing
enable_testing()

add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
target_link_libraries(test_ring_buffer PRIVATE blindside_lib)
add_test(NAME test_ring_buffer COMMAND test_ring_buffer)

add_executable(test_gaze_math tests/test_gaze_math.cpp)
target_link_libraries(test_gaze_math PRIVATE blindside_lib)
add_test(NAME test_gaze_math COMMAND test_gaze_math)

add_executable(test_eavesdropper tests/test_eavesdropper.cpp)
target_link_libraries(test_eavesdropper PRIVATE blindside_lib)
add_test(NAME test_eavesdropper COMMAND test_eavesdropper)

add_executable(test_liveness tests/test_liveness.cpp)
target_link_libraries(test_liveness PRIVATE blindside_lib)
add_test(NAME test_liveness COMMAND test_liveness)
