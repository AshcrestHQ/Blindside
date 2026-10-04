# 🐍 Blindside Python Reference Implementation & Validation Harness

> **NOTICE**: This directory contains an auxiliary Python reference implementation and developer validation harness.
> It is **NOT** production runtime code. The official production daemon for Blindside is and remains **100% C++20**.

---

## 1. Why This Reference Implementation Exists

During development and architecture planning (such as preparing for V4), engineers need a fast, interactive way to:
* Validate mathematical transformations (e.g., Rodrigues rotation vectors to Euler angles, gaze vector projection).
* Experiment with computer vision model behavior (e.g., YuNet landmark outputs, confidence scoring).
* Prototype threat evaluation logic (e.g., Eye Aspect Ratio blink thresholds, micro-motion variance) without requiring an incremental C++ compile cycle.
* Provide an offline algorithmic baseline for automated sanity checks and test assertions.

---

## 2. Production Status & Separation

* **Production Binary**: The shipping application is compiled from C++20 source files in `src/` and `include/` into `blindside_daemon` (or `blindside_daemon.exe` on Windows).
* **Zero Production Dependency**: Neither Python, `opencv-python`, nor `numpy` are required to build or run the production C++ daemon.
* **Scope**: The code in this directory is restricted to developer evaluation, algorithm prototyping, and reference testing.

---

## 3. Algorithms Validated by This Harness

* **YuNet ONNX Face Detection**: Loading `models/face_detection_yunet_2023mar.onnx` and interpreting 5 facial landmarks (right eye, left eye, nose, right mouth, left mouth).
* **6D Pose Estimation (`solvePnP`)**: Projecting canonical 3D facial points against 2D landmarks via `cv2.solvePnP` (`SOLVEPNP_EPNP`), converting rotation vectors to Euler angles (pitch, yaw, roll), and calculating 3D unit gaze vectors.
* **Gaze Tolerance Bounds**: Testing yaw/pitch bounds ($\pm 25^\circ$ yaw, $\pm 20^\circ$ pitch) to verify whether head pose is directed toward the screen.
* **Eye Aspect Ratio (EAR) & Blink Calculation**: Ratio of vertical eye-nose distance to interocular distance to detect blinks.
* **Liveness Anti-Spoofing Baseline**: Variance of pitch/yaw micro-movements combined with minimum EAR blink detection over a rolling temporal window to reject static printed photos.

---

## 4. Known Differences from Production C++20 Daemon

While this harness provides algorithmic parity for isolated mathematical calculations, it intentionally differs from the production C++ daemon in critical ways:

| Dimension | Production C++20 Daemon (`src/`, `include/`) | Python Reference Harness (`tools/reference/`) |
| :--- | :--- | :--- |
| **Language & Toolchain** | ISO C++20 (`cmake`, MSVC / GCC / Clang). | Python 3.10+ (`opencv-python`, `numpy`). |
| **Pipeline Architecture** | Decoupled multithreaded pipeline (`std::jthread`, `std::stop_token`) with lock-free `RingBuffer<RawFrame, 4>`. Congestion drops oldest frame. | Synchronous single-threaded execution loop. |
| **Memory Allocation** | Zero heap allocations inside the hot frame-processing loop (`RingBuffer`, static rolling history array). | Dynamic Python object allocation on every frame and detection. |
| **Multi-Face Tracking** | `SecondaryFaceTrack` tracking table with persistent IDs, hit/miss counters, and 1.0s timeout pruning. | Raw per-frame detection iteration without temporal track persistence. |
| **Primary User Association** | Compares against calibrated baseline bounded by `primary_box_tolerance`. Declares user absent if out of bounds. | Unconditional `min()` distance selection without a maximum tolerance radius. |
| **Windows Platform Response** | Native Win32 `CreateWindowEx` with true Desktop Window Manager (DWM) hardware blur (`DwmEnableBlurBehindWindow`). | Basic alpha-layered transparent window (`SetLayeredWindowAttributes`). No DWM blur. |
| **Hardware Throttling** | Dynamic hardware capture frame-rate throttling (30 FPS active $\leftrightarrow$ 5 FPS idle). | Static frame delay simulation. |

---

## 5. Usage

To run the reference harness:
```bash
# Diagnostic capability report
python tools/reference/run_blindside.py --diagnostics

# Headless synthetic simulation demo (no webcam required)
python tools/reference/run_blindside.py --synthetic --trigger-mode soft

# Run reference test suite
python tools/reference/test_blindside.py
```
