# 🛡️ Blindside: Visual Eavesdropping & Physical Privacy Daemon

**Blindside** is a lightweight, zero-latency desktop physical security daemon for Linux and Windows workstations written in **C++20**. Using your workstation's front-facing camera, Blindside continuously monitors the physical space behind you. If an unrecognized face appears or someone looks over your shoulder at your display, Blindside instantly triggers a real-time privacy defense—such as edge glow notifications, blurred overlay windows, display dimming, or calling native OS system workstation lock APIs (`LockWorkStation` on Win32, `loginctl lock-session` / `xset ss activate` on Linux).

---

## ⚡ Key Features

- 🏎️ **Native C++20 Core**: High-performance, zero-allocation circular ring buffers (`RingBuffer<T, N>`) for ultra-low latency frame handling.
- 🎯 **3D Spatial Gaze Analysis**: Measures head pose (pitch, yaw, roll) and gaze vectors directly via OpenCV and ONNX Runtime C++ APIs.
- 👤 **Primary User Calibration**: Automatically registers the primary user's baseline face position directly in front of the screen.
- 🔍 **Shoulder Surfer & Eavesdropper Detection**: Flags secondary background faces whose 3D gaze vector is directed toward your display surface.
- ⚡ **Adaptive Frame Rate Sampling**: Dynamically shifts between **30 FPS (Active)** and **5 FPS (Idle)** to keep CPU utilization **under 2%**.
- 🛡️ **Active OS Privacy Response**:
  - **Soft Alert**: System tray notifications / native desktop edge glow alerts.
  - **Hard Defense**: Spawns transparent blurred window overlays and calls native OS workstation lock APIs after $>1.0\text{s}$ persistent eavesdropper gaze.
- 🔒 **100% On-Device Local Privacy**: No camera footage or biometric telemetry ever leaves memory or touches disk/network.

---

## 📐 Architecture Overview

```
                      ┌───────────────────────────────┐
                      │    Workstation Webcam Input   │
                      └───────────────┬───────────────┘
                                      │
                                      ▼
                      ┌───────────────────────────────┐
                      │    CameraCapture Component    │
                      └───────────────┬───────────────┘
                                      │
                         (Zero-Allocation RingBuffer)
                                      │
                                      ▼
┌───────────────────────────────────────────────────────────────────────────┐
│                       Spatial Vision Engine                               │
│  ┌───────────────────────┐                  ┌──────────────────────────┐  │
│  │ FaceDetector (ONNX)   │ ────────────────►│ PoseEstimator (3D Gaze)  │  │
│  └───────────────────────┘                  └──────────────────────────┘  │
│                                                          │                │
│                                                          ▼                │
│                                             ┌──────────────────────────┐  │
│                                             │ EavesdropperDetector     │  │
│                                             └──────────────────────────┘  │
└─────────────────────────────────────────────┬─────────────────────────────┘
                                              │
                                   (Hysteresis Filter > 1.0s)
                                              │
                                              ▼
                      ┌───────────────────────────────┐
                      │    PrivacyTriggerManager      │
                      │  - Soft Alert (Tray Glow)     │
                      │  - Hard Defense (Lock Workstn)│
                      │  - NIST/ISO Threat Logging    │
                      └───────────────────────────────┘
```

---

## 🛠️ Build & Installation Instructions

### Prerequisites
- **C++20 Compiler**: GCC 10+, Clang 11+, or MSVC 2022+
- **Build System**: CMake 3.20+
- **Dependencies**: OpenCV 4.x, ONNX Runtime C++ API, X11 (Linux) or Win32 API (Windows)

### Linux Compilation
Run the automated CMake build script:
```bash
chmod +x build_linux.sh
./build_linux.sh
```

To run unit tests manually via CMake/CTest:
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

### Windows Compilation (MSVC)
Run the automated batch script:
```cmd
build_windows.bat
```

---

## 🚀 Command-Line Usage & Flags

Run the compiled `blindside_daemon` executable with your preferred options:

```bash
# Run persistent background daemon with default settings (30 FPS active / 5 FPS idle)
./build/blindside_daemon --daemon

# Calibrate primary user face position on launch
./build/blindside_daemon --daemon --calibrate

# Run in synthetic emulation mode (for CI / headless server validation without physical camera)
./build/blindside_daemon --synthetic

# Customize frame rates and hysteresis threshold
./build/blindside_daemon --daemon --fps-active 30 --fps-idle 5 --hysteresis 1.5

# Specify privacy trigger mode (soft, hard, both, log)
./build/blindside_daemon --daemon --trigger-mode hard
```

### CLI Command Reference

| Flag | Description | Default |
| :--- | :--- | :--- |
| `--daemon` | Runs continuously in desktop background daemon mode | Disabled |
| `--calibrate` | Calibrates primary user center face location on launch | Disabled |
| `--synthetic` | Runs synthetic frame emulation mode (no hardware webcam required) | Disabled |
| `--trigger-mode` | Selects alert response (`soft`, `hard`, `both`, `log`) | `both` |
| `--fps-active` | Frame rate during active movement / secondary detection (Hz) | `30` |
| `--fps-idle` | Frame rate during calm idle monitoring (Hz) | `5` |
| `--hysteresis` | Required duration (seconds) of secondary gaze before hard lock | `1.0` |

---

## 📊 Security & Threat Model Compliance

Blindside implements physical security controls mapped to international compliance standards:

- **NIST SP 800-53 Rev. 5 (PE-3, PE-6, AC-11)**: Automates physical perimeter observation monitoring and screen-locking controls upon visual line-of-sight exposure.
- **ISO/IEC 27001:2022 (Control A.7.7 Clear Screen)**: Restricts unauthorized visual access to confidential workstation displays.

For full threat analysis, refer to [THREAT_MODEL.md](file:///home/medhansh/Documents/antigravity/jolly-pythagoras/THREAT_MODEL.md).

---

## 📜 License

Distributed under the MIT License. Open source and free for enterprise, security operations, and personal workstation deployment.
