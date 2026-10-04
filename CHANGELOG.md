# Changelog

All notable changes to the Blindside project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased] - V4 Development (Phase 6 Platform Hardening)

### Added
- Discrete platform capability abstraction (`PlatformCapabilities`) and diagnostics struct (`PlatformDiagnostics`).
- Native Win32 `WM_NCHITTEST -> HTTRANSPARENT` pass-through handler for reliable click-through behavior.
- Support for negative multi-monitor coordinate spaces via virtual desktop bounds (`SM_XVIRTUALSCREEN`, `SM_YVIRTUALSCREEN`).
- Non-blocking event pump interface (`pump_events()`) preventing UI thread starvation across Windows (`PeekMessageA`) and Linux X11 (`XPending`).
- Dedicated 10-test platform hardening suite (`tests/test_platform.cpp`) registered under CTest.
- Developer Python reference implementation and test suite (`tools/reference/`).

### Changed
- Clarified redaction vs. blur distinction across all platforms; explicitly reports `targeted_blur = false`.
- Updated Windows privacy overlay styles to include `WS_EX_NOACTIVATE` to prevent keyboard focus stealing.
- Enhanced Linux X11 overlay with idempotent resizing via `XMoveResizeWindow` without leaking window handles.
- Hardened Linux Wayland backend to declare `screen_redaction = false` and cleanly degrade to notifications and session lock.
- Clarified diagnostic output indicating that X11 `monitor_count` reflects X11 screens rather than physical XRandR heads.

### Fixed
- Fixed Win32 overlay freezing caused by unserviced desktop window manager messages.
- Fixed overlay displacement on secondary displays positioned above or to the left of the primary monitor.
- Fixed potential duplicate window creation on rapid consecutive threat events.

### Known Limitations
- Native C++ compilation and CTest execution require a local C++20 toolchain (Visual Studio 2022 or GCC 11+).
- Pure Wayland compositors prohibit client-directed absolute screen overlays; fallback to notification and session lock is enforced.

---

## [3.1.0] - 2026-09-20

### Added
- Explicit distinction between raw candidate face detections and validated primary/secondary faces in standard logging.
- Comprehensive frame-by-frame face count diagnostics.
- Camera recovery integration preventing busy-loop starvation on GStreamer / OpenCV device reconnect.

### Changed
- Refined 6D `cv::solvePnP` spatial gaze vector bounds.
- Preserved strict 1.0-second gaze hysteresis threshold.

### Fixed
- GStreamer capture segmentation fault during abrupt hardware webcam disconnection.
- Unhandled camera acquisition race condition on Linux V4L2 drivers.
