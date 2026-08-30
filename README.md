# 🛡️ Blindside

> Paranoia, but automated.

Blindside is a local-first physical privacy daemon that watches
for visual eavesdropping and reacts when someone is looking
over your shoulder.

No cloud.
No screenshots.
No facial database.

Just your webcam, some computer vision,
and an unhealthy amount of paranoia.

---

## 🏗️ What It Is & Why It Exists

If you're working on something sensitive in public, someone *is* going to look over your shoulder. Blindside runs a legit **OpenCV YuNet** face detection pipeline and uses **cv::solvePnP** to compute exactly where people are looking.

If a rando stares at your screen for too long, Blindside locks your workstation or slaps a targeted redaction overlay over your active window.

---

## 📊 Supported Platforms

| Platform      | Detection    | Privacy response        | Status                              |
| ------------- | ------------ | ----------------------- | ----------------------------------- |
| Windows       | YuNet + pose | platform redaction/lock | Demo validation                     |
| Linux X11     | YuNet + pose | screen redaction        | Build verified / runtime validation |
| Linux Wayland | YuNet + pose | session lock fallback   | Limited                             |
| Other         | —            | —                       | Unsupported                         |

---

## ⚡ Quick Start

# 1. Clone the repository
git clone https://github.com/AshcrestHQ/Blindside.git
cd Blindside

# 2. Install dependencies (Linux X11 / Wayland)

> **Note on V3 Linux Releases:** The standard V3 Linux artifact uses a **system-linked OpenCV runtime strategy** and is actively built and tested against **OpenCV 4.10.x**. It requires the host system to provide OpenCV 4.10+ (e.g., Ubuntu 26.04 or compiled from source).

```bash
# Ubuntu 26.04+ (or equivalent providing OpenCV 4.10+)
sudo apt-get install -y cmake ninja-build libopencv-core4.10 libopencv-videoio4.10 libopencv-objdetect4.10 libopencv-imgproc4.10 libopencv-calib3d4.10 libx11-dev libxext-dev
```

# 3. Download the YuNet model
./scripts/download_models.sh

# 4. Build
cmake -S . -B build && cmake --build build

# 5. Run diagnostics to verify your platform
./build/blindside_daemon --diagnostics

# 6. Run the daemon
./build/blindside_daemon --daemon
```

---

## 🎬 How to Demo

1. Start Blindside (`./build/blindside_daemon --daemon`).
2. Sit normally in front of your camera.
3. Have another person enter the camera's field of view.
4. Have them look directly toward the screen for more than 1 second.
5. Observe the privacy response (redaction overlay or lock).

---

## 🧠 Architecture & Privacy

*   **Real Computer Vision**: We use OpenCV's YuNet ONNX model for face detection and 5-point facial landmarking.
*   **True 6D Pose Estimation**: We map 2D facial landmarks to a generic 3D facial model using `cv::solvePnP` to calculate pitch, yaw, and roll. No naive trig math.
*   **Zero-Allocation Pipeline**: Once initialized, the main tracking loop uses a pre-allocated RingBuffer and runs with minimal overhead.
*   **Structured Local Security Logging**: We log *what* happened (e.g. `EAVESDROPPER_DETECTED`), not *who* it was. No image data is ever written to disk or sent over the network.

---

## ⚠️ Known Limitations

Let's not kid ourselves—CV on consumer hardware has limits:
*   **Webcam Dependency**: If your camera is blocked or off, Blindside cannot function.
*   **Lighting/Occlusions**: Heavy occlusion (masks, sunglasses) or poor lighting breaks YuNet facial landmarking.
*   **Rendered / Displayed Content**: Faces rendered inside digital content (e.g., Pinterest boards, advertisements, screens) can currently be misclassified as physical secondary faces by YuNet detection.
*   **Global Liveness Evaluation**: In V3.1.0, liveness verification is evaluated globally across secondary gaze events rather than independently per face track (independent per-track liveness is scheduled for V3.2).
*   **Wayland Fallback**: We can't do targeted window redaction overlays on pure Wayland due to protocol restrictions. We fall back to standard `loginctl` screen locks.
*   **Hardware Verification Status**: Local physical hardware tests are actively pending final validation for both X11 runtime and Windows runtime environments.

---

## 🛠️ Testing & Troubleshooting

Run headless tests:
```bash
ctest --test-dir build --output-on-failure
```

**Common Issues:**
*   **"YuNet model unavailable"**: The daemon looks for `models/face_detection_yunet_2023mar.onnx` relative to the executable. Pass an explicit path with `--model-path /path/to/model.onnx`.
*   **"opencv_world*.dll was not found" (Windows)**: If running outside the build directory, ensure the OpenCV runtime DLLs are copied next to the executable.

---

## 🤝 Community

Check the docs if you want to contribute:
- [CONTRIBUTING.md](CONTRIBUTING.md) — How to not get your PR rejected.
- [SECURITY.md](SECURITY.md) — How to report vulns.
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) — Don't be a jerk.

**License:** MIT. Do whatever you want with it, just don't blame us if it locks your screen during a presentation.
